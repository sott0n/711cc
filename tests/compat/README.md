# External project compatibility tests

These tests measure how far 711cc can compile pinned versions of external
projects. Their source trees and generated results are kept outside version
control.

## Git

The Git harness targets the core objects of Git v2.56.0 at commit
`a018953688f1b10bddf91bff8747068f5f4746a4`.

Clone the pinned version outside this repository and run the compatibility
target:

```shell
$ git clone --depth 1 --branch v2.56.0 https://github.com/git/git.git /tmp/git-v2.56.0
$ make compat-git GIT_SOURCE_DIR=/tmp/git-v2.56.0
```

The harness verifies the pinned commit and builds from a temporary source tree.
It disables optional Git dependencies so that the result focuses on C frontend
compatibility.

Each run writes the following files to a new `tmp-compat-git-*` directory:

- `summary.txt`: compiler probe and source compilation counts
- `build.log`: complete Git build output
- `attempts.tsv`: every compiler invocation and its status
- `first-probe-failure.*`: the first failed compiler capability probe
- `first-failure.*`: the first failed source compilation
