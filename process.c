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

// Set $? to "STATUS"
static void set_status (int status)
{
    char str[20];
    sprintf(str, "%d", status);
    setenv("?",str,1);
}

// Execute command list CMDLIST and return status of last command executed
void process (CMD *cmdList)
{
    CMD *pcmd = cmdList;
    int pid, status;    // fork(), wait()
    int fd[2];          // Read and write file descriptors for pipe()

    // SIMPLE
    //
    // SET VARIABLE********************
    if (pcmd->type == SIMPLE) {
        
        // cd
        if (strcmp(*(pcmd->argv),"cd") == 0) {
            
            if (pcmd->argc > 2) {
                fprintf(stderr, "usage: cd  OR  cd <directory-name>\n");
                set_status(1);
                return;
            }

            else {

                // set DIR to directory
                char *dir; 
                if (pcmd->argc == 1) {
                    dir = getenv("HOME");
                    if (dir == NULL) {
                        fprintf(stderr, "cd: $HOME variable not set\n");
                        set_status(1);
                        return;
                    }
                }
                else if (pcmd->argc == 2)
                    dir = *((pcmd->argv)+1);

                if (chdir(dir) == -1) {
                    perror("cd: chdir failed");
                    set_status(errno);
                }
                else // success
                    set_status(0);

                return;
            }
        }
        
        // wait *************STILL NEED TO WRITE "COMPLETED:" etc
        // TEST LATER
        else if (strcmp(*(pcmd->argv),"wait") == 0) {

            if (pcmd->argc > 1) {
                fprintf(stderr, "usage: wait\n");
                set_status(1);
                return;
            }

            else {
                errno = 0;
                while ((pid = waitpid(-1,NULL,0))) { // stay until children done
                    if (errno == ECHILD)
                        break;
                }
            }
        }

        // Other commands (dirs, external)
        else {
            if ((pid = fork()) < 0)
                errorExit("SIMPLE",errno);

            else if (pid == 0) {     // child
                
                // RED_IN
                if (pcmd->fromType == RED_IN) {
                    int in = open(pcmd->fromFile, O_RDONLY);
                    if (in == -1) 
                        error_Exit(pcmd->fromFile,errno);

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
                        error_Exit(pcmd->toFile,errno);

                    dup2(out, 1);
                    close(out);

                }
                
                // Set local variables only in this child process
                if (pcmd->nLocal > 0) {
                    for (int i = 0; i < pcmd->nLocal; i++) 
                        setenv(*((pcmd->locVar)+i), *((pcmd->locVal)+i), 1);
                }


                // Built-in command? (dirs)

                // Run external command
                execvp(*(pcmd->argv), pcmd->argv);
                error_Exit(*(pcmd->argv),errno);
            }
            else {                   // parent
                waitpid(pid, &status, 0); // **************CHANGE TO WAITPID IF MORE THAN 1 CHILD POSSIBLE

                // Set $? to status
                int program_status = (WIFEXITED(status) ? WEXITSTATUS(status) : 128+WTERMSIG(status));
                set_status(program_status);
                //close(0);
            }
        }
    }
    
    // PIPE
    // need to do status
    else if (pcmd->type == PIPE) {
        
        if (pipe(fd) == -1)
            errorExit("pipe",EXIT_FAILURE); // CHANGE TO STATUS

        if ((pid = fork()) < 0)
            errorExit("PIPE",EXIT_FAILURE);

        else if (pid == 0) {          // child
        
            close(fd[0]);      // no reading from new pipe

            if (fd[1] != 1) {
                dup2(fd[1],1); // left <stage> write to buffer
                close(fd[1]);
            }

            process(pcmd->left);
        }

        else {                        // parent

            if (fd[0] != 0) {         // read from new pipe
                dup2(fd[0],0);
                close(fd[0]);
            }

            close(fd[1]);             // no writing to new pipe
            process(pcmd->right);

        }
    }
    //wait(NULL);


    else if (pcmd->type == SUBCMD) {
    }

    // IF YOU PUT ANYTHING OUTSIDE THESE CONDITIONALS
    // MAKE SURE RETURN STATEMENTS ARE EVERYWHERE NECESSARY
        

}
