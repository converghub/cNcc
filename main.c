#include "ccc.h"

static Vector *globals;

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
    // parse
    Vector *nodes = parse(tokens);
    // semantic analysis
    globals = new_vector();
    Vector *codes = sema(nodes, globals);

    // gen code
    gen_x86(codes, globals);

    return 0;
} 
