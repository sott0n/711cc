// This file contains a recursive descent parser for C.
//
// Most functions in this file are named after the symbols they are
// supposed to read from an input token list. For example, stmt() is
// responsible for reading a statement from a token list. The function
// then construct an AST node representing a statement.
//
// Each function conceptually returns two values, an AST node and
// remaining part of the input tokens. Since C doesn't support
// multiple return values, the remaining tokens are returned to the
// caller via a pointer argument.
//
// Input tokens are represented by a linked list. Unlike many recursive
// descent parsers, we don't have the notion of the "input token stream".
// Most parsing functions don't change the global state of the parser.
// So it is very easy to lookahead arbitrary number of tokens in this
// parser.

#include "711cc.h"

// Scope for local, global variables or typedefs
// or enum constants
typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    char *name;
    int depth;

    Var *var;
    Type *type_def;
    Type *enum_ty;
    int enum_val;
};

// Scope for struct, union or union tags
typedef struct TagScope TagScope;
struct TagScope {
    TagScope *next;
    char *name;
    int depth;
    Type *ty;
};

// Variable attributes such as typedef or extern
typedef struct {
    bool is_typedef;
    bool is_static;
    bool is_extern;
    int align;
} VarAttr;

// This struct represents a variable initializer. Since initializers
// can be nested (e.g. `int x[2][2] = {{1, 2}, {3, 4}}`), this struct
// is a tree data structure.
typedef struct Initializer Initializer;
struct Initializer {
    Type *ty;
    Token *tok;

    // If len is 0, it's a leaf node, and `expr` has an initializer
    // expression. Otherwise, `children` has child nodes.
    int len;
    Node *expr;

    // `children` may contain null pointers if elements are omitted.
    // For example, an initializer for `int x[100] = {1}` has 99 null
    // pointers at the end of `children`.
    //
    // The C spec requires that, if an initializer is given, members
    // with no initializers will automatically be initialized with
    // zeros. So null childrens are equivalent to zeros.
    Initializer **children;
};

// For local variable initializer
typedef struct InitDesg InitDesg;
struct InitDesg {
    InitDesg *next;
    int idx;
    Member *member;
    Var *var;
};

// All local variable instances created during parsing are
// accumulated to this list.
static Var *locals;

// Likewise, global variables are accumulated to this list.
static Var *globals;

// C has two block scopes; one is for variables/typedefs and
// the other is for struct/union/enum tags.
static VarScope *var_scope;
static TagScope *tag_scope;

// scope_depth is incremented by one at the beginning of a block
// scope and decremented by one at the end of a block scope.
static int scope_depth;

// Points to the function object the parser is currently parsing.
static Var *current_fn;

// Points to a node representing a switch if we are parsing
// a switch statement. Otherwise, NULL.
static Node *current_switch;

static bool is_typename(Token *tok);
static Type *typespec(Token **rest, Token *tok, VarAttr *attr);
static Type *typename(Token **rest, Token *tok);
static Type *enum_specifier(Token **rest, Token *tok);
static Type *type_suffix(Token **rest, Token *tok, Type *ty);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Node *declaration(Token **rest, Token *tok);
static Initializer *initializer(Token **rest, Token *tok, Type *ty);
static Initializer *initializer2(Token **rest, Token *tok, Type *ty);
static Node *lvar_initializer(Token **rest, Token *tok, Var *var);
static void gvar_initializer(Token **rest, Token *tok, Var *var);
static Node *compound_stmt(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static long eval(Node *node);
static long eval_addr(Node *node, Var **var);
static long eval_rval(Node *node, Var **var);
static Node *assign(Token **rest, Token *tok);
static Node *logor(Token **rest, Token *tok);
static double eval_double(Node *node);
static Node *conditional(Token **rest, Token *tok);
static Node *logand(Token **rest, Token *tok);
static Node *bitor(Token **rest, Token *tok);
static Node *bitxor(Token **rest, Token *tok);
static Node *bitand(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *shift(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *new_add(Node *lhs, Node *rhs, Token *tok);
static Node *new_sub(Node *lhs, Node *rhs, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *cast(Token **rest, Token *tok);
static Type *struct_decl(Token **rest, Token *tok);
static Type *union_decl(Token **rest, Token *tok);
static Node *postfix(Token **rest, Token *tok);
static Node *funcall(Token **rest, Token *tok, Node *node);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

static void enter_scope(void) {
    scope_depth++;
}

static void leave_scope(void) {
    scope_depth--;

    while (var_scope && var_scope->depth > scope_depth)
        var_scope = var_scope->next;

    while (tag_scope && tag_scope->depth > scope_depth)
        tag_scope = tag_scope->next;
}

// Find a variable or a typedef by name
static VarScope *find_var(Token *tok) {
    for (VarScope *sc = var_scope; sc; sc = sc->next)
        if (strlen(sc->name) == tok->len && !strncmp(tok->loc, sc->name, tok->len))
            return sc;
    return NULL;
}

static TagScope *find_tag(Token *tok) {
    for (TagScope *sc = tag_scope; sc; sc = sc->next)
        if (strlen(sc->name) == tok->len && !strncmp(tok->loc, sc->name, tok->len))
            return sc;
    return NULL;
}

static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}

static Node *new_num(long val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

static Node *new_ulong(long val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    node->ty = ty_ulong;
    return node;
}

static Node *new_var_node(Var *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

Node *new_cast(Node *expr, Type *ty) {
    add_type(expr);

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_CAST;
    node->tok = expr->tok;
    node->lhs = expr;
    node->ty = copy_type(ty);
    return node;
}

static VarScope *push_scope(char *name) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->next = var_scope;
    sc->name = name;
    sc->depth = scope_depth;
    var_scope = sc;
    return sc;
}

static Initializer *new_init(Type *ty, int len, Node *expr, Token *tok) {
    Initializer *init = calloc(1, sizeof(Initializer));
    init->ty = ty;
    init->tok = tok;
    init->len = len;
    init->expr = expr;
    if (len)
        init->children = calloc(1, sizeof(Initializer *) * len);
    return init;
} 

static Var *new_var(char *name, Type *ty) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;
    var->align = ty->align;
    push_scope(name)->var = var;
    return var;
}

static Var *new_lvar(char *name, Type *ty) {
    Var *var = new_var(name, ty);
    var->is_local = true;
    var->next = locals;
    locals = var;
    return var;
}

static Var *new_gvar(char *name, Type *ty, bool is_static, bool is_definition) {
    Var *var = new_var(name, ty);
    var->is_static = is_static;
    if (is_definition) {
        var->next = globals;
        globals = var;
    }
    return var;
}

static char *new_unique_name(void) {
    static int id = 0;
    char *buf = calloc(1, 20);
    sprintf(buf, ".L.data.%d", id++);
    return buf;
}

static Var *new_string_literal(char *p, Type *ty) {
    Var *var = new_gvar(new_unique_name(), ty, true, true);
    var->init_data = p;
    return var;
}

static char *get_ident(Token *tok) {
    if (tok->kind != TK_IDENT)
        error_tok(tok, "expected a identifier");
    return strndup(tok->loc, tok->len);
}

static Type *find_typedef(Token *tok) {
    if (tok->kind == TK_IDENT) {
        VarScope *sc = find_var(tok);
        if (sc)
            return sc->type_def;
    }
    return NULL;
}

static void push_tag_scope(Token *tok, Type *ty) {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->next = tag_scope;
    sc->name = strndup(tok->loc, tok->len);
    sc->depth = scope_depth;
    sc->ty = ty;
    tag_scope = sc;
}

// Create a node for "__func__" local variable and add that
// to the current scope.
static void add_func_ident(char *func) {
    Type *ty = array_of(ty_char, strlen(func) + 1);
    Var *var = new_string_literal(func, ty);
    push_scope("__func__")->var = var;
}

// funcdef = typespec declarator compound-stmt
static Function *funcdef(Token **rest, Token *tok) {
    locals = NULL;

    VarAttr attr = {};
    Type *ty = typespec(&tok, tok, &attr);
    ty = declarator(&tok, tok, ty);

    if (!ty->name)
        error_tok(ty->name_pos, "function name omitted");

    Function *fn = calloc(1, sizeof(Function));
    fn->name = get_ident(ty->name);
    fn->is_static = attr.is_static;
    fn->is_variadic = ty->is_variadic;

    enter_scope();
    for (Type *t = ty->params; t; t = t->next) {
        if (!t->name)
            error_tok(t->name_pos, "parameter name omitted");
        new_lvar(get_ident(t->name), t);
    }
    fn->params = locals;

    tok = skip(tok, "{");
    add_func_ident(fn->name);
    fn->body = compound_stmt(rest, tok);
    fn->locals = locals;
    leave_scope();
    return fn;
}

// typespec = typename typename* 
// typename = "void" | "_Bool" | "char" | "short" | "int" | "long"
//          | struct-decl | union-decl | typedef-name
//
// The order of typenames in a type-specifier doesn't matter. For
// example, `int long static` means the same as `static long int`.
// That can also be written as `static long` because you can omit
// `int` if `long` or `short` are specified. However, something like
// `char int` is not a valid type specifier. We have to accept only a
// limited combinations of the typenames.
//
// In this function, we count the number of occurrences of each typename
// while keeping the "current" type object that the typenames up
// until that point represent. When we reach a non-typename token,
// we returns the current type object.
static Type *typespec(Token **rest, Token *tok, VarAttr *attr) {
    // We use a single integer as counters for all typenames.
    // For example, bits 0 and 1 represents how many times we saw the
    // keyword "void" so for. With this, we can use a switch statement
    // as you can see below.
    enum {
        VOID     = 1 << 0,
        BOOL     = 1 << 2,
        CHAR     = 1 << 4,
        SHORT    = 1 << 6,
        INT      = 1 << 8,
        LONG     = 1 << 10,
        FLOAT    = 1 << 12,
        DOUBLE   = 1 << 14,
        OTHER    = 1 << 16,
        SIGNED   = 1 << 17,
        UNSIGNED = 1 << 18,
    };

    Type *ty = ty_int;
    int counter = 0;
    bool is_const = false;

    while (is_typename(tok)) {
        // Handle storage class specifiers.
        if (equal(tok, "typedef") || equal(tok, "static") || equal(tok, "extern")) {
            if (!attr)
                error_tok(tok, "storage class specifier is not allowed in this context");

            if (equal(tok, "typedef"))
                attr->is_typedef = true;
            else if (equal(tok, "static"))
                attr->is_static = true;
            else
                attr->is_extern = true;

            if (attr->is_typedef + attr->is_static + attr->is_extern > 1)
                error_tok(tok, "typedef and static may not be used together");
            tok = tok->next;
            continue;
        }

        if (consume(&tok, tok, "const")) {
            is_const = true;
            continue;
        }

        if (consume(&tok, tok, "volatile") || consume(&tok, tok, "register") ||
            consume(&tok, tok, "_Noreturn"))
            continue;

        if (equal(tok, "_Alignas")) {
            if (!attr)
                error_tok(tok, "_Alignas is not allowed in this context");
            tok = skip(tok->next, "(");

            if (is_typename(tok))
                attr->align = typename(&tok, tok)->align;
            else
                attr->align = const_expr(&tok, tok);
            tok = skip(tok, ")");
            continue;
        }

        // Handle user-defined types
        Type *ty2 = find_typedef(tok);
        if (equal(tok, "struct") || equal(tok, "union") || equal(tok, "enum") || ty2) {
            if (counter)
                break;
            
            if (equal(tok, "struct")) {
                ty = struct_decl(&tok, tok->next);
            } else if (equal(tok, "union")) {
                ty = union_decl(&tok, tok->next);
            } else if (equal(tok, "enum")) {
                ty = enum_specifier(&tok, tok->next);
            } else {
                ty = ty2;
                tok = tok->next;
            }

            counter += OTHER;
            continue;
        }

        // Handle built-in types
        if (equal(tok, "void"))
            counter += VOID;
        else if (equal(tok, "_Bool"))
            counter += BOOL;
        else if (equal(tok, "char"))
            counter += CHAR;
        else if (equal(tok, "short"))
            counter += SHORT;
        else if (equal(tok, "int"))
            counter += INT;
        else if (equal(tok, "long"))
            counter += LONG;
        else if (equal(tok, "float"))
            counter += FLOAT;
        else if (equal(tok, "double"))
            counter += DOUBLE;
        else if (equal(tok, "signed"))
            counter |= SIGNED;
        else if (equal(tok, "unsigned"))
            counter |= UNSIGNED;
        else
            error_tok(tok, "internal error");

        switch (counter) {
        case VOID:
            ty = ty_void;
            break;
        case BOOL:
            ty = ty_bool;
            break;
        case CHAR:
            ty = ty_char;
            break;
        case SIGNED + CHAR:
            ty = ty_schar;
            break;
        case UNSIGNED + CHAR:
            ty = ty_uchar;
            break;
        case SHORT:
        case SHORT + INT:
            ty = ty_short;
            break;
        case SIGNED + SHORT:
        case SIGNED + SHORT + INT:
            ty = ty_sshort;
            break;
        case UNSIGNED + SHORT:
        case UNSIGNED + SHORT + INT:
            ty = ty_ushort;
            break;
        case INT:
            ty = ty_int;
            break;
        case SIGNED:
        case SIGNED + INT:
            ty = ty_sint;
            break;
        case UNSIGNED:
        case UNSIGNED + INT:
            ty = ty_uint;
            break;
        case LONG:
        case LONG + INT:
        case LONG + LONG:
        case LONG + LONG + INT:
            ty = ty_long;
            break;
        case SIGNED + LONG:
        case SIGNED + LONG + INT:
        case SIGNED + LONG + LONG:
        case SIGNED + LONG + LONG + INT:
            ty = ty_slong;
            break; 
        case UNSIGNED + LONG:
        case UNSIGNED + LONG + INT:
        case UNSIGNED + LONG + LONG:
        case UNSIGNED + LONG + LONG + INT:
            ty = ty_ulong;
            break;
        case FLOAT:
            ty = ty_float;
            break;
        case DOUBLE:
        case LONG + DOUBLE:
            ty = ty_double;
            break;
        default:
            error_tok(tok, "invalid type");
        }

        tok = tok->next;
    }

    if (is_const) {
        ty = copy_type(ty);
        ty->is_const = true;
    }
    
    *rest = tok;
    return ty;
}

// func-params = ("void" | param ("," param)* ("," "...")?)? ")"
// param       = typespec declarator
static Type *func_params(Token **rest, Token *tok, Type *ty) {
    if (equal(tok, "void") && equal(tok->next, ")")) {
        *rest = tok->next->next;
        return func_type(ty);
    }

    Type head = {};
    Type *cur = &head;
    bool is_variadic = false;

    while (!equal(tok, ")")) {
        if (cur != &head)
            tok = skip(tok, ",");

        if (equal(tok, "...")) {
            is_variadic = true;
            tok = tok->next;
            skip(tok, ")");
            break;
        }

        Type *ty2 = typespec(&tok, tok, NULL);
        ty2 = declarator(&tok, tok, ty2);

        // "array of T" is converted to "pointer to T" only in the parameter
        // context. For example, *argv[] is converted to **argv by this.
        if (ty2->kind == TY_ARRAY) {
            Token *name = ty2->name;
            ty2 = pointer_to(ty2->base);
            ty2->name = name;
        }

        cur = cur->next = copy_type(ty2);
    }

    ty = func_type(ty);
    ty->params = head.next;
    ty->is_variadic = is_variadic;
    *rest = tok->next;
    return ty;
}

// array-dimentions = const-expr? "]" type-suffix
static Type *array_dimentions(Token **rest, Token *tok, Type *ty) {
    if (equal(tok, "]")) {
        ty = type_suffix(rest, tok->next, ty);
        return array_of(ty, -1);
    }

    int sz = const_expr(&tok, tok);
    tok = skip(tok, "]");
    ty = type_suffix(rest, tok, ty);
    return array_of(ty, sz);
}

// type-suffix = "(" func-params
//             | "[" array-dimentions
//             | ε
static Type *type_suffix(Token **rest, Token *tok, Type *ty) {
    if (equal(tok, "("))
        return func_params(rest, tok->next, ty);

    if (equal(tok, "["))
        return array_dimentions(rest, tok->next, ty);

    *rest = tok;
    return ty;
}

// pointers = ("*" "const" | "volatile" | "restrict")*)*
static Type *pointers(Token **rest, Token *tok, Type *ty) {
    while (consume(&tok, tok, "*")) {
        ty = pointer_to(ty);
        while (equal(tok, "const") || equal(tok, "volatile") ||
               equal(tok, "restrict")) {
            if (equal(tok, "const"))
                ty->is_const = true;
            tok = tok->next;
        }
    }
    *rest = tok;
    return ty;
}

// declarator = pointers ("(" declarator ")" | ident) type-suffix
static Type *declarator(Token **rest, Token *tok, Type *ty) {
    ty = pointers(&tok, tok, ty);

    if (equal(tok, "(")) {
        Type *placeholder = calloc(1, sizeof(Type));
        Type *new_ty = declarator(&tok, tok->next, placeholder);
        tok = skip(tok, ")");
        *placeholder = *type_suffix(rest, tok, ty);
        return new_ty;
    }

    Token *name = NULL;
    Token *name_pos = tok;

    if (tok->kind == TK_IDENT) {
        name = tok;
        tok = tok->next;
    }

    ty = type_suffix(rest, tok, ty);
    ty->name = name;
    ty->name_pos = name_pos;
    return ty;
}

// abstract-declarator = pointers ("(" abstract-declarator ")")? type-suffix
static Type *abstract_declarator(Token **rest, Token *tok, Type *ty) {
    ty = pointers(&tok, tok, ty);

    if (equal(tok, "(")) {
        Type *placeholder = calloc(1, sizeof(Type));
        Type *new_ty = abstract_declarator(&tok, tok->next, placeholder);
        tok = skip(tok, ")");
        *placeholder = *type_suffix(rest, tok, ty);
        return new_ty;
    }

    return type_suffix(rest, tok, ty);
}

// type-name = typespec abstract-declarator
static Type *typename(Token **rest, Token *tok) {
    Type *ty = typespec(&tok, tok, NULL);
    return abstract_declarator(rest, tok, ty);
}

static bool is_end(Token *tok) {
    return equal(tok, "}") || (equal(tok, ",") && equal(tok->next, "}"));
}

static bool consume_end(Token **rest, Token *tok) {
    if (equal(tok, "}")) {
        *rest = tok->next;
        return true;
    }

    if (equal(tok, ",") && equal(tok->next, "}")) {
        *rest = tok->next->next;
        return true;
    }

    return false;
}

// enum-specifier = ident? "{" enum-list? "}"
//                | ident ("{" enum-list? "}")?
//
// enum-list = ident ("=" num)? ("," ident ("=" num)?)* ","?
static Type *enum_specifier(Token **rest, Token *tok) {
    Type *ty = enum_type();

    // Read a struct tag.
    Token *tag = NULL;
    if (tok->kind == TK_IDENT) {
        tag = tok;
        tok = tok->next;
    }

    if (tag && !equal(tok, "{")) {
        TagScope *sc = find_tag(tag);
        if (!sc)
            error_tok(tag, "unknown enum type");
        if (sc->ty->kind != TY_ENUM)
            error_tok(tag, "not an enum tag");
        *rest = tok;
        return sc->ty;
    }

    tok = skip(tok, "{");

    // Read an enum-list.
    int i = 0;
    int val = 0;
    while (!consume_end(rest, tok)) {
        if (i++ > 0)
            tok = skip(tok, ",");

        char *name = get_ident(tok);
        tok = tok->next;

        if (equal(tok, "="))
            val = const_expr(&tok, tok->next);

        VarScope *sc = push_scope(name);
        sc->enum_ty = ty;
        sc->enum_val = val++;
    }

    if (tag)
        push_tag_scope(tag, ty);
    return ty;
}

// declaration = typespec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node *declaration(Token **rest, Token *tok) {
    VarAttr attr = {};
    Type *basety = typespec(&tok, tok, &attr);

    Node head = {};
    Node *cur = &head;
    int i = 0;

    while (!equal(tok, ";")) {
        if (i++ > 0)
            tok = skip(tok, ",");

        Type *ty = declarator(&tok, tok, basety);
        if (!ty->name)
            error_tok(ty->name_pos, "variable declared void");

        if (ty->kind == TY_VOID)
            error_tok(tok, "variable declared void");

        if (attr.is_typedef) {
            push_scope(get_ident(ty->name))->type_def = ty;
            continue;
        }

        if (attr.is_static) {
            // static local variable
            Var *var = new_gvar(new_unique_name(), ty, true, true);
            push_scope(get_ident(ty->name))->var = var;

            if (equal(tok, "="))
                gvar_initializer(&tok, tok->next, var);
            continue;
        }

        Var *var = new_lvar(get_ident(ty->name), ty);
        if (attr.align)
            var->align = attr.align;

        if (equal(tok, "=")) {
            Node *expr = lvar_initializer(&tok, tok->next, var);
            cur = cur->next = new_unary(ND_EXPR_STMT, expr, tok);
        }
    }

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    *rest = tok->next;
    return node;
}

static Token *skip_excess_elements(Token *tok) {
    while (!consume_end(&tok, tok)) {
        tok = skip(tok, ",");
        if (equal(tok, "{"))
            tok = skip_excess_elements(tok->next);
        else
            assign(&tok, tok);
    }
    return tok;
}

static Token *skip_end(Token *tok) {
    if (consume_end(&tok, tok))
        return tok;
    warn_tok(tok, "excess elements in initializer");
    return skip_excess_elements(tok);
}

static int count_array_init_elements(Token *tok, Type *ty) {
    tok = skip(tok, "{");
    int len = 0;
    while (!is_end(tok)) {
        if (len++ > 0)
            tok = skip(tok, ",");
        initializer(&tok, tok, ty->base);
    }
    return len;
}

// string-initializer = string-literal
static Initializer *string_initializer(Token **rest, Token *tok, Type *ty) {
    Initializer *init = new_init(ty, ty->array_len, NULL, tok);
    int len = (ty->array_len < tok->ty->array_len)
        ? ty->array_len : tok->ty->array_len;

    for (int i = 0; i < len; i++) {
        Node *expr = new_num(tok->str[i], tok);
        init->children[i] = new_init(ty->base, 0, expr, tok);
    }
    *rest = tok->next;
    return init;
}

// array-initializer = "{" initializer ("," initializer)* ","? "}"
static Initializer *array_initializer(Token **rest, Token *tok, Type *ty) {
    bool has_paren = consume(&tok, tok, "{");
    Initializer *init = new_init(ty, ty->array_len, NULL, tok);

    for (int i = 0; i < ty->array_len && !is_end(tok); i++) {
        if (i > 0)
            tok = skip(tok, ",");
        init->children[i] = initializer(&tok, tok, ty->base);
    }

    if (has_paren)
        tok = skip_end(tok);
    *rest = tok;
    return init;
}

// struct-initializer = "{" initializer ("," initializer)* ","? "}"
//                    | initializer ("," initializer)* ","
static Initializer *struct_initializer(Token **rest, Token *tok, Type *ty) {
    if (!equal(tok, "{")) {
        Token *tok2;
        Node *expr = assign(&tok2, tok);
        add_type(expr);
        if (expr->ty->kind == TY_STRUCT) {
            Initializer *init = new_init(ty, 0, expr, tok);
            *rest = tok2;
            return init;
        }
    }

    int len = 0;
    for (Member *mem = ty->members; mem; mem = mem->next)
        len++;

    Initializer *init = new_init(ty, len, NULL, tok);
    bool has_paren = consume(&tok, tok, "{");

    int i = 0;
    for (Member *mem = ty->members; mem && !is_end(tok); mem = mem->next, i++) {
        if (i > 0)
            tok = skip(tok, ",");
        init->children[i] = initializer(&tok, tok, mem->ty);
    }

    if (has_paren)
        tok = skip_end(tok);
    *rest = tok;
    return init;
}

static Initializer *initializer2(Token **rest, Token *tok, Type *ty) {
    if (ty->kind == TY_ARRAY && ty->base->kind == TY_CHAR && tok->kind == TK_STR)
        return string_initializer(rest, tok, ty);

    if (ty->kind == TY_ARRAY)
        return array_initializer(rest, tok, ty);

    if (ty->kind == TY_STRUCT)
        return struct_initializer(rest, tok, ty);

    Token *start = tok;
    bool has_paren = consume(&tok, tok, "{");
    Initializer *init = new_init(ty, 0, assign(&tok, tok), start);
    if (has_paren)
        tok = skip_end(tok);
    *rest = tok;
    return init;
}

// initializer = string-initializer | array-initializer | struct-initializer
//             | "{" assign "}" | assign
static Initializer *initializer(Token **rest, Token *tok, Type *ty) {
    // An array length can be omitted if an array has an initializer
    // (e.g. `int x[] = {1,2,3}`). If it's omitted, count the number
    // of initializer elements.
    if (ty->kind == TY_ARRAY && ty->size < 0) {
        int len;
        if (ty->base->kind == TY_CHAR && tok->kind == TK_STR)
            len = tok->ty->array_len;
        else
            len = count_array_init_elements(tok, ty);
        *ty = *array_of(ty->base, len);
    }

    return initializer2(rest, tok, ty);
}

Node *init_desg_expr(InitDesg *desg, Token *tok) {
    if (desg->var)
        return new_var_node(desg->var, tok);

    if (desg->member) {
        Node *node = new_unary(ND_MEMBER, init_desg_expr(desg->next, tok), tok);
        node->member = desg->member;
        return node;
    }

    Node *lhs = init_desg_expr(desg->next, tok);
    Node *rhs = new_num(desg->idx, tok);
    return new_unary(ND_DEREF, new_add(lhs, rhs, tok), tok);
}

static Node *create_lvar_init(Initializer *init, Type *ty, InitDesg *desg, Token *tok) {
    if (ty->kind == TY_ARRAY) {
        Node *node = new_node(ND_NULL_EXPR, tok);
        for (int i = 0; i < ty->array_len; i++) {
            InitDesg desg2 = {desg, i};
            Initializer *child = init ? init->children[i] : NULL;
            Node *rhs = create_lvar_init(child, ty->base, &desg2, tok);
            node = new_binary(ND_COMMA, node, rhs, tok);
        }
        return node;
    }

    if (ty->kind == TY_STRUCT && (!init || init->len)) {
        Node *node = new_node(ND_NULL_EXPR, tok);
        int i = 0;
        for (Member *mem = ty->members; mem; mem = mem->next, i++) {
            InitDesg desg2 = {desg, 0, mem};
            Initializer *child = init ? init->children[i] : NULL;
            Node *rhs = create_lvar_init(child, mem->ty, &desg2, tok);
            node = new_binary(ND_COMMA, node, rhs, tok);
        }
        return node;
    }

    Node *lhs = init_desg_expr(desg, tok);
    Node *rhs = init ? init->expr : new_num(0, tok);
    Node *expr = new_binary(ND_ASSIGN, lhs, rhs, tok);
    expr->is_init = true;
    return expr;
}

// A variable definition with an initializer is a shorthand notation
// for a variable definition followed by assignments. This function
// generates assignment expressions for an initializer. For example,
// `int x[2][2] = {{6, 7}, {8, 9}}` is converted to the following
// expressions:
//
//   x[0][0] = 6;
//   x[0][1] = 7;
//   x[1][0] = 8;
//   x[1][1] = 9;
static Node *lvar_initializer(Token **rest, Token *tok, Var *var) {
    Initializer *init = initializer(rest, tok, var->ty);
    InitDesg desg = {NULL, 0, NULL, var};
    return create_lvar_init(init, var->ty, &desg, tok);
}

static long read_buf(char *buf, int sz) {
    switch (sz) {
    case 1:
        return *buf;
    case 2:
        return *(short *)buf;
    case 4:
        return *(int *)buf;
    default:
        assert(sz == 8);
        return *(long *)buf;
    }
}

static void write_buf(char *buf, long val, int sz) {
    switch (sz) {
    case 1:
        *buf = val;
        return;
    case 2:
        *(short *)buf = val;
        return;
    case 4:
        *(int *)buf = val;
        return;
    default:
        assert(sz == 8);
        *(long *)buf = val;
        return;
    }
}

static Relocation *
write_gvar_data(Relocation *cur, Initializer *init, Type *ty, char *buf, int offset) {
    if (ty->kind == TY_ARRAY) {
        int sz = ty->base->size;
        for (int i = 0; i < ty->array_len; i++) {
            Initializer *child = init->children[i];
            if (child)
                cur = write_gvar_data(cur, child, ty->base, buf, offset + sz * i);
        }
        return cur;
    }

    if (ty->kind == TY_STRUCT) {
        int i = 0;
        for (Member *mem = ty->members; mem; mem = mem->next, i++) {
            Initializer *child = init->children[i];
            if (!child)
                continue;

            if (mem->is_bitfield) {
                char *loc = buf + offset + mem->offset;
                long val = read_buf(loc, mem->ty->size);
                long mask = (1L << mem->bit_width) - 1;
                long newval = val | ((eval(child->expr) & mask) << mem->bit_offset);
                write_buf(loc, newval, mem->ty->size);
                continue;
            }

            cur = write_gvar_data(cur, child, mem->ty, buf, offset + mem->offset);
        }
        return cur;
    }

    if (ty->kind == TY_FLOAT) {
        *(float *)(buf + offset) = eval_double(init->expr);
        return cur;
    }

    if (ty->kind == TY_DOUBLE) {
        *(double *)(buf + offset) = eval_double(init->expr);
        return cur;
    }

    if (ty->kind == TY_PTR) {
        Var *var = NULL;
        long val = eval_addr(init->expr, &var);

        if (!var) {
            write_buf(buf + offset, val, ty->size);
            return cur;
        }

        Relocation *rel = calloc(1, sizeof(Relocation));
        rel->offset = offset;
        rel->label = var->name;
        rel->addend = val;
        cur->next = rel;
        return cur->next;
    }

    write_buf(buf + offset, eval(init->expr), ty->size);
    return cur;
}

// Initializers for global variables are evaluated at compile-time and
// embedded to .data section. This function serializes Initializer
// objects to a flat byte array. It is a compile error if  an
// initializer list contains a non-constant expression.
static void gvar_initializer(Token **rest, Token *tok, Var *var) {
    Initializer *init = initializer(rest, tok, var->ty);

    Relocation head = {};
    char *buf = calloc(1, var->ty->size);
    write_gvar_data(&head, init, var->ty, buf, 0);
    var->init_data = buf;
    var->rel = head.next;
}

// Returns true if a given token represents a type.
static bool is_typename(Token *tok) {
    static char *kw[] = {
        "void", "_Bool", "char", "short", "int", "long", "float", "double",
        "struct", "union", "typedef", "enum", "static", "extern", "_Alignas",
        "signed", "unsigned", "const", "volatile", "register", "_Noreturn",
    };

    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
        if (equal(tok, kw[i]))
            return true;
    return find_typedef(tok);
}

// stmt = "return" expr? ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "switch" "(" expr ")" stmt
//      | "case" const-expr ":" stmt
//      | "default" ":" stmt
//      | "for" "(" expr-stmt expr? ";" expr? ")" stmt
//      | "do" stmt "while" "(" expr ")" ";"
//      | "while" "(" expr ")" stmt
//      | "break" ";"
//      | "continue" ";"
//      | "goto" ident ";"
//      | ident ":" stmt
//      | "{" compound-stmt
//      | expr-stmt
static Node *stmt(Token **rest, Token *tok) {
    if (equal(tok, "return")) {
        Node *node = new_node(ND_RETURN, tok); 
        if (consume(rest, tok->next, ";"))
            return node;

        Node *exp = expr(&tok, tok->next);
        *rest = skip(tok, ";");

        add_type(exp);
        node->lhs = new_cast(exp, current_fn->ty->return_ty);
        return node;
    }

    if (equal(tok, "if")) {
        Node *node = new_node(ND_IF, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        if (equal(tok, "else"))
            node->els = stmt(&tok, tok->next);
        *rest = tok;
        return node;
    }

    if (equal(tok, "switch")) {
        Node *node = new_node(ND_SWITCH, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");

        Node *sw = current_switch;
        current_switch = node;
        node->then = stmt(rest, tok);
        current_switch = sw;
        return node;
    }

    if (equal(tok, "case")) {
        if (!current_switch)
            error_tok(tok, "stray case");

        Node *node = new_node(ND_CASE, tok);
        int val = const_expr(&tok, tok->next);
        tok = skip(tok, ":");
        node->lhs = stmt(rest, tok);
        node->val = val;
        node->case_next = current_switch->case_next;
        current_switch->case_next = node;
        return node;
    }

    if (equal(tok, "default")) {
        if (!current_switch)
            error_tok(tok, "stray case");

        Node *node = new_node(ND_CASE, tok);
        tok = skip(tok->next, ":");
        node->lhs = stmt(rest, tok);
        current_switch->default_case = node;
        return node;
    }

    if (equal(tok, "for")) {
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");

        enter_scope();

        if (is_typename(tok))
            node->init = declaration(&tok, tok);
        else
            node->init = expr_stmt(&tok, tok);

        if (!equal(tok, ";"))
            node->cond = expr(&tok, tok);
        tok = skip(tok, ";");

        if (!equal(tok, ")"))
            node->inc = expr(&tok, tok);
        tok = skip(tok, ")");

        node->then = stmt(rest, tok);
        leave_scope();
        return node;
    }

    if (equal(tok, "while")) {
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(rest, tok);
        return node;
    }

    if (equal(tok, "do")) {
        Node *node = new_node(ND_DO, tok);
        node->then = stmt(&tok, tok->next);
        tok = skip(tok, "while");
        tok = skip(tok, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        *rest = skip(tok, ";");
        return node;
    }

    if (equal(tok, "break")) {
        *rest = skip(tok->next, ";");
        return new_node(ND_BREAK, tok);
    }

    if (equal(tok, "continue")) {
        *rest = skip(tok->next, ";");
        return new_node(ND_CONTINUE, tok);
    }

    if (equal(tok, "goto")) {
        Node *node = new_node(ND_GOTO, tok);
        node->label_name = get_ident(tok->next);
        *rest = skip(tok->next->next, ";");
        return node;
    }

    if (tok->kind == TK_IDENT && equal(tok->next, ":")) {
        Node *node = new_node(ND_LABEL, tok);
        node->label_name = strndup(tok->loc, tok->len);
        node->lhs = stmt(rest, tok->next->next);
        return node;
    }

    if (equal(tok, "{"))
        return compound_stmt(rest, tok->next);

    return expr_stmt(rest, tok);
}


// compound-stmt = (declaration | stmt)* "}"
static Node *compound_stmt(Token **rest, Token *tok) {
    Node *node = new_node(ND_BLOCK, tok);

    Node head = {};
    Node *cur = &head;

    enter_scope();

    while (!equal(tok, "}")) {
        if (is_typename(tok))
            cur = cur->next = declaration(&tok, tok);
        else
            cur = cur->next = stmt(&tok, tok);
        add_type(cur);
    }

    leave_scope();

    node->body = head.next;
    *rest = tok->next;
    return node;
}

// expr-stmt = expr? ";"
static Node *expr_stmt(Token **rest, Token *tok) {
    if (equal(tok, ";")) {
        Node *node = new_node(ND_BLOCK, tok);
        *rest = tok->next;
        return node;
    }

    Node *node = new_node(ND_EXPR_STMT, tok);
    node->lhs = expr(&tok, tok);
    *rest = skip(tok, ";");
    return node;
}

// expr = assign ("," expr)?
static Node *expr(Token **rest, Token *tok) {
    Node *node = assign(&tok, tok);

    if (equal(tok, ","))
        return new_binary(ND_COMMA, node, expr(rest, tok->next), tok);

    *rest = tok;
    return node;
}

// Evaluate a given node as a constant expression.
static long eval(Node *node) {
    add_type(node);

    if (is_flonum(node->ty))
        return eval_double(node);

    switch (node->kind) {
    case ND_ADD:
        return eval(node->lhs) + eval(node->rhs);
    case ND_SUB:
        return eval(node->lhs) - eval(node->rhs);
    case ND_MUL:
        return eval(node->lhs) * eval(node->rhs);
    case ND_DIV:
        if (node->ty->is_unsigned)
            return (unsigned long)eval(node->lhs) / eval(node->rhs);
        return eval(node->lhs) / eval(node->rhs);
    case ND_BITAND:
        return eval(node->lhs) & eval(node->rhs);
    case ND_BITOR:
        return eval(node->lhs) | eval(node->rhs);
    case ND_BITXOR:
        return eval(node->lhs) ^ eval(node->rhs);
    case ND_SHL:
        return eval(node->lhs) << eval(node->rhs);
    case ND_SHR:
        if (node->ty->is_unsigned && node->ty->size == 8)
            return (unsigned long)eval(node->lhs) >> eval(node->rhs);
        return eval(node->lhs) >> eval(node->rhs);
    case ND_EQ:
        return eval(node->lhs) == eval(node->rhs);
    case ND_NE:
        return eval(node->lhs) != eval(node->rhs);
    case ND_LT:
        if (node->lhs->ty->is_unsigned)
            return (unsigned long)eval(node->lhs) < eval(node->rhs);
        return eval(node->lhs) < eval(node->rhs);
    case ND_LE:
        if (node->lhs->ty->is_unsigned)
            return (unsigned long)eval(node->lhs) <= eval(node->rhs);
        return eval(node->lhs) <= eval(node->rhs);
    case ND_COND:
        return eval(node->cond) ? eval(node->then) : eval(node->els);
    case ND_COMMA:
        return eval(node->rhs);
    case ND_NOT:
        return !eval(node->lhs);
    case ND_BITNOT:
        return ~eval(node->lhs);
    case ND_LOGAND:
        return eval(node->lhs) && eval(node->rhs);
    case ND_LOGOR:
        return eval(node->lhs) || eval(node->rhs);
    case ND_CAST: {
        long val = eval(node->lhs);
        if (!is_integer(node->ty) || node->ty->size == 8)
            return val;

        switch (node->ty->size) {
        case 1:
            if (node->ty->is_unsigned)
                return (unsigned char)val;
            return (char)val;
        case 2:
            if (node->ty->is_unsigned)
                return (unsigned short)val;
            return (short)val;
        default:
            assert(node->ty->size == 4);
            if (node->ty->is_unsigned)
                return (unsigned int)val;
            return (int)val;
        }
    }
    case ND_NUM:
        return node->val;
    }

    error_tok(node->tok, "not a constant expression");
}

// Evaluate a given node as a constant expression.
//
// A constant expression is either just a number or ptr+n where ptr
// is a pointer to a global variable and n is a positive/negative
// number. The latter from is accepted only as an initialization
// expression for a global variable.
static long eval_addr(Node *node, Var **var) {
    add_type(node);

    switch (node->kind) {
    case ND_ADD:
        return eval_addr(node->lhs, var) + eval(node->rhs);
    case ND_SUB:
        return eval_addr(node->lhs, var) - eval(node->rhs);
    case ND_ADDR:
        return eval_rval(node->lhs, var);
    case ND_CAST:
        if (node->lhs->ty->base)
            return eval_addr(node->lhs, var);
        return eval(node->lhs);
    case ND_VAR:
        if (node->var->ty->kind != TY_ARRAY && node->var->ty->kind != TY_FUNC)
            error_tok(node->tok, "invalid initializer");
        *var = node->var;
        return 0;
    case ND_NUM:
        return node->val;
    }

    error_tok(node->tok, "not a constant expression");
}

static long eval_rval(Node *node, Var **var) {
    switch (node->kind) {
    case ND_VAR:
        if (node->var->is_local)
            error_tok(node->tok, "not a compile-time constant");
        *var = node->var;
        return 0;
    case ND_DEREF:
        return eval_addr(node->lhs, var);
    case ND_MEMBER:
        return eval_rval(node->lhs, var) + node->member->offset;
    }

    error_tok(node->tok, "invalid initializer");
}

long const_expr(Token **rest, Token *tok) {
    Node *node = conditional(rest, tok);
    return eval(node);
}

static double eval_double(Node *node) {
    add_type(node);

    if (is_integer(node->ty)) {
        if (node->ty->is_unsigned)
            return (unsigned long)eval(node);
        return eval(node);
    }

    switch (node->kind) {
    case ND_ADD:
        return eval_double(node->lhs) + eval_double(node->rhs);
    case ND_SUB:
        return eval_double(node->lhs) - eval_double(node->rhs);
    case ND_MUL:
        return eval_double(node->lhs) * eval_double(node->rhs);
    case ND_DIV:
        return eval_double(node->lhs) / eval_double(node->rhs);
    case ND_COND:
        return eval_double(node->cond) ? eval_double(node->then) : eval_double(node->els);
    case ND_COMMA:
        return eval_double(node->rhs);
    case ND_CAST:
        if (is_flonum(node->lhs->ty))
            return eval_double(node->lhs);
        return eval(node->lhs);
    case ND_NUM:
        return node->fval;
    }

    error_tok(node->tok, "not a constant expression");
}

// Convert `A op= B` to `tmp = &A, *tmp = *tmp op B`
// where tmp is a fresh pointer variable.
static Node *to_assign(Node *binary) {
    add_type(binary->lhs);
    add_type(binary->rhs);

    Var *var = new_lvar("", pointer_to(binary->lhs->ty));
    Token *tok = binary->tok;

    Node *expr1 = new_binary(ND_ASSIGN, new_var_node(var, tok),
                             new_unary(ND_ADDR, binary->lhs, tok), tok);

    Node *expr2 =
        new_binary(ND_ASSIGN,
                   new_unary(ND_DEREF, new_var_node(var, tok), tok),
                   new_binary(binary->kind,
                              new_unary(ND_DEREF, new_var_node(var, tok), tok),
                              binary->rhs,
                              tok),
                   tok);

    return new_binary(ND_COMMA, expr1, expr2, tok);
}

// assign    = conditional (assign-op assign)?
// assign-op = "=" | "+=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
//           | "<<=" | ">>="
static Node *assign(Token **rest, Token *tok) {
    Node *node = conditional(&tok, tok);

    if (equal(tok, "="))
        return new_binary(ND_ASSIGN, node, assign(rest, tok->next), tok);

    if (equal(tok, "+="))
        return to_assign(new_add(node, assign(rest, tok->next), tok));

    if (equal(tok, "-="))
        return to_assign(new_sub(node, assign(rest, tok->next), tok));

    if (equal(tok, "*="))
        return to_assign(new_binary(ND_MUL, node, assign(rest, tok->next), tok));

    if (equal(tok, "/="))
        return to_assign(new_binary(ND_DIV, node, assign(rest, tok->next), tok));

    if (equal(tok, "%="))
        return to_assign(new_binary(ND_MOD, node, assign(rest, tok->next), tok));

    if (equal(tok, "&="))
        return to_assign(new_binary(ND_BITAND, node, assign(rest, tok->next), tok));

    if (equal(tok, "|="))
        return to_assign(new_binary(ND_BITOR, node, assign(rest, tok->next), tok));

    if (equal(tok, "^="))
        return to_assign(new_binary(ND_BITXOR, node, assign(rest, tok->next), tok));

    if (equal(tok, "<<="))
        return to_assign(new_binary(ND_SHL, node, assign(rest, tok->next), tok));

    if (equal(tok, ">>="))
        return to_assign(new_binary(ND_SHR, node, assign(rest, tok->next), tok));

    *rest = tok;
    return node;
}

// conditional = logor ("?" expr ":" conditional)?
static Node *conditional(Token **rest, Token *tok) {
    Node *node = logor(&tok, tok);

    if (!equal(tok, "?")) {
        *rest = tok;
        return node;
    }

    Node *cond = new_node(ND_COND, tok);
    cond->cond = node;
    cond->then = expr(&tok, tok->next);
    tok = skip(tok, ":");
    cond->els = conditional(rest, tok);
    return cond;
}

// logor = logand ("||" logand)*
static Node *logor(Token **rest, Token *tok) {
    Node *node = logand(&tok, tok);
    while (equal(tok, "||")) {
        Token *start = tok;
        node = new_binary(ND_LOGOR, node, logand(&tok, tok->next), start);
    }
    *rest = tok;
    return node;
}

// logand = bitor ("&&" bitor)*
static Node *logand(Token **rest, Token *tok) {
    Node *node = bitor(&tok, tok);
    while (equal(tok, "&&")) {
        Token *start = tok;
        node = new_binary(ND_LOGAND, node, bitor(&tok, tok->next), start);
    }
    *rest = tok;
    return node;
}

// bitor = bitxor ("|" bitxor)*
static Node *bitor(Token **rest, Token *tok) {
    Node *node = bitxor(&tok, tok);
    while (equal(tok, "|")) {
        Token *start = tok;
        node = new_binary(ND_BITOR, node, bitxor(&tok, tok->next), start);
    }
    *rest = tok;
    return node;
}

// bitxor = bitand ("^" bitand)*
static Node *bitxor(Token **rest, Token *tok) {
    Node *node = bitand(&tok, tok);
    while (equal(tok, "^")) {
        Token *start = tok;
        node = new_binary(ND_BITXOR, node, bitand(&tok, tok->next), start);
    }
    *rest = tok;
    return node;
}

// bitand = equality ("&" equality)*
static Node *bitand(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    while (equal(tok, "&")) {
        Token *start = tok;
        node = new_binary(ND_BITAND, node, equality(&tok, tok->next), start);
    }
    *rest = tok;
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);

    for (;;) {
        Token *start = tok;

        if (equal(tok, "==")) {
            node = new_binary(ND_EQ, node, relational(&tok, tok->next), start);
            continue;
        }

        if (equal(tok, "!=")) {
            node = new_binary(ND_NE, node, relational(&tok, tok->next), start);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
static Node *relational(Token **rest, Token *tok) {
    Node *node = shift(&tok, tok);

    for (;;) {
        Token *start = tok;

        if (equal(tok, "<")) {
            node = new_binary(ND_LT, node, shift(&tok, tok->next), start);
            continue;
        }

        if (equal(tok, "<=")) {
            node = new_binary(ND_LE, node, shift(&tok, tok->next), start);
            continue;
        }

        if (equal(tok, ">")) {
            node = new_binary(ND_LT, shift(&tok, tok->next), node, start);
            continue;
        }

        if (equal(tok, ">=")) {
            node = new_binary(ND_LE, shift(&tok, tok->next), node, start);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// shift = add ("<<" add | ">>" add)*
static Node *shift(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);

    for (;;) {
        Token *start = tok;

        if (equal(tok, "<<")) {
            node = new_binary(ND_SHL, node, add(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, ">>")) {
            node = new_binary(ND_SHR, node, add(&tok, tok->next), start);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// In C, `+` operator is overloaded to perform the pointer arithmetic.
// If p is a pointer, p+n adds not n but sizeof(*p)*n to the value of p,
// so that p+n points to the location n elements (not bytes) ahead of p.
// In other words, we need to scale an integer value before adding to a
// pointer value. This function takes care of the scaling.
static Node *new_add(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // num + num
    if (is_numeric(lhs->ty) && is_numeric(rhs->ty))
        return new_binary(ND_ADD, lhs, rhs, tok);

    if (lhs->ty->base && rhs->ty->base)
        error_tok(tok, "invalid operands");

    // Canonicalize `num + ptr` to `ptr + num`
    if (!lhs->ty->base && rhs->ty->base) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
    return new_binary(ND_ADD, lhs, rhs, tok);
} 

// Lile `+`, `-` is overloaded for the pointer type.
static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_numeric(lhs->ty) && is_numeric(rhs->ty))
        return new_binary(ND_SUB, lhs, rhs, tok);

    // ptr - num
    if (lhs->ty->base && is_integer(rhs->ty)) {
        rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
        return new_binary(ND_SUB, lhs, rhs, tok);
    }

    // ptr - ptr, which returns how many elements are between the two.
    if (lhs->ty->base && rhs->ty->base) {
        Node *node = new_binary(ND_SUB, lhs, rhs, tok);
        return new_binary(ND_DIV, node, new_num(lhs->ty->base->size, tok), tok);
    }

    error_tok(tok, "invalid operands");
} 

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);

    for (;;) {
        Token *start = tok;

        if (equal(tok, "+")) {
            node = new_add(node, mul(&tok, tok->next), start);
            continue;
        }

        if (equal(tok, "-")) {
            node = new_sub(node, mul(&tok, tok->next), start);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// mul = cast ("*" cast | "/" cast | "%" cast)*
static Node *mul(Token **rest, Token *tok) {
    Node *node = cast(&tok, tok);

    for (;;) {
        Token *start = tok;

        if (equal(tok, "*")) {
            node = new_binary(ND_MUL, node, cast(&tok, tok->next), start);
            continue;
        }

        if (equal(tok, "/")) {
            node = new_binary(ND_DIV, node, cast(&tok, tok->next), start);
            continue;
        }

        if (equal(tok, "%")) {
            node = new_binary(ND_MOD, node, cast(&tok, tok->next), start);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// compound-literal = initializer "}"
static Node *compound_literal(Token **rest, Token *tok, Type *ty, Token *start) {
    if (scope_depth == 0) {
        Var *var = new_gvar(new_unique_name(), ty, true, true);
        gvar_initializer(rest, tok, var);
        return new_var_node(var, start);
    }

    Var *var = new_lvar(new_unique_name(), ty);
    Node *lhs = lvar_initializer(rest, tok, var);
    Node *rhs = new_var_node(var, tok);
    return new_binary(ND_COMMA, lhs, rhs, tok);
}

// cast = "(" type-name ")" "{" compound-literal
//      | "(" type-name ")" cast
//      | unary
static Node *cast(Token **rest, Token *tok) {
    if (equal(tok, "(") && is_typename(tok->next)) {
        Token *start = tok;
        Type *ty = typename(&tok, tok->next);
        tok = skip(tok, ")");

        if (equal(tok, "{"))
            return compound_literal(rest, tok, ty, start);

        Node *node = new_unary(ND_CAST, cast(rest, tok), start);
        add_type(node->lhs);
        node->ty = ty;
        return node;
    }

    return unary(rest, tok);
}

// unary = ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
//       | postfix
static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+"))
        return cast(rest, tok->next);

    if (equal(tok, "-"))
        return new_binary(ND_SUB, new_num(0, tok), cast(rest, tok->next), tok);

    if (equal(tok, "&"))
        return new_unary(ND_ADDR, cast(rest, tok->next), tok);

    if (equal(tok, "*"))
        return new_unary(ND_DEREF, cast(rest, tok->next), tok);

    if (equal(tok, "!"))
        return new_unary(ND_NOT, cast(rest, tok->next), tok);

    if (equal(tok, "~"))
        return new_unary(ND_BITNOT, cast(rest, tok->next), tok);

    // Read ++i as i+=1
    if (equal(tok, "++"))
        return to_assign(new_add(unary(rest, tok->next), new_num(1, tok), tok));
    
    // Read --i as i-=1
    if (equal(tok, "--"))
        return to_assign(new_sub(unary(rest, tok->next), new_num(1, tok), tok));

    return postfix(rest, tok);
}

// struct-members = (typespec declarator ("," declarator)* ";")*
static Member *struct_members(Token **rest, Token *tok) {
    Member head = {};
    Member *cur = &head;

    while (!equal(tok, "}")) {
        VarAttr attr = {};
        Type *basety = typespec(&tok, tok, &attr);
        int i = 0;

        // Anonymous struct member
        if (basety->kind == TY_STRUCT && consume(&tok, tok, ";")) {
            Member *mem = calloc(1, sizeof(Member));
            mem->ty = basety;
            mem->align = attr.align ? attr.align : mem->ty->align;
            cur = cur->next = mem;
            continue;
        }

        // Regular struct member
        while (!consume(&tok, tok, ";")) {
            if (i++)
                tok = skip(tok, ",");

            Member *mem = calloc(1, sizeof(Member));
            mem->ty = declarator(&tok, tok, basety);
            mem->name = mem->ty->name;
            mem->align = attr.align ? attr.align : mem->ty->align;

            if (consume(&tok, tok, ":")) {
                mem->is_bitfield = true;
                mem->bit_width = const_expr(&tok, tok);

                // Unlike other variables, bitfields are unsigned by default
                // as per the x86-64 psABI spec.
                if (!mem->ty->is_signed) {
                    mem->ty = copy_type(mem->ty);
                    mem->ty->is_unsigned = true;
                }
            }

            cur = cur->next = mem;
        }
    }

    *rest = tok->next;
    return head.next;
}

// struct-union-decl = ident? ("{" struct-members)?
static Type *struct_union_decl(Token **rest, Token *tok) {
    // Read a tag
    Token *tag = NULL;
    if (tok->kind == TK_IDENT) {
        tag = tok;
        tok = tok->next;
    }

    if (tag && !equal(tok, "{")) {
        *rest = tok;

        TagScope *sc = find_tag(tag);
        if (sc)
            return sc->ty;

        Type *ty = struct_type();
        ty->size = -1;
        push_tag_scope(tag, ty);
        return ty;
    }

    tok = skip(tok, "{");

    // Construct a struct object
    Type *ty = struct_type();
    ty->members = struct_members(rest, tok);

    if (tag) {
        // If this is a redefinition, overwrite a previous type.
        // Otherwise, register the struct type.
        TagScope *sc = find_tag(tag);
        if (sc && sc->depth == scope_depth) {
            *sc->ty = *ty;
            return sc->ty;
        }

        push_tag_scope(tag, ty);
    }

    return ty;
}

// struct-decl = struct-union-decl
static Type *struct_decl(Token **rest, Token *tok) {
    Type *ty = struct_union_decl(rest, tok);

    // Assign offset within the struct to members
    int bits = 0;

    for (Member *mem = ty->members; mem; mem = mem->next) {
        if (mem->is_bitfield && mem->bit_width == 0) {
            // Zero-width anonymous bitfield has a special meaning.
            // It affects only alignment.
            bits = align_to(bits, mem->ty->size * 8);
        } else if (mem->is_bitfield) {
            int sz = mem->ty->size;
            if (bits / (sz * 8) != (bits + mem->bit_width - 1) / (sz * 8))
                bits = align_to(bits, sz * 8);

            mem->offset = align_down(bits / 8, sz);
            mem->bit_offset = bits % (sz * 8);
            bits += mem->bit_width;
        } else {
            bits = align_to(bits, mem->align * 8);
            mem->offset = bits / 8;
            bits += mem->ty->size * 8;
        }

        if (ty->align < mem->align)
            ty->align = mem->align;
    }

    ty->size = align_to(bits, ty->align * 8) / 8;
    return ty;
}

// union-decl = struct-union-decl
static Type *union_decl(Token **rest, Token *tok) {
    Type *ty = struct_union_decl(rest, tok);

    // If union, we don't have to assign offsets because they
    // are already initialized to zero. We need to compute the
    // alignment and the size though.
    for (Member *mem = ty->members; mem; mem = mem->next) {
        if (ty->align < mem->align)
            ty->align = mem->align;
        if (ty->size < mem->ty->size)
            ty->size = mem->ty->size;
    }
    ty->size = align_to(ty->size, ty->align);
    return ty;
}

// Find a struct member by name.
static Member *get_struct_member(Type *ty, Token *tok) {
    for (Member *mem = ty->members; mem; mem = mem->next) {
        // Anonymous struct member
        if (mem->ty->kind == TY_STRUCT && !mem->name) {
            if (get_struct_member(mem->ty, tok))
                return mem;
            continue;
        }

        // Regular struct member
        if (mem->name->len == tok->len &&
                !strncmp(mem->name->loc, tok->loc, tok->len))
            return mem;
    }
    return NULL;
}

// Crete a node representing a struct member access, such as foo.bar
// where foo is a struct and bar is a member name.
//
// C has a feature called "anonynous struct" which allows a struct to
// have another unnamed struct as a member like this:
//
//   struct { struct { int a; }; int b; } x;
//
// The members of an anonymous struct belong to the other struct's
// member namespace. Therefore, in the above example, you can access
// member "a" of the anonymous struct as "x.a".
//
// This function takes care of anonymous structs.
static Node *struct_ref(Node *node, Token *tok) {
    add_type(node);
    if (node->ty->kind != TY_STRUCT)
        error_tok(node->tok, "not a struct");

    Type *ty = node->ty;

    for (;;) {
        Member *mem = get_struct_member(ty, tok);
        if (!mem)
            error_tok(tok, "no such member");
        node = new_unary(ND_MEMBER, node, tok);
        node->member = mem;
        if (mem->name)
            break;
        ty = mem->ty;
    }
    return node;
}

// Convert A++ to `tmp = &A, *tmp = *tmp + 1, *tmp - 1`
// where tmp is a fresh pointer variable.
static Node *new_inc_dec(Node *node, Token *tok, int addend) {
    add_type(node);
    Var *var = new_lvar("", pointer_to(node->ty));

    Node *expr1 = new_binary(ND_ASSIGN, new_var_node(var, tok),
                            new_unary(ND_ADDR, node, tok), tok);

    Node *expr2 =
        new_binary(ND_ASSIGN,
                   new_unary(ND_DEREF, new_var_node(var, tok), tok),
                   new_add(new_unary(ND_DEREF, new_var_node(var, tok), tok),
                           new_num(addend, tok), tok),
                   tok);

    Node *expr3 = new_add(new_unary(ND_DEREF, new_var_node(var, tok), tok),
                         new_num(-addend, tok), tok);

    return new_binary(ND_COMMA, expr1, new_binary(ND_COMMA, expr2, expr3, tok), tok);
}

// postfix = ident "(" func-args ")" postfix-tail*
//         | primary postfix-tail
//
// postfix-tail = ("[" expr "]" | "(" func-args ")"
//              | "." ident | "->" ident | "++" | "--")
static Node *postfix(Token **rest, Token *tok) {
    Node *node = primary(&tok, tok);

    for (;;) {
        if (equal(tok, "(")) {
            node = funcall(&tok, tok->next, node);
            continue;
        }

        if (equal(tok, "[")) {
            // x[y] is short for *(x+y)
            Token *start = tok;
            Node *idx = expr(&tok, tok->next);
            tok = skip(tok, "]");
            node = new_unary(ND_DEREF, new_add(node, idx, start), start);
            continue;
        }

        if (equal(tok, ".")) {
            node = struct_ref(node, tok->next);
            tok = tok->next->next;
            continue;
        }

        if (equal(tok, "->")) {
            // x->y is short for (*x).y
            node = new_unary(ND_DEREF, node, tok);
            node = struct_ref(node, tok->next);
            tok = tok->next->next;
            continue;
        }

        if (equal(tok, "++")) {
            node = new_inc_dec(node, tok, 1);
            tok = tok->next;
            continue;
        }

        if (equal(tok, "--")) {
            node = new_inc_dec(node, tok, -1);
            tok = tok->next;
            continue;
        }

        *rest = tok;
        return node;
    }
}

// funcall = (assign ("," assign)*)? ")"
//
// foo(a,b,c) is compiled to (t1=a, t2=b, t3=c, foo(t1, t2, t3))
// where t1, t2 and t3 are fresh local variables.
static Node *funcall(Token **rest, Token *tok, Node *fn) {
    add_type(fn);

    if (fn->ty->kind != TY_FUNC &&
            (fn->ty->kind != TY_PTR || fn->ty->base->kind != TY_FUNC))
        error_tok(fn->tok, "not a function");

    Node *node = new_node(ND_NULL_EXPR, tok);
    Var **args = NULL;
    int nargs = 0;
    Type *ty = (fn->ty->kind == TY_FUNC) ? fn->ty : fn->ty->base;
    Type *param_ty = ty->params;

    while (!equal(tok, ")")) {
        if (nargs)
            tok = skip(tok, ",");

        Node *arg = assign(&tok, tok);
        add_type(arg);

        if (param_ty) {
            arg = new_cast(arg, param_ty);
            param_ty = param_ty->next;
        } else if (arg->ty->kind == TY_FLOAT) {
            arg = new_cast(arg, ty_double);
        }

        Var *var = arg->ty->base
            ? new_lvar("", pointer_to(arg->ty->base))
            : new_lvar("", arg->ty);

        args = realloc(args, sizeof(*args) * (nargs + 1));
        args[nargs] = var;
        nargs++;

        Node *expr = new_binary(ND_ASSIGN, new_var_node(var, tok), arg, tok);
        node = new_binary(ND_COMMA, node, expr, tok);
    }

    *rest = skip(tok, ")");

    Node *funcall = new_unary(ND_FUNCALL, fn, tok);
    funcall->func_ty = ty;
    funcall->ty = ty->return_ty;
    funcall->args = args;
    funcall->nargs = nargs;
    return new_binary(ND_COMMA, node, funcall, tok);
}

// primary = "(" "{" stmt stmt* "}" ")"
//         | "(" expr ")"
//         | "sizeof" "(" type-name ")"
//         | "sizeof" unary
//         | "_Alignof" "(" type-name ")"
//         | ident
//         | str
//         | num
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(") && equal(tok->next, "{")) {
        // This is a GNU statement expression
        Node *node = new_node(ND_STMT_EXPR, tok);
        node->body = compound_stmt(&tok, tok->next->next)->body;
        *rest = skip(tok, ")");
        return node;
    }

    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    if (equal(tok, "sizeof") && equal(tok->next, "(") && is_typename(tok->next->next)) {
        Type *ty = typename(&tok, tok->next->next);
        *rest = skip(tok, ")");
        return new_ulong(ty->size, tok);
    }

    if (equal(tok, "sizeof")) {
        Node *node = unary(rest, tok->next);
        add_type(node);
        return new_ulong(node->ty->size, tok);
    }

    if (equal(tok, "_Alignof")) {
        tok = skip(tok->next, "(");
        Type *ty = typename(&tok, tok);
        *rest = skip(tok, ")");
        return new_ulong(ty->align, tok);
    }

    if (tok->kind == TK_IDENT) {
        // Variable or enum constant
        VarScope *sc = find_var(tok);
        *rest = tok->next;

        if (sc) {
            if (sc->var)
                return new_var_node(sc->var, tok);
            if (sc->enum_ty)
                return new_num(sc->enum_val, tok);
        }

        if (equal(tok->next, "(")) {
            warn_tok(tok, "implicit declaration of a function");
            char *name = strndup(tok->loc, tok->len);
            Var *var = new_gvar(name, func_type(ty_int), true, false);
            return new_var_node(var, tok);
        }

        error_tok(tok, "undefined variable");
    }
    
    if (tok->kind == TK_STR) {
        Var *var = new_string_literal(tok->str, tok->ty);
        *rest = tok->next;
        return new_var_node(var, tok);
    }

    if (tok->kind != TK_NUM)
        error_tok(tok, "expected expression");

    Node *node;

    if (is_flonum(tok->ty)) {
        node = new_node(ND_NUM, tok);
        node->fval = tok->fval;
    } else {
        node = new_num(tok->val, tok);
    }

    node->ty = tok->ty;
    *rest = tok->next;
    return node;
}

// program = (funcdef | global-var)*
Program *parse(Token *tok) {
    // Add built-in function type
    new_gvar("__builtin_va_start", func_type(ty_void), true, false);

    // Read source code until EOF
    Function head = {};
    Function *cur = &head;
    globals = NULL;

    while (tok->kind != TK_EOF) {
        Token *start = tok;
        VarAttr attr = {};
        Type *basety = typespec(&tok, tok, &attr);
        if (consume(&tok, tok, ";"))
            continue;
        Type *ty = declarator(&tok, tok, basety);

        // Typedef
        if (attr.is_typedef) {
            for (;;) {
                if (!ty->name)
                    error_tok(ty->name_pos, "typedef name omitted");
                push_scope(get_ident(ty->name))->type_def = ty;

                if (consume(&tok, tok, ";"))
                    break;
                tok = skip(tok, ",");
                ty = declarator(&tok, tok, basety);
            }
            continue;
        }

        // Function
        if (ty->kind == TY_FUNC) {
            current_fn = new_gvar(get_ident(ty->name), ty, attr.is_static, false);
            if (!consume(&tok, tok, ";"))
                cur = cur->next = funcdef(&tok, start);
            continue;
        }

        // Global variable
        for (;;) {
            if (!ty->name)
                error_tok(ty->name_pos, "variable name omitted");

            Var *var = new_gvar(get_ident(ty->name), ty, attr.is_static, !attr.is_extern);
            if (attr.align)
                var->align = attr.align;

            if (equal(tok, "="))
                gvar_initializer(&tok, tok->next, var);

            if (consume(&tok, tok, ";"))
                break;
            tok = skip(tok, ",");
            ty = declarator(&tok, tok, basety);
        }
    }

    Program *prog = calloc(1, sizeof(Program));
    prog->globals = globals;
    prog->fns = head.next;
    return prog;
} 
