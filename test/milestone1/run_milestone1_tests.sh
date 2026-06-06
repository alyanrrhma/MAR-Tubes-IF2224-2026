#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT_DIR/bin/arion"

if [[ ! -x "$BIN" ]]; then
  echo "Binary belum ada. Jalankan: make all" >&2
  exit 1
fi

run_valid() {
  local name="$1"
  local input="$ROOT_DIR/test/milestone1/input/${name}.txt"
  local expected="$ROOT_DIR/test/milestone1/output/output${name#input}.txt"
  local actual
  actual="$(mktemp)"
  "$BIN" "$input" --lex-only -o "$actual" --save-dfa-trace /tmp/arion_trace_${name}.txt >/tmp/arion_${name}_stdout.txt
  diff -u "$expected" "$actual"
  rm -f "$actual"
  echo "[OK] $name"
}

run_error() {
  local name="$1"
  local input="$ROOT_DIR/test/milestone1/input/${name}.txt"
  local actual
  actual="$(mktemp)"
  if "$BIN" "$input" --lex-only -o "$actual" >/tmp/arion_${name}_stdout.txt; then
    echo "[FAIL] $name seharusnya menghasilkan lexical error" >&2
    cat /tmp/arion_${name}_stdout.txt >&2
    exit 1
  fi
  grep -q "lexical error" /tmp/arion_${name}_stdout.txt
  grep -q "unknown" "$actual"
  rm -f "$actual"
  echo "[OK] $name lexical error terdeteksi"
}

run_valid input13
run_valid input14
run_error input15
