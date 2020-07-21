#include "711cc.h"

FILE *output_file;
char **include_paths;
bool opt_E;
bool opt_fpic = true;

static char *input_path;
static char *output_path;

static void usage(int status) {
    fprintf(stderr, "711cc [ -o <path> ] <file>\n");
    exit(status);
}

static void add_include_path(char *path) {
    static int len = 2;
    include_paths = realloc(include_paths, sizeof(char *) * len);
    include_paths[len - 2] = path;
    include_paths[len - 1] = NULL;
    len++;
}

static void add_default_include_paths(char *argv0) {
    // We expect that this compiler's specific include files
    // are installed to ./include relative to argv[0].
    char *buf = calloc(1, strlen(argv0) + 10);
    sprintf(buf, "%s/include", dirname(strdup(argv0)));
    add_include_path(buf);

    // Add standard include paths.
    add_include_path("/usr/local/include");
    add_include_path("/usr/include/x86_64-linux-gnu");
    add_include_path("/usr/include");
}

static void define(char *str) {
    char *eq = strchr(str, '=');
    if (eq)
        define_macro(strndup(str, eq - str), eq + 1);
    else
        define_macro(str, "");
}

static char *get_output_filename() {
    // If no output filename was specified, the output filename
    // is made by replacing ".c" with ".s". If the input filename
    // doesn't end with ".c", we simply append ".s".
    char *filename = basename(strdup(input_path));
    int len = strlen(filename);

    if (3 <= len && strcmp(filename + len - 2, ".c") == 0) {
        filename[len - 1] = 's';
        return filename;
    }

    char *buf = calloc(1, len + 3);
    sprintf(buf, "%s.s", filename);
    return buf;
}

static void parse_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help"))
            usage(0);

        if (!strcmp(argv[i], "-o")) {
            if (!argv[++i])
                usage(1);
            output_path = argv[i];
            continue;
        }

        if (!strncmp(argv[i], "-o", 2)) {
            output_path = argv[i] + 2;
            continue;
        }

        if (!strcmp(argv[i], "-fpic") || !strcmp(argv[i], "-fPIC")) {
            opt_fpic = true;
            continue;
        }

        if (!strcmp(argv[i], "-fno-pic") || !strcmp(argv[i], "-fno-PIC")) {
            opt_fpic = false;
            continue;
        }

        if (!strcmp(argv[i], "-E")) {
            opt_E = true;
            continue;
        }

        if (!strncmp(argv[i], "-I", 2)) {
            add_include_path(argv[i] + 2);
            continue;
        }

        if (!strcmp(argv[i], "-D")) {
            if (!argv[++i])
                usage(1);
            define(argv[i]);
            continue;
        }

        if (!strncmp(argv[i], "-D", 2)) {
            define(argv[i] + 2);
            continue;
        }

        if (argv[i][0] == '-' && argv[i][1] != '\0')
            error("unknown argument, %s", argv[i]);

        input_path = argv[i];
    }

    if (!input_path)
        error("no input files");

    if (!output_path)
        output_path = get_output_filename();
}

// Print tokens to stdout. Used for -E.
static void print_tokens(Token *tok) {
    int line = 1;
    for (; tok->kind != TK_EOF; tok = tok->next) {
        if (line > 1 && tok->at_bol)
            printf("\n");
        if (tok->has_space && !tok->at_bol)
            printf(" ");
        printf("%.*s", tok->len, tok->loc);
        line++;
    }
    printf("\n");
}

int main(int argc, char **argv) {
    add_default_include_paths(argv[0]);
    parse_args(argc, argv);

    // Open the output file.
    if (strcmp(output_path, "-") == 0) {
        output_file = stdout;
    } else {
        output_file = fopen(output_path, "w");
        if (!output_file)
            error("cannot open output file: %s: %s", output_file, strerror(errno));
    }

    // Tokenize
    Token *tok = tokenize_file(input_path);
    if (!tok)
        error("%s: %s", input_path, strerror(errno));

    // Preprocess
    tok = preprocess(tok);

    // If -E is given, print out preprocessed C code as a result.
    if (opt_E) {
        print_tokens(tok);
        exit(0);
    }

    // Pares
    Program *prog = parse(tok);

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        // Besides local variables, callee-saved registers take 32 bytes
        // and the variable-argument save area takes 48 bytes in the stack.
        int offset = fn->is_variadic ? 128 : 32;

        for (Var *var = fn->locals; var; var = var->next) {
            offset = align_to(offset, var->align);
            offset += var->ty->size;
            var->offset = offset;
        }
        fn->stack_size = align_to(offset, 16);
    }

    // Traverse the AST to emit assembly
    codegen(prog);

    return 0;
}
