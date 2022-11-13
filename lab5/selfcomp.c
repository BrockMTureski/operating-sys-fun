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
char compromise[120] = {
    0x00,
};

// string variable to probe the stack and find the correct
// values for the shell code.
char * compromise1 =
    "xxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxx"
    "MNOPWXYZ"
    "xxxxxxxx";

void doTest(){
    char buffer[136];
    int i;
    /* copy the contents of the compromise
       string onto the stack
       - change compromise1 to compromise
         when shell code is written */
    for (i = 0; compromise1[i]; i++){
	buffer[i] = compromise1[i];
    }
}

