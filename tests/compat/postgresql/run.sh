#!/bin/bash

set -euo pipefail

postgresql_tag=REL_18_6
postgresql_version=18.6
postgresql_commit=724edf9bde9d356724ad384a2e196edc3c9f80f7

script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "$script_dir/../../.." && pwd)
source_dir=${1:-}

if [ -z "$source_dir" ]; then
    echo "usage: make compat-postgresql POSTGRESQL_SOURCE_DIR=/path/to/postgres-$postgresql_tag" >&2
    exit 2
fi

if [ ! -x "$source_dir/configure" ] || [ ! -f "$source_dir/GNUmakefile.in" ]; then
    echo "not a PostgreSQL release source tree: $source_dir" >&2
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

resolved_version=$("$source_dir/configure" --version | sed -n '1s/^PostgreSQL configure //p')
if [ "$resolved_version" != "$postgresql_version" ]; then
    echo "PostgreSQL source must be version $postgresql_version" >&2
    echo "resolved version: $resolved_version" >&2
    exit 2
fi

resolved_commit=release-tarball
if [ -d "$source_dir/.git" ]; then
    resolved_commit=$(git -C "$source_dir" rev-parse "$postgresql_tag^{commit}" 2>/dev/null || true)
    if [ "$resolved_commit" != "$postgresql_commit" ]; then
        echo "PostgreSQL repository must contain $postgresql_tag at $postgresql_commit" >&2
        echo "resolved commit: ${resolved_commit:-missing}" >&2
        exit 2
    fi
fi

if [ -n "${COMPAT_OUTPUT_DIR:-}" ]; then
    output_dir=$COMPAT_OUTPUT_DIR
    if [ -e "$output_dir" ]; then
        echo "output path already exists: $output_dir" >&2
        exit 2
    fi
    mkdir -p "$output_dir"
else
    output_dir=$(mktemp -d "$repo_root/tmp-compat-postgresql-$postgresql_version.XXXXXX")
fi
output_dir=$(cd "$output_dir" && pwd)

work_dir=$(mktemp -d /tmp/711cc-compat-postgresql.XXXXXX)
keep_work_dir=${KEEP_COMPAT_WORK_DIR:-false}
server_started=false
cleanup() {
    if $server_started; then
        "$work_dir/install/bin/pg_ctl" -D "$work_dir/data" -m immediate stop >/dev/null 2>&1 || true
    fi
    if [ "$keep_work_dir" = true ]; then
        echo "preserved work directory: $work_dir" >&2
    else
        rm -rf "$work_dir"
    fi
}
trap cleanup EXIT

mkdir -p "$work_dir/source" "$work_dir/build" "$work_dir/install"
(cd "$source_dir" && tar -cf - .) | tar -x -C "$work_dir/source"
patch -d "$work_dir/source" -p1 <"$script_dir/711cc-atomics.patch"

set +e
(
    cd "$work_dir/build"
    CC="$host_cc" \
    USE_SLICING_BY_8_CRC32C=1 \
    pgac_cv__builtin_constant_p=no \
    pgac_cv__builtin_op_overflow=no \
    pgac_cv__builtin_unreachable=no \
    pgac_cv__builtin_bswap16=no \
    pgac_cv__builtin_bswap32=no \
    pgac_cv__builtin_bswap64=no \
    pgac_cv__builtin_clz=no \
    pgac_cv__builtin_ctz=no \
    pgac_cv__builtin_popcount=no \
    pgac_cv__builtin_frame_address=no \
    pgac_cv__128bit_int_bug=broken \
    pgac_cv__get_cpuid=no \
    pgac_cv__cpuid=no \
    pgac_cv__get_cpuid_count=no \
    pgac_cv__cpuidex=no \
    pgac_cv_gcc_sync_char_tas=no \
    pgac_cv_gcc_sync_int32_tas=no \
    pgac_cv_gcc_sync_int32_cas=no \
    pgac_cv_gcc_sync_int64_cas=no \
    pgac_cv_gcc_atomic_int32_cas=yes \
    pgac_cv_gcc_atomic_int64_cas=yes \
    pgac_cv_xsave_intrinsics=no \
    pgac_cv_avx512_pclmul_intrinsics=no \
    pgac_cv_avx512_popcnt_intrinsics=no \
    "$work_dir/source/configure" \
        --prefix="$work_dir/install" \
        --without-icu \
        --without-readline \
        --without-zlib
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
    env -u MAKELEVEL -u MAKEFLAGS \
        make -C "$work_dir/build" -j1 world-bin install-world-bin \
        CC="$script_dir/cc-wrapper.sh" \
        2>&1 | tee "$output_dir/build.log"
    build_status=${PIPESTATUS[0]}
    set -e
fi

if [ "$build_status" = 0 ]; then
    set +e
    (
        set -e
        bindir="$work_dir/install/bin"
        socket_dir="$work_dir/socket"
        mkdir -p "$socket_dir"
        trap 'if $server_started; then "$bindir/pg_ctl" -D "$work_dir/data" -m immediate stop >/dev/null 2>&1 || true; fi' EXIT

        echo '$ postgres --version'
        version=$($bindir/postgres --version)
        echo "$version"
        test "$version" = "postgres (PostgreSQL) $postgresql_version"

        echo '$ initdb'
        "$bindir/initdb" -D "$work_dir/data" --no-locale --encoding=UTF8 --auth=trust

        echo '$ pg_ctl start'
        "$bindir/pg_ctl" -D "$work_dir/data" \
            -o "-F -k $socket_dir -p 55432 -c listen_addresses=" -w start
        server_started=true

        echo '$ psql -c SELECT 6 * 7'
        answer=$($bindir/psql -h "$socket_dir" -p 55432 -d postgres \
            -Atqc 'SELECT 6 * 7')
        echo "$answer"
        test "$answer" = 42

        "$bindir/pg_ctl" -D "$work_dir/data" -m fast -w stop
        server_started=false
        trap - EXIT
        echo "PostgreSQL runtime smoke test passed"
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
    echo "PostgreSQL compatibility baseline"
    echo "tag: $postgresql_tag"
    echo "source revision: $resolved_commit"
    echo "target: world-bin install-world-bin"
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
    if [ "$keep_work_dir" = true ]; then
        echo "work directory: $work_dir"
    fi
} | tee "$output_dir/summary.txt"

if [ "$configure_status" -ne 0 ]; then
    exit "$configure_status"
fi
if [ "$build_status" != 0 ]; then
    exit "$build_status"
fi
exit "$runtime_status"
