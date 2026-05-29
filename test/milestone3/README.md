# Milestone 3 Tugas Besar IF2224 - Teori Bahasa Formal dan Automata : Semantic Analysis

## Description
This project is an implementation of a **Semantic Analyzer** for the **Arion programming language** as part of **Milestone 3 - IF2224 Formal Language and Automata Theory**. The semantic analyzer is the third phase of compilation, responsible for checking the meaning of a syntactically valid program.

This milestone extends the previous lexer and parser by:
- converting parse trees into an **Abstract Syntax Tree (AST)**
- building a **decorated AST**
- constructing and managing **symbol tables**
- performing **scope checking**
- performing **type checking**
- validating several semantic rules such as assignment compatibility, array access, record field access, procedure/function call arguments, and variable initialization

The program is implemented in **C/C++ GNU** and supports running semantic analysis directly from source code or from the parse tree produced in milestone 2.

---

## Implemented Components

| Component | Description |
|---|---|
| `src/ast.cpp` | Main driver for semantic analysis and parse-tree reading |
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
- Assignment compatibility checking
- Procedure/function call argument checking
- Record field access validation
- Array index type and static bound checking
- Subrange validation
- Variable use-before-initialization checking

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
make
make ast
```

---

## How To Run

To run semantic analysis from source code:

```bash
./bin/ast <source.txt> [-o <ast_output.txt>]
```

### Example

```bash
./bin/ast test/milestone3/input/input1.txt -o test/milestone3/output/output1.txt
```

To run semantic analysis from parse tree output of milestone 2:

```bash
./bin/ast --parse-tree <parse_tree.txt> [-o <ast_output.txt>]
```

### Example

```bash
./bin/ast --parse-tree test/milestone2/output/parse_tree1.txt -o test/milestone3/output/output1.txt
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

The semantic analyzer is expected to produce:
- AST / decorated AST output
- identifier table (`tab`)
- block table (`btab`)
- array table (`atab`)
- semantic/type error messages when invalid constructs are found

---

## Build Commands

```bash
make
```
Build the main program.

```bash
make ast
```
Build the semantic analysis executable.

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
test/milestone3/
├── input/
│   ├── input1.txt
│   ├── ...
│   └── input18.txt
├── output/
│   ├── output1.txt
│   ├── ...
│   └── output18.txt
├── test3.txt
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
