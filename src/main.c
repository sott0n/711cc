#include "711cc.h"

FILE *output_file;
bool opt_fpic = true;

static char *input_path;
static char *output_path = "-";

static void usage(int status) {
    fprintf(stderr, "711cc [ -o <path> ] <file>\n");
    exit(status);
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

        if (argv[i][0] == '-' && argv[i][1] != '\0')
            error("unknown argument, %s", argv[i]);

        input_path = argv[i];
    }

    if (!input_path)
        error("no input files");
}

int main(int argc, char **argv) {
    parse_args(argc, argv);

    // Open the output file.
    if (strcmp(output_path, "-") == 0) {
        output_file = stdout;
    } else {
        output_file = fopen(output_path, "w");
        if (!output_file)
            error("cannot open output file: %s: %s", output_file, strerror(errno));
    }

    // Tokenize and parse
    Token *tok = tokenize_file(input_path);
    Program *prog = parse(tok);

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        // Besides local variables, callee-saved registers take 32 bytes
        // and the variable-argument save area takes 48 bytes in the stack.
        int offset = fn->is_variadic ? 128 : 32;

        for (Var *var = fn->locals; var; var = var->next) {
            offset = align_to(offset, var->align);
            offset += size_of(var->ty);
            var->offset = offset;
        }
        fn->stack_size = align_to(offset, 16);
    }

    // Emit a .file directive for the assembler
    fprintf(output_file, ".file 1 \"%s\"\n", argv[1]);

    // Traverse the AST to emit assembly
    codegen(prog);

    return 0;
}
