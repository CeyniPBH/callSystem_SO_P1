#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <string>

// Definición de la estructura del nodo del árbol de Huffman
class Nodo {
public:
    unsigned char caracter;
    int frecuencia;
    Nodo* izquierda;
    Nodo* derecha;

    // Constructor y destructor
    Nodo(unsigned char c, int f, Nodo* izq = nullptr, Nodo* der = nullptr)
        : caracter(c), frecuencia(f), izquierda(izq), derecha(der) {}

    ~Nodo() {
        delete izquierda; // Liberar memoria de los nodos hijos
        delete derecha;
    }
};

void compress(const std::string& filename);
void decompress(const std::string& filename);

#endif // HUFFMAN_H