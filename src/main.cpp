#include <fstream>
#include <iostream>

int main(int argc, char* argv[]){
    if (argc < 2){
        std::cerr << "Usage: ./arion <program.txt> [-o <output_file.txt>]\n";
        return 1;
    }

    std::string output_filename;
    if (argc > 3 && std::string(argv[2]) == "-o") {
        output_filename = argv[3];
    } else {
        output_filename = argv[1];
        output_filename = "out_" + output_filename;
    }
}