#include "cncc.h"


void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}


char *format(char *fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return strdup(buf);
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
    Vector *vec = calloc(1, sizeof(Vector));
    vec->data = calloc(1, sizeof(void *) * 16);
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
    Map *map = calloc(1, sizeof(Map));
    map->keys = new_vector();
    map->vals = new_vector();
    return map;
}

void map_put(Map *map, char *key, void *val) {
    vec_push(map->keys, key);
    vec_push(map->vals, val);
}

void map_puti(Map *map, char *key, int val) {
  map_put(map, key, (void *)(intptr_t)val);
}

void *map_get(Map *map, char *key) {
    for (int i = map->keys->len - 1; i >= 0; i--)
        if (strcmp(map->keys->data[i], key) == 0)
            return map->vals->data[i];
    return NULL;
}

int map_geti(Map *map, char *key, int _default) {
  for (int i = map->keys->len - 1; i >= 0; i--)
    if (!strcmp(map->keys->data[i], key))
      return (intptr_t)map->vals->data[i];
  return _default;
}

bool map_exist(Map *map, char *key) {
    for (int i = 0; i < map->keys->len; i++)
        if (!strcmp(map->keys->data[i], key))
            return true;
    return false;
}


Type *ptr_to(Type *base) {
    Type *ctype = calloc(1, sizeof(Type));
    ctype->ty = PTR;
    ctype->align = 8;
    ctype->size = 8;
    ctype->ptrto = base;
    return ctype;
}

Type *ary_of(Type *base, int len) {
    Type *cty = calloc(1, sizeof(Type));
    cty->ty = ARY;
    cty->align = base->align;
    cty->size = base->size * len;
    cty->aryof = base;
    cty->len = len;
    return cty;
}

Type *ctype_of_ary(Type *cty) {
    if (cty->aryof)
        return ctype_of_ary(cty->aryof);
    return cty;
}

int roundup(int num, int multiple) {
    // multiple should be 2^n
    assert( multiple && ( (multiple & (multiple-1)) == 0 ) );
    return (num + multiple - 1) & -multiple;
}

Node *addr_of(Node *base, Type *cty) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_ADDR;
    node->cty = ptr_to(cty);
    node->expr = base;
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