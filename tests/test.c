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
int mul(int x, int y) {return x * y;}
int add(int a,int b,int c,int d,int e,int f) { return a+b+c+d+e+f; }


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


    /* FIX THIS: Segmentation fault occurs if the followings are uncommented.

    /* 
    EXPECT(11, ({ int a=1; while (a<11) a=a+1; a; }) );
    EXPECT(11, ({ int b=1; while (11+1>b+1) b=b+1; b; }) );
    EXPECT(45, ({ int x=0; int y=0; do { y=y+x; x=x+1; } while (x<10); y; }) );
    EXPECT(60, ({ int sum=0; for (int i=10; i<15; i=i+1) sum = sum + i; sum; }) );
    EXPECT(60, ({ int sum=0; for (int i=10; i<15; i=i+1) {sum = sum + i; sum = sum + 0;} sum; }) );
    */
    
    // TODO: The followings should be compiled correctly
    /*
    EXPECT(3, (1, 2, 3));
    */


    printf("OK.\n");
    return 0;
}
