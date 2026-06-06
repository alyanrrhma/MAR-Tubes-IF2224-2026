#include "code_generator.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace backend {

namespace {

std::string unsupported(const semantic::AstNode& node) {
    return std::string("CodeGenerator belum mendukung node ") + node.kindName();
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
    symbolTable_ = &symbolTable;
    program_ = InstructionProgram{};
    currentLevel_ = 0;
    procAddresses_.clear();
    procPsizeByLevel_ = {0}; // level 0 = program utama, psze = 0

    generateNode(root);
    return program_;
}

void CodeGenerator::generateNode(const semantic::AstNode* node) {
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
    if (!node) return;

    switch (node->getKind()) {
    case semantic::AstKind::Assign:
        generateAssign(static_cast<const semantic::AssignNode&>(*node));
        break;
    case semantic::AstKind::ProcCall: {
        const auto& callNode = static_cast<const semantic::ProcCallNode&>(*node);
        generateProcCall(callNode);
        // Jika ini pemanggilan fungsi (bukan prosedur), nilai kembalian ada di stack — buang
        if (callNode.tabIdx != semantic::NO_INDEX && symbolTable_) {
            const auto& entry = symbolTable_->tabAt(callNode.tabIdx);
            if (entry.obj == semantic::ObjectKind::Function) {
                program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::POP));
            }
        }
        break;
    }
    case semantic::AstKind::Return:
        generateReturn(static_cast<const semantic::ReturnNode&>(*node));
        break;
    case semantic::AstKind::If:
        generateIf(static_cast<const semantic::IfNode&>(*node));
        break;
    case semantic::AstKind::While:
        generateWhile(static_cast<const semantic::WhileNode&>(*node));
        break;
    case semantic::AstKind::Repeat:
        generateRepeat(static_cast<const semantic::RepeatNode&>(*node));
        break;
    case semantic::AstKind::For:
        generateFor(static_cast<const semantic::ForNode&>(*node));
        break;
    case semantic::AstKind::Case:
        generateCase(static_cast<const semantic::CaseNode&>(*node));
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
    if (!node) return;

    switch (node->getKind()) {
    case semantic::AstKind::ProcCall:
        // Pemanggilan fungsi sebagai ekspresi — nilai kembalian tetap di stack
        generateProcCall(static_cast<const semantic::ProcCallNode&>(*node));
        break;
    case semantic::AstKind::Var:
        generateVar(static_cast<const semantic::VarNode&>(*node));
        break;
    case semantic::AstKind::IntLit:
        generateIntLit(static_cast<const semantic::IntLitNode&>(*node));
        break;
    case semantic::AstKind::BoolLit:
        generateBoolLit(static_cast<const semantic::BoolLitNode&>(*node));
        break;
    case semantic::AstKind::StringLit:
        generateStringLit(static_cast<const semantic::StringLitNode&>(*node));
        break;
    case semantic::AstKind::CharLit:
        generateCharLit(static_cast<const semantic::CharLitNode&>(*node));
        break;
    case semantic::AstKind::ArrayAccess:
        generateArrayAccess(static_cast<const semantic::ArrayAccessNode&>(*node));
        break;
    case semantic::AstKind::FieldAccess:
        generateFieldAccess(static_cast<const semantic::FieldAccessNode&>(*node));
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
    if (!target) return;

    switch (target->getKind()) {
    case semantic::AstKind::Var:
        program_.emit(OpCode::STO, levelDifference(*target), variableAddress(*target));
        break;
    case semantic::AstKind::ArrayAccess:
    case semantic::AstKind::FieldAccess:
        // Nilai sudah di stack (dari generateAssign); dorong alamat lalu STOI
        generateAddress(target);
        program_.emit(OpCode::STOI, 0, 0);
        break;
    default:
        throw std::runtime_error(std::string("generateStore: target assignment tidak didukung: ") + target->kindName());
    }
}

void CodeGenerator::generateProgram(const semantic::ProgramNode& node) {
    currentLevel_ = node.level == semantic::NO_INDEX ? 0 : node.level;
    while (static_cast<int>(procPsizeByLevel_.size()) <= currentLevel_)
        procPsizeByLevel_.push_back(0);
    procPsizeByLevel_[currentLevel_] = 0;

    program_.emit(OpCode::INT, 0, initialFrameSize(node));

    if (node.declaration) generateNestedDeclarations(*node.declaration);

    if (node.statements) generateCompound(*node.statements);
    program_.emit(OpCode::RET, 0, 0);
}

void CodeGenerator::generateBlock(const semantic::BlockNode& node) {
    const int savedLevel = currentLevel_;
    currentLevel_ = node.level == semantic::NO_INDEX ? savedLevel : node.level;
    if (node.statements) generateCompound(*node.statements);
    currentLevel_ = savedLevel;
}

void CodeGenerator::generateNestedDeclarations(const semantic::DeclarationNode& decls) {
    for (const auto& decl : decls.declarations) {
        if (!decl) continue;
        if (decl->getKind() == semantic::AstKind::ProcDecl)
            generateProcDecl(static_cast<const semantic::ProcDeclNode&>(*decl));
        else if (decl->getKind() == semantic::AstKind::FuncDecl)
            generateFuncDecl(static_cast<const semantic::FuncDeclNode&>(*decl));
        // VarDecl, ConstDecl, TypeDecl dll. sudah ditangani symbol table — lewati
    }
}

void CodeGenerator::generateProcDecl(const semantic::ProcDeclNode& node) {
    if (node.tabIdx == semantic::NO_INDEX || !symbolTable_) {
        throw std::runtime_error(unsupported(node));
    }
    const int btabIdx = node.typeRef;
    const int psze = symbolTable_->btabAt(btabIdx).psze;

    // Lompati tubuh prosedur saat eksekusi linear program utama
    const int skipJump = program_.emit(OpCode::JMP, 0, 0);
    procAddresses_[node.tabIdx] = program_.currentAddress();

    const int savedLevel = currentLevel_;
    currentLevel_ = node.level;
    while (static_cast<int>(procPsizeByLevel_.size()) <= currentLevel_)
        procPsizeByLevel_.push_back(0);
    procPsizeByLevel_[currentLevel_] = psze;

    // Alokasikan slot variabel lokal (parameter sudah di stack dari caller)
    program_.emit(OpCode::INT, 0, procFrameSize(btabIdx));

    if (node.block) {
        if (node.block->declaration) generateNestedDeclarations(*node.block->declaration);
        if (node.block->statements) generateCompound(*node.block->statements);
    }

    // RET: bersihkan psze slot parameter dan kembalikan tanpa nilai
    program_.emit(OpCode::RET, psze, 0);

    currentLevel_ = savedLevel;
    program_.patch(skipJump, program_.currentAddress());
}

void CodeGenerator::generateFuncDecl(const semantic::FuncDeclNode& node) {
    if (node.tabIdx == semantic::NO_INDEX || !symbolTable_) {
        throw std::runtime_error(unsupported(node));
    }
    const int btabIdx = node.typeRef;
    const int psze = symbolTable_->btabAt(btabIdx).psze;

    const int skipJump = program_.emit(OpCode::JMP, 0, 0);
    procAddresses_[node.tabIdx] = program_.currentAddress();

    const int savedLevel = currentLevel_;
    currentLevel_ = node.level;
    while (static_cast<int>(procPsizeByLevel_.size()) <= currentLevel_)
        procPsizeByLevel_.push_back(0);
    procPsizeByLevel_[currentLevel_] = psze;

    program_.emit(OpCode::INT, 0, procFrameSize(btabIdx));

    if (node.block) {
        if (node.block->declaration) generateNestedDeclarations(*node.block->declaration);
        if (node.block->statements) generateCompound(*node.block->statements);
    }

    // Fallback jika fungsi jatuh tanpa return eksplisit: kembalikan 0
    program_.emit(OpCode::LIT, 0, 0);
    program_.emit(OpCode::RET, psze, 1);

    currentLevel_ = savedLevel;
    program_.patch(skipJump, program_.currentAddress());
}

void CodeGenerator::generateReturn(const semantic::ReturnNode& node) {
    const int psze = (currentLevel_ >= 0 && currentLevel_ < static_cast<int>(procPsizeByLevel_.size()))
                     ? procPsizeByLevel_[currentLevel_] : 0;
    if (node.value) {
        generateExpression(node.value.get());
        program_.emit(OpCode::RET, psze, 1);
    } else {
        program_.emit(OpCode::RET, psze, 0);
    }
}

void CodeGenerator::generateCompound(const semantic::CompoundNode& node) {
    for (const auto& statement : node.statements) {
        generateStatement(statement.get());
    }
}

void CodeGenerator::generateAssign(const semantic::AssignNode& node) {
    generateExpression(node.value.get());
    generateStore(node.target.get());
}

void CodeGenerator::generateProcCall(const semantic::ProcCallNode& node) {
    const std::string name = lowerName(node.name);

    // Prosedur I/O bawaan
    if (name == "write" || name == "writeln") {
        if (node.args.empty()) {
            throw std::runtime_error("CodeGenerator membutuhkan argumen untuk '" + node.name + "'");
        }
        for (std::size_t i = 0; i < node.args.size(); ++i) {
            generateExpression(node.args[i].get());
            const bool isLast = i + 1 == node.args.size();
            const OprCode outputOp = (name == "writeln" && isLast) ? OprCode::WRTLN : OprCode::WRT;
            program_.emit(OpCode::OPR, 0, static_cast<int>(outputOp));
        }
        return;
    }

    if (node.tabIdx == semantic::NO_INDEX || !symbolTable_) {
        throw std::runtime_error("unresolved call: '" + node.name + "'");
    }

    const auto& entry = symbolTable_->tabAt(node.tabIdx);
    if (entry.obj != semantic::ObjectKind::Procedure &&
        entry.obj != semantic::ObjectKind::Function) {
        throw std::runtime_error(unsupported(node));
    }

    const auto addrIt = procAddresses_.find(node.tabIdx);
    if (addrIt == procAddresses_.end()) {
        throw std::runtime_error("prosedur '" + node.name + "' belum dideklarasikan sebelum penggunaan");
    }

    // Dorong semua argumen ke stack (by value)
    for (const auto& arg : node.args) {
        generateExpression(arg.get());
    }

    program_.emit(OpCode::CAL, levelDifference(node), addrIt->second);
    // Nilai kembalian (jika fungsi) kini ada di atas stack; caller menentukan nasibnya
}

void CodeGenerator::generateIf(const semantic::IfNode& node) {
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
    const int loopStart = program_.currentAddress();

    generateExpression(node.condition.get());
    const int exitJump = program_.emit(OpCode::JPC, 0, 0);

    generateStatement(node.body.get());
    program_.emit(OpCode::JMP, 0, loopStart);
    program_.patch(exitJump, program_.currentAddress());
}

void CodeGenerator::generateRepeat(const semantic::RepeatNode& node) {
    const int loopStart = program_.currentAddress();
    generateCompound(*node.body);
    generateExpression(node.condition.get());
    program_.emit(OpCode::JPC, 0, loopStart);
}

void CodeGenerator::generateFor(const semantic::ForNode& node) {
    if (node.tabIdx == semantic::NO_INDEX || !symbolTable_) {
        throw std::runtime_error("for control variable belum memiliki entri symbol table");
    }

    generateExpression(node.startExpr.get());
    program_.emit(OpCode::STO, levelDifference(node), variableAddress(node));

    const int loopStart = program_.currentAddress();
    program_.emit(OpCode::LOD, levelDifference(node), variableAddress(node));
    generateExpression(node.endExpr.get());
    program_.emit(OpCode::OPR, 0, static_cast<int>(node.downto ? OprCode::GEQ : OprCode::LEQ));
    const int exitJump = program_.emit(OpCode::JPC, 0, 0);

    generateStatement(node.body.get());

    program_.emit(OpCode::LOD, levelDifference(node), variableAddress(node));
    program_.emit(OpCode::LIT, 0, 1);
    program_.emit(OpCode::OPR, 0, static_cast<int>(node.downto ? OprCode::SUB : OprCode::ADD));
    program_.emit(OpCode::STO, levelDifference(node), variableAddress(node));
    program_.emit(OpCode::JMP, 0, loopStart);

    program_.patch(exitJump, program_.currentAddress());
}

void CodeGenerator::generateCase(const semantic::CaseNode& node) {
    std::vector<int> endJumps;

    for (const auto& branch : node.branches) {
        if (!branch || branch->labels.empty()) continue;

        std::vector<int> jumpToBody;
        for (const auto& label : branch->labels) {
            generateExpression(node.selector.get());
            generateExpression(label.get());
            program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::EQL));
            const int nextLabelJump = program_.emit(OpCode::JPC, 0, 0);
            jumpToBody.push_back(program_.emit(OpCode::JMP, 0, 0));
            program_.patch(nextLabelJump, program_.currentAddress());
        }

        const int noMatchJump = program_.emit(OpCode::JMP, 0, 0);
        const int bodyAddress = program_.currentAddress();
        for (int jump : jumpToBody) {
            program_.patch(jump, bodyAddress);
        }

        generateStatement(branch->statement.get());
        endJumps.push_back(program_.emit(OpCode::JMP, 0, 0));
        program_.patch(noMatchJump, program_.currentAddress());
    }

    const int endAddress = program_.currentAddress();
    for (int jump : endJumps) {
        program_.patch(jump, endAddress);
    }
}

void CodeGenerator::generateVar(const semantic::VarNode& node) {
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
    program_.emit(OpCode::LITB, 0, node.value ? 1 : 0);
}

void CodeGenerator::generateStringLit(const semantic::StringLitNode& node) {
    const int idx = program_.addString(node.value);
    program_.emit(OpCode::LITS, 0, idx);
}

void CodeGenerator::generateCharLit(const semantic::CharLitNode& node) {
    program_.emit(OpCode::LIT, 0, static_cast<unsigned char>(node.value));
}

void CodeGenerator::generateArrayAccess(const semantic::ArrayAccessNode& node) {
    generateAddress(&node);
    program_.emit(OpCode::LODI, 0, 0);
}

void CodeGenerator::generateFieldAccess(const semantic::FieldAccessNode& node) {
    generateAddress(&node);
    program_.emit(OpCode::LODI, 0, 0);
}

void CodeGenerator::generateAddress(const semantic::AstNode* node) {
    if (!node) throw std::runtime_error("generateAddress: node null");

    switch (node->getKind()) {
    case semantic::AstKind::Var:
        program_.emit(OpCode::ADDR, levelDifference(*node), variableAddress(*node));
        break;

    case semantic::AstKind::ArrayAccess: {
        const auto& arr = static_cast<const semantic::ArrayAccessNode&>(*node);
        generateAddress(arr.base.get()); // dorong alamat dasar array

        int atabIdx = arr.base->typeRef;
        for (const auto& idxExpr : arr.indices) {
            if (atabIdx == semantic::NO_INDEX) {
                throw std::runtime_error("generateAddress: tidak ada info atab untuk dimensi array");
            }
            const auto& atab = symbolTable_->atabAt(atabIdx);
            generateExpression(idxExpr.get());         // dorong indeks
            program_.emit(OpCode::CHK, atab.low, atab.high); // validasi batas runtime
            program_.emit(OpCode::LIT, 0, atab.low);  // dorong batas bawah
            program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::SUB)); // i - low
            program_.emit(OpCode::LIT, 0, atab.elsz); // dorong ukuran elemen
            program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::MUL)); // (i-low)*elsz
            program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::ADD)); // base + offset
            atabIdx = atab.eref; // masuk ke dimensi berikutnya
        }
        break;
    }

    case semantic::AstKind::FieldAccess: {
        const auto& fld = static_cast<const semantic::FieldAccessNode&>(*node);
        generateAddress(fld.base.get()); // dorong alamat dasar record
        if (fld.tabIdx != semantic::NO_INDEX) {
            const int fieldOffset = symbolTable_->tabAt(fld.tabIdx).adr;
            if (fieldOffset != 0) {
                program_.emit(OpCode::LIT, 0, fieldOffset);
                program_.emit(OpCode::OPR, 0, static_cast<int>(OprCode::ADD));
            }
        }
        break;
    }

    default:
        throw std::runtime_error(std::string("generateAddress: tidak bisa mengambil alamat dari ") + node->kindName());
    }
}

void CodeGenerator::generateUnaryOp(const semantic::UnaryOpNode& node) {
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
    if (!symbolTable_ || node.tabIdx == semantic::NO_INDEX) {
        throw std::runtime_error("node belum memiliki alamat symbol table");
    }
    const auto& entry = symbolTable_->tabAt(node.tabIdx);
    // Cari psze milik scope tempat variabel/parameter dideklarasikan
    const int declLevel = entry.lev;
    const int psze = (declLevel >= 0 && declLevel < static_cast<int>(procPsizeByLevel_.size()))
                     ? procPsizeByLevel_[declLevel] : 0;
    if (entry.obj == semantic::ObjectKind::Parameter) {
        // Parameter didorong caller SEBELUM header; offsetnya negatif dari frameBase
        // adr=0 → operand = -psze, adr=1 → operand = -psze+1, dst.
        return entry.adr - psze;
    }
    // Variabel lokal: lewati header (3 slot) lalu offset dari awal area variabel
    return entry.adr - psze + FRAME_HEADER_SIZE;
}

int CodeGenerator::levelDifference(const semantic::AstNode& node) const {
    if (!symbolTable_ || node.tabIdx == semantic::NO_INDEX) {
        return 0;
    }
    const int declarationLevel = symbolTable_->tabAt(node.tabIdx).lev;
    return std::max(0, currentLevel_ - declarationLevel);
}

int CodeGenerator::initialFrameSize(const semantic::ProgramNode& node) const {
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

int CodeGenerator::procFrameSize(int btabIdx) const {
    if (!symbolTable_) return FRAME_HEADER_SIZE;
    const auto& btab = symbolTable_->btabAt(btabIdx);
    // Header + slot variabel lokal saja; slot parameter sudah didorong caller
    return FRAME_HEADER_SIZE + (btab.vsze - btab.psze);
}

OprCode CodeGenerator::mapBinaryOp(semantic::BinOpKind op) {
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
