#include "huffman.h"
#include <iostream>
#include <queue>
#include <fstream>
#include <unordered_map>
#include <bitset>

struct Comparar {
    bool operator()(Nodo* a, Nodo* b) {
        return a->frecuencia > b->frecuencia;
    }
};

Nodo* ConstruirArbolHuffman(const std::unordered_map<unsigned char, int>& frecuencias) {
    std::priority_queue<Nodo*, std::vector<Nodo*>, Comparar> cola;
    for (auto& par : frecuencias)
        cola.push(new Nodo(par.first, par.second));
    while (cola.size() > 1) {
        Nodo* izquierda = cola.top(); cola.pop();
        Nodo* derecha = cola.top(); cola.pop();
        Nodo* padre = new Nodo('\0', izquierda->frecuencia + derecha->frecuencia, izquierda, derecha);
        cola.push(padre);
    }
    return cola.top();
}

void generarCodigo(Nodo* raiz, std::string codigo, std::unordered_map<unsigned char, std::string>& codigos) {
    if (!raiz) return;
    if (raiz->caracter != '\0')
        codigos[raiz->caracter] = codigo;
    generarCodigo(raiz->izquierda, codigo + "0", codigos);
    generarCodigo(raiz->derecha, codigo + "1", codigos);
}

void compress(const std::string& filename){
    std::ifstream archivoOriginal(filename, std::ios::binary);
    if (!archivoOriginal.is_open()) {
        perror("Error al abrir el archivo original");
        return;
    }

    std::unordered_map<unsigned char, int> frecuencias;
    char ch;
    while (archivoOriginal.get(ch)) {
        frecuencias[static_cast<unsigned char>(ch)]++;
    }
    archivoOriginal.clear();
    archivoOriginal.seekg(0);

    Nodo* raiz = ConstruirArbolHuffman(frecuencias);
    std::unordered_map<unsigned char, std::string> codigos;
    generarCodigo(raiz, "", codigos);

    std::string nombreBase = filename.substr(0, filename.find_last_of("."));
    std::ofstream archivoComprimido(nombreBase + ".huff", std::ios::binary);
    if (!archivoComprimido.is_open()) {
        perror("Error al crear el archivo comprimido");
        return;
    }

    int numCaracteresDistintos = codigos.size();
    archivoComprimido.write(reinterpret_cast<const char*>(&numCaracteresDistintos), sizeof(int));
    for (auto& par : codigos) {
        archivoComprimido.put(par.first);
        u_int8_t longitud = par.second.size();
        archivoComprimido.put(longitud);
        archivoComprimido.write(par.second.c_str(), longitud);
    }

    std::string bufferComprimido;
    while (archivoOriginal.get(ch)) {
        bufferComprimido += codigos[static_cast<unsigned char>(ch)];
    }
    archivoOriginal.close();

    while (bufferComprimido.size() % 8 != 0) {
        bufferComprimido += "0";
    }

    for (size_t i = 0; i < bufferComprimido.size(); i += 8) {
        u_int8_t byte = 0;
        byte = 0;
        for (size_t j = 0; j < 8; j++) {
            byte = (byte << 1) | (bufferComprimido[i + j] - '0');
        }
        archivoComprimido.put(byte);
    }

    archivoComprimido.close();
    delete raiz;
    std::cout << "Archivo comprimido con éxito como: " << nombreBase + ".huff" << std::endl;
}

void decompress(const std::string& filename) {
    std::ifstream archivoComprimido(filename, std::ios::binary);
    if (!archivoComprimido.is_open()) {
        perror("Error al abrir el archivo comprimido");
        return;
    }

    int numCaracteresDistintos;
    archivoComprimido.read(reinterpret_cast<char*>(&numCaracteresDistintos), sizeof(int));

    std::unordered_map<std::string, unsigned char> codigosInversos;
    for (int i = 0; i < numCaracteresDistintos; i++) {
        unsigned char caracter = archivoComprimido.get();
        u_int8_t longitud = archivoComprimido.get();
        std::string codigo;
        for (int j = 0; j < longitud; j++) {
            char bit;
            archivoComprimido.get(bit);
            codigo += (bit == 1) ? '1' : '0';
        }
        codigosInversos[codigo] = caracter;
    }

    std::string nombreBase = filename.substr(0, filename.find_last_of("."));
    std::ofstream archivoOriginal(nombreBase, std::ios::binary);
    if (!archivoOriginal.is_open()) {
        perror("Error al crear el archivo original");
        return;
    }

    std::string bufferBits;
    char byte;
    while (archivoComprimido.get(byte)) {
        bufferBits += std::bitset<8>(byte).to_string();
    }

    std::string codigoActual;
    for (char bit : bufferBits) {
        codigoActual += bit;
        if (codigosInversos.count(codigoActual)) {
            archivoOriginal.put(codigosInversos[codigoActual]);
            codigoActual.clear();
        }
    }

    archivoComprimido.close();
    archivoOriginal.close();
    std::cout << "Archivo descomprimido con éxito: " << nombreBase << std::endl;
}