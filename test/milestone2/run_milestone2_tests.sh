#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT_DIR/bin/arion"
INPUT_DIR="$ROOT_DIR/test/milestone2/input"
OUTPUT_DIR="$ROOT_DIR/test/milestone2/output"
TMP_DIR="$ROOT_DIR/test/milestone2/tmp"

mkdir -p "$TMP_DIR"

if [[ ! -x "$BIN" ]]; then
  echo "Binary tidak ditemukan. Jalankan: make all" >&2
  exit 1
fi

assert_same_file() {
  local expected="$1"
  local actual="$2"
  diff -u <(tr -d "\r" < "$expected") <(tr -d "\r" < "$actual")
}

# Token files are Milestone 1 outputs used by the Milestone 2 test set.
token_cases=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20)
for n in "${token_cases[@]}"; do
  actual="$TMP_DIR/token$n.txt"
  "$BIN" "$INPUT_DIR/input$n.txt" --lex-only -o "$actual" > /dev/null
  assert_same_file "$OUTPUT_DIR/token$n.txt" "$actual"
  echo "[OK] lexer token input$n"
done

# Successful parser cases. The older tests store parse-tree outputs as parse_treeN.txt.
parse_tree_cases=(1 2 3 4 5 6 7 8 9 10 16 17 18 19 20)
for n in "${parse_tree_cases[@]}"; do
  actual="$TMP_DIR/parse_tree$n.txt"
  "$BIN" "$INPUT_DIR/input$n.txt" --parse-only --save-parse-tree "$actual" > /dev/null
  assert_same_file "$OUTPUT_DIR/parse_tree$n.txt" "$actual"
  echo "[OK] parser input$n"
done

# Newer numbered parser cases introduced for strict Milestone 2 coverage.
for n in 21 22 23; do
  actual="$TMP_DIR/output$n.txt"
  "$BIN" "$INPUT_DIR/input$n.txt" --parse-only --save-parse-tree "$actual" > /dev/null
  assert_same_file "$OUTPUT_DIR/output$n.txt" "$actual"
  echo "[OK] parser input$n"
done

actual="$TMP_DIR/output24.txt"
"$BIN" --from-tokens "$INPUT_DIR/input24.txt" --parse-only --save-parse-tree "$actual" > /dev/null
assert_same_file "$OUTPUT_DIR/output24.txt" "$actual"
echo "[OK] parser input24 from tokens"

invalid_cases=(11 12 13 14 15 25 26 27)
for n in "${invalid_cases[@]}"; do
  set +e
  "$BIN" "$INPUT_DIR/input$n.txt" --parse-only --save-parse-tree "$TMP_DIR/invalid$n.txt" > "$TMP_DIR/stdout$n.txt" 2> "$TMP_DIR/stderr$n.txt"
  code=$?
  set -e
  if [[ $code -eq 0 ]]; then
    echo "[FAIL] input$n seharusnya syntax error" >&2
    exit 1
  fi
  grep -qi "ParseError" "$TMP_DIR/stderr$n.txt"
  echo "[OK] parser input$n rejected invalid syntax"
done

echo "Milestone 2 parser regression tests passed."
