#include "symbol_table.hpp"

#include <cctype>
#include <iomanip>
#include <ostream>
#include <utility>

namespace semantic {

const char* toString(ObjectKind kind) {
    switch (kind) {
    case ObjectKind::Undefined: return "undefined";
    case ObjectKind::Reserved:  return "reserved";
    case ObjectKind::Constant:  return "constant";
    case ObjectKind::Variable:  return "variable";
    case ObjectKind::Type:      return "type";
    case ObjectKind::Procedure: return "procedure";
    case ObjectKind::Function:  return "function";
    case ObjectKind::Program:   return "program";
    case ObjectKind::Parameter: return "parameter";
    case ObjectKind::Field:     return "field";
    }
    return "?";
}

const char* toString(TypeKind kind) {
    switch (kind) {
    case TypeKind::Unknown:    return "unknown";
    case TypeKind::Void:       return "void";
    case TypeKind::Error:      return "error";
    case TypeKind::Integer:    return "integer";
    case TypeKind::Real:       return "real";
    case TypeKind::Char:       return "char";
    case TypeKind::Boolean:    return "boolean";
    case TypeKind::String:     return "string";
    case TypeKind::Array:      return "array";
    case TypeKind::Record:     return "record";
    case TypeKind::Subrange:   return "subrange";
    case TypeKind::Enumerated: return "enumerated";
    }
    return "?";
}

bool SymbolTable::nameEquals(const std::string& lhs, const std::string& rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        const auto a = static_cast<unsigned char>(lhs[i]);
        const auto b = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(a) != std::tolower(b)) return false;
    }
    return true;
}

SymbolTable::SymbolTable() {
    tab_.push_back(TabEntry{});
    btab_.push_back(BTabEntry{});
    atab_.push_back(ATabEntry{});
    scopeStack_.push_back(0);
}

int SymbolTable::nextUserIndex() const {
    return static_cast<int>(tab_.size());
}

int SymbolTable::insert(const std::string& name,
                        ObjectKind obj,
                        TypeKind type,
                        int ref,
                        int adr,
                        bool nrm) {
    const int block = scopeStack_.empty() ? 0 : scopeStack_.back();

    TabEntry entry;
    entry.identifier = name;
    entry.link = btab_[block].last;
    entry.obj = obj;
    entry.type = type;
    entry.ref = ref;
    entry.nrm = nrm;
    entry.lev = level_;
    entry.adr = adr;

    tab_.push_back(std::move(entry));
    const int index = static_cast<int>(tab_.size()) - 1;
    btab_[block].last = index;
    return index;
}

int SymbolTable::lookup(const std::string& name) const {
    for (int i = static_cast<int>(scopeStack_.size()) - 1; i >= 0; --i) {
        int current = btab_[scopeStack_[i]].last;
        while (current != NO_INDEX) {
            if (nameEquals(tab_[current].identifier, name)) return current;
            current = tab_[current].link;
        }
    }
    return NO_INDEX;
}

int SymbolTable::lookupCurrentScope(const std::string& name) const {
    if (scopeStack_.empty()) return NO_INDEX;

    int current = btab_[scopeStack_.back()].last;
    while (current != NO_INDEX) {
        if (nameEquals(tab_[current].identifier, name)) return current;
        current = tab_[current].link;
    }
    return NO_INDEX;
}

int SymbolTable::pushBlock() {
    btab_.push_back(BTabEntry{});
    const int index = static_cast<int>(btab_.size()) - 1;
    scopeStack_.push_back(index);
    ++level_;
    return index;
}

void SymbolTable::popBlock() {
    if (scopeStack_.size() <= 1) return;
    scopeStack_.pop_back();
    if (level_ > 0) --level_;
}

int SymbolTable::currentBlock() const {
    return scopeStack_.empty() ? NO_INDEX : scopeStack_.back();
}

int SymbolTable::addArrayType(int low,
                              int high,
                              TypeKind xtyp,
                              TypeKind etyp,
                              int eref,
                              int elsz) {
    ATabEntry entry;
    entry.xtyp = xtyp;
    entry.etyp = etyp;
    entry.eref = eref;
    entry.low = low;
    entry.high = high;
    entry.elsz = elsz > 0 ? elsz : 1;

    const int count = high >= low ? high - low + 1 : 0;
    entry.size = count * entry.elsz;

    atab_.push_back(entry);
    return static_cast<int>(atab_.size()) - 1;
}

void SymbolTable::initPredefined() {
    if (predefinedInitialized_) return;

    static const char* reservedWords[] = {
        "and", "array", "begin", "case", "const", "div", "downto", "do",
        "else", "end", "for", "function", "if", "mod", "not", "of", "or",
        "procedure", "program", "record", "repeat", "then", "to", "type",
        "until", "var", "while"
    };

    for (const char* word : reservedWords) {
        insert(word, ObjectKind::Reserved, TypeKind::Unknown);
    }

    insert("integer", ObjectKind::Type, TypeKind::Integer, NO_INDEX, 1);
    insert("real", ObjectKind::Type, TypeKind::Real, NO_INDEX, 1);
    insert("char", ObjectKind::Type, TypeKind::Char, NO_INDEX, 1);
    insert("boolean", ObjectKind::Type, TypeKind::Boolean, NO_INDEX, 1);
    insert("string", ObjectKind::Type, TypeKind::String, NO_INDEX, 1);

    insert("false", ObjectKind::Constant, TypeKind::Boolean, NO_INDEX, 0);
    insert("true", ObjectKind::Constant, TypeKind::Boolean, NO_INDEX, 1);

    insert("writeln", ObjectKind::Procedure, TypeKind::Void);
    insert("readln", ObjectKind::Procedure, TypeKind::Void);

    firstUserIndex_ = static_cast<int>(tab_.size());
    predefinedInitialized_ = true;
}

void SymbolTable::printTab(std::ostream& out) const {
    out << "tab (identifier table)\n";
    out << "  idx  identifier         link  obj         type        ref  nrm  lev  adr\n";
    out << "  ---  ----------------   ----  ----------  ----------  ---  ---  ---  ---\n";

    for (std::size_t i = 0; i < tab_.size(); ++i) {
        const auto& entry = tab_[i];
        out << "  " << std::setw(3) << i
            << "  " << std::left << std::setw(16)
            << (entry.identifier.empty() ? "-" : entry.identifier) << std::right
            << "   " << std::setw(4) << entry.link
            << "  " << std::left << std::setw(10) << toString(entry.obj) << std::right
            << "  " << std::left << std::setw(10) << toString(entry.type) << std::right
            << "  " << std::setw(3) << entry.ref
            << "  " << std::setw(3) << (entry.nrm ? 1 : 0)
            << "  " << std::setw(3) << entry.lev
            << "  " << std::setw(3) << entry.adr
            << '\n';
    }

    out << "  firstUserIndex = " << firstUserIndex_ << '\n';
}

void SymbolTable::printBtab(std::ostream& out) const {
    out << "btab (block table)\n";
    out << "  idx  last  lpar  psze  vsze\n";
    out << "  ---  ----  ----  ----  ----\n";

    for (std::size_t i = 0; i < btab_.size(); ++i) {
        const auto& entry = btab_[i];
        out << "  " << std::setw(3) << i
            << "  " << std::setw(4) << entry.last
            << "  " << std::setw(4) << entry.lpar
            << "  " << std::setw(4) << entry.psze
            << "  " << std::setw(4) << entry.vsze
            << '\n';
    }
}

void SymbolTable::printAtab(std::ostream& out) const {
    out << "atab (array table)\n";
    out << "  idx  xtyp        etyp        eref   low  high  elsz  size\n";
    out << "  ---  ----------  ----------  ----  ----  ----  ----  ----\n";

    for (std::size_t i = 0; i < atab_.size(); ++i) {
        const auto& entry = atab_[i];
        out << "  " << std::setw(3) << i
            << "  " << std::left << std::setw(10) << toString(entry.xtyp) << std::right
            << "  " << std::left << std::setw(10) << toString(entry.etyp) << std::right
            << "  " << std::setw(4) << entry.eref
            << "  " << std::setw(4) << entry.low
            << "  " << std::setw(4) << entry.high
            << "  " << std::setw(4) << entry.elsz
            << "  " << std::setw(4) << entry.size
            << '\n';
    }
}

void SymbolTable::printAll(std::ostream& out) const {
    printTab(out);
    out << '\n';
    printBtab(out);
    out << '\n';
    printAtab(out);
}

} 
