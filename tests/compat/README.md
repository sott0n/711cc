# External project compatibility tests

These tests measure how far 711cc can compile pinned versions of external
projects. Their source trees and generated results are kept outside version
control.

## CPython

The CPython harness targets the `python` executable from CPython v3.13.0 at
commit `60403a5409ff2c3f3b07dd2ca91a7a3e096839c7`.

Clone the pinned version outside this repository and run the compatibility
target:

```shell
$ git clone --depth 1 --branch v3.13.0 https://github.com/python/cpython.git /tmp/cpython-v3.13.0
$ make compat-cpython CPYTHON_SOURCE_DIR=/tmp/cpython-v3.13.0
```

The harness configures CPython with the system C compiler, then uses 711cc for
each C compilation and the system compiler as the linker driver. It currently
records the first unsupported source construct and exits unsuccessfully until
the `python` target and its runtime smoke test pass. The build selects
CPython's GCC-compatible atomic builtin implementation, which 711cc lowers to
x86-64 atomic instructions.

Each run writes `summary.txt`, `configure.log`, `build.log`, `attempts.tsv`,
and first-failure details to a new `tmp-compat-cpython-*` directory. Once the
build succeeds, it also writes `runtime.log` after checking the version and a
small Python expression.

## Git

The Git harness targets the Git executable from Git v2.56.0 at commit
`a018953688f1b10bddf91bff8747068f5f4746a4`.

Clone the pinned version outside this repository and run the compatibility
target:

```shell
$ git clone --depth 1 --branch v2.56.0 https://github.com/git/git.git /tmp/git-v2.56.0
$ make compat-git GIT_SOURCE_DIR=/tmp/git-v2.56.0
```

The harness verifies the pinned commit and builds from a temporary source tree.
711cc compiles every C source file and the system C compiler is used only as
the final linker driver. Optional Git dependencies are disabled so that the
result focuses on C frontend compatibility.

After linking, the harness runs the generated Git executable in an isolated
repository. It verifies the version and exercises `init`, `config`, `commit`,
`rev-parse`, `log`, `status`, and `fsck`.

Each run writes the following files to a new `tmp-compat-git-*` directory:

- `summary.txt`: compiler probe and source compilation counts
- `build.log`: complete Git build output
- `runtime.log`: commands and output from the runtime smoke test
- `attempts.tsv`: every compiler invocation and its status
- `first-probe-failure.*`: the first failed compiler capability probe
- `first-failure.*`: the first failed source compilation
- `first-link-failure.*`: the first failed link invocation
