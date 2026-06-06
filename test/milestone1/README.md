# Milestone 1 Tugas Besar IF2224 - Lexical Analysis

## Description
This project implements a **Lexical Analyzer (Lexer)** for the **Arion programming language**. The lexer is the first phase of the compiler pipeline and transforms raw source code into a sequence of meaningful tokens.

The lexer is implemented in **C++ GNU** using a **Deterministic Finite Automaton (DFA)**. The DFA is defined in `config/config_lexer.txt` using three kinds of entries:

- `START <state>` for the initial state,
- `FINAL <TOKEN_TYPE> <state>` for accepting states,
- `<ascii-code> <from-state> <to-state>` for transitions.

During lexical analysis, the scanner reads source code **one character at a time**, feeds each character into the DFA, and emits a token only when an accepting state has been reached. Whitespace is treated only as a separator. Comments are recognized as DFA token paths and are emitted as `comment` tokens in lexer-only output, then ignored by the parser in later milestones.

The implementation also provides a DFA trace mode so that the transition sequence can be inspected during testing:

```bash
./bin/arion <program.txt> --lex-only -o <tokens.txt> --save-dfa-trace <trace.txt>
```

Each trace line records the source position, the character read, the previous state, the next state, the current lexeme, and whether the state is an accepting state. This file can be used as evidence that the lexer follows DFA transitions instead of classifying tokens by ad-hoc string matching.

---

## Token List

| No | Token | Description | Example |
|----|-------|-------------|---------|
| 1 | **intcon** | Integer constant | `1`, `3`, `48` |
| 2 | **realcon** | Real number constant | `3.14`, `26.7` |
| 3 | **charcon** | Single character constant enclosed in single quotes | `'j'`, `'k'` |
| 4 | **string** | Character sequence enclosed in single quotes | `'IRK'`, `'TBFO'` |
| 5 | **notsy** | Logical NOT operator | `NOT` |
| 6 | **plus** | Addition operator | `+` |
| 7 | **minus** | Subtraction operator | `-` |
| 8 | **times** | Multiplication operator | `*` |
| 9 | **idiv** | Integer division operator | `div` |
| 10 | **rdiv** | Real division operator | `/` |
| 11 | **imod** | Modulo operator | `MOD` |
| 12 | **andsy** | Logical AND operator | `AND` |
| 13 | **orsy** | Logical OR operator | `OR` |
| 14 | **eql** | Equal operator | `==` |
| 15 | **neq** | Not equal | `<>` |
| 16 | **gtr** | Greater than | `>` |
| 17 | **geq** | Greater than or equal | `>=` |
| 18 | **lss** | Less than | `<` |
| 19 | **leq** | Less than or equal | `<=` |
| 20 | **lparent** | Left parenthesis | `(` |
| 21 | **rparent** | Right parenthesis | `)` |
| 22 | **lbrack** | Left bracket | `[` |
| 23 | **rbrack** | Right bracket | `]` |
| 24 | **comma** | Comma separator | `,` |
| 25 | **semicolon** | Semicolon separator | `;` |
| 26 | **period** | Period | `.` |
| 27 | **colon** | Colon | `:` |
| 28 | **becomes** | Assignment operator | `:=` |
| 29 | **constsy** | Constant declaration keyword | `const` |
| 30 | **typesy** | Type declaration keyword | `type` |
| 31 | **varsy** | Variable declaration keyword | `var` |
| 32 | **functionsy** | Function declaration keyword | `function` |
| 33 | **proceduresy** | Procedure declaration keyword | `procedure` |
| 34 | **arraysy** | Array declaration keyword | `array` |
| 35 | **recordsy** | Record declaration keyword | `record` |
| 36 | **programsy** | Program declaration keyword | `program` |
| 37 | **ident** | Identifier | `x`, `PI`, `MyInt`, `arrayzz` |
| 38 | **beginsy** | Begin block keyword | `begin` |
| 39 | **ifsy** | If keyword | `if` |
| 40 | **casesy** | Case keyword | `case` |
| 41 | **repeatsy** | Repeat keyword | `repeat` |
| 42 | **whilesy** | While keyword | `while` |
| 43 | **forsy** | For keyword | `for` |
| 44 | **endsy** | End keyword | `end` |
| 45 | **elsesy** | Else keyword | `else` |
| 46 | **untilsy** | Until keyword | `until` |
| 47 | **ofsy** | Of keyword | `of` |
| 48 | **dosy** | Do keyword | `do` |
| 49 | **tosy** | To keyword | `to` |
| 50 | **downtosy** | Downto keyword | `downto` |
| 51 | **thensy** | Then keyword | `then` |
| 52 | **comment** | Comment token | `{ ... }`, `(* ... *)`, including allowed mixed endings handled by the DFA configuration |
| 53 | **unknown** | Invalid lexical sequence | `12abc`, unterminated string/comment |

---

## Requirements

- GNU C++ compiler with C++17 support
- GNU Make
- Unix/Linux shell for the provided test script

---

## Build

```bash
make all
```

---

## How To Run Milestone 1

Run lexical analysis only and save tokens:

```bash
./bin/arion <program.txt> --lex-only -o <output_file.txt>
```

Run lexical analysis and save both tokens and DFA transition trace:

```bash
./bin/arion <program.txt> --lex-only \
  -o <output_file.txt> \
  --save-dfa-trace <trace_file.txt>
```

Example:

```bash
./bin/arion test/milestone1/input/input13.txt \
  --lex-only \
  -o test/milestone1/output/output13.txt \
  --save-dfa-trace test/milestone1/trace/trace13.txt
```

When the lexer detects an invalid token, it emits an `unknown` token, prints a lexical error with source position, and exits with a non-zero status code. This makes lexical error tests checkable by automated scripts.

---

## Milestone 1 Regression Tests

Additional test cases are provided to strengthen DFA conformance, error handling, and edge-case coverage:

The additional regression files continue the existing numbered naming pattern: `input13.txt` through `input15.txt`, with matching `output13.txt` through `output15.txt` and DFA traces `trace13.txt` through `trace15.txt`.

| Test input | Purpose |
|---|---|
| `input13.txt` / `output13.txt` | Verifies keyword/identifier boundaries, case-insensitive keywords, logical operators, relational operators, and separators. |
| `input14.txt` / `output14.txt` | Verifies char, string with escaped single quote, real number, range `1..10`, and both comment delimiters. |
| `input15.txt` / `output15.txt` | Verifies invalid lexical sequence and unterminated string handling. |

Run all Milestone 1 regression tests:

```bash
make all
./test/milestone1/run_milestone1_tests.sh
```

The script validates expected token files and checks that invalid lexical input fails with a lexical error.

---

## Contributors

| **NIM** | **Name** | **Contribution** |
|--------|----------|------------------|
| 13524017 | Aziza Dharma Putri | Implemented token module, Makefile, report writing, testing |
| 13524053 | Ahmad Zaky Robbani | Implemented DFA configuration and main program, report writing, DFA revision |
| 13524063 | Marcel Luther Sitorus | Implemented DFA module and report writing |
| 13524081 | Alya Nur Rahmah | Implemented lexer module and testing |
