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

    RuntimeValue executeOpr(int operand);
    RuntimeValue unaryNeg();
    RuntimeValue binaryArithmetic(OprCode opcode);
    RuntimeValue binaryComparison(OprCode opcode);

    int popInteger();
    bool popCondition();
};

}  // namespace backend

#endif
