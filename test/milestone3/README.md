# Milestone 3 Tugas Besar IF2224 - Teori Bahasa Formal dan Automata : Semantic Analysis

## Description
This project is an implementation of a **Semantic Analyzer** for the **Arion programming language** as part of **Milestone 3 - IF2224 Formal Language and Automata Theory**. The semantic analyzer is the third phase of compilation, responsible for checking the meaning of a syntactically valid program.

This milestone extends the previous lexer and parser by:
- converting parse trees into an **Abstract Syntax Tree (AST)**
- building a **decorated AST**
- constructing and managing **symbol tables**
- performing **scope checking**
- performing **type checking**
- validating several semantic rules such as assignment compatibility, array access, record field access, procedure/function call arguments, variable initialization, formal parameter/global-name conflicts, and function return rules

The program is implemented in **C/C++ GNU** and supports running semantic analysis directly from source code or from the parse tree produced in Milestone 2 through `--from-parse-tree`.

---

## Implemented Components

| Component | Description |
|---|---|
| `src/main.cpp` | Main driver for the integrated lexer-parser-semantic pipeline |
| `src/semantic/ast_builder.cpp/hpp` | Converts parse tree into AST |
| `src/semantic/ast_nodes.cpp/hpp` | AST node definitions and AST printing |
| `src/semantic/scope_builder.cpp/hpp` | Builds symbol tables and performs scope/declaration checking |
| `src/semantic/type_checker.cpp/hpp` | Performs type checking and semantic validation |
| `src/semantic/symbol_table.cpp/hpp` | Implements `tab`, `btab`, and `atab` |

---

## Semantic Features

The semantic analyzer supports:
- Parse tree to AST conversion
- Decorated AST generation
- Symbol table generation (`tab`, `btab`, `atab`)
- Predefined identifier initialization
- Declaration and scope validation
- Type inference for expressions
- Strict assignment compatibility checking for array, record, and enumerated type references
- Procedure/function call argument checking
- Record field access validation
- Array index type and static bound checking
- Subrange validation
- Variable use-before-initialization checking
- Function return validation
- Rejection of assignment to a function name outside the function body
- Formal parameter validation against global identifier names
- Semantic analysis from Milestone 2 parse-tree output with `--from-parse-tree`
- Non-zero process exit status whenever semantic/type errors are detected, while still writing decorated AST diagnostics to `--save-ast`

---

## Requirements
To build and run this program, you need:
- **GNU C++ Compiler** with C++17 support
- **GNU Make**
- A Unix/Linux environment or any environment capable of running `make` and `g++`

---

## Installation Guide

### 1. Clone the Repository

```bash
git clone https://github.com/alyanrrhma/MAR-Tubes-IF2224-2026.git
cd MAR-Tubes-IF2224-2026
```

### 2. Compile the Program

```bash
make all
```

---

## How To Run

To run semantic analysis from source code and save the decorated AST plus symbol tables:

```bash
./bin/arion <source.txt> --save-ast <ast_output.txt>
```

### Example

```bash
./bin/arion test/milestone3/input/input19.txt --save-ast test/milestone3/output/output19.txt
```

To run semantic analysis from parse tree output of Milestone 2:

```bash
./bin/arion --from-parse-tree <parse_tree.txt> --save-ast <ast_output.txt>
```

### Example

```bash
./bin/arion test/milestone3/input/input25.txt --parse-only --save-parse-tree test/milestone3/tmp/parse_tree25.txt
./bin/arion --from-parse-tree test/milestone3/tmp/parse_tree25.txt --save-ast test/milestone3/output/output25.txt
```

To run the Milestone 3 regression tests:

```bash
./test/milestone3/run_milestone3_tests.sh
```

---

## Test Cases

The sample inputs in `test/milestone3/input/` cover multiple semantic constructs:

| File | Focus |
|---|---|
| `input1.txt` | Basic variable declaration, assignment, arithmetic expression, procedure call |
| `input2.txt` | Constant declaration, type declaration, subrange, enumerated type |
| `input3.txt` | Array, record, field access, array access, `for` loop |
| `input4.txt` | `if-else` and `case` statements |
| `input5.txt` | `while`, `repeat`, and `for downto` loops |
| `input19.txt` | Valid source-to-decorated-AST regression test |
| `input20.txt` | Invalid formal parameter shadowing global identifier |
| `input21.txt` | Invalid function with no return assignment/return statement |
| `input22.txt` | Invalid assignment to function name outside its own body |
| `input23.txt` | Invalid undeclared `for` control variable; verifies no crash |
| `input24.txt` | Invalid array index out of declared bounds |
| `input25.txt` | Semantic analysis from parse-tree input mode |
| `input26.txt` | Invalid assignment between two separately declared array types with identical shape |
| `input27.txt` | Invalid assignment between two separately declared enumerated types |

The semantic analyzer is expected to produce:
- AST / decorated AST output
- identifier table (`tab`)
- block table (`btab`)
- array table (`atab`)
- semantic/type error messages when invalid constructs are found
- non-zero exit code for invalid semantic/type cases

---

## Build Commands

```bash
make all
```
Build the main integrated compiler program.

```bash
make clean
```
Remove build files and binaries.

```bash
make rebuild
```
Clean and rebuild the main program.

---

## Project Structure

```text
├── config/
│   └── config_lexer.txt
├── doc/
├── src/
│   ├── main.cpp
│   ├── lexer/
│   ├── parser/
│   └── semantic/
│       ├── ast_builder.cpp
│       ├── ast_builder.hpp
│       ├── ast_nodes.cpp
│       ├── ast_nodes.hpp
│       ├── scope_builder.cpp
│       ├── scope_builder.hpp
│       ├── symbol_table.cpp
│       ├── symbol_table.hpp
│       ├── type_checker.cpp
│       └── type_checker.hpp
├── test/
│   └── milestone3/
│       ├── input/
│       │   ├── input1.txt
│       │   ├── ...
│       │   ├── input18.txt
│       │   ├── input19.txt
│       │   ├── ...
│       │   └── input27.txt
│       ├── output/
│       │   ├── output1.txt
│       │   ├── ...
│       │   ├── output18.txt
│       │   ├── output19.txt
│       │   ├── ...
│       │   └── output27.txt
│       ├── tmp/
│       ├── run_milestone3_tests.sh
│       └── README.md
├── Makefile
└── README.md
```

---

## Contributors

| **NIM** | **Name** | **Contribution** |
|---|---|---|
| 13524017 | Aziza Dharma Putri | Implemented AST foundations and symbol table initialization, including predefined identifiers |
| 13524053 | Ahmad Zaky Robbani | Implemented visitor / SDT and parse tree to AST conversion |
| 13524063 | Marcel Luther Sitorus | Implemented scope handling and symbol table population (`tab`, `btab`, `atab`) |
| 13524081 | Alya Nur Rahmah | Implemented type checking, decorated AST annotation, and semantic error handling |
