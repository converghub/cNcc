#include "ccc.h"

#define GET_TK(TK, POS) ((Token *)(TK)->data[(POS)])

static Vector *tokens;
static int pos;



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

    if (GET_TK(tokens, pos)->ty == TK_RETURN || GET_TK(tokens, pos)->ty == ';') {
        if (strchr("=", *GET_TK(tokens, pos)->input)) 
            error("'='が不適切な場所に含まれています\n");        
        return NULL;
    }    

    error("不適切なトークンです: %s", GET_TK(tokens, pos)->input);

    return NULL;
}


static Node *mul() {
    Node *node = term();

    for (;;) {
        if (consume('*'))
            node = new_node('*', node, term());
        else if (consume('/'))
            node = new_node('/', node, term());
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

static Node *equality() {
    Node *node = add();

    for (;;) {
        if (consume(TK_EQ)) 
            node = new_node(ND_EQ, node, add());
        else if (consume(TK_NE))
            node = new_node(ND_NE, node, add());
        else
            return node;
    }
}

static Node *assign() {
    Node *node = equality();

    if (consume('='))
        return new_node('=', node, assign());
    else
        return node;
}


static Node *stmt() {
    Node *node = assign();

    for (;;) {
        if (GET_TK(tokens, pos)->ty == TK_EOF)
            return node;
        else if (consume(';')) {
            return node;
        }
        else if (consume(TK_RETURN)) {
            node = new_node(TK_RETURN, NULL, equality());
            expect(';');
            return node;
        }
    }

    if (!consume(';'))
        error("';'ではないトークンです: %s", GET_TK(tokens, pos)->input); 
    
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

    if (GET_TK(tokens, pos)->ty != TK_IDENT)
        error("function() : Name expected, but got %s\n", GET_TK(tokens, pos)->input);
    node->name = GET_TK(tokens, pos)->name;
    pos++;

    expect('(');
    while (GET_TK(tokens, pos)->ty != (')'))
        vec_push(node->args, term());
    expect(')');

    expect('{');
    node->body = cmpd_stmt();
    expect('}');

    return node;
}


Vector *parse(Vector *tk) {
    tokens = tk;

    Vector *v = new_vector();

    while (GET_TK(tokens, pos)->ty != TK_EOF)
        vec_push(v, function());
    return v;
}