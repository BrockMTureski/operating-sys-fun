#include <stdio.h>

int main(int argc, char * argv[]){
    struct passwd *pw = getpwuid(getuid());
    printf("Hello %s\n", pw.pw_name);
}
