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
        if (ty->is_unsigned)
            println("  uxtb w%s, w%s", r + 1, r + 1);
        else
            println("  sxtb %s, w%s", r, r + 1);
    } else if (ty->size == 2) {
        if (ty->is_unsigned)
            println("  uxth w%s, w%s", r + 1, r + 1);
        else
            println("  sxth %s, w%s", r, r + 1);
    } else if (ty->size == 4) {
        if (ty->is_unsigned)
            println("  mov w%s, w%s", r + 1, r + 1);
        else
            println("  sxtw %s, w%s", r, r + 1);
    }
}

static void gen_expr(Node *node);

static void gen_addr(Node *node) {
    switch (node->kind) {
    case ND_VAR:
        if (node->var->is_local) {
            mov_imm(reg(top), node->var->offset);
            println("  sub %s, x29, %s", reg(top), reg(top));
        } else {
            println("  adrp %s, %s", reg(top), node->var->name);
            println("  add %s, %s, :lo12:%s", reg(top), reg(top),
                    node->var->name);
        }
        top++;
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        return;
    }

    error_tok(node->tok, "not an lvalue");
}

static void load(Type *ty) {
    if (ty->kind == TY_ARRAY || ty->kind == TY_FUNC)
        return;
    if (ty->kind == TY_STRUCT)
        error("struct loads are not yet supported on AArch64");
    if (is_flonum(ty))
        error("floating-point loads are not yet supported on AArch64");

    char *r = reg(top - 1);
    if (ty->size == 1) {
        if (ty->is_unsigned)
            println("  ldrb w%s, [%s]", r + 1, r);
        else
            println("  ldrsb %s, [%s]", r, r);
    } else if (ty->size == 2) {
        if (ty->is_unsigned)
            println("  ldrh w%s, [%s]", r + 1, r);
        else
            println("  ldrsh %s, [%s]", r, r);
    } else if (ty->size == 4) {
        if (ty->is_unsigned)
            println("  ldr w%s, [%s]", r + 1, r);
        else
            println("  ldrsw %s, [%s]", r, r);
    }
    else if (ty->size == 8)
        println("  ldr %s, [%s]", r, r);
    else
        error("unsupported load size on AArch64: %d", ty->size);
}

static void store(Type *ty) {
    if (ty->kind == TY_STRUCT)
        error("struct stores are not yet supported on AArch64");
    if (is_flonum(ty))
        error("floating-point stores are not yet supported on AArch64");

    char *value = reg(top - 2);
    char *addr = reg(top - 1);
    if (ty->size == 1)
        println("  strb w%s, [%s]", value + 1, addr);
    else if (ty->size == 2)
        println("  strh w%s, [%s]", value + 1, addr);
    else if (ty->size == 4)
        println("  str w%s, [%s]", value + 1, addr);
    else if (ty->size == 8)
        println("  str %s, [%s]", value, addr);
    else
        error("unsupported store size on AArch64: %d", ty->size);
    top--;
}

static void load_arg(Var *arg, int idx) {
    if (is_flonum(arg->ty))
        error_tok(arg->tok, "floating-point arguments are not yet supported on AArch64");
    if (arg->ty->kind == TY_STRUCT)
        error_tok(arg->tok, "struct arguments are not yet supported on AArch64");

    mov_imm("x17", arg->offset);
    println("  sub x17, x29, x17");
    if (arg->ty->size == 1) {
        if (arg->ty->is_unsigned)
            println("  ldrb w%d, [x17]", idx);
        else
            println("  ldrsb x%d, [x17]", idx);
    } else if (arg->ty->size == 2) {
        if (arg->ty->is_unsigned)
            println("  ldrh w%d, [x17]", idx);
        else
            println("  ldrsh x%d, [x17]", idx);
    } else if (arg->ty->size == 4) {
        if (arg->ty->is_unsigned)
            println("  ldr w%d, [x17]", idx);
        else
            println("  ldrsw x%d, [x17]", idx);
    } else if (arg->ty->size == 8) {
        println("  ldr x%d, [x17]", idx);
    } else {
        error_tok(arg->tok, "unsupported argument size on AArch64: %d",
                  arg->ty->size);
    }
}

static void store_param(Var *param, int idx) {
    if (is_flonum(param->ty))
        error_tok(param->tok, "floating-point parameters are not yet supported on AArch64");
    if (param->ty->kind == TY_STRUCT)
        error_tok(param->tok, "struct parameters are not yet supported on AArch64");

    mov_imm("x17", param->offset);
    println("  sub x17, x29, x17");
    if (param->ty->size == 1)
        println("  strb w%d, [x17]", idx);
    else if (param->ty->size == 2)
        println("  strh w%d, [x17]", idx);
    else if (param->ty->size == 4)
        println("  str w%d, [x17]", idx);
    else if (param->ty->size == 8)
        println("  str x%d, [x17]", idx);
    else
        error_tok(param->tok, "unsupported parameter size on AArch64: %d",
                  param->ty->size);
}

static void gen_expr(Node *node) {
    println("  .loc %d %d", node->tok->file_no, node->tok->line_no);

    switch (node->kind) {
    case ND_NULL_EXPR:
        mov_imm(reg(top++), 0);
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
    case ND_COMMA:
        gen_expr(node->lhs);
        top--;
        gen_expr(node->rhs);
        return;
    case ND_FUNCALL:
        if (node->nargs > 8)
            error_tok(node->tok, "more than 8 arguments are not yet supported on AArch64");
        if (is_flonum(node->ty) || node->ty->kind == TY_STRUCT)
            error_tok(node->tok, "this function return type is not yet supported on AArch64");

        gen_expr(node->lhs);
        println("  mov x16, %s", reg(--top));
        for (int i = 0; i < node->nargs; i++)
            load_arg(node->args[i], i);
        println("  blr x16");
        println("  mov %s, x0", reg(top));
        normalize_int(node->ty, reg(top));
        top++;
        return;
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

        int nparams = 0;
        for (Var *param = fn->params; param; param = param->next)
            nparams++;
        if (nparams > 8)
            error_tok(fn->var->tok,
                      "more than 8 parameters are not yet supported on AArch64");
        int param_idx = nparams;
        for (Var *param = fn->params; param; param = param->next)
            store_param(param, --param_idx);

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
