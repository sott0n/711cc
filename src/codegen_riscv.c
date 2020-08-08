#include "711cc.h"

static int top;
static int brknum;
static int contnum;
static char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};
static char *argreg16[] = {"%di", "%si", "%dx", "%cx", "%r8w", "%r9w"};
static char *argreg32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static char *argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static Function *current_fn;

static int count(void) {
    static int i = 1;
    return i++;
}

static char *reg(int idx) {
    static char *r[] = {"t1", "t2", "t3", "t4", "t5", "t6"};
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static char *xreg(Type *ty, int idx) {
    if (ty->base || ty->size == 8)
        return reg(idx);

    static char *r[] = {"t1", "t2", "t3", "t4", "t5", "t6"};
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static char *freg(int idx) {
    static char *r[] = {"%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13"};
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
        if (node->var->is_local) {
            // A local variable resides on the stack and has a fixed offset
            // from the base pointer.
            println("  addi %s, s0, -%d", reg(top++), node->var->offset);
            return;
        }

        // Here we compute an absolute address of a given global variable.
        // There are three different ways to compute a global variable below.
        //
        // 1. If -fno-pic is given, the resulting ELF module (i.e. an executable
        //    or a .so file) doesn't have to be position-independent, meaning
        //    that we can assume that the resulting code and data will be loaded
        //    at a fixed memory location below 4 GiB. In this case, a 4-byte
        //    absolute address can be computed at link-time and directly
        //    embedded to a mov instruction.
        //
        // 2. If -fno-pic isn't given, the resulting ELF module may be loaded
        //    anywhere in the 64-bit address space. We don't know the load
        //    address at link-time. We have two different ways to compute an
        //    address in this case:
        //
        // 2-1. A file-scope global variable: Because an ELF module is loaded to
        //      memory as a unit, the relative offset between code and data in
        //      the same ELF module is fixed whenever the module is loaded. A
        //      file-scope global variable always resides in the same ELF object
        //      as a use site, so we can use the RIP-relative addressing to
        //      refer a variable. `foo(%rip)` refers address %RIP+addend where
        //      addend is the offset between a use site and the location of
        //      variable foo. The addend is computed by the linker at link-time.
        //
        // 2-2. A non-file-scope global variable: that variable may reside in a
        //      different ELF module whose address is not known until run-time.
        //      We know nothing about the location of the variable at link-time.
        //      For those variable, each ELF module has a table of absolute
        //      address of global variables. The table is called "GOT" (Global
        //      Offset Tabel) and is filled by the loader. For example, if we
        //      have a global variable foo which may not exist in the same ELF
        //      module, we have a table entry for foo in a GOT, and at runtime
        //      the table entry has a 8-byte absolute address of foo.
        //
        //      Since each ELF module has a GOT, and a relative address to a GOT
        //      entry within the same ELF module doesn't change whenever the
        //      module is loaded, we can use the RIP-relative memory access to
        //      load a 8-byte value from GOT.
        //
        //      Not all variables need a GOT entry. By appending "@GOT" or
        //      "@GOTPCREL" to a variable name, you can tell the linker you need
        //      a GOT entry for that variable. "foo@GOTPCREL(%RIP)" refers a GOT
        //      entry of variable foo at runtime.
        if (!opt_fpic) {
            // Load a 32-bit fixed address to a register.
            println("  mov $%s, %s", node->var->name, reg(top++));
        } else if (node->var->is_static) {
            // Set %RIP+addend to a register.
            println("  lea %s(%%rip), %s", node->var->name, reg(top++));
        } else {
            // Load a 64-bit address value from memory and set it to a register.
            println("  mov %s@GOTPCREL(%%rip), %s", node->var->name, reg(top++));
        }
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
    if (ty->kind == TY_ARRAY || ty->kind == TY_STRUCT || ty->kind == TY_FUNC) {
        // If it is an array, do nothing because in general we can't load
        // an entire array to a register. As a result, the result of an
        // evaluation of an array becomes not the array itself but the
        // address of the array. In other words, this is where "array is
        // automatically converted to a pointer to the first element of
        // the array in C" occurs.
        return;
    }

    if (ty->kind == TY_FLOAT) {
        println("  movss (%s), %s", reg(top - 1), freg(top - 1));
        return;
    }

    if (ty->kind == TY_DOUBLE) {
        println("  movsd (%s), %s", reg(top - 1), freg(top - 1));
        return;
    }
    
    char *rs = reg(top - 1);
    char *rd = xreg(ty, top - 1);
    char *insn = ty->is_unsigned ? "movz" : "movs";

    // When we load a char or a short value to a register, we always
    // extend them to the size of int, so we can assume the lower half of
    // a register always contains a valid value. The upper half of a
    // register for char, short and int may contain garbage. When we load
    // a long value to a register, it simply occupies the entire register.
    if (ty->size == 1)
        println("  %sbl (%s), %s", insn, rs, rd);
    else if (ty->size == 2)
        println("  %swl (%s), %s", insn, rs, rd);
    else if (ty->size == 4)
        println("  lw %s, 0(%s)", rd, rs);
    else {
        println("  ld %s, 0(%s)", rd, rs);
    }
}

static void store(Type *ty) {
    char *rd = reg(top - 1);
    char *rs = reg(top - 2);

    if (ty->kind == TY_STRUCT) {
        for (int i = 0; i < ty->size; i++) {
            println("  mov %d(%s), %%al", i, rs);
            println("  mov %%al, %d(%s)", i, rd);
        }
    } else if (ty->kind == TY_FLOAT) {
        println("  movss %s, (%s)", freg(top - 2), rd);
    } else if (ty->kind == TY_DOUBLE) {
        println("  movsd %s, (%s)", freg(top - 2), rd);
    } else if (ty->size == 1) {
        println("  mov %sb, (%s)", rs, rd);
    } else if (ty->size == 2) {
        println("  mov %sw, (%s)", rs, rd);
    } else if (ty->size == 4) {
        println("  sw %s, 0(%s)", rs, rd);
    } else {
        println("  sd %s, 0(%s)", rs, rd);
    }

    top--;
}

static void cmp_zero(Type *ty) {
    if (ty->kind == TY_FLOAT) {
        println("  xorps %%xmm0, %%xmm0");
        println("  ucomiss %%xmm0, %s", freg(--top));
    } else if (ty->kind == TY_DOUBLE) {
        println("  xorpd %%xmm0, %%xmm0");
        println("  ucomisd %%xmm0, %s", freg(--top));
    } else {
        println("  cmp $0, %s", reg(--top));
    }
}

// Convert uint64 to double.
static void convert_ulong_double(char *r, char *fr) {
    // This conversion is little tricky because x86 doesn't have an
    // instruction to convert uint64 to double. All we have is cvtsi2sd
    // which takes a signed 64-bit integer. Here is the strategy:
    //
    // 1. If the "sign" bit of a given uint64 value is 0, then we can
    //    simply use cvtsi2sd.
    //
    // 2. Otherwise, We halve a uint64 value first to clear the most
    //    significant bit, convert it to double using cvtsi2sd and then
    //    double the result.
    //
    //    This is a lossy conversion because double's fraction part (52
    //    bits long) can't represent all 64-bit integers. We need to
    //    keep the least significant bit to prevent a rounding error.
    int c = count();
    println("  cmp $0, %s", r);
    println("  jl .L.cast.%d", c);
    println("  cvtsi2sd %s, %s", r, fr);
    println("  jmp .L.cast.end.%d", c);
    println(".L.cast.%d:", c);
    println("  mov %s, %%rax", r);
    println("  and $1, %%rax");
    println("  shr %s", r);
    println("  or %%rax, %s", r);
    println("  cvtsi2sd %s, %s", r, fr);
    println("  addsd %s, %s", fr, fr);
    println(".L.cast.end.%d:", c);
}

static void cast(Type *from, Type *to) {
    if (to->kind == TY_VOID)
        return;

    char *r = reg(top - 1);
    char *fr = freg(top - 1);

    if (to->kind == TY_BOOL) {
        cmp_zero(from);
        println("  setne %sb", reg(top));
        println("  movzx %sb, %s", reg(top), reg(top));
        top++;
        return;
    }

    if (from->kind == TY_FLOAT) {
        if (to->kind == TY_FLOAT)
            return;

        if (to->kind == TY_DOUBLE)
            println("  cvtss2sd %s, %s", fr, fr);
        else
            println("  cvttss2si %s, %s", fr, r);
        return;
    }

    if (from->kind == TY_DOUBLE) {
        if (to->kind == TY_DOUBLE)
            return;

        if (to->kind == TY_FLOAT)
            println("  cvtsd2ss %s, %s", fr, fr);
        else
            println("  cvttsd2si %s, %s", fr, r);
        return;
    }

    if (to->kind == TY_FLOAT) {
        println("  cvtsi2ss %s, %s", r, fr);
        return;
    }

    if (to->kind == TY_DOUBLE) {
        if (from->size == 8 && from->is_unsigned)
            convert_ulong_double(r, fr);
        else
            println("  cvtsi2sd %s, %s", r, fr);
        return;
    }

    char *insn = to->is_unsigned ? "movzx" : "movsx";

    if (to->size == 1) {
        println("  %s %sb, %s", insn, r, r);
    } else if (to->size == 2) {
        println("  %s %sw, %s", insn, r, r);
    } else if (to->size == 4) {
        println("  mv %s, %s", r, r);
    } else if (is_integer(from) && from->size < 8 && !from->is_unsigned) {
        println("  mv %s, %s", r, r);
    }
}

static void divmod(Node *node, char *rs, char *rd, char *r64) {
    if (node->ty->is_unsigned) {
        println("  divu %s, %s, %s", rd, rd, rs);
    } else {
        println("  div %s, %s, %s", rd, rd, rs);
    }
}

static void builtin_va_start(Node *node) {
    int gp = 0;
    int fp = 0;

    for (Var *var = current_fn->params; var; var = var->next) {
        if (is_flonum(var->ty))
            fp++;
        else
            gp++;
    }

    println("  mov -%d(%%rbp), %%rax", node->args[0]->offset);
    println("  movl $%d, (%%rax)", gp * 8);
    println("  movl $%d, 4(%%rax)", 48 + fp * 8);;
    println("  mov %%rbp, 16(%%rax)");
    println("  subq $128, 16(%%rax)");
    top++;
}

// Load a local variable at RSP+offset to a xmm register.
static void load_fp_arg(Type *ty, int offset, int r) {
    if (ty->kind == TY_FLOAT)
        println("  movss -%d(%%rbp), %%xmm%d", offset, r);
    else
        println("  movsd -%d(%%rbp), %%xmm%d", offset, r);
}

// Load a local variable at RSP+offset to argreg[r].
static void load_gp_arg(Type *ty, int offset, int r) {
    char *insn = ty->is_unsigned ? "movz" : "movs";

    if (ty->size == 1)
        println("  %sbl -%d(%%rbp), %s", insn, offset, argreg32[r]);
    else if (ty->size == 2)
        println("  %swl -%d(%%rbp), %s", insn, offset, argreg32[r]);
    else if (ty->size == 4)
        println("  mov -%d(%%rbp), %s", offset, argreg32[r]);
    else
        println("  mov -%d(%%rbp), %s", offset, argreg64[r]);
}

// Pushs a local variable at RSP+offset to the stack.
static void push_arg(Type *ty, int offset) {
    if (is_flonum(ty)) {
        if (ty->kind == TY_FLOAT)
            println("  mov -%d(%%rbp), %%eax", offset);
        else
            println("  mov -%d(%%rbp), %%rax", offset);
    } else {
        char *insn = ty->is_unsigned ? "movz" : "movs";
        if (ty->size == 1)
            println("  %sbl -%d(%%rbp), %%eax", insn, offset);
        else if (ty->size == 2)
            println("  %swl -%d(%%rbp), %%eax", insn, offset);
        else if (ty->size == 4)
            println("  mov -%d(%%rbp), %%eax", offset);
        else
            println("  mov -%d(%%rbp), %%rax", offset);
    }

    println("  push %%rax");
}

// Load function call arguments. Arguments are already evaluated and
// stored to the stack as local variables. What we need to do in this
// function is to load them to registers or push them to the stack as
// specified by the x86-64 psABI. Here is what the spec says:
//
// - Up to 6 arguments of integral type are passed using RDI, RSI,
//   RDX, RCX, R8 and R9.
//
// - Up to 8 arguments of floating-point type are passed using XMM0 to
//   XMM7.
//
// - If all registers of an appropriate type are already used, push an
//   argument to the stack in the right-to-left order.
//
// - Each argument passed on the stack takes 8 bytes, and the end of
//   the argument area must be aligned to a 16 byte boundary.
//
// - If a function is variadic, set the number of floating-point type
//   arguments to RSP.
static int load_args(Node *node) {
    int gp = 0;
    int fp = 0;
    int stack_size = 0;
    bool *pass_stack = calloc(node->nargs, sizeof(bool));

    // Load as many arguments as possible to the registers.
    for (int i = 0; i < node->nargs; i++) {
        Var *arg = node->args[i];

        if (is_flonum(arg->ty)) {
            if (fp < 8) {
                load_fp_arg(arg->ty, arg->offset, fp++);
                continue;
            }
        } else {
            if (gp < 6) {
                load_gp_arg(arg->ty, arg->offset, gp++);
                continue;
            }
        }

        pass_stack[i] = true;
        stack_size += 8;
    }

    // If we have arguments passed on the stack, push them to the stack.
    if (stack_size) {
        if (stack_size % 16) {
            println("  sub $8, %%rsp");
            stack_size += 8;
        }

        for (int i = node->nargs - 1; i >= 0; i--) {
            if (!pass_stack[i])
                continue;
            Var *arg = node->args[i];
            push_arg(arg->ty, arg->offset);
        }
    }

    // Set the number of floating-point arguments to RSP. Technically,
    // we don't have to do this if a function isn't variadic, but we do
    // this unconditionally for the sake of simplicity.
    //
    // The background of this ABI requirement: At the beginning of a
    // variadic function, there exists code to save all parameter-passing
    // registers to the stack so that va_arg can retrieve them one by one
    // later. But in most cases, a variadic function doesn't take any
    // floating-point argument, or even if it does, it takes only a few.
    // So, saving all XMM0 to XMM7 registers is in most cases wasteful.
    // By passing the actual number of floating-point arguments, the
    // prologue code can save that cost.
    int n = 0;
    for (int i = 0; i < node->nargs; i++)
        if (is_flonum(node->args[i]->ty))
            n++;
    println("  mov $%d, %%rax", n);

    return stack_size;
}

// Generate code for a given node.
static void gen_expr(Node *node) {
    println("  .loc %d %d", node->tok->file_no, node->tok->line_no);

    switch (node->kind) {
    case ND_NUM:
        if (node->ty->kind == TY_FLOAT) {
            float val = node->fval;
            println("  mov $%u, %%eax", *(int *)&val);
            println("  movd %%eax, %s", freg(top++));
        } else if (node->ty->kind == TY_DOUBLE) {
            println("  movabs $%lu, %%rax", *(long *)&node->fval);
            println("  movq %%rax, %s", freg(top++));
        } else if (node->ty->kind == TY_LONG) {
            println("  movabs $%lu, %s", node->val, reg(top++));
        } else {
            println("  li %s, %lu", reg(top++), node->val);
        }
        return;
    case ND_VAR:
        gen_addr(node);
        load(node->ty);
        return;
    case ND_MEMBER: {
        gen_addr(node);
        load(node->ty);

        Member *mem = node->member;
        if (mem->is_bitfield) {
            println("  shl $%d, %s", 64 - mem->bit_width - mem->bit_offset, reg(top - 1));
            if (mem->ty->is_unsigned)
                println("  shr $%d, %s", 64 - mem->bit_width, reg(top - 1));
            else
                println("  sar $%d, %s", 64 - mem->bit_width, reg(top - 1));
        }
        return;
    }
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

        if (node->lhs->kind == ND_MEMBER && node->lhs->member->is_bitfield) {
            // If the lhs is a bitfield, we need to read a value from memory
            // and merge it with a new value.
            Member *mem = node->lhs->member;
            println("  mov %s, %s", reg(top - 1), reg(top));
            top++;
            load(mem->ty);

            println("  and $%ld, %s", (1L << mem->bit_width) - 1, reg(top - 3));
            println("  shl $%d, %s", mem->bit_offset, reg(top - 3));

            long mask = ((1L << mem->bit_width) - 1) << mem->bit_offset;
            println("  movabs $%ld, %%rax", ~mask);
            println("  and %%rax, %s", reg(top - 1));
            println("  or %s, %s", reg(top - 1), reg(top - 3));
            top--;
        }

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
    case ND_COND: {
        int c = count();
        gen_expr(node->cond);
        //cmp_zero(node->cond->ty);
        println("  beqz %s, .L.else.%d", reg(--top), c);
        gen_expr(node->then);
        top--;
        println("  j .L.end.%d", c);
        println(".L.else.%d:", c);
        gen_expr(node->els);
        println(".L.end.%d:", c);
        return;
    }
    case ND_NOT:
        gen_expr(node->lhs);
        cmp_zero(node->lhs->ty);
        println("  sete %sb", reg(top));
        println("  movzx %sb, %s", reg(top), reg(top));
        top++;
        return;
    case ND_BITNOT:
        gen_expr(node->lhs);
        println("  not %s", reg(top - 1));
        return;
    case ND_LOGAND: {
        int c = count();
        gen_expr(node->lhs);
        cmp_zero(node->lhs->ty);
        println("  je .L.false.%d", c);
        gen_expr(node->rhs);
        cmp_zero(node->rhs->ty);
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
        cmp_zero(node->lhs->ty);
        println("  jne .L.true.%d", c);
        gen_expr(node->rhs);
        cmp_zero(node->rhs->ty);
        println("  jne .L.true.%d", c);
        println("  mov $0, %s", reg(top));
        println("  jmp .L.end.%d", c);
        println(".L.true.%d:", c);
        println("  mov $1, %s", reg(top++));
        println(".L.end.%d:", c);
        return;
    }
    case ND_FUNCALL: {
        if (node->lhs->kind == ND_VAR &&
                !strcmp(node->lhs->var->name, "__builtin_va_start")) {
            builtin_va_start(node);
            return;
        }

        // Save caller-saved registers
        println("  sub sp, sp, 64");
        println("  mv ra, 0(sp)");
        println("  mv t0, 8(sp)");
        println("  mv t1, 16(sp)");
        println("  mv t2, 24(sp)");
        println("  mv t3, 32(sp)");
        println("  mv t4, 40(sp)");
        println("  mv t5, 48(sp)");
        println("  mv t6, 56(sp)");
        println("  mv a0, 64(sp)");
        println("  mv a1, 72(sp)");
        println("  mv a2, 80(sp)");
        println("  mv a3, 88(sp)");
        println("  mv a4, 96(sp)");
        println("  mv a5, 104(sp)");
        println("  mv a6, 112(sp)");
        println("  mv a7, 120(sp)");
        println("  mv ft0, 128(sp)");
        println("  mv ft1, 136(sp)");
        println("  mv ft2, 144(sp)");
        println("  mv ft3, 152(sp)");
        println("  mv ft4, 160(sp)");
        println("  mv ft5, 168(sp)");
        println("  mv ft6, 176(sp)");
        println("  mv ft7, 184(sp)");
        println("  mv ft8, 184(sp)");
        println("  mv ft9, 184(sp)");
        println("  mv ft10, 184(sp)");
        println("  mv ft11, 184(sp)");
        println("  mv fa0, 192(sp)");
        println("  mv fa1, 200(sp)");
        println("  mv fa2, 208(sp)");
        println("  mv fa3, 216(sp)");
        println("  mv fa4, 232(sp)");
        println("  mv fa5, 240(sp)");
        println("  mv fa6, 248(sp)");
        println("  mv fa7, 256(sp)");

        gen_expr(node->lhs);
        int memarg_size = load_args(node);

        // Call a function
        println("  call *%s", reg(--top));

        if (memarg_size)
            println("  sub $%d, %%rsp", memarg_size);

        // Restore caller-saved registers
        println("  mov (%%rsp), %%r10");
        println("  mov 8(%%rsp), %%r11");
        println("  movsd 16(%%rsp), %%xmm8");
        println("  movsd 24(%%rsp), %%xmm9");
        println("  movsd 32(%%rsp), %%xmm10");
        println("  movsd 40(%%rsp), %%xmm11");
        println("  movsd 48(%%rsp), %%xmm12");
        println("  movsd 56(%%rsp), %%xmm13");
        println("  add $64, %%rsp");

        if (node->ty->kind == TY_FLOAT)
            println("  movss %%xmm0, %s", freg(top++));
        else if (node->ty->kind == TY_DOUBLE)
            println("  movsd %%xmm0, %s", freg(top++));
        else
            println("  mov %%rax, %s", reg(top++));
        return;
    }
    }

    // Binary expressions
    gen_expr(node->lhs);
    gen_expr(node->rhs);

    char *rd = xreg(node->lhs->ty, top - 2);
    char *rs = xreg(node->lhs->ty, top - 1);
    char *fd = freg(top - 2);
    char *fs = freg(top - 1);
    top--;

    switch (node->kind) {
    case ND_ADD:
        if (node->ty->kind == TY_FLOAT)
            println("  addss %s, %s", fs, fd);
        else if (node->ty->kind == TY_DOUBLE)
            println("  addsd %s, %s", fs, fd);
        else
            println("  add %s, %s, %s", rd, rd, rs);
        return;
    case ND_SUB:
        if (node->ty->kind == TY_FLOAT)
            println("  subss %s, %s", fs, fd);
        else if (node->ty->kind == TY_DOUBLE)
            println("  subsd %s, %s", fs, fd);
        else
            println("  sub %s, %s, %s", rd, rd, rs);
        return;
    case ND_MUL:
        if (node->ty->kind == TY_FLOAT)
            println("  mulss %s, %s", fs, fd);
        else if (node->ty->kind == TY_DOUBLE)
            println("  mulsd %s, %s", fs, fd);
        else
            println("  mul %s, %s, %s", rd, rd, rs);
        return;
    case ND_DIV:
        if (node->ty->kind == TY_FLOAT)
            println("  divss %s, %s", fs, fd);
        else if (node->ty->kind == TY_DOUBLE)
            println("  divsd %s, %s", fs, fd);
        else
            divmod(node, rs, rd, "%rax");
        return;
    case ND_MOD:
        divmod(node, rs, rd, "%rdx");
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
        if (node->lhs->ty->kind == TY_FLOAT)
            println("  ucomiss %s, %s", fs, fd);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            println("  ucomisd %s, %s", fs, fd);
        else {
            println("  sub %s, zero, %s", rs, rs);
            println("  add %s, %s, %s", rd, rd, rs);
        }
        println("  seqz %s, %s", rd, rd);
        return;
    case ND_NE:
        if (node->lhs->ty->kind == TY_FLOAT)
            println("  ucomiss %s, %s", fs, fd);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            println("  ucomisd %s, %s", fs, fd);
        else {
            println("  sub %s, zero, %s", rs, rs);
            println("  add %s, %s, %s", rd, rd, rs);
        }
        println("  snez %s, %s", rd, rd);
        return;
    case ND_LT:
        if (node->lhs->ty->kind == TY_FLOAT) {
            println("  ucomiss %s, %s", fs, fd);
            println("  setb %%al");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            println("  ucomisd %s, %s", fs, fd);
            println("  setb %%al");
        } else {
            if (node->lhs->ty->is_unsigned)
                println("  setb %%al");
            else
                println("  slt %s, %s, %s", rd, rd, rs);
        }
        return;
    case ND_LE:
        if (node->lhs->ty->kind == TY_FLOAT) {
            println("  ucomiss %s, %s", fs, fd);
            println("  setbe %%al");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            println("  ucomisd %s, %s", fs, fd);
            println("  setbe %%al");
        } else {
            if (node->lhs->ty->is_unsigned)
                println("  setbe %%al");
            else {
                println("  addi %s, %s, 1", rs, rs);
                println("  slt %s, %s, %s", rd, rd, rs);
            }
        }
        return;
    case ND_SHL:
        println("  mov %s, %%rcx", reg(top));
        println("  shl %%cl, %s", rd);
        return;
    case ND_SHR:
        println("  mov %s, %%rcx", reg(top));
        if (node->lhs->ty->is_unsigned)
            println("  shr %%cl, %s", rd);
        else
            println("  sar %%cl, %s", rd);
        return;
    default:
        error_tok(node->tok, "invalid expression");
    }

}

static void gen_stmt(Node *node) {
    println("  .loc %d %d", node->tok->file_no, node->tok->line_no);

    switch (node->kind) {
    case ND_IF: {
        int c = count();
        if (node->els) {
            gen_expr(node->cond);
            println("  seqz %s, .L.else.%d", reg(--top), c);
            gen_stmt(node->then);
            println("  j .L.end.%d", c);
            println(".L.else.%d:", c);
            gen_stmt(node->els);
            println(".L.end.%d:", c);
        } else {
            gen_expr(node->cond);
            println("  beqz %s, .L.end.%d", reg(--top), c);
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
            println("  beqz %s, .L.break.%d", reg(--top), c);
        }
        gen_stmt(node->then);
        println(".L.continue.%d:", c);
        if (node->inc) {
            gen_expr(node->inc);
            top--;
        }
        println("  j .L.begin.%d", c);
        println(".L.break.%d:", c);

        brknum = brk;
        contnum = cont;
        return;
    }
    case ND_DO: {
        int c = count();
        int brk = brknum;
        int cont = contnum;
        brknum = contnum = c;

        println(".L.begin.%d:", c);
        gen_stmt(node->then);
        println(".L.continue.%d:", c);
        gen_expr(node->cond);
        cmp_zero(node->cond->ty);
        println("  jne .L.begin.%d", c);
        println(".L.break.%d:", c);

        brknum = brk;
        contnum = cont;
        return;
    }
    case ND_SWITCH: {
        int c = count();
        int brk = brknum;
        brknum = c;
        node->case_label = c;

        gen_expr(node->cond);

        for (Node *n = node->case_next; n; n = n->case_next) {
            n->case_label = count();
            n->case_end_label = c;
            println("  cmp $%ld, %s", n->val, reg(top - 1));
            println("  je .L.case.%d", n->case_label);
        }
        top--;

        if (node->default_case) {
            int i = count();
            node->default_case->case_end_label = c;
            node->default_case->case_label = i;
            println("  jmp .L.case.%d", i);
        }

        println("  jmp .L.break.%d", c);
        gen_stmt(node->then);
        println(".L.break.%d:", c);

        brknum = brk;
        return;
    }
    case ND_CASE:
        println(".L.case.%d:", node->case_label);
        gen_stmt(node->lhs);
        return;
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
        if (node->lhs) {
            gen_expr(node->lhs);
            if (is_flonum(node->lhs->ty))
                println("  movsd %s, %%xmm0", freg(--top));
            else
                println("  mv a0, %s", reg(--top));
        }
        println("  j .L.return.%s", current_fn->name);
        return;
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        top--;
        return;
    default:
        error_tok(node->tok, "invalid statement");
    }
}

static void emit_bss(Program *prog) {
    println("  .bss");

    for (Var *var = prog->globals; var; var = var->next) {
        if (var->init_data)
            continue;

        println("  .align %d", var->align);
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

        println("  .align %d", var->align);
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
        if (!fn->is_static) {
            println("  .globl %s", fn->name);
            println("  .type %s, @function", fn->name);
        }
        println("%s:", fn->name);
        current_fn = fn;
    
        // Prologue. %r12-15 are callee-saved retisters.
        println("  addi sp, sp, -%d", fn->stack_size);
        println("  sd ra, %d(sp)", fn->stack_size - 8);
        println("  sd s0, %d(sp)", fn->stack_size - 16);
        println("  addi s0, sp, %d", fn->stack_size);

        //// Save arg registers if function is variadic
        //if (fn->is_variadic) {
        //    println("  mov %%rdi, -128(%%rbp)");
        //    println("  mov %%rsi, -120(%%rbp)");
        //    println("  mov %%rdx, -112(%%rbp)");
        //    println("  mov %%rcx, -104(%%rbp)");
        //    println("  mov %%r8, -96(%%rbp)");
        //    println("  mov %%r9, -88(%%rbp)");
        //    println("  movsd %%xmm0, -80(%%rbp)");
        //    println("  movsd %%xmm1, -72(%%rbp)");
        //    println("  movsd %%xmm2, -64(%%rbp)");
        //    println("  movsd %%xmm3, -56(%%rbp)");
        //    println("  movsd %%xmm4, -48(%%rbp)");
        //    println("  movsd %%xmm5, -40(%%rbp)");
        //}

        //// Push arguments to the stack
        //int gp = 0, fp = 0;
        //for (Var *var = fn->params; var; var = var->next) {
        //    if (is_flonum(var->ty))
        //        fp++;
        //    else
        //        gp++;
        //}

        //for (Var *var = fn->params; var; var = var->next) {
        //    if (var->ty->kind == TY_FLOAT) {
        //        println("  movss %%xmm%d, -%d(%%rbp)", --fp, var->offset);
        //    } else if (var->ty->kind == TY_DOUBLE) {
        //        println("  movsd %%xmm%d, -%d(%%rbp)", --fp, var->offset);
        //    } else {
        //        char *r = get_argreg(var->ty->size, --gp);
        //        println("  mov %s, -%d(%%rbp)", r, var->offset);
        //    }
        //}
    
        // Emit code
        gen_stmt(fn->body);
        assert(top == 0);

        // The C spec defines a special rule for the main function.
        // Reaching the end of the main function is equivalent to
        // returning 0, even though the behavior is undefined for the
        // other functions. See C11 5.1.2.2.3.
        //if (strcmp(fn->name, "main") == 0)
        //    println("  mov $0, %%rax");
    
        // Epilogue
        println(".L.return.%s:", fn->name);
        println("  ld s0, %d(sp)", fn->stack_size - 16);
        println("  ld ra, %d(sp)", fn->stack_size - 8);
        println("  addi sp, sp, -%d", fn->stack_size);
        println("  jr ra");
    }
}

void codegen_riscv64(Program *prog) {
    char **paths = get_input_files();
    for (int i = 0; paths[i]; i++)
        println("  .file %d \"%s\"", i + 1, paths[i]);

    emit_bss(prog);
    emit_data(prog);
    emit_text(prog);
}
