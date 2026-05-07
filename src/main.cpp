#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "lexer/dfa.hpp"
#include "lexer/lexer.hpp"
#include "lexer/lexer_exception.hpp"
#include "parser/parser.hpp"
#include "parser/parse_tree.hpp"

namespace fs = std::filesystem;

void printUsage(const char* programName) {
    std::cerr
        << "Usage:\n"
        << "  " << programName
        << " <program.txt> [-o <token_output.txt>] [-p <parse_tree_output.txt>] [--lex-only]\n\n"
        << "Options:\n"
        << "  -o <file>     Save lexer/token output to <file>\n"
        << "  -p <file>     Save parser/parse-tree output to <file>\n"
        << "  --lex-only    Run only lexer, do not run parser\n";
}

std::string defaultOutputPath(const std::string& inputFilename,
                              const std::string& milestoneFolder) {
    fs::path inputPath(inputFilename);
    std::string stem = inputPath.stem().string();

    return (fs::path("test") / milestoneFolder / "output" / (stem + ".txt")).string();
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFilename = argv[1];
    std::string tokenOutputFilename;
    std::string parseTreeOutputFilename;
    bool lexOnly = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "ERROR: -o membutuhkan nama file output.\n";
                printUsage(argv[0]);
                return 1;
            }

            tokenOutputFilename = argv[++i];
        } else if (arg == "-p") {
            if (i + 1 >= argc) {
                std::cerr << "ERROR: -p membutuhkan nama file output parse tree.\n";
                printUsage(argv[0]);
                return 1;
            }

            parseTreeOutputFilename = argv[++i];
        } else if (arg == "--lex-only") {
            lexOnly = true;
        } else {
            std::cerr << "ERROR: Argumen tidak dikenal: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (tokenOutputFilename.empty()) {
        tokenOutputFilename = defaultOutputPath(inputFilename, "milestone1");
    }

    if (parseTreeOutputFilename.empty()) {
        parseTreeOutputFilename = defaultOutputPath(inputFilename, "milestone2");
    }

    std::ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        std::cerr << "Gagal membuka file input: " << inputFilename << "\n";
        return 1;
    }

    try {
        ensureParentDirectoryExists(tokenOutputFilename);

        std::ofstream tokenOutputFile(tokenOutputFilename);
        if (!tokenOutputFile.is_open()) {
            std::cerr << "Gagal membuka file output token: " << tokenOutputFilename << "\n";
            return 1;
        }

        auto dfa = std::make_shared<DFA>();
        dfa->loadConfig("config/config_lexer.txt");

        Lexer lexer(inputFile, dfa, &tokenOutputFile);

        while (!lexer.eof()) {
            lexer.process_next_token();
        }

        tokenOutputFile.close();

        std::cout << "Lexing selesai.\n";
        std::cout << "Output token disimpan di: " << tokenOutputFilename << "\n";

        if (lexOnly) {
            return 0;
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
        parse_tree::NodePtr root = parser.parse();

        ensureParentDirectoryExists(parseTreeOutputFilename);

        std::ofstream parseTreeOutputFile(parseTreeOutputFilename);
        if (!parseTreeOutputFile.is_open()) {
            std::cerr << "Gagal membuka file output parse tree: "
                      << parseTreeOutputFilename << "\n";
            return 1;
        }

        parse_tree::printTree(root.get(), std::cout);
        parse_tree::printTree(root.get(), parseTreeOutputFile);
        parseTreeOutputFile.close();

        std::cout << "Parsing selesai.\n";
        std::cout << "Output parse tree disimpan di: "
                  << parseTreeOutputFilename << "\n";
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