/*
 * This is a C compiler tests.
 */

#include "include1.h"

int printf();
int exit();
int strcmp(char *p, char *q);
int memcmp(char *, char *);
double add_double(double x, double y);
float add_float(float x, float y);
float ret_float(float x);

int g1, g2[4];

typedef int MyInt, MyInt2[4];

char g3 = 3;
short g4 = 4;
int g5 = 5;
long g6 = 6;
int g9[3] = {0, 1, 2};
struct {char a; int b;} g11[2] = {{1, 2}, {3, 4}};
struct {int a[2];} g12[2] = {{{1, 2}}};
char g17[] = "foobar";
char g18[10] = "foobar";
char g19[3] = "foobar";
char *g20 = g17+0;
char *g21 = g17+3;
char *g22 = &g17-3;
char *g23[] = {g17+0, g17+3, g17-3};
int g24=3;
int *g25=&g24;
int g26[3] = {1, 2, 3};
int *g27 = g26 + 1;
int *g28 = &g11[1].a;

struct {int a[2];} g30[2] = {{1, 2}, 3, 4};
struct {int a[2];} g31[2] = {1, 2, 3, 4};
char g33[][4] = {'f', 'o', 'o', 0, 'b', 'a', 'r', 0};
char *g34 = {"foo"};

float g40 = 1.5;
double g41 = 0.0 ? 55 : (0, 1 + 1 * 5.0 / 2 * (double)2 * (int)2.0);

typedef struct Tree {
    int val;
    struct Tree *lhs;
    struct Tree *rhs;
} Tree;

Tree *tree = &(Tree) {
    1,
    &(Tree) {
        2,
        &(Tree){ 3, 0, 0 },
        &(Tree){ 4, 0, 0 },
    },
    0
};

int assert(int expected, int actual, char *code) {
    if (expected == actual) {
        printf("%s => %ld\n", code, actual);
    } else {
        printf("%s => %ld expected but got %ld\n", code, expected, actual);
        exit(1);
    }
}

int ret3(void) {
    return 3;
    return 5;
}

int add2(int x, int y) {
    return x + y;
}

int sub2(int x, int y) {
    return x - y;
}

int add5(int a, int b, int c, int d, int e) {
    return a + b + c + d + e;
}

int add6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
}

int addx(int *x, int y) {
    return *x + y;
}

int sub_char(char a, char b, char c) {
    return a - b - c;
}

int sub_short(short a, short b, short c) {
    return a - b - c;
}

int sub_long(long a, long b, long c) {
    return a - b - c;
}

int fib(int x) {
    if (x<=1)
        return 1;
    return fib(x-1) + fib(x-2);
}

int *g1_ptr() { return &g1; }
char int_to_char(int x) { return x; }

int div_long(long a, long b) {
    return a / b;
}

_Bool bool_fn_add(_Bool x) { return x + 1; }
_Bool bool_fn_sub(_Bool x) { return x - 1; }

void ret_none() {
    return;
}

static int static_fn() { return 3; }

int param_decay(int x[]) { return x[0]; }

extern int ext1;
extern int *ext2;
static int ext3 = 3;

int;
struct {char a; int b;};
typedef struct {char a; int b;} Ty1;

int _Alignas(512) g_aligned1;
int _Alignas(512) g_aligned2;

int counter() {
    static int i;
    static int j = 1+1;
    return i++ + j++;
}

_Bool true_fn();
_Bool false_fn();

typedef struct {
    int gp_offset;
    int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} va_list[1];

int add_all1(int x, ...);
int add_all3(int z, int b, int c, ...);
int sprintf(char *buf, char *fmt, ...);
int vsprintf(char *buf, char *fmt, va_list ap);

char *fmt(char *buf, char *fmt, ...) {
    va_list ap;
    __builtin_va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
}

_Noreturn noreturn_fn(void) {
    exit(0);
}

double add_double3(double x, double y, double z) {
    return x + y + z;
}

float add_float3(float x, float y, float z) {
    return x + y + z;
}

int (*fnptr(void))(int) {
    return ret3;
}

int main() {
    assert(0, 0, "0");
    assert(42, 42, "42");
    assert(21, 5+20-4, "5+20-4");
    assert(41,  12 + 34 - 5 , " 12 + 34 - 5 ");
    assert(47, 5+6*7, "5+6*7");
    assert(15, 5*(9-6), "5*(9-6)");
    assert(4, (3+5)/2, "(3+5)/2");
    assert(10, -10+20, "-10+20");
    assert(10, - -10, "- -10");
    assert(10, - - +10, "- - +10");

    assert(0, 0==1, "0==1");
    assert(1, 42==42, "42==42");
    assert(1, 0!=1, "0!=1");
    assert(0, 42!=42, "42!=42");

    assert(1, 0<1, "0<1");
    assert(0, 1<1, "1<1");
    assert(0, 2<1, "2<1");
    assert(1, 0<=1, "0<=1");
    assert(1, 1<=1, "1<=1");
    assert(0, 2<=1, "2<=1");

    assert(1, 1>0, "1>0");
    assert(0, 1>1, "1>1");
    assert(0, 1>2, "1>2");
    assert(1, 1>=0, "1>=0");
    assert(1, 1>=1, "1>=1");
    assert(0, 1>=2, "1>=2");

    assert(3, ({ int a; a=3; a; }), "({ int a; a=3; a; })");
    assert(3, ({ int a=3; a; }), "({ int a=3; a; })");
    assert(8, ({ int a=3; int z=5; a+z; }), "({ int a=3; int z=5; a+z; })");

    assert(3, ({ int a=3; a; }), "({ int a=3; a; })");
    assert(8, ({ int a=3; int z=5; a+z; }), "({ int a=3; int z=5; a+z; })");
    assert(6, ({ int a; int b; a=b=3; a+b; }), "({ int a; int b; a=b=3; a+b; })");
    assert(3, ({ int foo=3; foo; }), "({ int foo=3; foo; })");
    assert(8, ({ int foo123=3; int bar=5; foo123+bar; }), "({ int foo123=3; int bar=5; foo123+bar; })");

    assert(3, ({ int x; if (0) x=2; else x=3; x; }), "({ int x; if (0) x=2; else x=3; x; })");
    assert(3, ({ int x; if (1-1) x=2; else x=3; x; }), "({ int x; if (1-1) x=2; else x=3; x; })");
    assert(2, ({ int x; if (1) x=2; else x=3; x; }), "({ int x; if (1) x=2; else x=3; x; })");
    assert(2, ({ int x; if (2-1) x=2; else x=3; x; }), "({ int x; if (2-1) x=2; else x=3; x; })");

    assert(55, ({ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; j; }), "({ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; j; })");

    assert(10, ({ int i=0; while(i<10) i=i+1; i; }), "({ int i=0; while(i<10) i=i+1; i; })");

    assert(3, ({ 1; {2;} 3; }), "({ 1; {2;} 3; })");
    assert(5, ({ ;;; 5; }), "({ ;;; 5; })");

    assert(10, ({ int i=0; while(i<10) i=i+1; i; }), "({ int i=0; while(i<10) i=i+1; i; })");
    assert(55, ({ int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} j; }), "({ int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} j; })");

    assert(3, ({ int x=3; *&x; }), "({ int x=3; *&x; })");
    assert(3, ({ int x=3; int *y=&x; int **z=&y; **z; }), "({ int x=3; int *y=&x; int **z=&y; **z; })");
    assert(5, ({ int x=3; int y=5; *(&x+1); }), "({ int x=3; int y=5; *(&x+1); })");
    assert(3, ({ int x=3; int y=5; *(&y-1); }), "({ int x=3; int y=5; *(&y-1); })");
    assert(5, ({ int x=3; int *y=&x; *y=5; x; }), "({ int x=3; int *y=&x; *y=5; x; })");
    assert(7, ({ int x=3; int y=5; *(&x+1)=7; y; }), "({ int x=3; int y=5; *(&x+1)=7; y; })");
    assert(7, ({ int x=3; int y=5; *(&y-1)=7; x; }), "({ int x=3; int y=5; *(&y-1)=7; x; })");
    assert(2, ({ int x=3; (&x+2)-&x; }), "({ int x=3; (&x+2)-&x; })");
    assert(8, ({ int x, y; x=3; y=5; x+y; }), "({ int x, y; x=3; y=5; x+y; })");
    assert(8, ({ int x=3, y=5; x+y; }), "({ int x=3, y=5; x+y; })");

    assert(3, ret3(), "ret3()");
    assert(8, add2(3, 5), "add2(3, 5)");
    assert(2, sub2(5, 3), "sub2(5, 3)");
    assert(15, add5(1,2,3,4,5), "add5(1,2,3,4,5)");

    assert(7, add2(3,4), "add2(3,4)");
    assert(1, sub2(4,3), "sub2(4,3)");
    assert(55, fib(9), "fib(9)");

    assert(3, ({ int x[2]; int *y=&x; *y=3; *x; }), "({ int x[2]; int *y=&x; *y=3; *x; })");

    assert(3, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *x; }), "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *x; })");
    assert(4, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+1); }), "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+1); })");
    assert(5, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+2); }), "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+2); })");

    assert(0, ({ int x[2][3]; int *y=x; *y=0; **x; }), "({ int x[2][3]; int *y=x; *y=0; **x; })");
    assert(1, ({ int x[2][3]; int *y=x; *(y+1)=1; *(*x+1); }), "({ int x[2][3]; int *y=x; *(y+1)=1; *(*x+1); })");
    assert(2, ({ int x[2][3]; int *y=x; *(y+2)=2; *(*x+2); }), "({ int x[2][3]; int *y=x; *(y+2)=2; *(*x+2); })");
    assert(3, ({ int x[2][3]; int *y=x; *(y+3)=3; **(x+1); }), "({ int x[2][3]; int *y=x; *(y+3)=3; **(x+1); })");
    assert(4, ({ int x[2][3]; int *y=x; *(y+4)=4; *(*(x+1)+1); }), "({ int x[2][3]; int *y=x; *(y+4)=4; *(*(x+1)+1); })");
    assert(5, ({ int x[2][3]; int *y=x; *(y+5)=5; *(*(x+1)+2); }), "({ int x[2][3]; int *y=x; *(y+5)=5; *(*(x+1)+2); })");

    assert(3, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *x; }), "({ int x[3]; *x=3; x[1]=4; x[2]=5; *x; })");
    assert(4, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+1); }), "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+1); })");
    assert(5, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); }), "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })");
    assert(5, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); }), "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })");
    assert(5, ({ int x[3]; *x=3; x[1]=4; 2[x]=5; *(x+2); }), "({ int x[3]; *x=3; x[1]=4; 2[x]=5; *(x+2); })");

    assert(0, ({ int x[2][3]; int *y=x; y[0]=0; x[0][0]; }), "({ int x[2][3]; int *y=x; y[0]=0; x[0][0]; })");
    assert(1, ({ int x[2][3]; int *y=x; y[1]=1; x[0][1]; }), "({ int x[2][3]; int *y=x; y[1]=1; x[0][1]; })");
    assert(2, ({ int x[2][3]; int *y=x; y[2]=2; x[0][2]; }), "({ int x[2][3]; int *y=x; y[2]=2; x[0][2]; })");
    assert(3, ({ int x[2][3]; int *y=x; y[3]=3; x[1][0]; }), "({ int x[2][3]; int *y=x; y[3]=3; x[1][0]; })");
    assert(4, ({ int x[2][3]; int *y=x; y[4]=4; x[1][1]; }), "({ int x[2][3]; int *y=x; y[4]=4; x[1][1]; })");
    assert(5, ({ int x[2][3]; int *y=x; y[5]=5; x[1][2]; }), "({ int x[2][3]; int *y=x; y[5]=5; x[1][2]; })");

    assert(4, ({ int x; sizeof(x); }), "({ int x; sizeof(x); })");
    assert(4, ({ int x; sizeof x; }), "({ int x; sizeof x; })");
    assert(8, ({ int *x; sizeof(x); }), "({ int *x; sizeof(x); })");
    assert(16, ({ int x[4]; sizeof(x); }), "({ int x[4]; sizeof(x); })");
    assert(48, ({ int x[3][4]; sizeof(x); }), "({ int x[3][4]; sizeof(x); })");
    assert(16, ({ int x[3][4]; sizeof(*x); }), "({ int x[3][4]; sizeof(*x); })");
    assert(4, ({ int x[3][4]; sizeof(**x); }), "({ int x[3][4]; sizeof(**x); })");
    assert(5, ({ int x[3][4]; sizeof(**x) + 1; }), "({ int x[3][4]; sizeof(**x) + 1; })");
    assert(5, ({ int x[3][4]; sizeof **x + 1; }), "({ int x[3][4]; sizeof **x + 1; })");
    assert(4, ({ int x[3][4]; sizeof(**x + 1); }), "({ int x[3][4]; sizeof(**x + 1); })");
    assert(4, ({ int x=1; sizeof(x=2); }), "({ int x=1; sizeof(x=2); })");
    assert(1, ({ int x=1; sizeof(x=2); x; }), "({ int x=1; sizeof(x=2); x; })");

    assert(0, g1, "g1");
    assert(3, ({ g1=3; g1; }), "({ g1=3; g1; })");
    assert(0, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[0]; }), "({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[0]; })");
    assert(1, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[1]; }), "({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[1]; })");
    assert(2, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[2]; }), "({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[2]; })");
    assert(3, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[3]; }), "({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[3]; })");

    assert(4, sizeof(g1), "sizeof(g1)");
    assert(16, sizeof(g2), "sizeof(g2)");

    assert(1, ({ char x=1; x; }), "({ char x=1; x; })");
    assert(1, ({ char x=1; char y=2; x; }), "({ char x=1; char y=2; x; })");
    assert(2, ({ char x=1; char y=2; y; }), "({ char x=1; char y=2; y; })");

    assert(1, ({ char x; sizeof(x); }), "({ char x; sizeof(x); })");
    assert(10, ({ char x[10]; sizeof(x); }), "({ char x[10]; sizeof(x); })");
    assert(1, ({ sub_char(7, 3, 3); }), "({ sub_char(7, 3, 3); })");

    assert(0, ""[0], "\"\"[0]");
    assert(1, sizeof(""), "sizeof(\"\")");

    assert(97, "abc"[0], "\"abc\"[0]");
    assert(98, "abc"[1], "\"abc\"[1]");
    assert(99, "abc"[2], "\"abc\"[2]");
    assert(0, "abc"[3], "\"abc\"[3]");
    assert(4, sizeof("abc"), "sizeof(\"abc\")");

    assert(7, "\a"[0], "\"\\a\"[0]");
    assert(8, "\b"[0], "\"\\b\"[0]");
    assert(9, "\t"[0], "\"\\t\"[0]");
    assert(10, "\n"[0], "\"\\n\"[0]");
    assert(11, "\v"[0], "\"\\v\"[0]");
    assert(12, "\f"[0], "\"\\f\"[0]");
    assert(13, "\r"[0], "\"\\r\"[0]");
    assert(27, "\e"[0], "\"\\e\"[0]");

    assert(106, "\j"[0], "\"\\j\"[0]");
    assert(107, "\k"[0], "\"\\k\"[0]");
    assert(108, "\l"[0], "\"\\l\"[0]");

    assert(7, "\ax\ny"[0], "\"\\ax\\ny\"[0]");
    assert(120, "\ax\ny"[1], "\"\\ax\\ny\"[1]");
    assert(10, "\ax\ny"[2], "\"\\ax\\ny\"[2]");
    assert(121, "\ax\ny"[3], "\"\\ax\\ny\"[3]");

    assert(0, "\0"[0], "\"\\0\"[0]");
    assert(16, "\20"[0], "\"\\20\"[0]");
    assert(65, "\101"[0], "\"\\101\"[0]");
    assert(104, "\1500"[0], "\"\\1500\"[0]");
    assert(0, "\x00"[0], "\"\\x00\"[0]");
    assert(119, "\x77"[0], "\"\\x77\"[0]");

    assert(2, ({ int x=2; { int x=3; } x; }), "({ int x=2; { int x=3; } x; })");
    assert(2, ({ int x=2; { int x=3; } int y=4; x; }), "({ int x=2; { int x=3; } int y=4; x; })");
    assert(3, ({ int x=2; { x=3; } x; }), "({ int x=2; { x=3; } x; })");

    assert(3, (1,2,3), "(1,2,3)");
    assert(5, ({ int i=2, j=3; (i=5,j)=6; i; }), "({ int i=2, j=3; (i=5,j)=6; i; })");
    assert(6, ({ int i=2, j=3; (i=5,j)=6; j; }), "({ int i=2, j=3; (i=5,j)=6; j; })");

    assert(21, add6(1,2,3,4,5,6), "add6(1,2,3,4,5,6)");

    assert(2, ({ int x[5]; int *y=x+2; y-x; }), "({ int x[5]; int *y=x+2; y-x; })");

    assert(1, ({ struct {int a; int b;} x; x.a=1, x.b=2; x.a; }), "({ struct {int a; int b;} x; x.a=1, x.b=2; x.a; })");
    assert(2, ({ struct {int a; int b;} x; x.a=1, x.b=2; x.b; }), "({ struct {int a; int b;} x; x.a=1, x.b=2; x.b; })");
    assert(1, ({ struct {int a; int b; char c;} x; x.a=1, x.b=2; x.c=3; x.a; }), "({ struct {int a; int b; char c;} x; x.a=1, x.b=2; x.c=3; x.a; })");
    assert(2, ({ struct {int a; int b; char c;} x; x.a=1, x.b=2; x.c=3; x.b; }), "({ struct {int a; int b; char c;} x; x.a=1, x.b=2; x.c=3; x.b; })");
    assert(3, ({ struct {int a; int b; char c;} x; x.a=1, x.b=2; x.c=3; x.c; }), "({ struct {int a; int b; char c;} x; x.a=1, x.b=2; x.c=3; x.c; })");

    assert(0, ({ struct {char a; char b;} x[3]; char *p=x; p[0]=0; x[0].a; }), "({ struct {char a; char b;} x[3]; char *p=x; p[0]=0; x[0].a; })");
    assert(1, ({ struct {char a; char b;} x[3]; char *p=x; p[1]=1; x[0].b; }), "({ struct {char a; char b;} x[3]; char *p=x; p[1]=1; x[0].b; })");
    assert(2, ({ struct {char a; char b;} x[3]; char *p=x; p[2]=2; x[1].a; }), "({ struct {char a; char b;} x[3]; char *p=x; p[2]=2; x[1].a; })");
    assert(3, ({ struct {char a; char b;} x[3]; char *p=x; p[3]=3; x[1].b; }), "({ struct {char a; char b;} x[3]; char *p=x; p[3]=3; x[1].b; })");

    assert(6, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.a[0]=6; p[0]; }), "({ struct {char a[3]; char b[5];} x; char *p=&x; x.a[0]=6; p[0]; })");
    assert(7, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.b[0]=7; p[3]; }), "({ struct {char a[3]; char b[5];} x; char *p=&x; x.a[0]=6; p[0]; })");

    assert(6, ({ struct { struct { char b; } a; } x; x.a.b=6; x.a.b; }), "({ struct { struct { char b; } a; } x; x.a.b=6; x.a.b; })");
    assert(4, ({ struct {int a;} x; sizeof(x); }), "({ struct {int a;} x; sizeof(x); })");
    assert(8, ({ struct {int a; int b;} x; sizeof(x); }), "({ struct {int a; int b;} x; sizeof(x); })");
    assert(8, ({ struct {int a, b;} x; sizeof(x); }), "({ struct {int a, b;} x; sizeof(x); })");
    assert(12, ({ struct {int a[3];} x; sizeof(x); }), "({ struct {int a[3];} x; sizeof(x); })");
    assert(16, ({ struct {int a;} x[4]; sizeof(x); }), "({ struct {int a;} x[4]; sizeof(x); })");
    assert(24, ({ struct {int a[3];} x[2]; sizeof(x); }), "({ struct {int a[3];} x[2]; sizeof(x); })");
    assert(2, ({ struct {char a; char b;} x; sizeof(x); }), "({ struct {char a; char b;} x; sizeof(x); })");
    assert(8, ({ struct {char a; int b;} x; sizeof(x); }), "({ struct {char a; int b;} x; sizeof(x); })");
    assert(8, ({ struct {int a; char b;} x; sizeof(x); }), "({ struct {int a; char b;} x; sizeof(x); })");

    assert(7, ({ int x; int y; char z; char *a=&y; char *b=&z; b-a; }), "({ int x; int y; char z; char *a=&y; char *b=&z; b-a; })");
    assert(1, ({ int x; char y; int z; char *a=&y; char *b=&z; b-a; }), "({ int x; char y; int z; char *a=&y; char *b=&z; b-a; })");

    assert(2, ({ struct t {char a[2];}; { struct t {char a[4];}; } struct t y; sizeof(y); }), "({ struct t {char a[2];}; { struct t {char a[4];}; } struct t y; sizeof(y); })");
    assert(3, ({ struct t {int x;}; int t=1; struct t y; y.x=2; t+y.x; }), "({ struct t {int x;}; int t=1; struct t y; y.x=2; t+y.x; })");

    assert(3, ({ struct t {char a;} x; struct t *y = &x; x.a=3; y->a; }), "({ struct t {char a;} x; struct t *y = &x; x.a=3; y->a; })");
    assert(3, ({ struct t {char a;} x; struct t *y = &x; y->a=3; x.a; }), "({ struct t {char a;} x; struct t *y = &x; y->a=3; x.a; })");
    assert(8, ({ union { int a; char b[6]; } x; sizeof(x); }), "({ union { int a; char b[6]; } x; sizeof(x); })");
    assert(3, ({ union { int a; char b[4]; } x; x.a = 515; x.b[0]; }), "({ union { int a; char b[4]; } x; x.a = 515; x.b[0]; })");
    assert(2, ({ union { int a; char b[4]; } x; x.a = 515; x.b[1]; }), "({ union { int a; char b[4]; } x; x.a = 515; x.b[1]; })");
    assert(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[2]; }), "({ union { int a; char b[4]; } x; x.a = 515; x.b[2]; })");
    assert(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[3]; }), "({ union { int a; char b[4]; } x; x.a = 515; x.b[3]; })");

    assert(3, ({ struct {int a,b;} x,y; x.a=3; y=x; y.a; }), "({ struct {int a,b;} x,y; x.a=3; y=x; y.a; })");
    assert(5, ({ struct t {int a,b;}; struct t x; x.a=5; struct t y=x; y.a; }), "({ struct t {int a,b;}; struct t x; x.a=5; struct t y=x; y.a; })");
    assert(7, ({ struct t {int a,b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x; y.a; }), "({ struct t {int a,b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x; y.a; })");
    assert(7, ({ struct t {int a,b;}; struct t x; x.a=7; struct t y, *p=&x, *q=&y; *q=*p; y.a; }), "({ struct t {int a,b;}; struct t x; x.a=7; struct t y, *p=&x, *q=&y; *q=*p; y.a; })");
    assert(5, ({ struct t {char a, b;} x, y; x.a=5; y=x; y.a; }), "({ struct t {char a, b;} x, y; x.a=5; y=x; y.a; })");

    assert(8, ({ struct t {int a; int b;} x; struct t y; sizeof(y); }), "({ struct t {int a; int b;} x; struct t y; sizeof(y); })");
    assert(8, ({ struct t {int a; int b;}; struct t y; sizeof(y); }), "({ struct t {int a; int b;}; struct t y; sizeof(y); })");

    assert(2, ({ short x; sizeof(x); }), "({ short x; sizeof(x); })");
    assert(4, ({ struct {char a; short b;} x; sizeof(x); }), "({ struct {char a; short b;} x; sizeof(x); })");

    assert(8, ({ long x; sizeof(x); }), "({ long x; sizeof(x); })");
    assert(16, ({ struct {char a; long b;} x; sizeof(x); }), "({ struct {char a; long b;} x; sizeof(x); })");

    assert(1, sub_short(7, 3, 3), "sub_short(7, 3, 3)");
    assert(1, sub_long(7, 3, 3), "sub_long(7, 3, 3)");

    assert(24, ({ int *x[3]; sizeof(x); }), "({ int *x[3]; sizeof(x); })");
    assert(8, ({ int (*x)[3]; sizeof(x); }), "({ int (*x)[3]; sizeof(x); })");
    assert(3, ({ int *x[3]; int y; x[0]=&y; y=3; x[0][0]; }), "({ int *x[3]; int y; x[0]=&y; y=3; x[0][0]; })");
    assert(4, ({ int x[3]; int (*y)[3]=x; y[0][0]=4; y[0][0]; }), "({ int x[3]; int (*y)[3]=x; y[0][0]=4; y[0][0]; })");

    { void *x; }

    assert(1, ({ char x; sizeof(x); }), "({ char x; sizeof(x); })");
    assert(2, ({ short int x; sizeof(x); }), "({ short int x; sizeof(x); })");
    assert(2, ({ int short x; sizeof(x); }), "({ int short x; sizeof(x); })");
    assert(4, ({ int x; sizeof(x); }), "({ int x; sizeof(x); })");
    assert(8, ({ long int x; sizeof(x); }), "({ long int x; sizeof(x); })");
    assert(8, ({ int long x; sizeof(x); }), "({ int long x; sizeof(x); })");
    assert(8, ({ long long x; sizeof(x); }), "({ long long x; sizeof(x); })");
    assert(8, ({ long long int x; sizeof(x); }), "({ long long int x; sizeof(x); })");

    assert(1, ({ typedef int t; t x=1; x; }), "({ typedef int t; t x=1; x; })");
    assert(1, ({ typedef struct {int a;} t; t x; x.a=1; x.a; }), "({ typedef struct {int a;} t; t x; x.a=1; x.a; })");
    assert(1, ({ typedef int t; t t=1; t; }), "({ typedef int t; t t=1; t; })");
    assert(2, ({ typedef struct {int a;} t; { typedef int t; } t x; x.a=2; x.a; }), "({ typedef struct {int a;} t; { typedef int t; } t x; x.a=2; x.a; })");
    assert(4, ({ typedef t; t x; sizeof(x); }), "({ typedef t; t x; sizeof(x); })");
    assert(3, ({ MyInt x=3; x; }), "({ MyInt x=3; x; })");
    assert(16, ({ MyInt2 x; sizeof(x); }), "({ MyInt2 x; sizeof(x)}; )");

    assert(1, sizeof(char), "sizeof(char)");
    assert(2, sizeof(short), "sizeof(short)");
    assert(2, sizeof(short int), "sizeof(short int)");
    assert(2, sizeof(int short), "sizeof(int short)");
    assert(4, sizeof(int), "sizeof(int)");
    assert(8, sizeof(long), "sizeof(long)");
    assert(8, sizeof(long int), "sizeof(long int)");
    assert(8, sizeof(long long int), "sizeof(long long int)");
    assert(8, sizeof(char *), "sizeof(char *)");
    assert(8, sizeof(int *), "sizeof(int *)");
    assert(8, sizeof(long *), "sizeof(long *)");
    assert(8, sizeof(int **), "sizeof(int **)");
    assert(8, sizeof(int(*)[4]), "sizeof(int(*)[4])");
    assert(32, sizeof(int*[4]), "sizeof(int*[4])");
    assert(16, sizeof(int[4]), "sizeof(int[4])");
    assert(48, sizeof(int[3][4]), "sizeof(int[3][4])");
    assert(8, sizeof(struct {int a; int b;}), "sizeof(struct {int a; int b;})");

    assert(131585, (int)8590066177, "(int)8590066177");
    assert(513, (short)8590066177, "(short)8590066177");
    assert(1, (char)8590066177, "(char)8590066177");
    assert(1, (long)1, "(long)1");
    assert(0, (long)&*(int *)0, "(long)&*(int *)0");
    assert(513, ({ int x=512; *(char *)&x=1; x; }), "({ int x=512; *(char *)&x=1; x; })");
    assert(5, ({ int x=5; long y=(long)&x; *(int*)y; }), "({ int x=5; long y=(long)&x; *(int*)y; })");

    (void)1;

    assert(4, sizeof(-10 + 5), "sizeof(-10 + 5)");
    assert(4, sizeof(-10 - 5), "sizeof(-10 - 5)");
    assert(4, sizeof(-10 * 5), "sizeof(-10 * 5)");
    assert(4, sizeof(-10 / 5), "sizeof(-10 / 5)");
  
    assert(8, sizeof(-10 + (long)5), "sizeof(-10 + (long)5)");
    assert(8, sizeof(-10 - (long)5), "sizeof(-10 - (long)5)");
    assert(8, sizeof(-10 * (long)5), "sizeof(-10 * (long)5)");
    assert(8, sizeof(-10 / (long)5), "sizeof(-10 / (long)5)");
    assert(8, sizeof((long)-10 + 5), "sizeof((long)-10 + 5)");
    assert(8, sizeof((long)-10 - 5), "sizeof((long)-10 - 5)");
    assert(8, sizeof((long)-10 * 5), "sizeof((long)-10 * 5)");
    assert(8, sizeof((long)-10 / 5), "sizeof((long)-10 / 5)");
  
    assert((long)-5, -10 + (long)5, "-10 + (long)5");
    assert((long)-15, -10 - (long)5, "-10 - (long)5");
    assert((long)-50, -10 * (long)5, "-10 * (long)5");
    assert((long)-2, -10 / (long)5, "-10 / (long)5");
  
    assert(1, -2 < (long)-1, "-2 < (long)-1");
    assert(1, -2 <= (long)-1, "-2 <= (long)-1");
    assert(0, -2 > (long)-1, "-2 > (long)-1");
    assert(0, -2 >= (long)-1, "-2 >= (long)-1");
  
    assert(1, (long)-2 < -1, "(long)-2 < -1");
    assert(1, (long)-2 <= -1, "(long)-2 <= -1");
    assert(0, (long)-2 > -1, "(long)-2 > -1");
    assert(0, (long)-2 >= -1, "(long)-2 >= -1");
  
    assert(0, 2147483647 + 2147483647 + 2, "2147483647 + 2147483647 + 2");
    assert((long)-1, ({ long x; x=-1; x; }), "({ long x; x=-1; x; })");
  
    assert(1, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[0]; }), "({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[0]; })");
    assert(0, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[-1]; }), "({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[-1]; })");
    assert(5, ({ struct t {char a;} x, y; x.a=5; y=x; y.a; }), "({ struct t {char a;} x, y; x.a=5; y=x; y.a; })");

    assert(3, *g1_ptr(), "*g1_ptr()");
    assert(5, int_to_char(261), "int_to_char(261)");
    assert(-5, div_long(-10, 2), "div_long(-10, 2)");

    assert(0, ({ _Bool x=0; x; }), "({ _Bool x=0; x; })");
    assert(1, ({ _Bool x=1; x; }), "({ _Bool x=1; x; })");
    assert(1, ({ _Bool x=2; x; }), "({ _Bool x=2; x; })");
    assert(1, (_Bool)1, "(_Bool)1");
    assert(1, (_Bool)2, "(_Bool)2");
    assert(0, (_Bool)(char)256, "(_Bool)(char)256");
    assert(1, bool_fn_add(3), "bool_fn_add(3)");
    assert(0, bool_fn_sub(3), "bool_fn_sub(3)");
    assert(1, bool_fn_add(-3), "bool_fn_add(-3)");
    assert(0, bool_fn_sub(-3), "bool_fn_sub(-3)");
    assert(1, bool_fn_add(0), "bool_fn_add(0)");
    assert(1, bool_fn_sub(0), "bool_fn_sub(0)");

    assert(97, 'a', "'a'");
    assert(10, '\n', "'\n'");
    assert(4, sizeof('a'), "sizeof('a')");

    assert(0, ({ enum { zero, one, two }; zero; }), "({ enum { zero, one, two }; zero; })");
    assert(1, ({ enum { zero, one, two }; one; }), "({ enum { zero, one, two }; one; })");
    assert(2, ({ enum { zero, one, two }; two; }), "({ enum { zero, one, two }; two; })");
    assert(5, ({ enum { five=5, six, seven }; five; }), "({ enum { five=5, six, seven }; five; })");
    assert(6, ({ enum { five=5, six, seven }; six; }), "({ enum { five=5, six, seven }; six; })");
    assert(0, ({ enum { zero, five=5, three=3, four }; zero; }), "({ enum { zero, five=5, three, four }; zero; })");
    assert(5, ({ enum { zero, five=5, three=3, four }; five; }), "({ enum { zero, five=5, three, four }; five; })");
    assert(3, ({ enum { zero, five=5, three=3, four }; three; }), "({ enum { zero, five=5, three, four }; three; })");
    assert(4, ({ enum { zero, five=5, three=3, four }; four; }), "({ enum { zero, five=5, three, four }; four; })");
    assert(4, ({ enum { zero, one, two } x; sizeof(x); }), "({ enum { zero, one, two } x; sizeof(x); })");
    assert(4, ({ enum t { zero, one, two }; enum t y; sizeof(y); }), "({ enum t { zero, one, two }; enum t y; sizeof(y); })");

    assert(3, static_fn(), "static_fn()");

    assert(55, ({ int j=0; for (int i=0; i<=10; i=i+1) j=j+i; j; }), "({ int j=0; for (int i=0; i<=10; i=i+1) j=j+i; j; })");
    assert(3, ({ int i=3; int j=0; for (int i=0; i<=10; i=i+1) j=j+i; i; }), "({ int i=3; int j=0; for (int i=0; i<=10; i=i+1) j=j+i; i; })");

    assert(7, ({ int i=2; i+=5; i; }), "({ int i=2; i+=5; i; })");
    assert(7, ({ int i=2; i+=5; }), "({ int i=2; i+=5; })");
    assert(3, ({ int i=5; i-=2; i; }), "({ int i=5; i-=2; i; })");
    assert(3, ({ int i=5; i-=2; }), "({ int i=5; i-=2; })");
    assert(6, ({ int i=3; i*=2; i; }), "({ int i=3; i*=2; i; })");
    assert(6, ({ int i=3; i*=2; }), "({ int i=3; i*=2; })");
    assert(3, ({ int i=6; i/=2; i; }), "({ int i=6; i/=2; i; })");
    assert(3, ({ int i=6; i/=2; }), "({ int i=6; i/=2; })"); assert(3, ({ int i=2; ++i; }), "({ int i=2; ++i; })");
    assert(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; ++*p; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; ++*p; })");
    assert(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; --*p; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; --*p; })");

    assert(2, ({int i=2; i++; }), "({int i=2; i++; })");
    assert(2, ({int i=2; i--; }), "({int i=2; i--; })");
    assert(3, ({int i=2; i++; i; }), "({int i=2; i++; i; })");
    assert(1, ({int i=2; i--; i; }), "({int i=2; i--; i; })");
    assert(1, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p++; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p++; })");
    assert(1, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p--; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p--; })");

    assert(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; })");
    assert(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*(p--))--; a[1]; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*(p--))--; a[1]; })");
    assert(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; a[2]; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; a[2]; })");
    assert(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; p++; *p; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; p++; *p; })");

    assert(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; })");
    assert(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[1]; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[1]; })");
    assert(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[2]; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[2]; })");
    assert(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; *p; }), "({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; *p; })");

    assert(511, 0777, "0777");
    assert(0, 0x0, "0x0");
    assert(10, 0xa, "0xa");
    assert(10, 0XA, "0XA");
    assert(48879, 0xbeef, "0xbeef");
    assert(48879, 0xBEEF, "0xBEEF");
    assert(48879, 0XBEEF, "0XBEEF");
    assert(0, 0b0, "0b0");
    assert(1, 0b1, "0b1");
    assert(47, 0b101111, "0b101111");
    assert(47, 0B101111, "0B101111");

    assert(0, !1, "!1");
    assert(0, !2, "!2");
    assert(1, !0, "!0");
    assert(1, !(char)0, "!(char)0");
    assert(0, !(long)3, "!(long)3");
    assert(4, sizeof(!(char)0), "sizeof(!(char)0)");
    assert(4, sizeof(!(long)0), "sizeof(!(long)0)");

    assert(-1, ~0, "~0");
    assert(0, ~-1, "~-1");

    assert(5, 17%6, "17%6");
    assert(5, ((long)17)%6, "((long)17)%6");
    assert(2, ({ int i=10; i%=4; i; }), "({ int i=10; i%4; i; })");
    assert(2, ({ long i=10; i%=4; i; }), "({ long i=10; i%4; i; })");

    assert(0, 0&1, "0&1");
    assert(1, 3&1, "3&1");
    assert(3, 7&3, "7&3");
    assert(10, -1&10, "-1&10");

    assert(0, 0^0, "0^0");
    assert(0, 0b1111^0b1111, "0b1111^0b1111");
    assert(0b110100, 0b111000^0b001100, "0b111000^0b001100");

    assert(2, ({ int i=6; i&=3; i; }), "({ int i=6; i&=3; i; })");
    assert(7, ({ int i=6; i|=3; i; }), "({ int i=6; i|=3; i; })");
    assert(10, ({ int i=15; i^=5; i; }), "({ int i=15; i^=5; i; })");

    assert(1, 0||1, "0||1");
    assert(1, 0||(2-2)||5, "0||(2-2)||5");
    assert(0, 0||0, "0||0");
    assert(0, 0||(2-2), "0||(2-2)");

    assert(0, 0&&1, "0&&1");
    assert(0, (2-2)&&5, "(2-2)&&5");
    assert(1, 1&&5, "1&&5");

    assert(8, sizeof(int(*)[10]), "sizeof(int(*)[10])");
    assert(8, sizeof(int(*)[][10]), "sizeof(int(*)[][10])");

    assert(3, ({ int x[2]; x[0]=3; param_decay(x); }), "({ int x[2]; x[0]=3; param_decay(x); })");

    assert(8, ({ struct foo *bar; sizeof(bar); }), "({ struct foo *bar; sizeof(bar); })");
    assert(4, ({ struct T *foo; struct T {int x;}; sizeof(struct T); }), "({ struct T *foo; struct T {int x;}; sizeof(struct T); })");
    assert(1, ({ struct T { struct T *next; int x; } a; struct T b; b.x=1; a.next=&b; a.next->x; }), "({ struct T { struct T *next; int x; } a; struct T b; b.x=1; a.next=&b; a.next->x; })");
    assert(4, ({ typedef struct T T; struct T { int x; }; sizeof(T); }), "({ typedef struct T T; struct T { int x; }; sizeof(T); })");

    assert(3, ({ int i=0; for(;i<10;i++) { if (i == 3) break; } i; }), "({ int i=0; for(;i<10;i++) { if (i == 3) break; } i; })");
    assert(4, ({ int i=0; while (1) { if (i++ == 3) break; } i; }), "({ int i=0; while (i) { if (i++ == 3) break; } i; })");
    assert(3, ({ int i=0; for(;i<10;i++) { for (;;) break; if (i == 3) break; } i; }), "({ int i=0; for(;i<10;i++) { for (;;) break; if (i == 3) break; } i; })");
    assert(4, ({ int i=0; while (1) { while(1) break; if (i++ == 3) break; } i; }), "({ int i=0; while (1) { while(1) break; if (i++ == 3) break; } i; })");

    assert(10, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } i; }), "({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } i; })");
    assert(6, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } j; }), "({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } j; })");
    assert(10, ({ int i=0; int j=0; for(;!i;) { for (;j!=10;j++) continue; break; } j; }), "({ int i=0; int j=0; for(;!i;) { for (;j!=10;j++) continue; break; } j; })");
    assert(11, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } i; }), "({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } i; })");
    assert(5, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } j; }), "({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } j; })");
    assert(11, ({ int i=0; int j=0; while(!i) { while (j++!=10) continue; break; } j; }), "({ int i=0; int j=0; while(!i) { while (j++!=10) continue; break; } j; })");

    assert(3, ({ int i=0; goto a; a: i++; b: i++; c: i++; i; }), "({ int i=0; goto a; a: i++; b: i++; c: i++; i; })");
    assert(2, ({ int i=0; goto e; d: i++; e: i++; f: i++; i; }), "({ int i=0; goto e; d: i++; e: i++; f: i++; i; })");
    assert(1, ({ int i=0; goto i; g: i++; h: i++; i: i++; i; }), "({ int i=0; goto i; g: i++; h: i++; i: i++; i; })");

    assert(5, ({ int i=0; switch(0) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }), "({ int i=0; switch(0) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; })");
    assert(6, ({ int i=0; switch(1) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }), "({ int i=0; switch(1) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; })");
    assert(7, ({ int i=0; switch(2) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }), "({ int i=0; switch(2) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; })");
    assert(0, ({ int i=0; switch(3) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }), "({ int i=0; switch(3) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; })");
    assert(5, ({ int i=0; switch(0) { case 0:i=5;break; default:i=7; } i; }), "({ int i=0; switch(0) { case 0:i=5;break; default:i=7; } i; })");
    assert(7, ({ int i=0; switch(1) { case 0:i=5;break; default:i=7; } i; }), "({ int i=0; switch(1) { case 0:i=5;break; default:i=7; } i; })");
    assert(2, ({ int i=0; switch(1) { case 0: 0; case 1: 0; case 2: 0; i=2; } i; }), "({ int i=0; switch(1) { case 0: 0; case 1: 0; case 2: 0; i=2; } i; })");
    assert(0, ({ int i=0; switch(3) { case 0: 0; case 1: 0; case 2: 0; i=2; } i; }), "({ int i=0; switch(3) { case 0: 0; case 1: 0; case 2: 0; i=2; } i; })");

    assert(1, 1<<0, "1<<0");
    assert(8, 1<<3, "1<<3");
    assert(10, 5<<1, "5<<1");
    assert(2, 5>>1, "5>>1");
    assert(-1, -1>>1, "-1>>1");
    assert(1, ({ int i=1; i<<=0; i;}), "({ int i=1; i<<=0; i;})");
    assert(8, ({ int i=1; i<<=3; i;}), "({ int i=1; i<<=3; i;})");
    assert(10, ({ int i=5; i<<=1; i;}), "({ int i=5; i<<=1; i;})");
    assert(2, ({ int i=5; i>>=1; i;}), "({ int i=5; i>>=1; i;})");
    assert(-1, -1, "-1");
    assert(-1, ({ int i=-1; i; }), "({ int i=-1; i; })");
    assert(-1, ({ int i=-1; i>>=1; i; }), "({ int i=-1; i>>=1; i; })");

    assert(2, 0?1:2, "0?1:2");
    assert(1, 1?1:2, "1?1:2");
    assert(-1, 0?-2:-1, "0?-2:-1");
    assert(-2, 1?-2:-1, "1?-2:-1");
    assert(4, sizeof(0?1:2), "sizeof(0?1:2)");
    assert(8, sizeof(0?(long)1:(long)2), "sizeof(0?(long)1:(long)2)");
    assert(-1, 0?(long)-2:-1, "0?(long)-2:-1");
    assert(-1, 0?-2:(long)-1, "0?-2:(long)-1");
    assert(-2, 1?(long)-2:-1, "1?(long)-2:-1");
    assert(-2, 1?-2:(long)-1, "1?-2:(long)-1");

    1 ? -2 : (void)-1;

    assert(10, ({ enum { ten=1+2+3+4 }; ten; }), "({ enum { ten=1+2+3+4 }; ten; })");
    assert(1, ({ int i=0; switch(3) { case 5-2+0*3: i++; } i; }), "({ int i=0; switch(3) { case 5-2+0*3: i++; } i; })");
    assert(8, ({ int x[1+1]; sizeof(x); }), "({ int x[1+1]; sizeof(x); })");
    assert(6, ({ char x[8-2]; sizeof(x); }), "({ char x[8-2]; sizeof(x); })");
    assert(6, ({ char x[2*3]; sizeof(x); }), "({ char x[2*3]; sizeof(x); })");
    assert(3, ({ char x[12/4]; sizeof(x); }), "({ char x[12/4]; sizeof(x); })");
    assert(0b100, ({ char x[0b110&0b101]; sizeof(x); }), "({ char x[0b110&0b101]; sizeof(x); })");
    assert(0b111, ({ char x[0b110|0b101]; sizeof(x); }), "({ char x[0b110|0b101]; sizeof(x); })");
    assert(0b110, ({ char x[0b111^0b001]; sizeof(x); }), "{ char x[0b111^0b001]; sizeof(x); })");
    assert(4, ({ char x[1<<2]; sizeof(x); }), "({ char x[1<<2]; sizeof(x); })");
    assert(2, ({ char x[4>>1]; sizeof(x); }), "({ char x[4>>1]; sizeof(x); })");
    assert(2, ({ char x[(1==1)+1]; sizeof(x); }), "({ char x[(1==1)+1]; sizeof(x); })");
    assert(1, ({ char x[(1!=1)+1]; sizeof(x); }), "({ char x[(1!=1)+1]; sizeof(x); })");
    assert(1, ({ char x[(1<1)+1]; sizeof(x); }), "({ char x[(1<1)+1]; sizeof(x); })");
    assert(2, ({ char x[(1<=1)+1]; sizeof(x); }), "({ char x[(1<=1)+1]; sizeof(x); })");
    assert(2, ({ char x[1?2:3]; sizeof(x); }), "({ char x[1?2:3]; sizeof(x); })");
    assert(3, ({ char x[0?2:3]; sizeof(x); }), "({ char x[0?2:3]; sizeof(x); })");
    assert(3, ({ char x[(1,3)]; sizeof(x); }), "({ char x[(1,3)]; sizeof(x); })");
    assert(2, ({ char x[!0+1]; sizeof(x); }), "({ char x[!0+1]; sizeof(x); })");
    assert(1, ({ char x[!1+1]; sizeof(x); }), "({ char x[!1+1]; sizeof(x); })");
    assert(2, ({ char x[~-3]; sizeof(x); }), "({ char x[~-3]; sizeof(x); })");
    assert(2, ({ char x[(5||6)+1]; sizeof(x); }), "({ char x[(5||6)+1]; sizeof(x); })");
    assert(1, ({ char x[(0||0)+1]; sizeof(x); }), "({ char x[(0||0)+1]; sizeof(x); })");
    assert(2, ({ char x[(1&&1)+1]; sizeof(x); }), "({ char x[(1&&1)+1]; sizeof(x); })");
    assert(1, ({ char x[(1&&0)+1]; sizeof(x); }), "({ char x[(1&&0)+1]; sizeof(x); })");
    assert(3, ({ char x[(int)3]; sizeof(x); }), "({ char x[(int)3]; sizeof(x); })");
    assert(15, ({ char x[(char)0xffffff0f]; sizeof(x); }), "({ char x[(char)0xffffff0f]; sizeof(x); })");
    assert(0x10f, ({ char x[(short)0xffff010f]; sizeof(x); }), "({ char x[(short)0xffff010f]; sizeof(x); })");
    assert(4, ({ char x[(int)0xfffffffffff+5]; sizeof(x); }), "({ char x[(int)0xfffffffffff+5]; sizeof(x); })");
    assert(8, ({ char x[(int*)0+2]; sizeof(x); }), "({ char x[(int*)0+2]; sizeof(x); })");
    assert(12, ({ char x[(int*)16-1]; sizeof(x); }), "({ char x[(int*)16-1]; sizeof(x); })");
    assert(3, ({ char x[(int*)16-(int*)4]; sizeof(x); }), "({ char x[(int*)16-(int*)4]; sizeof(x); })");

    assert(1, ({ int x[3]={1,2,3}; x[0]; }), "({ int x[3]={1,2,3}; x[0]; })");
    assert(2, ({ int x[3]={1,2,3}; x[1]; }), "({ int x[3]={1,2,3}; x[1]; })");
    assert(3, ({ int x[3]={1,2,3}; x[2]; }), "({ int x[3]={1,2,3}; x[2]; })");
    assert(3, ({ int x[3]={1,2,3}; x[2]; }), "({ int x[3]={1,2,3}; x[2]; })");

    assert(2, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[0][1]; }), "({ int x[2][3]={{1,2,3},{4,5,6}}; x[0][1]; })");
    assert(4, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][0]; }), "({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][0]; })");
    assert(6, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][2]; }), "({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][2]; })");

    assert(0, ({ int x[3]={}; x[0]; }), "({ int x[3]={}; x[0]; })");
    assert(0, ({ int x[3]={}; x[1]; }), "({ int x[3]={}; x[1]; })");
    assert(0, ({ int x[3]={}; x[2]; }), "({ int x[3]={}; x[2]; })");

    assert(2, ({ int x[2][3]={{1,2}}; x[0][1]; }), "({ int x[2][3]={{1,2}}; x[0][1]; })");
    assert(0, ({ int x[2][3]={{1,2}}; x[1][0]; }), "({ int x[2][3]={{1,2}}; x[1][0]; })");
    assert(0, ({ int x[2][3]={{1,2}}; x[1][2]; }), "({ int x[2][3]={{1,2}}; x[1][2]; })");

    assert('a', ({ char x[4]="abc"; x[0]; }), "({ char x[4]=\"abc\"; x[0]; })");
    assert('c', ({ char x[4]="abc"; x[2]; }), "({ char x[4]=\"abc\"; x[2]; })");
    assert(0, ({ char x[4]="abc"; x[3]; }), "({ char x[4]=\"abc\"; x[3]; })");
    assert('a', ({ char x[2][4]={"abc","def"}; x[0][0]; }), "({ char x[2][4]={\"abc\",\"def\"}; x[0][0]; })");
    assert(0, ({ char x[2][4]={"abc","def"}; x[0][3]; }), "({ char x[2][4]={\"abc\",\"def\"}; x[0][3]; })");
    assert('d', ({ char x[2][4]={"abc","def"}; x[1][0]; }), "({ char x[2][4]={\"abc\",\"def\"}; x[1][0]; })");
    assert('f', ({ char x[2][4]={"abc","def"}; x[1][2]; }), "({ char x[2][4]={\"abc\",\"def\"}; x[1][2]; })");

    assert(4, ({ int x[]={1,2,3,4}; x[3]; }), "({ int x[]={1,2,3,4}; x[3]; })");
    assert(16, ({ int x[]={1,2,3,4}; sizeof(x); }), "({ int x[]={1,2,3,4}; sizeof(x); })");
    assert(4, ({ char x[]="foo"; sizeof(x); }), "({ char x[]=\"foo\"; sizeof(x); })");

    assert(1, ({ struct {int a; int b; int c;} x={1,2,3}; x.a; }), "({ struct {int a; int b; int c;} x={1,2,3}; x.a; })");
    assert(2, ({ struct {int a; int b; int c;} x={1,2,3}; x.b; }), "({ struct {int a; int b; int c;} x={1,2,3}; x.b; })");
    assert(3, ({ struct {int a; int b; int c;} x={1,2,3}; x.c; }), "({ struct {int a; int b; int c;} x={1,2,3}; x.c; })");
    assert(1, ({ struct {int a; int b; int c;} x={1}; x.a; }), "({ struct {int a; int b; int c;} x={1}; x.a; })");
    assert(0, ({ struct {int a; int b; int c;} x={1}; x.b; }), "({ struct {int a; int b; int c;} x={1}; x.b; })");
    assert(0, ({ struct {int a; int b; int c;} x={1}; x.c; }), "({ struct {int a; int b; int c;} x={1}; x.c; })");
    assert(1, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].a; }), "({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].a; })");
    assert(2, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].b; }), "({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].b; })");
    assert(3, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].a; }), "({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].a; })");
    assert(4, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].b; }), "({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].b; })");

    assert(0, ({ struct {int a; int b;} x[2]={{1,2}}; x[1].b; }), "({ struct {int a; int b;} x[2]={{1,2}}; x[1].b; })");

    assert(0, ({ struct {int a; int b;} x={}; x.a; }), "({ struct {int a; int b;} x={}; x.a; })");
    assert(0, ({ struct {int a; int b;} x={}; x.b; }), "({ struct {int a; int b;} x={}; x.b; })");

    assert(1, ({ typedef struct {int a,b;} T; T x={1,2}; T y=x; y.a; }), "({ typedef struct {int a,b;} T; T x={1,2}; T y=x; y.a; })");
    assert(5, ({ typedef struct {int a,b,c,d,e,f;} T; T x={1,2,3,4,5,6}; T y; y=x; y.e; }), "({ typedef struct {int a,b,c,d,e,f;} T; T x={1,2,3,4,5,6}; T y; y=x; y.e; })");
    assert(2, ({ typedef struct {int a,b;} T; T x={1,2}; T y, z; z=y=x; z.b; }), "({ typedef struct {int a,b;} T; T x={1,2}; T y, z; z=y=x; z.b; })");

    assert(3, g3, "g3");
    assert(4, g4, "g4");
    assert(5, g5, "g5");
    assert(6, g6, "g6");

    assert(0, g9[0], "g9[0]");
    assert(1, g9[1], "g9[1]");
    assert(2, g9[2], "g9[2]");

    assert(1, g11[0].a, "g11[0].a");
    assert(2, g11[0].b, "g11[0].b");
    assert(3, g11[1].a, "g11[1].a");
    assert(4, g11[1].b, "g11[1].b");

    assert(1, g12[0].a[0], "g12[0].a[0]");
    assert(2, g12[0].a[1], "g12[0].a[1]");
    assert(0, g12[1].a[0], "g12[1].a[0]");
    assert(0, g12[1].a[1], "g12[1].a[1]");

    assert(7, sizeof(g17), "sizeof(g17)");
    assert(10, sizeof(g18), "sizeof(g18)");
    assert(3, sizeof(g19), "sizeof(g19)");

    assert(0, memcmp(g17, "foobar", 7), "memcmp(g17, \"foobar\", 7)");
    assert(0, memcmp(g18, "foobar\0\0\0", 10), "memcmp(g18, \"foobar\\0\\0\\0\", 10)");
    assert(0, memcmp(g19, "foo", 3), "memcmp(g19, \"foo\", 3)");

    assert(0, strcmp(g20, "foobar"), "strcmp(g20, \"foobar\")");
    assert(0, strcmp(g21, "bar"), "strcmp(g21, \"bar\")");
    assert(0, strcmp(g22+3, "foobar"), "strcmp(g22+3, \"foobar\")");

    assert(0, strcmp(g23[0], "foobar"), "strcmp(g23[0], \"foobar\")");
    assert(0, strcmp(g23[1], "bar"), "strcmp(g23[1], \"bar\")");
    assert(0, strcmp(g23[2]+3, "foobar"), "strcmp(g23[2]+3, \"foobar\")");

    assert(3, g24, "g24");
    assert(3, *g25, "*g25");
    assert(2, *g27, "*g27");
    assert(3, *g28, "*g28");

    assert(1, g30[0].a[0], "g30[0].a[0]");
    assert(2, g30[0].a[1], "g30[0].a[1]");
    assert(3, g30[1].a[0], "g30[1].a[0]");
    assert(4, g30[1].a[1], "g30[1].a[1]");

    assert(1, g31[0].a[0], "g31[0].a[0]");
    assert(2, g31[0].a[1], "g31[0].a[1]");
    assert(3, g31[1].a[0], "g31[1].a[0]");
    assert(4, g31[1].a[1], "g31[1].a[1]");

    assert(0, ({ int x[2][3]={0,1,2,3,4,5}; x[0][0]; }), "({ int x[2][3]={0,1,2,3,4,5}; x[0][0]; })");
    assert(3, ({ int x[2][3]={0,1,2,3,4,5}; x[1][0]; }), "({ int x[2][3]={0,1,2,3,4,5}; x[1][0]; })");

    assert(0, ({ struct {int a; int b;} x[2]={0,1,2,3}; x[0].a; }), "({ struct {int a; int b;} x[2]={0,1,2,3}; x[0].a; })");
    assert(2, ({ struct {int a; int b;} x[2]={0,1,2,3}; x[1].a; }), "({ struct {int a; int b;} x[2]={0,1,2,3}; x[1].a; })");

    assert(0, strcmp(g33[0], "foo"), "strcmp(g33[0], \"foo\")");
    assert(0, strcmp(g33[1], "bar"), "strcmp(g33[1], \"bar\")");
    assert(0, strcmp(g34, "foo"), "strcmp(g34, \"foo\")");

    assert(3, ({ int a[]={1,2,3,}; a[2]; }), "({ int a[]={1,2,3,}; a[2]; })");
    assert(1, ({ struct {int a,b,c;} x={1,2,3,}; x.a; }), "({ struct {int a,b,c;} x={1,2,3,}; x.a; })");
    assert(2, ({ enum {x,y,z,}; z; }), "({ enum {x,y,z,}; z; })");

    ext1 = 5;
    assert(5, ext1, "ext1");

    ext2 = &ext1;
    assert(5, *ext2, "*ext2");

    assert(1, _Alignof(char), "_Alignof(char)");
    assert(2, _Alignof(short), "_Alignof(short)");
    assert(4, _Alignof(int), "_Alignof(int)");
    assert(8, _Alignof(long), "_Alignof(long)");
    assert(8, _Alignof(long long), "_Alignof(long long)");
    assert(1, _Alignof(char[3]), "_Alignof(char[3])");
    assert(4, _Alignof(int[3]), "_Alignof(int[3])");
    assert(1, _Alignof(struct {char a; char b;}[2]), "_Alignof(struct {char a; char b;}[2])");
    assert(8, _Alignof(struct {char a; long b;}[2]), "_Alignof(struct {char a; long b;}[2])");

    assert(1, ({ _Alignas(char) char x, y; &y-&x; }), "({ _Alignas(char) char x, y; &y-&x; })");
    assert(8, ({ _Alignas(long) char x, y; &y-&x; }), "({ _Alignas(long) char x, y; &y-&x; })");
    assert(32, ({ _Alignas(32) char x, y; &y-&x; }), "({ _Alignas(32) char x, y; &y-&x; })");
    assert(32, ({ _Alignas(32) int *x, *y; ((char *)&y)-((char *)&x); }), "({ _Alignas(32) int *x, *y; ((char *)&y)-((char *)&x); })");
    assert(16, ({ struct { _Alignas(16) char x, y; } a; &a.y-&a.x; }), "({ struct { _Alignas(16) char x, y; } a; &a.y-&a.x; })");
    assert(8, ({ struct T { _Alignas(8) char a; }; _Alignof(struct T); }), "({ struct T { _Alignas(8) char a; }; _Alignof(struct T); })");
    assert(0, (long)(char *)&g_aligned1 % 512, "(long)(char *)&g_aligned1 % 512");
    assert(0, (long)(char *)&g_aligned2 % 512, "(long)(char *)&g_aligned2 % 512");

    assert(2, counter(), "counter()");
    assert(4, counter(), "counter()");
    assert(6, counter(), "counter()");

    assert(1, (int){1}, "(int){1}");
    assert(2, ((int[]){0,1,2})[2], "((int[]){0,1,2})[2]");
    assert('a', ((struct {char a; int b;}){'a', 3}).a, "((struct {char a; int b;}){'a', 3}).a");
    assert(3, ({ int x=3; (int){x}; }), "({ int x=3; (int){x}; })");
    (int){3} = 5;

    assert(1, tree->val, "tree->val");
    assert(2, tree->lhs->val, "tree->lhs->val");
    assert(3, tree->lhs->lhs->val, "tree->lhs->lhs->val");
    assert(4, tree->lhs->rhs->val, "tree->lhs->rhs->val");

    ret_none();

    assert(3, ext3, "ext3");

    assert(2, ({ int i=6; i&=3; i; }), "({ int i=6; i&=3; i; }");
    assert(7, ({ int i=6; i|=3; i; }), "({ int i=6; i|=3; i; }");
    assert(10, ({ int i=15; i^=5; i; }), "({ int i=15; i^=5; i; }");

    assert(7, ({ int i=0; int j=0; do { j++; } while (i++ < 6); j; }), "({ int i=0; int j=0; do { j++; } while (i++ < 6); j; })");
    assert(4, ({ int i=0; int j=0; int k=0; do { if (++j > 3) break; continue; k++; } while (1); j; }), "({ int i=0; int j=0; int k=0; do { if (++j > 3) break; continue; k++; } while (1); j; })");

    assert(1, true_fn(), "true_fn()");
    assert(0, false_fn(), "false_fn()");

    assert(6, add_all1(1,2,3,0), "add_all1(1,2,3,0)");
    assert(5, add_all1(1,2,3,-1,0), "add_all1(1,2,3,-1,0)");

    assert(6, add_all3(1,2,3,0), "add_all3(1,2,3,0)");
    assert(5, add_all3(1,2,3,-1,0), "add_all3(1,2,3,-1,0)");

    assert(0, ({char buf[100]; sprintf(buf, "%d %d %s", 1, 2, "foo"); strcmp("1 2 foo", buf); }), "({char buf[100]; sprintf(buf, \"%d %d %s\", 1, 2, \"foo\"); strcmp(\"1 2 foo\", buf); })");
    assert(0, ({char buf[100]; fmt(buf, "%d %d %s", 1, 2, "foo"); strcmp("1 2 foo", buf); }), "({char buf[100]; fmt(buf, \"%d %d %s\", 1, 2, \"foo\"); strcmp(\"1 2 foo\", buf); })");

    assert(1, sizeof(char), "sizeof(char)");
    assert(1, sizeof(signed char), "sizeof(signed char)");
    assert(1, sizeof(signed char signed), "sizeof(signed char signed)");
    assert(1, sizeof(unsigned char), "sizeof(unsigned char)");
    assert(1, sizeof(unsigned char unsigned), "sizeof(unsigned char unsigned)");

    assert(2, sizeof(short), "sizeof(short)");
    assert(2, sizeof(int short), "sizeof(int short)");
    assert(2, sizeof(short int), "sizeof(short int)");
    assert(2, sizeof(signed short), "sizeof(signed short)");
    assert(2, sizeof(int short signed), "sizeof(int short signed)");
    assert(2, sizeof(unsigned short), "sizeof(unsigned short)");
    assert(2, sizeof(int short unsigned), "sizeof(int short unsigned)");

    assert(4, sizeof(int), "sizeof(int)");
    assert(4, sizeof(signed int), "sizeof(signed int)");
    assert(4, sizeof(signed), "sizeof(signed)");
    assert(4, sizeof(signed signed), "sizeof(signed signed)");
    assert(4, sizeof(unsigned int), "sizeof(unsigned int)");
    assert(4, sizeof(unsigned), "sizeof(unsigned)");
    assert(4, sizeof(unsigned unsigned), "sizeof(unsigned unsigned)");

    assert(8, sizeof(long), "sizeof(long)");
    assert(8, sizeof(signed long), "sizeof(signed long)");
    assert(8, sizeof(signed long int), "sizeof(signed long int)");
    assert(8, sizeof(unsigned long), "sizeof(unsigned long)");
    assert(8, sizeof(unsigned long int), "sizeof(unsigned long int)");

    assert(8, sizeof(long long), "sizeof(long long)");
    assert(8, sizeof(signed long long), "sizeof(signed long long)");
    assert(8, sizeof(signed long long int), "sizeof(signed long long int)");
    assert(8, sizeof(unsigned long long), "sizeof(unsigned long long)");
    assert(8, sizeof(unsigned long long int), "sizeof(unsigned long long int)");

    assert(1, sizeof((char)1), "sizeof((char)1)");
    assert(2, sizeof((short)1), "sizeof((short)1)");
    assert(4, sizeof((int)1), "sizeof((int)1)");
    assert(8, sizeof((long)1), "sizeof((long)1)");
    assert(-1, (char)255, "(char)255");
    assert(-1, (signed char)255, "(signed char)255");
    assert(255, (unsigned char)255, "(unsigned char)255");
    assert(-1, (short)65535, "(short)65535");
    assert(65535, (unsigned short)65535, "(unsigned short)65535");
    assert(-1, (int)0xffffffff, "(int)0xffffffff");
    assert(0xffffffff, (unsigned)0xffffffff, "(unsigned)0xffffffff");

    assert(4, sizeof((char)1 + (char)1), "sizeof((char)1 + (char)1)");
    assert(4, sizeof((short)1 + (short)1), "sizeof((short)1 + (short)1)");
    assert(4, sizeof(1?2:3), "sizeof(1?2:3)");
    assert(4, sizeof(1?(short)2:(char)3), "sizeof(1?(short)2:(char)3)");
    assert(8, sizeof(1?(long)2:(char)3), "sizeof(1?(long)2:(char)3)");

    assert(1, -1<1, "-1<1");
    assert(0, -1<(unsigned)1, "-1<(unsigned)1");
    assert(254, (char)127+(char)127, "(char)127+(char)127");
    assert(65534, (short)32767+(short)32767, "(short)32767+(short)32767");
    assert(-1, -1>>1, "-1>>1");
    assert(-1, (unsigned long)-1, "(unsigned long)-1");
    assert(2147483647, ((unsigned)-1)>>1, "((unsigned)-1)>>1");
    assert(-50, (-100)/2, "(-100)/2");
    assert(2147483598, ((unsigned)-100)/2, "((unsigned)-100)/2");
    assert(9223372036854775758, ((unsigned long)-100)/2, "((unsigned long)-100)/2");
    assert(0, ((long)-1)/(unsigned)100, "((long)-1)/(unsigned)100");
    assert(-2, (-100)%7, "(-100)%7");
    assert(2, ((unsigned)-100)%7, "((unsigned)-100)%7");
    assert(6, ((unsigned long)-100)%9, "((unsigned long)-100)%9");

    assert(65535, (int)(unsigned short)65535, "(int)(unsigned short)65535");
    assert(65535, ({ unsigned short x = 65535; x; }), "({ unsigned short x = 65535; x; })");
    assert(65535, ({ unsigned short x = 65535; (int)x; }), "({ unsigned short x = 65535; (int)x; })");

    assert(-1, ({ typedef short T; T x = 65535; (int)x; }), "({ typedef short T; T x = 65535; (int)x; })");
    assert(65535, ({ typedef unsigned short T; T x = 65535; (int)x; }), "({ typedef unsigned short T; T x = 65535; (int)x; })");

    assert(4, sizeof(0), "sizeof(0)");
    assert(8, sizeof(0L), "sizeof(0L)");
    assert(8, sizeof(0LU), "sizeof(0LU)");
    assert(8, sizeof(0UL), "sizeof(0UL)");
    assert(8, sizeof(0LL), "sizeof(0LL)");
    assert(8, sizeof(0LLU), "sizeof(0LLU)");
    assert(8, sizeof(0Ull), "sizeof(0Ull)");
    assert(8, sizeof(0l), "sizeof(0l)");
    assert(8, sizeof(0ll), "sizeof(0ll)");
    assert(8, sizeof(0x0L), "sizeof(0x0L)");
    assert(8, sizeof(0b0L), "sizeof(0b0L)");
    assert(4, sizeof(2147483647), "sizeof(2147483647)");
    assert(8, sizeof(2147483648), "sizeof(2147483648)");
    assert(-1, 0xffffffffffffffff, "0xffffffffffffffff");
    assert(8, sizeof(0xffffffffffffffff), "sizeof(0xffffffffffffffff)");
    assert(4, sizeof(4294967295U), "sizeof(4294967295U)");
    assert(8, sizeof(4294967296U), "sizeof(4294967296U)");

    assert(3, -1U>>30, "-1U>>30");
    assert(3, -1Ul>>62, "-1Ul>>62");
    assert(3, -1ull>>62, "-1ull>>62");

    assert(1, 0xffffffffffffffffl>>63, "0xffffffffffffffffl>>63");
    assert(1, 0xffffffffffffffffll>>63, "0xffffffffffffffffll>>63");

    assert(-1, 18446744073709551615, "18446744073709551615");
    assert(8, sizeof(18446744073709551615), "sizeof(18446744073709551615)");
    assert(-1, 18446744073709551615>>63, "18446744073709551615>>63");

    assert(-1, 0xffffffffffffffff, "0xffffffffffffffff");
    assert(8, sizeof(0xffffffffffffffff), "sizeof(0xffffffffffffffff)");
    assert(1, 0xffffffffffffffff>>63, "0xffffffffffffffff>>63");

    assert(-1, 01777777777777777777777, "01777777777777777777777");
    assert(8, sizeof(01777777777777777777777), "sizeof(01777777777777777777777)");
    assert(1, 01777777777777777777777>>63, "01777777777777777777777>>63");

    assert(-1, 0b1111111111111111111111111111111111111111111111111111111111111111, "0b1111111111111111111111111111111111111111111111111111111111111111");
    assert(8, sizeof(0b1111111111111111111111111111111111111111111111111111111111111111), "sizeof(0b1111111111111111111111111111111111111111111111111111111111111111)");
    assert(1, 0b1111111111111111111111111111111111111111111111111111111111111111>>63, "0b1111111111111111111111111111111111111111111111111111111111111111>>63");

    assert(8, sizeof(2147483648), "sizeof(2147483648)");
    assert(4, sizeof(2147483647), "sizeof(2147483647)");

    assert(8, sizeof(0x1ffffffff), "sizeof(0x1ffffffff)");
    assert(4, sizeof(0xffffffff), "sizeof(0xffffffff)");
    assert(1, 0xffffffff>>31, "0xffffffff>>31");

    assert(8, sizeof(040000000000), "sizeof(040000000000)");
    assert(4, sizeof(037777777777), "sizeof(037777777777)");
    assert(1, 037777777777>>31, "037777777777>>31");

    assert(8, sizeof(0b111111111111111111111111111111111), "sizeof(0b111111111111111111111111111111111)");
    assert(4, sizeof(0b11111111111111111111111111111111), "sizeof(0b11111111111111111111111111111111)");
    assert(1, 0b11111111111111111111111111111111>>31, "0b11111111111111111111111111111111>>31");

    assert(-1, 1 << 31 >> 31, "1 << 31 >> 31");
    assert(-1, 01 << 31 >> 31, "01 << 31 >> 31");
    assert(-1, 0x1 << 31 >> 31, "0x1 << 31 >> 31");
    assert(-1, 0b1 << 31 >> 31, "0b1 << 31 >> 31");

    assert(1, sizeof(char) << 31 >> 31, "sizeof(char) << 31 >> 31");
    assert(1, sizeof(char) << 63 >> 63, "sizeof(char) << 63 >> 63");
    assert(1, _Alignof(char) << 31 >> 31, "_Alignof(char) << 31 >> 31");
    assert(1, _Alignof(char) << 63 >> 63, "_Alignof(char) << 63 >> 63");

    assert(4, ({ char x[(-1>>31)+5]; sizeof(x); }), "({ char x[(-1>>31)+5]; sizeof(x); })");
    assert(255, ({ char x[(unsigned char)0xffffffff]; sizeof(x); }), "({ char x[(unsigned char)0xffffffff]; sizeof(x); })");
    assert(0x800f, ({ char x[(unsigned short)0xffff800f]; sizeof(x); }), "({ char x[(unsigned short)0xffff800f]; sizeof(x); })");
    assert(1, ({ char x[(unsigned int)0xfffffffffff>>31]; sizeof(x); }), "({ char x[(unsigned int)0xfffffffffff>>31]; sizeof(x); })");
    assert(1, ({ char x[(long)-1/((long)1<<62)+1]; sizeof(x); }), "({ char x[(long)-1/((long)1<<62)+1]; sizeof(x); })");
    assert(4, ({ char x[(unsigned long)-1/((long)1<<62)+1]; sizeof(x); }), "({ char x[(unsigned long)-1/((long)1<<62)+1]; sizeof(x); })");
    assert(1, ({ char x[(unsigned)1<-1]; sizeof(x); }), "({ char x[(unsigned)1<-1]; sizeof(x); })");
    assert(1, ({ char x[(unsigned)1<=-1]; sizeof(x); }), "({ char x[(unsigned)1<=-1]; sizeof(x); })");

    { const x; }
    { int const x; }
    { const int x; }
    { const int const const x; }
    assert(5, ({ const x = 5; x; }), "({ const x = 5; x; })");
    assert(8, ({ const x = 8; int *const y=&x; *y; }), "({ const x = 8; int *const y=&x; *y; })");
    assert(6, ({ const x = 6; *(const * const)&x; }), "({ const x = 6; *(const * const)&x; })");

    { volatile x; }
    { int volatile x; }
    { volatile int x; }
    { volatile int volatile volatile x; }
    { int volatile * volatile volatile x; }
    { int ** restrict const volatile *x; }

    0.0;
    1.0;
    3e+8;
    0x10.1p0;
    .1E4f;

    assert(4, sizeof(8f), "sizeof(8f)");
    assert(4, sizeof(0.3F), "sizeof(0.3F)");
    assert(8, sizeof(0.), "sizeof(0.)");
    assert(8, sizeof(.0), "sizeof(.0)");
    assert(8, sizeof(5.l), "sizeof(5.l)");
    assert(8, sizeof(2.0L), "sizeof(2.0L)");

    assert(1, 2e3==2e3, "2e3==2e3");
    assert(0, 2e3==2e5, "2e3==2e5");
    assert(1, 2.0==2, "2.0==2");
    assert(0, 5.1<5, "5.1<5");
    assert(0, 5.0<5, "5.0<5");
    assert(1, 4.9<5, "4.9<5");
    assert(0, 5.1<=5, "5.1<=5");
    assert(1, 5.0<=5, "5.0<=5");
    assert(1, 4.9<=5, "4.9<=5");

    assert(1, 2e3f==2e3f, "2e3f==2e3f");
    assert(0, 2e3f==2e5, "2e3f==2e5");
    assert(1, 2.0f==2, "2.0f==2");
    assert(0, 5.1f<5, "5.1f<5");
    assert(0, 5.0f<5, "5.0f<5");
    assert(1, 4.9f<5, "5.9f<5");
    assert(0, 5.1f<=5, "5.1f<=5");
    assert(1, 5.0f<=5, "5.0f<=5");
    assert(1, 4.9f<=5, "5.9f<=5");

    assert(1, ({ float x=2e3f; x==2e3; }), "({ float x=2e3f; x==2e3; })");
    assert(0, ({ float x=2e3f; x==2e5; }), "({ float x=2e3f; x==2e5; })");
    assert(0, ({ float x=5.1f; x<5; }), "({ float x=5.1f; x<5; })");
    assert(0, ({ float x=5.0f; x<5; }), "({ float x=5.0f; x<5; })");
    assert(1, ({ float x=4.9f; x<5; }), "({ float x=4.9f; x<5; })");
    assert(0, ({ float x=5.1f; x<=5; }), "({ float x=5.1f; x<=5; })");
    assert(1, ({ float x=5.0f; x<=5; }), "({ float x=5.0f; x<=5; })");
    assert(1, ({ float x=4.9f; x<=5; }), "({ float x=4.9f; x<=5; })");

    assert(1, ({ double x=2e3f; x==2e3; }), "({ double x=2e3f; x==2e3; })");
    assert(0, ({ double x=2e3f; x==2e5; }), "({ double x=2e3f; x==2e5; })");
    assert(0, ({ double x=5.1f; x<5; }), "({ double x=5.1f x<5; })");
    assert(0, ({ double x=5.0f; x<5; }), "({ double x=5.0f; x<5; })");
    assert(1, ({ double x=4.9f; x<5; }), "({ double x=4.9f; x<5; })");
    assert(0, ({ double x=5.1f; x<=5; }), "({ double x=5.1f; x<=5; })");
    assert(1, ({ double x=5.0f; x<=5; }), "({ double x=5.0f; x<=5; })");
    assert(1, ({ double x=4.9f; x<=5; }), "({ double x=4.9f; x<=5; })");

    assert(0, (_Bool)0.0, "(_Bool)0.0");
    assert(1, (_Bool)0.1, "(_Bool)0.1");
    assert(3, (char)3.0, "(char)3.0");
    assert(1000, (short)1000.3, "(short)1000.3");
    assert(3, (int)3.99, "(int)3.99");
    assert(2000000000000000, (long)2e15, "(long)2e15");
    assert(3, (float)3.5, "(float)3.5");
    assert(5, (double)(float)5.5, "(double)(float)5.5");
    assert(3, (float)3, "(float)3");
    assert(3, (double)3, "(double)3");
    assert(3, (float)3L, "(float)3L");
    assert(3, (double)3L, "(double)3L");

    assert(4, sizeof(float), "sizeof(float)");
    assert(8, sizeof(double), "sizeof(double)");
    assert(8, sizeof(long double), "sizeof(long double)");

    assert(4, sizeof(1f+2), "sizeof(1f+2)");
    assert(8, sizeof(1.0+2), "sizeof(1.0+2)");
    assert(4, sizeof(1f-2), "sizeof(1f-2)");
    assert(8, sizeof(1.0-2), "sizeof(1.0-2)");
    assert(4, sizeof(1f*2), "sizeof(1f*2)");
    assert(8, sizeof(1.0*2), "sizeof(1.0*2)");
    assert(4, sizeof(1f/2), "sizeof(1f/2)");
    assert(8, sizeof(1.0/2), "sizeof(1.0/2)");

    assert(6, 2.3+3.8, "2.3+3.8");
    assert(-1, 2.3-3.8, "2.3-3.8");
    assert(-3, -3.8, "-3.8");
    assert(13, 3.3*4, "3.3*4");
    assert(2, 5.0/2, "5.0/2");

    assert(6, 2.3f+3.8f, "2.3f+3.8f");
    assert(6, 2.3f+3.8, "2.3f+3.8");
    assert(-1, 2.3f-3.8, "2.3f-3.8");
    assert(-3, -3.8f, "-3.8f");
    assert(13, 3.3f*4, "3.3f*4");
    assert(2, 5.0f/2, "5.0f/2");

    assert(0, !3., "!3.");
    assert(1, !0., "!0.");
    assert(0, 0.0 && 0.0, "0.0 && 0.0");
    assert(0, 0.0 && 0.1, "0.0 && 0.1");
    assert(0, 0.3 && 0.0, "0.3 && 0.0");
    assert(1, 0.3 && 0.5, "0.3 && 0.5");
    assert(0, 0.0 || 0.0, "0.0 || 0.0");
    assert(1, 0.0 || 0.1, "0.0 || 0.1");
    assert(1, 0.3 || 0.0, "0.3 || 0.0");
    assert(1, 0.3 || 0.5, "0.3 || 0.5");
    assert(5, 0.0 ? 3 : 5, "0.0 ? 3 : 5");
    assert(3, 1.2 ? 3 : 5, "1.2 ? 3 : 5");
    assert(5, ({ int x; if (0.0) x=3; else x=5; x; }), "({ int x; if (0.0) x=3; else x=5; x; })");
    assert(3, ({ int x; if (0.3) x=3; else x=5; x; }), "({ int x; if (0.3) x=3; else x=5; x; })");
    assert(10, ({ double i=10.0; int j=0; for (; i; i--, j++); j; }), "({ double i=10.0; int j=0; for (; i; i--, j++); j; })");
    assert(10, ({ double i=10.0; int j=0; do j++; while(--i); j; }), "({ double i=10.0; int j=0; do j++; while(--i); j; })");

    assert(2, ret_float(2.8), "ret_float(2.8)");
    assert(6, add_float(2.3, 3.8), "add_float(2.3, 3.8)");
    assert(6, add_double(2.3, 3.8), "add_double(2.3, 3.8)");

    assert(7, add_float3(2.5, 2.5, 2.5), "add_float3(2.5, 2.5, 2.5)");
    assert(7, add_double3(2.5, 2.5, 2.5), "add_double3(2.5, 2.5, 2.5)");

    assert(0, ({ char buf[100]; sprintf(buf, "%.1f", (float)3.5); strcmp(buf, "3.5"); }), "({ char buf[100]; sprintf(buf, \"%.1f\", (float)3.5); strcmp(buf, \"3.5\"); })");

    assert(0, ({ char buf[100]; fmt(buf, "%.1f", (float)3.5); strcmp(buf, "3.5"); }), "({ char buf[100]; fmt(buf, \"%.1f\", (float)3.5); strcmp(buf, \"3.5\"); })");

    assert(1, g40==1.5, "g40==1.5");
    assert(1, g41==11, "g41==11");

    assert(5, (add2)(2,3), "(add2)(2,3)");
    assert(5, (&add2)(2,3), "(&add2)(2,3)");
    assert(7, ({ int (*fn)(int,int) = add2; fn(2,5); }), "({ int (*fn)(int,int) = add2; fn(2,5); })");
    assert(3, fnptr()(), "fnptr()()");

#

    assert(5, include1, "include1");
    assert(7, include2, "include2");

    printf("OK\n");
    return 0;
}
