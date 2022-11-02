#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/stat.h>

//+
// File:	shell.c
//
// Pupose:	This program implements a simple shell program. It does not start
//		processes at this point in time. However, it will change directory
//		and list the contents of the current directory.
//
//		The commands are:
//		   cd name -> change to directory name, print an error if the directory doesn't exist.
//		              If there is no parameter, then change to the home directory.
//		   ls -> list the entries in the current directory.
//			      If no arguments, then ignores entries starting with .
//			      If -a then all entries
//		   pwd -> print the current directory.
//		   exit -> exit the shell (default exit value 0)
//				any argument must be numeric and is the exit value
//
//		if the command is not recognized an error is printed.
//-

#define CMD_BUFFSIZE 1024
#define MAXARGS 10

int splitCommandLine(char * commandBuffer, char* args[], int maxargs);
int doInternalCommand(char * args[], int nargs);
int doProgram(char * args[], int nargs);

//+
// Function:	main
//
// Purpose:	The main function. Contains the read
//		eval print loop for the shell.
//
// Parameters:	(none)
//
// Returns:	integer (exit status of shell)
//-

int main() {

    char commandBuffer[CMD_BUFFSIZE];
    // note the plus one, allows for an extra null
    char *args[MAXARGS+1];

    int internal, program;

    // print prompt.. fflush is needed because
    // stdout is line buffered, and won't
    // write to terminal until newline
    printf("%%> ");
    fflush(stdout);

    while(fgets(commandBuffer,CMD_BUFFSIZE,stdin) != NULL){
        printf("%s",commandBuffer);

	// remove newline at end of buffer
	int cmdLen = strlen(commandBuffer);
	if (commandBuffer[cmdLen-1] == '\n'){
	    commandBuffer[cmdLen-1] = '\0';
	    cmdLen--;
            printf("<%s>\n",commandBuffer);
	}

	// split command line into words.(Step 2)
    int nargs = splitCommandLine(commandBuffer, args, MAXARGS);
	

	// add a null to end of array (Step 2)
    args[nargs+1] = '\0';
	

	// debugging
	printf("%d\n", nargs);
	//int i;
	//for (i = 0; i < nargs; i++){
	//   printf("%d: %s\n",i,args[i]);
	//}
	// element just past nargs
	//printf("%d: %x\n",i, args[i]);

        
        if (nargs>1){
            internal = doInternalCommand(args, nargs);
        }
        
        // TODO: if doInternalCommand returns 0, call doProgram  (Step 4)
        if (internal == 0){
            program = doProgram(args, nargs);
        }
        // TODO: if doProgram returns 0, print error message (Step 3 & 4)
        // that the command was not found.
        if(program == 0){
            printf("Error, command was not found.\n");
        }

	// print prompt
	printf("%%> ");
	fflush(stdout);
    }
    return 0;
}

////////////////////////////// String Handling (Step 1) ///////////////////////////////////

//+
// Function:	skipChar
//
// Purpose:	Called by splitCommandLine to return pointers to the first
//          character of a word following a space. 
//
// Parameters:
//    charPtr	Pointer to string
//    skip	character to skip
//
// Returns:	Pointer to first character after skipped chars
//		ID function if the string doesn't start with skip,
//		or skip is the null character
//-

char * skipChar(char * charPtr, char skip){
    if (skip == '\0'){
        return charPtr;
    }
    while(*charPtr == skip){
        charPtr++;
    }

    return charPtr;
}

//+
// Funtion:	splitCommandLine
//
// Purpose:	Fills array args[] with pointers to the beginning of words following
//          a space. Calls skipChar to get pointers.
//
// Parameters:
//	commandBuffer   pointer to the beginning of array 
//  args[]          array of character pointers pointing to words in commandBuffer
//  maxargs         maximum number of arguments allowed in args[] 
//
// Returns:	Number of arguments (< maxargs).
//
//-

int splitCommandLine(char * commandBuffer, char* args[], int maxargs){
   int f = 0;
    int len = strlen(commandBuffer);
    for(int i = 0; i<len;i++){
        if(commandBuffer[i] == ' ' && f < maxargs){
            args[f]=skipChar(commandBuffer,' ');
            f++;
        }
    }
   return f+1;
}

////////////////////////////// External Program  (Note this is step 4, complete doeInternalCommand first!!) ///////////////////////////////////

// list of directorys to check for command.
// terminated by null value
char * path[] = {
    ".",
    "/usr/bin",
    NULL
};

//+
// Funtion:	doProgram
//
// Purpose:	
//
// Parameters:
//	TODO: add paramters and description
//
// Returns	int
//		1 = found and executed the file
//		0 = could not find and execute the file
//-

int doProgram(char * args[], int nargs){
  // find the executable
  // TODO: add body.
  // Note this is step 4, complete doInternalCommand first!!!
  int i = 0;
  int pathElement = 0;
  int commandLength = 0;
  char *cmd_path = NULL;
   
  int length = strlen(*path);
  for(i=0; i<length; i++){
    
    pathElement = strlen(path[i]);
    commandLength = strlen(args[0]);
    char * cmd_path=malloc(pathElement+commandLength+2);
    
    sprintf(cmd_path,"%s/%s", path[i], args[0]);
    struct stat stats;

    if(stat(cmd_path, &stats)==0){
        if(!(S_ISREG(stats.st_mode))){
            printf("Error from S_ISREG, irregular");
            return 0;
        }
        if(!(S_IXUSR && stats.st_mode)){
            printf("Error from S_IXUSR, unexecutable");
            return 0;
        }
        break;
    }

    free(cmd_path);
    cmd_path = NULL;
  }
  
  if(cmd_path == NULL){
      return 0;
  }

  pid_t outFork = fork();
  if(outFork == -1){
      perror("Child not created");
  }
  if(outFork == 0){
      execv(cmd_path, args);
  }

  free(cmd_path);
  wait(NULL);

  return 1;
}

////////////////////////////// Internal Command Handling (Step 3) ///////////////////////////////////

// define command handling function pointer type
typedef void(*commandFunc)(char * args[], int nargs);

// associate a command name with a command handling function
struct cmdStruct{
   char 	* cmdName;
   commandFunc 	cmdFunc;
};

// prototypes for command handling functions
// TODO: add prototype for each comamand function
void exitFunc (char *args[], int nargs);
void pwdFunc (char *args[], int nargs);
void lsFunc (char *args[], int nargs);
void cdFunc (char *args[], int nargs);
int checkFilter(const struct dirent *d);

// list commands and functions
// must be terminated by {NULL, NULL} 
// in a real shell, this would be a hashtable.
struct cmdStruct commands[] = {
   {"exit", exitFunc},
   {"pwd", pwdFunc},
   {"ls", lsFunc},
   {"cd", cdFunc},
   { NULL, NULL}		// terminator
};

//+
// Function:	doInternalCommand
//
// Purpose:	TODO: add description
//
// Parameters:
//	TODO: add parameter names and descriptions
//
// Returns	int
//		1 = args[0] is an internal command
//		0 = args[0] is not an internal command
//-

int doInternalCommand(char * args[], int nargs){
    
    int i;
    while (commands[i].cmdName != NULL){
        if((strcmp(args[0], commands[i].cmdName))==0){
            commands[i].cmdFunc(args, nargs);
            return 1;
        }
        i++;
    }
    return 0;
}

///////////////////////////////
// comand Handling Functions //
///////////////////////////////


//+
// Function:	exitFunc
//
// Purpose:	Exit the shell with exit code of 0
//
// Parameters:
//	TODO: add parameter names and descriptions
//
// Returns:	void
//-
void exitFunc (char *args[], int nargs){
    exit(0);
}

//+
// Function:	pwdFunc
//
// Purpose:	Print the current working directory 
//
// Parameters:
//	TODO: add parameter names and descriptions
//
// Returns:	void
//-
void pwdFunc (char *args[], int nargs){
    char * cwd = getcwd(NULL, 0);
    printf("Current directory: %s\n", cwd);
    free(cwd);
}

//+
// Function:	lsFunc
//
// Purpose:	List the contents of the current directory. Hidden files 
//          are not listed. Parameter "-a" lists all files
//
// Parameters:
//	TODO: add parameter names and descriptions
//
// Returns: void	
//-
void lsFunc (char *args[], int nargs){
    struct dirent ** namelist;
    int (*filter)(const struct dirent * d); 
    int numEnts = scandir(".", &namelist, NULL, NULL);
    int numPrinted=0;


    if(nargs == 1){
        int numEnts = scandir(".", &namelist, filter, NULL);
        while(numPrinted!=numEnts){
            if(checkFilter(namelist[numPrinted])== 1){
                printf("%s\n", namelist[numPrinted]->d_name);
                numPrinted++;
            }
        }

    }

    if(nargs == 2){
         if(strcmp(args[1], "-a") == 0){
            int numEnts = scandir(".", &namelist, NULL, NULL);
            for(int i=0; i<numEnts; i++){
                printf("%s\n",namelist[i]->d_name);
            }
        }else{
            fprintf(stderr, "Error, argument \'-a\' not present.");
        }
    }
    return; 
}

//+
// Function:	checkFilter
//
// Purpose:	Exit the shell with exit code of 0
//
// Parameters:
//	TODO: add parameter names and descriptions
//
// Returns: int	
//		1 = not a hidden file
//		0 = hidden file
//-
int checkFilter(const struct dirent *d){
    if(d->d_name[0]=='.'){
        return 0;
    }else{
        return 1;
    }

}

//+
// Function:	cdFunc
//
// Purpose:	Change the current directory to the one given by parameter.
//
// Parameters:
//	TODO: add parameter names and descriptions
//
// Returns: void
//-
void cdFunc (char *args[], int nargs){
    struct passwd *pw=getpwuid(getuid());
   
    if(pw==NULL){
        fprintf(stderr, "Invalid input: NULL\n");  
    }

    char* newDirName = pw->pw_dir;
    if(nargs == 1){
        chdir(newDirName);
    }

    if(chdir(newDirName)!=0){
        fprintf(stderr, "Directory does not exist\n");
    }
             
}