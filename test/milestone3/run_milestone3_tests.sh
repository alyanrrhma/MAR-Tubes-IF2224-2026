#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT_DIR/bin/arion"
INPUT_DIR="$ROOT_DIR/test/milestone3/input"
OUTPUT_DIR="$ROOT_DIR/test/milestone3/output"
TMP_DIR="$ROOT_DIR/test/milestone3/tmp"

mkdir -p "$OUTPUT_DIR" "$TMP_DIR"

if [[ ! -x "$BIN" ]]; then
  echo "Binary tidak ditemukan. Jalankan: make all" >&2
  exit 1
fi

pass=0

run_semantic_ok() {
  local id="$1"
  local name="$2"
  local input="$INPUT_DIR/input${id}.txt"
  local output="$OUTPUT_DIR/output${id}.txt"
  if ! "$BIN" "$input" --save-ast "$output" > "$OUTPUT_DIR/${name}.stdout.txt" 2> "$OUTPUT_DIR/${name}.stderr.txt"; then
    echo "[FAIL] $name gagal dijalankan" >&2
    exit 1
  fi
  if grep -qE "Semantic errors|Type errors" "$output"; then
    echo "[FAIL] $name tidak boleh menghasilkan semantic/type error" >&2
    exit 1
  fi
  if ! grep -q "tab (identifier table)" "$output" || ! grep -q "btab (block table)" "$output" || ! grep -q "atab (array table)" "$output"; then
    echo "[FAIL] $name tidak mencetak tab/btab/atab" >&2
    exit 1
  fi
  echo "[OK] $name"
  pass=$((pass + 1))
}

run_semantic_expect() {
  local id="$1"
  local name="$2"
  local expected="$3"
  local input="$INPUT_DIR/input${id}.txt"
  local output="$OUTPUT_DIR/output${id}.txt"
  set +e
  "$BIN" "$input" --save-ast "$output" > "$OUTPUT_DIR/${name}.stdout.txt" 2> "$OUTPUT_DIR/${name}.stderr.txt"
  local status=$?
  set -e
  if [[ $status -eq 0 ]]; then
    echo "[FAIL] $name seharusnya exit non-zero saat semantic/type error" >&2
    exit 1
  fi
  if ! grep -q "$expected" "$output"; then
    echo "[FAIL] $name tidak menemukan pesan error yang diharapkan: $expected" >&2
    exit 1
  fi
  echo "[OK] $name rejected invalid semantic case"
  pass=$((pass + 1))
}

run_from_parse_tree() {
  local input="$INPUT_DIR/input37.txt"
  local parse_tree="$TMP_DIR/parse_tree37.txt"
  local output="$OUTPUT_DIR/output37.txt"
  if ! "$BIN" "$input" --parse-only --save-parse-tree "$parse_tree" > "$OUTPUT_DIR/parser_input37.stdout.txt" 2> "$OUTPUT_DIR/parser_input37.stderr.txt"; then
    echo "[FAIL] from_parse_tree gagal membuat parse tree" >&2
    exit 1
  fi
  if ! "$BIN" --from-parse-tree "$parse_tree" --save-ast "$output" > "$OUTPUT_DIR/from_parse_tree.stdout.txt" 2> "$OUTPUT_DIR/from_parse_tree.stderr.txt"; then
    echo "[FAIL] from_parse_tree gagal menjalankan semantic analysis dari parse tree" >&2
    exit 1
  fi
  if grep -qE "Semantic errors|Type errors" "$output"; then
    echo "[FAIL] from_parse_tree tidak boleh menghasilkan semantic/type error" >&2
    exit 1
  fi
  if ! grep -q "ProgramNode(name: 'M3FromParseTree')" "$output"; then
    echo "[FAIL] from_parse_tree output AST tidak sesuai" >&2
    exit 1
  fi
  echo "[OK] parser_from_parse_tree_input37"
  pass=$((pass + 1))
}

run_semantic_ok 31 "semantic_input31_valid_basic"
run_semantic_expect 32 "semantic_input32_param_shadow" "parameter formal 'x' tidak boleh memakai nama identifier global"
run_semantic_expect 33 "semantic_input33_missing_return" "fungsi 'f' tidak memiliki return statement"
run_semantic_expect 34 "semantic_input34_function_assign_outside" "assignment ke nama fungsi 'f' hanya boleh dilakukan"
run_semantic_expect 35 "semantic_input35_for_undeclared" "variabel kontrol for 'i' belum dideklarasikan"
run_semantic_expect 36 "semantic_input36_array_bounds" "indeks array 4 di luar batas"
run_from_parse_tree
run_semantic_expect 38 "semantic_input38_array_type_ref" "type mismatch dalam assignment"
run_semantic_expect 39 "semantic_input39_enum_type_ref" "type mismatch dalam assignment"

echo "Milestone 3 semantic regression tests passed ($pass tests)."
