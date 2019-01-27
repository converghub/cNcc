#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./ccc "$input" > tmp.s
  gcc -o tmp tmp.s tmp-plus.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

echo 'int plus(int x, int y){ return x + y; }' | gcc -S -xc -c -o tmp-plus.s -

try 3 "int one(int x) { return x+2; } int main() {return one(1);}"
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