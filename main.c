#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//

typedef enum {
    TK_RESERVED,    // Keywords or punctuators
    TK_NUM,         // Numeric literals
    TK_EOF,         // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
    TokenKind kind;     // Token kind
    Token *next;       // Next token
    long val;           // If kind is TK_NUM, its value
    char *loc;          // Token location
    int len;            // Token length
};

// Input string
static char *current_input;

// Reports an error and exit
static void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Reports an error location and exit
static void verror_at(char *loc, char *fmt, va_list ap) {
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

static void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

// Consumes the current token if it matches `s`.
static bool equal(Token *tok, char *s) {
    return strlen(s) == tok->len &&
        !strncmp(tok->loc, s, tok->len);
}

// Ensure that the current token is `s`.
static Token *skip(Token *tok, char *s) {
    if (!equal(tok, s))
        error_tok(tok, "expected '%s'", s);
    return tok->next;
}

// Ensure that the current token is TK_NUM.
static long get_number(Token *tok) {
    if (tok->kind != TK_NUM)
        error_tok(tok, "expected a number");
    return tok->val;
}

// Create a new token and add it as the next token of `cur`.
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// Tokenize `current_input` and returns new tokens
static Token *tokenize(void) {
    char *p = current_input;
    Token head = {};
    Token *cur = &head;

    while (*p) {
        // Skip whitespace characters
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Numeric literal
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        // Punctuators
        if (ispunct(*p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        error_at(p, "invalid token");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

//
// Parser
//

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // Integer
} NodeKind;

// Ast node type
typedef struct Node Node;
struct Node {
    NodeKind kind;  // Node kind
    Node *lhs;      // Left-hand node
    Node *rhs;      // Right-hand node
    long val;       // Used if kind == ND_NUM
};

static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_num(long val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

static Node *expr(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

static Node *expr(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);

    for (;;) {
        if (equal(tok, "+")) {
            Node *rhs = mul(&tok, tok->next);
            node = new_binary(ND_ADD, node, rhs);
            continue;
        }

        if (equal(tok, "-")) {
            Node *rhs = mul(&tok, tok->next);
            node = new_binary(ND_SUB, node, rhs);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// mul = primary ("*" primary | "/" primary)*
static Node *mul(Token **rest, Token *tok) {
    Node *node = primary(&tok, tok);

    for (;;) {
        if (equal(tok, "*")) {
            Node *rhs = primary(&tok, tok->next);
            node = new_binary(ND_MUL, node, rhs);
            continue;
        }

        if (equal(tok, "/")) {
            Node *rhs = primary(&tok, tok->next);
            node = new_binary(ND_DIV, node, rhs);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    Node *node = new_num(get_number(tok));
    *rest = tok->next;
    return node;
}

//
// Code generator
//

static char *reg(int idx) {
    static char *r[] = {"%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static int top;

static void gen_expr(Node *node) {
    if (node->kind == ND_NUM) {
        printf("  mov $%lu, %s\n", node->val, reg(top++));
        return;
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);

    char *rd = reg(top - 2);
    char *rs = reg(top - 1);
    top--;

    switch (node->kind) {
    case ND_ADD:
        printf("  add %s, %s\n", rs, rd);
        return;
    case ND_SUB:
        printf("  sub %s, %s\n", rs, rd);
        return;
    case ND_MUL:
        printf("  imul %s, %s\n", rs, rd);
        return;
    case ND_DIV:
        printf("  mov %s, %%rax\n", rd);
        printf("  cqo\n");
        printf("  idiv %s\n", rs);
        printf("  mov %%rax, %s\n", rd);
        return;
    default:
        error("invalid expression");
    }

}

int main(int argc, char **argv) {
    if (argc != 2)
        error("%s: invalid number of arguments\n", argv[0]);

    // Tokenize and parse
    current_input = argv[1];
    Token *tok = tokenize();
    Node *node = expr(&tok, tok);

    if (tok->kind != TK_EOF)
        error_tok(tok, "extra token");

    printf(".globl main\n");
    printf("main:\n");

    // Save callee-saved registers
    printf("  push %%r12\n");
    printf("  push %%r13\n");
    printf("  push %%r14\n");
    printf("  push %%r15\n");

    // Traverse the AST to emit assembly
    gen_expr(node);

    // Set the result of the expression to RAX so that
    // the result becomes a return value of this function.
    printf("  mov %s, %%rax\n", reg(top - 1));

    printf("  pop %%r15\n");
    printf("  pop %%r14\n");
    printf("  pop %%r13\n");
    printf("  pop %%r12\n");
    printf("  ret\n");
    return 0;
}
