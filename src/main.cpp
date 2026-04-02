#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "lexer/lexer.hpp"
#include "lexer/dfa.hpp"
#include "lexer/lexer_exception.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./arion <program.txt> [-o <output_file.txt>]\n";
        return 1;
    }

    std::string input_filename = argv[1];
    std::string output_filename;

    if (argc > 3 && std::string(argv[2]) == "-o") {
        output_filename = argv[3];
    } else {
        output_filename = "out_" + input_filename;
    }

    std::ifstream input_file(input_filename);
    if (!input_file.is_open()) {
        std::cerr << "Gagal membuka file input: " << input_filename << "\n";
        return 1;
    }

    std::ofstream output_file(output_filename);
    if (!output_file.is_open()) {
        std::cerr << "Gagal membuka file output: " << output_filename << "\n";
        return 1;
    }

    try {
        auto dfa = std::make_shared<DFA>();

        // Ganti path ini jika file konfigurasi DFA Anda berada di lokasi lain
        dfa->loadConfig("config/dfa.txt");

        Lexer lexer(input_file, dfa, &output_file);

        while (!lexer.eof()) {
            lexer.get_next_token();
        }

        std::cout << "Lexing selesai.\n";
        std::cout << "Output disimpan di: " << output_filename << "\n";
    }
    catch (const LexerException& e) {
        std::cerr << e.full_message() << "\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}