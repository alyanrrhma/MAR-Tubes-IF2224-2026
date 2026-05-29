#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "lexer/dfa.hpp"
#include "lexer/lexer.hpp"
#include "lexer/lexer_exception.hpp"
#include "parser/parser.hpp"
#include "semantic/ast_builder.hpp"
#include "semantic/ast_nodes.hpp"
#include "semantic/scope_builder.hpp"
#include "semantic/type_checker.hpp"

namespace fs = std::filesystem;

namespace {

void printUsage(const char* programName) { // EDIT MARK
    std::cerr
        << "Usage:\n"
        << "  " << programName << " <source.txt> [-o <ast_output.txt>]\n"
        << "  " << programName << " --parse-tree <parse_tree.txt> [-o <ast_output.txt>]\n\n"
        << "Options:\n"
        << "  --parse-tree <file>   Baca parse tree dari file (output milestone 2)\n"
        << "  -o <file>             Simpan output AST ke file (default: stdout)\n";
}

void ensureParentDirectoryExists(const std::string& filename) { // EDIT MARK
    fs::path path(filename);
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
}

bool isIgnoredByParser(const Token& token) { // EDIT MARK
    return token.get_type_name() == "COMMENT";
}

bool isLexicalErrorToken(const Token& token) { // EDIT MARK
    return token.get_type_name() == "UNKNOWN";
}

int indentLevel(const std::string& line) {
    int spaces = 0;
    for (char c : line) {
        if (c == ' ') {
            ++spaces;
        } else {
            break;
        }
    }
    return spaces / 2;
}

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool isNonTerminal(const std::string& t) {
    return t.size() >= 2 && t.front() == '<' && t.back() == '>';
}

std::string nonTerminalName(const std::string& t) {
    return t.substr(1, t.size() - 2);
}

struct TerminalInfo {
    std::string type;
    std::string lexeme;
    int line = -1;
    int col = -1;
};

TerminalInfo parseTerminal(const std::string& t) {
    TerminalInfo info;
    auto paren = t.find('(');
    if (paren == std::string::npos) {
        info.type = trim(t);
        return info;
    }

    info.type = trim(t.substr(0, paren));

    auto bracket = t.rfind('[');
    std::string rest;
    if (bracket != std::string::npos && bracket > paren) {
        rest = trim(t.substr(paren, bracket - paren));
        std::string locStr = t.substr(bracket + 1);
        auto colon = locStr.find(':');
        if (colon != std::string::npos) {
            try {
                info.line = std::stoi(locStr.substr(0, colon));
                info.col = std::stoi(locStr.substr(colon + 1));
            } catch (...) {
            }
        }
    } else {
        rest = trim(t.substr(paren));
    }

    if (rest.size() >= 2 && rest.front() == '(') {
        rest = rest.substr(1);
        if (!rest.empty() && rest.back() == ')') rest.pop_back();
        info.lexeme = rest;
    }

    return info;
}

bool isMetaLine(const std::string& t) {
    if (t.find("selesai") != std::string::npos) return true;
    if (t.find("disimpan di") != std::string::npos) return true;
    if (t.find("Output") == 0) return true;
    return false;
}

parse_tree::NodePtr readParseTree(std::istream& in) {
    std::vector<std::pair<int, std::string>> lines;
    std::string raw;

    while (std::getline(in, raw)) {
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();

        std::string t = trim(raw);
        if (t.empty() || isMetaLine(t)) continue;

        const bool looksLikeNT = isNonTerminal(t);
        const bool looksLikeT = (t.find('(') != std::string::npos);
        if (!looksLikeNT && !looksLikeT) continue;

        lines.push_back({indentLevel(raw), t});
    }

    if (lines.empty()) return nullptr;

    struct Frame {
        int level;
        parse_tree::Node* node;
    };
    std::stack<Frame> stk;

    parse_tree::NodePtr root = nullptr;

    for (auto& [lvl, content] : lines) {
        parse_tree::NodePtr newNode;

        if (isNonTerminal(content)) {
            newNode = parse_tree::makeNonTerminal(nonTerminalName(content));
        } else {
            TerminalInfo ti = parseTerminal(content);
            newNode = parse_tree::makeTerminal(ti.type, ti.lexeme, ti.line, ti.col);
        }

        parse_tree::Node* newRaw = newNode.get();

        if (stk.empty()) {
            root = std::move(newNode);
            stk.push({lvl, newRaw});
            continue;
        }

        while (!stk.empty() && stk.top().level >= lvl) {
            stk.pop();
        }

        if (stk.empty()) {
            break;
        }

        stk.top().node->addChild(std::move(newNode));
        stk.push({lvl, newRaw});
    }

    return root;
}

}

int main(int argc, char* argv[]) { // EDIT MARK
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFilename;
    std::string parseTreeFilename;  // --parse-tree mode
    std::string astOutputFilename;
    bool fromParseTree = false;

    // Mode: --parse-tree <file> atau source file biasa
    {
        int i = 1;
        std::string firstArg = argv[1];
        if (firstArg == "--parse-tree") {
            fromParseTree = true;
            if (argc < 3) {
                std::cerr << "ERROR: --parse-tree membutuhkan nama file.\n";
                printUsage(argv[0]);
                return 1;
            }
            parseTreeFilename = argv[2];
            i = 3;
        } else {
            inputFilename = firstArg;
            i = 2;
        }

        for (; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-o") {
                if (i + 1 >= argc) {
                    std::cerr << "ERROR: -o membutuhkan nama file output.\n";
                    printUsage(argv[0]);
                    return 1;
                }
                astOutputFilename = argv[++i];
            } else {
                std::cerr << "ERROR: Argumen tidak dikenal: " << arg << "\n";
                printUsage(argv[0]);
                return 1;
            }
        }
    }

    try {
        parse_tree::NodePtr parseRoot;

        if (fromParseTree) {
            // ---- Mode milestone 3: baca parse tree dari file teks ----
            std::ifstream ptFile(parseTreeFilename);
            if (!ptFile.is_open()) {
                std::cerr << "Gagal membuka file parse tree: " << parseTreeFilename << "\n";
                return 1;
            }
            parseRoot = readParseTree(ptFile);
            if (!parseRoot) {
                std::cerr << "ERROR: Gagal membaca parse tree dari "
                          << parseTreeFilename << "\n";
                return 1;
            }
            std::cerr << "Parse tree dibaca dari: " << parseTreeFilename << "\n";
        } else {
            // ---- Mode lama: baca source program, jalankan lexer + parser ----
            std::ifstream inputFile(inputFilename);
            if (!inputFile.is_open()) {
                std::cerr << "Gagal membuka file input: " << inputFilename << "\n";
                return 1;
            }

            std::ostringstream tokenSink;
            auto dfa = std::make_shared<DFA>();
            dfa->loadConfig("config/config_lexer.txt");

            Lexer lexer(inputFile, dfa, &tokenSink);
            while (!lexer.eof()) {
                lexer.process_next_token();
            }

            std::vector<Token> parserTokens;
            parserTokens.reserve(lexer.getResult().size());

            for (const Token& token : lexer.getResult()) {
                if (isIgnoredByParser(token)) continue;
                if (isLexicalErrorToken(token)) {
                    std::cerr << "Lexical error: token tidak dikenal "
                              << token.to_string() << "\n";
                    return 1;
                }
                parserTokens.push_back(token);
            }

            Parser parser(parserTokens);
            parseRoot = parser.parse();
        }

        AstBuilder builder;
        semantic::AstPtr astRoot = builder.build(parseRoot);

        ScopeBuilder scopeBuilder;
        scopeBuilder.build(astRoot);

        // Orang 4: jalankan type-checking & annotation pass
        TypeChecker typeChecker;
        typeChecker.check(astRoot.get(), scopeBuilder.symbolTable());

        const auto printSemanticResult = [&](std::ostream& out) {
            semantic::printAst(out, astRoot.get());
            out << "\n";
            scopeBuilder.printTables(out);
            // Cetak error dari ScopeBuilder (undeclared, redeclaration, dll.)
            if (scopeBuilder.hasErrors()) {
                out << "\nSemantic errors\n";
                scopeBuilder.printErrors(out);
            }
            // Cetak error dari TypeChecker (type mismatch, incompatible, dll.)
            if (typeChecker.hasErrors()) {
                out << "\nType errors\n";
                typeChecker.printErrors(out);
            }
        };

        if (astOutputFilename.empty()) {
            printSemanticResult(std::cout);
        } else {
            ensureParentDirectoryExists(astOutputFilename);
            std::ofstream astOutputFile(astOutputFilename);
            if (!astOutputFile.is_open()) {
                std::cerr << "Gagal membuka file output AST: "
                          << astOutputFilename << "\n";
                return 1;
            }
            printSemanticResult(astOutputFile);
            std::cout << "AST dan symbol table output disimpan di: " << astOutputFilename << "\n";
        }
    } catch (const LexerException& e) {
        std::cerr << e.full_message() << "\n";
        return 1;
    } catch (const ParseException& e) {
        std::cerr << e.full_message() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
