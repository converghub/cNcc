#include "cncc.h"

extern Vector *strings;

static Type int_cty = {INT, 4, 4, NULL};

// Block scope
typedef struct Blk {
    Map *vars; 
    struct Blk *superset;
} Blk;

static Blk *blk;
static int stacksize;

static Blk *new_blk(Blk *superset) {
    Blk *blk = calloc(1, sizeof(Blk));
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
    Var *var = calloc(1, sizeof(Var));
    var->cty = cty;
    var->is_local = false;
    var->name = name;
    var->data = data;
    var->len = len;
    return var;
}

static void check_lval(Node *node) {
  int ty = node->ty;
  if (ty == ND_LVAR || ty == ND_GVAR || ty == ND_DEREF || ty == ND_DOT)
    return;
  error("not an lvalue: %d (%s)", ty, node->name);
}

static bool check_push(Node *node) {
    if (node == NULL) 
        error("check_push(): pointer to node is NULL.");
    if (node->upper == NULL) 
        error("check_push(): pointer to upper node is NULL.");

    switch (node->ty) {
        case ND_VAR_DEF:
        case ND_EXPR_STMT:
        case ND_CMPD_STMT:
        case ND_IF:
        case ND_WHILE:
        case ND_DO_WHILE:
        case ND_FOR:
            switch (node->upper->ty) {
                case ND_FUNC_DEF:
                    return true;
                case ND_IF:
                case ND_WHILE:
                case ND_DO_WHILE:
                case ND_FOR:
                    return false;
                case ND_CMPD_STMT:
                    return check_push(node->upper);
                case ND_STMT_EXPR:
                    if (node->is_last) return false;
                    return true;
            }
            error("check_push() Type %d : Unknown upper node type %d", node->ty, node->upper->ty);
        case ND_STMT_EXPR:
        case ND_NUM:
        case ND_SIZEOF:
        case ND_ALIGNOF:
        case ND_LVAR:
        case ND_GVAR:
        case ND_FUNC_CALL:
        case ND_DEREF:
        case ND_DOT:
        case '!':   
        case '%':
        case '*':
        case '+':
        case ',':
        case '-':
        case '/':
        case '<':
        case '=':
        case '>':
        case '?':
        case '^':
        case '|':
        case '~':
        case ND_ADDR:
        case ND_EQ:
        case ND_NE:
        case ND_LE:
        case ND_GE:
        case ND_LOR:
        case ND_LAND:
        case ND_SHL:
        case ND_SHR:
        case ND_PRE_INC:
        case ND_PRE_DEC:
        case ND_POST_INC:
        case ND_POST_DEC:
            switch (node->upper->ty) {
                case ND_EXPR_STMT:
                    return check_push(node->upper);
                case ',':
                    if (node->is_last) return false;
                    return true;
                case ND_FUNC_CALL:
                case ND_VAR_DEF:
                case ND_IF:
                case ND_WHILE:
                case ND_DO_WHILE:
                case ND_FOR:
                case ND_DEREF:
                case ND_ADDR:
                case ND_DOT:
                case ND_RETURN:
                case ND_SIZEOF:
                case ND_ALIGNOF:
                case '!':
                case '%':
                case '*':
                case '+':
                case '-':
                case '/':
                case '<':
                case '=':
                case '>':
                case '?':
                case '^':
                case '|':
                case '~':
                case ND_EQ:
                case ND_NE:
                case ND_LE:
                case ND_GE:
                case ND_LOR:
                case ND_LAND:
                case ND_SHL:
                case ND_SHR:
                case ND_PRE_INC:
                case ND_PRE_DEC:
                case ND_POST_INC:
                case ND_POST_DEC:
                    return false;
            }
            error("check_push() Type %d : Unknown upper node type %d", node->ty, node->upper->ty);
        case ND_RETURN:
            switch (node->upper->ty) {
                case ND_CMPD_STMT:
                    return check_push(node->upper);
            }            
            error("check_push() ND_RETURN: Unknown upper %d", node->upper->ty);
        default:
            error("check_push() : Unknown node type coming: %d", node->ty);
            exit(0);
    }
}

static Node *walk(Node *node, Node *pfunc, Node *upper) {
    node->pfunc = pfunc;
    node->upper = upper;
    node->no_push = false;

    switch(node->ty) {
        case ND_NUM:
            node->no_push = check_push(node);
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

            if (sema_var->cty->ty == ARY && upper->ty != ND_SIZEOF && upper->ty != ND_ALIGNOF) {
                node = addr_of(node, sema_var->cty->aryof);
                node->pfunc = pfunc;
                node->upper = upper;
            } else
                node->cty = sema_var->cty;   

            node->no_push = check_push(node);      
            return node;
        }
        case ND_VAR_DEF: {
            // Local variable definition
            stacksize = roundup(stacksize, node->cty->align);
            stacksize += node->cty->size;
            node->offset = stacksize;

            Var *var = calloc(1, sizeof(Var));
            var->cty = node->cty;
            var->is_local = true;
            var->offset = stacksize;
            map_put(blk->vars, node->name, var);

            if (node->init)
                node->init = walk(node->init, pfunc, node);
            
            node->no_push = check_push(node);
            return node;
        }
        case ND_IF:
            node->bl_expr = walk(node->bl_expr, pfunc, node);
            node->tr_stmt = walk(node->tr_stmt, pfunc, node);
            if (node->els_stmt)
                node->els_stmt = walk(node->els_stmt, pfunc, node);
            node->no_push = check_push(node);
            return node;
        case ND_FOR:
            node->init = walk(node->init, pfunc, node);
            node->bl_expr = walk(node->bl_expr, pfunc, node);
            node->inc = walk(node->inc, pfunc, node);
            node->body = walk(node->body, pfunc, node);
            node->no_push = check_push(node);
            return node;
        case ND_WHILE:
        case ND_DO_WHILE:
            node->bl_expr = walk(node->bl_expr, pfunc, node);
            node->body = walk(node->body, pfunc, node);
            node->no_push = check_push(node);
            return node;
        case '+':
        case '-':
            node->lhs = walk(node->lhs, pfunc, node);
            node->rhs = walk(node->rhs, pfunc, node);
            node->no_push = check_push(node);

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
            node->lhs = walk(node->lhs, pfunc, node);
            check_lval(node->lhs);
            node->rhs = walk(node->rhs, pfunc, node);
            node->cty = node->lhs->cty;
            node->no_push = check_push(node);
            return node;
        case '?':
            node->bl_expr = walk(node->bl_expr, pfunc, node);
            node->tr_stmt = walk(node->tr_stmt, pfunc, node);
            node->els_stmt = walk(node->els_stmt, pfunc, node);
            node->cty = node->tr_stmt->cty;
            node->no_push = check_push(node);
            return node;
        case ND_DOT:
            node->expr = walk(node->expr, pfunc, node);
            if (node->expr->cty->ty != STRUCT)
                error("sema(): struct type expected before '.'");

            Type *cty = node->expr->cty;
            for (int i = 0; i < cty->mbrs->len; i++) {
                Node *mbr = cty->mbrs->data[i];
                if (strcmp(mbr->name, node->mbr_name))
                    continue;
                node->cty = mbr->cty;
                node->offset = mbr->cty->offset;
                node->no_push = check_push(node);
                return node;
            }
            error("sema(): member missing %s", node->mbr_name);
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
            node->lhs = walk(node->lhs, pfunc, node);
            node->rhs = walk(node->rhs, pfunc, node);
            node->cty = node->lhs->cty;
            node->no_push = check_push(node);
            return node;
        case ND_ADDR:
            node->expr = walk(node->expr, pfunc, node);
            check_lval(node->expr);
            node->cty = ptr_to(node->expr->cty);
            node->no_push = check_push(node);
            return node;
        case ND_DEREF:
            node->expr = walk(node->expr, pfunc, node);
            if (node->expr->cty->ty != PTR)
                error("sema(): operand must be a pointer");   
            node->cty = node->expr->cty->ptrto;
            node->no_push = check_push(node);
            return node;
        case ND_PRE_INC:
        case ND_PRE_DEC:
        case ND_POST_INC:
        case ND_POST_DEC:
        case '!':
        case '~':
            node->expr = walk(node->expr, pfunc, node);
            node->cty = node->expr->cty;
            node->no_push = check_push(node);
            return node;
        case ',':
            node->lhs = walk(node->lhs, pfunc, node);
            node->rhs = walk(node->rhs, pfunc, node);
            node->cty = node->rhs->cty;
            node->no_push = check_push(node);
            return node;
        case ND_RETURN:
            node->expr = walk(node->expr, pfunc, node);
            return node;
        case ND_SIZEOF:
            node->expr = walk(node->expr, pfunc, node);

            Node *ret = calloc(1, sizeof(Node));
            ret->ty = ND_NUM;
            ret->cty = &int_cty;
            ret->val = node->expr->cty->size;
            node->expr = ret;
            node->no_push = check_push(node);
            return node->expr;
        case ND_ALIGNOF:
            node->expr = walk(node->expr, pfunc, node);

            Node *ret1 = calloc(1, sizeof(Node));
            ret1->ty = ND_NUM;
            ret1->cty = &int_cty;
            ret1->val = node->expr->cty->align;
            node->expr = ret1;
            node->no_push = check_push(node);
            return node->expr;
        case ND_FUNC_DEF:
            for (int i = 0; i < node->args->len; i++)
                node->args->data[i] = walk(node->args->data[i], node, node);
            node->body = walk(node->body, node, node);
            return node;
        case ND_FUNC_CALL:
            for (int i = 0; i <node->args->len; i++)
                node->args->data[i] = walk(node->args->data[i], pfunc, node);
            // cutrrently only "int" is supported
            node->cty = &int_cty;
            node->no_push = check_push(node);
            return node;
        case ND_CMPD_STMT:
            blk = new_blk(blk);
            for (int i = 0; i < node->stmts->len; i++)
                node->stmts->data[i] = walk(node->stmts->data[i], pfunc, node);
            node->no_push = check_push(node);
            blk = blk->superset;
            return node;
        case ND_EXPR_STMT:
            node->expr = walk(node->expr, pfunc, node);
            node->cty = node->expr->cty;
            node->no_push = check_push(node);
            return node;
        case ND_STMT_EXPR: {
            blk = new_blk(blk);
            Node *expr_node = node->stmt_expr;
            for (int i = 0; i < expr_node->stmts->len; i++) {
                if (i == expr_node->stmts->len - 1)
                    ((Node *)expr_node->stmts->data[i])->is_last = true;
                expr_node->stmts->data[i] = walk(expr_node->stmts->data[i], pfunc, node);        
            }

            if (expr_node->stmts->len > 0) {
                Node *last = expr_node->stmts->data[expr_node->stmts->len - 1];
                if (last->ty != ND_EXPR_STMT)
                    error("sema(): The last thing in Statement expressions should be an expression");                
                node->cty = last->cty;
            } else {
                // TODO: cutrrently "int", but should be "void"
                node->cty = &int_cty;
            }
            node->no_push = check_push(node);
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
        walk(node, NULL, NULL);
        node->stacksize = stacksize;
    }
    return nodes;
}