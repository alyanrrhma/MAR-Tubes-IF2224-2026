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
- integer, boolean, string, and character literals
- unary expression (`+`, `-`, `not`)
- binary arithmetic (`+`, `-`, `*`, `div`, `mod`)
- comparison (`==`, `<>`, `<`, `<=`, `>`, `>=`)
- `if` and `if-else`
- `while`, `for`, and `repeat-until`
- `case` branch selection
- procedure and function calls through `CAL`/`RET`
- array access and record field access
- `writeln(expr)` and `writeln(a, b, ...)`
- TAC generation with `--print-tac`
- program execution with `--run`
- direct intermediate-code execution with `--run-ir` for runtime/vulnerability tests
- reloadable decorated AST bundle through `--save-ast ... --embed-parse-tree` and `--from-decorated-ast`
- serialized string pool metadata in TAC output so `LITS` instructions can be re-run through `--run-ir`
- semantic error blocking before TAC generation
- runtime error reporting for division by zero, invalid opcode, invalid address, invalid jump target, stack underflow/overflow, integer overflow, and dynamic array out-of-bounds

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
| `RET` | Return from procedure/function or stop main program |
| `LITB` | Push Boolean literal |
| `LITS` | Push String literal from string pool |
| `ADDR` | Push absolute address of an lvalue |
| `LODI` | Indirect load through stack address |
| `STOI` | Indirect store through stack address |
| `CHK` | Runtime bounds check for array index |

---

## Limitations

The current Milestone 4 backend intentionally still limits:
- `readln` execution, because runtime input is not required for the main execution tests
- full real-number runtime arithmetic; `real` is recognized semantically, but the stack-machine execution focuses on integer/ordinal arithmetic
- arbitrary hand-written decorated-AST text is not accepted; `--from-decorated-ast` expects the decorated AST bundle produced by this compiler using `--save-ast <file> --embed-parse-tree`

The implemented backend covers the main executable subset required for code generation, stack-machine execution, runtime vulnerability checks, and M4 execution from a saved decorated AST artifact.

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

To run saved stack-machine intermediate code directly:

```bash
./bin/arion --run-ir <intermediate-code.txt>
```

To save a decorated AST bundle and use it as the Milestone 4 input artifact:

```bash
./bin/arion <source.txt> --save-ast <decorated_ast.txt> --embed-parse-tree
./bin/arion --from-decorated-ast <decorated_ast.txt> --print-tac
./bin/arion --from-decorated-ast <decorated_ast.txt> --run
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

The Milestone 4 regression suite covers these execution scenarios:

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
| Dynamic bounds check | Runtime `CHK` rejects out-of-bounds array index |
| Invalid jump | `--run-ir` rejects invalid jump target |
| Stack underflow | `--run-ir` rejects stack underflow |
| Decorated AST input | Backend reloads a saved decorated AST bundle and executes it |
| Serialized IR reload | `--run-ir` executes TAC with string-pool metadata |

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
│   ├── lexer/
│   ├── parser/
│   └── semantic/
├── test/
│   └── milestone4/
│       ├── input/
│       ├── output/
│       ├── ir/
│       ├── run_milestone4_tests.sh
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
