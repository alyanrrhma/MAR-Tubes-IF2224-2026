#include "stack_machine.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace backend {

RuntimeValue RuntimeValue::integer(int value) {
    return RuntimeValue(Kind::Integer, value, 0.0, {});
}

RuntimeValue RuntimeValue::real(double value) {
    return RuntimeValue(Kind::Real, 0, value, {});
}

RuntimeValue RuntimeValue::boolean(bool value) {
    return RuntimeValue(Kind::Boolean, value ? 1 : 0, 0.0, {});
}

RuntimeValue RuntimeValue::string(std::string value) {
    return RuntimeValue(Kind::String, 0, 0.0, std::move(value));
}

int RuntimeValue::asInteger() const {
    if (kind_ != Kind::Integer) {
        throw std::runtime_error("RuntimeValue bukan integer");
    }
    return value_;
}

double RuntimeValue::asReal() const {
    if (kind_ == Kind::Real) {
        return realValue_;
    }
    if (kind_ == Kind::Integer) {
        return static_cast<double>(value_);
    }
    if (kind_ == Kind::Boolean) {
        return value_ != 0 ? 1.0 : 0.0;
    }
    throw std::runtime_error("RuntimeValue bukan real/numerik");
}

bool RuntimeValue::asBoolean() const {
    if (kind_ != Kind::Boolean) {
        throw std::runtime_error("RuntimeValue bukan boolean");
    }
    return value_ != 0;
}

const std::string& RuntimeValue::asString() const {
    if (kind_ != Kind::String) {
        throw std::runtime_error("RuntimeValue bukan string");
    }
    return strValue_;
}

std::string RuntimeValue::toString() const {
    switch (kind_) {
    case Kind::Integer:
        return std::to_string(value_);
    case Kind::Real: {
        std::ostringstream out;
        out << std::setprecision(15) << realValue_;
        std::string text = out.str();
        if (text.find('.') != std::string::npos) {
            while (!text.empty() && text.back() == '0') text.pop_back();
            if (!text.empty() && text.back() == '.') text.pop_back();
        }
        return text;
    }
    case Kind::Boolean:
        return value_ != 0 ? "true" : "false";
    case Kind::String:
        return strValue_;
    }
    return "?";
}

RuntimeValue::RuntimeValue(Kind kind, int intValue, double realValue, std::string strValue)
    : kind_(kind), value_(intValue), realValue_(realValue), strValue_(std::move(strValue)) {}

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
        // Static link tersimpan di current+0
        const int link = stackAt(current).asInteger();
        if (link < 0 || static_cast<std::size_t>(link) >= current) {
            throw std::runtime_error("stack corruption: invalid static link during chain walk");
        }
        current = static_cast<std::size_t>(link);
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
