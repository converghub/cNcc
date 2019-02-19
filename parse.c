#include "cncc.h"

#define GET_TK(TK, POS) ((Token *)(TK)->data[(POS)])

static Vector *tokens;
static int pos;
static int globals_counter;
static Type int_cty = {INT, NULL};
static Type char_cty = {CHAR, NULL};
static Node null_stmt = {ND_NULL};

// for sema.c, codegen.c
Vector *strings;

static void expect(int ty) {
    Token *t = tokens->data[pos];
    if (t->ty != ty)
        error("%c (%d) expected, but got %c (%d). input: %s.", ty, ty, t->ty, t->ty, t->input);    
    pos++;
}

static int consume(int ty) {
    if (GET_TK(tokens, pos)->ty != ty)
        return 0;
    pos++;
    return 1;
}

static Type *get_ctype() {
    Token *t = GET_TK(tokens, pos);
    if (t->ty == TK_INT)
        return &int_cty;
    if (t->ty == TK_CHAR)
        return &char_cty;
    return NULL;
}

static Type *ctype() {
    Type *cty = get_ctype();
    if (!cty)
        error("ctype(): Typename expected, but got %s. tokens pos = %d.\n",
            GET_TK(tokens, pos)->input, pos);
    
    pos++;
    while (consume('*'))
        cty = ptr_to(cty);
    return cty;
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
static Node *comma();
static Node *stmt();
static Node *cmpd_stmt();


static Node *term() {
    if (consume('(')) {
        if (consume('{')) {
            Node *node = malloc(sizeof(Node));
            node->ty = ND_STMT_EXPR;
            node->stmt_expr = cmpd_stmt();
            expect('}');
            expect(')');
            return node;
        }


        Node *node = comma();
        expect(')');
        return node;
    }

    if (GET_TK(tokens, pos)->ty == TK_NUM)
        return new_node_num(GET_TK(tokens, pos++)->val);

    if (GET_TK(tokens, pos)->ty == TK_STR) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_STR;
        node->cty = ary_of(&char_cty, strlen(GET_TK(tokens, pos)->str));
        node->str = GET_TK(tokens, pos)->str;
        node->name = format(".G.str%d", globals_counter++);
        vec_push(strings, node);
        pos++;
        return node;
    }

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

    error("term(): incorrect token %s", GET_TK(tokens, pos)->input);
    return NULL;
}


static Node *postfix() {
    Node *lhs = term();

    for (;;) {
        if (consume(TK_INC)) {
            Node *node = malloc(sizeof(Node));
            node->ty = ND_POST_INC;
            node->expr = lhs;
            lhs = node;
            continue;
        }

        if (consume(TK_DEC)) {
            Node *node = malloc(sizeof(Node));
            node->ty = ND_POST_DEC;
            node->expr = lhs;
            lhs = node;
            continue;
        }

        if (consume('[')) {
            Node *node = malloc(sizeof(Node));
            node->ty = ND_DEREF;
            node->expr = new_node('+', lhs, assign());
            lhs = node;
            expect(']');
            continue;
        }

        return lhs;
    }
}


static Node *unary() {
    if (consume('*')) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_DEREF;
        node->expr = unary();
        return node;
    }
    if (consume('&')) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_ADDR;
        node->expr = unary();
        return node;
    }
    if (consume('!')) {
        Node *node = malloc(sizeof(Node));
        node->ty = '!';
        node->expr = unary();
        return node;
    }
    if (consume('~')) {
        Node *node = malloc(sizeof(Node));
        node->ty = '~';
        node->expr = unary();
        return node;        
    }
    if (consume(TK_INC)) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_PRE_INC;
        node->expr = unary();
        return node;             
    }
    if (consume(TK_DEC)) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_PRE_DEC;
        node->expr = unary();
        return node;             
    }
    if (consume(TK_SIZEOF)) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_SIZEOF;
        node->expr = unary();
        return node;
    }
    if(consume(TK_ALIGNOF)) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_ALIGNOF;
        node->expr = unary();
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
        else if (consume('%'))
            node = new_node('%', node, unary());            
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


static Node *shift() {
    Node *node = add();

    for (;;) {
        if (consume(TK_SHL))
            node = new_node(ND_SHL, node, add());
        else if (consume(TK_SHR))
            node = new_node(ND_SHR, node, add());
        else
            return node;
    }
}


static Node *rel() {
    Node *node = shift();

    for (;;) {
        if (consume('<'))
            node = new_node('<', node, shift());
        else if (consume('>'))
            node = new_node('>', node, shift());
        else if (consume(TK_LE))
            node = new_node(ND_LE, node, shift());
        else if (consume(TK_GE))
            node = new_node(ND_GE, node, shift());
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


static Node *b_xor() {
    Node *node = equality();

    for (;;) {
        if (consume('^'))
            node = new_node('^', node, equality());
        else
            return node;        
    }
}


static Node *b_or() {
    Node *node = b_xor();

    for (;;) {
        if (consume('|'))
            node = new_node('|', node, b_xor());
        else
            return node;        
    }
}


static Node *land() {
    Node *node = b_or();

    for (;;) {
        if (consume(TK_LAND))
            node = new_node(ND_LAND, node, b_or());
        else
            return node;        
    }
}


static Node *lor() {
    Node *node = land();

    for (;;) {
        if (consume(TK_LOR))
            node = new_node(ND_LOR, node, land());
        else
            return node;
    }
}


static Node *cndtnl() {
    Node *node = lor();

    if (consume('?')) {
        Node *cnode = malloc(sizeof(Node));
        cnode->ty = '?';
        cnode->bl_expr = node;
        cnode->tr_stmt = assign();
        expect(':');
        cnode->els_stmt = assign();
        return cnode;
    } else
        return node;
}


static Node *assign() {
    Node *node = cndtnl();

    if (consume('='))
        return new_node('=', node, assign());
    else
        return node;
}


static Node *comma() {
    Node *node = assign();

    for (;;) {
        if (consume(',')) {
            node->is_last =false;
            node = new_node(',', node, comma());
        } else {
            node->is_last = true;
            return node;
        }
    }
}


static Type *read_array(Type *cty) {
    Vector *ary_size = new_vector();
    while (consume('[')) {
        Node *len = comma();
        if (len->ty != ND_NUM)
            error("decl(): number expected.\n");
        vec_push(ary_size, len);   
        expect(']');
    }
    for (int i = ary_size->len - 1; i >= 0; i--) {
        Node *len_node = ary_size->data[i];
        cty = ary_of(cty, len_node->val);
    }

    return cty;
}


static Node* decl() {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_VAR_DEF;

    // Set C type name
    node->cty = ctype();

    // Store identifer
    if (GET_TK(tokens, pos)->ty != TK_IDENT)
        error("variable name expected, but got %s", GET_TK(tokens, pos)->input);
    node->name = GET_TK(tokens, pos)->name;
    pos++;

    // Check & read array
   node->cty = read_array(node->cty);

    // Check initializer
    // TODO: add commas act as seperators in this line, not as an operator
    if (consume('='))
        node->init = assign();
    else
        node->init = NULL;
    return node;
}


static Node *stmt() {
    Node *node = malloc(sizeof(Node));

    if (consume(TK_IF)) {
        node->ty = ND_IF;
        // condition
        expect('(');
        node->bl_expr = comma();
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
        } else
            node->els_stmt = NULL;
        return node;

    } else if (GET_TK(tokens, pos)->ty == TK_INT || GET_TK(tokens, pos)->ty == TK_CHAR) {
        node = decl();
        expect(';');
        return node;

    } else if (consume(TK_RETURN)) {
        node->ty = ND_RETURN;
        node->expr = comma();
        expect(';');
        return node;

    } else if (consume(TK_WHILE)) {
        node->ty = ND_WHILE;
        expect('(');
        node->bl_expr = comma();
        expect(')');
        if(consume('{')) {
            node->body = cmpd_stmt();
            expect('}');
        } else {
            node->body = stmt();
        }
        return node;

    } else if (consume(TK_DO)) {
        node->ty = ND_DO_WHILE;
        if(consume('{')) {
            node->body = cmpd_stmt();
            expect('}');
        } else {
            node->body = stmt();
        }
        expect(TK_WHILE);
        expect('(');
        node->bl_expr = comma();
        expect(')');
        expect(';');
        return node;

    } else if (consume(TK_FOR)) {
        node->ty = ND_FOR;
        expect('(');
        if (GET_TK(tokens, pos)->ty == TK_INT || GET_TK(tokens, pos)->ty == TK_CHAR) {
            node->init = decl();
        } else
            node->init = comma();
        expect(';');
        node->bl_expr = comma();
        expect(';');
        node->inc = comma();
        expect(')');
        if(consume('{')) {
            node->body = cmpd_stmt();
            expect('}');
        } else {
            node->body = stmt();
        }
        return node;
    } else if (consume('{')) {
        node->ty = ND_CMPD_STMT;
        node->stmts = new_vector();
        while (!consume('}'))
            vec_push(node->stmts, stmt());
        return node;
    } else if (consume(';')) {
        return &null_stmt;
    } else {
        node->ty = ND_EXPR_STMT;
        node->expr = comma();
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


static Node *top() {
    bool is_extern = consume(TK_EXTERN);
    strings = new_vector();
    Node *node = malloc(sizeof(Node));

    Type *cty = ctype();
    if (!cty)
        error("type name expected, but got %s", GET_TK(tokens, pos)->input);

    if (GET_TK(tokens, pos)->ty != TK_IDENT)
        error("top() : Name expected, but got %s\n", GET_TK(tokens, pos)->input);
    node->name = GET_TK(tokens, pos)->name;
    pos++;

    // Function
    if (consume('(')) {
        node->ty = ND_FUNC_DEF;
        node->args = new_vector();

        int argc = 0;
        while (GET_TK(tokens, pos)->ty != (')')) {
            Type *arg_cty = ctype();
            if (arg_cty)
                vec_push(node->args, term());
            else
                error("function(): Argument type expected, but got %s.\n", GET_TK(tokens, pos)->input);

            Node *argument = node->args->data[argc];
            argument->ty = ND_VAR_DEF;
            argument->cty = arg_cty;
            argument->init = NULL;
            argc++;

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
    
    // String literal
    if (strings->len != 0)
        node->strings = strings;
    else
        node->strings = NULL;    
    return node;
    }

    // Global variable
    node->ty = ND_VAR_DEF;
    node->cty = read_array(cty);
    if (is_extern) {
        node->is_extern = true;
    } else {
        node->data = calloc(1, size_of(node->cty));
        node->len = size_of(node->cty);
        node->is_extern = false;
    }
    node->strings = NULL;
    expect(';');

    return node;
}


Vector *parse(Vector *tk) {
    tokens = tk;
    Vector *v = new_vector();
    globals_counter = 0;

    while (GET_TK(tokens, pos)->ty != TK_EOF) {
        vec_push(v, top());
    }
    return v;
}