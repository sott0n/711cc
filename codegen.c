#include "711cc.h"

static int top;

static char *reg(int idx) {
    static char *r[] = {"%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

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
    case ND_EQ:
        printf("  cmp %s, %s\n", rs, rd);
        printf("  sete %%al\n");
        printf("  movzb %%al, %s\n", rd);
        return;
    case ND_NE:
        printf("  cmp %s, %s\n", rs, rd);
        printf("  setne %%al\n");
        printf("  movzb %%al, %s\n", rd);
        return;
    case ND_LT:
        printf("  cmp %s, %s\n", rs, rd);
        printf("  setl %%al\n");
        printf("  movzb %%al, %s\n", rd);
        return;
    case ND_LE:
        printf("  cmp %s, %s\n", rs, rd);
        printf("  setle %%al\n");
        printf("  movzb %%al, %s\n", rd);
        return;
    default:
        error("invalid expression");
    }

}

static void gen_stmt(Node *node) {
    switch (node->kind) {
    case ND_RETURN:
        gen_expr(node->lhs);
        printf("  mov %s, %%rax\n", reg(--top));
        printf("  jmp .L.return\n");
        return;
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        top--;
        return;
    default:
        error("invalid statement");
    }
}

void codegen(Node *node) {
    printf(".globl main\n");
    printf("main:\n");

    // Save callee-saved registers
    printf("  push %%r12\n");
    printf("  push %%r13\n");
    printf("  push %%r14\n");
    printf("  push %%r15\n");

    for (Node *n = node; n; n = n->next) {
        gen_stmt(n);
        assert(top == 0);
    }

    printf(".L.return:\n");
    printf("  pop %%r15\n");
    printf("  pop %%r14\n");
    printf("  pop %%r13\n");
    printf("  pop %%r12\n");
    printf("  ret\n");
}
