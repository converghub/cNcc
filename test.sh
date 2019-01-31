#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./ccc "$input" > tmp.s
  gcc -o tmp tmp.s tmp-test.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

cat <<EOF | gcc -xc -c -o tmp-test.o -
long plus(long x, long y) { return x + y; }
long *alloc1(long x, long y) {
  static long arr[2];
  arr[0] = x;
  arr[1] = y;
  return arr;
}
long *alloc2(long x, long y) {
  static long arr[2];
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

try 21 'int main() { 5+20-4;}'
try 41 "int main() {12 + 34 - 5;}"
try 47 "int main() {5+6*7;}"
try 15 "int main() {5*(9 - 6);}"
try 4 "int main() {(3+5)/2;}"

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
try 4 'int one(int x) { return x*2; } int main() { if (1) {return one(2);} return 3; }'
try 2 'int main() { if (1) return 2; else return 3; }'
try 3 'int main() { if (0) return 2; else {return 3;} }'

try 0 'int main() { return 1<0; }'
try 1 'int main() { return 0<1; }'
try 0 'int main() { return 0>1; }'
try 1 'int main() { return 1>0; }'
try 0 'int main() { return 0<0; }'
try 0 'int main() { return 0>0; }'

try 11 'int main(){int a; a=1; while (a<11) a=a+1; return a;}'
try 11 'int main(){int a; a=1; while (a<11) {a=a+2;a=a-1;} return a;}'

try 60 'int main() {int sum=0; for (int i=10; i<15; i=i+1) sum = sum + i; return sum;}'
try 60 'int main() {int sum=0; for (int i=10; i<15; i=i+1) {sum = sum + i; sum = sum + 0;} return sum;}'

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

# for now, "int" is treated as 8 bytes
try 8 'int main() { int x; return sizeof(x); }'
try 8 'int main() { int x; return sizeof(x); }'
try 8 'int main() { int *x; return sizeof x; }'
try 32 'int main() { int x[4]; return sizeof x; }'

try 5 'int main() { int x; int *p = &x; x = 5; return p[0];}'
try 3 'int main() { int ary[2]; ary[0]=1; ary[1]=2; return ary[0] + ary[1];}'