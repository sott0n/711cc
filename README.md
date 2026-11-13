# 711cc C Compiler

711cc is a small, self-hosting C compiler written to explore how C compilers
work. It has hand-written preprocessing and parsing stages and can emit code
for x86-64 and RISC-V 64-bit targets, plus an experimental AArch64 backend.

## Status

- The x86-64 backend is the primary target and can build 711cc itself.
- The RISC-V backend can be assembled with a cross toolchain and tested with
  QEMU. Variadic functions are not yet supported on RISC-V.
- The AArch64 backend is experimental. It supports integer and pointer
  expressions, local and global variables, arrays, scalar struct-member
  access, assignments, control flow, and integer function calls, including
  stack-passed arguments. Floating-point and aggregate struct values,
  bit-fields, and variadic definitions are not yet supported.
- The frontend supports C11 features plus a growing set of GNU extensions.
- The x86-64 backend can compile and run pinned releases of Git, CPython, and
  PostgreSQL. 711cc compiles their C translation units while the system C
  compiler handles configuration and linking:
  - Git v2.56.0: builds the `git` executable and exercises a repository
    workflow.
  - CPython v3.13.0: builds the `python` executable and runs Python code.
  - PostgreSQL 18.6: builds and installs `world-bin`, initializes a database
    cluster, starts the server, and executes a SQL query.

711cc is an educational compiler, not a drop-in replacement for a production
C toolchain.

## Build

A C11 compiler, GNU make, an assembler, and a system linker are required.

```sh
make
```

To exercise self-hosting, build stage 2 with the host-built 711cc and then
build stage 3 with stage 2:

```sh
make 711cc-stage2
make 711cc-stage3
```

## Usage

Compile a C source file to an x86-64 object file, link it with the system
compiler, and run it:

```sh
./711cc -c -o hello.o hello.c
cc -o hello hello.o
./hello
```

Use `-S` to stop after assembly generation:

```sh
./711cc -S -o hello.s hello.c
```

Select the RISC-V backend explicitly:

```sh
./711cc --feature=riscv64 -S -o hello.s hello.c
```

Select the experimental AArch64 backend explicitly:

```sh
./711cc --feature=aarch64 -S -o hello.s hello.c
```

### Options

| Option | Description |
| --- | --- |
| `--help` | Show command-line help |
| `--feature=x86_64` | Select the x86-64 backend (default) |
| `--feature=riscv64` | Select the RISC-V 64-bit backend |
| `--feature=aarch64` | Select the experimental AArch64 backend |
| `-o FILE` | Set the output path |
| `-c` | Compile to an object file |
| `-S` | Emit assembly instead of an object file |
| `-E` | Print preprocessed source |
| `-Ipath` | Add an include search path |
| `-Dname[=value]` | Define a preprocessor macro |
| `-fpic`, `-fPIC` | Emit position-independent references (default) |
| `-fno-pic`, `-fno-PIC` | Emit non-position-independent references |

The dependency-generation options `-M`, `-MD`, `-MP`, `-MT`, and `-MF` are
also supported.

## Tests

Run the native x86-64 tests:

```sh
make test
```

Other useful targets are:

```sh
make test-nopic   # static, non-PIC x86-64 test
make test-stage2  # run tests with the self-hosted compiler
make test-stage3  # verify that stage 2 and stage 3 are identical
make test-riscv   # assemble for RISC-V and execute with QEMU
make test-aarch64-smoke # run the experimental AArch64 smoke test with QEMU
make test-aarch64-validation # check AArch64 unsupported-feature diagnostics
make test-all     # run all of the above
```

The RISC-V test requires a cross compiler and QEMU user-mode emulator. On
Debian or Ubuntu:

```sh
sudo apt-get install gcc-riscv64-linux-gnu qemu-user
```

The AArch64 smoke test similarly requires `gcc-aarch64-linux-gnu` and
`qemu-user`. Override `AARCH64_CC`, `AARCH64_CFLAGS`, or `AARCH64_QEMU` to
use another cross toolchain or emulator.

Override `RISCV_CC`, `RISCV_CFLAGS`, or `RISCV_QEMU` when the tools are not in
`PATH` or require a custom sysroot:

```sh
make test-riscv \
  RISCV_CC=/path/to/riscv64-linux-gnu-gcc \
  RISCV_CFLAGS=--sysroot=/path/to/sysroot \
  RISCV_QEMU=/path/to/qemu-riscv64
```

Pinned external-project compatibility tests cover Git v2.56.0, CPython
v3.13.0, and PostgreSQL 18.6. Each harness builds its target with 711cc and
runs a runtime smoke test. Setup instructions and details are documented in
[tests/compat/README.md](tests/compat/README.md).

## Examples

Programs such as `cat` and Fibonacci are available in the
[examples](examples) directory. For example:

```sh
./711cc -c -o examples/cat.o examples/cat.c
cc -o cat examples/cat.o
./cat examples/fib.c
```

## Roadmap

- Expand real-world compatibility beyond Git, CPython, and PostgreSQL
- Add an intermediate representation and optimization passes
- Improve validation and diagnostics
- Complete RISC-V ABI support, including variadic functions
- Explore additional backends such as WebAssembly

## License

711cc is distributed under the [MIT License](LICENSE).
