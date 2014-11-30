// process.c                                      Daniel Kim (11/29/14)
//
// Backend for Bsh.  See spec for details.

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include "/c/cs323/Hwk6/parse.h"

// Print error message and die with STATUS
#define errorExit(msg, status)  perror(msg), exit(status)
#define error_Exit(msg, status)  perror(msg), _exit(status)

// Execute command list CMDLIST and return status of last command executed
void process (CMD *cmdList)
{
    CMD *pcmd = cmdList;
    int pid, status;    // fork(), wait()
    int fd[2];          // Read and write file descriptors for pipe()

    // SIMPLE
    if (pcmd->type == SIMPLE) {
        

        if ((pid = fork()) < 0)
            errorExit("SIMPLE",EXIT_FAILURE);

        else if (pid == 0) {     // child

            // RED_IN
            if (pcmd->fromType == RED_IN) {
                int in = open(pcmd->fromFile, O_RDONLY);
                if (in == -1) 
                    error_Exit(pcmd->fromFile,EXIT_FAILURE);

                dup2(in, 0);
                close(in);
            }
            
            // RED_OUT, RED_APP
            if (pcmd->toType != NONE) {
                int obits = O_CREAT | O_WRONLY;
                if (pcmd->toType == RED_APP)
                    obits = obits | O_APPEND;

                int out = open(pcmd->toFile, obits, 0644);

                if (out == -1)
                    error_Exit(pcmd->toFile,EXIT_FAILURE);

                dup2(out, 1);
                close(out);

            }
            
            // Run command
            execvp(*(pcmd->argv), pcmd->argv);
            error_Exit(*(pcmd->argv),EXIT_FAILURE);
        }
        else {                   // parent
            wait(&status); // **************CHANGE TO WAITPID IF MORE THAN 1 CHILD POSSIBLE
        }
    }

    // PIPE
    else if (pcmd->type == PIPE) {
        
        if ((pid = fork()) < 0)
            errorExit("PIPE",EXIT_FAILURE);

        else if (pid == 0) {          // child

            if (pipe(fd) == -1)
                error_Exit("pipe",EXIT_FAILURE);




    }

}
