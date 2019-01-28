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

static Type *ptrof (Type *base) {
    Type *ctype = malloc(sizeof(Type));
    ctype->ty = PTR;
    ctype->ptrof = base;
    return ctype;
}

static Type *ctype(int CTYPE) {
    if (CTYPE != TK_INT)
        error("ctype(): Typename expected, but got %s\n", GET_TK(tokens, pos)->input);

    Type *ctype = &int_cty;
    while (consume('*'))
        ctype = ptrof(ctype);
    return ctype;
}

int size_of(Type *ctype) {
    if (ctype->ty == INT) 
        return 4;
    else if (ctype->ty == PTR)
        return 8;
    else
        error("size_of(): invalid ty value.\n");
        return 0;
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
    return term();
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


static Node* decl() {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_VAR_DEF;
    if (GET_TK(tokens, pos)->ty != TK_IDENT)
        error("variable name expected, but got %s", GET_TK(tokens, pos)->input);
    node->name = GET_TK(tokens, pos)->name;

    stacksize += 8;
    Var *var = malloc(sizeof(Var));
    var->cty = node->cty;
    var->offset = stacksize;
    map_put(vars, node->name, var);
    pos++;

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
        node->cty = ctype(TK_INT);
        node = decl();
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
            node->init = decl();
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
            stacksize += 8;
            Var *var = malloc(sizeof(Var));
            var->cty = node->cty;
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