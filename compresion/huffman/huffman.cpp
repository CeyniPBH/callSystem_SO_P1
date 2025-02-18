#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>

// Mostrar mensaje de ayuda
void show_help() {
    std::cout << "Uso: Compresor [opciones] <archivo>" << std::endl;
    std::cout << "Opciones:" << std::endl;
    std::cout << "  -h, --help: Mostrar este mensaje de ayuda" << std::endl;
    std::cout << "  -v, --version: Mostrar información sobre el autor del programa" << std::endl;
    std::cout << "  -c, --compress: Comprimir el archivo" << std::endl;
    std::cout << "  -x, --decompress: Descomprimir el archivo" << std::endl;
}

// Mostrar la versión del programa
void show_version() {
    std::cout << "Compresor 1.0" << std::endl;
}

// Definición de la estructura del nodo del árbol de Huffman
struct Nodo {
    unsigned char caracter;
    int frecuencia;
    Nodo* izquierda;
    Nodo* derecha;

    Nodo(unsigned char c, int f, Nodo* izq = nullptr, Nodo* der = nullptr) 
        : caracter(c), frecuencia(f), izquierda(izq), derecha(der) {}

    ~Nodo() {
        delete izquierda;
        delete derecha;
    }
};

// Comparador para la cola de prioridad, ordena por frecuencia ascendente
struct Comparar {
    bool operator()(Nodo* a, Nodo* b) {
        return a->frecuencia > b->frecuencia;
    }
};

// Construcción del árbol de Huffman basado en las frecuencias
Nodo* construirArbolHuffman(const std::unordered_map<unsigned char, int>& frecuencias) {
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

// Generación de los códigos de Huffman recorriendo el árbol
void generarCodigos(Nodo* raiz, std::string codigo, std::unordered_map<unsigned char, std::string>& codigos) {
    if (!raiz) return;
    if (raiz->caracter != '\0')
        codigos[raiz->caracter] = codigo;
    generarCodigos(raiz->izquierda, codigo + "0", codigos);
    generarCodigos(raiz->derecha, codigo + "1", codigos);
}

// Comprimir archivo
void compress(const std::string& filename) {
    std::ifstream archivoOriginal(filename, std::ios::binary);
    if (!archivoOriginal.is_open()) {
        perror("Error al abrir el archivo original");
        return;
    }
    
    // Contar las frecuencia de cada caracter
    std::unordered_map<unsigned char, int> frecuencias;
    char ch;
    while (archivoOriginal.get(ch)) { // Leer el archivo caracter por caracter
        frecuencias[static_cast<unsigned char>(ch)]++; // Incrementar la frecuencia del caracter
    }
    archivoOriginal.clear();
    archivoOriginal.seekg(0);

    // Construir el árbol de Huffman y generar los códigos
    Nodo* raiz = construirArbolHuffman(frecuencias);

    std::unordered_map<unsigned char, std::string> codigos;
    generarCodigos(raiz, "", codigos);

    // Crear el archivo comprimido
    std::string nombreBase = filename.substr(0, filename.find_last_of(".")); // Nombre del archivo sin extensión
    std::ofstream archivoComprimido(nombreBase+ ".huff", std::ios::binary);
    int numCaracteresDistintos = frecuencias.size();
    archivoComprimido.write(reinterpret_cast<const char*>(&numCaracteresDistintos), sizeof(int));
    for (auto& par : codigos){
        archivoComprimido.put(par.first);
        u_int8_t longitud = par.second.size();
        archivoComprimido.put(longitud);
        archivoComprimido.write(par.second.data(), longitud);
    }

    // Crear los metadatos del archivo comprimido
    std::string bufferComprimido;
    while (archivoOriginal.get(ch)) {
        bufferComprimido += codigos[static_cast<unsigned char>(ch)];
    }
    archivoOriginal.close();

    // Rellenar con ceros los datos faltantes para que la longitud sea múltiplo de 8
    u_int8_t padding = 8 - (bufferComprimido.size() % 8);
    for (int i = 0; i < padding; i++) bufferComprimido += "o";
        archivoComprimido.put(padding);

    // Escribir los datos comprimidos en el archivo
    for (size_t i=0; i < bufferComprimido.size(); i += 8) {
        u_int8_t byte = 0;
        for (size_t j = 0; j < 8; j++) {
            byte = (byte << 1) | (bufferComprimido[i + j] - '0');
        }
        archivoComprimido.put(byte);
    }
    // Liberar memoria y cerrar archivos
    archivoComprimido.close();
    delete raiz;
    std::cout << "Archivo comprimido con éxito como: " << nombreBase + ".huff" << std::endl;
}

// Descomprimir archivo
void decompress(const std::string& filename) {
    std::ifstream archivoComprimido(filename, std::ios::binary);
    if (!archivoComprimido.is_open()) {
        perror("Error al abrir el archivo comprimido");
        return;
    }

    // Leer los metadatos del archivo comprimido
    int numCaracteresDistintos;
    archivoComprimido.read(reinterpret_cast<char*>(&numCaracteresDistintos), sizeof(int));

    // Leer los códigos de Huffman
    std::unordered_map<std::string, unsigned char> codigosInversos;
    for (int i = 0; i < numCaracteresDistintos; i++) {
        unsigned char caracter = archivoComprimido.get();
        u_int8_t longitud = archivoComprimido.get();
        std::string codigo;
        char* buffer = new char[longitud];
        archivoComprimido.read(buffer, longitud);
        codigo.assign(buffer, longitud);
        delete[] buffer;
        codigosInversos[codigo] = caracter;
    }

    // Leer el padding
    u_int8_t padding;
    archivoComprimido.get(reinterpret_cast<char&>(padding));
    std::string bufferBits;
    char byte;
    while (archivoComprimido.get(byte)) { // Leer el archivo comprimido byte por byte
        for (int i = 7; i >= 0; i--) {
            bufferBits += ((byte >> i) & 1) ? '1' : '0';
        }
    }

    archivoComprimido.close();
    // Leer el contenido comprimido del archivo
    bufferBits = bufferBits.substr(0, bufferBits.size() - padding); // Eliminar los bits de relleno
    
    std::string codigoActual;
    std::string outputFilename = filename.substr(0, filename.find_last_of("."));
    std::ofstream archivoOriginal(outputFilename, std::ios::binary);
    for (char bit : bufferBits) {
        codigoActual += bit;
        if (codigosInversos.count(codigoActual)) {
            archivoOriginal.put(codigosInversos[codigoActual]);
            codigoActual.clear();
        }
    }

    archivoOriginal.close();
    std::cout << "Archivo descomprimido con éxito: " << outputFilename << std::endl;
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
        compress(argv[2]);
    } else if (opcion == "-x" || opcion == "--decompress") {
        decompress(argv[2]);
    } else {
        std::cerr << "Opción no reconocida. Use -h o --help para obtener ayuda." << std::endl;
        return 1;
    }
    return 0;
}