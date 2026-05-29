#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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
    std::string outputFile;
    bool lexOnly = false;
    bool verbose = false;
};

void printUsage(const char* programName) {
    std::cout
        << "Usage:\n"
        << "  " << programName << " <program.txt> --lex-only -o <tokens.txt>\n"
        << "  " << programName << " <program.txt> [--verbose]\n"
        << "  " << programName << " <program.txt> --save-tokens <file> [--save-parse-tree <file>] [--save-ast <file>] [--verbose]\n\n"
        << "Options:\n"
        << "  --lex-only              Run lexical analysis only, print tokens, then stop\n"
        << "  --verbose               Print tokens, parse tree, and decorated AST to stdout\n"
        << "  --save-tokens <file>    Save tokens to <file>\n"
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

        if (arg == "--lex-only") {
            options.lexOnly = true;
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

    if (!options.outputFile.empty()) {
        if (options.lexOnly) {
            options.saveTokens = options.outputFile;
        } else {
            options.saveAst = options.outputFile;
        }
    }

    return options;
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
        std::ostringstream parseTreeBuffer;
        std::ostringstream decoratedAstBuffer;

        auto dfa = std::make_shared<DFA>();
        dfa->loadConfig("config/config_lexer.txt");

        Lexer lexer(inputFile, dfa, &tokenBuffer);
        while (!lexer.eof()) {
            lexer.process_next_token();
        }

        if (!options.saveTokens.empty()) {
            writeOutputFile(options.saveTokens, tokenBuffer.str());
        }

        if (options.lexOnly) {
            std::cout << tokenBuffer.str();
            return 0;
        }

        std::vector<Token> parserTokens;
        parserTokens.reserve(lexer.getResult().size());
        for (const Token& token : lexer.getResult()) {
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
        parse_tree::NodePtr parseRoot = parser.parse();
        parse_tree::printTree(parseRoot.get(), parseTreeBuffer);

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

        if (!options.saveParseTree.empty()) {
            writeOutputFile(options.saveParseTree, parseTreeBuffer.str());
        }

        if (!options.saveAst.empty()) {
            writeOutputFile(options.saveAst, decoratedAstBuffer.str());
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
