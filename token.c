#include "ccc.h"
#define SYMBOL_NUMBER (sizeof symbols)/(sizeof (struct symbol))


static Token *add_token(Vector *v, int ty, char *input) {
    Token *t = malloc(sizeof(Token));
    t->ty = ty;
    t->input = input;
    vec_push(v, t);
    return t;
}


static struct symbol{
    char *name;
    int ty;
} symbols[] = {
    {"==", TK_EQ},         {"!=", TK_NE},            {"&&", TK_LAND}, 
    {"||", TK_LOR},        {"if", TK_IF},            {"else", TK_ELSE},
    {"return", TK_RETURN}, {"while", TK_WHILE},      {"for", TK_FOR},
    {"int", TK_INT},       {"char", TK_CHAR},        {"sizeof", TK_SIZEOF},
    {"extern", TK_EXTERN}, {"do", TK_DO},            {"<=", TK_LE},
    {">=", TK_GE},         {"_Alignof", TK_ALIGNOF},
};


// Tokenize input 
Vector *tokenize(char *p) {
    Vector *v = new_vector();

loop:
    while (*p) {
        // Skip space
        if (isspace(*p)) {
            p++;
            continue;
        }

        // String literal
        if (*p == '"') {
            Token *t = add_token(v, TK_STR, p);
            p++;

            int len = 0;
            while(p[len] && p[len] != '"')
                len++;
            if (!p[len])
                error("tokenize(): incorrect end of string literal. %s", p);
            t->str = strndup(p, len);
            p = p + len;
            p++;
            continue;
        }

        // Multi-letter symbol/keyword
        for (int i = 0; i < SYMBOL_NUMBER; i++) {
            char *name = symbols[i].name;
            int len = strlen(name);
            if (strncmp(p, name, len)) {
                continue;
            }

            add_token(v, symbols[i].ty, p);
            p += len;
            goto loop;
        }

        // Single-letter token
        if (strchr("+-*/=;(),{}><&[]", *p)) {
            add_token(v, *p, p);
            p++;
            continue;
        }

        // Identifier
        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while (isalpha(p[len]) || isdigit(p[len]) || p[len] == '_' )
                len++;

            Token *t = add_token(v, TK_IDENT, p);
            t->name = strndup(p, len);
            p += len;
            continue;
        }

        if (isdigit(*p)) {
            Token *t = add_token(v, TK_NUM, p);
            t->val = strtol(p, &p, 10);
            continue;
        }

        error("tokenize(): Cannot tokenize %s\n", p);
    }

    add_token(v, TK_EOF, p);
    return v;
}
