#include "ccc.h"
#define SYMBOL_NUMBER (sizeof symbols)/(sizeof (struct symbol))

Map *keywords;

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
    {"==", TK_EQ}, {"!=", TK_NE}, {"&&", TK_LAND}, {"||", TK_LOR}
};


// pが指している文字列をトークンに分割してtokensに保存する
Vector *tokenize(char *p) {
    keywords = new_map();
    map_put(keywords, "if", (void *)TK_IF);
    map_put(keywords, "else", (void *)TK_ELSE);
    map_put(keywords, "return", (void *)TK_RETURN);
    map_put(keywords, "while", (void *)TK_WHILE);
    map_put(keywords, "for", (void *)TK_FOR);
    map_put(keywords, "int", (void *)TK_INT);
    map_put(keywords, "char", (void *)TK_CHAR);
    map_put(keywords, "sizeof", (void * )TK_SIZEOF);
    map_put(keywords, "extern", (void *)TK_EXTERN);

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

        // Keyword
        if (isalpha(*p) || *p == '_') {
            int len = 1;
            while (isalpha(p[len]) || isdigit(p[len]) || p[len] == '_' )
                len++;
            
            char *name = strndup(p, len);
            int ty = (intptr_t)map_get(keywords, name);
            if (!ty)
                ty = TK_IDENT;

            Token *t = add_token(v, ty, p);
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
