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
        Boolean
    };

    static RuntimeValue integer(int value);
    static RuntimeValue boolean(bool value);

    Kind kind() const { return kind_; }
    int asInteger() const;
    bool asBoolean() const;

    std::string toString() const;

private:
    RuntimeValue(Kind kind, int value);

    Kind kind_;
    int value_;
};

class StackMachine {
public:
    void allocate(std::size_t count);

    void push(RuntimeValue value);
    RuntimeValue pop();
    const RuntimeValue& peek() const;

    RuntimeValue load(std::size_t address) const;
    void store(std::size_t address, RuntimeValue value);

    std::size_t stackSize() const;
    std::size_t memorySize() const;

private:
    static RuntimeValue defaultValue();

    std::vector<RuntimeValue> memory_;
    std::vector<RuntimeValue> stack_;
};

}  // namespace backend

#endif
