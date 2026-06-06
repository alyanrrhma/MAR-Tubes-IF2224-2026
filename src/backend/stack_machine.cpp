#include "stack_machine.hpp"

#include <stdexcept>

namespace backend {

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

RuntimeValue StackMachine::stackAt(std::size_t index) const {
    if (index >= stack_.size()) {
        throw std::out_of_range("invalid stack access: index " +
                                std::to_string(index) +
                                " di luar ukuran stack " +
                                std::to_string(stack_.size()));
    }
    return stack_[index];
}

void StackMachine::setStackAt(std::size_t index, RuntimeValue value) {
    if (index >= stack_.size()) {
        throw std::out_of_range("invalid stack access: index " +
                                std::to_string(index) +
                                " di luar ukuran stack " +
                                std::to_string(stack_.size()));
    }
    stack_[index] = value;
}

void StackMachine::popTo(std::size_t index) {
    if (index > stack_.size()) {
        throw std::out_of_range("popTo: target index melebihi ukuran stack saat ini");
    }
    stack_.erase(stack_.begin() + index, stack_.end());
}

std::size_t StackMachine::stackTop() const {
    return stack_.size();
}

std::size_t StackMachine::walkStaticChain(std::size_t bp, int levels) const {
    std::size_t current = bp;
    for (int i = 0; i < levels; ++i) {
        current = static_cast<std::size_t>(stackAt(current).asInteger());
    }
    return current;
}

std::size_t StackMachine::stackSize() const {
    return stack_.size();
}

RuntimeValue StackMachine::defaultValue() {
    return RuntimeValue::integer(0);
}

} 
