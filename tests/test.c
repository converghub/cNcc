extern int *stderr;

#define EXPECT(expected, expr)                                  \
  do {                                                          \
    int e1 = (expected);                                        \
    int e2 = (expr);                                            \
    if (e1 == e2) {                                             \
      fprintf(stderr, "%s => %d\n", #expr, e2);                 \
    } else {                                                    \
      fprintf(stderr, "%d: %s: %d expected, but got %d\n",      \
              __LINE__, #expr, e1, e2);                         \
      exit(1);                                                  \
    }                                                           \
  } while (0)


extern int global_arr[1];

int one() { return 1; }
int two() { return 2; }
int plus(int x, int y) { return x + y; }
int mul(int x, int y) { return x * y; }
int add(int a,int b,int c,int d,int e,int f) { return a+b+c+d+e+f; }
int f(int a) { return 2*a; }
int f2(int a, int b) { return 2*a+b; }

int main() {
    EXPECT(0, 0);
    EXPECT(21, 5+20-4);
    EXPECT(41, 12+34-5);
    EXPECT(47, 5+6*7);
    EXPECT(15, 5*(9 - 6));
    EXPECT(4, (3+5)/2);
    EXPECT(4, 19 % 5);
    EXPECT(0, 9 % 3);

    EXPECT(21, ({ 5+20-4; }));
    EXPECT(3, ({ int a=3; a; }));
    EXPECT(14, ({ int a=3;int b=5*6-8;a+b/2; }));
    EXPECT(0, 1==2);
    EXPECT(4, ({int a; int b; int c; a=b=c=1;a+b+c+1;}) );
    EXPECT(0, 1!=1);
    EXPECT(5, ({int a=(1==1)+(1!=1)*2+(0!=2)*4+(4!=4);a;}) );
    EXPECT(2, ({int a_2 = 1; a_2+1;}) );

    EXPECT(1, one());
    EXPECT(7, plus(2, 5));
    EXPECT(3, one() + two());
    EXPECT(6, mul(2, 3));
    EXPECT(21, add(1,2,3,4,5,6));

    EXPECT(2, ({ int a; if(1) a=2; else a=3; a;}) );
    EXPECT(3, ({ int a; if(0) a=2; else a=3; a;}) );
    EXPECT(3, ({ int a; if(0) a=2; else a=3; a;}) );
    EXPECT(3, ({ int a; if(0) a=2; else a=3; a;}) );
    EXPECT(3, ({ int a; if(0) a=2; else a=3; a;}) );
    EXPECT(4, ({ int a; if(1) a=mul(2,2); else a=3; a;}) );

    EXPECT(0, 1 < 0);
    EXPECT(1, 0 < 1);
    EXPECT(0, 0 > 1);
    EXPECT(1, 1 > 0);
    EXPECT(0, 0 < 0);
    EXPECT(0, 0 > 0);

    EXPECT(1, 4 <= 5);
    EXPECT(1, 5 <= 5);
    EXPECT(0, 6 <= 5);
    EXPECT(0, 4 >= 5);
    EXPECT(1, 5 >= 5);
    EXPECT(1, 6 >= 5);

    EXPECT(1, 0 || 1);
    EXPECT(1, 1 || 1);
    EXPECT(0, 0 && 0);
    EXPECT(0, 1 && 0);
    EXPECT(0, 0 && 1);
    EXPECT(1, 1 && 1);

    EXPECT(8, 1 << 3);
    EXPECT(4, 16 >> 2);
    EXPECT(11, 9 | 2);
    EXPECT(11, 9 | 3);
    EXPECT(0, !1);
    EXPECT(1, !0);
    EXPECT(5, 6 ^ 3);
    EXPECT(11, 100 ^ 111);

    EXPECT(3, 1 ? 3 : 5);
    EXPECT(5, 0 ? 3 : 5);

    EXPECT(0, ({ int a=1; a=a-2; ~a; }) );
    EXPECT(10, ({ int a=0; a=a-11; ~a; }) );

    EXPECT(11, ({ int a=1; while (a<11) a=a+1; a; }) );
    EXPECT(11, ({ int b=1; while (11+1>b+1) b=b+1; b; }) );
    EXPECT(45, ({ int x=0; int y=0; do { y=y+x; x=x+1; } while (x<10); y; }) );
    EXPECT(60, ({ int sum=0; for (int i=10; i<15; i=i+1) sum = sum + i; sum; }) );
    EXPECT(60, ({ int sum=0; for (int i=10; i<15; i=i+1) {sum = sum + i; sum = sum + 0;} sum; }) );

    EXPECT(4, ({ int b; int c = f(f(b=1)); c; }) );
    EXPECT(9, ({ int e; int f; int d = 1; int c = f2( f2(d, e=1), f2(1, f=1) ); c; }) );

    EXPECT(5, ({ int x; int *p = &x; x = 5; *p; }) );
    EXPECT(1, ({ int a[3]; *a = 1; *a; }) );    
    EXPECT(2, ({ int a[3]; *(1+a) = 2; *(a+1); }) );    
    EXPECT(6, ({ int ary[3]; *ary=1; *(ary+1)=2; *(ary+2) = 3; *ary + *(ary+1) + *(ary+2); }) );

    EXPECT(4, ({ int x; sizeof(x); }) );
    EXPECT(8, ({ int *x; sizeof(x); }) );
    EXPECT(16, ({ int x[4]; sizeof(x); }) );
    EXPECT(1, ({ char x; _Alignof(x); }) );
    EXPECT(4, ({ int x; _Alignof(x); }) );
    EXPECT(8, ({ int *x; _Alignof(x); }) );
    EXPECT(4, ({ int x[4]; _Alignof(x); }) );
    EXPECT(8, ({ int *x[4]; _Alignof(x); }) );

    EXPECT(5, ({ int x; int *p = &x; x = 5; p[0]; }) );
    EXPECT(3, ({ int ary[2]; ary[0]=1; ary[1]=2; ary[0] + ary[1]; }) );
    EXPECT(3, ({ int ary[2]; ary[0]=1; ary[1]=2; ary[0] + ary[1*1*1+1*1+0-1]; }) );

    EXPECT(97, ({ char *p = "abc"; p[0]; }));
    EXPECT(98, ({ char *p = "abc"; p[1]; }));
    EXPECT(99, ({ char *p = "abc"; p[2]; }));
    EXPECT('a', ({ char *p = "abc"; p[0]; }));
    EXPECT('b', ({ char *p = "abc"; p[1]; }));
    EXPECT('c', ({ char *p = "abc"; p[2]; }));
    EXPECT(0, ({ char *p = "abc"; p[3]; }) );

    EXPECT(5, ({ global_arr[0]; }) );

    EXPECT(1, ({ ; 1; }) );

    EXPECT(3, ({ int i=3; i++; }) );
    EXPECT(3, ({ int i=3; i--; }) );
    EXPECT(4, ({ int i=3; ++i; }) );   
    EXPECT(2, ({ int i=3; --i; }) );
    EXPECT(1, ({ int ary[2]; ary[0]=1; ary[1]=2; int *p=ary; *p++; }) );
    EXPECT(2, ({ int ary[2]; ary[0]=1; ary[1]=2; int *p=ary; *++p; }) );

    EXPECT(1, ({ int x=1; {int x=2;} x; }) );
    EXPECT(3, ({ int x=1; int y=2; {int x=2; int y=3;} x+y; }) );
    EXPECT(5, ({ int a=1; 3 + ({ int a=2; int b; int c; int d; a; }); }) );

    EXPECT(8, ({ 1; 1+1; 1+2+5; }) );
    EXPECT(8, ({ 1; {1+1;} 1+2+5; }) );
    EXPECT(8, ({ 1+2; { ({1;}); } {({1;});} 1+2+5; }) );

    EXPECT(3, (1, 2, 3) );
    EXPECT(3, ((1), 2, 3) );
    EXPECT(3, (1, (2), 3) );
    EXPECT(3, (1, (2), (3)) );
    EXPECT(3, ( ({int a=1;a;}), 2, 3) );
    EXPECT(4, ( ({int a=1;a;}), ({int a=1; {{ {1;} }}  a;}), 4 ) );
    EXPECT(3, ({ int a; a = (1,2,3); a; })  );
    EXPECT(4, ( ({ int a; a = (1,2,3); a; }), ({ int a; a = (1,2,3); a; }), ({ int a; a = (1,2,4); a; }) )  );

    printf("OK.\n");
    return 0;
}