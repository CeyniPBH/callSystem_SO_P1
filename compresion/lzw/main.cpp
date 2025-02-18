#include "lzw.h"
#include <iostream>
#include <cstring>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        showHelp();
        return 1;
    }

    bool compress = false, decompress = false;
    std::string filename;


    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            showHelp();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            showVersion();
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compress") == 0) {
            if (i + 1 < argc) {
                filename = argv[i + 1];
                compress = true;
                i++; 
            } else {
                std::cerr << "Error: Falta el nombre del archivo para la compresión" << std::endl;
                return 1;
            }
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--decompress") == 0) {
            if (i + 1 < argc) {
                filename = argv[i + 1];
                decompress = true;
                i++; 
            } else {
                std::cerr << "Error: Falta el nombre del archivo para la descompresión" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: Opción desconocida: " << argv[i] << std::endl;
            std::cerr << "Use --help para obtener información de uso" << std::endl;
            return 1;
        }
    }

    if (!compress && !decompress) {
        std::cerr << "Error: Debe especificar una operación (comprimir o descomprimir)" << std::endl;
        return 1;
    }

    if (compress && decompress) {
        std::cerr << "Error: No puede especificar comprimir y descomprimir al mismo tiempo" << std::endl;
        return 1;
    }

    if (compress) {
        if (!compressFile(filename)) {
            return 1;
        }
    } else if (decompress) {
        if (!decompressFile(filename)) {
            return 1;
        }
    }

    return 0;
}