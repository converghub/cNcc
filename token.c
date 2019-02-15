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
    {"||", TK_LOR},        {"<=", TK_LE},
    {">=", TK_GE},         {"_Alignof", TK_ALIGNOF}, {"<<", TK_SHL},
    {">>", TK_SHR},        {"++", TK_INC},           {"--", TK_DEC},
};


static Map *new_keywords() {
    Map *map = new_map();
    map_puti(map, "if", TK_IF);
    map_puti(map, "else", TK_ELSE);
    map_puti(map, "return", TK_RETURN);
    map_puti(map, "while", TK_WHILE);
    map_puti(map, "for", TK_FOR);
    map_puti(map, "int", TK_INT);
    map_puti(map, "char", TK_CHAR);
    map_puti(map, "sizeof", TK_SIZEOF);
    map_puti(map, "extern", TK_EXTERN);
    map_puti(map, "do", TK_DO);
    map_puti(map, "_Alignof", TK_ALIGNOF);
    return map;
}


// Tokenize input 
Vector *tokenize(char *p) {
    Vector *v = new_vector();
    Map *keywords = new_keywords();

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

        // Multi-letter symbol
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

        // Single-letter symbol
        if (strchr("+-*/=;(),{}><&[]|!^?:~%", *p)) {
            add_token(v, *p, p);
            p++;
            continue;
        }

        // Keyword / Identifier
        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while (isalpha(p[len]) || isdigit(p[len]) || p[len] == '_' )
                len++;

            char *name = strndup(p, len);
            int ty = map_geti(keywords, name, -1);

            Token *t = add_token(v, (ty == -1) ? TK_IDENT : ty, p);
            t->name = name;
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
