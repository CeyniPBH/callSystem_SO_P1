#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>

// Definición de la estructura del nodo del árbol de Huffman
struct Nodo {
    char caracter;
    int frecuencia;
    Nodo* izquierda;
    Nodo* derecha;

    // Constructor del nodo
    Nodo(char c, int f, Nodo* izq = nullptr, Nodo* der = nullptr) 
        : caracter(c), frecuencia(f), izquierda(izq), derecha(der) {}
};

// Comparador para la cola de prioridad, ordena por frecuencia ascendente
struct Comparar {
    bool operator()(Nodo* a, Nodo* b) {
        return a->frecuencia > b->frecuencia; // Menor frecuencia tiene mayor prioridad
    }
};

// Construcción del árbol de Huffman basado en las frecuencias
Nodo* construirArbolHuffman(const std::unordered_map<char, int>& frecuencias) {
    std::priority_queue<Nodo*, std::vector<Nodo*>, Comparar> cola;

    // Insertamos cada carácter en la cola de prioridad
    for (auto& par : frecuencias)
        cola.push(new Nodo(par.first, par.second));

    // Construcción del árbol de Huffman
    while (cola.size() > 1) {
        Nodo* izquierda = cola.top(); cola.pop(); // Extrae el nodo con menor frecuencia
        Nodo* derecha = cola.top(); cola.pop();   // Extrae el segundo nodo con menor frecuencia

        // Se crea un nuevo nodo padre con la suma de las frecuencias
        Nodo* padre = new Nodo('\0', izquierda->frecuencia + derecha->frecuencia, izquierda, derecha);
        cola.push(padre);
    }

    return cola.top(); // Raíz del árbol de Huffman
}

// Generación de los códigos de Huffman recorriendo el árbol
void generarCodigos(Nodo* raiz, std::string codigo, std::unordered_map<char, std::string>& codigos) {
    if (!raiz) return;

    // Si el nodo no es nulo y es una hoja, almacenar el código generado
    if (raiz->caracter != '\0')
        codigos[raiz->caracter] = codigo;
    
    // Recorrer la izquierda con un "0"
    generarCodigos(raiz->izquierda, codigo + "0", codigos);
    // Recorrer la derecha con un "1"
    generarCodigos(raiz->derecha, codigo + "1", codigos);
}

int main() {
    // Ejemplo de frecuencias de caracteres
    std::unordered_map<char, int> frecuencias = {
        {'a', 5}, {'b', 9}, {'c', 12}, {'d', 13}, {'e', 16}, {'f', 45}
    };

    // Construir el árbol de Huffman
    Nodo* raiz = construirArbolHuffman(frecuencias);

    // Generar los códigos de Huffman
    std::unordered_map<char, std::string> codigos;
    generarCodigos(raiz, "", codigos);

    // Imprimir los códigos de Huffman
    for (auto& par : codigos) {
        std::cout << par.first << ": " << par.second << std::endl;
    }

    return 0;
}
