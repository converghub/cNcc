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

try 21 'main() { 5+20-4;}'
try 41 "main() {12 + 34 - 5;}"
try 47 "main() {5+6*7;}"
try 15 "main() {5*(9 - 6);}"
try 4 "main() {(3+5)/2;}"

try 21 "main() {1+2; 5+20-4;}"
try 3 "main() {a=3;a;}"
try 14 "main() {a=3;b=5*6-8;a+b/2;}"
try 4 "main() {a=b=2;a+b;}"
try 0 "main() {1==2;}"
try 4 "main() {a=b=c=1==1;a+b+c+1;}"
try 0 "main() {1!=1;}"
try 5 "main() {a=(1==1)+(1!=1)*2+(0!=2)*4+(4!=4);a;}"
try 2 "main() {a_2 = 1; a_2+1;}"

try 2 "main() {a_2=1; return a_2+1;}"
try 7 "main() {return plus(2, 5);}"

try 2 "one() { return 1+(2==2); } main() { return one(); }"
try 0 "one() { return 1+2==2; } main() { return one(); }"

try 3 "one(x) { return x+2; } main() {return one(1);}"
try 6 'one(x) { return x*2; } two(y) { return 2+y; } main() { return one(1)+two(2); }'
try 6 'mul(a, b) { return a * b; } main() { return mul(2, 3); }'
try 21 'add(a,b,c,d,e,f) { return a+b+c+d+e+f; } main() { return add(1,2,3,4,5,6); }'

try 2 'main() { if (1) return 2; return 3; }'
try 3 'main() { if (0) return 2; return 3; }'
try 4 'one(x) { return x*2; } main() { if (1) {return one(2);} return 3; }'
try 2 'main() { if (1) return 2; else return 3; }'
try 3 'main() { if (0) return 2; else {return 3;} }'

try 0 'main() { return 1<0; }'
try 1 'main() { return 0<1; }'
try 0 'main() { return 0>1; }'
try 1 'main() { return 1>0; }'
try 0 'main() { return 0<0; }'
try 0 'main() { return 0>0; }'

try 11 'main(){a=1; while (a<11) a=a+1; return a;}'
try 11 'main(){a=1; while (a<11) {a=a+2;a=a-1;} return a;}'

try 60 'main() { sum=0; for (i=10; i<15; i=i+1) sum = sum + i; return sum;}'
try 60 'main() { sum=0; for (i=10; i<15; i=i+1) {sum = sum + i; sum = sum + 0;} return sum;}'

try 1 "main(){(1 == 1) && (2 == 2);}"
try 0 "main(){(1 == 1) && (2 == 0);}"
try 0 "main(){(1 == 0) && (2 == 2);}"
try 0 "main(){(1 == 0) && (2 == 4);}"
try 1 "main(){(1 == 1) || (2 == 2);}"
try 1 "main(){(1 == 1) || (2 == 0);}"
try 1 "main(){(1 == 3) || (2 == 2);}"
try 0 "main(){(1 == 3) || (2 == 0);}"

try 4 'f(a){2 * a;} main(){c=f(f(b=1));c;}'
try 9 'f(a, b){return 2 * a + b;} main(){d=1; c=f(f(d,e=1), f(1,f=1)); return c;}'