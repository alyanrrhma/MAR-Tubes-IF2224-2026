# MAR-Tubes-IF2224-2026
# Tugas Besar IF2224 Teori Bahasa Formal dan Automata

## Description 
This project is an implementation of a compiler for the **Arion programming language** as part of the **IF2224 Formal Language and Automata Theory** course project.

The project is developed incrementally through several milestones. Each milestone focuses on one major compiler phase, starting from lexical analysis and continuing to later stages . Based on the available milestone 1 specification, the compiler must be implemented in **C/C++ GNU**, use a **DFA-based lexer**, read source code from `.txt` files, and be organized in a modular structure to support subsequent milestones. 

---

## Project Milestones

### [Milestone 1 - Lexical Analysis](./test/milestone1/README.md) 
Implementation of a **Lexical Analyzer (Lexer)** for the Arion programming language using a **Deterministic Finite Automaton (DFA)**.  
The lexer reads source code character by character and transforms it into a sequence of valid tokens. Input is provided in `.txt` format, and the tokenized output is also generated in `.txt` format.

## Requirements
To build and run this program, you need:
- **GNU C++ Compiler** with C++ standard support
- **GNU Make**
- A Unix/Linux environment or any environment capable of running `make` and `g++`
---

## Installation Guide
To compile the program, follow these steps:

**1. Clone the Repository**
```bash
git clone https://github.com/alyanrrhma/MAR-Tubes-IF2224-2026.git
cd MAR-Tubes-IF2224-2026
```

**3. Compile the program**:
```bash
make
```

## Project Structure
The repository structure follows the milestone requirements for storing source code, documentation, and test files.
```
MAR-Tubes-IF2224-2026/
├── src/
│   ├── main.cpp
│   └── lexer/
├── doc/
│   └── Laporan-1-MAR.pdf
├── test/
│   ├── milestone1/
│   │   ├── input/
│   │   ├── output/
│   │   └── README.md
│   ├── milestone2/
│   ├── milestone3/
│   └── milestone4/
├── Makefile
└── README.md
```
## Contributors

| **NIM** | **Name** |
|--------|----------|
| 13524017 | Aziza Dharma Putri |
| 13524053 | Ahmad Zaky Robbani | 
| 13524063 | Marcel Luther Sitorus | 
| 13524081 | Alya Nur Rahmah | 