#include "ccc.h"
#define GET_TK(TK, POS) ((Token *)(TK)->data[(POS)])

void dump_tokens(Vector *tks) {
    int pos = 0;
    char t[4096];
    FILE *fplog;
    fplog = fopen("dump_tokens_log.txt", "w");
    if (!fplog) 
        error("dump_tokens(): cannot open file");

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
