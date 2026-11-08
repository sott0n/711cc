#!/bin/bash

set -u

: "${CC_UNDER_TEST:?CC_UNDER_TEST is required}"
: "${CC_LOG_DIR:?CC_LOG_DIR is required}"
: "${HOST_CC:?HOST_CC is required}"

stderr_file=$(mktemp "$CC_LOG_DIR/stderr.XXXXXX")
source_file=""
attempt_kind=link
is_compile=false

for arg in "$@"; do
    case "$arg" in
    -c)
        is_compile=true
        attempt_kind=probe
        ;;
    *.c)
        source_file=$arg
        attempt_kind=source
        ;;
    esac
done

compiler=$HOST_CC
if $is_compile; then
    compiler=$CC_UNDER_TEST
fi

"$compiler" "$@" 2>"$stderr_file"
status=$?

{
    printf '%d\t%s\t%s\t' "$status" "$attempt_kind" "$source_file"
    printf '%q ' "$compiler" "$@"
    printf '\n'
} >>"$CC_LOG_DIR/attempts.tsv"

failure_prefix=first-failure
if [ "$attempt_kind" = probe ]; then
    failure_prefix=first-probe-failure
elif [ "$attempt_kind" = link ]; then
    failure_prefix=first-link-failure
fi

if [ "$status" -ne 0 ] && [ ! -e "$CC_LOG_DIR/$failure_prefix.command" ]; then
    {
        printf '%q ' "$compiler" "$@"
        printf '\n'
    } >"$CC_LOG_DIR/$failure_prefix.command"
    cp "$stderr_file" "$CC_LOG_DIR/$failure_prefix.stderr"
    printf '%s\n' "$source_file" >"$CC_LOG_DIR/$failure_prefix.source"
fi

cat "$stderr_file" >&2
rm -f "$stderr_file"
exit "$status"
