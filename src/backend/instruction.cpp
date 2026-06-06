#include "instruction.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace backend {

namespace {

std::string escapePoolString(const std::string& value) {
    std::string out;
    for (char c : value) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        case '\r': out += "\\r"; break;
        case '"': out += "\\\""; break;
        default: out += c; break;
        }
    }
    return out;
}

}  // namespace

const char* toString(OpCode opcode) {
    switch (opcode) {
    case OpCode::INT: return "INT";
    case OpCode::LIT: return "LIT";
    case OpCode::LOD: return "LOD";
    case OpCode::STO: return "STO";
    case OpCode::CAL:  return "CAL";
    case OpCode::JMP:  return "JMP";
    case OpCode::JPC:  return "JPC";
    case OpCode::OPR:  return "OPR";
    case OpCode::RET:  return "RET";
    case OpCode::LITB: return "LITB";
    case OpCode::LITS: return "LITS";
    case OpCode::LITR: return "LITR";
    case OpCode::ADDR: return "ADDR";
    case OpCode::LODI: return "LODI";
    case OpCode::STOI: return "STOI";
    case OpCode::CHK:  return "CHK";
    }
    return "?";
}

const char* toString(OprCode opcode) {
    switch (opcode) {
    case OprCode::NEG: return "NEG";
    case OprCode::ADD: return "ADD";
    case OprCode::SUB: return "SUB";
    case OprCode::MUL: return "MUL";
    case OprCode::DIV: return "DIV";
    case OprCode::MOD: return "MOD";
    case OprCode::EQL: return "EQL";
    case OprCode::NEQ: return "NEQ";
    case OprCode::LSS: return "LSS";
    case OprCode::GEQ: return "GEQ";
    case OprCode::GTR: return "GTR";
    case OprCode::LEQ: return "LEQ";
    case OprCode::WRT: return "WRT";
    case OprCode::WRTLN: return "WRTLN";
    case OprCode::POP: return "POP";
    case OprCode::RDI: return "RDI";
    case OprCode::RDLN: return "RDLN";
    }
    return "?";
}

Instruction::Instruction(OpCode opcode, int level, int operand)
    : opcode(opcode), level(level), operand(operand) {}

int InstructionProgram::emit(OpCode opcode, int level, int operand) {
    return emit(Instruction(opcode, level, operand));
}

int InstructionProgram::emit(const Instruction& instruction) {
    instructions_.push_back(instruction);
    return static_cast<int>(instructions_.size()) - 1;
}

void InstructionProgram::patch(int address, int operand) {
    if (address < 0 || address >= static_cast<int>(instructions_.size())) {
        throw std::out_of_range("alamat instruksi untuk patch tidak valid");
    }
    instructions_[static_cast<std::size_t>(address)].operand = operand;
}

int InstructionProgram::currentAddress() const {
    return static_cast<int>(instructions_.size());
}

int InstructionProgram::addString(const std::string& value) {
    stringPool_.push_back(value);
    return static_cast<int>(stringPool_.size()) - 1;
}

const std::string& InstructionProgram::getString(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= stringPool_.size()) {
        throw std::out_of_range("string pool index out of range: " + std::to_string(index));
    }
    return stringPool_[static_cast<std::size_t>(index)];
}

int InstructionProgram::addReal(double value) {
    realPool_.push_back(value);
    return static_cast<int>(realPool_.size()) - 1;
}

double InstructionProgram::getReal(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= realPool_.size()) {
        throw std::out_of_range("real pool index out of range: " + std::to_string(index));
    }
    return realPool_[static_cast<std::size_t>(index)];
}

const std::vector<Instruction>& InstructionProgram::getInstructions() const {
    return instructions_;
}

void InstructionProgram::prettyPrint(std::ostream& out) const {
    for (std::size_t i = 0; i < stringPool_.size(); ++i) {
        out << "#STRING " << i << " \"" << escapePoolString(stringPool_[i]) << "\"\n";
    }
    for (std::size_t i = 0; i < realPool_.size(); ++i) {
        out << "#REAL " << i << ' ' << std::setprecision(17) << realPool_[i] << "\n";
    }
    for (std::size_t i = 0; i < instructions_.size(); ++i) {
        const auto& instruction = instructions_[i];
        out << i << ' '
            << toString(instruction.opcode) << ' '
            << instruction.level << ' '
            << instruction.operand;
        if (instruction.opcode == OpCode::OPR) {
            const OprCode opr = static_cast<OprCode>(instruction.operand);
            const char* name = toString(opr);
            if (std::string(name) != "?") {
                out << " ; " << name;
            }
        }
        out << '\n';
    }
}

std::string InstructionProgram::prettyPrint() const {
    std::ostringstream out;
    prettyPrint(out);
    return out.str();
}

}  
