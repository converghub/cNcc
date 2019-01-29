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
    TK_RETURN,      // "return"
    TK_SIZEOF,      // "sizeof"
    TK_IDENT,       // Identifier
    TK_INT,         // "int"
    TK_IF,          // if
    TK_ELSE,        // else
    TK_WHILE,       // while
    TK_FOR,         // for
    TK_EQ,          // ==
    TK_NE,          // !=
    TK_LOR,         // ||
    TK_LAND,        // &&
    TK_EOF,         // End of input
};

typedef struct {
    int ty;
    int val;
    char *name;     // Identifier
    char *input;
} Token;


// Pointer
enum {
    INT,
    PTR,
    ARY,
};

typedef struct Type {
    int ty;

    // Pointer
    struct Type *ptrof;

    // Array
    struct Type *aryof;
    int len;
} Type;


// Var
typedef struct {
  Type *cty;
  int offset;
} Var;


// Node
enum {
    ND_NUM = 256,   // Integer
    ND_RETURN,      // Return
    ND_SIZEOF,      // Sizeof
    ND_IDENT,       // Identifier
    ND_VAR_DEF,     // Variable definition
    ND_IF,          // If
    ND_WHILE,       // While
    ND_FOR,         // For
    ND_DEREF,       // Pointer dereference : *
    ND_ADDR,        // Address-of operater : &
    ND_CMPD_STMT,   // Compound statement
    ND_FUNC_CALL,   // Function call
    ND_FUNC_DEF,    // Function definition
    ND_EQ,          // Equal operation : ==
    ND_NE,          // Not-equal operation : !=
    ND_LOR,         // Logical OR operation : ||
    ND_LAND,        // Logical AND operation : &&
};

typedef struct Node {
    int ty;              // Node type
    Type *cty;           // C type
    struct Node *lhs;
    struct Node *rhs;
    int val;             // for ND_NUM
    char *name;          // for ND_IDENT
    struct Node *expr;   // for ND_RETURN, ND_DEREF
    Vector *stmts;       // for ND_CMPD_STMT

    // Function
    Vector *args;        // for Function call arguments
    struct Node *body;   // for Function defenition
    int stacksize;       // for Function stacksize

    struct Node *init;
    struct Node *inc;

    // If
    struct Node *bl_expr;
    struct Node *tr_stmt;
    struct Node *els_stmt;
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
Type *ptr_of (Type *base);
int size_of(Type *ctype);
Type *ary_of(Type *base, int len);

// codegen.c
void gen(Node *node, ...);
void gen_code(Vector *code);