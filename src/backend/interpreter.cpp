#include "interpreter.hpp"

#include <climits>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <sstream>
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

void Interpreter::execute(const InstructionProgram& program, std::istream* input) {
    machine_ = StackMachine{};
    output_.clear();
    bp_ = 0;
    ip_ = 0;
    input_ = input ? input : &std::cin;

    machine_.push(RuntimeValue::integer(0));
    machine_.push(RuntimeValue::integer(0));
    machine_.push(RuntimeValue::integer(0));
    bp_ = 0;

    const auto& instructions = program.getInstructions();
    bool running = true;

    while (running && ip_ < instructions.size()) {
        const Instruction instruction = instructions[ip_++];
        validateInstruction(instruction, instructions.size());

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
        case OpCode::LITR: {
            machine_.push(RuntimeValue::real(program.getReal(instruction.operand)));
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
        case OpCode::CHK: {
            const RuntimeValue indexValue = machine_.pop();
            const int index = (indexValue.kind() == RuntimeValue::Kind::Boolean)
                                  ? (indexValue.asBoolean() ? 1 : 0)
                                  : indexValue.asInteger();
            if (index < instruction.level || index > instruction.operand) {
                throw std::out_of_range("array index out of bounds: " +
                                        std::to_string(index) + " not in [" +
                                        std::to_string(instruction.level) + ".." +
                                        std::to_string(instruction.operand) + "]");
            }
            machine_.push(RuntimeValue::integer(index));
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

void Interpreter::validateInstruction(const Instruction& instruction, std::size_t programSize) const {
    auto requireLevelZero = [&](const char* name) {
        if (instruction.level != 0) {
            throw std::runtime_error(std::string("invalid instruction level for ") + name + ": " +
                                     std::to_string(instruction.level));
        }
    };

    switch (instruction.opcode) {
    case OpCode::INT:
        requireLevelZero("INT");
        if (instruction.operand < 3) {
            throw std::runtime_error("invalid INT operand: frame size must be at least 3");
        }
        break;
    case OpCode::LIT:
    case OpCode::LITB:
    case OpCode::LITS:
    case OpCode::LITR:
    case OpCode::JMP:
    case OpCode::JPC:
    case OpCode::OPR:
    case OpCode::LODI:
    case OpCode::STOI:
        requireLevelZero(toString(instruction.opcode));
        break;
    case OpCode::LOD:
    case OpCode::STO:
    case OpCode::CAL:
    case OpCode::ADDR:
        if (instruction.level < 0) {
            throw std::runtime_error(std::string("invalid instruction level for ") +
                                     toString(instruction.opcode) + ": " +
                                     std::to_string(instruction.level));
        }
        break;
    case OpCode::RET:
        if (instruction.level < 0) {
            throw std::runtime_error("invalid RET parameter count: " + std::to_string(instruction.level));
        }
        if (instruction.operand != 0 && instruction.operand != 1) {
            throw std::runtime_error("invalid RET return flag: " + std::to_string(instruction.operand));
        }
        break;
    case OpCode::CHK:
        if (instruction.level > instruction.operand) {
            throw std::runtime_error("invalid CHK bounds: lower bound exceeds upper bound");
        }
        break;
    }

    if ((instruction.opcode == OpCode::JMP || instruction.opcode == OpCode::JPC ||
         instruction.opcode == OpCode::CAL) &&
        (instruction.operand < 0 || static_cast<std::size_t>(instruction.operand) > programSize)) {
        throw std::out_of_range("invalid address: jump target di luar program");
    }
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
    case OprCode::RDI:
        executeRead(false);
        break;
    case OprCode::RDLN:
        executeRead(true);
        break;
    default:
        throw std::runtime_error("invalid opcode: OPR " + std::to_string(operand));
    }
}

void Interpreter::executeRead(bool readLine) {
    const int typeCode = popInteger();
    const int rawAddr = popInteger();
    if (rawAddr < 0 || static_cast<std::size_t>(rawAddr) >= machine_.stackTop()) {
        throw std::out_of_range("invalid address: RDI/RDLN target address out of range");
    }

    std::string text;
    if (readLine) {
        (*input_) >> std::ws;
        if (!std::getline(*input_, text)) {
            throw std::runtime_error("input error: readln gagal membaca nilai");
        }
    } else if (!(*input_ >> text)) {
        throw std::runtime_error("input error: read gagal membaca nilai");
    }

    RuntimeValue value = RuntimeValue::integer(0);
    try {
        switch (typeCode) {
        case 1:
            value = RuntimeValue::integer(std::stoi(text));
            break;
        case 2:
            value = RuntimeValue::real(std::stod(text));
            break;
        case 3:
            if (text.empty()) throw std::runtime_error("char kosong");
            value = RuntimeValue::integer(static_cast<unsigned char>(text.front()));
            break;
        case 4: {
            std::string lower = text;
            for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (lower != "true" && lower != "false") {
                throw std::runtime_error("boolean harus true atau false");
            }
            value = RuntimeValue::boolean(lower == "true");
            break;
        }
        case 5:
            value = RuntimeValue::string(text);
            break;
        default:
            throw std::runtime_error("type code read tidak dikenal: " + std::to_string(typeCode));
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("input error: nilai '" + text + "' tidak cocok dengan tipe target read/readln");
    }

    machine_.setStackAt(static_cast<std::size_t>(rawAddr), value);
}

void Interpreter::unaryNeg() {
    bool isReal = false;
    const double value = popNumber(isReal);
    if (isReal) {
        machine_.push(RuntimeValue::real(-value));
        return;
    }
    const long long result = -static_cast<long long>(value);
    if (result > INT_MAX || result < INT_MIN) {
        throw std::runtime_error("integer overflow");
    }
    machine_.push(RuntimeValue::integer(static_cast<int>(result)));
}

void Interpreter::binaryArithmetic(OprCode opcode) {
    bool rhsReal = false;
    const double rhs = popNumber(rhsReal);
    bool lhsReal = false;
    const double lhs = popNumber(lhsReal);
    const bool useReal = lhsReal || rhsReal || opcode == OprCode::DIV;

    if (opcode == OprCode::MOD && useReal) {
        throw std::runtime_error("operator mod membutuhkan operand integer");
    }

    if (useReal) {
        double result = 0.0;
        switch (opcode) {
        case OprCode::ADD: result = lhs + rhs; break;
        case OprCode::SUB: result = lhs - rhs; break;
        case OprCode::MUL: result = lhs * rhs; break;
        case OprCode::DIV:
            if (rhs == 0.0) throw std::runtime_error("division by zero");
            result = lhs / rhs;
            break;
        default: throw std::runtime_error("invalid opcode");
        }
        if (!std::isfinite(result)) {
            throw std::runtime_error("real overflow/underflow");
        }
        machine_.push(RuntimeValue::real(result));
        return;
    }

    const int rhsInt = static_cast<int>(rhs);
    const int lhsInt = static_cast<int>(lhs);
    long long result = 0;

    switch (opcode) {
    case OprCode::ADD: result = static_cast<long long>(lhsInt) + rhsInt; break;
    case OprCode::SUB: result = static_cast<long long>(lhsInt) - rhsInt; break;
    case OprCode::MUL: result = static_cast<long long>(lhsInt) * rhsInt; break;
    case OprCode::DIV:
        if (rhsInt == 0) throw std::runtime_error("division by zero");
        result = static_cast<long long>(lhsInt) / rhsInt;
        break;
    case OprCode::MOD:
        if (rhsInt == 0) throw std::runtime_error("division by zero");
        result = static_cast<long long>(lhsInt) % rhsInt;
        break;
    default: throw std::runtime_error("invalid opcode");
    }

    if (result > INT_MAX || result < INT_MIN) {
        throw std::runtime_error("integer overflow");
    }

    machine_.push(RuntimeValue::integer(static_cast<int>(result)));
}

void Interpreter::binaryComparison(OprCode opcode) {
    const RuntimeValue rhsVal = machine_.pop();
    const RuntimeValue lhsVal = machine_.pop();
    bool result = false;

    if (lhsVal.kind() == RuntimeValue::Kind::String ||
        rhsVal.kind() == RuntimeValue::Kind::String) {
        const std::string ls = lhsVal.toString();
        const std::string rs = rhsVal.toString();
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

    const double rhs = rhsVal.asReal();
    const double lhs = lhsVal.asReal();
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
    if (value.kind() == RuntimeValue::Kind::Real) {
        return static_cast<int>(value.asReal());
    }
    return value.asInteger();
}

double Interpreter::popNumber(bool& isReal) {
    const RuntimeValue value = machine_.pop();
    isReal = value.kind() == RuntimeValue::Kind::Real;
    return value.asReal();
}

bool Interpreter::popCondition() {
    const RuntimeValue value = machine_.pop();
    if (value.kind() == RuntimeValue::Kind::Boolean) {
        return value.asBoolean();
    }
    if (value.kind() == RuntimeValue::Kind::Real) {
        return value.asReal() != 0.0;
    }
    return value.asInteger() != 0;
}

}  
