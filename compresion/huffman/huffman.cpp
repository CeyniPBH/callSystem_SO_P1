#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <fstream>

//Mostrar mensaje de ayuda
void show_help(){
    std::cout << "Uso: Compresor [opciones] <archivo>" << std::endl;
    std::cout << "Opciones:" << std::endl;
    std::cout << "  -h, --help: Mostrar este mensaje de ayuda" << std::endl;
    std::cout << "  -v, --version: Mostrar información sobre el autor del programa" << std::endl;
    std::cout << "  -c, --compress: Comprimir el archivo" << std::endl;
    std::cout << "  -x, --decompress: Descomprimir el archivo" << std::endl;
    
}
// Mostrar la versión del programa
void show_version(){
    std::cout << "Compresor 1.0" << std::endl;
}

// Definición de la estructura del nodo del árbol de Huffman
struct Nodo {
    unsigned char caracter;
    int frecuencia;
    Nodo* izquierda;
    Nodo* derecha;

    // Constructor del nodo
    Nodo(unsigned char c, int f, Nodo* izq = nullptr, Nodo* der = nullptr) 
        : caracter(c), frecuencia(f), izquierda(izq), derecha(der) {}
};

// Comparador para la cola de prioridad, ordena por frecuencia ascendente
struct Comparar {
    bool operator()(Nodo* a, Nodo* b) {
        return a->frecuencia > b->frecuencia; // Menor frecuencia tiene mayor prioridad
    }
};

// Construcción del árbol de Huffman basado en las frecuencias
Nodo* construirArbolHuffman(const std::unordered_map<unsigned char, int>& frecuencias) {
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
void generarCodigos(Nodo* raiz, std::string codigo, std::unordered_map<unsigned char, std::string>& codigos) {
    if (!raiz) return;

    // Si el nodo no es nulo y es una hoja, almacenar el código generado
    if (raiz->caracter != '\0')
        codigos[raiz->caracter] = codigo;
    
    // Recorrer la izquierda con un "0"
    generarCodigos(raiz->izquierda, codigo + "0", codigos);
    // Recorrer la derecha con un "1"
    generarCodigos(raiz->derecha, codigo + "1", codigos);
}

// Comprimir archivo
void compress(const std::string& filename) {

    // Abrir el archivo en modo lectura.
    int fd = open(filename.c_str(), O_RDONLY); //.c_str sirve para convertir un string a un char*
    if (fd == -1) {
        perror("Error al abrir el archivo");
        return;
    }   
    // Leer el archivo y contar las frecuencias de los caracteres
    unsigned char buffer[5600];
    std::unordered_map<unsigned char, int> frecuencias;
    ssize_t bytesLeidos;
    while((bytesLeidos= read(fd, buffer, sizeof(buffer)))>0){ // Lee hasta 127 bytes
        for (ssize_t i = 0; i < bytesLeidos; i++) {
            frecuencias[buffer[i]]++; // Incrementa la frecuencia del carácter
        }
    }
    close(fd); // Cierra el archivo original

    // Construir el árbol de Huffman
    Nodo* raiz = construirArbolHuffman(frecuencias);

    // Generar los códigos de Huffman
    std::unordered_map<unsigned char, std::string> codigos; //codigos es un mapa que contiene el caracter y su código binario.
    generarCodigos(raiz, "", codigos);
    
    // Crear el archivo comprimido
    std::string nombreBase = filename.substr(0, filename.find_last_of(".")); 
    std::ofstream archivoComprimido(nombreBase + ".huff", std::ios::binary);
    if (!archivoComprimido.is_open()) {
        perror("Error al crear el archivo comprimido");
        return;
    }
    // Escribir el número de caracteres distintos del archivo original.
    int numCaracteresDistintos = frecuencias.size();
    archivoComprimido.write(reinterpret_cast<const char*>(&numCaracteresDistintos), sizeof(int)); // el reinterpret_cast convierte el puntero de int a char
    
    // Escribir las frecuencias de los caracteres
    for (auto& par: codigos){
        archivoComprimido.put(par.first); // Escribir el carácter
        u_int8_t longitud = par.second.size(); // Longitud del código, donde u_int8_t es un entero sin signo de 8 bits
        archivoComprimido.put(longitud); // Escribir la longitud del código binario
        for (char bit : par.second) {
            archivoComprimido.put(bit == '1' ? 1 : 0); // Escribir el bit
        }
    }
    
    // Abrir el archivo original
    fd = open(filename.c_str(), O_RDONLY); 
    if (fd == -1) {
        perror("Error al reabrir el archivo para comprimir");
        return;
    }
    // Leer el archivo original y escribir el archivo comprimido
    std::string bufferComprimido;
    while ((bytesLeidos = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytesLeidos; i++) {
            bufferComprimido += codigos[buffer[i]]; // Concatenar el código del carácter con el código binario
        }
    }
    
    while (bufferComprimido.size() % 8 != 0) {
        bufferComprimido += "0"; // Rellenar con ceros el total del código para que sea múltiplo de 8
    }

    size_t totalBytes = bufferComprimido.size() / 8;
    for (size_t i = 0; i < totalBytes; i++) {
        u_int8_t byte = 0;
        for (size_t j = 0; j < 8; j++) {
            byte |= (bufferComprimido[i * 8 + j] - '0') << (7 - j); // Crear el byte
        }
        archivoComprimido.put(byte); // Escribir el byte
    }

    close(fd); // Cierra el archivo original
    archivoComprimido.close(); // Cierra el archivo comprimido

    std::cout << "Archivo comprimido con éxito como: " << nombreBase + ".huff" << std::endl;


}

void decompress(const std::string& filename) {
    std::cout << "Descomprimiendo archivo: " << filename << std::endl;
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        show_help();
        return 1;
    }
    if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
        show_help();
    } else if (std::string(argv[1]) == "-v" || std::string(argv[1]) == "--version") {
        show_version();
    } else if (std::string(argv[1]) == "-c" || std::string(argv[1]) == "--compress") {
        compress(argv[2]);
    } else if (std::string(argv[1]) == "-x" || std::string(argv[1]) == "--decompress")
    {
        decompress(argv[2]);
    } else {
        std:perror("Opción no reconocida. Use -h o --help para obtener ayuda.");
        return 1;
    }

    return 0;
}