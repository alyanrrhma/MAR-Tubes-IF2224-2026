#include "interpreter.hpp"

#include <cstddef>
#include <stdexcept>

namespace backend {

namespace {

std::size_t checkedAddress(int address, std::size_t limit, const char* context) {
    // Memastikan alamat memori yang digunakan instruksi LOD/STO berada dalam area memori runtime yang valid
    if (address < 0 || static_cast<std::size_t>(address) >= limit) {
        throw std::out_of_range(std::string("invalid address: ") + context);
    }
    return static_cast<std::size_t>(address);
}

std::size_t checkedJumpTarget(int address, std::size_t limit) {
    // Memastikan target JMP/JPC menunjuk ke alamat instruksi yang valid sebelum instruction pointer diperbarui/ namespace
    if (address < 0 || static_cast<std::size_t>(address) > limit) {
        throw std::out_of_range("invalid address: jump target di luar program");
    }
    return static_cast<std::size_t>(address);
}

const int FRAME_HEADER_SIZE = 3;

}  

void Interpreter::execute(const InstructionProgram& program) {
    // Setiap eksekusi program dimulai dengan runtime state baru
    machine_ = StackMachine{};
    output_.clear();
    bp_ = 0;
    ip_ = 0;

    // Dorong tiga slot header sentinel untuk frame program utama:
    // static link = 0, dynamic link = 0, return address = 0
    machine_.push(RuntimeValue::integer(0)); // BP+0: static link
    machine_.push(RuntimeValue::integer(0)); // BP+1: dynamic link
    machine_.push(RuntimeValue::integer(0)); // BP+2: return address
    bp_ = 0;

    const auto& instructions = program.getInstructions();
    bool running = true;

    while (running && ip_ < instructions.size()) {
        // Siklus fetch-decode-execute
        const Instruction instruction = instructions[ip_++];

        switch (instruction.opcode) {
        case OpCode::INT: {
            // Alokasikan slot variabel lokal untuk frame saat ini.
            // Header tiga slot sudah ada (didorong oleh CAL atau sentinel program utama),
            // jadi INT hanya mendorong slot variabel sebanyak (operand - FRAME_HEADER_SIZE).
            if (instruction.operand < 0) {
                throw std::out_of_range("invalid address: ukuran frame negatif");
            }
            const int varCount = instruction.operand - 3; // 3 = FRAME_HEADER_SIZE
            for (int i = 0; i < varCount; ++i) {
                machine_.push(RuntimeValue::integer(0));
            }
            break;
        }
        case OpCode::LIT:
            machine_.push(RuntimeValue::integer(instruction.operand));
            break;
        case OpCode::LOD: {
            // Temukan frame tempat variabel dideklarasikan lewat static-link chain,
            // lalu baca nilai dari slot variabel relative terhadap BP frame tersebut.
            const std::size_t frameBase = machine_.walkStaticChain(bp_, instruction.level);
            if (instruction.operand < FRAME_HEADER_SIZE) {
                throw std::out_of_range("invalid address: LOD/STO operand points into frame header");
            }

            // bp_ sebagai batas atas untuk menghindari pembacaan lintas frame
            const std::size_t frameTop = (frameBase == bp_) ? machine_.stackTop() : bp_;
            const std::size_t addr = checkedAddress(
                static_cast<int>(frameBase) + instruction.operand,
                frameTop, "load");
            machine_.push(machine_.stackAt(addr));
            break;
        }
        case OpCode::STO: {
            const std::size_t frameBase = machine_.walkStaticChain(bp_, instruction.level);
            if (instruction.operand < FRAME_HEADER_SIZE) {
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
            // Dorong header frame baru: static link, dynamic link, return address.
            // instruction.level  = selisih level leksikal antara pemanggil dan callee
            // instruction.operand = alamat instruksi pertama callee
            const std::size_t staticLink = machine_.walkStaticChain(bp_, instruction.level);
            machine_.push(RuntimeValue::integer(static_cast<int>(staticLink))); // BP+0
            machine_.push(RuntimeValue::integer(static_cast<int>(bp_)));        // BP+1: dynamic link
            machine_.push(RuntimeValue::integer(static_cast<int>(ip_)));        // BP+2: return address
            bp_ = machine_.stackTop() - 3;
            ip_ = checkedJumpTarget(instruction.operand, instructions.size());
            break;
        }
        case OpCode::RET: {
            // Baca header frame saat ini, lepas frame, pulihkan caller.
            const int returnAddr = machine_.stackAt(bp_ + 2).asInteger();
            const int callerBp   = machine_.stackAt(bp_ + 1).asInteger();
            if (callerBp < 0 || static_cast<std::size_t>(callerBp) >= bp_) {
                throw std::runtime_error("stack corruption: invalid dynamic link in frame header");
            }
            machine_.popTo(bp_);
            if (bp_ == 0 && callerBp == 0) {
                // Frame program utama — tidak ada caller, hentikan eksekusi
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
}

int Interpreter::popInteger() {
    const RuntimeValue value = machine_.pop();
    if (value.kind() == RuntimeValue::Kind::Boolean) {
        return value.asBoolean() ? 1 : 0;
    }
    return value.asInteger();
}

bool Interpreter::popCondition() {
    // Mengevaluasi nilai kondisi untuk instruksi JPC
    // Boolean digunakan langsung, sedangkan integer dianggap true jika tidak bernilai nol
    const RuntimeValue value = machine_.pop();
    if (value.kind() == RuntimeValue::Kind::Boolean) {
        return value.asBoolean();
    }
    return value.asInteger() != 0;
}

}  
