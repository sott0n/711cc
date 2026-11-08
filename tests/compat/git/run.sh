#!/bin/bash

set -euo pipefail

git_tag=v2.56.0
git_commit=a018953688f1b10bddf91bff8747068f5f4746a4

script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "$script_dir/../../.." && pwd)
source_dir=${1:-}

if [ -z "$source_dir" ]; then
    echo "usage: make compat-git GIT_SOURCE_DIR=/path/to/git-$git_tag" >&2
    exit 2
fi

if [ ! -d "$source_dir/.git" ]; then
    echo "not a Git source repository: $source_dir" >&2
    exit 2
fi

compiler="$repo_root/711cc"
if [ ! -x "$compiler" ]; then
    echo "compiler not found: $compiler" >&2
    exit 2
fi

host_cc=${HOST_CC:-cc}
if ! command -v "$host_cc" >/dev/null 2>&1; then
    echo "host linker driver not found: $host_cc" >&2
    exit 2
fi

resolved_commit=$(git -C "$source_dir" rev-parse "$git_tag^{commit}" 2>/dev/null || true)
if [ "$resolved_commit" != "$git_commit" ]; then
    echo "Git source must contain $git_tag at $git_commit" >&2
    echo "resolved commit: ${resolved_commit:-missing}" >&2
    exit 2
fi

if [ -n "${COMPAT_OUTPUT_DIR:-}" ]; then
    output_dir=$COMPAT_OUTPUT_DIR
    if [ -e "$output_dir" ]; then
        echo "output path already exists: $output_dir" >&2
        exit 2
    fi
    mkdir -p "$output_dir"
else
    output_dir=$(mktemp -d "$repo_root/tmp-compat-git-$git_tag.XXXXXX")
fi
output_dir=$(cd "$output_dir" && pwd)

work_dir=$(mktemp -d /tmp/711cc-compat-git.XXXXXX)
cleanup() {
    rm -rf "$work_dir"
}
trap cleanup EXIT

git -C "$source_dir" archive "$git_tag" | tar -x -C "$work_dir"

export CC_UNDER_TEST=$compiler
export CC_LOG_DIR=$output_dir
export HOST_CC=$host_cc
: >"$output_dir/attempts.tsv"

set +e
make -C "$work_dir" -j1 git \
    V=1 \
    CC="$script_dir/cc-wrapper.sh" \
    NO_CURL=YesPlease \
    NO_EXPAT=YesPlease \
    NO_GETTEXT=YesPlease \
    NO_ICONV=YesPlease \
    NO_OPENSSL=YesPlease \
    NO_PERL=YesPlease \
    NO_PTHREADS=YesPlease \
    NO_PYTHON=YesPlease \
    NO_RUST=YesPlease \
    NO_TCLTK=YesPlease \
    2>&1 | tee "$output_dir/build.log"
build_status=${PIPESTATUS[0]}
set -e

runtime_status=skipped
if [ "$build_status" -eq 0 ]; then
    smoke_dir="$work_dir/711cc-smoke"
    mkdir -p "$smoke_dir/home" "$smoke_dir/templates"

    set +e
    (
        set -e
        export PATH="$work_dir:/usr/bin:/bin"
        export HOME="$smoke_dir/home"
        export GIT_CONFIG_NOSYSTEM=1
        export GIT_CONFIG_GLOBAL=/dev/null
        export GIT_TEMPLATE_DIR="$smoke_dir/templates"

        git_bin="$work_dir/git"
        test_repo="$smoke_dir/repository"

        echo '$ git --version'
        version=$("$git_bin" --version)
        echo "$version"
        test "$version" = "git version 2.56.0"

        echo '$ git init --initial-branch=main repository'
        "$git_bin" init --initial-branch=main "$test_repo"
        "$git_bin" -C "$test_repo" config user.name "711cc compatibility test"
        "$git_bin" -C "$test_repo" config user.email "711cc@example.invalid"

        echo '$ git commit --allow-empty -m smoke-test'
        "$git_bin" -C "$test_repo" commit --allow-empty -m smoke-test

        echo '$ git rev-parse HEAD'
        head=$("$git_bin" -C "$test_repo" rev-parse HEAD)
        echo "$head"
        test "${#head}" -eq 40

        echo '$ git log -1 --format=%s'
        subject=$("$git_bin" -C "$test_repo" log -1 --format=%s)
        echo "$subject"
        test "$subject" = smoke-test

        echo '$ git status --porcelain'
        status=$("$git_bin" -C "$test_repo" status --porcelain)
        test -z "$status"

        echo '$ git fsck --full'
        "$git_bin" -C "$test_repo" fsck --full
        echo "Git runtime smoke test passed"
    ) 2>&1 | tee "$output_dir/runtime.log"
    runtime_status=${PIPESTATUS[0]}
    set -e
fi

probes=$(awk -F '\t' '$2 == "probe" { count++ } END { print count + 0 }' "$output_dir/attempts.tsv")
probe_failures=$(awk -F '\t' '$2 == "probe" && $1 != 0 { count++ } END { print count + 0 }' "$output_dir/attempts.tsv")
attempted=$(awk -F '\t' '$2 == "source" { count++ } END { print count + 0 }' "$output_dir/attempts.tsv")
succeeded=$(awk -F '\t' '$2 == "source" && $1 == 0 { count++ } END { print count + 0 }' "$output_dir/attempts.tsv")
failed=$(awk -F '\t' '$2 == "source" && $1 != 0 { count++ } END { print count + 0 }' "$output_dir/attempts.tsv")
links=$(awk -F '\t' '$2 == "link" { count++ } END { print count + 0 }' "$output_dir/attempts.tsv")
link_failures=$(awk -F '\t' '$2 == "link" && $1 != 0 { count++ } END { print count + 0 }' "$output_dir/attempts.tsv")
first_failure=none
if [ -f "$output_dir/first-failure.source" ]; then
    first_failure=$(cat "$output_dir/first-failure.source")
fi

{
    echo "Git compatibility baseline"
    echo "tag: $git_tag"
    echo "commit: $git_commit"
    echo "target: git"
    echo "compiler: $compiler"
    echo "linker driver: $host_cc"
    echo "probes: $probes"
    echo "probe failures: $probe_failures"
    echo "source files attempted: $attempted"
    echo "source files succeeded: $succeeded"
    echo "source files failed: $failed"
    echo "link attempts: $links"
    echo "link failures: $link_failures"
    echo "first failure: $first_failure"
    echo "build status: $build_status"
    echo "runtime status: $runtime_status"
    echo "output: $output_dir"
} | tee "$output_dir/summary.txt"

if [ "$build_status" -ne 0 ]; then
    exit "$build_status"
fi
exit "$runtime_status"
