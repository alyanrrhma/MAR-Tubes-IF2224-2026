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

}  

void Interpreter::execute(const InstructionProgram& program) {
    // Setiap eksekusi program dimulai dengan runtime state baru agar tidak ada data yang tersisa dari eksekusi sebelumnya
    machine_ = StackMachine{};
    output_.clear();

    const auto& instructions = program.getInstructions();
    std::size_t ip = 0;
    bool running = true;

    while (running && ip < instructions.size()) {
        // Siklus fetch-decode-execute:
        // 1. Ambil instruksi pada IP
        // 2. Decode opcode
        // 3. Jalankan operasi
        // 4. Perbarui IP bila diperlukan
        const Instruction instruction = instructions[ip++];

        switch (instruction.opcode) {
        case OpCode::INT: // Mengalokasikan area memori runtime sesuai ukuran frame yang dihitung saat code generation
            if (instruction.operand < 0) {
                throw std::out_of_range("invalid address: ukuran frame negatif");
            }
            machine_.allocate(static_cast<std::size_t>(instruction.operand));
            break;
        case OpCode::LIT: // Push literal ke evaluation stack
            machine_.push(RuntimeValue::integer(instruction.operand));
            break;
        case OpCode::LOD: // LOD: membaca nilai dari memori runtime ke evaluation stack
            machine_.push(machine_.load(checkedAddress(instruction.operand,
                                                       machine_.memorySize(),
                                                       "load")));
            break;
        case OpCode::STO: // STO: menyimpan nilai dari evaluation stack ke memori runtime
            machine_.store(checkedAddress(instruction.operand,
                                          machine_.memorySize(),
                                          "store"),
                           machine_.pop());
            break;
        case OpCode::JMP:
            ip = checkedJumpTarget(instruction.operand, instructions.size());
            break;
        case OpCode::JPC: // JPC melakukan lompatan hanya ketika kondisi bernilai false
            if (!popCondition()) {
                ip = checkedJumpTarget(instruction.operand, instructions.size());
            }
            break;
        case OpCode::OPR:
            executeOpr(instruction.operand);
            break;
        case OpCode::RET:
            running = false;
            break;
        case OpCode::CAL: // CALL procedure/function belum diimplementasikan pada subset Milestone 4 yang digunakan interpreter ini
            throw std::runtime_error("invalid opcode: CAL belum didukung interpreter");
        default:
            throw std::runtime_error("invalid opcode");
        }
    }
}

std::string Interpreter::getOutput() const {
    return output_;
}

RuntimeValue Interpreter::executeOpr(int operand) { // Menjalankan seluruh operasi OPR sesuai tabel opcode pada spesifikasi Milestone 4
    const OprCode opcode = static_cast<OprCode>(operand);

    switch (opcode) {
    case OprCode::NEG:
        return unaryNeg();
    case OprCode::ADD:
    case OprCode::SUB:
    case OprCode::MUL:
    case OprCode::DIV:
    case OprCode::MOD:
        return binaryArithmetic(opcode); // Operasi aritmetika selalu mengambil dua operand dari evaluation stack lalu mendorong hasilnya kembali
    case OprCode::EQL:
    case OprCode::NEQ:
    case OprCode::LSS:
    case OprCode::GEQ:
    case OprCode::GTR:
    case OprCode::LEQ:
        return binaryComparison(opcode); // Operasi perbandingan menghasilkan RuntimeValue bertipe Boolean yang kemudian dapat digunakan oleh JPC
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
    const int value = popInteger(); // Boolean direpresentasikan sebagai 0/1 pada runtime, sehingga dapat diperlakukan sebagai integer ketika diperlukan
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
    // Mengevaluasi nilai kondisi untuk instruksi JPC
    // Boolean digunakan langsung, sedangkan integer dianggap true jika tidak bernilai nol
    const RuntimeValue value = machine_.pop();
    if (value.kind() == RuntimeValue::Kind::Boolean) {
        return value.asBoolean();
    }
    return value.asInteger() != 0;
}

}  
