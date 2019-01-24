#include "ccc.h"


static Map *vars;
static int bpoff;
static char *args[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static int label_counter = 0;

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

void gen(Node *node, ...) {
    va_list parent_func;
    va_start(parent_func, node);

    if (node->ty == ND_FUNC_DEF) {
        printf(".global %s\n", node->name);
        printf("%s:\n", node->name);
        // Prologue.
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");

        // 関数内の変数領域確保
        printf("    sub rsp, 208\n");

        // Arity
        for (int j = 0; j < node->args->len; j++) {
            gen_lval(node->args->data[j]);
            printf("    pop rax\n");
            printf("    mov [rax], %s\n", args[j]);
        }

        // Body
        for (int i = 0; i < node->body->stmts->len; i++) {
            gen(node->body->stmts->data[i], node);
        }

        // Epilogue
        printf(".%s_end:\n", node->name);
        // pop this value so the stack in not overflown
        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    }

    if (node->ty == ND_IF) {
        gen(node->bl_expr);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        int else_label = label_counter++;
        printf("    je .else_%d\n", else_label);

        if (node->tr_stmt->stmts != NULL) {
            for (int i = 0; i < node->tr_stmt->stmts->len; i++) 
                gen(node->tr_stmt->stmts->data[i], va_arg(parent_func, Node*));
        } else {
            gen(node->tr_stmt, va_arg(parent_func, Node*));
        }

        printf(".else_%d:\n", else_label);
        if (node->els_stmt != NULL) {
            if (node->els_stmt->stmts != NULL) {
                for (int i = 0; i < node->els_stmt->stmts->len; i++) 
                    gen(node->els_stmt->stmts->data[i], va_arg(parent_func, Node*));
            } else {
                gen(node->els_stmt, va_arg(parent_func, Node*));
            }
        }
        return;
    }

    if (node->ty == ND_WHILE) {
        int while_label = label_counter++;
        int while_end = label_counter++;
        printf(".while_%d:\n", while_label);
        gen(node->bl_expr);
        printf("    pop rax;\n");
        printf("    cmp rax, 0\n");
        printf("    je .while_end_%d\n", while_end);

        if (node->body->stmts != NULL) {
            for (int i = 0; i < node->body->stmts->len; i++)
                gen(node->body->stmts->data[i], va_arg(parent_func, Node*));
        } else {
            gen(node->body);
        }

        printf("    jmp .while_%d\n", while_label);
        printf(".while_end_%d:\n", while_end);
        return;
    }

    if (node->ty == ND_FOR) {
        int for_label = label_counter++;
        int for_end = label_counter++;

        gen(node->init);

        printf(".for_%d:\n", for_label);
        gen(node->bl_expr);
        printf("    pop rax;\n");
        printf("    cmp rax, 0\n");
        printf("    je .for_end_%d\n", for_end);

        if (node->body->stmts != NULL) {
            for (int i = 0; i < node->body->stmts->len; i++)
                gen(node->body->stmts->data[i], va_arg(parent_func, Node*));
        } else {
            gen(node->body);
        }

        gen(node->inc);
        printf("    jmp .for_%d\n", for_label);
        printf(".for_end_%d:\n", for_end);
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
        for (int i = 0; i < node->args->len; i++)
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
        gen(node->rhs);
        printf("    jmp .%s_end\n", va_arg(parent_func, Node*)->name);
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
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n"); 
            break;
        case '>':
            printf("    cmp rax, rdi\n");
            printf("    setg al\n");
            printf("    movzb rax, al\n");
            break;
        case '<':
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            break;
    }

    printf("    push rax\n");
}

void gen_code(Vector *code) {
    printf(".intel_syntax noprefix\n");

    vars = new_map();
    bpoff = 8;

    // 先頭からコード生成
    for (int i = 0; i < code->len; i++) {
        gen(code->data[i]);
    }

}