#ifndef SEMANTIC_SYMBOL_TABLE_HPP
#define SEMANTIC_SYMBOL_TABLE_HPP

#include <iosfwd>
#include <string>
#include <vector>

namespace semantic {

constexpr int NO_INDEX = -1;

// Index convention:
// - tab[0] is an empty sentinel. Valid lookup results are never 0.
// - btab[0] is the global/predefined block.
// - atab[0] is a "no array" sentinel.
// - NO_INDEX means no link/reference/lookup result.

enum class ObjectKind {
    Undefined,
    Reserved,
    Constant,
    Variable,
    Type,
    Procedure,
    Function,
    Program,
    Parameter,
    Field
};

const char* toString(ObjectKind kind);

enum class TypeKind {
    Unknown,
    Void,
    Error,
    Integer,
    Real,
    Char,
    Boolean,
    String,
    Array,
    Record,
    Subrange,
    Enumerated
};

const char* toString(TypeKind kind);

struct TabEntry {
    std::string identifier;
    int link = NO_INDEX;              
    ObjectKind obj = ObjectKind::Undefined;
    TypeKind type = TypeKind::Unknown;
    int ref = NO_INDEX;              
    bool nrm = true;                 
    int lev = 0;
    int adr = 0;
};

struct BTabEntry {
    int last = NO_INDEX;            
    int lpar = NO_INDEX;             
    int psze = 0;
    int vsze = 0;
};

struct ATabEntry {
    TypeKind xtyp = TypeKind::Unknown;
    TypeKind etyp = TypeKind::Unknown;
    int eref = NO_INDEX;
    int low = 0;
    int high = 0;
    int elsz = 1;
    int size = 0;
};

class SymbolTable {
public:
    SymbolTable();

    void initPredefined();

    int firstUserIndex() const { return firstUserIndex_; }
    int nextUserIndex() const;

    int insert(const std::string& name,
               ObjectKind obj,
               TypeKind type,
               int ref = NO_INDEX,
               int adr = 0,
               bool nrm = true);

    int lookup(const std::string& name) const;
    int lookupCurrentScope(const std::string& name) const;

    int pushBlock();
    void popBlock();

    int currentBlock() const;
    int currentLevel() const { return level_; }

    int addArrayType(int low,
                     int high,
                     TypeKind xtyp,
                     TypeKind etyp,
                     int eref = NO_INDEX,
                     int elsz = 1);

    const TabEntry& tabAt(int index) const { return tab_[index]; }
    const BTabEntry& btabAt(int index) const { return btab_[index]; }
    const ATabEntry& atabAt(int index) const { return atab_[index]; }

    TabEntry& tabAt(int index) { return tab_[index]; }
    BTabEntry& btabAt(int index) { return btab_[index]; }
    ATabEntry& atabAt(int index) { return atab_[index]; }

    int tabSize() const { return static_cast<int>(tab_.size()); }
    int btabSize() const { return static_cast<int>(btab_.size()); }
    int atabSize() const { return static_cast<int>(atab_.size()); }

    void printTab(std::ostream& out) const;
    void printBtab(std::ostream& out) const;
    void printAtab(std::ostream& out) const;
    void printAll(std::ostream& out) const;

private:
    static bool nameEquals(const std::string& lhs, const std::string& rhs);

    std::vector<TabEntry> tab_;
    std::vector<BTabEntry> btab_;
    std::vector<ATabEntry> atab_;
    std::vector<int> scopeStack_;

    int level_ = 0;
    int firstUserIndex_ = 1;
    bool predefinedInitialized_ = false;
};

} 

#endif
