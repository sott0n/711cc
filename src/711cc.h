#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct Type Type;
typedef struct Member Member;
typedef struct Relocation Relocation;

//
// tokenize.c
//

// Token
typedef enum {
    TK_RESERVED,    // Keywords or punctuators
    TK_IDENT,       // Identifiers
    TK_STR,         // String literals
    TK_NUM,         // Numeric literals
    TK_EOF,         // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
    TokenKind kind;     // Token kind
    Token *next;        // Next token
    long val;           // If kind is TK_NUM, its value
    Type *ty;           // Used if TK_NUM
    char *loc;          // Token location
    int len;            // Token length

    char *contents;     // String literal contents including terminating '\0'
    int cont_len;       // String literal length

    int line_no;        // Line number
};

void error(char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
void warn_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
bool consume(Token **rest, Token *tok, char *str);
Token *tokenize_file(char *filename);

//
// parse.c
//

// Variable
typedef struct Var Var;
struct Var {
    Var *next;
    char *name;     // Variable name
    Type *ty;       // Type
    Token *tok;     // representative token
    bool is_local;  // local or global
    int align;      // alignment

    // Local variable 
    int offset;     
    
    // Global variable
    bool is_static;
    char *init_data;
    Relocation *rel;
};

// Global variable can be initialized either by a constant expression
// or a pointer to another global variable. This struct represents the
// latter.
typedef struct Relocation Relocation;
struct Relocation {
    Relocation *next;
    int offset;
    char *label;
    long addend;
};

// AST node
typedef enum {
    ND_ADD,         // +
    ND_SUB,         // -
    ND_MUL,         // *
    ND_DIV,         // /
    ND_MOD,         // %
    ND_BITAND,      // &
    ND_BITOR,       // |
    ND_BITXOR,      // ^
    ND_SHL,         // <<
    ND_SHR,         // >>
    ND_EQ,          // ==
    ND_NE,          // !=
    ND_LT,          // <
    ND_LE,          // <=
    ND_ASSIGN,      // =
    ND_COND,        // ?:
    ND_COMMA,       // ,
    ND_MEMBER,      // . (struct member access)
    ND_ADDR,        // unary &
    ND_DEREF,       // unary *
    ND_NOT,         // !
    ND_BITNOT,      // ~
    ND_LOGAND,      // &&
    ND_LOGOR,       // ||
    ND_RETURN,      // "return"
    ND_IF,          // "if"
    ND_FOR,         // "for" or "while"
    ND_DO,          // "do"
    ND_SWITCH,      // "switch"
    ND_CASE,        // "case"
    ND_BLOCK,       // { ... }
    ND_BREAK,       // "break"
    ND_CONTINUE,    // "continue"
    ND_GOTO,        // "goto"
    ND_LABEL,       // Labeled statement
    ND_FUNCALL,     // Function call 
    ND_EXPR_STMT,   // Expression statement
    ND_STMT_EXPR,   // Statement expression
    ND_NULL_EXPR,   // Do nothing
    ND_VAR,         // Variable
    ND_NUM,         // Integer
    ND_CAST,        // Type cast
} NodeKind;

// Ast node type
typedef struct Node Node;
struct Node {
    NodeKind kind;  // Node kind
    Node *next;     // Next node
    Type *ty;       // Type, e.g. int or pointer to int
    Token *tok;     // Representative token

    Node *lhs;      // Left-hand node
    Node *rhs;      // Right-hand node

    // "if" or "for" statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // Assignment
    bool is_init;

    // Block or statement expression
    Node *body;

    // Struct member access
    Member *member;

    // Function call
    char *funcname;
    Type *func_ty;
    Var **args;
    int nargs;

    // Goto or labeled statement
    char *label_name;

    // Switch-case
    Node *case_next;
    Node *default_case;
    int case_label;
    int case_end_label;

    // Variable
    Var *var;

    // Numeric literal
    long val;
};

typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    Var *params;
    bool is_static;
    bool is_variadic;

    Node *body;
    Var *locals;
    int stack_size;
};

typedef struct {
    Var *globals;
    Function *fns;
} Program;

Node *new_cast(Node *expr, Type *ty);
Program *parse(Token *tok);

//
// type.c
//

typedef enum {
    TY_VOID,
    TY_BOOL,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_ENUM,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
    TY_STRUCT,
} TypeKind;

struct Type {
    TypeKind kind;
    int size;           // sizeof() value
    int align;          // alignment
    bool is_unsigned;   // unsigned or signed
    bool is_incomplete; // incomplete type
    bool is_const;

    // Pointer-to or array-of type. We intentionally use the same member
    // to represent pointer/array duality in C.
    //
    // In many contexts in which a pointer is expected, we examine this
    // member instead of "kind" member to determine whether a type is a
    // pointer or not. That means in many contexts "array of T" is
    // naturally handled as if it were "pointer to T", as required by
    // the C spec.
    Type *base;

    // Declaration
    Token *name;
    Token *name_pos;

    // Array
    int array_len;

    // Struct
    Member *members;

    // Function type
    Type *return_ty;
    Type *params;
    bool is_variadic;
    Type *next;
};

// Struct member
struct Member {
    Member *next;
    Type *ty;
    Token *tok; // for error message
    Token *name;
    int align;
    int offset;
};

extern Type *ty_void;
extern Type *ty_bool;

extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_int;
extern Type *ty_long;

extern Type *ty_uchar;
extern Type *ty_ushort;
extern Type *ty_uint;
extern Type *ty_ulong;

bool is_integer(Type *ty);
Type *copy_type(Type *ty);
int align_to(int n, int align);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int size);
Type *enum_type(void);
Type *struct_type(void);
int size_of(Type *ty);
Type *copy_type(Type *ty);
void add_type(Node *node);

//
// codegen.c
//

void codegen(Program *prog);

//
// main.c
//
extern FILE *output_file;
