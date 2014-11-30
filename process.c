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
    int pid, status;

    // SIMPLE
    if (pcmd->type == SIMPLE) {
        
        if ((pid = fork()) < 0)
            errorExit("simple",EXIT_FAILURE);

        else if (pid == 0) {     // child
            execvp(*(pcmd->argv), pcmd->argv);
            error_Exit(*(pcmd->argv),EXIT_FAILURE);
        }
        else {                   // parent
            wait(&status);
        }
    }

    // 

}
