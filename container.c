#include "ccc.h"


void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}


static int expect(int line, int expected, int actual) {
    if (expected == actual) 
        return 0;
    fprintf(stderr, "%d: %d expected, but got %d\n",
            line, expected, actual);
    exit(1);
}



// Vector
Vector *new_vector() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * 16);
    vec->capacity = 16;
    vec->len = 0;
    return vec;
}

void vec_push(Vector *vec, void *elem) {
    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}


// Map
Map *new_map() {
    Map *map = malloc(sizeof(Map));
    map->keys = new_vector();
    map->vals = new_vector();
    return map;
}

void map_put(Map *map, char *key, void *val) {
    vec_push(map->keys, key);
    vec_push(map->vals, val);
}

void *map_get(Map *map, char *key) {
    for (int i = map->keys->len - 1; i >= 0; i--)
        if (strcmp(map->keys->data[i], key) == 0)
            return map->vals->data[i];
    return NULL;
}

bool map_exist(Map *map, char *key) {
    for (int i = 0; i < map->keys->len; i++)
        if (!strcmp(map->keys->data[i], key))
            return true;
    return false;
}


//
Type *ptr_of(Type *base) {
    Type *ctype = malloc(sizeof(Type));
    ctype->ty = PTR;
    ctype->ptrof = base;
    return ctype;
}

Type *ary_of(Type *base, int len) {
    Type *cty = malloc(sizeof(Type));
    cty->ty = ARY;
    cty->aryof = base;
    cty->len = len;
    return cty;
}

int size_of(Type *cty) {
    if (cty->ty == INT) 
        return 8;
    else if (cty->ty == PTR)
        return 8;
    else if (cty->ty == ARY)
        return size_of(cty->aryof) * cty->len;
    else
        error("size_of(): invalid ty value.\n");
        return 0;
}

Node *addr_of(Node *base, Type *cty) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_ADDR;
    node->cty = ptr_of(cty);
    node->rhs = base;
    return node;
}


/* test functions */ 

static void vec_test () {
    Vector *vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for (int i = 0; i < 100; i++)
        vec_push(vec, (void *)(intptr_t)i);

    expect(__LINE__, 100, vec->len);
    expect(__LINE__,  0, (intptr_t)vec->data[0]);
    expect(__LINE__, 50, (intptr_t)vec->data[50]);
    expect(__LINE__, 99, (intptr_t)vec->data[99]);

    fprintf(stdout, "vec_test() ...pass\n");
}


static void map_test() {
    Map *map = new_map();
    expect(__LINE__, 0, (intptr_t)map_get(map, "foo"));

    map_put(map, "foo", (void *)2);
    expect(__LINE__, 2, (intptr_t)map_get(map, "foo"));

    map_put(map, "bar", (void *)4);
    expect(__LINE__, 4, (intptr_t)map_get(map, "bar"));

    map_put(map, "foo", (void *)6);
    expect(__LINE__, 6, (intptr_t)map_get(map, "foo"));

    expect(__LINE__, true, map_exist(map, "foo"));
    expect(__LINE__, false, map_exist(map, "baz"));

    fprintf(stdout, "map_test() ...pass\n");
}


void run_test () {
    vec_test();
    map_test();
}