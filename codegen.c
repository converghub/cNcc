#include "ccc.h"


static Map *vars;
static int bpoff;

// gen
static void gen_lval(Node *node) {
    if (node->ty != ND_IDENT)
        error("gen_lval(): not a lvalue.");


    if (!map_exist(vars, node->name)) {
        map_put(vars, node->name, (void *)(intptr_t)bpoff);
        bpoff += 8;
    }

    int offset = (intptr_t)map_get(vars, node->name);
    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", offset);
    printf("    push rax\n");
}

void gen(Node *node) {
    if (node->ty == ND_FUNC_DEF) {
        printf(".global %s\n", node->name);
        printf("%s:\n", node->name);
        // プロローグ
        // 変数26個分の領域確保
        // TO DO: 汎用性向上
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, 208\n");

        for (int i = 0; i < node->body->stmts->len; i++)
            gen(node->body->stmts->data[i]);

        // スタックに一つの値が残っているはずなので溢れないようにpop
        printf("    pop rax\n");
        // エピローグ
        // 最期の結果がraxに残っているのでそれが返り値
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    }


    if (node->ty == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    if (node->ty == ND_IDENT) {
        gen_lval(node);
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    }


    if (node->ty == ND_FUNC_CALL) {
        char *args[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

        for (int i = 0; node->args->data[i]; i++)
            printf("    mov %s, %d\n", args[i], ((Node *)(node->args->data[i]))->val);
 
        printf("    call %s\n", node->name);
        printf("    push rax\n");
        return;
    }


    if (node->ty == '=') {
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("    pop rdi\n");
        printf("    pop rax\n");
        printf("    mov [rax], rdi\n");
        printf("    push rdi\n");
        return;
    }

    if (node->ty == ND_RETURN) {
        // TODO : ND_RETURNの実装、関数の場合
        gen(node->rhs);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->ty) {
        case '+':
            printf("    add rax, rdi\n");
            break;
        case '-':
            printf("    sub rax, rdi\n");
            break;
        case '*':
            printf("    mul rdi\n");
            break;
        case '/':
            printf("    mov rdx, 0\n");
            printf("    div rdi\n");
            break;
        case ND_EQ:
            printf("    cmp rdi, rax\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_NE:
            printf("    cmp rdi, rax\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");        
    }

    printf("    push rax\n");
}

void gen_code(Vector *code) {
    printf(".intel_syntax noprefix\n");

    vars = new_map();
    bpoff = 0;

    // 先頭からコード生成
    for (int i = 0; i < code->len; i++) {
        gen(code->data[i]);
    }

}