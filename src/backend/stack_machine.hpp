#ifndef BACKEND_STACK_MACHINE_HPP
#define BACKEND_STACK_MACHINE_HPP

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace backend {

class RuntimeValue {
public:
    enum class Kind {
        Integer,
        Real,
        Boolean,
        String
    };

    static RuntimeValue integer(int value);
    static RuntimeValue real(double value);
    static RuntimeValue boolean(bool value);
    static RuntimeValue string(std::string value);

    Kind kind() const { return kind_; }
    int asInteger() const;
    double asReal() const;
    bool asBoolean() const;
    const std::string& asString() const;

    std::string toString() const;

private:
    RuntimeValue(Kind kind, int intValue, double realValue, std::string strValue);

    Kind kind_;
    int value_ = 0;
    double realValue_ = 0.0;
    std::string strValue_;
};

class StackMachine {
public:
    static constexpr std::size_t MAX_STACK_SIZE = 65536;

    void push(RuntimeValue value);
    RuntimeValue pop();
    const RuntimeValue& peek() const;

    RuntimeValue stackAt(std::size_t index) const;
    void setStackAt(std::size_t index, RuntimeValue value);
    void popTo(std::size_t index);

    std::size_t stackTop() const;
    std::size_t stackSize() const;

    std::size_t walkStaticChain(std::size_t bp, int levels) const;

private:
    static RuntimeValue defaultValue();

    std::vector<RuntimeValue> stack_;
};

} 

#endif
