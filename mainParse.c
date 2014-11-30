// mainParse.c                                    Stan Eisenstat (10/20/12)
//
// Prompts for commands, expands environment variables, parses them into
// command structures, and dumps the command structures to stdout.
//
// Bash version based on recursive descent parse tree
// Dumps token list or CMD tree if DUMP_LIST is set.

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "/c/cs323/Hwk4/getLine.h"
#include "parse.h"

int main()
{
    int nCmd = 1;                   // Command number
    char *line;                     // Initial command line
    token *list;                    // Linked list of tokens
    CMD *cmd;                       // Parsed command

    for ( ; ; ) {
	printf ("(%d)$ ", nCmd);                // Prompt for command
	fflush (stdout);
	if ((line = getLine (stdin)) == NULL)   // Read line
	    break;                              //   Break on end of file

	list = lex (line);                      // Lex line into tokens
	free (line);
	if (list == NULL) {
	    continue;
	} else if (getenv ("DUMP_LIST")) {      // Dump token list only if
	    dumpList (list);                    //   environment variable set
	    printf ("\n");
	}

	cmd = parse (list);                     // Parsed command?
	freeList (list);
	if (cmd == NULL) {
	    continue;
	}

	dumpTree (cmd, 0);
	freeCMD (cmd);                          // Free associated storage
	nCmd++;                                 // Adjust prompt

    }

    printf ("\n");                              // Add final newline
    return EXIT_SUCCESS;
}


// Allocate, initialize, and return a pointer to an empty command structure
CMD *mallocCMD (void)
{
    CMD *new = malloc(sizeof(*new));

    new->type     = NONE;
    new->argc     = 0;
    new->argv     = malloc (sizeof(char *));
    new->argv[0]  = NULL;
    new->nLocal   = 0;
    new->locVar   = NULL;
    new->locVal   = NULL;
    new->fromType = NONE;
    new->fromFile = NULL;
    new->toType   = NONE;
    new->toFile   = NULL;
    new->left     = NULL;
    new->right    = NULL;

    return new;
}


// Free list of commands CMDLIST
void freeCMD (CMD *cmdList)
{
    if (!cmdList)
	return;

    for (int i = 0; i < cmdList->nLocal; i++) {
	free (cmdList->locVar[i]);
	free (cmdList->locVal[i]);
    }
    free (cmdList->locVar);
    free (cmdList->locVal);

    for (char **p = cmdList->argv;  *p;  p++)
	free (*p);
    free (cmdList->argv);

    free (cmdList->fromFile);
    free (cmdList->toFile);

    freeCMD (cmdList->left);
    freeCMD (cmdList->right);

    free (cmdList);
}


// Free list of tokens LIST
void freeList (token *list)
{
    token *p, *pnext;
    for (p = list;  p;  p = pnext)  {
	pnext = p->next;  p->next = NULL;       // Zap p->next and p->text
	free(p->text);    p->text = NULL;       //   to stop accidental reuse
	free(p);
    }
}


// Print list of tokens LIST
void dumpList (struct token *list)
{
    struct token *p;

    for (p = list;  p != NULL;  p = p->next)    // Walk down linked list
	printf ("%s:%d ", p->text, p->type);    //   printing token and type
    putchar ('\n');                             // Terminate line
}


// Print arguments in command data structure rooted at *C
void dumpArgs (CMD *c)
{
    for (char **q = c->argv;  *q;  q++)
	fprintf (stdout, ",  argv[%ld] = %s", q-(c->argv), *q);
}


// Print input/output redirections in command data structure rooted at *C
void dumpRedirect (CMD *c)
{
    if (c->fromType == NONE && c->fromFile == NULL)
	;
    else if (c->fromType == RED_IN && c->fromFile != NULL)
	fprintf (stdout, "  <%s", c->fromFile);
    else
	fprintf (stdout, "  ILLEGAL INPUT REDIRECTION");

    if (c->toType == NONE && c->toFile == NULL)
	;
    else if (c->toType == RED_OUT && c->toFile != NULL)
	fprintf (stdout, "  >%s", c->toFile);
    else if (c->toType == RED_APP && c->toFile != NULL)
	fprintf (stdout, "  >>%s", c->toFile);
    else
	fprintf (stdout, "  ILLEGAL OUTPUT REDIRECTION");

    if (c->nLocal > 0) {
	fprintf (stdout, "\n         LOCAL: ");
	for (int i = 0; i < c->nLocal; i++)
	    fprintf (stdout, "%s=%s, ", c->locVar[i], c->locVal[i]);
    } else if (c->nLocal == 0) {
	;
    } else {
	fprintf (stdout, "  INVALID NLOCAL");
    }
}


// Print command data structure rooted at *C at level LEVEL
void dumpSimple (CMD *c, int level)
{
    fprintf (stdout, "level = %d,  argc = %d", level, c->argc);

    if (c->type == SIMPLE)
	dumpArgs (c);
    else if (c->type == PIPE)
	fprintf (stdout, ",  PIPE");
    else if (c->type == SUBCMD)
	fprintf (stdout, ",  SUBCMD");

    dumpRedirect (c);
}


// Print command data structure rooted at *C; return SEP_END or SEP_BG
int dumpType (CMD *c, int level)
{
    int type = SEP_END;

    if (c->argc < 0)
	fprintf (stdout, "  ARGC < 0");
    else if (c->argv == NULL)
	fprintf (stdout, "  ARGV = NULL");
    else if (c->argv[c->argc] != NULL)
	fprintf (stdout, "  ARGV[ARGC] != NULL");

    if (c->type == SIMPLE) {
	dumpSimple (c, level);
	if (c->left != NULL)
	    fprintf (stdout, "  <simple> HAS LEFT CHILD");
	if (c->right != NULL)
	    fprintf (stdout, "  <simple> HAS RIGHT CHILD");

    } else if (c->argc > 0
	    || c->argv == NULL
	    || c->argv[0] != NULL) {
	fprintf (stdout, "  INVALID ARGUMENT LIST IN NON-SIMPLE");

    } else if (c->type == SUBCMD) {
	dumpSimple (c, level);
	fprintf (stdout, "\nCMD:   ");
	type = dumpType (c->left, level+1);
	if (c->right)
	    fprintf (stdout, "  SUBCMD HAS RIGHT CHILD");
	char sep = (type == SEP_BG) ? '&' : ';';
	fprintf (stdout, "  %c", sep);
	type = SEP_END;

    } else if (c->fromType != NONE
	    || c->fromFile != NULL
	    || c->toType != NONE
	    || c->toFile != NULL) {
	fprintf (stdout, "  INVALID I/O REDIRECTION IN NON-SIMPLE NON-SUBCMD");

    } else if (c->type == PIPE) {
	dumpSimple (c, level);
	fprintf (stdout, "\nCMD:   ");
	type = dumpType (c->left, level+1);
	fprintf (stdout, "  |\nCMD: | ");

	CMD *p;
	for (p = c->right; p->type == PIPE; p = p->right) {
	    type = dumpType (p->left, level+1);
	    fprintf (stdout, "  |\nCMD: | ");
	}
	type = dumpType (p, level+1);
	char sep = (type == SEP_BG) ? '&' : ';';
	fprintf (stdout, "  %c", sep);
	type = SEP_END;

    } else if (c->type == SEP_AND) {
	type = dumpType (c->left, level);
	fprintf (stdout, "  &&\nCMD:   ");
	type = dumpType (c->right, level);

    } else if (c->type == SEP_OR) {
	type = dumpType (c->left, level);
	fprintf (stdout, "  ||\nCMD:   ");
	type = dumpType (c->right, level);

    } else if (c->type == SEP_END) {
	type = dumpType (c->left, level);
	if (c->right) {
	    char sep = (type == SEP_BG) ? '&' : ';';
	    fprintf (stdout, "  %c\nCMD:   ", sep);
	    type = dumpType (c->right, level);
	} else {
	    fprintf (stdout, "  SEP_END MISSING RIGHT CHILD");
	}

    } else if (c->type == SEP_BG) {
	dumpType (c->left, level);
	type = SEP_BG;
	if (c->right) {
	    fprintf (stdout, "  &\nCMD:   ");
	    type = dumpType (c->right, level);
	}

    } else {
	fprintf (stdout, "  ILLEGAL CMD TYPE");
    }

    return type;
}


// Print command data structure rooted at *C
void dumpCMD (CMD *c, int level)
{
    fprintf (stdout, "CMD:   ");
    int type = dumpType (c, level);
    char sep = (type == SEP_BG) ? '&' : ';';
    fprintf (stdout, "  %c\n", sep);
}


// Print in in-order command data structure rooted at *C at depth LEVEL
void dumpTree (CMD *c, int level)
{
    if (!c)
	return;

    dumpTree (c->left, level+1);

    fprintf (stdout, "CMD (Depth = %d):  ", level);
    if (c->type == SIMPLE) {
	fprintf (stdout, "SIMPLE");
	dumpArgs (c);
	dumpRedirect (c);
    } else if (c->type == SUBCMD) {
	fprintf (stdout, "SUBCMD");
	dumpRedirect (c);
    } else if (c->type == PIPE) {
	fprintf (stdout, "PIPE");
    } else if (c->type == SEP_AND) {
	fprintf (stdout, "SEP_AND");
    } else if (c->type == SEP_OR) {
	fprintf (stdout, "SEP_OR");
    } else if (c->type == SEP_END) {
	fprintf (stdout, "SEP_END");
    } else if (c->type == SEP_BG) {
	fprintf (stdout, "SEP_BG");
    } else {
	fprintf (stdout, "NONE");
    }
    fprintf (stdout, "\n");

    dumpTree (c->right, level+1);
}
