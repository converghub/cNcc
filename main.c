#include "ccc.h"

int main(int argc, char** argv) {
    if (argc == 2 && !strcmp(argv[1], "-test")) {
        run_test();
        return 0;
    }

    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // tokenize
    Vector *tokens = tokenize(argv[1]);
    Vector *code = parse(tokens);

    // gen code
    gen_code(code);

    return 0;
} 
