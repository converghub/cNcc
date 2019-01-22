#include "ccc.h"


Node *code[100];

int main(int argc, char** argv) {
    if (argc == 2 && !strcmp(argv[1], "-test")) {
        run_test();
        return 0;
    }

    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // tokenizeする
    Vector *tokens = tokenize(argv[1]);
//    for (int i = 0; tokens->data[i]; i++)
//        printf("tokens->data[%d])->input: %s\n", i, ((Token *)tokens->data[i])->input);
    program(code, tokens);

    // assembly 前半
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    // 変数26個分の領域確保
    // TO DO: 汎用性向上
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, 208\n");

    gen_code(code);

    // エピローグ
    // 最期の結果がraxに残っているのでそれが返り値
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");

    return 0;
} 
