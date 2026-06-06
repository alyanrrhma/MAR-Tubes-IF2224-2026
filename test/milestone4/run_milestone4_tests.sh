#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT_DIR/bin/arion"
INPUT_DIR="$ROOT_DIR/test/milestone4/input"
OUTPUT_DIR="$ROOT_DIR/test/milestone4/output"
IR_DIR="$ROOT_DIR/test/milestone4/ir"
TMP_DIR="$ROOT_DIR/test/milestone4/tmp"

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

for n in 1 2 3 4 5 6 7 8 9; do
  "$BIN" "$INPUT_DIR/input$n.txt" --run > "$TMP_DIR/run$n.txt"
  assert_same_file "$OUTPUT_DIR/run$n.txt" "$TMP_DIR/run$n.txt"
  "$BIN" "$INPUT_DIR/input$n.txt" --print-tac > "$TMP_DIR/tac$n.txt"
  assert_same_file "$OUTPUT_DIR/tac$n.txt" "$TMP_DIR/tac$n.txt"
  echo "[OK] milestone4 input$n run+tac"
done

# M4 can also start from the Milestone 2 parse-tree artifact and continue to codegen/execution.
parse_tree="$TMP_DIR/input1_parse_tree.txt"
from_tree_run="$TMP_DIR/run_from_parse_tree1.txt"
"$BIN" "$INPUT_DIR/input1.txt" --parse-only --save-parse-tree "$parse_tree" > /dev/null
"$BIN" --from-parse-tree "$parse_tree" --run > "$from_tree_run"
assert_same_file "$OUTPUT_DIR/run1.txt" "$from_tree_run"
echo "[OK] milestone4 run from parse tree"

run_invalid_source() {
  local n="$1"
  local expected="$OUTPUT_DIR/error$n.txt"
  local stdout_tmp="$TMP_DIR/stdout$n.txt"
  local stderr_tmp="$TMP_DIR/stderr$n.txt"
  set +e
  "$BIN" "$INPUT_DIR/input$n.txt" --run > "$stdout_tmp" 2> "$stderr_tmp"
  local code=$?
  set -e
  if [[ $code -eq 0 ]]; then
    echo "[FAIL] input$n seharusnya runtime error" >&2
    exit 1
  fi
  assert_same_file "$expected" "$stderr_tmp"
  echo "[OK] milestone4 input$n rejected runtime error"
}

run_invalid_ir() {
  local name="$1"
  local n="$2"
  local expected="$OUTPUT_DIR/error$n.txt"
  local stdout_tmp="$TMP_DIR/stdout$n.txt"
  local stderr_tmp="$TMP_DIR/stderr$n.txt"
  set +e
  "$BIN" --run-ir "$IR_DIR/$name.ir" > "$stdout_tmp" 2> "$stderr_tmp"
  local code=$?
  set -e
  if [[ $code -eq 0 ]]; then
    echo "[FAIL] $name.ir seharusnya runtime error" >&2
    exit 1
  fi
  assert_same_file "$expected" "$stderr_tmp"
  echo "[OK] milestone4 $name.ir rejected runtime error"
}

run_invalid_source 10
run_invalid_source 11
run_invalid_ir invalid_jump 12
run_invalid_ir stack_underflow 13

echo "Milestone 4 backend regression tests passed (9 valid + 4 runtime/vulnerability cases)."
