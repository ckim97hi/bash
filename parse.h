// parse.h                                        Stan Eisenstat (11/07/14)
//
// Header file for command line parser used in Parse
//
// Bash version based on recursive descent parse tree

#ifndef PARSE_INCLUDED
#define PARSE_INCLUDED          // parse.h has been #include-d

/////////////////////////////////////////////////////////////////////////////

// A token is one of
//  1) a maximal contiguous sequence of nonblank printing characters other
//       than the metacharacters <, >, ;, &, |, (, and ).
//  2) a redirection symbol (<, <<, >, >>, or |);
//  3) a command separator (;, &, &&, or ||)
//  4) a left or right parenthesis (used to group commands).
//  The token type is specified by the symbolic constants defined below.

#define METACHARS       "<>;&|()"       // Chars that start a non-SIMPLE

// Token types used by lex() and parse()
enum {
   // Token types used by lex()
      SIMPLE,           // Maximal contiguous sequence ... (as above)
      RED_IN,           // <
      RED_OUT,          // >
      RED_APP,          // >>
      RED_PIPE,         // |
      SEP_AND,          // &&
      SEP_OR,           // ||
      SEP_END,          // ;
      SEP_BG,           // &
      PAR_LEFT,         // (
      PAR_RIGHT,        // )

   // Token types used by parse()
      NONE,             // Nontoken: Did not find a token
      ERROR,            // Nontoken: Encountered an error
      PIPE,             // Nontoken: CMD struct for pipeline
      SUBCMD            // Nontoken: CMD struct for subcommand
};

/////////////////////////////////////////////////////////////////////////////

// A token list is a headless linked list of typed tokens.  All storage is
// allocated by malloc() / realloc().
typedef struct token {          // Struct for each token in linked list
  char *text;                   //   String containing token
  int type;                     //   Corresponding type
  struct token *next;           //   Pointer to next token in linked list
} token;

// lex() breaks the string LINE into a headless linked list of typed tokens
// and returns a pointer to the first token (or NULL if none were found).
token *lex (const char *line);

/////////////////////////////////////////////////////////////////////////////

// The syntax for a command is
//
//   <stage>    = <simple> / (<command>)
//   <pipeline> = <stage> / <stage> | <pipeline>
//   <and-or>   = <pipeline> / <pipeline> && <and-or> / <pipeline> || <and-or>
//   <list>     = <and-or> / <and-or> ; <command> / <and-or> & <command>
//   <command>  = <list> / <list> ; / <list> &
//
// where a <simple> is a single command with arguments but no |, &&, ||, ;, &,
// (, or ).  This grammar also captures the precedence of these operators.
//
// Note that I/O redirection is associated with a <stage> (i.e., a <simple> or
// subcommand), but not with a <pipeline> (input/output redirection for the
// first/last stage is associated with the stage, not the pipeline).
//
// The parse() function translates the sequence of tokens representing a
// command into a tree of CMD structs containing its <simple> commands and the
// "operators" |, &&, ||, ;, &, and SUBCMD.  The tree corresponds to how the
// command would be parsed by a recursive descent parser for the grammar above.
// (See the URL https://en.wikipedia.org/wiki/Recursive_descent_parser for a
// description )
//
// A CMD struct contains the following fields:

typedef struct cmd {

    int type;             // Node type (SIMPLE, PIPE, SUBCMD, SEP_AND, SEP_OR,
			  //   SEP_END, SEP_BG, or NONE)

    int nLocal;           // Number of local variable assignments
    char **locVar;        // Array of local variable names and values to assign
    char **locVal;        //   to them when command executes or NULL (default)

    int argc;             // Number of command-line arguments
    char **argv;          // Null-terminated argument vector or NULL (default)

    int fromType;         // Redirect stdin? (NONE (default) or RED_IN)
    char *fromFile;       // File to redirect stdin or NULL (default)

    int toType;           // Redirect stdout?  (NONE (default), RED_OUT, or
			  //   RED_APP)
    char *toFile;         // File to redirect stdout or NULL (default)

    struct cmd *left;     // Left subtree or NULL (default)
    struct cmd *right;    // Right subtree or NULL (default)
} CMD;
     
// The tree for a <simple> is a single struct of type SIMPLE that specifies its
// arguments (argc, argv[]); and whether and where to redirect its standard
// input (fromType, fromFile) and its standard output (toType, toFile).  The
// left and right children are NULL.
//
// The tree for a <stage> is either the tree for a <simple> or a CMD struct
// of type SUBCMD (which may have redirection) whose left child is the tree
// representing a <command> and whose right child is NULL.
//
// The tree for a <pipeline> is either the tree for a <stage> or a CMD struct
// of type PIPE whose left child is the tree representing the <stage> and whose
// right child is the tree representing the rest of the <pipeline>.
//
// The tree for an <and-or> is either the tree for a <pipeline> or a CMD struct
// of type && (= SEP_AND) or || (= SEP_OR) whose left child is the tree
// representing the <pipeline> and whose right child is the tree representing
// the <and-or>.
//
// The tree for a <list> is either the tree for an <and-or> or a CMD struct of
// type ; (= SEP_END) or & (= SEP_BG) whose left and right children are trees
// representing an <and-or>.
//
// The tree for a <command> is either the tree for a <list> or a CMD struct of
// type ; (= SEP_END) or & (= SEP_BG) whose left child is the tree representing
// a <list> and whose right child is NULL.
//
// Notes:  The type NONE is used only to initialize a CMD struct.  Only a
// SIMPLE or SUBCMD may have local variables or redirection.  Only a SIMPLE
// may have arguments.  A SIMPLE may not have subtrees; a PIPE, SEP_AND, or
// SEP_OR must have two subtrees; a SUBCMD must have only a left subtree; a
// SEP_END or SEP_BG must have a left subtree and may have a right subtree.
//
// Examples (where A, B, C, D, and E are <simple>):
//
// 1) < A B | C | D | E > F          PIPE                                    //
//                                  /    \                                   //
//                                B <A    PIPE                               //
//                                       /    \                              //
//                                      C      PIPE                          //
//                                            /    \                         //
//                                           D      E >F                     //
//                                                                           //
// 2) A && B || C && D &               &                                     //
//                                    /                                      //
//                                  &&                                       //
//                                 /  \                                      //
//                                A    ||                                    //
//                                    /  \                                   //
//                                   B    &&                                 //
//                                       /  \                                //
//                                      C    D                               //
//                                                                           //
// 3) A ; B & C ; D || E            ;                                        //
//                                 / \                                       //
//                                A   &                                      //
//                                   / \                                     //
//                                  B   ;                                    //
//                                     / \                                   //
//                                    C   ||                                 //
//                                       /  \                                //
//                                      D    E                               //
//                                                                           //
// 4) (A ; B &) | (C || D) && E                   &&                         //
//                                               /  \                        //
//                                           PIPE    E                       //
//                                          /    \                           //
//                                    SUB >|      <| SUB                     //
//                                   /           /                           //
//                                  ;          ||                            //
//                                 / \        /  \                           //
//                                A   &      C    D                          //
//                                   /                                       //
//                                  B                                        //

// Allocate, initialize, and return a pointer to an empty command structure
CMD *mallocCMD (void);

// Free the command structure CMD
void freeCMD (CMD *cmd);

// Parse a token list into a command structure and return a pointer to
// that structure (NULL if errors found).
CMD *parse (token *tok);

/////////////////////////////////////////////////////////////////////////////

// Print tree of CMD structs in in-order starting at LEVEL
void dumpTree (CMD *exec, int level);

// Print list of tokens LIST
void dumpList (struct token *list);

// Free list of tokens LIST (implemented in mainLex.c)
void freeList (token *list);

#endif
