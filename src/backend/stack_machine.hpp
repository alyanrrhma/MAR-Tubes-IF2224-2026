#ifndef BACKEND_STACK_MACHINE_HPP
#define BACKEND_STACK_MACHINE_HPP

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace backend {

// Layout memori runtime:
//
// 0 : Static Link
// 1 : Dynamic Link
// 2 : Return Address
// 3+ : Variable Area
//
// Layout ini mengikuti model activation record
// yang digunakan pada spesifikasi Milestone 4.

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
    static constexpr std::size_t MAX_STACK_SIZE = 65536;

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

} 

#endif
