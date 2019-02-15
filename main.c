#include "ccc.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static Vector *globals;
static char *read_file(char *filename);

int main(int argc, char** argv) {
    char *filename;
    char *input;

    if (argc == 2 && !strcmp(argv[1], "-test")) {
        run_test();
        return 0;
    }

    if (argc != 2) {
        if (argc == 3 && !strcmp(argv[1], "-file")) {
            filename = argv[2];
        } else {
            fprintf(stderr, "引数の個数が正しくありません\n");
            return 1;
        }
    }

    // input
    input = argv[1];
    if (filename != NULL)
        input = read_file(filename);
    // tokenize
    Vector *tokens = tokenize(input);
    // parse
    Vector *nodes = parse(tokens);
    // semantic analysis
    globals = new_vector();
    Vector *codes = sema(nodes, globals);

    // gen code
    gen_x86(codes, globals);

    return 0;
} 




static char *read_file(char *filename) {
    long file_size;
    struct stat stbuf;
    int fd;
    FILE *fp;
    char *input;

    // get read file size
    fd = open(filename, O_RDONLY);
    if (fd == -1) 
        error("read_file(): cannot open file");

    fp = fopen(filename, "r");
    if (!fp) 
        error("read_file(): cannot open file");

    if (fstat(fd, &stbuf) == -1) 
        error("read_file(): cannot get file state");

    file_size = stbuf.st_size;

    // malloc buffer and read
    input = malloc(file_size);
    fread(input, 1, file_size, fp);

    fclose(fp);
    return input;
}