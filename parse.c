#include "ccc.h"

#define GET_TK(TK, POS) ((Token *)(TK)->data[(POS)])

static Vector *tokens;
static int pos;
static int stacksize;
static Type int_cty = {INT, NULL};

// for codegen.c
Map *vars;

static void expect(int ty) {
    Token *t = tokens->data[pos];
    if (t->ty != ty)
        error("%c (%d) expected, but got %c (%d)", ty, ty, t->ty, t->ty);    
    pos++;
}

static int consume(int ty) {
    if (GET_TK(tokens, pos)->ty != ty)
        return 0;
    pos++;
    return 1;
}

static Type *ctype(int CTYPE) {
    if (CTYPE != TK_INT)
        error("ctype(): Typename expected, but got %s\n", GET_TK(tokens, pos)->input);

    Type *ctype = &int_cty;
    while (consume('*'))
        ctype = ptr_of(ctype);
    return ctype;
}

// Node
static Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->cty = &int_cty;
    node->val = val;
    return node;
}

static Node *new_node_ident(char *name) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_IDENT;
    node->name = name;
    return node;    
}


static Node *assign();
static Node *mul();
static Node *stmt();
static Node *cmpd_stmt();


static Node *term() {
    if (consume('(')) {
        Node *node = assign();

        if (!consume(')'))
            error("term(): 開きカッコに対応する閉じカッコがありません: %s",
                GET_TK(tokens, pos)->input);
        return node;
    }

    if (GET_TK(tokens, pos)->ty == TK_NUM)
        return new_node_num(GET_TK(tokens, pos++)->val);

    if (GET_TK(tokens, pos)->ty == TK_IDENT) {
        if (GET_TK(tokens, pos + 1)->ty == '(') {
            // function
            Node *node = malloc(sizeof(Node));
            node->ty = ND_FUNC_CALL;
            node->name = GET_TK(tokens, pos)->name;
            node->args = new_vector();
            pos += 2;

            // no orgs
            if (consume(')'))
                return node;

            // if args exist
            vec_push(node->args, assign());
            while (consume(','))
                vec_push(node->args, assign());
            expect(')');
            return node; 
        }
        
        return new_node_ident(GET_TK(tokens, pos++)->name);       
    }

    error("term() 不適切なトークンです: %s", GET_TK(tokens, pos)->input);
    return NULL;
}


static Node *postfix() {
    Node *lhs = term();
    while (consume('[')) {
        // TODO: fill this
        Node *node = malloc(sizeof(Node));
        node->ty = ND_DEREF;
        node->rhs = new_node('+', lhs, term());
        lhs = node;
        expect(']');
    }
    return lhs;
}


static Node *unary() {
    if (consume('*')) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_DEREF;
        node->rhs = mul();
        return node;
    }
    if (consume('&')) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_ADDR;
        node->rhs = mul();
        return node;
    }
    if (consume(TK_SIZEOF)) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_SIZEOF;
        node->rhs = unary();
        if (node->rhs->ty == ND_IDENT) {
            Var *var = map_get(vars, node->rhs->name);
            node->rhs->cty = var->cty;
        }
        Node *ret = malloc(sizeof(Node));
        ret->ty = ND_NUM;
        ret->cty = INT;
        ret->val = size_of(node->rhs->cty);
        node->rhs = ret;
        return node;
    }
    
    return postfix();
}


static Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume('*'))
            node = new_node('*', node, unary());
        else if (consume('/'))
            node = new_node('/', node, unary());
        else
            return node;
    }
}


static Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume('+'))
            node = new_node('+', node, mul());
        else if (consume('-'))
            node = new_node('-', node, mul());
        else
            return node;
    }
}


static Node *rel() {
    Node *node = add();

    for (;;) {
        if (consume('<'))
            node = new_node('<', node, add());
        else if (consume('>'))
            node = new_node('>', node, add());
        else
            return node;
    }
}


static Node *equality() {
    Node *node = rel();

    for (;;) {
        if (consume(TK_EQ)) 
            node = new_node(ND_EQ, node, rel());
        else if (consume(TK_NE))
            node = new_node(ND_NE, node, rel());
        else
            return node;
    }
}


static Node *land() {
    Node *node = equality();

    for (;;) {
        if (consume(TK_LAND))
            node = new_node(ND_LAND, node, land());
        else
            return node;        
    }
}


static Node *lor() {
    Node *node = land();

    for (;;) {
        if (consume(TK_LOR))
            node = new_node(ND_LOR, node, lor());
        else
            return node;
    }
}


static Node *assign() {
    Node *node = lor();

    if (consume('='))
        return new_node('=', node, assign());
    else
        return node;
}


static Node* decl(int CTYPE) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_VAR_DEF;

    // Set C type name
    node->cty = ctype(CTYPE);

    // Store identifer
    if (GET_TK(tokens, pos)->ty != TK_IDENT)
        error("variable name expected, but got %s", GET_TK(tokens, pos)->input);
    node->name = GET_TK(tokens, pos)->name;
    pos++;

    // Check array 
    Vector *ary_size = new_vector();
    while (consume('[')) {
        Node *len = term();
        if (len->ty != ND_NUM)
            error("decl(): number expected.\n");
        vec_push(ary_size, len);   
        expect(']');
    }
    for (int i = ary_size->len - 1; i >= 0; i--) {
        Node *len_node = ary_size->data[i];
        node->cty = ary_of(node->cty, len_node->val);
    }

    // Set stacksize
    stacksize += size_of(node->cty);  
    Var *var = malloc(sizeof(Var));
    var->offset = stacksize;
    var->cty = node->cty;
    map_put(vars, node->name, var);

    // Check initializer
    if (consume('='))
        node->init = assign();
    return node;
}


static Node *stmt() {
    Node *node = malloc(sizeof(Node));

    if (consume(TK_IF)) {
        node->ty = ND_IF;
        // condition
        expect('(');
        node->bl_expr = assign();
        expect(')');
        // if true statements
        if (consume('{')) {
            node->tr_stmt = cmpd_stmt();
            expect('}');
        } else {
            node->tr_stmt = stmt();
        }
        // else statements
        if (consume(TK_ELSE)) {
            if (consume('{')) {
                node->els_stmt = cmpd_stmt();
                expect('}');
            } else {
                node->els_stmt = stmt();
            }
        }
        return node;

    } else if (consume(TK_INT)) {
        node = decl(TK_INT);
        expect(';');
        return node;

    } else if (consume(TK_RETURN)) {
        node = new_node(TK_RETURN, NULL, lor());
        expect(';');
        return node;

    } else if (consume(TK_WHILE)) {
        node->ty = ND_WHILE;
        expect('(');
        node->bl_expr = assign();
        expect(')');
        if(consume('{')) {
            node->body = cmpd_stmt();
            expect('}');
        } else {
            node->body = stmt();
        }
        return node;

    } else if (consume(TK_FOR)) {
        node->ty = ND_FOR;
        expect('(');
        if (consume(TK_INT))
            node->init = decl(TK_INT);
        else
            node->init = assign();
        expect(';');
        node->bl_expr = assign();
        expect(';');
        node->inc = assign();
        expect(')');
        if(consume('{')) {
            node->body = cmpd_stmt();
            expect('}');
        } else {
            node->body = stmt();
        }
        return node;

    } else {
        node = assign();
        expect(';');
        return node;
    }
}


static Node *cmpd_stmt() {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_CMPD_STMT;
    node->stmts = new_vector();

    while (GET_TK(tokens, pos)->ty != '}')
        vec_push(node->stmts, stmt());
    return node;
}


static Node *function() {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_FUNC_DEF;
    node->args = new_vector();
    stacksize = 0;

    if (GET_TK(tokens, pos)->ty != TK_INT) 
        error("function-return type expected, but got %s", GET_TK(tokens, pos)->input);
    pos++;

    if (GET_TK(tokens, pos)->ty != TK_IDENT)
        error("function() : Name expected, but got %s\n", GET_TK(tokens, pos)->input);
    node->name = GET_TK(tokens, pos)->name;
    pos++;

    expect('(');
    int argc = 0;
    while (GET_TK(tokens, pos)->ty != (')')) {
        if (consume(TK_INT))
            vec_push(node->args, term());
        else
            error("function(): Argument type expected, but got %s.\n", GET_TK(tokens, pos)->input);

        if (!map_exist(vars, ((Node *)node->args->data[argc])->name)) {
            ((Node *)node->args->data[argc])->cty = ctype(TK_INT);
            stacksize += size_of( ((Node *)node->args->data[argc])->cty );
            Var *var = malloc(sizeof(Var));
            var->cty = ((Node *)node->args->data[argc])->cty;
            var->offset = stacksize;
            map_put(vars, ((Node *)node->args->data[argc++])->name, var);
        }

        if (consume(',')) {
            if (GET_TK(tokens, pos)->ty == (')'))
                error("function() ','が不適切な場所にあります\n") ;           
            continue;
        }
    }
    expect(')');

    expect('{');
    node->body = cmpd_stmt();
    expect('}');

    return node;
}


Vector *parse(Vector *tk) {
    tokens = tk;
    Vector *v = new_vector();
    vars = new_map();
    int func_counter = 0;

    while (GET_TK(tokens, pos)->ty != TK_EOF) {
        vec_push(v, function());
        ((Node* )v->data[func_counter++])->stacksize = stacksize;
    }
    return v;
}