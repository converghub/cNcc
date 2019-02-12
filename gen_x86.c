#include "ccc.h"

// parse.c
extern Map *vars;

static char *args_reg8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
static char *args_reg32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
static char *args_reg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static int label_counter = 0;

// gen
static void gen_lval(Node *node) {
    if (node->ty == ND_DEREF) {
        return gen(node->expr);
    }
    if (node->ty != ND_IDENT && node->ty != ND_VAR_DEF && node->ty != ND_ADDR)
        error("gen_lval(): not a lvalue.");

    if (!map_exist(vars, node->name)) {
        error("undefined variable: %s", node->name);
    }

    Var *var = map_get(vars, node->name);
    node->cty = var->cty;

    if (var->is_local) {
        printf("    mov rax, rbp\n");
        printf("    sub rax, %d\n", var->offset);
        printf("    push rax\n");        
    } else {
        printf("    lea rax, %s\n", var->name);
        printf("    push rax\n");
    }
    return;
}

void gen(Node *node, ...) {
    va_list parent_func;
    va_start(parent_func, node);

    if (node->ty == ND_NULL)
        return;
    if (node->ty == ND_FUNC_DEF) {
        printf(".global %s\n", node->name);
        printf("%s:\n", node->name);
        // Prologue.
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");

        // 関数内の変数領域確保
        printf("    sub rsp, %d\n", node->stacksize);

        // Arity
        for (int j = 0; j < node->args->len; j++) {
            gen_lval(node->args->data[j]);
            printf("    pop rax\n");
            int data_size = size_of(((Node *)node->args->data[j])->cty);
            if (data_size == 4)
                printf("    mov [rax], %s\n", args_reg32[j]);
            else if (data_size == 1)
                printf("    mov [rax], %s\n", args_reg8[j]);
            else
                printf("    mov [rax], %s\n", args_reg64[j]);
        }

        // Body
        for (int i = 0; i < node->body->stmts->len; i++) {
            gen(node->body->stmts->data[i], node);
        }

        printf(".%s_end:\n", node->name);
        // pop this value so the stack in not overflown
        printf("    pop rax\n");
        // Epilogue        
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    }

    if (node->ty == ND_EXPR_STMT) {
        gen(node->expr);
        return;
    }

    if (node->ty == ND_STMT_EXPR) {
        Node *expr_node = node->stmt_expr;
        for (int i = 0; i < expr_node->stmts->len; i++) {
            gen(expr_node->stmts->data[i]);
        }
        return;
    }

    if (node->ty == ND_IF) {
        gen(node->bl_expr);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        int if_else_label = label_counter++;
        printf("    je .else_%d\n", if_else_label);

        if (node->tr_stmt->stmts != NULL) {
            for (int i = 0; i < node->tr_stmt->stmts->len; i++) 
                gen(node->tr_stmt->stmts->data[i], va_arg(parent_func, Node*));
        } else {
            gen(node->tr_stmt, va_arg(parent_func, Node*));
        }
        printf("    jmp .if_end_%d\n", if_else_label);

        printf(".else_%d:\n", if_else_label);
        if (node->els_stmt != NULL) {
            if (node->els_stmt->stmts != NULL) {
                for (int i = 0; i < node->els_stmt->stmts->len; i++) 
                    gen(node->els_stmt->stmts->data[i], va_arg(parent_func, Node*));
            } else {
                gen(node->els_stmt, va_arg(parent_func, Node*));
            }
        }
        printf(".if_end_%d:\n", if_else_label);
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

    if (node->ty == ND_DO_WHILE) {
        int do_while_label = label_counter++;
        int do_while_end = label_counter++;        
        printf(".do_while_%d:\n", do_while_label);

        if (node->body->stmts != NULL) {
            for (int i = 0; i < node->body->stmts->len; i++)
                gen(node->body->stmts->data[i], va_arg(parent_func, Node*));
        } else {
            gen(node->body);
        }

        gen(node->bl_expr);
        printf("    pop rax;\n");
        printf("    cmp rax, 0\n");
        printf("    je .do_while_end_%d\n", do_while_end);

        printf("    jmp .do_while_%d\n", do_while_label);
        printf(".do_while_end_%d:\n", do_while_end);
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

    if (node->ty == ND_STR) {
        printf("    lea rax, %s\n", node->name);
        printf("    push rax\n");
        return;
    }

    if (node->ty == ND_IDENT) {
        gen_lval(node);
        if (node->cty->ty == ARY) return;
        printf("    pop rax\n");
        if (size_of(node->cty) == 4) {
            printf("    mov eax, [rax]\n");            
        }
        else if (size_of(node->cty) == 1) {
            printf("    mov al, [rax]\n");
            printf("    and rax, 0xFF\n");
        } else
            printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    }

    if (node->ty == ND_VAR_DEF) {
        if (node->init == NULL) 
            return;
        gen_lval(node);
        gen(node->init);
        printf("    pop rdi\n");
        printf("    pop rax\n");

        if(size_of(node->cty) == 4) 
            printf("    mov [rax], edi\n");
        else if (size_of(node->cty) == 1)
            printf("    mov [rax], dil\n");
        else
            printf("    mov [rax], rdi\n");
        printf("    push rdi\n");
        return;
    }

    if (node->ty == ND_FUNC_CALL) {
        for (int i = 0; i < node->args->len; i++) {
           gen(node->args->data[i]);
        }

        for (int i = node->args->len - 1; i >= 0; i--) {
            printf("    pop %s\n", args_reg64[i]);
        }

        printf("    call %s\n", node->name);
        printf("    push rax\n");
        return;
    }


    if (node->ty == '=') {
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("    pop rdi\n");
        printf("    pop rax\n");


        if(size_of(node->lhs->cty) == 4) 
            printf("    mov [rax], edi\n");
        else if (size_of(node->lhs->cty) == 1)
            printf("    mov [rax], dil\n");
        else
            printf("    mov [rax], rdi\n");
        printf("    push rdi\n");
        return;
    }

    if (node->ty == ND_DEREF) {
        gen(node->expr);
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    }

    if (node->ty == ND_ADDR) {
        gen_lval(node->expr);
        return;
    }

    if (node->ty == '!') {
        gen(node->expr);
        printf("    pop rax\n");
        if (align_of(node->expr->cty) == 4) {
            printf("    mov rcx, 0xFFFFFFFF\n");
            printf("    and rax, rcx\n");            
        }
        else if (align_of(node->expr->cty) == 1) {
            printf("    and rax, 0xFF\n");
        } 
        printf("    mov rdi, 0\n");
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        printf("    push rax\n");
        return;
    }

    if (node->ty == '~') {
        gen(node->expr);
        printf("    pop rax\n");
        printf("    not rax\n");
        printf("    push rax\n");
        return;
    }

    if (node->ty == '?') {        
        int cndtnl_label = label_counter++;

        gen(node->bl_expr);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .cndtnl_else_%d\n", cndtnl_label);

        if (node->tr_stmt->stmts != NULL) {
            for (int i = 0; i < node->tr_stmt->stmts->len; i++) 
                gen(node->tr_stmt->stmts->data[i], va_arg(parent_func, Node*));
        } else {
            gen(node->tr_stmt, va_arg(parent_func, Node*));
        }
        printf("    jmp .cndtnl_end_%d\n", cndtnl_label);

        printf(".cndtnl_else_%d:\n", cndtnl_label);
        if (node->els_stmt != NULL) {
            if (node->els_stmt->stmts != NULL) {
                for (int i = 0; i < node->els_stmt->stmts->len; i++) 
                    gen(node->els_stmt->stmts->data[i], va_arg(parent_func, Node*));
            } else {
                gen(node->els_stmt, va_arg(parent_func, Node*));
            }
        }
        printf(".cndtnl_end_%d:\n", cndtnl_label);
        return;
    }

    if (node->ty == ND_RETURN) {
        gen(node->expr);
        printf("    jmp .%s_end\n", va_arg(parent_func, Node*)->name);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    if (node->ty == ',') return;

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->ty) {
        case '+':
            if (node->rhs->cty->ty == PTR || node->rhs->cty->ty == ARY) {
                printf("    push rdi\n");
                int coeff = 0;
                if (node->rhs->cty->ty == ARY) 
                    coeff = align_of(node->rhs->cty);
                else
                    coeff = align_of(node->rhs->cty->ptrof);
                printf("    mov rdi, %d\n", coeff);
                printf("    mul rdi\n");
                printf("    pop rdi\n");                
            } else if (node->lhs->cty->ty == PTR || node->lhs->cty->ty == ARY) {
                printf("    push rax\n");
                printf("    mov rax, rdi\n");
                int coeff = 0;
                if (node->lhs->cty->ty == ARY) 
                    coeff = align_of(node->lhs->cty);
                else
                    coeff = align_of(node->lhs->cty->ptrof);                
                printf("    mov rdi, %d\n", coeff);
                printf("    mul rdi\n");
                printf("    mov rdi, rax\n");
                printf("    pop rax\n");
            }
            printf("    add rax, rdi\n");
            break;
        case '-':
            if (node->rhs->cty->ty == PTR || node->rhs->cty->ty == ARY) {
                error("gen():  <expression> - <pointer> is not supported currently.\n");
            } else if (node->lhs->cty->ty == PTR || node->lhs->cty->ty == ARY) {
                printf("    push rax\n");
                printf("    mov rax, rdi\n");
                int coeff = 0;
                if (node->lhs->cty->ty == ARY) 
                    coeff = align_of(node->lhs->cty);
                else
                    coeff = align_of(node->lhs->cty->ptrof);  
                printf("    mov rdi, %d\n",  coeff);
                printf("    mul rdi\n");
                printf("    mov rdi, rax\n");
                printf("    pop rax\n");                
            }
            printf("    sub rax, rdi\n");
            break;
        case '*':
            printf("    mul rdi\n");
            break;
        case '/':
            printf("    mov rdx, 0\n");
            printf("    div rdi\n");
            break;
        case '%':
            printf("    mov rdx, 0\n");
            printf("    div rdi\n");
            printf("    mov rax, rdx\n");
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
        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            break;             
        case ND_GE:
            printf("    cmp rax, rdi\n");
            printf("    setge al\n");
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
        case '|':
            printf("    or rax, rdi\n");
            break;
        case '^':
            printf("    xor rax, rdi\n");
            break;
        case ND_LAND:
            printf("    and rax, rdi\n");
            break;
        case ND_LOR:
            printf("    or rax, rdi\n");
            break;
        case ND_SHL:
            printf("    mov cl, dil\n");
            printf("    shl rax, cl\n");        
            break;
        case ND_SHR:
            printf("    mov cl, dil\n");
            printf("    shr rax, cl\n");        
            break;
    }

    printf("    push rax\n");
}


static char *escape(char *s, int len) {
  char *buf = malloc(len * 4);
  char *p = buf;
  for (int i = 0; i < len; i++) {
    if (s[i] == '\\') {
      *p++ = '\\';
      *p++ = '\\';
    } else if (isgraph(s[i]) || s[i] == ' ') {
      *p++ = s[i];
    } else {
      sprintf(p, "\\%03o", s[i]);
      p += 4;
    }
  }
  *p = '\0';
  return buf;
}

void gen_x86(Vector *code) {
    printf(".intel_syntax noprefix\n");


    printf(".data\n");
    // Global variable
    for (int i = 0; i < code->len; i++) {
        Node *node = code->data[i];
        if (node->ty == ND_VAR_DEF) {
            Var *var = map_get(vars, node->name);
            if (var->is_extern)
                continue;
            printf("%s:\n", var->name);
            printf("    .ascii \"%s\"\n", escape(var->data, var->len));          
        } else
            continue;
    }

    printf(".text\n");
    // String literal
    for (int i = 0; i < code->len; i++) {
        Node *node = code->data[i];
        if (node->strings) {
            for (int j = 0; j < node->strings->len; j++) {
                Node *str = node->strings->data[j];
                printf("%s:\n", str->name);
                printf("    .asciz \"%s\"\n", str->str);
            }
        } else
            continue;
    }

    // Functions
    for (int i = 0; i < code->len; i++) {
        if (((Node *)code->data[i])->ty == ND_VAR_DEF) 
            continue;
        gen(code->data[i]);
    }

}