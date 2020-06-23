#include "711cc.h"

static int top;
static int brknum;
static int contnum;
static char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};
static char *argreg16[] = {"%di", "%si", "%dx", "%cx", "%r8w", "%r9w"};
static char *argreg32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
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

static char *xreg(Type *ty, int idx) {
    if (ty->base || size_of(ty) == 8)
        return reg(idx);

    static char *r[] = {"%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d"};
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
    case ND_COMMA:
        gen_expr(node->lhs);
        top--;
        gen_addr(node->rhs);
        return;
    case ND_MEMBER:
        gen_addr(node->lhs);
        println("  add $%d, %s", node->member->offset, reg(top - 1));
        return;
    }

    error_tok(node->tok, "not an lvalue");
}

// Load a value from where the stack top is pointing to.
static void load(Type *ty) {
    if (ty->kind == TY_ARRAY || ty->kind == TY_STRUCT) {
        // If it is an array, do nothing because in general we can't load
        // an entire array to a register. As a result, the result of an
        // evaluation of an array becomes not the array itself but the
        // address of the array. In other words, this is where "array is
        // automatically converted to a pointer to the first element of
        // the array in C" occurs.
        return;
    }
    
    char *rs = reg(top - 1);
    char *rd = xreg(ty, top - 1);
    int sz = size_of(ty);

    // When we load a char or a short value to a register, we always
    // extend them to the size of int, so we can assume the lower half of
    // a register always contains a valid value. The upper half of a
    // register for char, short and int may contain garbage. When we load
    // a long value to a register, it simply occupies the entire register.
    if (sz == 1)
        println("  movsbl (%s), %s", rs, rd);
    else if (sz == 2)
        println("  movswl (%s), %s", rs, rd);
    else
        println("  mov (%s), %s", rs, rd);
}

static void store(Type *ty) {
    char *rd = reg(top - 1);
    char *rs = reg(top - 2);
    int sz = size_of(ty);

    if (ty->kind == TY_STRUCT) {
        for (int i = 0; i < sz; i++) {
            println("  mov %d(%s), %%al", i, rs);
            println("  mov %%al, %d(%s)", i, rd);
        }
    } else if (sz == 1) {
        println("  mov %sb, (%s)", rs, rd);
    } else if (sz == 2) {
        println("  mov %sw, (%s)", rs, rd);
    } else if (sz == 4) {
        println("  mov %sd, (%s)", rs, rd);
    } else {
        println("  mov %s, (%s)", rs, rd);
    }

    top--;
}

static void cast(Type *from, Type *to) {
    if (to->kind == TY_VOID)
        return;

    char *r = reg(top - 1);

    if (to->kind == TY_BOOL) {
        println("  cmp $0, %s", r);
        println("  setne %sb", r);
        println("  movzx %sb, %s", r, r);
    }

    if (size_of(to) == 1)
        println("  movsx %sb, %s", r, r);
    else if (size_of(to) == 2)
        println("  movsx %sw, %s", r, r);
    else if (size_of(to) == 4)
        println("  mov %sd, %sd", r, r);
    else if (is_integer(from) && size_of(from) < 8)
        println("  movsx %sd, %s", r, r);
}

static void divmod(Node *node, char *rs, char *rd, char *r64, char *r32) {
    if (size_of(node->ty) == 8) {
        println("  mov %s, %%rax", rd);
        println("  cqo");
        println("  idiv %s", rs);
        println("  mov %s, %s", r64, rd);
    } else {
        println("  mov %s, %%eax", rd);
        println("  cdq");
        println("  idiv %s", rs);
        println("  mov %s, %s", r32, rd);
    }
}

// Generate code for a given node.
static void gen_expr(Node *node) {
    println("  .loc 1 %d", node->tok->line_no);

    switch (node->kind) {
    case ND_NUM:
        println("  mov $%lu, %s", node->val, reg(top++));
        return;
    case ND_VAR:
    case ND_MEMBER:
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
    case ND_NULL_EXPR:
        top++;
        return;
    case ND_COMMA:
        gen_expr(node->lhs);
        top--;
        gen_expr(node->rhs);
        return;
    case ND_CAST:
        gen_expr(node->lhs);
        cast(node->lhs->ty, node->ty);
        return;
    case ND_NOT:
        gen_expr(node->lhs);
        println("  cmp $0, %s", reg(top - 1));
        println("  sete %sb", reg(top - 1));
        println("  movzx %sb, %s", reg(top - 1), reg(top - 1));
        return;
    case ND_BITNOT:
        gen_expr(node->lhs);
        println("  not %s", reg(top - 1));
        return;
    case ND_LOGAND: {
        int c = count();
        gen_expr(node->lhs);
        println("  cmp $0, %s", reg(--top));
        println("  je .L.false.%d", c);
        gen_expr(node->rhs);
        println("  cmp $0, %s", reg(--top));
        println("  je .L.false.%d", c);
        println("  mov $1, %s", reg(top));
        println("  jmp .L.end.%d", c);
        println(".L.false.%d:", c);
        println("  mov $0, %s", reg(top++));
        println(".L.end.%d:", c);
        return;
    }
    case ND_LOGOR: {
        int c = count();
        gen_expr(node->lhs);
        println("  cmp $0, %s", reg(--top));
        println("  jne .L.true.%d", c);
        gen_expr(node->rhs);
        println("  cmp $0, %s", reg(--top));
        println("  jne .L.true.%d", c);
        println("  mov $0, %s", reg(top));
        println("  jmp .L.end.%d", c);
        println(".L.true.%d:", c);
        println("  mov $1, %s", reg(top++));
        println(".L.end.%d:", c);
        return;
    }
    case ND_FUNCALL: {
        // Save caller-saved registers
        println("  push %%r10");
        println("  push %%r11");

        // Load arguments from the stack
        for (int i = 0; i < node->nargs; i++) {
            Var *arg = node->args[i];
            int sz = size_of(arg->ty);

            if (sz == 1)
                println("  movsbl -%d(%%rbp), %s", arg->offset, argreg32[i]);
            else if (sz == 2)
                println("  movswl -%d(%%rbp), %s", arg->offset, argreg32[i]);
            else if (sz == 4)
                println("  mov -%d(%%rbp), %s", arg->offset, argreg32[i]);
            else
                println("  mov -%d(%%rbp), %s", arg->offset, argreg64[i]);
        }

        println("  mov $0, %%rax");
        println("  call %s", node->funcname);
        println("  pop %%r11");
        println("  pop %%r10");
        println("  mov %%rax, %s", reg(top++));
        return;
    }
    }

    // Binary expressions
    gen_expr(node->lhs);
    gen_expr(node->rhs);

    char *rd = xreg(node->lhs->ty, top - 2);
    char *rs = xreg(node->lhs->ty, top - 1);
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
        divmod(node, rs, rd, "%rax", "%eax");
        return;
    case ND_MOD:
        divmod(node, rs, rd, "%rdx", "%edx");
        return;
    case ND_BITAND:
        println("  and %s, %s", rs, rd);
        return;
    case ND_BITOR:
        println("  or %s, %s", rs, rd);
        return;
    case ND_BITXOR:
        println("  xor %s, %s", rs, rd);
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
        int brk = brknum;
        int cont = contnum;
        brknum = contnum = c;

        if (node->init)
            gen_stmt(node->init);
        println(".L.begin.%d:", c);
        if (node->cond) {
            gen_expr(node->cond);
            println("  cmp $0, %s", reg(--top));
            println("  je .L.break.%d", c);
        }
        gen_stmt(node->then);
        println(".L.continue.%d:", c);
        if (node->inc) {
            gen_expr(node->inc);
            top--;
        }
        println("  jmp .L.begin.%d", c);
        println(".L.break.%d:", c);

        brknum = brk;
        contnum = cont;
        return;
    }
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next)
            gen_stmt(n);
        return;
    case ND_BREAK:
        if (brknum == 0)
            error_tok(node->tok, "stray break");
        println("  jmp .L.break.%d", brknum);
        return;
    case ND_CONTINUE:
        if (contnum == 0)
            error_tok(node->tok, "stray continue");
        println("  jmp .L.continue.%d", contnum);
        return;
    case ND_GOTO:
        println("  jmp .L.label.%s.%s", current_fn->name, node->label_name);
        return;
    case ND_LABEL:
        println(".L.label.%s.%s:", current_fn->name, node->label_name);
        gen_stmt(node->lhs);
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
            println("  .zero %d", size_of(var->ty));
            continue;
        }

        for (int i = 0; i < size_of(var->ty); i++)
            println("  .byte %d", var->init_data[i]);
    }
}

static char *get_argreg(int sz, int idx) {
    if (sz == 1)
        return argreg8[idx];
    if (sz == 2)
        return argreg16[idx];
    if (sz == 4)
        return argreg32[idx];
    assert(sz == 8);
    return argreg64[idx];
}

static void emit_text(Program *prog) {
    println("  .text");

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        if (!fn->is_static)
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
        for (Var *var = fn->params; var; var = var->next) {
            char *r = get_argreg(var->ty->size, --i);
            println("  mov %s, -%d(%%rbp)", r, var->offset);
        }
    
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
