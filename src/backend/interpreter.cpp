#include "interpreter.hpp"

#include <cstddef>
#include <stdexcept>

namespace backend {

namespace {

std::size_t checkedAddress(int address, std::size_t limit, const char* context) {
    if (address < 0 || static_cast<std::size_t>(address) >= limit) {
        throw std::out_of_range(std::string("invalid address: ") + context);
    }
    return static_cast<std::size_t>(address);
}

std::size_t checkedJumpTarget(int address, std::size_t limit) {
    if (address < 0 || static_cast<std::size_t>(address) > limit) {
        throw std::out_of_range("invalid address: jump target di luar program");
    }
    return static_cast<std::size_t>(address);
}

}  

void Interpreter::execute(const InstructionProgram& program) {
    machine_ = StackMachine{};
    output_.clear();
    bp_ = 0;
    ip_ = 0;

    machine_.push(RuntimeValue::integer(0));
    machine_.push(RuntimeValue::integer(0));
    machine_.push(RuntimeValue::integer(0));
    bp_ = 0;

    const auto& instructions = program.getInstructions();
    bool running = true;

    while (running && ip_ < instructions.size()) {
        const Instruction instruction = instructions[ip_++];

        switch (instruction.opcode) {
        case OpCode::INT: {
            if (instruction.operand < 0) {
                throw std::out_of_range("invalid address: ukuran frame negatif");
            }
            const int varCount = instruction.operand - 3;
            for (int i = 0; i < varCount; ++i) {
                machine_.push(RuntimeValue::integer(0));
            }
            break;
        }
        case OpCode::LIT:
            machine_.push(RuntimeValue::integer(instruction.operand));
            break;
        case OpCode::LOD: {
            const std::size_t frameBase = machine_.walkStaticChain(bp_, instruction.level);
            const std::size_t addr = checkedAddress(
                static_cast<int>(frameBase) + instruction.operand,
                machine_.stackTop(), "load");
            machine_.push(machine_.stackAt(addr));
            break;
        }
        case OpCode::STO: {
            const std::size_t frameBase = machine_.walkStaticChain(bp_, instruction.level);
            const std::size_t addr = checkedAddress(
                static_cast<int>(frameBase) + instruction.operand,
                machine_.stackTop(), "store");
            machine_.setStackAt(addr, machine_.pop());
            break;
        }
        case OpCode::JMP:
            ip_ = checkedJumpTarget(instruction.operand, instructions.size());
            break;
        case OpCode::JPC:
            if (!popCondition()) {
                ip_ = checkedJumpTarget(instruction.operand, instructions.size());
            }
            break;
        case OpCode::OPR:
            executeOpr(instruction.operand);
            break;
        case OpCode::CAL: {
            const std::size_t staticLink = machine_.walkStaticChain(bp_, instruction.level);
            machine_.push(RuntimeValue::integer(static_cast<int>(staticLink)));
            machine_.push(RuntimeValue::integer(static_cast<int>(bp_)));
            machine_.push(RuntimeValue::integer(static_cast<int>(ip_)));
            bp_ = machine_.stackTop() - 3;
            ip_ = checkedJumpTarget(instruction.operand, instructions.size());
            break;
        }
        case OpCode::RET: {
            const int returnAddr = machine_.stackAt(bp_ + 2).asInteger();
            const int callerBp   = machine_.stackAt(bp_ + 1).asInteger();
            machine_.popTo(bp_);
            if (bp_ == 0 && callerBp == 0) {
                running = false;
            } else {
                bp_ = static_cast<std::size_t>(callerBp);
                ip_ = static_cast<std::size_t>(returnAddr);
            }
            break;
        }
        default:
            throw std::runtime_error("invalid opcode");
        }
    }
}

std::string Interpreter::getOutput() const {
    return output_;
}

RuntimeValue Interpreter::executeOpr(int operand) {
    const OprCode opcode = static_cast<OprCode>(operand);

    switch (opcode) {
    case OprCode::NEG:
        return unaryNeg();
    case OprCode::ADD:
    case OprCode::SUB:
    case OprCode::MUL:
    case OprCode::DIV:
    case OprCode::MOD:
        return binaryArithmetic(opcode);
    case OprCode::EQL:
    case OprCode::NEQ:
    case OprCode::LSS:
    case OprCode::GEQ:
    case OprCode::GTR:
    case OprCode::LEQ:
        return binaryComparison(opcode);
    case OprCode::WRT: {
        const RuntimeValue value = machine_.pop();
        output_ += value.toString();
        return value;
    }
    case OprCode::WRTLN: {
        const RuntimeValue value = machine_.pop();
        output_ += value.toString();
        output_ += '\n';
        return value;
    }
    default:
        throw std::runtime_error("invalid opcode: OPR " + std::to_string(operand));
    }
}

RuntimeValue Interpreter::unaryNeg() {
    const int value = popInteger();
    const RuntimeValue result = RuntimeValue::integer(-value);
    machine_.push(result);
    return result;
}

RuntimeValue Interpreter::binaryArithmetic(OprCode opcode) {
    const int rhs = popInteger();
    const int lhs = popInteger();
    int result = 0;

    switch (opcode) {
    case OprCode::ADD:
        result = lhs + rhs;
        break;
    case OprCode::SUB:
        result = lhs - rhs;
        break;
    case OprCode::MUL:
        result = lhs * rhs;
        break;
    case OprCode::DIV:
        if (rhs == 0) {
            throw std::runtime_error("division by zero");
        }
        result = lhs / rhs;
        break;
    case OprCode::MOD:
        if (rhs == 0) {
            throw std::runtime_error("division by zero");
        }
        result = lhs % rhs;
        break;
    default:
        throw std::runtime_error("invalid opcode");
    }

    const RuntimeValue value = RuntimeValue::integer(result);
    machine_.push(value);
    return value;
}

RuntimeValue Interpreter::binaryComparison(OprCode opcode) {
    const int rhs = popInteger();
    const int lhs = popInteger();
    bool result = false;

    switch (opcode) {
    case OprCode::EQL:
        result = lhs == rhs;
        break;
    case OprCode::NEQ:
        result = lhs != rhs;
        break;
    case OprCode::LSS:
        result = lhs < rhs;
        break;
    case OprCode::GEQ:
        result = lhs >= rhs;
        break;
    case OprCode::GTR:
        result = lhs > rhs;
        break;
    case OprCode::LEQ:
        result = lhs <= rhs;
        break;
    default:
        throw std::runtime_error("invalid opcode");
    }

    const RuntimeValue value = RuntimeValue::boolean(result);
    machine_.push(value);
    return value;
}

int Interpreter::popInteger() {
    const RuntimeValue value = machine_.pop();
    if (value.kind() == RuntimeValue::Kind::Boolean) {
        return value.asBoolean() ? 1 : 0;
    }
    return value.asInteger();
}

bool Interpreter::popCondition() {
    const RuntimeValue value = machine_.pop();
    if (value.kind() == RuntimeValue::Kind::Boolean) {
        return value.asBoolean();
    }
    return value.asInteger() != 0;
}

}  
