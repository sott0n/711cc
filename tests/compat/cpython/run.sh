#!/bin/bash

set -euo pipefail

cpython_tag=v3.13.0
cpython_commit=60403a5409ff2c3f3b07dd2ca91a7a3e096839c7

script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "$script_dir/../../.." && pwd)
source_dir=${1:-}

if [ -z "$source_dir" ]; then
    echo "usage: make compat-cpython CPYTHON_SOURCE_DIR=/path/to/cpython-$cpython_tag" >&2
    exit 2
fi

if [ ! -d "$source_dir/.git" ]; then
    echo "not a CPython source repository: $source_dir" >&2
    exit 2
fi

compiler="$repo_root/711cc"
if [ ! -x "$compiler" ]; then
    echo "compiler not found: $compiler" >&2
    exit 2
fi

host_cc=${HOST_CC:-cc}
if ! command -v "$host_cc" >/dev/null 2>&1; then
    echo "host compiler and linker driver not found: $host_cc" >&2
    exit 2
fi

resolved_commit=$(git -C "$source_dir" rev-parse "$cpython_tag^{commit}" 2>/dev/null || true)
if [ "$resolved_commit" != "$cpython_commit" ]; then
    echo "CPython source must contain $cpython_tag at $cpython_commit" >&2
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
    output_dir=$(mktemp -d "$repo_root/tmp-compat-cpython-$cpython_tag.XXXXXX")
fi
output_dir=$(cd "$output_dir" && pwd)

work_dir=$(mktemp -d /tmp/711cc-compat-cpython.XXXXXX)
cleanup() {
    rm -rf "$work_dir"
}
trap cleanup EXIT

mkdir -p "$work_dir/source" "$work_dir/build"
git -C "$source_dir" archive "$cpython_tag" | tar -x -C "$work_dir/source"

set +e
(
    cd "$work_dir/build"
    CC="$host_cc" "$work_dir/source/configure" \
        --disable-test-modules \
        --without-ensurepip \
        --without-mimalloc
) 2>&1 | tee "$output_dir/configure.log"
configure_status=${PIPESTATUS[0]}
set -e

export CC_UNDER_TEST=$compiler
export CC_LOG_DIR=$output_dir
export HOST_CC=$host_cc
: >"$output_dir/attempts.tsv"

build_status=skipped
runtime_status=skipped
if [ "$configure_status" -eq 0 ]; then
    set +e
    make -C "$work_dir/build" -j1 python \
        CC="$script_dir/cc-wrapper.sh" \
        LINKCC="$host_cc" \
        LDSHARED="$host_cc -shared" \
        BLDSHARED="$host_cc -shared" \
        2>&1 | tee "$output_dir/build.log"
    build_status=${PIPESTATUS[0]}
    set -e
fi

if [ "$build_status" = 0 ]; then
    set +e
    (
        set -e
        python_bin="$work_dir/build/python"
        echo '$ python --version'
        version=$($python_bin --version)
        echo "$version"
        test "$version" = "Python 3.13.0"

        echo '$ python -c "print(6 * 7)"'
        answer=$($python_bin -c 'print(6 * 7)')
        echo "$answer"
        test "$answer" = 42
        echo "CPython runtime smoke test passed"
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
    echo "CPython compatibility baseline"
    echo "tag: $cpython_tag"
    echo "commit: $cpython_commit"
    echo "target: python"
    echo "compiler: $compiler"
    echo "host compiler and linker driver: $host_cc"
    echo "configure status: $configure_status"
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

if [ "$configure_status" -ne 0 ]; then
    exit "$configure_status"
fi
if [ "$build_status" != 0 ]; then
    exit "$build_status"
fi
exit "$runtime_status"
