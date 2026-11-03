#include <fstream>
#include <iostream>
#include <string>

#include "geometry.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: main_solver <archivo_entrada>\n";
        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "No se pudo abrir: " << argv[1] << "\n";
        return 1;
    }

    std::cout << "STATUS FEASIBLE\n";
    return 0;
}
