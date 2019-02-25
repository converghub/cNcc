#include "cncc.h"

// parse.c
extern Map *vars;

static char *args_reg8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
static char *args_reg32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
static char *args_reg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static int label_counter = 0;
static int break_label;

// gen
static void gen_lval(Node *node) {
    if (node->ty == ND_DEREF) {
        return gen(node->expr);
    }

    if (node->ty == ND_DOT) {
        gen_lval(node->expr);
        printf("    pop rax\n");
        printf("    add rax, %d\n", node->offset);
        printf("    push rax\n");
        return;
    }

    if (node->ty == ND_LVAR || node->ty == ND_VAR_DEF) {
        printf("    mov rax, rbp\n");
        printf("    sub rax, %d\n", node->offset);
        printf("    push rax\n"); 
    } else {
        printf("    lea rax, %s\n", node->name);
        printf("    push rax\n");
    }
    return;
}

void gen(Node *node) {
    if (node->ty == ND_FUNC_DEF) {
        printf(".global %s\n", node->name);
        printf("%s:\n", node->name);
        // Prologue.
        printf("    push rbx\n");
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");

        // Set stack data size
        printf("    sub rsp, %d\n", roundup(node->stacksize, 16));

        // Arity
        for (int j = 0; j < node->args->len; j++) {
            gen_lval(node->args->data[j]);
            printf("    pop rax\n");
            int data_size = ((Node *)node->args->data[j])->cty->size;
            if (data_size == 4)
                printf("    mov [rax], %s\n", args_reg32[j]);
            else if (data_size == 1)
                printf("    mov [rax], %s\n", args_reg8[j]);
            else
                printf("    mov [rax], %s\n", args_reg64[j]);
        }

        // Body
        for (int i = 0; i < node->body->stmts->len; i++) {
            gen(node->body->stmts->data[i]);
        }

        printf(".%s_end:\n", node->name);
        // Epilogue        
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    pop rbx\n");
        printf("    ret\n");
        return;
    }

    if (node->ty == ND_NULL)
        return;    

    if (node->ty == ND_EXPR_STMT) {
        gen(node->expr);
        return;
    }

    if (node->ty == ND_CMPD_STMT) {
        for (int i = 0; i < node->stmts->len; i++)
            gen(node->stmts->data[i]);
        return;
    }

    if (node->ty == ND_IF) {
        int if_else_label = label_counter++;
        gen(node->bl_expr);
        if (!node->bl_expr->no_push)
            printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .else_%d\n", if_else_label);

        if (node->tr_stmt->stmts != NULL) {
            for (int i = 0; i < node->tr_stmt->stmts->len; i++) {
                gen(node->tr_stmt->stmts->data[i]);
                if ( !((Node*)node->tr_stmt->stmts->data[i])->no_push )
                    printf("    pop rax\n");
            }
        } else {
            gen(node->tr_stmt);
            if (!node->tr_stmt->no_push)
                printf("    pop rax\n");
        }

        printf("    jmp .if_end_%d\n", if_else_label);

        printf(".else_%d:\n", if_else_label);
        if (node->els_stmt) {
            if (node->els_stmt->stmts != NULL) {
                for (int i = 0; i < node->els_stmt->stmts->len; i++) {
                    gen(node->els_stmt->stmts->data[i]);
                    if ( !((Node*)node->els_stmt->stmts->data[i])->no_push )
                        printf("    pop rax\n");
                }
            } else {
                gen(node->els_stmt);
                if (!node->els_stmt->no_push)
                    printf("    pop rax\n");
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
        if (!node->bl_expr->no_push)
            printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .while_end_%d\n", while_end);

        if (node->body->stmts != NULL) {
            for (int i = 0; i < node->body->stmts->len; i++) {
                gen(node->body->stmts->data[i]);
                if ( !((Node*)node->body->stmts->data[i])->no_push )
                    printf("    pop rax\n");
            }
        } else {
            gen(node->body);
            if (!node->body->no_push)
                printf("    pop rax\n");
        }

        printf("    jmp .while_%d\n", while_label);
        printf(".while_end_%d:\n", while_end);
        return;
    }

    if (node->ty == ND_DO_WHILE) {
        int do_while_label = label_counter++;
        int do_while_end = label_counter++;
        break_label = label_counter++;
        int here_break_label = break_label;

        printf(".do_while_%d:\n", do_while_label);

        if (node->body->stmts != NULL) {
            for (int i = 0; i < node->body->stmts->len; i++) {
                gen(node->body->stmts->data[i]);
                if ( !((Node*)node->body->stmts->data[i])->no_push )
                    printf("    pop rax\n");
            }
        } else {
            gen(node->body);
            if (!node->body->no_push)
                printf("    pop rax\n");
        }


        gen(node->bl_expr);
        if (!node->bl_expr->no_push)
            printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .do_while_end_%d\n", do_while_end);

        printf("    jmp .do_while_%d\n", do_while_label);
        printf(".do_while_end_%d:\n", do_while_end);
        printf(".break%d:\n", here_break_label);
        return;
    }

    if (node->ty == ND_FOR) {
        int for_label = label_counter++;
        int for_end = label_counter++;
        break_label = label_counter++;
        int here_break_label = break_label;
        // init
        if (node->init->ty != ND_NULL) {
            gen(node->init);
            if (!node->init->no_push)
                printf("    pop rax\n");
        }

        printf(".for_%d:\n", for_label);
        // cond
        if (node->bl_expr->ty != ND_NULL) {
            gen(node->bl_expr);
            if (!node->bl_expr->no_push)
                printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .for_end_%d\n", for_end);
        }
        // body
        if (node->body->stmts != NULL) {
            for (int i = 0; i < node->body->stmts->len; i++) {
                gen(node->body->stmts->data[i]);
                if ( !((Node*)node->body->stmts->data[i])->no_push )
                    printf("    pop rax\n");
            }
        } else {
            gen(node->body);
            if (!node->body->no_push)
                printf("    pop rax\n");
        }
        // inc
        if (node->inc->ty != ND_NULL) {
            gen(node->inc);
            if(!node->inc->no_push)
                printf("    pop rax\n");
        }
        printf("    jmp .for_%d\n", for_label);
        printf(".for_end_%d:\n", for_end);
        printf(".break%d:\n", here_break_label);
        return;
    }

    if (node->ty == ND_STMT_EXPR) {
        // expr_node->ty is ND_CMPD_STMT
        Node *expr_node = node->stmt_expr;
        for (int i = 0; i < expr_node->stmts->len; i++)
            gen(expr_node->stmts->data[i]);
        /* the last stmt's "no_push" should be always false 
           (see check_push() in semantic_analysis.c),  
           so the following is for an upper node like other expressions. */           
        printf("    pop rax\n");
        if (!node->no_push)
            printf("    push rax\n");
        return;
    }

    if (node->ty == ND_NUM) {
        printf("    mov rax, %d\n", node->val);
        if (!node->no_push)
            printf("    push %d\n", node->val);
        return;
    }

    if (node->ty == ND_STR) {
        printf("    lea rax, %s\n", node->name);
        printf("    push rax\n");
        return;
    }

    if (node->ty == ND_LVAR || node->ty == ND_GVAR) {
        gen_lval(node);

        printf("    pop rax\n");
        if (node->cty->size == 4) {
            printf("    mov eax, [rax]\n");            
        }
        else if (node->cty->size == 1) {
            printf("    mov al, [rax]\n");
            printf("    and rax, 0xFF\n");
        } else
            printf("    mov rax, [rax]\n");
        
        if (!node->no_push)
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

        if(node->cty->size == 4) 
            printf("    mov [rax], edi\n");
        else if (node->cty->size == 1)
            printf("    mov [rax], dil\n");
        else
            printf("    mov [rax], rdi\n");

        if (!node->no_push)
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

        printf("    mov rax, 0\n");
        printf("    call %s\n", node->name);
        printf("    push rax\n");
        return;
    }


    if (node->ty == '=') {
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("    pop rdi\n");
        printf("    pop rax\n");


        if(node->lhs->cty->size == 4) 
            printf("    mov [rax], edi\n");
        else if (node->lhs->cty->size == 1)
            printf("    mov [rax], dil\n");
        else
            printf("    mov [rax], rdi\n");

        // for example, if "a=b=c...", "func(a=1)"
        printf("    mov rax, [rax]\n");
        if (!node->no_push)
            printf("    push rdi\n");
        return;
    }

    if (node->ty == ND_DEREF) {
        gen(node->expr);
        printf("    pop rax\n");

        if (node->expr->cty->ptrto->align == 4) {
            printf("    mov eax, [rax]\n");  
        } else if (node->expr->cty->ptrto->align == 1) {
            printf("    mov al, [rax]\n");
            printf("    and rax, 0xFF\n");            
        } else 
            printf("    mov rax, [rax]\n");

        if (!node->no_push)
            printf("    push rax\n");
        return;
    }

    if (node->ty == ND_ADDR) {
        gen_lval(node->expr);
        return;
    }

    if (node->ty == ND_DOT) {
        gen_lval(node->expr);
        printf("    pop rax\n");

        printf("    add rax, %d\n", node->offset);
        if (node->expr->cty->align == 4) {
            printf("    mov eax, [rax]\n");  
        } else if (node->expr->cty->align == 1) {
            printf("    mov al, [rax]\n");
            printf("    and rax, 0xFF\n");            
        } else 
            printf("    mov rax, [rax]\n");

        if (!node->no_push)
            printf("    push rax\n");
        return;
    }

    if (node->ty == ND_PRE_INC || node->ty == ND_PRE_DEC) {
        gen(node->expr);
        printf("    pop rax\n");
        gen_lval(node->expr);
        if (node->cty->size == 4) {
            printf("    mov eax, [rax]\n");  
        }
        else if (node->cty->size == 1) {
            printf("    mov al, [rax]\n");
            printf("    and rax, 0xFF\n");
        } else
            printf("    mov rax, [rax]\n");
        int num = 1;
        if (node->expr->cty->ty == PTR)
            num = node->expr->cty->ptrto->align;
        if (node->ty == ND_PRE_INC)
            printf("    add rax, %d\n", num);
        else if (node->ty == ND_PRE_DEC)
            printf("    sub rax, %d\n", num);        
        printf("    pop rdx\n");
        printf("    mov [rdx], rax\n");

        if (!node->no_push)
            printf("    push rax\n");
        return;
    }

    if (node->ty == ND_POST_INC || node->ty == ND_POST_DEC) {
        gen(node->expr);
        printf("    pop r11\n");
        gen_lval(node->expr);
        if (node->cty->size == 4) {
            printf("    mov eax, [rax]\n");  
        }
        else if (node->cty->size == 1) {
            printf("    mov al, [rax]\n");
            printf("    and rax, 0xFF\n");
        } else
            printf("    mov rax, [rax]\n");
        int num = 1;
        if (node->expr->cty->ty == PTR)
            num = node->expr->cty->ptrto->align;
        if (node->ty == ND_POST_INC)
            printf("    add rax, %d\n", num);
        else if (node->ty == ND_POST_DEC)
            printf("    sub rax, %d\n", num);        
        printf("    pop rdx\n");
        printf("    mov [rdx], rax\n");

        printf("    mov rax, r11\n");
        if (!node->no_push)
            printf("    push rax\n");
        return;
    }

    if (node->ty == '!') {
        gen(node->expr);
        printf("    pop rax\n");
        if (node->expr->cty->align == 4) {
            printf("    mov rcx, 0xFFFFFFFF\n");
            printf("    and rax, rcx\n");            
        }
        else if (node->expr->cty->align == 1) {
            printf("    and rax, 0xFF\n");
        } 
        printf("    mov rdi, 0\n");
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        if (!node->no_push)
            printf("    push rax\n");
        return;
    }

    if (node->ty == ND_NEG) {
        if (!node->expr) error("HOGE!");
        gen(node->expr);
        printf("    pop rax\n");
        if (node->expr->cty->align == 4) {
            printf("    mov rcx, 0xFFFFFFFF\n");
            printf("    and rax, rcx\n");            
        }
        else if (node->expr->cty->align == 1) {
            printf("    and rax, 0xFF\n");
        } 
        printf("neg rax\n");
        if (!node->no_push)
            printf("    push rax\n");
        return;
    }

    if (node->ty == '~') {
        gen(node->expr);
        printf("    pop rax\n");
        printf("    not rax\n");
        if (!node->no_push)
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
            for (int i = 0; i < node->tr_stmt->stmts->len; i++) {
                gen(node->tr_stmt->stmts->data[i]);
                if (node->no_push)
                    printf("    pop rax\n");
                }
        } else {
            gen(node->tr_stmt);
            if (node->no_push)
                printf("    pop rax\n");
        }
        printf("    jmp .cndtnl_end_%d\n", cndtnl_label);

        printf(".cndtnl_else_%d:\n", cndtnl_label);
        if (node->els_stmt != NULL) {
            if (node->els_stmt->stmts != NULL) {
                for (int i = 0; i < node->els_stmt->stmts->len; i++) {
                    gen(node->els_stmt->stmts->data[i]);
                    if (node->no_push)
                        printf("    pop rax\n");
                }
            } else {
                gen(node->els_stmt);
                if (node->no_push)
                    printf("    pop rax\n");
            }
        }
        printf(".cndtnl_end_%d:\n", cndtnl_label);
        return;
    }

    if (node->ty == ND_BREAK) {
        printf("    jmp .break%d\n", break_label);
        return;
    }

    if (node->ty == ND_RETURN) {
        gen(node->expr);
        printf("    pop rax\n");
        printf("    jmp .%s_end\n", node->pfunc->name);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    if (node->ty == ',') {
        return;
    }

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->ty) {
        case '+':
            if (node->rhs->cty->ty == PTR || node->rhs->cty->ty == ARY) {
                printf("    push rdi\n");
                int coeff = 0;
                if (node->rhs->cty->ty == ARY) 
                    coeff = node->rhs->cty->align;
                else
                    coeff = node->rhs->cty->ptrto->align;
                printf("    mov rdi, %d\n", coeff);
                printf("    mul rdi\n");
                printf("    pop rdi\n");                
            } else if (node->lhs->cty->ty == PTR || node->lhs->cty->ty == ARY) {
                printf("    push rax\n");
                printf("    mov rax, rdi\n");
                int coeff = 0;
                if (node->lhs->cty->ty == ARY) 
                    coeff = node->lhs->cty->align;
                else
                    coeff = node->lhs->cty->ptrto->align;                
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
                    coeff = node->lhs->cty->align;
                else
                    coeff = node->lhs->cty->ptrto->align;  
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

    if (!node->no_push) 
        printf("    push rax\n");
    return;
}


static char *escape(char *s, int len) {
  char *buf = calloc(1, len * 4);
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

void gen_x86(Vector *code, Vector *global_vars) {
    printf(".intel_syntax noprefix\n");


    printf(".data\n");
    // Global variable
    for (int i = 0; i < global_vars->len; i++) {
        Var *var = global_vars->data[i];
        if (var->is_extern)
            continue;
        printf("%s:\n", var->name);
        printf("    .ascii \"%s\"\n", escape(var->data, var->len));          
    }

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

    printf(".text\n");
    // Functions
    for (int i = 0; i < code->len; i++) {
        if (((Node *)code->data[i])->ty == ND_VAR_DEF) 
            continue;
        gen(code->data[i]);
    }

}