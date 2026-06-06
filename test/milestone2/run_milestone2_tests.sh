#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

BIN="./bin/arion"
if [[ ! -x "$BIN" ]]; then
  make all >/dev/null
fi

mkdir -p test/milestone2/output

run_success() {
  local name="$1"
  shift
  "$@" >"test/milestone2/output/${name}.stdout.txt" 2>"test/milestone2/output/${name}.stderr.txt"
  echo "[OK] ${name}"
}

run_failure() {
  local name="$1"
  shift
  if "$@" >"test/milestone2/output/${name}.stdout.txt" 2>"test/milestone2/output/${name}.stderr.txt"; then
    echo "[FAIL] ${name}: command unexpectedly succeeded" >&2
    exit 1
  fi
  echo "[OK] ${name} rejected invalid syntax"
}

run_success "parser_input21" \
  "$BIN" test/milestone2/input/input21.txt \
  --parse-only --save-parse-tree test/milestone2/output/output21.txt

grep -q "<program>" test/milestone2/output/output21.txt
grep -q "<assignment-statement>" test/milestone2/output/output21.txt

run_success "parser_input22" \
  "$BIN" test/milestone2/input/input22.txt \
  --parse-only --save-parse-tree test/milestone2/output/output22.txt

grep -q "<while-statement>" test/milestone2/output/output22.txt
grep -q "<for-statement>" test/milestone2/output/output22.txt
grep -q "<compound-statement>" test/milestone2/output/output22.txt

run_success "parser_input23" \
  "$BIN" test/milestone2/input/input23.txt \
  --parse-only --save-parse-tree test/milestone2/output/output23.txt

grep -q "<array-type>" test/milestone2/output/output23.txt
grep -q "<record-type>" test/milestone2/output/output23.txt
grep -q "<component-variable>" test/milestone2/output/output23.txt

run_success "parser_input24_from_tokens" \
  "$BIN" --from-tokens test/milestone2/input/input24.txt \
  --parse-only --save-parse-tree test/milestone2/output/output24.txt

grep -q "ident (FromTokens)" test/milestone2/output/output24.txt

run_failure "parser_input25_invalid_while" \
  "$BIN" test/milestone2/input/input25.txt --parse-only

cp test/milestone2/output/parser_input25_invalid_while.stdout.txt test/milestone2/output/output25.txt
grep -q "Body while harus berupa compound-statement" test/milestone2/output/output25.txt

run_failure "parser_input26_invalid_for" \
  "$BIN" test/milestone2/input/input26.txt --parse-only

cp test/milestone2/output/parser_input26_invalid_for.stdout.txt test/milestone2/output/output26.txt
grep -q "Body for harus berupa compound-statement" test/milestone2/output/output26.txt

run_failure "parser_input27_invalid_extra_token" \
  "$BIN" test/milestone2/input/input27.txt --parse-only

cp test/milestone2/output/parser_input27_invalid_extra_token.stdout.txt test/milestone2/output/output27.txt
grep -q "Token tambahan setelah akhir program" test/milestone2/output/output27.txt

echo "Milestone 2 parser regression tests passed."
