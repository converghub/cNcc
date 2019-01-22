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

try 0 0
try 42 42
try 21 '5+20-4'
try 41 " 12 + 34 - 5 "
try 47 "5+6*7"
try 15 "5*(9 - 6)"
try 4 "(3+5)/2"

try 21 "1+2; 5+20-4;"
try 3 "a=3;a;"
try 14 "a=3;b=5*6-8;a+b/2;"
try 4 "a=b=2;a+b;"
try 0 "1==2;"
try 4 "a=b=c=1==1;a+b+c+1;"
try 0 "1!=1;"
try 5 "a=(1==1)+(1!=1)*2+(0!=2)*4+(4!=4);a;"
try 2 "a_2 = 1; a_2+1;"

try 2 "a_2=1; return a_2+1;"

try 7 "return plus(2, 5);"
