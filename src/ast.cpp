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
#include "semantic/ast_builder.hpp"
#include "semantic/ast_nodes.hpp"

namespace fs = std::filesystem;

namespace {

void printUsage(const char* programName) { // EDIT MARK
    std::cerr
        << "Usage:\n"
        << "  " << programName << " <program.txt> [-o <ast_output.txt>]\n\n"
        << "Options:\n"
        << "  -o <file>     Save AST output to file instead of stdout\n";
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

}

int main(int argc, char* argv[]) { // EDIT MARK
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFilename = argv[1];
    std::string astOutputFilename;

    for (int i = 2; i < argc; ++i) {
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

    std::ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        std::cerr << "Gagal membuka file input: " << inputFilename << "\n";
        return 1;
    }

    try {
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
            if (isIgnoredByParser(token)) {
                continue;
            }

            if (isLexicalErrorToken(token)) {
                std::cerr << "Lexical error: token tidak dikenal "
                          << token.to_string() << "\n";
                return 1;
            }

            parserTokens.push_back(token);
        }

        Parser parser(parserTokens);
        parse_tree::NodePtr parseRoot = parser.parse();

        AstBuilder builder;
        semantic::AstPtr astRoot = builder.build(parseRoot);

        if (astOutputFilename.empty()) {
            semantic::printAst(std::cout, astRoot.get());
        } else {
            ensureParentDirectoryExists(astOutputFilename);
            std::ofstream astOutputFile(astOutputFilename);
            if (!astOutputFile.is_open()) {
                std::cerr << "Gagal membuka file output AST: "
                          << astOutputFilename << "\n";
                return 1;
            }
            semantic::printAst(astOutputFile, astRoot.get());
            std::cout << "AST output disimpan di: " << astOutputFilename << "\n";
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
