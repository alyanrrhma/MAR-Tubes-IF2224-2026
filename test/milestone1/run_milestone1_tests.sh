#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT_DIR/bin/arion"
INPUT_DIR="$ROOT_DIR/test/milestone1/input"
OUTPUT_DIR="$ROOT_DIR/test/milestone1/output"
TRACE_DIR="$ROOT_DIR/test/milestone1/trace"
TMP_DIR="$ROOT_DIR/test/milestone1/tmp"

mkdir -p "$TMP_DIR" "$OUTPUT_DIR" "$TRACE_DIR"

if [[ ! -x "$BIN" ]]; then
  echo "Binary tidak ditemukan. Jalankan: make all" >&2
  exit 1
fi

assert_same_file() {
  local expected="$1"
  local actual="$2"
  diff -u <(tr -d "\r" < "$expected") <(tr -d "\r" < "$actual")
}

valid_cases=(1 2 3 4 5 6 7 8 9 10 13 14)
invalid_cases=(11 12 15)
trace_cases=(13 14 15)

for n in "${valid_cases[@]}"; do
  actual="$TMP_DIR/output$n.txt"
  "$BIN" "$INPUT_DIR/input$n.txt" --lex-only -o "$actual" > /dev/null
  assert_same_file "$OUTPUT_DIR/output$n.txt" "$actual"
  echo "[OK] input$n"
done

for n in "${trace_cases[@]}"; do
  actual="$TMP_DIR/output$n.txt"
  trace="$TMP_DIR/trace$n.txt"
  set +e
  "$BIN" "$INPUT_DIR/input$n.txt" --lex-only -o "$actual" --save-dfa-trace "$trace" > "$TMP_DIR/stdout$n.txt" 2> "$TMP_DIR/stderr$n.txt"
  code=$?
  set -e

  if [[ "$n" == "15" ]]; then
    if [[ $code -eq 0 ]]; then
      echo "[FAIL] input$n seharusnya lexical error" >&2
      exit 1
    fi
    grep -qi "lexical error" "$TMP_DIR/stderr$n.txt"
  else
    if [[ $code -ne 0 ]]; then
      cat "$TMP_DIR/stderr$n.txt" >&2
      exit 1
    fi
  fi

  assert_same_file "$OUTPUT_DIR/output$n.txt" "$actual"
  [[ -s "$trace" ]]
  echo "[OK] input$n DFA trace"
done

for n in "${invalid_cases[@]}"; do
  actual="$TMP_DIR/output$n.txt"
  set +e
  "$BIN" "$INPUT_DIR/input$n.txt" --lex-only -o "$actual" > "$TMP_DIR/stdout$n.txt" 2> "$TMP_DIR/stderr$n.txt"
  code=$?
  set -e

  if [[ $code -eq 0 ]]; then
    echo "[FAIL] input$n seharusnya lexical error" >&2
    exit 1
  fi
  grep -qi "lexical error" "$TMP_DIR/stderr$n.txt"
  if [[ -f "$OUTPUT_DIR/output$n.txt" && -s "$actual" ]]; then
    assert_same_file "$OUTPUT_DIR/output$n.txt" "$actual"
  fi
  echo "[OK] input$n lexical error terdeteksi"
done

echo "Milestone 1 lexer regression tests passed (${#valid_cases[@]} valid + ${#invalid_cases[@]} invalid)."
