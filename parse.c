#include "711cc.h"

static Node *expr(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

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

static Node *new_unary(NodeKind kind, Node *expr) {
    Node *node = new_node(kind);
    node->lhs = expr;
    return node;
}

static Node *new_num(long val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

static long get_number(Token *tok) {
    if (tok->kind != TK_NUM)
        error_tok(tok, "expected a number");
    return tok->val;
}

// stmt = "return" expr ";"
//      | expr-stmt
static Node *stmt(Token **rest, Token *tok) {
    if (equal(tok, "return")) {
        Node *node = new_unary(ND_RETURN, expr(&tok, tok->next));
        *rest = skip(tok, ";");
        return node;
    }

    return expr_stmt(rest, tok);
}

// expr-stmt = expr ";"
static Node *expr_stmt(Token **rest, Token *tok) {
    Node *node = new_unary(ND_EXPR_STMT, expr(&tok, tok));
    *rest = skip(tok, ";");
    return node;
}

// expr = equality
static Node *expr(Token **rest, Token *tok) {
    return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);

    for (;;) {
        if (equal(tok, "==")) {
            Node *rhs = relational(&tok, tok->next);
            node = new_binary(ND_EQ, node, rhs);
            continue;
        }

        if (equal(tok, "!=")) {
            Node *rhs = relational(&tok, tok->next);
            node = new_binary(ND_NE, node, rhs);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);

    for (;;) {
        if (equal(tok, "<")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary(ND_LT, node, rhs);
            continue;
        }

        if (equal(tok, "<=")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary(ND_LE, node, rhs);
            continue;
        }

        if (equal(tok, ">")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary(ND_LT, rhs, node);
            continue;
        }

        if (equal(tok, ">=")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary(ND_LE, rhs, node);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
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

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);

    for (;;) {
        if (equal(tok, "*")) {
            Node *rhs = unary(&tok, tok->next);
            node = new_binary(ND_MUL, node, rhs);
            continue;
        }

        if (equal(tok, "/")) {
            Node *rhs = unary(&tok, tok->next);
            node = new_binary(ND_DIV, node, rhs);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// unary = ("+" | "-") unary
//       | primary
static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+"))
        return unary(rest, tok->next);

    if (equal(tok, "-"))
        return new_binary(ND_SUB, new_num(0), unary(rest, tok->next));
    
    return primary(rest, tok);
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

// program = stmt*
Node *parse(Token *tok) {
    Node head = {};
    Node *cur = &head;

    while (tok->kind != TK_EOF)
        cur = cur->next = stmt(&tok, tok);
    return head.next;
} 
