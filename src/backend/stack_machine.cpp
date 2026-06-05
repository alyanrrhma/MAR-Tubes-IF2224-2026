#include "stack_machine.hpp"

#include <stdexcept>

namespace backend {

// RuntimeValue merepresentasikan nilai yang dapat disimpan pada stack evaluasi maupun area memori interpreter
// Milestone 4 saat ini mendukung Integer dan Boolean.
RuntimeValue RuntimeValue::integer(int value) {
    return RuntimeValue(Kind::Integer, value);
}

RuntimeValue RuntimeValue::boolean(bool value) {
    return RuntimeValue(Kind::Boolean, value ? 1 : 0);
}

int RuntimeValue::asInteger() const {
    if (kind_ != Kind::Integer) {
        throw std::runtime_error("RuntimeValue bukan integer");
    }
    return value_;
}

bool RuntimeValue::asBoolean() const {
    if (kind_ != Kind::Boolean) {
        throw std::runtime_error("RuntimeValue bukan boolean");
    }
    return value_ != 0;
}

std::string RuntimeValue::toString() const {
    switch (kind_) {
    case Kind::Integer:
        return std::to_string(value_);
    case Kind::Boolean:
        return value_ != 0 ? "true" : "false";
    }
    return "?";
}

RuntimeValue::RuntimeValue(Kind kind, int value)
    : kind_(kind), value_(value) {}

void StackMachine::allocate(std::size_t count) {
    // Mengalokasikan area memori runtime
    // Seluruh slot diinisialisasi dengan nilai default (0)
    memory_.assign(count, defaultValue());
}

void StackMachine::push(RuntimeValue value) {
    if (stack_.size() >= MAX_STACK_SIZE) {
        throw std::runtime_error("stack overflow: ukuran stack melebihi batas runtime");
    }

    stack_.push_back(value);
}

RuntimeValue StackMachine::pop() {
    if (stack_.empty()) {
        throw std::runtime_error("stack underflow: pop dari stack kosong");
    }

    RuntimeValue value = stack_.back();
    stack_.pop_back();
    return value;
}

const RuntimeValue& StackMachine::peek() const {
    if (stack_.empty()) {
        throw std::runtime_error("stack underflow: peek dari stack kosong");
    }

    return stack_.back();
}

RuntimeValue StackMachine::load(std::size_t address) const {
    // LOD pada interpreter membaca nilai dari alamat absolut yang telah ditentukan oleh CodeGenerator
    if (address >= memory_.size()) {
        throw std::out_of_range("invalid memory access: load address " +
                                std::to_string(address) +
                                " di luar ukuran memory " +
                                std::to_string(memory_.size()));
    }

    return memory_[address];
}

void StackMachine::store(std::size_t address, RuntimeValue value) {
    // STO pada interpreter menulis nilai ke alamat memori runtime.
    if (address >= memory_.size()) {
        throw std::out_of_range("invalid memory access: store address " +
                                std::to_string(address) +
                                " di luar ukuran memory " +
                                std::to_string(memory_.size()));
    }

    memory_[address] = value;
}

std::size_t StackMachine::stackSize() const {
    return stack_.size();
}

std::size_t StackMachine::memorySize() const {
    return memory_.size();
}

RuntimeValue StackMachine::defaultValue() {
    // Seluruh variabel diinisialisasi ke integer 0 ketika area memori runtime dibuat
    return RuntimeValue::integer(0);
}

/*
Unit-style contoh penggunaan:

backend::StackMachine machine;
machine.allocate(5);                       // slot 0..4 tersedia
machine.store(3, backend::RuntimeValue::integer(10));
machine.push(machine.load(3));
machine.push(backend::RuntimeValue::integer(5));
auto rhs = machine.pop();
auto lhs = machine.pop();
machine.store(4, backend::RuntimeValue::integer(lhs.asInteger() + rhs.asInteger()));
*/

} 
