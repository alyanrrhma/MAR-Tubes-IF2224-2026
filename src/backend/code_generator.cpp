#include "code_generator.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace backend {

namespace {

std::string unsupported(const semantic::AstNode& node) {
    // Membantu menghasilkan pesan error yang konsisten ketika backend menemukan node AST yang belum didukung
    return std::string("CodeGenerator belum mendukung node ") + node.kindName();
}

bool isExecutableDeclaration(semantic::AstKind kind) {
    return kind == semantic::AstKind::ProcDecl ||
           kind == semantic::AstKind::FuncDecl;
}

std::string lowerName(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

} 

CodeGenerator::CodeGenerator() = default;

InstructionProgram CodeGenerator::generate(const semantic::AstNode* root,
                                           const semantic::SymbolTable& symbolTable) {
    // Titik masuk utama code generation.
    // Input berupa Decorated AST dan Symbol Table hasil Milestone 3, sedangkan output berupa program TAC yang siap diinterpretasikan
    symbolTable_ = &symbolTable;
    program_ = InstructionProgram{};
    currentLevel_ = 0;

    generateNode(root);
    return program_;
}

void CodeGenerator::generateNode(const semantic::AstNode* node) {
    // Dispatcher utama AST
    // Node tingkat atas diterjemahkan ke fungsi generator yang sesuai
    if (!node) return;

    switch (node->getKind()) {
    case semantic::AstKind::Program:
        generateProgram(static_cast<const semantic::ProgramNode&>(*node));
        break;
    case semantic::AstKind::Block:
        generateBlock(static_cast<const semantic::BlockNode&>(*node));
        break;
    case semantic::AstKind::Compound:
        generateCompound(static_cast<const semantic::CompoundNode&>(*node));
        break;
    default:
        generateStatement(node);
        break;
    }
}

void CodeGenerator::generateStatement(const semantic::AstNode* node) {
    // Mengubah statement AST menjadi rangkaian instruksi TAC
    if (!node) return;

    switch (node->getKind()) {
    case semantic::AstKind::Assign:
        generateAssign(static_cast<const semantic::AssignNode&>(*node));
        break;
    case semantic::AstKind::ProcCall:
        generateProcCall(static_cast<const semantic::ProcCallNode&>(*node));
        break;
    case semantic::AstKind::If:
        generateIf(static_cast<const semantic::IfNode&>(*node));
        break;
    case semantic::AstKind::While:
        generateWhile(static_cast<const semantic::WhileNode&>(*node));
        break;
    case semantic::AstKind::Compound:
        generateCompound(static_cast<const semantic::CompoundNode&>(*node));
        break;
    case semantic::AstKind::DeclPart:
        break;
    case semantic::AstKind::ConstDecl:
    case semantic::AstKind::TypeDecl:
    case semantic::AstKind::VarDecl:
    case semantic::AstKind::SimpleType:
    case semantic::AstKind::ArrayType:
    case semantic::AstKind::RecordType:
    case semantic::AstKind::Range:
    case semantic::AstKind::EnumeratedType:
        break;
    default:
        throw std::runtime_error(unsupported(*node));
    }
}

void CodeGenerator::generateExpression(const semantic::AstNode* node) {
    // Menghasilkan kode evaluasi ekspresi
    // Hasil evaluasi selalu ditempatkan pada evaluation stack
    if (!node) return;

    switch (node->getKind()) {
    case semantic::AstKind::Var:
        generateVar(static_cast<const semantic::VarNode&>(*node));
        break;
    case semantic::AstKind::IntLit:
        generateIntLit(static_cast<const semantic::IntLitNode&>(*node));
        break;
    case semantic::AstKind::BoolLit:
        generateBoolLit(static_cast<const semantic::BoolLitNode&>(*node));
        break;
    case semantic::AstKind::UnaryOp:
        generateUnaryOp(static_cast<const semantic::UnaryOpNode&>(*node));
        break;
    case semantic::AstKind::BinOp:
        generateBinOp(static_cast<const semantic::BinOpNode&>(*node));
        break;
    default:
        throw std::runtime_error(unsupported(*node));
    }
}

void CodeGenerator::generateStore(const semantic::AstNode* target) {
    // Menyimpan hasil ekspresi ke lokasi tujuan assignment
    // Pada Milestone 4 hanya assignment ke variabel biasa yang didukung
    if (!target) return;

    if (target->getKind() != semantic::AstKind::Var) {
        throw std::runtime_error("CodeGenerator fase 1 hanya mendukung assignment ke variabel biasa");
    }

    program_.emit(OpCode::STO, levelDifference(*target), variableAddress(*target));
}

void CodeGenerator::generateProgram(const semantic::ProgramNode& node) {
    // Menghasilkan TAC untuk program utama
    // Instruksi pertama selalu INT untuk mengalokasikan frame, sedangkan instruksi terakhir adalah RET
    currentLevel_ = node.level == semantic::NO_INDEX ? 0 : node.level;

    program_.emit(OpCode::INT, 0, initialFrameSize(node));

    if (node.declaration) {
        for (const auto& declaration : node.declaration->declarations) {
            if (declaration && isExecutableDeclaration(declaration->getKind())) {
                throw std::runtime_error(unsupported(*declaration));
            }
        }
    }

    if (node.statements) {
        generateCompound(*node.statements);
    }
    program_.emit(OpCode::RET, 0, 0);
}

void CodeGenerator::generateBlock(const semantic::BlockNode& node) {
    const int savedLevel = currentLevel_;
    currentLevel_ = node.level == semantic::NO_INDEX ? savedLevel : node.level;
    if (node.statements) generateCompound(*node.statements);
    currentLevel_ = savedLevel;
}

void CodeGenerator::generateCompound(const semantic::CompoundNode& node) {
    // Compound statement diterjemahkan dengan menghasilkan kode untuk setiap statement secara berurutan
    for (const auto& statement : node.statements) {
        generateStatement(statement.get());
    }
}

void CodeGenerator::generateAssign(const semantic::AssignNode& node) {
    // Assignment dievaluasi dalam dua tahap:
    // 1. Hitung nilai ekspresi
    // 2. Simpan hasil ke alamat target menggunakan STO
    generateExpression(node.value.get());
    generateStore(node.target.get());
}

void CodeGenerator::generateProcCall(const semantic::ProcCallNode& node) {
    // Milestone 4 hanya mendukung prosedur output bawaan (write/writeln) yang diterjemahkan menjadi operasi OPR
    const std::string name = lowerName(node.name);
    const bool isWrite = name == "write";
    const bool isWriteln = name == "writeln";

    if (!isWrite && !isWriteln) {
        throw std::runtime_error(unsupported(node));
    }

    if (node.args.empty()) {
        throw std::runtime_error("CodeGenerator membutuhkan argumen untuk '" + node.name + "'");
    }

    for (std::size_t i = 0; i < node.args.size(); ++i) {
        generateExpression(node.args[i].get());
        const bool isLast = i + 1 == node.args.size();
        const OprCode outputOp = (isWriteln && isLast) ? OprCode::WRTLN : OprCode::WRT;
        program_.emit(OpCode::OPR, 0, static_cast<int>(outputOp));
    }
}

void CodeGenerator::generateIf(const semantic::IfNode& node) {
    // IF diterjemahkan menggunakan teknik backpatching:
    //
    // condition
    // JPC <else>
    // then-part
    // JMP <end>
    // else-part
    //
    // Target lompatan diperbaiki setelah alamat akhir diketahui.
    generateExpression(node.condition.get());

    const int falseJump = program_.emit(OpCode::JPC, 0, 0);

    generateStatement(node.thenStmt.get());

    if (node.elseStmt) {
        const int endJump = program_.emit(OpCode::JMP, 0, 0);
        program_.patch(falseJump, program_.currentAddress());
        generateStatement(node.elseStmt.get());
        program_.patch(endJump, program_.currentAddress());
        return;
    }

    program_.patch(falseJump, program_.currentAddress());
}

void CodeGenerator::generateWhile(const semantic::WhileNode& node) {
    // WHILE diterjemahkan menjadi:
    //
    // loopStart:
    //   condition
    //   JPC exit
    //   body
    //   JMP loopStart
    // exit:
    const int loopStart = program_.currentAddress();

    generateExpression(node.condition.get());
    const int exitJump = program_.emit(OpCode::JPC, 0, 0);

    generateStatement(node.body.get());
    program_.emit(OpCode::JMP, 0, loopStart);
    program_.patch(exitJump, program_.currentAddress());
}

void CodeGenerator::generateVar(const semantic::VarNode& node) {
    // Variabel diterjemahkan menjadi instruksi LOD, sedangkan konstanta diterjemahkan menjadi LIT
    if (!symbolTable_ || node.tabIdx == semantic::NO_INDEX) {
        throw std::runtime_error("variabel '" + node.name + "' belum memiliki entri symbol table");
    }

    const auto& entry = symbolTable_->tabAt(node.tabIdx);
    if (entry.obj == semantic::ObjectKind::Constant) {
        program_.emit(OpCode::LIT, 0, entry.adr);
        return;
    }

    if (entry.obj != semantic::ObjectKind::Variable &&
        entry.obj != semantic::ObjectKind::Parameter) {
        throw std::runtime_error("identifier '" + node.name + "' bukan nilai yang dapat dimuat");
    }

    program_.emit(OpCode::LOD, levelDifference(node), variableAddress(node));
}

void CodeGenerator::generateIntLit(const semantic::IntLitNode& node) {
    program_.emit(OpCode::LIT, 0, static_cast<int>(node.value));
}

void CodeGenerator::generateBoolLit(const semantic::BoolLitNode& node) {
    program_.emit(OpCode::LIT, 0, node.value ? 1 : 0);
}

void CodeGenerator::generateUnaryOp(const semantic::UnaryOpNode& node) {
    // Operasi unary diterjemahkan ke opcode TAC yang ekuivalen
    // NOT direalisasikan sebagai perbandingan terhadap nol
    generateExpression(node.operand.get());

    switch (node.op) {
    case semantic::UnaryOpKind::Plus:
        return;
    case semantic::UnaryOpKind::Minus:
        program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::NEG));
        return;
    case semantic::UnaryOpKind::Not:
        program_.emit(OpCode::LIT, 0, 0);
        program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::EQL));
        return;
    }
}

void CodeGenerator::generateBinOp(const semantic::BinOpNode& node) {
    // Evaluasi operator biner menggunakan model stack machine dengan operand kiri dan kanan dievaluasi terlebih dahulu, kemudian instruksi OPR dijalankan terhadap keduanya
    generateExpression(node.lhs.get());
    generateExpression(node.rhs.get());

    if (node.op == semantic::BinOpKind::Or) {
        program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::ADD));
        program_.emit(OpCode::LIT, 0, 0);
        program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::GTR));
        return;
    }

    program_.emit(OpCode::OPR, 0, static_cast<int>(mapBinaryOp(node.op)));
}

int CodeGenerator::variableAddress(const semantic::AstNode& node) const {
    // Mengubah alamat simbol menjadi alamat runtime
    // FRAME_HEADER_SIZE ditambahkan untuk melewati:
    //
    // 0 = Static Link
    // 1 = Dynamic Link
    // 2 = Return Address
    if (!symbolTable_ || node.tabIdx == semantic::NO_INDEX) {
        throw std::runtime_error("node belum memiliki alamat symbol table");
    }
    return symbolTable_->tabAt(node.tabIdx).adr + FRAME_HEADER_SIZE;
}

int CodeGenerator::levelDifference(const semantic::AstNode& node) const {
    // Menghitung selisih level lexical antara lokasi penggunaan dan lokasi deklarasi identifier
    //Saat ini interpreter hanya menggunakan alamat absolut, tetapi informasi level tetap dihasilkan agar format TAC sesuai spesifikasi
    if (!symbolTable_ || node.tabIdx == semantic::NO_INDEX) {
        return 0;
    }
    const int declarationLevel = symbolTable_->tabAt(node.tabIdx).lev;
    return std::max(0, currentLevel_ - declarationLevel);
}

int CodeGenerator::initialFrameSize(const semantic::ProgramNode& node) const {
    // Menentukan ukuran frame awal program berdasarkan jumlah variabel yang dialokasikan oleh Scope Builder
    if (!symbolTable_) return FRAME_HEADER_SIZE;

    int blockIndex = 0;
    if (node.tabIdx != semantic::NO_INDEX) {
        const auto& programEntry = symbolTable_->tabAt(node.tabIdx);
        if (programEntry.ref != semantic::NO_INDEX) {
            blockIndex = programEntry.ref;
        }
    }

    return FRAME_HEADER_SIZE + symbolTable_->btabAt(blockIndex).vsze;
}

OprCode CodeGenerator::mapBinaryOp(semantic::BinOpKind op) {
    // Memetakan operator AST ke opcode OPR yang ekuivalen
    // Mapping ini mengikuti tabel operasi pada spesifikasi Milestone 4.
    switch (op) {
    case semantic::BinOpKind::Add: return OprCode::ADD;
    case semantic::BinOpKind::Sub: return OprCode::SUB;
    case semantic::BinOpKind::Mul: return OprCode::MUL;
    case semantic::BinOpKind::Div: return OprCode::DIV;
    case semantic::BinOpKind::IntDiv: return OprCode::DIV;
    case semantic::BinOpKind::Mod: return OprCode::MOD;
    case semantic::BinOpKind::Eq: return OprCode::EQL;
    case semantic::BinOpKind::Ne: return OprCode::NEQ;
    case semantic::BinOpKind::Lt: return OprCode::LSS;
    case semantic::BinOpKind::Le: return OprCode::LEQ;
    case semantic::BinOpKind::Gt: return OprCode::GTR;
    case semantic::BinOpKind::Ge: return OprCode::GEQ;
    case semantic::BinOpKind::And:
        return OprCode::MUL;
    case semantic::BinOpKind::Or:
        return OprCode::ADD;
    }
    throw std::runtime_error("operator biner tidak dikenal");
}

}  
