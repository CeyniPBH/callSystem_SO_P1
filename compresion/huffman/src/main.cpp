#include "huffman.h"
#include <iostream>


void show_help() {
    std::cout << "Uso: Compresor [opciones] <archivo>" << std::endl;
    std::cout << "Opciones:" << std::endl;
    std::cout << "  -h, --help: Mostrar este mensaje de ayuda" << std::endl;
    std::cout << "  -v, --version: Mostrar información sobre el autor del programa" << std::endl;
    std::cout << "  -c, --compress: Comprimir el archivo" << std::endl;
    std::cout << "  -x, --decompress: Descomprimir el archivo" << std::endl;
}

void show_version() {
    std::cout << "Compresor 1.0" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }
    std::string opcion = argv[1];
    if (opcion == "-h" || opcion == "--help") {
        show_help();
    } else if (opcion == "-v" || opcion == "--version") {
        show_version();
    } else if (opcion == "-c" || opcion == "--compress") {
        if(argc < 3){
            std::cerr << "Falta el archivo a comprimir" << std::endl;
            return 1;
        }
        compress(argv[2]);
    } else if (opcion == "-x" || opcion == "--decompress") {
        if(argc < 3){
            std::cerr << "Falta el archivo a descomprimir" << std::endl;
            return 1;
        }
        decompress(argv[2]);
    } else {
        std::cerr << "Opción no reconocida. Use -h o --help para obtener ayuda." << std::endl;
        return 1;
    }
    return 0;
}
