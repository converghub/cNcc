#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>


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
    TK_STR,         // String
    TK_RETURN,      // "return"
    TK_SIZEOF,      // "sizeof"
    TK_ALIGNOF,     // "_Alignof"
    TK_IDENT,       // Identifier
    TK_EXTERN,      // "extern"
    TK_INT,         // "int"
    TK_CHAR,        // "char"
    TK_STRUCT,      // "struct"
    TK_IF,          // "if"
    TK_ELSE,        // "else"
    TK_DO,          // "do"
    TK_WHILE,       // "while"
    TK_FOR,         // "for"
    TK_EQ,          // ==
    TK_NE,          // !=
    TK_LE,          // <=
    TK_GE,          // >=
    TK_LOR,         // ||
    TK_LAND,        // &&
    TK_SHL,         // <<
    TK_SHR,         // >>
    TK_INC,         // ++
    TK_DEC,         // --
    TK_ARROW,       // ->
    TK_EOF,         // End of input
};

typedef struct {
    int ty;
    int val;
    // String literal
    char *str;

    // Identifier
    char *name;     
    char *input;
} Token;


// Pointer
enum {
    INT,
    CHAR,
    PTR,
    ARY,
    STRUCT,
};

typedef struct Type {
    int ty;
    int size;
    int align;

    // Pointer
    struct Type *ptrto;

    // Array
    struct Type *aryof;
    int len;

    // Struct
    Vector *mbrs;
    int offset;
} Type;


// Var
typedef struct {
    Type *cty;
    bool is_local;

    // for local
    int offset;

    // for global
    char *name;
    bool is_extern;
    char *data;
    int len;
} Var;


// Node
enum {
    ND_NUM = 256,   // Integer
    ND_STR,         // String literal
    ND_RETURN,      // return
    ND_SIZEOF,      // sizeof
    ND_ALIGNOF,     // alignof
    ND_IDENT,       // Identifier
    ND_STRUCT,      // Struct
    ND_VAR_DEF,     // Variable definition
    ND_LVAR,        // Local variable
    ND_GVAR,        // Global variable
    ND_IF,          // if
    ND_WHILE,       // while
    ND_DO_WHILE,    // do ~ while
    ND_FOR,         // for
    ND_DEREF,       // Pointer dereference : *
    ND_ADDR,        // Address-of operater : &
    ND_DOT,         // Struct member access
    ND_EXPR_STMT,   // Expression statement
    ND_STMT_EXPR,   // Statement expression
    ND_CMPD_STMT,   // Compound statement
    ND_FUNC_CALL,   // Function call
    ND_FUNC_DEF,    // Function definition
    ND_EQ,          // Equal operation : ==
    ND_NE,          // Not-equal operation : !=
    ND_LE,          // : <=
    ND_GE,          // : >=
    ND_LOR,         // Logical OR operation : ||
    ND_LAND,        // Logical AND operation : &&
    ND_SHL,         // : <<
    ND_SHR,         // : >>
    ND_PRE_INC,     // pre ++
    ND_PRE_DEC,     // pre --
    ND_POST_INC,    // post ++
    ND_POST_DEC,    // post --
    ND_NULL,        // Null statement
};

typedef struct Node {
    int ty;                 // Node type
    Type *cty;              // C type
    struct Node *lhs;
    struct Node *rhs;
    int val;                // for ND_NUM
    char *str;              // for ND_STR
    char *name;             // for ND_IDENT
    struct Node *expr;      // for ND_RETURN, ND_DEREF, ND_SIZEOF
    struct Node *stmt_expr; // for ND_STMT_EXPR
    bool is_last;           // for ND_STMT_EXPR, comma operation ',' 
    Vector *stmts;          // for ND_CMPD_STMT
    struct Node *pfunc;     // pointer to parent function
    struct Node *upper;     // pointer to upper node
    bool no_push;           // push or not

    // Function
    Vector *args;           // for Function call arguments
    struct Node *body;      // for Function defenition
    int stacksize;          // for Function stacksize
    Vector *strings;        // for String literal

    struct Node *init;
    struct Node *inc;

    // Global variable
    bool is_extern;
    char *data;
    int len;

    // Local variable, beginning of Struct
    int offset;

    // Struct
    Vector *mbrs;
    // Struct member
    char *mbr_name;

    // If
    struct Node *bl_expr;
    struct Node *tr_stmt;
    struct Node *els_stmt;
} Node;


// parse.c
Vector *parse(Vector *tk);

// token.c
Vector *tokenize(char *p);

// semantic_analysis.c
Vector *sema(Vector *nodes, Vector *global_vars);

// container.c
void error(char *fmt, ...);
char *format(char *fmt, ...);
void run_test();
Vector *new_vector();
void vec_push(Vector *vec, void *elem);
Map *new_map();
void map_put(Map *map, char *key, void *val);
void map_puti(Map *map, char *key, int val);
void *map_get(Map *map, char *key);
int map_geti(Map *map, char *key, int _default);
bool map_exist(Map *map, char *key);
Type *ptr_to (Type *base);
int size_of(Type *cty);
int align_of(Type *cty);
Type *ary_of(Type *base, int len);
Type *ctype_of_ary(Type *cty);
Node *addr_of(Node *base, Type *cty);
int roundup(int num, int multiple);

// logger.c
void dump_tokens(Vector *tks);

// codegen.c
void gen(Node *node);
void gen_x86(Vector *code, Vector *global_vars);