#include "711cc.h"

static int top;
static Function *current_fn;

static char *reg(int idx) {
    static char *r[] = {
        "x19", "x20", "x21", "x22", "x23",
        "x24", "x25", "x26", "x27", "x28",
    };

    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static void mov_imm(char *rd, unsigned long val) {
    println("  movz %s, #%lu", rd, val & 0xffff);
    for (int shift = 16; shift < 64; shift += 16) {
        unsigned long part = (val >> shift) & 0xffff;
        if (part)
            println("  movk %s, #%lu, lsl #%d", rd, part, shift);
    }
}

static void normalize_int(Type *ty, char *r) {
    if (!is_integer(ty) && ty->kind != TY_PTR)
        return;

    if (ty->kind == TY_BOOL) {
        println("  cmp %s, #0", r);
        println("  cset %s, ne", r);
    } else if (ty->size == 1) {
        println("  %s %s, %s", ty->is_unsigned ? "uxtb" : "sxtb", r, r);
    } else if (ty->size == 2) {
        println("  %s %s, %s", ty->is_unsigned ? "uxth" : "sxth", r, r);
    } else if (ty->size == 4) {
        if (ty->is_unsigned)
            println("  mov w%s, w%s", r + 1, r + 1);
        else
            println("  sxtw %s, w%s", r, r + 1);
    }
}

static void gen_expr(Node *node) {
    println("  .loc %d %d", node->tok->file_no, node->tok->line_no);

    switch (node->kind) {
    case ND_NUM:
        if (is_flonum(node->ty))
            error_tok(node->tok, "floating-point expressions are not yet supported on AArch64");
        mov_imm(reg(top++), node->val);
        return;
    case ND_CAST:
        if (is_flonum(node->lhs->ty) || is_flonum(node->ty))
            error_tok(node->tok, "floating-point casts are not yet supported on AArch64");
        gen_expr(node->lhs);
        normalize_int(node->ty, reg(top - 1));
        return;
    case ND_NOT:
        gen_expr(node->lhs);
        println("  cmp %s, #0", reg(top - 1));
        println("  cset %s, eq", reg(top - 1));
        return;
    case ND_BITNOT:
        gen_expr(node->lhs);
        println("  mvn %s, %s", reg(top - 1), reg(top - 1));
        normalize_int(node->ty, reg(top - 1));
        return;
    default:
        break;
    }

    switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_BITAND:
    case ND_BITOR:
    case ND_BITXOR:
    case ND_SHL:
    case ND_SHR:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
        break;
    default:
        error_tok(node->tok, "expression is not yet supported on AArch64");
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);

    char *lhs = reg(top - 2);
    char *rhs = reg(top - 1);
    top--;

    switch (node->kind) {
    case ND_ADD:
        println("  add %s, %s, %s", lhs, lhs, rhs);
        break;
    case ND_SUB:
        println("  sub %s, %s, %s", lhs, lhs, rhs);
        break;
    case ND_MUL:
        println("  mul %s, %s, %s", lhs, lhs, rhs);
        break;
    case ND_DIV:
        println("  %s %s, %s, %s", node->ty->is_unsigned ? "udiv" : "sdiv",
                lhs, lhs, rhs);
        break;
    case ND_MOD:
        println("  %s x16, %s, %s", node->ty->is_unsigned ? "udiv" : "sdiv",
                lhs, rhs);
        println("  msub %s, x16, %s, %s", lhs, rhs, lhs);
        break;
    case ND_BITAND:
        println("  and %s, %s, %s", lhs, lhs, rhs);
        break;
    case ND_BITOR:
        println("  orr %s, %s, %s", lhs, lhs, rhs);
        break;
    case ND_BITXOR:
        println("  eor %s, %s, %s", lhs, lhs, rhs);
        break;
    case ND_SHL:
        println("  lsl %s, %s, %s", lhs, lhs, rhs);
        break;
    case ND_SHR:
        println("  %s %s, %s, %s", node->lhs->ty->is_unsigned ? "lsr" : "asr",
                lhs, lhs, rhs);
        break;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE: {
        char *cond;
        if (node->kind == ND_EQ)
            cond = "eq";
        else if (node->kind == ND_NE)
            cond = "ne";
        else if (node->kind == ND_LT)
            cond = node->lhs->ty->is_unsigned ? "lo" : "lt";
        else
            cond = node->lhs->ty->is_unsigned ? "ls" : "le";
        println("  cmp %s, %s", lhs, rhs);
        println("  cset %s, %s", lhs, cond);
        return;
    }
    default:
        error_tok(node->tok, "expression is not yet supported on AArch64");
    }

    normalize_int(node->ty, lhs);
}

static void gen_stmt(Node *node) {
    println("  .loc %d %d", node->tok->file_no, node->tok->line_no);

    switch (node->kind) {
    case ND_NULL_EXPR:
        return;
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next)
            gen_stmt(n);
        return;
    case ND_RETURN:
        if (node->lhs) {
            gen_expr(node->lhs);
            println("  mov x0, %s", reg(--top));
        }
        println("  b .L.return.%s", current_fn->name);
        return;
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        top--;
        return;
    default:
        error_tok(node->tok, "statement is not yet supported on AArch64");
    }
}

static void emit_bss(Program *prog) {
    println("  .bss");

    for (Var *var = prog->globals; var; var = var->next) {
        if (var->init_data)
            continue;

        println("  .balign %d", var->align);
        if (!var->is_static)
            println("  .globl %s", var->name);
        println("%s:", var->name);
        println("  .zero %d", var->ty->size);
    }
}

static void emit_data(Program *prog) {
    println("  .data");

    for (Var *var = prog->globals; var; var = var->next) {
        if (!var->init_data)
            continue;

        println("  .balign %d", var->align);
        if (!var->is_static)
            println("  .globl %s", var->name);
        println("%s:", var->name);

        Relocation *rel = var->rel;
        int pos = 0;
        while (pos < var->ty->size) {
            if (rel && rel->offset == pos) {
                println("  .quad %s%+ld", rel->label, rel->addend);
                rel = rel->next;
                pos += 8;
            } else {
                println("  .byte %d", var->init_data[pos++]);
            }
        }
    }
}

static void emit_text(Program *prog) {
    println("  .text");

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        if (fn->params || fn->locals)
            error_tok(fn->var->tok,
                      "parameters and local variables are not yet supported on AArch64");

        println("  .p2align 2");
        if (!fn->is_static)
            println("  .globl %s", fn->name);
        println("  .type %s, %%function", fn->name);
        println("%s:", fn->name);
        current_fn = fn;

        println("  stp x29, x30, [sp, #-16]!");
        println("  mov x29, sp");
        println("  sub sp, sp, #%d", fn->stack_size);
        println("  stp x19, x20, [x29, #-16]");
        println("  stp x21, x22, [x29, #-32]");
        println("  stp x23, x24, [x29, #-48]");
        println("  stp x25, x26, [x29, #-64]");
        println("  stp x27, x28, [x29, #-80]");

        gen_stmt(fn->body);
        assert(top == 0);

        if (!strcmp(fn->name, "main"))
            println("  mov x0, #0");

        println(".L.return.%s:", fn->name);
        println("  ldp x19, x20, [x29, #-16]");
        println("  ldp x21, x22, [x29, #-32]");
        println("  ldp x23, x24, [x29, #-48]");
        println("  ldp x25, x26, [x29, #-64]");
        println("  ldp x27, x28, [x29, #-80]");
        println("  mov sp, x29");
        println("  ldp x29, x30, [sp], #16");
        println("  ret");
        println("  .size %s, .-%s", fn->name, fn->name);
    }
}

void codegen_aarch64(Program *prog) {
    char **paths = get_input_files();
    for (int i = 0; paths[i]; i++)
        println("  .file %d \"%s\"", i + 1, paths[i]);

    emit_bss(prog);
    emit_data(prog);
    emit_text(prog);
}
