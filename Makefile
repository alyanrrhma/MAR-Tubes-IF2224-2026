# Makefile — Arion Compiler (Milestone 1: Lexical Analyzer)

# Compiler & flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
# Direktori
SRC_DIR   = src
LEXER_DIR = $(SRC_DIR)/lexer
PARSE_DIR = $(SRC_DIR)/parser
# Output binary
TARGET = arion
# Source files
SRCS = $(SRC_DIR)/main.cpp \
       $(LEXER_DIR)/dfa.cpp \
       $(LEXER_DIR)/lexer.cpp \
       $(LEXER_DIR)/token.cpp \
       $(PARSER_DIR)/parse_tree.cpp
# Object files
OBJS = $(SRCS:.cpp=.o)
# Default target — build semua
run: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build sukses! Binary: ./$(TARGET)"
# Compile setiap .cpp jadi .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Clean selesai."
# Rebuild — clean lalu build ulang
rebuild: clean all
# Help — tampilkan cara pemakaian
help:
	@echo "========================================"
	@echo "  Arion Lexer — Makefile Commands"
	@echo "========================================"
	@echo "  make                   -> build program"
	@echo "  make run N=1           -> run dengan input1.txt"
	@echo "  make run N=2           -> run dengan input2.txt"
	@echo "  make run N=1 SAVE=1    -> run + simpan output1.txt"
	@echo "  make clean             -> hapus hasil build"
	@echo "  make rebuild           -> clean + build ulang"
	@echo "========================================"

.PHONY: all run clean rebuild help