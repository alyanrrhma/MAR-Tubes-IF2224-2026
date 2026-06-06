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

# M4 input requirement: reload a decorated AST artifact produced by Milestone 3 output.
decorated_ast="$TMP_DIR/input1_decorated_ast_bundle.txt"
from_decorated_run="$TMP_DIR/run_from_decorated_ast1.txt"
"$BIN" "$INPUT_DIR/input1.txt" --save-ast "$decorated_ast" --embed-parse-tree > /dev/null
"$BIN" --from-decorated-ast "$decorated_ast" --run > "$from_decorated_run"
assert_same_file "$OUTPUT_DIR/run1.txt" "$from_decorated_run"
echo "[OK] milestone4 run from decorated AST"

# Serialized IR can be executed again, including string pool metadata for LITS.
serialized_ir="$TMP_DIR/input7_serialized.ir"
run_from_ir="$TMP_DIR/run_from_serialized_ir7.txt"
"$BIN" "$INPUT_DIR/input7.txt" --print-tac > "$serialized_ir"
"$BIN" --run-ir "$serialized_ir" > "$run_from_ir"
assert_same_file "$OUTPUT_DIR/run7.txt" "$run_from_ir"
echo "[OK] milestone4 run serialized IR with string pool"

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

echo "Milestone 4 backend regression tests passed (9 valid + decorated AST/IR reload + 4 runtime/vulnerability cases)."
