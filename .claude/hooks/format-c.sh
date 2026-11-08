#!/usr/bin/env bash
# PostToolUse hook: run clang-format on the C file Claude just wrote.
# Parses stdin with sed because jq is not guaranteed on every platform.

command -v clang-format >/dev/null 2>&1 || exit 0

file=$(sed -n 's/.*"file_path"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -n 1)
file=$(printf '%s' "$file" | tr -s '\\' '/')

case "$file" in
    */stb_*.h | stb_*.h) exit 0 ;;
    *.c | *.h) clang-format -i "$file" ;;
esac
