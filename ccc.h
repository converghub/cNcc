#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>


// Vector
typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;



// Map
typedef struct {
    Vector *keys;
    Vector *vals;
} Map;


// Token
enum {
    TK_NUM = 256,   // Integer
    TK_RETURN,      // Return
    TK_IDENT,       // Identifier
    TK_IF,          // if
    TK_EQ,          // Equal : ==
    TK_NE,          // Not-equal : !=
    TK_EOF,         // End of input
};

typedef struct {
    int ty;
    int val;
    char *name;     // Identifier
    char *input;
} Token;


// Node
enum {
    ND_NUM = 256,   // Integer
    ND_RETURN,      // Return
    ND_IDENT,       // Identifier
    ND_IF,          // If
    ND_CMPD_STMT,   // Compound statement
    ND_FUNC_CALL,   // Function call
    ND_FUNC_DEF,    // Function definition
    ND_EQ,          // Equal operation : ==
    ND_NE,          // Not-equal operation : !=
};

typedef struct Node {
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;             // for ND_NUM
    char *name;          // for ND_IDENT
    struct Node *expr;   // for ND_RETURN
    Vector *stmts;       // for ND_CMPD_STMT

    // Function
    Vector *args;        // for Function call arguments
    struct Node *body;   // for Function defenition

    // If
    struct Node *bl_expr;
    struct Node *tr_stmt;
} Node;



// parse.c
Vector *parse(Vector *tk);

// token.c
Vector *tokenize(char *p);

// container.c
void error(char *fmt, ...);
void run_test();
Vector *new_vector();
void vec_push(Vector *vec, void *elem);
Map *new_map();
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);
bool map_exist(Map *map, char *key);


// codegen.c
void gen(Node *node, ...);
void gen_code(Vector *code);