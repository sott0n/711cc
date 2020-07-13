// This file implements the C preprocessor.
//
// The preprocessor takes a list of tokens as an input and returns a
// new list of tokens as an output.
//
// The preprocessing language is designed in such a way that that's
// guaranteed to stop even if there is a recursive macro.
// Informally speaking, a macro is applied only once for each token.
// That is, if a macro token T appears in a result of direct or
// indirect macro expression of T, T won't be expanded any further.
// For example, if T is defined as U, and U is defined as T, then
// token T is expanded to U and then to T and the macro expansion
// stops at that point.
//
// To archive the above behavior, we attach for each token a set of
// macro names from which the token is expanded. The set is called
// "hideset". Hideset is initially empty, and every time we expand a
// macro, the macro name is added to the resulting tokens' hidesets.

#include "711cc.h"

typedef struct MacroParam MacroParam;
struct MacroParam {
    MacroParam *next;
    char *name;
};

typedef struct MacroArg MacroArg;
struct MacroArg {
    MacroArg *next;
    char *name;
    Token *tok;
};

typedef struct Macro Macro;
struct Macro {
    Macro *next;
    char *name;
    bool is_objlike;    // Object-like or function-like
    MacroParam *params;
    Token *body;
    bool deleted;
};

// `#if` can be nested, so we use a stack to manage nested `#if`s.
typedef struct CondIncl CondIncl;
struct CondIncl {
    CondIncl *next; 
    enum { IN_THEN, IN_ELIF, IN_ELSE } ctx;
    Token *tok;
    bool included;
};

typedef struct Hideset Hideset;
struct Hideset {
    Hideset *next;
    char *name;
};

static Macro *macros;
static CondIncl *cond_incl;

static Token *preprocess2(Token *tok);
static Macro *find_macro(Token *tok);

static bool is_hash(Token *tok) {
    return tok->at_bol && equal(tok, "#");
}

// Some preprocessor directives such as #include allow extraneous
// tokens befor newline. This function skips such tokens.
static Token *skip_line(Token *tok) {
    if (tok->at_bol)
        return tok;
    warn_tok(tok, "extra token");
    while (tok->at_bol)
        tok = tok->next;
    return tok;
}

static Token *copy_token(Token *tok) {
    Token *t = calloc(1, sizeof(Token));
    *t = *tok;
    t->next = NULL;
    return t;
}

static Token *new_eof(Token *tok) {
    Token *t = copy_token(tok);
    t->kind = TK_EOF;
    t->len = 0;
    return t;
}

static Hideset *new_hideset(char *name) {
    Hideset *hs = calloc(1, sizeof(Hideset));
    hs->name = name;
    return hs;
}

static Hideset *hideset_union(Hideset *hs1, Hideset *hs2) {
    Hideset head = {};
    Hideset *cur = &head;

    for (; hs1; hs1 = hs1->next)
        cur = cur->next = new_hideset(hs1->name);
    cur->next = hs2;
    return head.next;
}

static bool hideset_contains(Hideset *hs, char *s, int len) {
    for (; hs; hs = hs->next)
        if (strlen(hs->name) == len && !strncmp(hs->name, s, len))
            return true;
    return false;
}

static Hideset *hideset_intersection(Hideset *hs1, Hideset *hs2) {
    Hideset head = {};
    Hideset *cur = &head;

    for (; hs1; hs1 = hs1->next)
        if (hideset_contains(hs2, hs1->name, strlen(hs1->name)))
            cur = cur->next = new_hideset(hs1->name);
    return head.next;
}

static Token *add_hideset(Token *tok, Hideset *hs) {
    Token head = {};
    Token *cur = &head;

    for (; tok; tok = tok->next) {
        Token *t = copy_token(tok);
        t->hideset = hideset_union(t->hideset, hs);
        cur = cur->next = t;
    }
    return head.next;
}

// Append tok2 to the end of tok1.
static Token *append(Token *tok1, Token *tok2) {
    if (tok1->kind == TK_EOF)
        return tok2;

    Token head = {};
    Token *cur = &head;

    for (; tok1->kind != TK_EOF; tok1 = tok1->next)
        cur = cur->next = copy_token(tok1);
    cur->next = tok2;
    return head.next;
}

static Token *skip_cond_incl2(Token *tok) {
    while (tok->kind != TK_EOF) {
        if (is_hash(tok) &&
               (equal(tok->next, "if") || equal(tok->next, "ifdef") ||
                equal(tok->next, "ifndef"))) {
            tok = skip_cond_incl2(tok->next->next);
            continue;
        }
        if (is_hash(tok) && equal(tok->next, "endif"))
            return tok->next->next;
        tok = tok->next;
    }
    return tok;
}

// Skip until next `#else`, `#elif` or `#endif`.
// Nested `#if` and `#endif` are skipped.
static Token *skip_cond_incl(Token *tok) {
    while (tok->kind != TK_EOF) {
        if (is_hash(tok) &&
                (equal(tok->next, "if") || equal(tok->next, "ifdef") ||
                 equal(tok->next, "ifndef"))) {
            tok = skip_cond_incl2(tok->next->next);
            continue;
        }
        if (is_hash(tok) &&
                (equal(tok->next, "elif") || equal(tok->next, "else") ||
                 equal(tok->next, "endif")))
            break;
        tok = tok->next;
    }
    return tok;
}

// Double-quote a given string and returns it.
static char *quote_string(char *str) {
    int bufsize = 3;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\\' || str[i] == '"')
            bufsize++;
        bufsize++;
    }

    char *buf = calloc(1, bufsize);
    char *p = buf;
    *p++ = '"';
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\\' || str[i] == '"')
            *p++ = '\\';
        *p++ = str[i];
    }
    *p++ = '"';
    *p++ = '\0';
    return buf;
}

static Token *new_str_token(char *str, Token *tmpl) {
    char *buf = quote_string(str);
    return tokenize(tmpl->filename, tmpl->file_no, buf);
}

// Copy all tokens until the next newline, terminate them with
// an EOF token and then returns them. This function is used to
// create a new list of tokens for `#if` arguments.
static Token *copy_line(Token **rest, Token *tok) {
    Token head = {};
    Token *cur = &head;

    for (; !tok->at_bol; tok = tok->next)
        cur = cur->next = copy_token(tok);

    cur->next = new_eof(tok);
    *rest = tok;
    return head.next;
}

static Token *new_num_token(int val, Token *tmpl) {
    char *buf = calloc(1, 30);
    sprintf(buf, "%d\n", val);
    return tokenize(tmpl->filename, tmpl->file_no, buf);
}

static Token *read_const_expr(Token **rest, Token *tok) {
    tok = copy_line(rest, tok);

    Token head = {};
    Token *cur = &head;

    while (tok->kind != TK_EOF) {
        // "defined(foo)" or "defined foo" becomes "1" if macro "foo" 
        // is defined. Otherwise "0".
        if (equal(tok, "defined")) {
            Token *start = tok;
            bool has_paren = consume(&tok, tok->next, "(");

            if (tok->kind != TK_IDENT)
                error_tok(start, "macro name must be an identifier");
            Macro *m = find_macro(tok);
            tok = tok->next;

            if (has_paren)
                tok = skip(tok, ")");

            cur = cur->next = new_num_token(m ? 1 : 0, start);
            continue;
        }

        cur = cur->next = tok;
        tok = tok->next;
    }

    cur->next = tok;
    return head.next;
}

// Read and evaluate a constant expression.
static long eval_const_expr(Token **rest, Token *tok) {
    Token *expr = read_const_expr(rest, tok);
    expr = preprocess2(expr);

    // The standard requires we replace remaining non-macro
    // identifiers with "0" before evaluating a constant expression.
    for (Token *t = expr; t->kind != TK_EOF; t = t->next) {
        if (t->kind == TK_IDENT) {
            Token *next = t->next;
            *t = *new_num_token(0, t);
            t->next = next;
        }
    }

    Token *rest2;
    long val = const_expr(&rest2, expr);
    if (rest2->kind != TK_EOF)
        error_tok(rest2, "extra token");
    return val;
}

static CondIncl *push_cond_incl(Token *tok, bool included) {
    CondIncl *ci = calloc(1, sizeof(CondIncl));
    ci->next = cond_incl;
    ci->ctx = IN_THEN;
    ci->tok = tok;
    ci->included = included;
    cond_incl = ci;
    return ci;
}

static Macro *find_macro(Token *tok) {
    if (tok->kind != TK_IDENT)
        return NULL;

    for (Macro *m = macros; m; m = m->next)
        if (strlen(m->name) == tok->len && !strncmp(m->name, tok->loc, tok->len))
            return m->deleted ? NULL : m;
    return NULL;
}

static Macro *add_macro(char *name, bool is_objlike, Token *body) {
    Macro *m = calloc(1, sizeof(Macro));
    m->next = macros;
    m->name = name;
    m->is_objlike = is_objlike;
    m->body = body;
    macros = m;
    return m;
}

static MacroParam *read_macro_params(Token **rest, Token *tok) {
    MacroParam head = {};
    MacroParam *cur = &head;

    while (!equal(tok, ")")) {
        if (cur != &head)
            tok = skip(tok, ",");

        if (tok->kind != TK_IDENT)
            error_tok(tok, "expected an identifier");
        MacroParam *m = calloc(1, sizeof(MacroParam));
        m->name = strndup(tok->loc, tok->len);
        cur = cur->next = m;
        tok = tok->next;
    }
    *rest = tok->next;
    return head.next;
}

static void read_macro_definition(Token **rest, Token *tok) {
    if (tok->kind != TK_IDENT)
        error_tok(tok, "macro name must be an identifier");
    char *name = strndup(tok->loc, tok->len);
    tok = tok->next;

    if (!tok->has_space && equal(tok, "(")) {
        // Function-like macro
        MacroParam *params = read_macro_params(&tok, tok->next);
        Macro *m = add_macro(name, false, copy_line(rest, tok));
        m->params = params;
    } else {
        // Object-like macro
        add_macro(name, true, copy_line(rest, tok));
    }
}

static MacroArg *read_macro_arg_one(Token **rest, Token *tok) {
    Token head = {};
    Token *cur = &head;
    int level = 0;

    while (level > 0 || (!equal(tok, ",") && !equal(tok, ")"))) {
        if (tok->kind == TK_EOF)
            error_tok(tok, "paramater end of input");

        if (equal(tok, "("))
            level++;
        else if (equal(tok, ")"))
            level--;

        cur = cur->next = copy_token(tok);
        tok = tok->next;
    }

    cur->next = new_eof(tok);

    MacroArg *arg = calloc(1, sizeof(MacroArg));
    arg->tok = head.next;
    *rest = tok;
    return arg;
}

static MacroArg *read_macro_args(Token **rest, Token *tok, MacroParam *params) {
    Token *start = tok;
    tok = tok->next->next;

    MacroArg head = {};
    MacroArg *cur = &head;

    MacroParam *pp = params;
    for (; pp; pp = pp->next) {
        if (cur != &head)
            tok = skip(tok, ",");
        cur = cur->next = read_macro_arg_one(&tok, tok);
        cur->name = pp->name;
    }

    if (pp)
        error_tok(start, "too many arguments");
    skip(tok, ")");
    *rest = tok;
    return head.next;
}

static Token *find_arg(MacroArg *args, Token *tok) {
    for (MacroArg *ap = args; ap; ap = ap->next)
        if (tok->len == strlen(ap->name) && !strncmp(tok->loc, ap->name, tok->len))
            return ap->tok;
    return NULL;
}

// Concatenates all tokens in `tok` and returns a new string.
static char *join_tokens(Token *tok, Token *end) {
    // Compute the length of the resulting token.
    int len = 1;
    for (Token *t = tok; t != end && t->kind != TK_EOF; t = t->next) {
        if (t != tok && t->has_space)
            len++;
        len += t->len;
    }

    char *buf = calloc(1, len);

    // Copy token texts.
    int pos = 0;
    for (Token *t = tok; t != end && t->kind != TK_EOF; t = t->next) {
        if (t != tok && t->has_space)
            buf[pos++] = ' ';
        strncpy(buf + pos, t->loc, t->len);
        pos += t->len;
    }
    buf[pos] = '\0';
    return buf;
}

// Concatenates all tokens in `arg` and returns a new string token.
// This function is used for the stringizing operator (#).
static Token *stringize(Token *hash, Token *arg) {
    // Create a new string token. We neet to set some value to its
    // source location for error reporting function, so we use a macro
    // name token as a template.
    char *s = join_tokens(arg, NULL);
    return new_str_token(s, hash);
}

// Concatenate two tokens to create a new token.
static Token *paste(Token *lhs, Token *rhs) {
    // Paste the two tokens.
    char *buf = calloc(1, lhs->len + rhs->len + 1);
    sprintf(buf, "%.*s%.*s", lhs->len, lhs->loc, rhs->len, rhs->loc);

    // Tokenize the resulting string.
    Token *tok = tokenize(lhs->filename, lhs->file_no, buf);
    if (tok->next->kind != TK_EOF)
        error_tok(lhs, "pasting forms '%s', an invalid token", buf);
    return tok;
}

// Replace func-like macro paramters with given arguments.
static Token *subst(Token *tok, MacroArg *args) {
    Token head = {};
    Token *cur = &head;

    while (tok->kind != TK_EOF) {
        // "#" followed by a parameter is replaced with stringized actuals.
        if (equal(tok, "#")) {
            Token *arg = find_arg(args, tok->next);
            if (!arg)
                error_tok(tok->next, "'#' is not followed by a macro paramter");
            cur = cur->next = stringize(tok, arg);
            tok = tok->next->next;
            continue;
        }

        // Handle ## (token-pasting operator). x##y is replaced with xy.
        if (equal(tok->next, "##")) {
            Token *x = tok;
            Token *y = tok->next->next;
            Token *lhs = find_arg(args, x);
            Token *rhs = find_arg(args, y);

            // x##y becomes y if x is the empty argument list.
            if (lhs && lhs->kind == TK_EOF) {
                tok = y;
                continue;
            }

            if (lhs) {
                for (Token *t = lhs; t->kind != TK_EOF; t = t->next)
                    cur = cur->next = copy_token(t);
            } else {
                cur = cur->next = copy_token(x);
            }

            // x##y becomes x if y is the empty argument list.
            if (rhs && rhs->kind == TK_EOF) {
                tok = y->next;
                continue;
            }

            if (rhs) {
                *cur = *paste(cur, rhs);
                for (Token *t = rhs->next; t->kind != TK_EOF; t = t->next)
                    cur = cur->next = copy_token(t);
            } else {
                *cur = *paste(cur, y);
            }

            tok = y->next;
            continue;
        }

        // Handle a macro token. Macro arguments are completely macro-expanded
        // before they are substituted into a macro body.
        Token *arg = find_arg(args, tok);
        if (arg) {
            arg = preprocess2(arg);
            for (Token *t = arg; t->kind != TK_EOF; t = t->next)
                cur = cur->next = copy_token(t);
            tok = tok->next;
            continue;
        }

        // Handle a non-macro token.
        cur = cur->next = copy_token(tok);
        tok = tok->next;
        continue;
    }

    cur->next = tok;
    return head.next;
}

// If tok is a macro, expand it and return true.
// Otherwise, do nothing and return false.
static bool expand_macro(Token **rest, Token *tok) {
    if (hideset_contains(tok->hideset, tok->loc, tok->len))
        return false;

    Macro *m = find_macro(tok);
    if (!m)
        return false;

    // Object-like macro application
    if (m->is_objlike) {
        Hideset *hs = hideset_union(tok->hideset, new_hideset(m->name));
        Token *body = add_hideset(m->body, hs);
        *rest = append(body, tok->next);
        return true;
    }

    // If a funclike macro token is not followed by an argument list.
    // treat if as a normal identifier.
    if (!equal(tok->next, "("))
        return false;

    // Function-like macro application
    Token *macro_token = tok;
    MacroArg *args = read_macro_args(&tok, tok, m->params);
    Token *rparen = tok;

    // Tokens that consist a func-like macro invocation may have different
    // hidesets, and if that's the case, it's not clear what the hideset
    // for the new tokens should be. We take the intersection of the
    // macro token and the closing parenthesis and use it as a new bideset
    // as explained in the Dave Prossor's algorighm.
    Hideset *hs = hideset_intersection(macro_token->hideset, rparen->hideset);
    hs = hideset_union(hs, new_hideset(m->name));

    Token *body = subst(m->body, args);
    body = add_hideset(body, hs);
    *rest = append(body, tok->next);
    return true;
}

// Returns a new string "dir/file".
static char *join_paths(char *dir, char *file) {
    char *buf = calloc(1, strlen(dir) + strlen(file) + 2);
    sprintf(buf, "%s/%s", dir, file);
    return buf;
}

// Returns true if a given file exists.
static bool file_exists(char *path) {
    struct stat st;
    return !stat(path, &st);
}

// Read an #include argument.
static char *read_include_path(Token **rest, Token *tok) {
    // Pattern 1: #include "foo.h"
    if (tok->kind == TK_STR) {
        // A double-quoted filename for #include is a special kind of
        // token, and we don't want to interpret any escape sequence in it.
        // For example, "\f" in "C:\foo" is not a formfeed character but
        // just two non-control characters, backslash and f.
        // So we don't want to use token->contents.
        char *filename = strndup(tok->loc + 1, tok->len - 2);
        *rest = skip_line(tok->next);
        return filename;
    }

    // Pattern 2: #include <foo.h>
    if (equal(tok, "<")) {
        // Reconstruct a filename from a sequence of tokens between
        // "<" and ">"
        Token *start = tok;

        // Find closing ">".
        for (; !equal(tok, ">"); tok = tok->next)
            if (tok->kind == TK_EOF)
                error_tok(tok, "expected '>'");

        char *filename = join_tokens(start->next, tok);
        *rest = skip_line(tok->next);

        // Search a file from the include paths.
        // TODO: implement the actual include paths.
        char *path = join_paths(".", filename);
        if (!file_exists(path))
            error_tok(start, "'%s': file not found", filename);
        return path;
    }

    // Pattern 3: #include FOO
    // In this case FOO must be macro-expanded to either
    // a single string token or a sequence of "<" ... ">".
    if (tok->kind == TK_IDENT) {
        Token *tok2 = preprocess(copy_line(rest, tok));
        return read_include_path(&tok2, tok2);
    }

    error_tok(tok, "expected a filename");
}

// Visit all tokens in `tok` while evaluating preprocessing
// macros and directives.
static Token *preprocess2(Token *tok) {
    Token head = {};
    Token *cur = &head;

    while (tok->kind != TK_EOF) {
        // If it is a macro, expand it.
        if (expand_macro(&tok, tok))
            continue;

        // Pass through if it not a "#"
        if (!is_hash(tok)) {
            cur = cur->next = tok;
            tok = tok->next;
            continue;
        }

        Token *start = tok;
        tok = tok->next;

        if (equal(tok, "include")) {
            char *path = read_include_path(&tok, tok->next);
            Token *tok2 = tokenize_file(path);
            if (!tok2)
                error_tok(tok, "%s", strerror(errno));
            tok = append(tok2, tok);
            continue;
        }

        if (equal(tok, "define")) {
            read_macro_definition(&tok, tok->next);
            continue;
        }

        if (equal(tok, "undef")) {
            tok = tok->next;
            if (tok->kind != TK_IDENT)
                error_tok(tok, "macro name must be an identifier");
            char *name = strndup(tok->loc, tok->len);
            tok = skip_line(tok->next);

            Macro *m = add_macro(name, true, NULL);
            m->deleted = true;
            continue;
        }

        if (equal(tok, "if")) {
            long val = eval_const_expr(&tok, tok->next);
            push_cond_incl(start, val);
            if (!val)
                tok = skip_cond_incl(tok);
            continue;
        }

        if (equal(tok, "ifdef")) {
            bool defined = find_macro(tok->next);
            push_cond_incl(tok, defined);
            tok = skip_line(tok->next->next);
            if (!defined)
                tok = skip_cond_incl(tok);
            continue;
        }

        if (equal(tok, "ifndef")) {
            bool defined = find_macro(tok->next);
            push_cond_incl(tok, !defined);
            tok = skip_line(tok->next->next);
            if (defined)
                tok = skip_cond_incl(tok);
            continue;
        }

        if (equal(tok, "elif")) {
            if (!cond_incl || cond_incl->ctx == IN_ELSE)
                error_tok(start, "stray #elif");
            cond_incl->ctx = IN_ELIF;

            if (!cond_incl->included && eval_const_expr(&tok, tok->next))
                cond_incl->included = true;
            else
                tok = skip_cond_incl(tok);
            continue;
        }

        if (equal(tok, "else")) {
            if (!cond_incl || cond_incl->ctx == IN_ELSE)
                error_tok(start, "stray #else");
            cond_incl->ctx = IN_ELSE;
            tok = skip_line(tok->next);

            if (cond_incl->included)
                tok = skip_cond_incl(tok);
            continue;
        }

        if (equal(tok, "endif")) {
            if (!cond_incl)
                error_tok(start, "stray #endif");
            cond_incl = cond_incl->next;
            tok = skip_line(tok->next);
            continue;
        }

        // `#`-only line is legal. It's called a null directive.
        if (tok->at_bol)
            continue;

        error_tok(tok, "invalid preprocessor directive");
    }

    cur->next = tok;
    return head.next;
}

// Entry point function of the preprocessor.
Token *preprocess(Token *tok) {
    tok = preprocess2(tok);
    if (cond_incl)
        error_tok(cond_incl->tok, "unterminated conditional directive");
    convert_keywords(tok);
    return tok;
}
