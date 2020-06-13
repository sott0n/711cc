#include "711cc.h"

static int top;
static char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "r9b"};
static char *argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static Function *current_fn;

static void println(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(output_file, fmt, ap);
    fprintf(output_file, "\n");
}

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
            println("  lea -%d(%%rbp), %s", node->var->offset, reg(top++));
        else
            println("  mov $%s, %s", node->var->name, reg(top++));
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
        println("  movsbq (%s), %s", r, r);
    else
        println("  mov (%s), %s", r, r);
}

static void store(Type *ty) {
    char *rd = reg(top - 1);
    char *rs = reg(top - 2);

    if (ty->size == 1)
        println("  mov %sb, (%s)", rs, rd);
    else
        println("  mov %s, (%s)", rs, rd);
    top--;
}

// Generate code for a given node.
static void gen_expr(Node *node) {
    println("  .loc 1 %d", node->tok->line_no);

    switch (node->kind) {
    case ND_NUM:
        println("  mov $%lu, %s", node->val, reg(top++));
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
            println("  mov %s, %s", reg(--top), argreg64[nargs - i]);

        println("  push %%r10");
        println("  push %%r11");
        println("  mov $0, %%rax");
        println("  call %s", node->funcname);
        println("  pop %%r11");
        println("  pop %%r10");
        println("  mov %%rax, %s", reg(top++));
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
        println("  add %s, %s", rs, rd);
        return;
    case ND_SUB:
        println("  sub %s, %s", rs, rd);
        return;
    case ND_MUL:
        println("  imul %s, %s", rs, rd);
        return;
    case ND_DIV:
        println("  mov %s, %%rax", rd);
        println("  cqo");
        println("  idiv %s", rs);
        println("  mov %%rax, %s", rd);
        return;
    case ND_EQ:
        println("  cmp %s, %s", rs, rd);
        println("  sete %%al");
        println("  movzb %%al, %s", rd);
        return;
    case ND_NE:
        println("  cmp %s, %s", rs, rd);
        println("  setne %%al");
        println("  movzb %%al, %s", rd);
        return;
    case ND_LT:
        println("  cmp %s, %s", rs, rd);
        println("  setl %%al");
        println("  movzb %%al, %s", rd);
        return;
    case ND_LE:
        println("  cmp %s, %s", rs, rd);
        println("  setle %%al");
        println("  movzb %%al, %s", rd);
        return;
    default:
        error_tok(node->tok, "invalid expression");
    }

}

static void gen_stmt(Node *node) {
    println("  .loc 1 %d", node->tok->line_no);

    switch (node->kind) {
    case ND_IF: {
        int c = count();
        if (node->els) {
            gen_expr(node->cond);
            println("  cmp $0, %s", reg(--top));
            println("  je .L.else.%d", c);
            gen_stmt(node->then);
            println("  jmp .L.end.%d", c);
            println(".L.else.%d:", c);
            gen_stmt(node->els);
            println(".L.end.%d:", c);
        } else {
            gen_expr(node->cond);
            println("  cmp $0, %s", reg(--top));
            println("  je .L.end.%d", c);
            gen_stmt(node->then);
            println(".L.end.%d:", c);
        }
        return;
    }
    case ND_FOR: {
        int c = count();
        if (node->init)
            gen_stmt(node->init);
        println(".L.begin.%d:", c);
        if (node->cond) {
            gen_expr(node->cond);
            println("  cmp $0, %s", reg(--top));
            println("  je .L.end.%d", c);
        }
        gen_stmt(node->then);
        if (node->inc) {
            gen_expr(node->inc);
            top--;
        }
        println("  jmp .L.begin.%d", c);
        println(".L.end.%d:", c);
        return;
    }
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next)
            gen_stmt(n);
        return;
    case ND_RETURN:
        gen_expr(node->lhs);
        println("  mov %s, %%rax", reg(--top));
        println("  jmp .L.return.%s", current_fn->name);
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
    println("  .data");

    for (Var *var = prog->globals; var; var = var->next) {
        println("%s:", var->name);

        if (!var->init_data) {
            println("  .zero %d", var->ty->size);
            continue;
        }

        for (int i = 0; i < var->ty->size; i++)
            println("  .byte %d", var->init_data[i]);
    }
}

static void emit_text(Program *prog) {
    println("  .text");

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        println("  .globl %s", fn->name);
        println("%s:", fn->name);
        current_fn = fn;
    
        // Prologue. %r12-15 are callee-saved retisters.
        println("  push %%rbp");
        println("  mov %%rsp, %%rbp");
        println("  sub $%d, %%rsp", fn->stack_size);
        println("  mov %%r12, -8(%%rbp)");
        println("  mov %%r13, -16(%%rbp)");
        println("  mov %%r14, -24(%%rbp)");
        println("  mov %%r15, -32(%%rbp)");

        // Save arguments to the stack
        int i = 0;
        for (Var *var = fn->params; var; var = var->next)
            i++;
        for (Var *var = fn->params; var; var = var->next)
            if (var->ty->size == 1)
                println("  mov %s, -%d(%%rbp)", argreg8[--i], var->offset);
            else
                println("  mov %s, -%d(%%rbp)", argreg64[--i], var->offset);
    
        // Emit code
        gen_stmt(fn->body);
        assert(top == 0);
    
        // Epilogue
        println(".L.return.%s:", fn->name);
        println("  mov -8(%%rbp), %%r12");
        println("  mov -16(%%rbp), %%r13");
        println("  mov -24(%%rbp), %%r14");
        println("  mov -32(%%rbp), %%r15");
        println("  mov %%rbp, %%rsp");
        println("  pop %%rbp");
        println("  ret");
    }
}

void codegen(Program *prog) {
    emit_data(prog);
    emit_text(prog);
}
