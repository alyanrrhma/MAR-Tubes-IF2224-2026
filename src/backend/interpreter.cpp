#include "interpreter.hpp"
#include <climits>

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

const int FRAME_HEADER_SIZE = 3;

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
            // Tolak operand yang menunjuk ke slot header (0,1,2). Operand negatif
            // diizinkan karena menunjuk ke parameter yang didorong caller sebelum header.
            if (instruction.operand >= 0 && instruction.operand < FRAME_HEADER_SIZE) {
                throw std::out_of_range("invalid address: LOD/STO operand points into frame header");
            }
            // bp_ sebagai batas atas untuk frame ancestor, mencegah akses lintas frame
            const std::size_t frameTop = (frameBase == bp_) ? machine_.stackTop() : bp_;
            const std::size_t addr = checkedAddress(
                static_cast<int>(frameBase) + instruction.operand,
                frameTop, "load");
            machine_.push(machine_.stackAt(addr));
            break;
        }
        case OpCode::STO: {
            const std::size_t frameBase = machine_.walkStaticChain(bp_, instruction.level);
            if (instruction.operand >= 0 && instruction.operand < FRAME_HEADER_SIZE) {
                throw std::out_of_range("invalid address: LOD/STO operand points into frame header");
            }
            const std::size_t frameTop = (frameBase == bp_) ? machine_.stackTop() : bp_;
            const std::size_t addr = checkedAddress(
                static_cast<int>(frameBase) + instruction.operand,
                frameTop, "store");
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
            // instruction.level  = jumlah parameter yang didorong caller (dibersihkan di sini)
            // instruction.operand = 1 jika fungsi mengembalikan nilai (di atas stack), 0 jika tidak
            const bool hasReturn = (instruction.operand == 1);
            const int psze = instruction.level;

            if (psze < 0 || static_cast<std::size_t>(psze) > bp_) {
                throw std::runtime_error("invalid frame: parameter count exceeds frame base");
            }

            // Simpan nilai kembalian sebelum frame dibongkar
            RuntimeValue retVal = RuntimeValue::integer(0);
            if (hasReturn) retVal = machine_.pop();

            const int returnAddr = machine_.stackAt(bp_ + 2).asInteger();
            const int callerBp   = machine_.stackAt(bp_ + 1).asInteger();

            // cleanupBase: posisi stack setelah frame DAN parameter caller dibersihkan
            const std::size_t cleanupBase = bp_ - static_cast<std::size_t>(psze);

            // Validasi dynamic link hanya untuk frame non-utama
            if (bp_ > 0 && (callerBp < 0 || static_cast<std::size_t>(callerBp) >= cleanupBase)) {
                throw std::runtime_error("stack corruption: invalid dynamic link in frame header");
            }

            machine_.popTo(cleanupBase);

            if (bp_ == 0 && callerBp == 0) {
                running = false;
            } else {
                bp_ = static_cast<std::size_t>(callerBp);
                ip_ = static_cast<std::size_t>(returnAddr);
                if (hasReturn) machine_.push(retVal);
            }
            break;
        }
        case OpCode::LITB:
            machine_.push(RuntimeValue::boolean(instruction.operand != 0));
            break;
        case OpCode::LITS: {
            machine_.push(RuntimeValue::string(program.getString(instruction.operand)));
            break;
        }
        case OpCode::ADDR: {
            // Dorong alamat absolut slot variabel ke stack sebagai Integer
            const std::size_t frameBase = machine_.walkStaticChain(bp_, instruction.level);
            const int absAddr = static_cast<int>(frameBase) + instruction.operand;
            if (absAddr < 0) {
                throw std::out_of_range("invalid address: ADDR result is negative");
            }
            machine_.push(RuntimeValue::integer(absAddr));
            break;
        }
        case OpCode::LODI: {
            // Baca nilai dari alamat absolut yang ada di atas stack
            const int rawAddr = machine_.pop().asInteger();
            if (rawAddr < 0) {
                throw std::out_of_range("invalid address: LODI address is negative");
            }
            const std::size_t addr = static_cast<std::size_t>(rawAddr);
            if (addr >= machine_.stackTop()) {
                throw std::out_of_range("invalid address: LODI address out of range");
            }
            machine_.push(machine_.stackAt(addr));
            break;
        }
        case OpCode::STOI: {
            // Tulis nilai ke alamat absolut: pop alamat, lalu pop nilai, simpan
            const int rawAddr = machine_.pop().asInteger();
            if (rawAddr < 0) {
                throw std::out_of_range("invalid address: STOI address is negative");
            }
            const std::size_t addr = static_cast<std::size_t>(rawAddr);
            if (addr >= machine_.stackTop()) {
                throw std::out_of_range("invalid address: STOI address out of range");
            }
            machine_.setStackAt(addr, machine_.pop());
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

void Interpreter::executeOpr(int operand) { // Menjalankan seluruh operasi OPR sesuai tabel opcode pada spesifikasi Milestone 4
    const OprCode opcode = static_cast<OprCode>(operand);

    switch (opcode) {
    case OprCode::NEG:
        unaryNeg();
        break;
    case OprCode::ADD:
    case OprCode::SUB:
    case OprCode::MUL:
    case OprCode::DIV:
    case OprCode::MOD:
        binaryArithmetic(opcode); // Operasi aritmetika selalu mengambil dua operand dari evaluation stack lalu mendorong hasilnya kembali
        break;
    case OprCode::EQL:
    case OprCode::NEQ:
    case OprCode::LSS:
    case OprCode::GEQ:
    case OprCode::GTR:
    case OprCode::LEQ:
        binaryComparison(opcode); // Operasi perbandingan menghasilkan RuntimeValue bertipe Boolean yang kemudian dapat digunakan oleh JPC
        break;
    case OprCode::WRT: {
        const RuntimeValue value = machine_.pop();
        output_ += value.toString();
        break;
    }
    case OprCode::WRTLN: {
        const RuntimeValue value = machine_.pop();
        output_ += value.toString();
        output_ += '\n';
        break;
    }
    case OprCode::POP:
        machine_.pop();
        break;
    default:
        throw std::runtime_error("invalid opcode: OPR " + std::to_string(operand));
    }
}

void Interpreter::unaryNeg() {
    const int value = popInteger(); // Boolean direpresentasikan sebagai 0/1 pada runtime, sehingga dapat diperlakukan sebagai integer ketika diperlukan
    const RuntimeValue result = RuntimeValue::integer(-value);
    machine_.push(result);
}

void Interpreter::binaryArithmetic(OprCode opcode) {
    const int rhs = popInteger();
    const int lhs = popInteger();
    long long result = 0;

    switch (opcode) {
    case OprCode::ADD:
        result = static_cast<long long>(lhs) + rhs;
        break;
    case OprCode::SUB:
        result = static_cast<long long>(lhs) - rhs;
        break;
    case OprCode::MUL:
        result = static_cast<long long>(lhs) * rhs;
        break;
    case OprCode::DIV:
        if (rhs == 0) {
            throw std::runtime_error("division by zero");
        }
        result = static_cast<long long>(lhs) / rhs;
        break;
    case OprCode::MOD:
        if (rhs == 0) {
            throw std::runtime_error("division by zero");
        }
        result = static_cast<long long>(lhs) % rhs;
        break;
    default:
        throw std::runtime_error("invalid opcode");
    }

    if (result > INT_MAX || result < INT_MIN) {
        throw std::runtime_error("integer overflow");
    }

    const auto value = RuntimeValue::integer(static_cast<int>(result));
    machine_.push(value);
}

void Interpreter::binaryComparison(OprCode opcode) {
    const RuntimeValue rhsVal = machine_.pop();
    const RuntimeValue lhsVal = machine_.pop();
    bool result = false;

    // String equality/inequality
    if (lhsVal.kind() == RuntimeValue::Kind::String ||
        rhsVal.kind() == RuntimeValue::Kind::String) {
        const std::string& ls = lhsVal.toString();
        const std::string& rs = rhsVal.toString();
        switch (opcode) {
        case OprCode::EQL: result = ls == rs; break;
        case OprCode::NEQ: result = ls != rs; break;
        case OprCode::LSS: result = ls <  rs; break;
        case OprCode::GEQ: result = ls >= rs; break;
        case OprCode::GTR: result = ls >  rs; break;
        case OprCode::LEQ: result = ls <= rs; break;
        default: throw std::runtime_error("invalid opcode");
        }
        machine_.push(RuntimeValue::boolean(result));
        return;
    }

    // Numeric comparison (integer or boolean-as-integer)
    const int rhs = (rhsVal.kind() == RuntimeValue::Kind::Boolean) ? (rhsVal.asBoolean() ? 1 : 0) : rhsVal.asInteger();
    const int lhs = (lhsVal.kind() == RuntimeValue::Kind::Boolean) ? (lhsVal.asBoolean() ? 1 : 0) : lhsVal.asInteger();

    switch (opcode) {
    case OprCode::EQL: result = lhs == rhs; break;
    case OprCode::NEQ: result = lhs != rhs; break;
    case OprCode::LSS: result = lhs <  rhs; break;
    case OprCode::GEQ: result = lhs >= rhs; break;
    case OprCode::GTR: result = lhs >  rhs; break;
    case OprCode::LEQ: result = lhs <= rhs; break;
    default: throw std::runtime_error("invalid opcode");
    }

    machine_.push(RuntimeValue::boolean(result));
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
