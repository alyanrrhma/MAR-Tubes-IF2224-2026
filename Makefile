# Makefile — Arion Compiler (Milestone 1: Lexical Analyzer)

# Compiler & flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
# Direktori
SRC_DIR   = src
LEXER_DIR = $(SRC_DIR)/lexer
# Output binary
TARGET = arion
# Source files
SRCS = $(SRC_DIR)/main.cpp \
       $(LEXER_DIR)/dfa.cpp \
       $(LEXER_DIR)/lexer.cpp \
       $(LEXER_DIR)/token.cpp
# Object files
OBJS = $(SRCS:.cpp=.o)
# Default target — build semua
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build sukses! Binary: ./$(TARGET)"
# Compile setiap .cpp jadi .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
# Run — jalankan lexer dengan input file
N    ?= 1
SAVE ?= 0
TEST_INPUT  = test/milestone1/input/input$(N).txt
TEST_OUTPUT = test/milestone1/output/output$(N).txt
run: $(TARGET)
ifeq ($(SAVE), 1)
	./$(TARGET) $(TEST_INPUT) -o $(TEST_OUTPUT)
	@echo "Output disimpan ke $(TEST_OUTPUT)"
else
	./$(TARGET) $(TEST_INPUT)
endif
# Clean — hapus semua file hasil build
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