#include "cncc.h"
#define GET_TK(TK, POS) ((Token *)(TK)->data[(POS)])

void dump_tokens(Vector *tks) {
    int pos = 0;
    char t[4096];
    FILE *fplog;
    fplog = fopen("dump_tokens_log.txt", "w");
    if (!fplog) 
        error("dump_tokens(): cannot open file");

    fprintf(fplog, "<--- Node enum Dump --->\n");
    fprintf(fplog, "ND_NUM : %d\n", ND_NUM);
    fprintf(fplog, "ND_STR : %d\n", ND_STR);
    fprintf(fplog, "ND_RETURN : %d\n", ND_RETURN);
    fprintf(fplog, "ND_SIZEOF : %d\n", ND_SIZEOF);
    fprintf(fplog, "ND_ALIGNOF : %d\n", ND_ALIGNOF);
    fprintf(fplog, "ND_IDENT : %d\n", ND_IDENT);
    fprintf(fplog, "ND_VAR_DEF : %d\n", ND_VAR_DEF);
    fprintf(fplog, "ND_LVAR : %d\n", ND_LVAR);
    fprintf(fplog, "ND_GVAR: %d\n", ND_GVAR);
    fprintf(fplog, "ND_IF : %d\n", ND_IF);
    fprintf(fplog, "ND_WHILE : %d\n", ND_WHILE);
    fprintf(fplog, "ND_DO_WHILE : %d\n", ND_DO_WHILE);
    fprintf(fplog, "ND_FOR : %d\n", ND_FOR);
    fprintf(fplog, "ND_DEREF : %d\n", ND_DEREF);
    fprintf(fplog, "ND_ADDR : %d\n", ND_ADDR);
    fprintf(fplog, "ND_DOT : %d\n", ND_DOT);
    fprintf(fplog, "ND_EXPR_STMT : %d\n", ND_EXPR_STMT);
    fprintf(fplog, "ND_STMT_EXPR : %d\n", ND_STMT_EXPR);
    fprintf(fplog, "ND_CMPD_STMT : %d\n", ND_CMPD_STMT);
    fprintf(fplog, "ND_FUNC_CALL : %d\n", ND_FUNC_CALL);
    fprintf(fplog, "ND_FUNC_DEF: %d\n", ND_FUNC_DEF);
    fprintf(fplog, "ND_EQ: %d\n", ND_EQ);
    fprintf(fplog, "ND_NE: %d\n", ND_NE);
    fprintf(fplog, "ND_LE: %d\n", ND_LE);
    fprintf(fplog, "ND_GE: %d\n", ND_GE);
    fprintf(fplog, "ND_LOR: %d\n", ND_LOR);
    fprintf(fplog, "ND_LAND: %d\n", ND_LAND);
    fprintf(fplog, "ND_SHL: %d\n", ND_SHL);
    fprintf(fplog, "ND_SHR: %d\n", ND_SHR);
    fprintf(fplog, "ND_PRE_INC: %d\n", ND_PRE_INC);
    fprintf(fplog, "ND_PRE_DEC: %d\n", ND_PRE_DEC);
    fprintf(fplog, "ND_POST_INC: %d\n", ND_POST_INC);
    fprintf(fplog, "ND_POST_DEC: %d\n", ND_POST_DEC);
    fprintf(fplog, "ND_NULL: %d\n", ND_NULL);
    fprintf(fplog, "<--- Operator Dump --->\n");
    fprintf(fplog, "! : %d\n", '!');
    fprintf(fplog, "%% : %d\n", '%');
    fprintf(fplog, "* : %d\n", '*');
    fprintf(fplog, "+ : %d\n", '+');
    fprintf(fplog, ", : %d\n", ',');
    fprintf(fplog, "- : %d\n", '-');
    fprintf(fplog, "/ : %d\n", '/');
    fprintf(fplog, "< : %d\n", '<');
    fprintf(fplog, "= : %d\n", '=');
    fprintf(fplog, "> : %d\n", '>');
    fprintf(fplog, "? : %d\n", '?');
    fprintf(fplog, "^ : %d\n", '^');
    fprintf(fplog, "| : %d\n", '|');
    fprintf(fplog, "~ : %d\n", '~');

    fprintf(fplog, "<--- Dump tokens --->\n");
    while (GET_TK(tks, pos)->ty != TK_EOF) {
        int num = strlen(GET_TK(tks, pos)->input) - strlen(GET_TK(tks, pos+1)->input);

        memset(t, '\0', sizeof(t));
        strncpy(t, GET_TK(tks, pos)->input, num);
        fprintf(fplog, "%d: %s\n", pos, t);
        pos++;
    }
    fprintf(fplog, "<--- End of Dump %d tokens --->\n", pos);

    fclose(fplog);
    return;
}
