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
    static char *r[] = {
        "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%rbx", "%r8",
    };
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static char *dreg(int idx) {
    static char *r[] = {
        "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d", "%ebx", "%r8d",
    };
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static char *xreg(Type *ty, int idx) {
    if (ty->base || ty->size == 8)
        return reg(idx);
    return dreg(idx);
}

static char *breg(int idx) {
    static char *r[] = {
        "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b", "%bl", "%r8b",
    };
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static char *wreg(int idx) {
    static char *r[] = {
        "%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w", "%bx", "%r8w",
    };
    if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
        error("register out of range: %d", idx);
    return r[idx];
}

static char *freg(int idx) {
    static char *r[] = {
        "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13",
        "%xmm14", "%xmm15",
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
            println("  lea -%d(%%rbp), %s", node->var->offset, reg(top++));
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
    case ND_FUNCALL:
        if (node->ty->kind == TY_STRUCT) {
            gen_expr(node);
            return;
        }
        break;
    case ND_COMMA:
        gen_expr(node->lhs);
        top--;
        gen_addr(node->rhs);
        return;
    case ND_MEMBER:
        gen_expr(node->lhs);
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
    else
        println("  mov (%s), %s", rs, rd);
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
        println("  mov %s, (%s)", breg(top - 2), rd);
    } else if (ty->size == 2) {
        println("  mov %s, (%s)", wreg(top - 2), rd);
    } else if (ty->size == 4) {
        println("  mov %s, (%s)", xreg(ty, top - 2), rd);
    } else {
        println("  mov %s, (%s)", rs, rd);
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

static void sign_extend_for_float(Type *from, char *r) {
    if (!is_integer(from) || from->is_unsigned || from->size == 8)
        return;

    if (from->size == 1)
        println("  movsx %s, %s", breg(top - 1), r);
    else if (from->size == 2)
        println("  movsx %s, %s", wreg(top - 1), r);
    else
        println("  movsx %s, %s", dreg(top - 1), r);
}

static void cast(Type *from, Type *to) {
    if (to->kind == TY_VOID || to->kind == TY_STRUCT)
        return;

    char *r = reg(top - 1);
    char *fr = freg(top - 1);

    if (to->kind == TY_BOOL) {
        cmp_zero(from);
        println("  setne %s", breg(top));
        println("  movzx %s, %s", breg(top), reg(top));
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
        sign_extend_for_float(from, r);
        println("  cvtsi2ss %s, %s", r, fr);
        return;
    }

    if (to->kind == TY_DOUBLE) {
        if (from->size == 8 && from->is_unsigned)
            convert_ulong_double(r, fr);
        else {
            sign_extend_for_float(from, r);
            println("  cvtsi2sd %s, %s", r, fr);
        }
        return;
    }

    char *insn = to->is_unsigned ? "movzx" : "movsx";

    if (to->size == 1) {
        println("  %s %s, %s", insn, breg(top - 1), r);
    } else if (to->size == 2) {
        println("  %s %s, %s", insn, wreg(top - 1), r);
    } else if (to->size == 4) {
        println("  mov %s, %s", dreg(top - 1), dreg(top - 1));
    } else if (is_integer(from) && from->size < 8 && !from->is_unsigned) {
        println("  movsx %s, %s", dreg(top - 1), r);
    }
}

static void divmod(Node *node, char *rs, char *rd, char *r64, char *r32) {
    if (node->ty->size == 8) {
        println("  mov %s, %%rax", rd);
        if (node->ty->is_unsigned) {
            println("  mov $0, %%rdx");
            println("  div %s", rs);
        } else {
            println("  cqo");
            println("  idiv %s", rs);
        }
        println("  mov %s, %s", r64, rd);
    } else {
        println("  mov %s, %%eax", rd);
        if (node->ty->is_unsigned) {
            println("  mov $0, %%edx");
            println("  div %s", rs);
        } else {
            println("  cdq");
            println("  idiv %s", rs);
        }
        println("  mov %s, %s", r32, rd);
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
    println("  lea 16(%%rbp), %%rcx");
    println("  mov %%rcx, 8(%%rax)");
    println("  mov %%rbp, 16(%%rax)");
    println("  subq $128, 16(%%rax)");
    top++;
}

static void builtin_va_arg(Node *node) {
    gen_expr(node->lhs);

    char *ap = reg(top - 1);
    int offset = is_flonum(node->ty) ? 4 : 0;
    int limit = is_flonum(node->ty) ? 96 : 48;
    int c = count();

    println("  mov %d(%s), %%eax", offset, ap);
    println("  cmp $%d, %%eax", limit);
    println("  jae .L.va_arg.stack.%d", c);
    println("  mov 16(%s), %%rcx", ap);
    println("  add %%rax, %%rcx");
    println("  addl $8, %d(%s)", offset, ap);
    println("  mov %%rcx, %s", ap);
    println("  jmp .L.va_arg.end.%d", c);
    println(".L.va_arg.stack.%d:", c);
    println("  mov 8(%s), %%rax", ap);
    println("  lea %d(%%rax), %%rcx", align_to(node->ty->size, 8));
    println("  mov %%rcx, 8(%s)", ap);
    println("  mov %%rax, %s", ap);
    println(".L.va_arg.end.%d:", c);
    load(node->ty);
}

static void builtin_alloca(Node *node) {
    if (node->nargs != 1)
        error_tok(node->tok, "alloca expects one argument");

    Var *size = node->args[0];
    char *r = reg(top);

    println("  mov -%d(%%rbp), %s", size->offset, r);
    println("  add $15, %s", r);
    println("  and $-16, %s", r);
    println("  sub %s, %%rsp", r);
    println("  mov %%rsp, %s", r);
    top++;
}

static int atomic_size(Node *node) {
    if (node->nargs < 1 || !node->args[0]->ty->base)
        error_tok(node->tok, "atomic operation expects a pointer");

    int size = node->args[0]->ty->base->size;
    if (size != 1 && size != 2 && size != 4 && size != 8)
        error_tok(node->tok, "unsupported atomic operand size: %d", size);
    return size;
}

static char *atomic_suffix(int size) {
    if (size == 1)
        return "b";
    if (size == 2)
        return "w";
    if (size == 4)
        return "l";
    return "q";
}

static char *atomic_reg(int size, char *r8, char *r16, char *r32, char *r64) {
    if (size == 1)
        return r8;
    if (size == 2)
        return r16;
    if (size == 4)
        return r32;
    return r64;
}

static void atomic_result(void) {
    println("  mov %%rax, %s", reg(top++));
}

static void builtin_atomic(Node *node, char *name) {
    if (!strcmp(name, "__atomic_thread_fence")) {
        if (node->nargs != 1)
            error_tok(node->tok, "__atomic_thread_fence expects one argument");
        println("  mfence");
        println("  mov $0, %s", reg(top++));
        return;
    }

    int size = atomic_size(node);
    char *suffix = atomic_suffix(size);
    char *acc = atomic_reg(size, "%al", "%ax", "%eax", "%rax");
    char *desired = atomic_reg(size, "%dl", "%dx", "%edx", "%rdx");
    char *operand = atomic_reg(size, "%sil", "%si", "%esi", "%rsi");

    println("  mov -%d(%%rbp), %%rcx", node->args[0]->offset);

    if (!strcmp(name, "__atomic_load_n")) {
        if (node->nargs != 2)
            error_tok(node->tok, "__atomic_load_n expects two arguments");
        println("  xor %%eax, %%eax");
        println("  mov%s (%%rcx), %s", suffix, acc);
        atomic_result();
        return;
    }

    if (!strcmp(name, "__atomic_store_n")) {
        if (node->nargs != 3)
            error_tok(node->tok, "__atomic_store_n expects three arguments");
        println("  mov -%d(%%rbp), %%rax", node->args[1]->offset);
        println("  xchg%s %s, (%%rcx)", suffix, acc);
        println("  xor %%eax, %%eax");
        atomic_result();
        return;
    }

    if (!strcmp(name, "__atomic_fetch_add")) {
        if (node->nargs != 3)
            error_tok(node->tok, "__atomic_fetch_add expects three arguments");
        println("  mov -%d(%%rbp), %%rax", node->args[1]->offset);
        println("  lock xadd%s %s, (%%rcx)", suffix, acc);
        atomic_result();
        return;
    }

    if (!strcmp(name, "__atomic_exchange_n")) {
        if (node->nargs != 3)
            error_tok(node->tok, "__atomic_exchange_n expects three arguments");
        println("  mov -%d(%%rbp), %%rax", node->args[1]->offset);
        println("  xchg%s %s, (%%rcx)", suffix, acc);
        atomic_result();
        return;
    }

    if (!strcmp(name, "__atomic_fetch_and") ||
        !strcmp(name, "__atomic_fetch_or")) {
        if (node->nargs != 3)
            error_tok(node->tok, "%s expects three arguments", name);
        int c = count();
        char *insn = !strcmp(name, "__atomic_fetch_and") ? "and" : "or";
        println("  mov -%d(%%rbp), %%rsi", node->args[1]->offset);
        println("  xor %%eax, %%eax");
        println("  mov%s (%%rcx), %s", suffix, acc);
        println(".L.atomic.retry.%d:", c);
        println("  mov %%rax, %%rdx");
        println("  %s%s %s, %s", insn, suffix, operand, desired);
        println("  lock cmpxchg%s %s, (%%rcx)", suffix, desired);
        println("  jne .L.atomic.retry.%d", c);
        atomic_result();
        return;
    }

    if (!strcmp(name, "__atomic_compare_exchange_n")) {
        if (node->nargs != 6)
            error_tok(node->tok,
                      "__atomic_compare_exchange_n expects six arguments");
        int c = count();
        println("  mov -%d(%%rbp), %%rdx", node->args[1]->offset);
        println("  mov -%d(%%rbp), %%rsi", node->args[2]->offset);
        println("  xor %%eax, %%eax");
        println("  mov%s (%%rdx), %s", suffix, acc);
        println("  lock cmpxchg%s %s, (%%rcx)", suffix, operand);
        println("  sete %%dil");
        println("  je .L.atomic.compare.end.%d", c);
        println("  mov%s %s, (%%rdx)", suffix, acc);
        println(".L.atomic.compare.end.%d:", c);
        println("  movzbq %%dil, %%rax");
        atomic_result();
        return;
    }

    error_tok(node->tok, "unknown atomic builtin: %s", name);
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

static bool pass_by_stack(Type *ty) {
    return ty->kind == TY_STRUCT && ty->size > 16;
}

static int gp_arg_slots(Type *ty) {
    if (ty->kind == TY_STRUCT)
        return (ty->size + 7) / 8;
    return 1;
}

static void load_small_struct_arg(Type *ty, int offset, int *gp) {
    for (int start = 0; start < ty->size; start += 8) {
        int len = ty->size - start < 8 ? ty->size - start : 8;
        int r = (*gp)++;

        println("  xor %s, %s", argreg32[r], argreg32[r]);
        for (int i = len - 1; i >= 0; i--) {
            if (i != len - 1)
                println("  shl $8, %s", argreg64[r]);
            println("  mov -%d(%%rbp), %s", offset - start - i,
                    argreg8[r]);
        }
    }
}

static int stack_arg_align(Type *ty) {
    if (ty->align < 8)
        return 8;
    if (ty->align > 16)
        return 16;
    return ty->align;
}

static int stack_arg_size(Type *ty) {
    return align_to(ty->size, 8);
}

static void copy_arg_to_stack(Type *ty, int src_offset, int dst_offset) {
    for (int i = 0; i < ty->size; i++) {
        println("  mov %d(%%rbp), %%al", i - src_offset);
        println("  mov %%al, %d(%%rsp)", dst_offset + i);
    }
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
static int load_args(Node *node, int gp) {
    int fp = 0;
    int stack_size = 0;
    bool *pass_stack = calloc(node->nargs, sizeof(bool));
    int *stack_offset = calloc(node->nargs, sizeof(int));

    // Load as many arguments as possible to the registers.
    for (int i = 0; i < node->nargs; i++) {
        Var *arg = node->args[i];
        int gp_slots = gp_arg_slots(arg->ty);

        if (pass_by_stack(arg->ty)) {
            pass_stack[i] = true;
        } else if (is_flonum(arg->ty)) {
            if (fp < 8) {
                load_fp_arg(arg->ty, arg->offset, fp++);
                continue;
            }
        } else if (gp + gp_slots <= 6) {
            if (arg->ty->kind == TY_STRUCT)
                load_small_struct_arg(arg->ty, arg->offset, &gp);
            else
                load_gp_arg(arg->ty, arg->offset, gp++);
            continue;
        }

        pass_stack[i] = true;
        stack_size = align_to(stack_size, stack_arg_align(arg->ty));
        stack_offset[i] = stack_size;
        stack_size += stack_arg_size(arg->ty);
    }

    // Reserve the stack argument area and copy arguments into it.
    if (stack_size) {
        stack_size = align_to(stack_size, 16);
        println("  sub $%d, %%rsp", stack_size);

        for (int i = 0; i < node->nargs; i++) {
            if (!pass_stack[i])
                continue;
            Var *arg = node->args[i];
            copy_arg_to_stack(arg->ty, arg->offset, stack_offset[i]);
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
    case ND_LABEL_VAL:
        println("  lea .L.label.%s.%s(%%rip), %s", current_fn->name,
                node->label_name, reg(top++));
        return;
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
            println("  mov $%lu, %s", node->val, reg(top++));
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
        cmp_zero(node->cond->ty);
        println("  je .L.else.%d", c);
        gen_expr(node->then);
        top--;
        println("  jmp .L.end.%d", c);
        println(".L.else.%d:", c);
        gen_expr(node->els);
        println(".L.end.%d:", c);
        return;
    }
    case ND_NOT:
        gen_expr(node->lhs);
        cmp_zero(node->lhs->ty);
        println("  sete %s", breg(top));
        println("  movzx %s, %s", breg(top), reg(top));
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

        if (node->lhs->kind == ND_VAR &&
                (!strcmp(node->lhs->var->name, "alloca") ||
                 !strcmp(node->lhs->var->name, "__builtin_alloca"))) {
            builtin_alloca(node);
            return;
        }

        if (node->lhs->kind == ND_VAR &&
                !strncmp(node->lhs->var->name, "__atomic_", 9)) {
            builtin_atomic(node, node->lhs->var->name);
            return;
        }

        // Save caller-saved registers
        println("  sub $96, %%rsp");
        println("  mov %%r10, (%%rsp)");
        println("  mov %%r11, 8(%%rsp)");
        println("  mov %%r8, 16(%%rsp)");
        println("  movsd %%xmm8, 24(%%rsp)");
        println("  movsd %%xmm9, 32(%%rsp)");
        println("  movsd %%xmm10, 40(%%rsp)");
        println("  movsd %%xmm11, 48(%%rsp)");
        println("  movsd %%xmm12, 56(%%rsp)");
        println("  movsd %%xmm13, 64(%%rsp)");
        println("  movsd %%xmm14, 72(%%rsp)");
        println("  movsd %%xmm15, 80(%%rsp)");

        gen_expr(node->lhs);
        println("  mov %s, %%r11", reg(--top));
        if (node->ret_buffer)
            println("  lea -%d(%%rbp), %%rdi", node->ret_buffer->offset);
        int memarg_size = load_args(node, node->ret_buffer ? 1 : 0);

        // Call a function
        println("  call *%%r11");

        if (memarg_size)
            println("  add $%d, %%rsp", memarg_size);

        // The Systen V x86-64 ABI has a special rule regarding a boolean
        // return value that only the lower 8 bits are valid for it and
        // the upper 56bits may contain garbage. Here, we clear the upper
        // 56 bits.
        if (node->ty->kind == TY_BOOL)
            println("  movzx %%al, %%eax");

        // Restore caller-saved registers
        println("  mov (%%rsp), %%r10");
        println("  mov 8(%%rsp), %%r11");
        println("  mov 16(%%rsp), %%r8");
        println("  movsd 24(%%rsp), %%xmm8");
        println("  movsd 32(%%rsp), %%xmm9");
        println("  movsd 40(%%rsp), %%xmm10");
        println("  movsd 48(%%rsp), %%xmm11");
        println("  movsd 56(%%rsp), %%xmm12");
        println("  movsd 64(%%rsp), %%xmm13");
        println("  movsd 72(%%rsp), %%xmm14");
        println("  movsd 80(%%rsp), %%xmm15");
        println("  add $96, %%rsp");

        if (node->ret_buffer)
            println("  lea -%d(%%rbp), %s", node->ret_buffer->offset,
                    reg(top++));
        else if (node->ty->kind == TY_FLOAT)
            println("  movss %%xmm0, %s", freg(top++));
        else if (node->ty->kind == TY_DOUBLE)
            println("  movsd %%xmm0, %s", freg(top++));
        else
            println("  mov %%rax, %s", reg(top++));
        return;
    }
    case ND_VA_ARG:
        builtin_va_arg(node);
        return;
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
            println("  add %s, %s", rs, rd);
        return;
    case ND_SUB:
        if (node->ty->kind == TY_FLOAT)
            println("  subss %s, %s", fs, fd);
        else if (node->ty->kind == TY_DOUBLE)
            println("  subsd %s, %s", fs, fd);
        else
            println("  sub %s, %s", rs, rd);
        return;
    case ND_MUL:
        if (node->ty->kind == TY_FLOAT)
            println("  mulss %s, %s", fs, fd);
        else if (node->ty->kind == TY_DOUBLE)
            println("  mulsd %s, %s", fs, fd);
        else
            println("  imul %s, %s", rs, rd);
        return;
    case ND_DIV:
        if (node->ty->kind == TY_FLOAT)
            println("  divss %s, %s", fs, fd);
        else if (node->ty->kind == TY_DOUBLE)
            println("  divsd %s, %s", fs, fd);
        else
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
        if (node->lhs->ty->kind == TY_FLOAT)
            println("  ucomiss %s, %s", fs, fd);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            println("  ucomisd %s, %s", fs, fd);
        else
            println("  cmp %s, %s", rs, rd);
        println("  sete %%al");
        println("  movzx %%al, %s", rd);
        return;
    case ND_NE:
        if (node->lhs->ty->kind == TY_FLOAT)
            println("  ucomiss %s, %s", fs, fd);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            println("  ucomisd %s, %s", fs, fd);
        else
            println("  cmp %s, %s", rs, rd);
        println("  setne %%al");
        println("  movzx %%al, %s", rd);
        return;
    case ND_LT:
        if (node->lhs->ty->kind == TY_FLOAT) {
            println("  ucomiss %s, %s", fs, fd);
            println("  setb %%al");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            println("  ucomisd %s, %s", fs, fd);
            println("  setb %%al");
        } else {
            println("  cmp %s, %s", rs, rd);
            if (node->lhs->ty->is_unsigned)
                println("  setb %%al");
            else
                println("  setl %%al");
        }
        println("  movzx %%al, %s", rd);
        return;
    case ND_LE:
        if (node->lhs->ty->kind == TY_FLOAT) {
            println("  ucomiss %s, %s", fs, fd);
            println("  setbe %%al");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            println("  ucomisd %s, %s", fs, fd);
            println("  setbe %%al");
        } else {
            println("  cmp %s, %s", rs, rd);
            if (node->lhs->ty->is_unsigned)
                println("  setbe %%al");
            else
                println("  setle %%al");
        }
        println("  movzx %%al, %s", rd);
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
    case ND_NULL_EXPR:
        return;
    case ND_IF: {
        int c = count();
        if (node->els) {
            gen_expr(node->cond);
            cmp_zero(node->cond->ty);
            println("  je .L.else.%d", c);
            gen_stmt(node->then);
            println("  jmp .L.end.%d", c);
            println(".L.else.%d:", c);
            gen_stmt(node->els);
            println(".L.end.%d:", c);
        } else {
            gen_expr(node->cond);
            cmp_zero(node->cond->ty);
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
            cmp_zero(node->cond->ty);
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
            println("  cmp $%ld, %s", n->val, xreg(node->cond->ty, top - 1));
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
    case ND_ASM:
        if (!node->asm_str[0])
            return;
        if (!node->var)
            error_tok(node->tok, "inline assembly operand is not a local variable");
        if (strstr(node->asm_str, "fnstcw %0")) {
            println("  fnstcw -%d(%%rbp)", node->var->offset);
            return;
        }
        if (strstr(node->asm_str, "fldcw %0")) {
            println("  fldcw -%d(%%rbp)", node->var->offset);
            return;
        }
        error_tok(node->tok, "unsupported inline assembly template");
    case ND_GOTO:
        if (node->lhs) {
            gen_expr(node->lhs);
            println("  jmp *%s", reg(--top));
            return;
        }
        println("  jmp .L.label.%s.%s", current_fn->name, node->label_name);
        return;
    case ND_LABEL:
        println(".L.label.%s.%s:", current_fn->name, node->label_name);
        gen_stmt(node->lhs);
        return;
    case ND_RETURN:
        if (node->lhs) {
            gen_expr(node->lhs);
            if (current_fn->ret_buffer) {
                char *src = reg(top - 1);
                println("  mov -%d(%%rbp), %%rax", current_fn->ret_buffer->offset);
                for (int i = 0; i < node->lhs->ty->size; i++) {
                    println("  mov %d(%s), %%dl", i, src);
                    println("  mov %%dl, %d(%%rax)", i);
                }
                top--;
            } else if (is_flonum(node->lhs->ty))
                println("  movsd %s, %%xmm0", freg(--top));
            else
                println("  mov %s, %%rax", reg(--top));
        }
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

static void store_small_struct_param(Var *var, int *gp) {
    for (int start = 0; start < var->ty->size; start += 8) {
        int len = var->ty->size - start < 8 ? var->ty->size - start : 8;
        println("  mov %s, %%rax", argreg64[(*gp)++]);
        for (int i = 0; i < len; i++) {
            println("  mov %%al, -%d(%%rbp)", var->offset - start - i);
            if (i + 1 < len)
                println("  shr $8, %%rax");
        }
    }
}

static void copy_stack_param(Var *var, int src_offset) {
    for (int i = 0; i < var->ty->size; i++) {
        println("  mov %d(%%rbp), %%al", src_offset + i);
        println("  mov %%al, %d(%%rbp)", i - var->offset);
    }
}

static void emit_text(Program *prog) {
    println("  .text");

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        if (!fn->is_static)
            println("  .globl %s", fn->name);
        println("%s:", fn->name);
        current_fn = fn;
    
        // Prologue. %rbx and %r12-15 are callee-saved registers.
        println("  push %%rbp");
        println("  mov %%rsp, %%rbp");
        println("  sub $%d, %%rsp", fn->stack_size);
        println("  mov %%r12, -8(%%rbp)");
        println("  mov %%r13, -16(%%rbp)");
        println("  mov %%r14, -24(%%rbp)");
        println("  mov %%r15, -32(%%rbp)");
        println("  mov %%rbx, -%d(%%rbp)", fn->is_variadic ? 136 : 40);

        // Save arg registers if function is variadic
        if (fn->is_variadic) {
            println("  mov %%rdi, -128(%%rbp)");
            println("  mov %%rsi, -120(%%rbp)");
            println("  mov %%rdx, -112(%%rbp)");
            println("  mov %%rcx, -104(%%rbp)");
            println("  mov %%r8, -96(%%rbp)");
            println("  mov %%r9, -88(%%rbp)");
            println("  movsd %%xmm0, -80(%%rbp)");
            println("  movsd %%xmm1, -72(%%rbp)");
            println("  movsd %%xmm2, -64(%%rbp)");
            println("  movsd %%xmm3, -56(%%rbp)");
            println("  movsd %%xmm4, -48(%%rbp)");
            println("  movsd %%xmm5, -40(%%rbp)");
        }

        // Save register and stack arguments to their local variable slots.
        int nparams = 0;
        for (Var *var = fn->params; var; var = var->next)
            nparams++;

        Var **params = calloc(nparams, sizeof(Var *));
        int i = nparams;
        for (Var *var = fn->params; var; var = var->next)
            params[--i] = var;

        int gp = 0;
        int fp = 0;
        int stack_offset = 16;
        if (fn->ret_buffer) {
            println("  mov %%rdi, -%d(%%rbp)", fn->ret_buffer->offset);
            gp = 1;
        }

        for (i = 0; i < nparams; i++) {
            Var *var = params[i];
            int gp_slots = gp_arg_slots(var->ty);
            bool on_stack = pass_by_stack(var->ty) ||
                (is_flonum(var->ty) ? fp >= 8 : gp + gp_slots > 6);

            if (on_stack) {
                stack_offset = align_to(stack_offset, stack_arg_align(var->ty));
                copy_stack_param(var, stack_offset);
                stack_offset += stack_arg_size(var->ty);
            } else if (var->ty->kind == TY_FLOAT) {
                println("  movss %%xmm%d, -%d(%%rbp)", fp++, var->offset);
            } else if (var->ty->kind == TY_DOUBLE) {
                println("  movsd %%xmm%d, -%d(%%rbp)", fp++, var->offset);
            } else if (var->ty->kind == TY_STRUCT) {
                store_small_struct_param(var, &gp);
            } else {
                char *r = get_argreg(var->ty->size, gp++);
                println("  mov %s, -%d(%%rbp)", r, var->offset);
            }
        }
    
        // Emit code
        gen_stmt(fn->body);
        assert(top == 0);

        // The C spec defines a special rule for the main function.
        // Reaching the end of the main function is equivalent to
        // returning 0, even though the behavior is undefined for the
        // other functions. See C11 5.1.2.2.3.
        if (strcmp(fn->name, "main") == 0)
            println("  mov $0, %%rax");
    
        // Epilogue
        println(".L.return.%s:", fn->name);
        println("  mov -8(%%rbp), %%r12");
        println("  mov -16(%%rbp), %%r13");
        println("  mov -24(%%rbp), %%r14");
        println("  mov -32(%%rbp), %%r15");
        println("  mov -%d(%%rbp), %%rbx", fn->is_variadic ? 136 : 40);
        println("  mov %%rbp, %%rsp");
        println("  pop %%rbp");
        println("  ret");
    }
}

void codegen(Program *prog) {
    char **paths = get_input_files();
    for (int i = 0; paths[i]; i++)
        println("  .file %d \"%s\"", i + 1, paths[i]);

    emit_bss(prog);
    emit_data(prog);
    emit_text(prog);
}
