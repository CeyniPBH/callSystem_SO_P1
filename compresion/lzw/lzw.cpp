#include "lzw.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cstdlib>


void showHelp() {
    std::cout << "Uso: lzw [OPCIONES] [ARCHIVO]\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  -h, --help                     Muestra este mensaje de ayuda\n";
    std::cout << "  -v, --version                  Muestra la versión del programa\n";
    std::cout << "  -c <archivo>, --compress <archivo> Comprime el archivo especificado\n";
    std::cout << "  -x <archivo>, --decompress <archivo> Descomprime el archivo especificado\n";
}

void showVersion() {
    std::cout << "LZW Compression Tool v" << VERSION << "\n";
}


bool compressFile(const std::string& filename) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile) {
        std::cerr << "Error: No se pudo abrir el archivo: " << filename << std::endl;
        return false;
    }

    std::string outputFilename = filename + ".lzw";
    std::ofstream outFile(outputFilename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: No se pudo crear el archivo de salida: " << outputFilename << std::endl;
        return false;
    }

    std::map<std::string, int> dictionary;
    for (int i = 0; i < 256; i++) {
        std::string s = "";
        s += char(i);
        dictionary[s] = i;
    }

    std::string buffer;
    char c;
    std::vector<int> result;
    int nextCode = 256;

    while (inFile.get(c)) {
        std::string currentBuffer = buffer + c;
        if (dictionary.count(currentBuffer)) {
            buffer = currentBuffer;
        } else {
            result.push_back(dictionary[buffer]);
            dictionary[currentBuffer] = nextCode++;
            buffer = std::string(1, c);
        }
    }


    if (!buffer.empty()) {
        result.push_back(dictionary[buffer]);
    }


    int resultSize = result.size();
    outFile.write(reinterpret_cast<const char*>(&resultSize), sizeof(resultSize));


    for (int code : result) {
        outFile.write(reinterpret_cast<const char*>(&code), sizeof(code));
    }

    inFile.close();
    outFile.close();

    std::cout << "Archivo comprimido exitosamente como: " << outputFilename << std::endl;
    return true;
}


bool decompressFile(const std::string& filename) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile) {
        std::cerr << "Error: No se pudo abrir el archivo: " << filename << std::endl;
        return false;
    }


    if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".lzw") {
        std::cerr << "Error: El archivo no tiene la extensión .lzw" << std::endl;
        return false;
    }

    std::string outputFilename = filename.substr(0, filename.length() - 4);
    std::ofstream outFile(outputFilename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: No se pudo crear el archivo de salida: " << outputFilename << std::endl;
        return false;
    }


    int resultSize;
    inFile.read(reinterpret_cast<char*>(&resultSize), sizeof(resultSize));

    std::vector<int> compressed;
    for (int i = 0; i < resultSize; i++) {
        int code;
        inFile.read(reinterpret_cast<char*>(&code), sizeof(code));
        compressed.push_back(code);
    }

    std::map<int, std::string> dictionary;
    for (int i = 0; i < 256; i++) {
        dictionary[i] = std::string(1, char(i));
    }


    std::string entry;
    int nextCode = 256;
    
    if (compressed.empty()) {
        inFile.close();
        outFile.close();
        std::cout << "Archivo descomprimido exitosamente como: " << outputFilename << std::endl;
        return true;
    }

    std::string result = dictionary[compressed[0]];
    outFile.write(result.c_str(), result.length());
    
    for (size_t i = 1; i < compressed.size(); i++) {
        int code = compressed[i];
        
        if (dictionary.count(code)) {
            entry = dictionary[code];
        } else if (code == nextCode) {
            entry = result + result[0];
        } else {
            std::cerr << "Error: Código inválido encontrado durante la descompresión" << std::endl;
            inFile.close();
            outFile.close();
            return false;
        }
        
        outFile.write(entry.c_str(), entry.length());
        
        dictionary[nextCode++] = result + entry[0];
        result = entry;
    }

    inFile.close();
    outFile.close();

    std::cout << "Archivo descomprimido exitosamente como: " << outputFilename << std::endl;
    return true;
}