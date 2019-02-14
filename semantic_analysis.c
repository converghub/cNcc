#include "ccc.h"

extern Vector *strings;

static Type int_cty = {INT, NULL};

// Block scope
typedef struct Blk {
    Map *vars; 
    struct Blk *superset;
} Blk;

static Blk *blk;
static int stacksize;

static Blk *new_blk(Blk *superset) {
    Blk *blk = malloc(sizeof(Blk));
    blk->vars = new_map();
    blk->superset = superset;
    return blk;
}

static Var *find_var(char *name) {
    for (Blk *fblk = blk; fblk; fblk = fblk->superset) {
        Var *var = map_get(fblk->vars, name);
        if (var)
            return var;
    }
    return NULL;
}

static Var *new_global(Type *cty, char *name, char *data, int len) {
    Var *var = malloc(sizeof(Var));
    var->cty = cty;
    var->is_local = false;
    var->name = name;
    var->data = data;
    var->len = len;
    return var;
}

static void check_lval(Node *node) {
  int ty = node->ty;
  if (ty == ND_LVAR || ty == ND_GVAR || ty == ND_DEREF)
    return;
  error("not an lvalue: %d (%s)", ty, node->name);
}


static Node *walk(Node *node,...) {
    va_list parent_node;
    va_start(parent_node, node);

    switch(node->ty) {
        case ND_NUM:
            return node;
        case ND_STR:
            return node;
        case ND_IDENT: {
            Var *sema_var = find_var(node->name);
            if (!sema_var)
                error("sema(), ND_IDENT: undefined variable: %s", node->name);

            if (sema_var->is_local) {
                node->ty = ND_LVAR;
                node->offset = sema_var->offset;
            } else
                node->ty = ND_GVAR;

            Node *parent = va_arg(parent_node, Node*);
            if (sema_var->cty->ty == ARY && parent->ty != ND_SIZEOF && parent->ty != ND_ALIGNOF)
                node = addr_of(node, sema_var->cty->aryof);
            else
                node->cty = sema_var->cty;         
            return node;
        }
        case ND_VAR_DEF: {
            // Local variable definition
            stacksize += size_of(node->cty);
            node->offset = stacksize;

            Var *var = malloc(sizeof(Var));
            var->cty = node->cty;
            var->is_local = true;
            var->offset = stacksize;
            map_put(blk->vars, node->name, var);

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
                error("sema(): <pointer> %c <pointer> in not supported", node->ty);
            if (node->lhs->cty->ty == PTR) 
                node->cty = node->lhs->cty;
            else if (node->rhs->cty->ty == PTR) 
                node->cty = node->rhs->cty;
            else
                node->cty = node->lhs->cty;
            return node;
        case '=':
            node->lhs = walk(node->lhs, node);
            check_lval(node->lhs);
            node->rhs = walk(node->rhs);
            node->cty = node->lhs->cty;
            return node;
        case '?':
            node->bl_expr = walk(node->bl_expr);
            node->tr_stmt = walk(node->tr_stmt);
            node->els_stmt = walk(node->els_stmt);
            node->cty = node->tr_stmt->cty;
            return node;
        case '*':
        case '/':
        case '%':
        case '<':
        case '>':
        case '|':
        case '^':
        case ND_EQ:
        case ND_NE:
        case ND_LE:
        case ND_GE:
        case ND_LAND:
        case ND_LOR:
        case ND_SHL:
        case ND_SHR:
            node->lhs = walk(node->lhs);
            node->rhs = walk(node->rhs);
            node->cty = node->lhs->cty;
            return node;
        case ND_ADDR:
            node->expr = walk(node->expr);
            check_lval(node->expr);
            node->cty = ptr_to(node->expr->cty);
            return node;
        case ND_DEREF:
            node->expr = walk(node->expr);
            if (node->expr->cty->ty != PTR)
                error("sema(): operand must be a pointer");   
            node->cty = node->expr->cty->ptrto;
            return node;
        case ND_PRE_INC:
        case ND_PRE_DEC:
        case ND_POST_INC:
        case ND_POST_DEC:
        case '!':
        case '~':
            node->expr = walk(node->expr);
            node->cty = node->expr->cty;
            return node;
        case ',':
            node->lhs = walk(node->lhs);
            node->rhs = walk(node->rhs);
            node->cty = node->rhs->cty;
            return node;
        case ND_RETURN:
            node->expr = walk(node->expr);
            return node;
        case ND_SIZEOF:
            node->expr = walk(node->expr, node);

            Node *ret = malloc(sizeof(Node));
            ret->ty = ND_NUM;
            ret->cty = &int_cty;
            ret->val = size_of(node->expr->cty);
            node->expr = ret;
            return node->expr;
        case ND_ALIGNOF:
            node->expr = walk(node->expr, node);

            Node *ret1 = malloc(sizeof(Node));
            ret1->ty = ND_NUM;
            ret1->cty = &int_cty;
            ret1->val = align_of(node->expr->cty);
            node->expr = ret1;
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
            blk = new_blk(blk);
            for (int i = 0; i < node->stmts->len; i++)
                node->stmts->data[i] = walk(node->stmts->data[i]);
            blk = blk->superset;
            return node;
        case ND_EXPR_STMT:
            node->expr = walk(node->expr);
            node->cty = node->expr->cty;
            return node;
        case ND_STMT_EXPR: {
            blk = new_blk(blk);
            Node *expr_node = node->stmt_expr;
            for (int i = 0; i < expr_node->stmts->len; i++)
                expr_node->stmts->data[i] = walk(expr_node->stmts->data[i]);        

            if (expr_node->stmts->len > 0) {
                Node *last = expr_node->stmts->data[expr_node->stmts->len - 1];
                if (last->ty != ND_EXPR_STMT)
                    error("sema(): The last thing in Statement expressions should be an expression");                
                node->cty = last->cty;
            } else {
                // TODO: cutrrently "int", but should be "void"
                node->cty = &int_cty;
            }
            blk = blk->superset;
            return node;
        }
        case ND_NULL:
            return node;
        default:
            assert(0 && "unknown node type");
    }
}


Vector *sema(Vector *nodes, Vector *global_vars) {
    blk = new_blk(NULL);

    for (int i = 0; i < nodes->len; i++){
        Node *node = nodes->data[i];
        
        if (node->ty == ND_VAR_DEF) {
            // global variables
            Var *var = new_global(node->cty, node->name, node->data, node->len);
            var->is_extern = node->is_extern;
            vec_push(global_vars, var);
            map_put(blk->vars, node->name, var);
            continue;
        }

        assert(node->ty == ND_FUNC_DEF);
        // function
        stacksize = 0;
        walk(node);
        node->stacksize = stacksize;
    }
    return nodes;
}