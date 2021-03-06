#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./cncc "$input" > tmp-legacy.s
  gcc -g -o tmp-legacy-test tmp-legacy.s tmp-test.o
  ./tmp-legacy-test
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

cat <<EOF | gcc -xc -c -o tmp-test.o -
int global_arr[1] = {5};
int plus(int x, int y) { return x + y; }
int *alloc1(int x, int y) {
  static int arr[2];
  arr[0] = x;
  arr[1] = y;
  return arr;
}
int *alloc2(int x, int y) {
  static int arr[2];
  arr[0] = x;
  arr[1] = y;
  return arr + 1;
}
long **alloc_ptr_ptr(long x) {
  static long **p;
  static long *q;
  static long r;
  r = x;
  q = &r;
  p = &q;
  return p;
}
EOF

try 3 "int main() { 3; }"
try 3 "int main() { 0-(-3); }"
try 21 'int main() { 5+20-4; }'
try 41 'int main() { 12 + 34 - 5; }'
try 47 'int main() { 5+6*7; }'
try 15 'int main() { 5*(9 - 6); }'
try 4 'int main() { (3+5)/2; }'
try 4 'int main() { 19 % 5; }'
try 0 'int main() { 9 % 3; }'

try 21 "int main() {1+2; 5+20-4;}"
try 3 "int main() {int a=3;a;}"
try 14 "int main() {int a=3;int b=5*6-8;a+b/2;}"
try 4 "int main() {int a; int b;a=b=2;a+b;}"
try 0 "int main() {1==2;}"
try 4 "int main() {int a; int b; int c;a=b=c=1==1;a+b+c+1;}"
try 0 "int main() {1!=1;}"
try 5 "int main() {int a=(1==1)+(1!=1)*2+(0!=2)*4+(4!=4);a;}"
try 2 "int main() {int a_2 = 1; a_2+1;}"

try 2 "int main() {int a_2=1; return a_2+1;}"
try 7 "int main() {return plus(2, 5);}"

try 2 "int one() { return 1+(2==2); } int main() { return one(); }"
try 0 "int one() { return 1+2==2; } int main() { return one(); }"

try 3 "int one(int x) { return x+2; } int main() {return one(1);}"
try 6 'int one(int x) { return x*2; } int two(int y) { return 2+y; } int main() { return one(1)+two(2); }'
try 6 'int mul(int a, int b) { return a * b; } int main() { return mul(2, 3); }'
try 21 'int add(int a,int b,int c,int d,int e,int f) { return a+b+c+d+e+f; } int main() { return add(1,2,3,4,5,6); }'

try 2 'int main() { if (1) return 2; return 3; }'
try 3 'int main() { if (0) return 2; return 3; }'
try 2 'int main() { int a; if (1) a=2; else a=3; return a; }'
try 4 'int one(int x) { return x*2; } int main() { if (1) {return one(2);} return 3; }'
try 2 'int main() { if (1) return 2; else return 3; }'
try 3 'int main() { if (0) return 2; else {return 3;} }'

try 0 'int main() { return 1<0; }'
try 1 'int main() { return 0<1; }'
try 0 'int main() { return 0>1; }'
try 1 'int main() { return 1>0; }'
try 0 'int main() { return 0<0; }'
try 0 'int main() { return 0>0; }'

try 1 'int main() { return 4<=5; }'
try 1 'int main() { return 5<=5; }'
try 0 'int main() { return 6<=5; }'
try 0 'int main() { return 4>=5; }'
try 1 'int main() { return 5>=5; }'
try 1 'int main() { return 6>=5; }'

try 8 'int main() { return 1 << 3; }'
try 4 'int main() { return 16 >> 2; }'
try 11 'int main() { return 9 | 2; }'
try 11 'int main() { return 9 | 3; }'
try 0 'int main() { return !1; }'
try 1 'int main() { return !0; }'
try 5 'int main() { return 6 ^ 3; }'
try 11 'int main() { return 100 ^ 111; }'
try 3 'int main() { return 1 ? 3 : 5; }'
try 5 'int main() { return 0 ? 3 : 5; }'

try 0 'int main() { int a=1; a = a-2; return ~a; }'
try 10 'int main() { int a=0; a = a-11; return ~a; }'

try 11 'int main() {int a; a=1; while (a<11) a=a+1; return a;}'
try 11 'int main() {int b; b=1; while (11+1>b+1) b=b+1; return b;}'
try 11 'int main() {int a; a=1; while (a+1<11+1) a=a+1; return a;}'
try 11 'int main() {int a; a=1; while (a<11) {a=a+2;a=a-1;} return a;}'
try 45 'int main() { int x=0; int y=0; do { y=y+x; x=x+1; } while (x < 10); return y; }'

try 60 'int main() {int sum=0; for (int i=10; i<15; i=i+1) sum = sum + i; return sum;}'
try 60 'int main() {int sum=0; for (int i=10; i<15; i=i+1) {sum = sum + i; sum = sum + 0;} return sum;}'
try 1 'int main() {int sum=0; int i = 1; for (int i=10; i<15; i=i+1) sum = sum + i; return i;}'
try 10 'int main() { int i=0; for (; i < 10 ;) { i++; } return i;  }'
try 5 'int main() { int i=0; for (0; i < 10; i++) if (i==5) break; i; }'

try 1 "int main(){(1 == 1) && (2 == 2);}"
try 0 "int main(){(1 == 1) && (2 == 0);}"
try 0 "int main(){(1 == 0) && (2 == 2);}"
try 0 "int main(){(1 == 0) && (2 == 4);}"
try 1 "int main(){(1 == 1) || (2 == 2);}"
try 1 "int main(){(1 == 1) || (2 == 0);}"
try 1 "int main(){(1 == 3) || (2 == 2);}"
try 0 "int main(){(1 == 3) || (2 == 0);}"

try 4 'int f(int a){2 * a;} int main(){int b; int c=f(f(b=1));c;}'
try 9 'int f(int a, int b){return 2 * a + b;} int main(){int e; int f; int d=1; int c=f(f(d,e=1), f(1,f=1)); return c;}'

try 5 'int main() { int x; int *p = &x; x = 5; return *p;}'
try 2 'int main() { int **p = alloc_ptr_ptr(2); return **p; }'

try 3 'int main() { int *p = alloc1(3, 5); return *(p); }'
try 5 'int main() { int *p = alloc1(3, 5); return *(1 + p); }'
try 9 'int main() { int *p = alloc1(3, 5); return *p + *p + *(p); }'
try 8 'int main() { int *p = alloc1(3, 5); return *p + *(1 + p); }'
try 8 'int main() { int *p = alloc1(3, 5); return *p + *(p + 1); }'
try 9 'int main() { int *p = alloc2(2, 7); return *p + *(p - 1); }'

try 1 'int main() { int a[3]; *a=1;  return *a;}'
try 2 'int main() { int a[3]; *a=1; *(1+a)=2; return *(a+1);}'
try 5 'int main() { int x; int *p = &x; x = 5; return *p;}'
try 2 'int main() { int **p = alloc_ptr_ptr(2); return **p; }'
try 3 'int main() { int a[2]; *a=1; *(1+a)=2; return *a+*(a+1);}'
try 3 'int main() { int a[2]; *a=1; *(a+1)=2; int *p; p=a; return *p+*(p+1);}'
try 6 'int main() { int ary[3]; *ary=1; *(ary+1)=2; *(ary+2) = 3; return *ary + *(ary+1) + *(ary+2);}'

try 4 'int main() { int x; return sizeof(x); }'
try 4 'int main() { int x; return sizeof(x); }'
try 8 'int main() { int *x; return sizeof x; }'
try 16 'int main() { int x[4]; return sizeof x; }'
try 1 'int main() { char x; return _Alignof(x); }'
try 4 'int main() { int x; return _Alignof(x); }'
try 8 'int main() { int *x; return _Alignof x; }'
try 4 'int main() { int x[4]; return _Alignof x; }'
try 8 'int main() { int *x[4]; return _Alignof x; }'

try 5 'int main() { int x; int *p = &x; x = 5; return p[0];}'
try 3 'int main() { int ary[2]; ary[0]=1; ary[1]=2; return ary[0] + ary[1];}'
try 3 'int main() { int ary[2]; ary[0]=1; ary[1]=2; return ary[0] + ary[1*1*1+1*1+0-1];}'

try 0 'int x; int main() { return x; }'
try 5 'int x; int main() { x = 5; return x; }'
try 20 'int x[5]; int main() { return sizeof(x); }'
try 15 'int x[5]; int main() { x[0] = 5; x[4] = 10; return x[0] + x[4]; }'

try 2 "int main() {char a=4; return a/2;}"
try 14 "int main() {char a=3;char b=5*6-8; return a+b/2;}"
try 42 'int main() { int x = 0; char *p = &x; p[1] = 0;  p[0] = 42; return x; }'
try 1 'int main() { char x; return sizeof x; }'
try 1 'int main(char x) { return sizeof(x); }'
try 2 'int main() { char x = 1; return x+1; }'
try 4 "int main() {char a; char b;a=b=2;a+b;}"
try 10 "int main() {char a[4]; for(int i=0; i < sizeof(a); i=i+1){ a[i]=i+1; } return a[0]+a[1]+a[2]+a[3];}"

try 97 'int main() { char *p = "abc";  return p[0]; }'
try 98 'int main() { char *p = "abc"; return p[1]; }'
try 99 'int main() { char *p = "abc"; return p[2]; }'
try 0 'int main() { char *p = "abc"; return p[3]; }'
try 97 'int main() { char *p = "abc"; char *q = "def"; return p[0]; }'
try 98 'int main() { char *p = "abc"; char *q = "def"; return p[1]; }'
try 99 'int main() { char *p = "abc"; char *q = "def"; return p[2]; }'
try 97 "char main() { 'a'; }"
try 98 "char main() { 'b'; }"
try 99 "char main() { 'c'; }"

try 5 'extern int global_arr[1]; int main() { return global_arr[0]; }'
try 1 'int main() {; return 1;}'

# https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
try 8 'int main() { return 3 + ({ 5; }); }'
try 28 'int main() { return 3 + ({ 5*5; }); }'
try 11 'int main() { return 3 + ({ 1+2+5; }); }'
try 11 'int main() { return 3 + ({ 1; 1; 1+2+5; }); }'

try 3 'int main() { int i=3; i++;}'
try 5 'int main() { int i=4; i++; i;}'
try 8 'int main() { int i=4; i++; 3+i++;}'
try 6 'int main() { int i=4; i++; 3+i++; i;}'
try 3 'int main() { int i=3; i--;}'
try 3 'int main() { int i=4; i--; i;}'
try 6 'int main() { int i=4; i--; 3+i--;}'
try 2 'int main() { int i=4; i--; 3+i--; i;}'
try 1 'int main() { int i=0; i++ + i++;}'
try 2 'int main() { int i=0; i++ + i++; i;}'
try 4 'int main() { int i=3; ++i;}'
try 2 'int main() { int i=3; --i;}'
try 1 'int main() { int ary[2]; ary[0]=1; ary[1]=2; int *p=ary; return *p++; }'
try 2 'int main() { int ary[2]; ary[0]=1; ary[1]=2; int *p=ary; *p++; return *p; }'
try 2 'int main() { int ary[2]; ary[0]=1; ary[1]=2; int *p=ary; return *++p; }'

try 1 'int main() { int x = 1; {int x = 2;} return x;}'
try 3 'int main() { int x = 1; int y = 2; {int x = 2; int y = 3;} return x+y;}'
try 5 'int main() { int a=1; return 3 + ({ int a = 2; int b; int c; int d; a; }); }'

try 1 'int main() { do { int e1 = 11;  int e2 = ({ 100 ^ 111; }); if (e1==e2) return 1; else return 0; }  while (0); }'
try 5 'extern int global_arr[1]; int main() { return ({ global_arr[0]; }); }'
try 1 'int main() { do { int e1 = (5); int e2 = (({ int x; int *p = &x; x = 5; *p; })); if (e1 == e2) { return 1; } else { return 0; } } while (0); }'
try 60 'int main(){ do { int e2 = (({ int sum=0; for (int i=10; i<15; i=i+1) {sum = sum + i;} sum; })); return e2; } while (0); }'
try 60 'int main(){ do { int e2 = (({ int sum=0; for (int i=10; i<15; i=i+1) {sum = sum + i; sum = sum + 0;} sum; })); return e2; } while (0); }'
try 1 'int main(){ do { int e1 = (60); int e2 = (({ int sum=0; for (int i=10; i<15; i=i+1) {sum = sum + i; sum = sum + 0;} sum; })); if (e1 == e2) { return 1; } else { return 0; } } while (0); }'
try 45 'int main() { do {int e2 = (({ int x=0; int y=0; do { y=y+x; x=x+1; } while (x<10); y; })); return e2; } while (0); }'
try 45 'int main() { do { int e1 = (45); int e2 = (({ int x=0; int y=0; do { y=y+x; x=x+1; } while (x<10); y; })); return e2; } while (0); }'
try 1 'int main() { do { int e1 = (45); int e2 = (({ int x=0; int y=0; do { y=y+x; x=x+1; } while (x<10); y; })); if (e1 == e2) { return 1; } else { return 0; } } while (0); }'
try 1 'int main() { int x = 1; {int x = 2;} return x;}'
try 11 'int main() { return 3 + ({ 100; 200; 1+2+5; }); }'
try 11 'int main() { return 3 + ({ 100; {200;} 1+2+5; }); }'
try 11 'int main() { return 3 + ({ 1; { ({1;}); } 1+2+5; }); }'
try 8 'int main() { ({ 1+2; { ({1;}); } {({1;});} 1+2+5; }); }'
try 8 'int main() { return ({ 1+2; { ({1;}); } {({1;});} 1+2+5; }); }'

try 11 'int main() { return 3 + ({ 1; 1+1; 1+2+5; }); }'
try 11 'int main() { return 3 + ({ 1; {1+1;} 1+2+5; }); }'

try 11 'int main() { return 3 + ({ 1; 1*1; 1+2+5; }); }'
try 11 'int main() { return 3 + ({ 1; {1*1;} 1+2+5; }); }'

try 11 'int main() { return 3 + ({ 1 ? 100 : 200; 1+2+5; }); }'
try 11 'int main() { return 3 + ({ {1 ? 100 : 200;} 1+2+5; }); }'

try 11 'int main() { return 3 + ({ ~100; 1+2+5; }); }'
try 11 'int main() { return 3 + ({ {~100;} 1+2+5; }); }'

try 11 'int main() { return 3 + ({ int i=1; i++; 1+2+5; }); }'
try 11 'int main() { return 3 + ({ {int i=1; i++;} 1+2+5; }); }'
try 5 'int main() { return 3 + ({ int i=1; i++; i; }); }'

try 11 'int main() { return 3 + ({ int test; test; 1+2+5; }); }'
try 11 'int main() { return 3 + ({ {int test; test;} 1+2+5; }); }'

try 11 'int main() { return 3 + ({ if (1) 200; 300; 1+2+5; }); }'
try 11 'int main() { return 3 + ({ {if (1) 200; 300;} 1+2+5; }); }'
try 11 'int main() { return 3 + ({ {if (1) {200;} 300;} 1+2+5; }); }'


try 11 'int main() { return 3 + ({ int a; if (1) a=200; a=300; 1+2+5; }); }'
try 11 'int main() { return 3 + ({ int a; if (1) {a=200;} a=300; 1+2+5; }); }'

# ref: https://en.wikipedia.org/wiki/Comma_operator
try 3 'int main() { return 1, 2, 3; }'
try 3 'int main() { return (1), 2, 3; }'
try 3 'int main() { return 1, (2), 3; }'
try 3 'int main() { return 1, 2, (3); }'
try 3 'int main() { return (1, 2, 3); }'
try 3 'int main() { return ( ({int a=1;a;}), 2, 3); }'
try 4 'int main() { return ( ({int a=1;a;}), ({int a=1; {{ {1;} }}  a;}), 4 ); }'
try 3 'int main() { return ({ int a; a = (1,2,3); a; }); }'
try 4 'int main() { return ({ int a; a = (1,2,3); a; }), ({ int a; a = (1,2,3); a; }), ({ int a; a = (1,2,4); a; }); }'

try 4 "int main() { struct {int a;} x; return sizeof(x); }"
try 8 "int main() { struct {int a; int b;} x; return sizeof(x); }"
try 12 "int main() { struct {char a; char b; int c; char d;} x; return sizeof(x); }"
try 24 "int main() { struct {char a; char b; int c; char d; char *e;} x; return sizeof(x); }"
try 24 "int main() { struct {char *e; char a; char b; int c; char d;} x; return sizeof(x); }"
try 32 "int main() { struct {char a; char *e; char b; int c; char d;} x; return sizeof(x); }"
try 32 "int main() { struct {char a; char *e; char b; int c; char d; int f;} x; return sizeof(x); }"

try 3 "int main() { struct { int a; } x; x.a = 3; return x.a; }"
try 8 "int main() { struct { char a; int b; } x; x.a = 3; x.b = 5; return x.a+x.b; }"
try 8 "int main() { struct { int a; int b; } x; x.a = 3; x.b = 5; return x.a+x.b; }"
try 10 "int main() { struct { int a; int b; char c; char d; } x; x.a = 3; x.b = 5; x.c=x.d=1; return x.a+x.b+x.c+x.d; }"
try 5 "int main() { struct { char a; int b; } x; x.a = 3; x.b = 5; }"
try 8 "int main() { struct { char a; int b; } x; struct { char a; int b;} *p = &x; x.a = 3; x.b = 5; return p->a + p->b; }"
try 8 "int main() { struct tag_test { char a; int b; } x; struct tag_test *p = &x; x.a=3; x.b=5; return p->a+p->b; }"
try 8 "int main() {
          struct {
            int hoge[2];
            struct {
              int b;
              int c[5];
            } a[2];
          } x;
          x.hoge[0] = 3;
          x.hoge[1] = 5;
          x.hoge[0] + x.hoge[1];
          }"
try 3 "int main() {
          struct {
            int hoge[2];
            struct {
              int b;
              int c[5];
            } a[2];
          } x;
          x.a[0].b = 3;
          x.a[0].b;
          }"
try 8 "int main() {
          struct {
            struct {
              int b;
              int c[5];
            } a[2];
          } x;
          x.a[0].b = 3;
          x.a[0].c[1] = 5;
          x.a[0].b + x.a[0].c[1];
          }"

try 3 "int main() { typedef int foo; foo x = 3; return x; }"
try 5 "int main() { typedef struct foo { int a; int b; } foo; foo x; x.a=3; x.b=2; x.a+x.b; }"
try 10 "int main() { typedef struct foo { int a; int *b; } foo; foo x; x.a=5; x.b = &x.a; x.a + *(x.b); }"

echo 'OK!'