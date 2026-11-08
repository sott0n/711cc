#!/bin/bash

set -u

: "${CC_UNDER_TEST:?CC_UNDER_TEST is required}"
: "${CC_LOG_DIR:?CC_LOG_DIR is required}"

stderr_file=$(mktemp "$CC_LOG_DIR/stderr.XXXXXX")
source_file=""
attempt_kind=probe

for arg in "$@"; do
    case "$arg" in
    *.c)
        source_file=$arg
        attempt_kind=source
        ;;
    esac
done

"$CC_UNDER_TEST" "$@" 2>"$stderr_file"
status=$?

{
    printf '%d\t%s\t%s\t' "$status" "$attempt_kind" "$source_file"
    printf '%q ' "$CC_UNDER_TEST" "$@"
    printf '\n'
} >>"$CC_LOG_DIR/attempts.tsv"

failure_prefix=first-failure
if [ "$attempt_kind" = probe ]; then
    failure_prefix=first-probe-failure
fi

if [ "$status" -ne 0 ] && [ ! -e "$CC_LOG_DIR/$failure_prefix.command" ]; then
    {
        printf '%q ' "$CC_UNDER_TEST" "$@"
        printf '\n'
    } >"$CC_LOG_DIR/$failure_prefix.command"
    cp "$stderr_file" "$CC_LOG_DIR/$failure_prefix.stderr"
    printf '%s\n' "$source_file" >"$CC_LOG_DIR/$failure_prefix.source"
fi

cat "$stderr_file" >&2
rm -f "$stderr_file"
exit "$status"
