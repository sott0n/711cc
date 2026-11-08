# External project compatibility tests

These tests measure how far 711cc can compile pinned versions of external
projects. Their source trees and generated results are kept outside version
control.

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
