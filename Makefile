# Makefile — Arion Compiler

# Compiler & flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
# Direktori
SRC_DIR   = src
LEXER_DIR = $(SRC_DIR)/lexer
PARSE_DIR = $(SRC_DIR)/parser
SEMANTIC_DIR = $(SRC_DIR)/semantic
BIN_DIR = bin
BUILD_DIR = build
# Output binary
TARGET = $(BIN_DIR)/arion
# Source files
SRCS = $(SRC_DIR)/main.cpp \
	$(LEXER_DIR)/dfa.cpp \
	$(LEXER_DIR)/lexer.cpp \
	$(LEXER_DIR)/token.cpp \
	$(PARSE_DIR)/parse_tree.cpp \
	$(PARSE_DIR)/parser.cpp \
	$(SEMANTIC_DIR)/ast_nodes.cpp \
	$(SEMANTIC_DIR)/symbol_table.cpp

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
# Default target — build saja, jangan jalankan binary tanpa argumen.
.DEFAULT_GOAL := all

all: $(TARGET)

run: all
	$(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build sukses! Binary: ./$(TARGET)"
# Compile setiap .cpp jadi .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Clean selesai."
# Rebuild — clean lalu build ulang
rebuild: clean all
# Help — tampilkan cara pemakaian
help:
	@echo "========================================"
	@echo "  Arion Compiler — Makefile Commands"
	@echo "========================================"
	@echo "  make                   -> build program"
	@echo "  make run N=1           -> run dengan input1.txt"
	@echo "  make run N=2           -> run dengan input2.txt"
	@echo "  make run N=1 SAVE=1    -> run + simpan output1.txt"
	@echo "  make clean             -> hapus hasil build"
	@echo "  make rebuild           -> clean + build ulang"
	@echo "========================================"

.PHONY: all run clean rebuild help
