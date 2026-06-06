#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT_DIR/bin/arion"
INPUT_DIR="$ROOT_DIR/test/milestone3/input"
OUTPUT_DIR="$ROOT_DIR/test/milestone3/output"
TMP_DIR="$ROOT_DIR/test/milestone3/tmp"

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

valid_cases=(19 25)
invalid_cases=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 20 21 22 23 24 26 27)

run_semantic_case() {
  local n="$1"
  local expected_status="$2"
  local input="$INPUT_DIR/input$n.txt"
  local expected="$OUTPUT_DIR/output$n.txt"
  local actual="$TMP_DIR/output$n.txt"
  local ast_tmp="$TMP_DIR/ast$n.txt"
  local stdout_tmp="$TMP_DIR/stdout$n.txt"
  local stderr_tmp="$TMP_DIR/stderr$n.txt"

  rm -f "$actual" "$ast_tmp" "$stdout_tmp" "$stderr_tmp"
  set +e
  "$BIN" "$input" --save-ast "$ast_tmp" > "$stdout_tmp" 2> "$stderr_tmp"
  local code=$?
  set -e

  if [[ "$expected_status" == "success" && $code -ne 0 ]]; then
    cat "$stderr_tmp" >&2
    echo "[FAIL] input$n seharusnya valid" >&2
    exit 1
  fi

  if [[ "$expected_status" == "failure" && $code -eq 0 ]]; then
    echo "[FAIL] input$n seharusnya semantic/syntax error" >&2
    exit 1
  fi

  if [[ -s "$ast_tmp" ]]; then
    cp "$ast_tmp" "$actual"
  elif [[ -s "$stderr_tmp" ]]; then
    cp "$stderr_tmp" "$actual"
  else
    cp "$stdout_tmp" "$actual"
  fi

  assert_same_file "$expected" "$actual"
}

for n in "${valid_cases[@]}"; do
  run_semantic_case "$n" success
  echo "[OK] semantic input$n"
done

for n in "${invalid_cases[@]}"; do
  run_semantic_case "$n" failure
  echo "[OK] semantic input$n rejected invalid case"
done

# Explicit Milestone 3 input requirement: semantic analysis from Milestone 2 parse-tree output.
parse_tree="$TMP_DIR/parse_tree25.txt"
ast_from_tree="$TMP_DIR/output25_from_parse_tree.txt"
"$BIN" "$INPUT_DIR/input25.txt" --parse-only --save-parse-tree "$parse_tree" > /dev/null
"$BIN" --from-parse-tree "$parse_tree" --save-ast "$ast_from_tree" > /dev/null
assert_same_file "$OUTPUT_DIR/output25.txt" "$ast_from_tree"
echo "[OK] semantic input25 from parse tree"

echo "Milestone 3 semantic regression tests passed (${#valid_cases[@]} valid + ${#invalid_cases[@]} invalid)."
