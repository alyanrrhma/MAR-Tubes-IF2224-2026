# Milestone 4 Tugas Besar IF2224 - Teori Bahasa Formal dan Automata : Code Generation and Execution

## Description
This project is an implementation of a **Code Generator and Interpreter** for the **Arion programming language** as part of **Milestone 4 - IF2224 Formal Language and Automata Theory**. This milestone is the fourth phase of compilation, responsible for transforming a semantically valid AST into intermediate code and executing it with a stack-based virtual machine.

This milestone extends the previous lexer, parser, and semantic analyzer by:
- generating **Three Address Code (TAC)** / stack-machine instructions from the decorated AST
- implementing an instruction representation and pretty printer
- implementing a runtime stack machine for memory and operand stack operations
- implementing an interpreter with a fetch-decode-execute loop
- integrating backend execution into the main executable through CLI options

The program is implemented in **C/C++ GNU** and supports printing generated TAC or running supported programs directly from source code.

---

## Implemented Components

| Component | Description |
|---|---|
| `src/backend/instruction.cpp/hpp` | Defines opcodes, OPR codes, instructions, instruction programs, patching, and TAC printing |
| `src/backend/code_generator.cpp/hpp` | Generates TAC from the decorated AST |
| `src/backend/stack_machine.cpp/hpp` | Implements runtime values, memory allocation, stack operations, load, and store |
| `src/backend/interpreter.cpp/hpp` | Executes TAC using a fetch-decode-execute loop |
| `src/main.cpp` | Integrates lexer, parser, AST builder, semantic analyzer, code generator, and interpreter |

---

## Backend Features

The Milestone 4 backend supports:
- variable assignment
- integer literals
- boolean literals
- unary expression (`+`, `-`, `not`)
- binary arithmetic (`+`, `-`, `*`, `div`, `mod`)
- comparison (`==`, `<>`, `<`, `<=`, `>`, `>=`)
- `if`
- `if-else`
- `while`
- `writeln(expr)`
- TAC generation with `--print-tac`
- program execution with `--run`
- semantic error blocking before TAC generation
- runtime error reporting for division by zero, invalid opcode, invalid address, and stack underflow

The interpreter supports these instructions:

| Instruction | Description |
|---|---|
| `INT` | Allocate runtime memory |
| `LIT` | Push literal value |
| `LOD` | Load value from memory |
| `STO` | Store value to memory |
| `JMP` | Unconditional jump |
| `JPC` | Conditional jump |
| `OPR` | Execute arithmetic, comparison, or output operation |
| `RET` | Stop execution |

---

## Limitations

The current Milestone 4 backend does not implement:
- procedure calls and function calls
- `CAL`
- complex activation records
- arrays and records at runtime
- `case`, `repeat`, and `for` code generation
- `readln`

The parser and semantic analyzer may recognize more language constructs than the backend can execute. Unsupported constructs are rejected by the code generator.

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
```

---

## How To Run

To generate and print TAC:

```bash
./bin/arion <source.txt> --print-tac
```

### Example

```bash
./bin/arion test/milestone4/test4.txt --print-tac
```

To execute a program:

```bash
./bin/arion <source.txt> --run
```

### Example

```bash
./bin/arion test/milestone4/test4.txt --run
```

The full frontend pipeline is still available:

```bash
./bin/arion <source.txt> --save-tokens <tokens.txt> --save-parse-tree <parse_tree.txt> --save-ast <ast.txt>
```

---

## Example

Source program:

```pascal
program OutputSample;

var
  x: integer;

begin
  x := 10;
  writeln(x)
end.
```

Generated TAC:

```text
0 INT 0 4
1 LIT 0 10
2 STO 0 3
3 LOD 0 3
4 OPR 0 14
5 RET 0 0
```

Program output:

```text
10
```

---

## Test Cases

The Milestone 4 audit covers these execution scenarios:

| Test | Focus |
|---|---|
| Assignment | Simple assignment and variable load/store |
| Addition | Binary arithmetic with `+` |
| Subtraction | Binary arithmetic with `-` |
| Multiplication | Binary arithmetic with `*` |
| Division | Binary arithmetic with `div` |
| Unary expression | Unary minus and OPR `NEG` |
| Comparison | Relational expression and boolean output |
| If | Conditional branch with `JPC` |
| If-else | Conditional branch with false and end jumps |
| While | Looping with `JMP` and `JPC` |
| Writeln | Output statement with `OPR 0 14` |
| Semantic error | Backend is skipped when semantic analysis fails |
| Runtime error | Interpreter reports division by zero |

---

## Build Commands

```bash
make
```
Build the main program.

```bash
make clean
```
Remove build files and binaries.

```bash
make rebuild
```
Clean and rebuild the main program.

```bash
make help
```
Display available commands.

---

## Project Structure

```text
├── config/
│   └── config_lexer.txt
├── doc/
├── src/
│   ├── main.cpp
│   ├── backend/
│   │   ├── code_generator.cpp
│   │   ├── code_generator.hpp
│   │   ├── instruction.cpp
│   │   ├── instruction.hpp
│   │   ├── interpreter.cpp
│   │   ├── interpreter.hpp
│   │   ├── stack_machine.cpp
│   │   └── stack_machine.hpp
│   ├── lexer/
│   ├── parser/
│   └── semantic/
├── test/
│   └── milestone4/
│       ├── test4.txt
│       └── README.md
├── Makefile
└── README.md
```

---

## Contributors

| **NIM** | **Name** | **Contribution** |
|---|---|---|
| 13524017 | Aziza Dharma Putri |  |
| 13524053 | Ahmad Zaky Robbani | |
| 13524063 | Marcel Luther Sitorus |  |
| 13524081 | Alya Nur Rahmah | |
