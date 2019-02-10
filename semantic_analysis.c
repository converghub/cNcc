#include "ccc.h"

extern Map *vars;
extern Vector *strings;

static Type int_cty = {INT, NULL};

static Node *walk(Node *node) {
    switch(node->ty) {
        case ND_NUM:
            return node;
        case ND_STR:
            return node;
        case ND_IDENT: {
            Var *sema_var = map_get(vars, node->name);
            if (!sema_var)
                error("sema(): undefined variable: %s", node->name);
            
            if (sema_var->cty->ty == ARY)
                node = addr_of(node, sema_var->cty->aryof);
            else
                node->cty = sema_var->cty;            
            return node;
        }
        case ND_VAR_DEF: {
            Var *sema_var = map_get(vars, node->name);
            if (!sema_var)
                error("sema(): undefined variable: %s", node->name);

            if (node->init)
                node->init = walk(node->init);
            return node;
        }
        case ND_IF:
            node->bl_expr = walk(node->bl_expr);
            node->tr_stmt = walk(node->tr_stmt);
            if (node->els_stmt)
                node->els_stmt = walk(node->els_stmt);
            return node;
        case ND_FOR:
            node->init = walk(node->init);
            node->bl_expr = walk(node->bl_expr);
            node->inc = walk(node->inc);
            node->body = walk(node->body);
            return node;
        case ND_WHILE:
        case ND_DO_WHILE:
            node->bl_expr = walk(node->bl_expr);
            node->body = walk(node->body);
            return node;
        
        case '+':
        case '-':
            node->lhs = walk(node->lhs);
            node->rhs = walk(node->rhs);

            if (node->lhs->cty->ty == PTR && node->rhs->cty->ty == PTR)
                error("<pointer> %c <pointer> in not supported", node->ty);
            if (node->lhs->cty->ty == PTR) 
                node->cty = node->lhs->cty;
            else if (node->rhs->cty->ty == PTR) 
                node->cty = node->rhs->cty;
            else
                node->cty = node->lhs->cty;
            return node;
        case '=':
            node->lhs = walk(node->lhs);
            if (node->lhs->ty != ND_IDENT && node->lhs->ty != ND_DEREF && node->lhs->ty != ND_ADDR)
                error("sema(): not an lvalue: %d (%s)", node->cty, node->name);
            node->rhs = walk(node->rhs);
            node->cty = node->lhs->cty;
            return node;
        case '*':
        case '/':
        case '<':
        case '>':
        case ND_EQ:
        case ND_NE:
        case ND_LE:
        case ND_GE:
        case ND_LAND:
        case ND_LOR:
            node->lhs = walk(node->lhs);
            node->rhs = walk(node->rhs);
            node->cty = node->lhs->cty;
            return node;
        case ND_ADDR:
            node->expr = walk(node->expr);
            node->cty = ptr_of(node->expr->cty);
            return node;
        case ND_DEREF:
            node->expr = walk(node->expr);
            if (node->expr->cty->ty != PTR)
                error("sema(): operand must be a pointer");   
            node->cty = node->expr->cty->ptrof;
            return node;
        case ND_RETURN:
            node->expr = walk(node->expr);
            return node;
        case ND_SIZEOF:
            node->expr = walk(node->expr);
            return node->expr;
        case ND_FUNC_DEF:
            for (int i = 0; i < node->args->len; i++)
                node->args->data[i] = walk(node->args->data[i]);
            node->body = walk(node->body);
            return node;
        case ND_FUNC_CALL:
            for (int i = 0; i <node->args->len; i++)
                node->args->data[i] = walk(node->args->data[i]);
            // cutrrently only "int" is supported
            node->cty = &int_cty;
            return node;
        case ND_CMPD_STMT:
            for (int i = 0; i < node->stmts->len; i++)
                node->stmts->data[i] = walk(node->stmts->data[i]);
            return node;
        case ND_EXPR_STMT:
            node->expr = walk(node->expr);
            return node;
        case ND_NULL:
            return node;
        default:
            assert(0 && "unknown node type");
    }
}


Vector *sema(Vector *nodes) {
    for (int i = 0; i < nodes->len; i++){
        Node *node = nodes->data[i];
        
        if (node->ty == ND_VAR_DEF)
            // global variables
            continue;
        else {
            // function
            assert(node->ty == ND_FUNC_DEF);
            walk(node);
        }
    }
    return nodes;
}