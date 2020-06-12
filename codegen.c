#include "711cc.h"

static int top;
static char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "r9b"};
static char *argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static Function *current_fn;

static int count(void) {
    static int i = 1;
    return i++;
}

static char *reg(int idx) {
    static char *r[] = {"%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static void gen_expr(Node *node);
static void gen_stmt(Node *node);

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node *node) {
    switch (node->kind) {
    case ND_VAR:
        if (node->var->is_local)
            printf("  lea -%d(%%rbp), %s\n", node->var->offset, reg(top++));
        else
            printf("  mov $%s, %s\n", node->var->name, reg(top++));
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        return;
    }

    error_tok(node->tok, "not an lvalue");
}

// Load a value from where the stack top is pointing to.
static void load(Type *ty) {
    if (ty->kind == TY_ARRAY) {
        // If it is an array, do nothing because in general we can't load
        // an entire array to a register. As a result, the result of an
        // evaluation of an array becomes not the array itself but the
        // address of the array. In other words, this is where "array is
        // automatically converted to a pointer to the first element of
        // the array in C" occurs.
        return;
    }
    
    char *r = reg(top - 1);
    if (ty->size == 1)
        printf("  movsbq (%s), %s\n", r, r);
    else
        printf("  mov (%s), %s\n", r, r);
}

static void store(Type *ty) {
    char *rd = reg(top - 1);
    char *rs = reg(top - 2);

    if (ty->size == 1)
        printf("  mov %sb, (%s)\n", rs, rd);
    else
        printf("  mov %s, (%s)\n", rs, rd);
    top--;
}

// Generate code for a given node.
static void gen_expr(Node *node) {
    switch (node->kind) {
    case ND_NUM:
        printf("  mov $%lu, %s\n", node->val, reg(top++));
        return;
    case ND_VAR:
        gen_addr(node);
        load(node->ty);
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        load(node->ty);
        return;
    case ND_ADDR:
        gen_addr(node->lhs);
        return;
    case ND_ASSIGN:
        if (node->ty->kind == TY_ARRAY)
            error_tok(node->tok, "not an lvalue");
 
        gen_expr(node->rhs);
        gen_addr(node->lhs);
        store(node->ty);
        return;
    case ND_STMT_EXPR:
        for (Node *n = node->body; n; n = n->next)
            gen_stmt(n);
        top++;
        return;
    case ND_FUNCALL: {
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next) {
            gen_expr(arg);
            nargs++;
        }

        for (int i = 1; i <= nargs; i++)
            printf("  mov %s, %s\n", reg(--top), argreg64[nargs - i]);

        printf("  push %%r10\n");
        printf("  push %%r11\n");
        printf("  mov $0, %%rax\n");
        printf("  call %s\n", node->funcname);
        printf("  pop %%r11\n");
        printf("  pop %%r10\n");
        printf("  mov %%rax, %s\n", reg(top++));
        return;
    }
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
        error_tok(node->tok, "invalid expression");
    }

}

static void gen_stmt(Node *node) {
    switch (node->kind) {
    case ND_IF: {
        int c = count();
        if (node->els) {
            gen_expr(node->cond);
            printf("  cmp $0, %s\n", reg(--top));
            printf("  je .L.else.%d\n", c);
            gen_stmt(node->then);
            printf("  jmp .L.end.%d\n", c);
            printf(".L.else.%d:\n", c);
            gen_stmt(node->els);
            printf(".L.end.%d:\n", c);
        } else {
            gen_expr(node->cond);
            printf("  cmp $0, %s\n", reg(--top));
            printf("  je .L.end.%d\n", c);
            gen_stmt(node->then);
            printf(".L.end.%d:\n", c);
        }
        return;
    }
    case ND_FOR: {
        int c = count();
        if (node->init)
            gen_stmt(node->init);
        printf(".L.begin.%d:\n", c);
        if (node->cond) {
            gen_expr(node->cond);
            printf("  cmp $0, %s\n", reg(--top));
            printf("  je .L.end.%d\n", c);
        }
        gen_stmt(node->then);
        if (node->inc) {
            gen_expr(node->inc);
            top--;
        }
        printf("  jmp .L.begin.%d\n", c);
        printf(".L.end.%d:\n", c);
        return;
    }
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next)
            gen_stmt(n);
        return;
    case ND_RETURN:
        gen_expr(node->lhs);
        printf("  mov %s, %%rax\n", reg(--top));
        printf("  jmp .L.return.%s\n", current_fn->name);
        return;
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        top--;
        return;
    default:
        error_tok(node->tok, "invalid statement");
    }
}

static void emit_data(Program *prog) {
    printf(".data\n");

    for (Var *var = prog->globals; var; var = var->next) {
        printf("%s:\n", var->name);

        if (!var->init_data) {
            printf("  .zero %d\n", var->ty->size);
            continue;
        }

        for (int i = 0; i < var->ty->size; i++)
            printf("  .byte %d\n", var->init_data[i]);
    }
}

static void emit_text(Program *prog) {
    printf(".text\n");

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        printf(".globl %s\n", fn->name);
        printf("%s:\n", fn->name);
        current_fn = fn;
    
        // Prologue. %r12-15 are callee-saved retisters.
        printf("  push %%rbp\n");
        printf("  mov %%rsp, %%rbp\n");
        printf("  sub $%d, %%rsp\n", fn->stack_size);
        printf("  mov %%r12, -8(%%rbp)\n");
        printf("  mov %%r13, -16(%%rbp)\n");
        printf("  mov %%r14, -24(%%rbp)\n");
        printf("  mov %%r15, -32(%%rbp)\n");

        // Save arguments to the stack
        int i = 0;
        for (Var *var = fn->params; var; var = var->next)
            i++;
        for (Var *var = fn->params; var; var = var->next)
            if (var->ty->size == 1)
                printf("  mov %s, -%d(%%rbp)\n", argreg8[--i], var->offset);
            else
                printf("  mov %s, -%d(%%rbp)\n", argreg64[--i], var->offset);
    
        // Emit code
        gen_stmt(fn->body);
        assert(top == 0);
    
        // Epilogue
        printf(".L.return.%s:\n", fn->name);
        printf("  mov -8(%%rbp), %%r12\n");
        printf("  mov -16(%%rbp), %%r13\n");
        printf("  mov -24(%%rbp), %%r14\n");
        printf("  mov -32(%%rbp), %%r15\n");
        printf("  mov %%rbp, %%rsp\n");
        printf("  pop %%rbp\n");
        printf("  ret\n");
    }
}

void codegen(Program *prog) {
    emit_data(prog);
    emit_text(prog);
}
