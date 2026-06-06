#ifndef BACKEND_INSTRUCTION_HPP
#define BACKEND_INSTRUCTION_HPP

#include <iosfwd>
#include <string>
#include <vector>

namespace backend {

enum class OpCode {
    INT,
    LIT,
    LOD,
    STO,
    CAL,
    JMP,
    JPC,
    OPR,
    RET,
    LITB,  // push Boolean literal  (operand: 0=false, 1=true)
    LITS,  // push String from pool  (operand: string pool index)
    ADDR,  // push absolute stack address of variable (like LOD but yields address, not value)
    LODI,  // indirect load:  pop address, push stack[address]
    STOI   // indirect store: pop address, pop value, stack[address] = value
};

const char* toString(OpCode opcode);

enum class OprCode {
    NEG = 1,
    ADD = 2,
    SUB = 3,
    MUL = 4,
    DIV = 5,
    MOD = 6,
    EQL = 7,
    NEQ = 8,
    LSS = 9,
    GEQ = 10,
    GTR = 11,
    LEQ = 12,
    WRT = 13,
    WRTLN = 14,
    POP = 15
};

const char* toString(OprCode opcode);

class Instruction {
public:
    Instruction(OpCode opcode, int level, int operand);

    OpCode opcode;
    int level;
    int operand;
};

class InstructionProgram {
public:
    int emit(OpCode opcode, int level, int operand);
    int emit(const Instruction& instruction);

    void patch(int address, int operand);
    int currentAddress() const;

    // String pool: stores string literals; returns pool index for LITS operand
    int addString(const std::string& value);
    const std::string& getString(int index) const;

    const std::vector<Instruction>& getInstructions() const;

    void prettyPrint(std::ostream& out) const;
    std::string prettyPrint() const;

private:
    std::vector<Instruction> instructions_;
    std::vector<std::string> stringPool_;
};

}  

#endif
