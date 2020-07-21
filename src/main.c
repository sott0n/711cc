#include "711cc.h"

char **include_paths;
bool opt_fpic = true;

static bool opt_E;
static bool opt_M;
static bool opt_MP;
static bool opt_S;
static bool opt_c;

static char *opt_MF;
static char *opt_MT;

static FILE *tempfile;
static char *input_path;
static char *output_path;
static char *tempfile_path;

void println(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(tempfile, fmt, ap);
    fprintf(tempfile, "\n");
}

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
    // If no output filename was specified, the output filename is made
    // by replacing ".c" with ".o" or ".s". If the input filename
    // doesn't end with ".c", we simply append ".o" or ".s".
    char *filename = basename(strdup(input_path));
    int len = strlen(filename);

    if (3 <= len && strcmp(filename + len - 2, ".c") == 0) {
        filename[len - 1] = opt_S ? 's' : 'o';
        return filename;
    }

    char *buf = calloc(1, len + 3);
    sprintf(buf, "%s.%c", filename, opt_S ? 's' : 'o');
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

        if (!strcmp(argv[i], "-M")) {
            opt_M = opt_E = true;
            continue;
        }

        if (!strcmp(argv[i], "-MD")) {
            opt_M = true;
            continue;
        }

        if (!strcmp(argv[i], "-MP")) {
            opt_MP = true;
            continue;
        }

        if (!strcmp(argv[i], "-MT")) {
            opt_MT = argv[++i];
            continue;
        }

        if (!strncmp(argv[i], "-MT", 3)) {
            opt_MT = argv[i] + 3;
            continue;
        }

        if (!strcmp(argv[i], "-MF")) {
            opt_MF = argv[++i];
            continue;
        }

        if (!strncmp(argv[i], "-MF", 3)) {
            opt_MF = argv[i] + 3;
            continue;
        }

        if (!strcmp(argv[i], "-S")) {
            opt_S = true;
            continue;
        }

        if (!strcmp(argv[i], "-c")) {
            opt_c = true;
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

// Handle -M, -MM and the like. If these options are given, the
// compiler write a list of input files to stdout (or a file if -MF is
// given) in a format that "make" command can read. This feature is
// used to automate file dependency management.
//
// You can ignore this function if you aren't sure what -M options are.
static void print_dependencies(void) {
    FILE *out;
    if (opt_MF) {
        out = fopen(opt_MF, "w");
        if (!out)
            error("-MF: cannot open %s: %s", opt_MF, strerror(errno));
    } else {
        out = stdout;
    }

    char **paths = get_input_files();
    fprintf(out, "%s:", opt_MT ? opt_MT : paths[0]);

    for (int i = 1; paths[i]; i++)
        fprintf(out, " \\\n %s", paths[i]);
    fprintf(out, "\n\n");

    if (opt_MP)
        for (int i = 1; paths[i]; i++)
            fprintf(out, "%s:\n\n", paths[i]);

    if (out != stdout)
        fclose(out);
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

static void copy_file(FILE *in, FILE *out) {
    char buf[4096];
    for (;;) {
        int nr = fread(buf, 1, sizeof(buf), in);
        if (nr == 0)
            break;
        fwrite(buf, 1, nr, out);
    }
}

static void cleanup(void) {
    if (tempfile_path)
        unlink(tempfile_path);
}

int main(int argc, char **argv) {
    init_macros();
    add_default_include_paths(argv[0]);
    parse_args(argc, argv);
    atexit(cleanup);

    // Open a temporary output file.
    tempfile_path = strdup("/tmp/711cc-XXXXXX");
    int fd = mkstemp(tempfile_path);
    if (!fd)
        error("cannot create a temporary file: %s: %s", tempfile_path, strerror(errno));
    tempfile = fdopen(fd, "w");

    // Tokenize
    Token *tok = tokenize_file(input_path);
    if (!tok)
        error("%s: %s", input_path, strerror(errno));

    // Preprocess
    tok = preprocess(tok);

    // If -M or -MD are given, print out dependency info for make command.
    if (opt_M)
        print_dependencies();

    // If -E is given, print out preprocessed C code as a result.
    if (opt_E) {
        print_tokens(tok);
        exit(0);
    }

    // Parse
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

    // If -S is given, assembly text is the final output.
    if (opt_S) {
        fseek(tempfile, 0, SEEK_SET);

        FILE *out;
        if (strcmp(output_path, "-") == 0) {
            out = stdout;
        } else {
            out = fopen(output_path, "w");
            if (!out)
                error("cannot open output file: %s: %s", output_path, strerror(errno));
        }
        copy_file(tempfile, out);
        exit(0);
    }

    // Otherwise, run the assembler to assemble our output.
    fclose(tempfile);

    pid_t pid;
    if ((pid = fork()) == 0) {
        // Child process. Run the assembler.
        execlp("as", "-c", "-o", output_path, tempfile_path, (char *)0);
        fprintf(stderr, "exec failed: as: %s", strerror(errno));
        _exit(1);
    }

    // Wait for the child process to finish.
    for (;;) {
        int status;
        int w = waitpid(pid, &status, 0);
        if (!w) {
            error("waitpid failed: %s", strerror(errno));
            exit(1);
        }
        if (WIFEXITED(status))
            break;
    }

    return 0;
}
