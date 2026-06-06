#ifndef BACKEND_INTERPRETER_HPP
#define BACKEND_INTERPRETER_HPP

#include "instruction.hpp"
#include "stack_machine.hpp"

#include <string>

namespace backend {

class Interpreter {
public:
    void execute(const InstructionProgram& program);
    std::string getOutput() const;

private:
    StackMachine machine_;
    std::string output_;
    std::size_t bp_ = 0;
    std::size_t ip_ = 0;

    void executeOpr(int operand);
    void unaryNeg();
    void binaryArithmetic(OprCode opcode);
    void binaryComparison(OprCode opcode);

    int popInteger();
    bool popCondition();
};

}  

#endif
