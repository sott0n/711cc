#include "711cc.h"

static int top;
static int brknum;
static int contnum;
static char *argreg[] = {"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
static char *fargreg[] = {"fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"};
static Function *current_fn;

static int count(void) {
    static int i = 1;
    return i++;
}

static char *reg(int idx) {
    static char *r[] = {
        "t1", "t2", "t3", "t4", "t5", "t6", "s0", "s1", "s2", "s3",
        "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    };
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static char *freg(int idx) {
    static char *r[] = {
        "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "ft8", "ft9",
        "ft10", "ft11", "fs0", "fs1", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
        "fs8", "fs9", "fs10", "fs11",
    };
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
            println("  li t0, %d", node->var->offset);
            println("  sub %s, s0, t0", reg(top++));
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
            // Load an address to a register.
            println("  lui %s, %%hi(%s)", reg(top), node->var->name);
            println("  addi %s, %s, %%lo(%s)", reg(top), reg(top), node->var->name);
            top++;
        } else {
            // Load a 64-bit address value from memory and set it to a register.
            println("  la %s, %s", reg(top++), node->var->name);
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
        println("  li t0, %d", node->member->offset);
        println("  add %s, %s, t0", reg(top - 1), reg(top - 1));
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
        // automatially converted to a pointer to the first element of
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
    char *rd = reg(top - 1);

    // When we load a char or a short value to a register, we always
    // extend them to the size of int, so we can assume the lower half of
    // a register always contains a valid value. The upper half of a
    // register for char, short and int may contain garbage. When we load
    // a long value to a register, it simply occupies the entire register.
    if (ty->size == 1) {
        if (ty->is_unsigned)
            println("  lbu %s, 0(%s)", rd, rs);
        else
            println("  lb %s, 0(%s)", rd, rs);
    }
    else if (ty->size == 2)
        if (ty->is_unsigned)
            println("  lhu %s, 0(%s)", rd, rs);
        else
            println("  lh %s, 0(%s)", rd, rs);
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
            println("  lb t0, %d(%s)", i, rs);
            println("  sb t0, %d(%s)", i, rd);
        }
    } else if (ty->kind == TY_FLOAT) {
        println("  movss %s, (%s)", freg(top - 2), rd);
    } else if (ty->kind == TY_DOUBLE) {
        println("  movsd %s, (%s)", freg(top - 2), rd);
    } else if (ty->size == 1) {
        println("  sb %s, 0(%s)", rs, rd);
    } else if (ty->size == 2) {
        println("  sh %s, 0(%s)", rs, rd);
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
        char *tr = reg(--top);
        println("  snez %s, %s", tr, tr);
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

    // Cast instruction stored a value to a stack as temporary,
    // and load a value with casted specified size from stack.
    // In this code, temporary stack is 0(s0), so save value on
    // 0(s0) to t0 register, and re-store it to 0(s0) after cast.
    println("  ld t0, 0(s0)");
    if (to->size == 1) {
        println("  sd %s, 0(s0)", r);
        if (to->is_unsigned)
            println("  lbu %s, 0(s0)", r);
        else
            println("  lb %s, 0(s0)", r);
    } else if (to->size == 2) {
        println("  sd %s, 0(s0)", r);
        if (to->is_unsigned)
            println("  lhu %s, 0(s0)", r);
        else
            println("  lh %s, 0(s0)", r);
    } else if (to->size == 4) {
        println("  sd %s, 0(s0)", r);
        println("  lw %s, 0(s0)", r);
    } else if (is_integer(from) && from->size < 8 && !from->is_unsigned) {
        println("  sd %s, 0(s0)", r);
        println("  ld %s, 0(s0)", r);
    }
    println("  sd t0, 0(s0)");
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
    println("  li t0, %d", offset);
    println("  sub t0, s0, t0");
    if (ty->size == 1) {
        if (ty->is_unsigned)
            println("  lb %s, 0(t0)", argreg[r]);
        else
            println("  lbu %s, 0(t0)", argreg[r]);
    } else if (ty->size == 2) {
        if (ty->is_unsigned)
            println("  lh %s, 0(t0)", argreg[r]);
        else
            println("  lhu %s, 0(t0)", argreg[r]);
    } else if (ty->size == 4) {
        println("  lw %s, 0(t0)", argreg[r]);
    } else {
        println("  ld %s, 0(t0)", argreg[r]);
    }
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
            if (gp < 8) {
                load_gp_arg(arg->ty, arg->offset, gp++);
                continue;
            }
        }

        pass_stack[i] = true;
        stack_size += 8;
    }

    // If we have arguments passed on the stack, push them to the stack.
    //if (stack_size) {
    //    if (stack_size % 16) {
    //        println("  sub s0, s0, 8");
    //        stack_size += 8;
    //    }

    //    for (int i = node->nargs - 1; i >= 0; i--) {
    //        if (!pass_stack[i])
    //            continue;
    //        Var *arg = node->args[i];
    //        push_arg(arg->ty, arg->offset);
    //    }
    //}

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
        println("  beqz %s, .L.else.%d", reg(--top), c);
        gen_expr(node->then);
        top--;
        println("  j .L.end.%d", c);
        println(".L.else.%d:", c);
        gen_expr(node->els);
        println(".L.end.%d:", c);
        return;
    }
    case ND_NOT: {
        gen_expr(node->lhs);
        char *tr = reg(--top);
        println("  seqz %s, %s", tr, tr);
        top++;
        return;
    }
    case ND_BITNOT: {
        gen_expr(node->lhs);
        char *tr = reg(top - 1);
        println("  not %s, %s", tr, tr);
        return;
    }
    case ND_LOGAND: {
        int c = count();
        gen_expr(node->lhs);
        println("  beqz %s, .L.false.%d", reg(--top), c);
        gen_expr(node->rhs);
        println("  beqz %s, .L.false.%d", reg(--top), c);
        println("  li %s, 1", reg(top));
        println("  j .L.end.%d", c);
        println(".L.false.%d:", c);
        println("  mv %s, zero", reg(top++));
        println(".L.end.%d:", c);
        return;
    }
    case ND_LOGOR: {
        int c = count();
        gen_expr(node->lhs);
        println("  bnez %s, .L.true.%d", reg(--top), c);
        gen_expr(node->rhs);
        println("  bnez %s, .L.true.%d", reg(--top), c);
        println("  mv %s, zero", reg(top));
        println("  j .L.end.%d", c);
        println(".L.true.%d:", c);
        println("  li %s, 1", reg(top++));
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
        println("  addi sp, sp, -72");
        println("  sd ra, 8(sp)");
        println("  sd t0, 16(sp)");
        println("  sd t1, 24(sp)");
        println("  sd t2, 32(sp)");
        println("  sd t3, 40(sp)");
        println("  sd t4, 48(sp)");
        println("  sd t5, 56(sp)");
        println("  sd t6, 64(sp)");

        int memarg_size = load_args(node);

        // Call a function
        println("  call %s", node->lhs->var->name);

        if (memarg_size)
            println("  sub s0, s0, %d", memarg_size);

        // Restore caller-saved registers
        println("  ld ra, 8(sp)");
        println("  ld t0, 16(sp)");
        println("  ld t1, 24(sp)");
        println("  ld t2, 32(sp)");
        println("  ld t3, 40(sp)");
        println("  ld t4, 48(sp)");
        println("  ld t5, 56(sp)");
        println("  ld t6, 64(sp)");
        println("  addi sp, sp, 72");

        if (node->ty->kind == TY_FLOAT)
            println("  movss %%xmm0, %s", freg(top++));
        else if (node->ty->kind == TY_DOUBLE)
            println("  movsd %%xmm0, %s", freg(top++));
        else
            println("  mv %s, a0", reg(top++));

        return;
    }
    }

    // Binary expressions
    gen_expr(node->lhs);
    gen_expr(node->rhs);

    char *rd = reg(top - 2);
    char *rs = reg(top - 1);
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
        if (node->ty->is_unsigned)
            println("  remu %s, %s, %s", rd, rd, rs);
        else
            println("  rem %s, %s, %s", rd, rd, rs);
        return;
    case ND_BITAND:
        println("  and %s, %s, %s", rd, rd, rs);
        return;
    case ND_BITOR:
        println("  or %s, %s, %s", rd, rd, rs);
        return;
    case ND_BITXOR:
        println("  xor %s, %s, %s", rd, rd, rs);
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
        println("  sll %s, %s, %s", rd, rd, reg(top));
        return;
    case ND_SHR:
        if (node->lhs->ty->is_unsigned)
            println("  srl %s, %s, %s", rd, rd, reg(top));
        else
            println("  sra %s, %s, %s", rd, rd, reg(top));
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
            println("  beqz %s, .L.else.%d", reg(--top), c);
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
        int buf_pos = 0;
        char buf[256];
        while (pos < var->ty->size) {
            if (rel && rel->offset == pos) {
                println("  .quad %s%+ld", rel->label, rel->addend);
                rel = rel->next;
                pos += 8;
            } else {
                // Follow escape sequence
                switch (var->init_data[pos]) {
                case '\0': break;
                case '"':
                case '\a':
                case '\b':
                case '\t':
                case '\v':
                case '\f':
                case '\r': {
                    buf[buf_pos++] = '\\';
                    buf[buf_pos++] = var->init_data[pos++];
                    continue;
                }
                case '\n':
                    buf[buf_pos++] = '\\';
                    buf[buf_pos++] = 'n';
                    pos++;
                    continue;
                }
                buf[buf_pos++] = var->init_data[pos++];
            }
        }
        println("  .string \"%s\"", buf);
    }
}

static void emit_text(Program *prog) {
    println("  .text");

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        println("  .align 1");
        if (!fn->is_static) {
            println("  .globl %s", fn->name);
        }
        println("  .type %s, @function", fn->name);
        println("%s:", fn->name);
        current_fn = fn;

        int stack_size = fn->stack_size;
    
        // Prologue. s0-11, fs0-11 are callee-saved retisters.
        println("  li t0, %d", fn->stack_size);
        println("  sub sp, sp, t0");
        if (strcmp(fn->name, "main") == 0) {
            println("  li t0, %d", fn->stack_size - 8);
            println("  add t0, sp, t0");
            println("  sd ra, 0(t0)");
            stack_size = stack_size - 8;
        }
        println("  li t0, %d", stack_size - 8);
        println("  add t0, sp, t0");
        println("  sd s0, 0(t0)");
        println("  li t0, %d", stack_size - 16);
        println("  add t0, sp, t0");
        println("  sd s1, 0(t0)");
        println("  li t0, %d", stack_size - 32);
        println("  add t0, sp, t0");
        println("  sd s2, 0(t0)");
        println("  li t0, %d", stack_size - 40);
        println("  add t0, sp, t0");
        println("  sd s3, 0(t0)");
        println("  li t0, %d", stack_size - 48);
        println("  add t0, sp, t0");
        println("  sd s4, 0(t0)");
        println("  li t0, %d", stack_size - 56);
        println("  add t0, sp, t0");
        println("  sd s5, 0(t0)");
        println("  li t0, %d", stack_size - 64);
        println("  add t0, sp, t0");
        println("  sd s6, 0(t0)");
        println("  li t0, %d", stack_size - 72);
        println("  add t0, sp, t0");
        println("  sd s7, 0(t0)");
        println("  li t0, %d", stack_size - 80);
        println("  add t0, sp, t0");
        println("  sd s8, 0(t0)");
        println("  li t0, %d", stack_size - 88);
        println("  add t0, sp, t0");
        println("  sd s9, 0(t0)");
        println("  li t0, %d", stack_size - 96);
        println("  add t0, sp, t0");
        println("  sd s10, 0(t0)");
        println("  li t0, %d", stack_size - 104);
        println("  add t0, sp, t0");
        println("  sd s11, 0(t0)");

        println("  li t0, %d", fn->stack_size);
        println("  add s0, sp, t0");

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

        // Push arguments to the stack
        int gp = 0, fp = 0;
        for (Var *var = fn->params; var; var = var->next) {
            if (is_flonum(var->ty))
                fp++;
            else
                gp++;
        }

        for (Var *var = fn->params; var; var = var->next) {
            if (var->ty->kind == TY_FLOAT) {
                println("  movss %%xmm%d, -%d(%%rbp)", --fp, var->offset);
            } else if (var->ty->kind == TY_DOUBLE) {
                println("  movsd %%xmm%d, -%d(%%rbp)", --fp, var->offset);
            } else {
                char *r = argreg[--gp];
                if (var->ty->size == 1)
                    println("  sb %s, -%d(s0)", r, var->offset);
                else if (var->ty->size == 2)
                    println("  sh %s, -%d(s0)", r, var->offset);
                else if (var->ty->size == 4)
                    println("  sw %s, -%d(s0)", r, var->offset);
                else
                    println("  sd %s, -%d(s0)", r, var->offset);
            }
        }
    
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
        println("  li t0, %d", stack_size - 8);
        println("  add t0, sp, t0");
        println("  ld s0, 0(t0)");
        println("  li t0, %d", stack_size - 16);
        println("  add t0, sp, t0");
        println("  ld s1, 0(t0)");
        println("  li t0, %d", stack_size - 32);
        println("  add t0, sp, t0");
        println("  ld s2, 0(t0)");
        println("  li t0, %d", stack_size - 40);
        println("  add t0, sp, t0");
        println("  ld s3, 0(t0)");
        println("  li t0, %d", stack_size - 48);
        println("  add t0, sp, t0");
        println("  ld s4, 0(t0)");
        println("  li t0, %d", stack_size - 56);
        println("  add t0, sp, t0");
        println("  ld s5, 0(t0)");
        println("  li t0, %d", stack_size - 64);
        println("  add t0, sp, t0");
        println("  ld s6, 0(t0)");
        println("  li t0, %d", stack_size - 72);
        println("  add t0, sp, t0");
        println("  ld s7, 0(t0)");
        println("  li t0, %d", stack_size - 80);
        println("  add t0, sp, t0");
        println("  ld s8, 0(t0)");
        println("  li t0, %d", stack_size - 88);
        println("  add t0, sp, t0");
        println("  ld s9, 0(t0)");
        println("  li t0, %d", stack_size - 96);
        println("  add t0, sp, t0");
        println("  ld s10, 0(t0)");
        println("  li t0, %d", stack_size - 104);
        println("  add t0, sp, t0");
        println("  ld s11, 0(t0)");

        if (strcmp(fn->name, "main") == 0) {
            println("  li t0, %d", fn->stack_size - 8);
            println("  add t0, sp, t0");
            println("  ld ra, 0(t0)");
        }

        println("  li t0, %d", fn->stack_size);
        println("  add sp, sp, t0");
        println("  jr ra");
        println("  .size %s, .-%s", fn->name, fn->name);
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
