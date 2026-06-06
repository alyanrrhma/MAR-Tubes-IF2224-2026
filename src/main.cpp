#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "backend/code_generator.hpp"
#include "backend/interpreter.hpp"
#include "lexer/dfa.hpp"
#include "lexer/lexer.hpp"
#include "lexer/lexer_exception.hpp"
#include "parser/parser.hpp"
#include "parser/parse_tree.hpp"
#include "semantic/ast_builder.hpp"
#include "semantic/ast_nodes.hpp"
#include "semantic/scope_builder.hpp"
#include "semantic/type_checker.hpp"

namespace fs = std::filesystem;

namespace {

struct Options {
    std::string inputFile;
    std::string saveTokens;
    std::string saveParseTree;
    std::string saveAst;
    std::string saveDfaTrace;
    std::string outputFile;
    bool lexOnly = false;
    bool parseOnly = false;
    bool tokenInput = false;
    bool parseTreeInput = false;
    bool verbose = false;
    bool printTac = false;
    bool run = false;
};

void printUsage(const char* programName) {
    std::cout
        << "Usage:\n"
        << "  " << programName << " <program.txt> --lex-only -o <tokens.txt> [--save-dfa-trace <trace.txt>]\n"
        << "  " << programName << " <program.txt> --parse-only --save-parse-tree <parse-tree.txt>\n"
        << "  " << programName << " --from-tokens <tokens.txt> --parse-only --save-parse-tree <parse-tree.txt>\n"
        << "  " << programName << " --from-parse-tree <parse-tree.txt> --save-ast <ast-output.txt>\n"
        << "  " << programName << " <program.txt> [--verbose]\n"
        << "  " << programName << " <program.txt> --print-tac\n"
        << "  " << programName << " <program.txt> --run\n"
        << "  " << programName << " <program.txt> --save-tokens <file> [--save-parse-tree <file>] [--save-ast <file>] [--verbose]\n\n"
        << "Options:\n"
        << "  --lex-only              Run lexical analysis only, print tokens, then stop\n"
        << "  --parse-only            Stop after syntax analysis and print/save parse tree\n"
        << "  --from-tokens <file>    Read Milestone 1 token output instead of source code\n"
        << "  --from-parse-tree <file> Read Milestone 2 parse tree output for semantic analysis\n"
        << "  --verbose               Print tokens, parse tree, and decorated AST to stdout\n"
        << "  --print-tac             Generate TAC, print it, then stop\n"
        << "  --run                   Generate TAC and run it with the interpreter\n"
        << "  --save-tokens <file>    Save tokens to <file>\n"
        << "  --save-dfa-trace <file> Save DFA transition trace during lexical analysis\n"
        << "  --save-parse-tree <file> Save parse tree to <file>\n"
        << "  --save-ast <file>       Save decorated AST, tab, btab, and atab to <file>\n"
        << "  -o <file>               Output file. With --lex-only this saves tokens; otherwise it saves AST\n";
}

void ensureParentDirectoryExists(const std::string& filename) {
    fs::path path(filename);
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
}

bool isIgnoredByParser(const Token& token) {
    return token.get_type_name() == "COMMENT";
}

bool isLexicalErrorToken(const Token& token) {
    return token.get_type_name() == "UNKNOWN";
}

void writeOutputFile(const std::string& filename, const std::string& content) {
    ensureParentDirectoryExists(filename);
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Gagal membuka file output: " + filename);
    }
    out << content;
}

Options parseOptions(int argc, char* argv[]) {
    Options options;

    if (argc < 2) {
        throw std::runtime_error("Argumen input tidak diberikan");
    }

    int i = 1;
    bool inputSet = false;

    while (i < argc) {
        std::string arg = argv[i];

        if (arg == "--verbose") {
            options.verbose = true;
            ++i;
            continue;
        }

        if (arg == "--print-tac") {
            options.printTac = true;
            ++i;
            continue;
        }

        if (arg == "--run") {
            options.run = true;
            ++i;
            continue;
        }

        if (arg == "--lex-only") {
            options.lexOnly = true;
            ++i;
            continue;
        }

        if (arg == "--parse-only") {
            options.parseOnly = true;
            ++i;
            continue;
        }

        if (arg == "--from-tokens") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--from-tokens membutuhkan nama file token");
            }
            options.inputFile = argv[++i];
            options.tokenInput = true;
            inputSet = true;
            ++i;
            continue;
        }

        if (arg == "--from-parse-tree") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--from-parse-tree membutuhkan nama file parse tree");
            }
            options.inputFile = argv[++i];
            options.parseTreeInput = true;
            inputSet = true;
            ++i;
            continue;
        }

        if (arg == "--save-tokens") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--save-tokens membutuhkan nama file");
            }
            options.saveTokens = argv[++i];
            ++i;
            continue;
        }

        if (arg == "--save-dfa-trace") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--save-dfa-trace membutuhkan nama file");
            }
            options.saveDfaTrace = argv[++i];
            ++i;
            continue;
        }

        if (arg == "--save-parse-tree") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--save-parse-tree membutuhkan nama file");
            }
            options.saveParseTree = argv[++i];
            ++i;
            continue;
        }

        if (arg == "--save-ast") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--save-ast membutuhkan nama file");
            }
            options.saveAst = argv[++i];
            ++i;
            continue;
        }

        if (arg == "-o") {
            if (i + 1 >= argc) {
                throw std::runtime_error("-o membutuhkan nama file");
            }
            options.outputFile = argv[++i];
            ++i;
            continue;
        }

        if (!inputSet && arg[0] != '-') {
            options.inputFile = arg;
            inputSet = true;
            ++i;
            continue;
        }

        throw std::runtime_error("Argumen tidak dikenal: " + arg);
    }

    if (!inputSet) {
        throw std::runtime_error("Argumen input tidak diberikan");
    }

    if (options.lexOnly && (options.tokenInput || options.parseTreeInput)) {
        throw std::runtime_error("--lex-only hanya dapat digunakan dengan input source code");
    }

    if (options.parseOnly && options.parseTreeInput) {
        throw std::runtime_error("--parse-only tidak dapat digabungkan dengan --from-parse-tree");
    }

    if (options.tokenInput && options.parseTreeInput) {
        throw std::runtime_error("--from-tokens dan --from-parse-tree tidak dapat digunakan bersamaan");
    }

    if (!options.outputFile.empty()) {
        if (options.lexOnly) {
            options.saveTokens = options.outputFile;
        } else if (options.parseOnly) {
            options.saveParseTree = options.outputFile;
        } else {
            options.saveAst = options.outputFile;
        }
    }

    return options;
}

std::string trim(const std::string& text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    if (first == text.end()) {
        return "";
    }
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    return std::string(first, last);
}

std::string toUpperTokenName(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return text;
}

std::vector<Token> readTokensFromMilestone1Output(std::istream& in, std::ostream& normalizedOut) {
    std::vector<Token> tokens;
    std::string line;
    int lineNo = 0;

    while (std::getline(in, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        std::string tokenName;
        std::string value;
        const std::size_t open = line.find('(');
        const std::size_t close = line.rfind(')');

        if (open != std::string::npos) {
            if (close == std::string::npos || close < open) {
                throw std::runtime_error("Format token tidak valid pada baris " + std::to_string(lineNo) + ": " + line);
            }
            tokenName = trim(line.substr(0, open));
            value = line.substr(open + 1, close - open - 1);
        } else {
            tokenName = line;
        }

        if (tokenName.empty()) {
            throw std::runtime_error("Nama token kosong pada baris " + std::to_string(lineNo));
        }

        std::string normalizedName = toUpperTokenName(tokenName);
        Token token(TokenType(normalizedName), value);
        normalizedOut << token.to_string() << "\n";
        tokens.push_back(token);
    }

    return tokens;
}


int parseTreeIndentLevel(const std::string& line) {
    int spaces = 0;
    for (char c : line) {
        if (c == ' ') ++spaces;
        else break;
    }
    return spaces / 2;
}

bool isParseTreeNonTerminal(const std::string& text) {
    return text.size() >= 2 && text.front() == '<' && text.back() == '>';
}

std::string parseTreeNonTerminalName(const std::string& text) {
    return text.substr(1, text.size() - 2);
}

struct ParseTreeTerminalInfo {
    std::string type;
    std::string lexeme;
    int line = -1;
    int column = -1;
};

ParseTreeTerminalInfo parseTreeTerminal(const std::string& text) {
    ParseTreeTerminalInfo info;
    const std::size_t open = text.find('(');
    if (open == std::string::npos) {
        info.type = trim(text);
        return info;
    }

    info.type = trim(text.substr(0, open));

    const std::size_t bracket = text.rfind('[');
    std::string payload;
    if (bracket != std::string::npos && bracket > open) {
        payload = trim(text.substr(open, bracket - open));
        const std::string loc = text.substr(bracket + 1);
        const std::size_t colon = loc.find(':');
        if (colon != std::string::npos) {
            try {
                info.line = std::stoi(loc.substr(0, colon));
                info.column = std::stoi(loc.substr(colon + 1));
            } catch (...) {
                info.line = -1;
                info.column = -1;
            }
        }
    } else {
        payload = trim(text.substr(open));
    }

    if (payload.size() >= 2 && payload.front() == '(') {
        payload = payload.substr(1);
        if (!payload.empty() && payload.back() == ')') payload.pop_back();
        info.lexeme = payload;
    }

    return info;
}

bool isParseTreeMetaLine(const std::string& text) {
    return text.find("selesai") != std::string::npos ||
           text.find("disimpan di") != std::string::npos ||
           text.rfind("Output", 0) == 0;
}

parse_tree::NodePtr readParseTreeFromMilestone2Output(std::istream& in) {
    std::vector<std::pair<int, std::string>> lines;
    std::string raw;

    while (std::getline(in, raw)) {
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();
        const std::string text = trim(raw);
        if (text.empty() || isParseTreeMetaLine(text)) continue;

        const bool looksLikeNonTerminal = isParseTreeNonTerminal(text);
        const bool looksLikeTerminal = text.find('(') != std::string::npos;
        if (!looksLikeNonTerminal && !looksLikeTerminal) continue;

        lines.push_back({parseTreeIndentLevel(raw), text});
    }

    if (lines.empty()) return nullptr;

    struct Frame {
        int level;
        parse_tree::Node* node;
    };
    std::stack<Frame> stack;
    parse_tree::NodePtr root = nullptr;

    for (const auto& item : lines) {
        const int level = item.first;
        const std::string& content = item.second;
        parse_tree::NodePtr node;

        if (isParseTreeNonTerminal(content)) {
            node = parse_tree::makeNonTerminal(parseTreeNonTerminalName(content));
        } else {
            ParseTreeTerminalInfo terminal = parseTreeTerminal(content);
            node = parse_tree::makeTerminal(terminal.type, terminal.lexeme,
                                            terminal.line, terminal.column);
        }

        parse_tree::Node* rawNode = node.get();
        if (stack.empty()) {
            root = std::move(node);
            stack.push({level, rawNode});
            continue;
        }

        while (!stack.empty() && stack.top().level >= level) {
            stack.pop();
        }

        if (stack.empty()) {
            throw std::runtime_error("Format parse tree tidak valid: ditemukan lebih dari satu root");
        }

        stack.top().node->addChild(std::move(node));
        stack.push({level, rawNode});
    }

    return root;
}

void printSemanticResult(std::ostream& out,
                         semantic::AstPtr& astRoot,
                         ScopeBuilder& scopeBuilder,
                         TypeChecker& typeChecker) {
    semantic::printAst(out, astRoot.get());
    out << "\n";
    scopeBuilder.printTables(out);

    if (scopeBuilder.hasErrors()) {
        out << "\nSemantic errors\n";
        scopeBuilder.printErrors(out);
    }

    if (typeChecker.hasErrors()) {
        out << "\nType errors\n";
        typeChecker.printErrors(out);
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    const char* programName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "arion";

    try {
        Options options = parseOptions(argc, argv);

        std::ifstream inputFile(options.inputFile);
        if (!inputFile.is_open()) {
            std::cout << "Gagal membuka file input: " << options.inputFile << "\n";
            return 1;
        }

        std::ostringstream tokenBuffer;
        std::ostringstream dfaTraceBuffer;
        std::ostringstream parseTreeBuffer;
        std::ostringstream decoratedAstBuffer;

        std::vector<Token> rawTokens;
        parse_tree::NodePtr parseRoot;

        if (options.parseTreeInput) {
            parseRoot = readParseTreeFromMilestone2Output(inputFile);
            if (!parseRoot) {
                throw std::runtime_error("Gagal membaca parse tree dari " + options.inputFile);
            }
            parse_tree::printTree(parseRoot.get(), parseTreeBuffer);
        } else if (options.tokenInput) {
            if (!options.saveDfaTrace.empty()) {
                throw std::runtime_error("--save-dfa-trace hanya berlaku untuk input source code, bukan --from-tokens");
            }
            rawTokens = readTokensFromMilestone1Output(inputFile, tokenBuffer);
        } else {
            auto dfa = std::make_shared<DFA>();
            dfa->loadConfig("config/config_lexer.txt");

            Lexer lexer(inputFile, dfa, &tokenBuffer);
            if (!options.saveDfaTrace.empty()) {
                lexer.enableTrace(&dfaTraceBuffer);
            }

            while (!lexer.eof()) {
                lexer.process_next_token();
            }

            if (!options.saveDfaTrace.empty()) {
                writeOutputFile(options.saveDfaTrace, dfaTraceBuffer.str());
            }

            if (lexer.hasErrors()) {
                for (const std::string& error : lexer.getErrors()) {
                    std::cout << error << "\n";
                }
                if (options.lexOnly) {
                    std::cout << tokenBuffer.str();
                }
                if (!options.saveTokens.empty()) {
                    writeOutputFile(options.saveTokens, tokenBuffer.str());
                }
                return 1;
            }

            rawTokens = lexer.getResult();
        }

        if (!options.parseTreeInput) {
            if (!options.saveTokens.empty()) {
                writeOutputFile(options.saveTokens, tokenBuffer.str());
            }

            if (options.lexOnly) {
                std::cout << tokenBuffer.str();
                return 0;
            }

            std::vector<Token> parserTokens;
        parserTokens.reserve(rawTokens.size());
        for (const Token& token : rawTokens) {
            if (isIgnoredByParser(token)) {
                continue;
            }

            if (isLexicalErrorToken(token)) {
                std::cout << "Lexical error: token tidak dikenal "
                          << token.to_string() << "\n";
                return 1;
            }

            parserTokens.push_back(token);
        }

            Parser parser(parserTokens);
            parseRoot = parser.parse();
            parse_tree::printTree(parseRoot.get(), parseTreeBuffer);
        }

        if (!options.saveParseTree.empty()) {
            writeOutputFile(options.saveParseTree, parseTreeBuffer.str());
        }

        if (options.parseOnly) {
            if (options.saveParseTree.empty()) {
                std::cout << parseTreeBuffer.str();
            }
            return 0;
        }

        AstBuilder builder;
        semantic::AstPtr astRoot = builder.build(parseRoot);

        ScopeBuilder scopeBuilder;
        scopeBuilder.build(astRoot);

        TypeChecker typeChecker;
        typeChecker.check(astRoot.get(), scopeBuilder.symbolTable());

        printSemanticResult(decoratedAstBuffer, astRoot, scopeBuilder, typeChecker);

        if (options.verbose) {
            std::cout << "=== Lexical analysis ===\n\n";
            std::cout << tokenBuffer.str();
            std::cout << "\n=== Syntax analysis ===\n\n";
            std::cout << parseTreeBuffer.str();
            std::cout << "\n=== Semantic analysis ===\n\n";
            std::cout << decoratedAstBuffer.str();
        }

        if (!options.saveAst.empty()) {
            writeOutputFile(options.saveAst, decoratedAstBuffer.str());
        }

        const bool semanticFailed = scopeBuilder.hasErrors() || typeChecker.hasErrors();
        if (semanticFailed) {
            // Milestone 3 failures must be observable from the process status.
            // The decorated AST and symbol tables are still written first so the
            // user/test can inspect the exact semantic/type error details.
            if (!options.verbose && options.saveAst.empty()) {
                std::cout << decoratedAstBuffer.str();
            }
            return 1;
        }

        if (options.printTac || options.run) {
            backend::InstructionProgram tac;
            try {
                backend::CodeGenerator codeGenerator;
                tac = codeGenerator.generate(astRoot.get(), scopeBuilder.symbolTable());
            } catch (const std::runtime_error& e) {
                std::cout << e.what() << "\n";
                return 1;
            }

            if (options.printTac) {
                tac.prettyPrint(std::cout);
                return 0;
            }

            try {
                backend::Interpreter interpreter;
                interpreter.execute(tac);
                std::cout << interpreter.getOutput();
            } catch (const std::runtime_error& e) {
                std::cout << e.what() << "\n";
                return 1;
            }
        }

        return 0;
    } catch (const LexerException& e) {
        std::cout << e.full_message() << "\n";
        return 1;
    } catch (const ParseException& e) {
        std::cout << e.full_message() << "\n";
        return 1;
    } catch (const std::runtime_error& e) {
        std::cout << e.what() << "\n";
        printUsage(programName);
        return 1;
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
