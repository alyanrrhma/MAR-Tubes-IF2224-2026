#ifndef BACKEND_STACK_MACHINE_HPP
#define BACKEND_STACK_MACHINE_HPP

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace backend {

// Layout frame (relatif terhadap BP):
//
// BP+0 : Static Link    (BP frame yang secara leksikal melingkupi frame ini)
// BP+1 : Dynamic Link   (BP frame pemanggil — dipulihkan saat RET)
// BP+2 : Return Address (IP yang dilanjutkan setelah RET)
// BP+3+: Variable Area  (satu slot per variabel lokal yang dideklarasikan)
//
// Setiap pemanggilan CAL mendorong header tiga slot ini lalu memperbarui BP.
// RET membaca kembali header tersebut, melepas frame, dan memulihkan BP dan IP.

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
