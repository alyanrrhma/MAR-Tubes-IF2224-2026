#ifndef BACKEND_INTERPRETER_HPP
#define BACKEND_INTERPRETER_HPP

#include "instruction.hpp"
#include "stack_machine.hpp"

#include <iosfwd>
#include <string>

namespace backend {

class Interpreter {
public:
    void execute(const InstructionProgram& program, std::istream* input = nullptr);
    std::string getOutput() const;

private:
    StackMachine machine_;
    std::string output_;
    std::size_t bp_ = 0;
    std::size_t ip_ = 0;
    std::istream* input_ = nullptr;

    void validateInstruction(const Instruction& instruction, std::size_t programSize) const;
    void executeOpr(int operand);
    void executeRead(bool readLine);
    void unaryNeg();
    void binaryArithmetic(OprCode opcode);
    void binaryComparison(OprCode opcode);

    int popInteger();
    double popNumber(bool& isReal);
    bool popCondition();
};

}  

#endif
