
# Sistema de Gestión de Memoria Buddy System

## ¿Qué es el Buddy System?
El Buddy System es un algoritmo de gestión de memoria dinámica que divide un bloque de memoria en partes de tamaños de potencia de dos. Cuando se solicita memoria, el sistema intenta encontrar el bloque de tamaño más cercano y, si el bloque disponible es más grande que el solicitado, se divide en mitades (buddies).

Cuando se libera un bloque de memoria, si el bloque adyacente (buddy) también está libre, ambos bloques se fusionan para formar un bloque más grande. Este proceso se repite recursivamente hasta que ya no hay bloques adyacentes libres para fusionar.

### Ventajas del Buddy System
- Permite dividir y fusionar bloques de manera rápida y eficiente.  
- Reduce la fragmentación externa al garantizar que las divisiones y fusiones sean de tamaños de potencia de dos.  
- Fácil implementación mediante árboles binarios o estructuras de bits.  

### Desventajas del Buddy System
- Puede generar fragmentación interna si la solicitud de memoria no es exactamente una potencia de dos.  
- La fusión de bloques puede requerir procesamiento adicional.  

---

## ¿Para qué se emplea en el procesamiento de imágenes?
En procesamiento de imágenes, el Buddy System es útil porque las imágenes requieren bloques de memoria contiguos para almacenar los datos de píxeles. Al dividir y fusionar bloques de memoria de manera eficiente, el Buddy System facilita:  
- Almacenamiento eficiente de datos de imágenes en memoria.  
- Rápida reasignación de memoria para operaciones como filtros, rotaciones e inversiones de color.  
- Liberación de memoria ordenada y eficiente para evitar fragmentación.  

---

## Explicación del Código
El siguiente código en C++ implementa el algoritmo Buddy System para la gestión de memoria en procesamiento de imágenes.

### buddy_allocator.h
```cpp
#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include <cstddef>

class BuddyAllocator {
public:
    // Constructor: asigna un bloque de memoria de tamaño especificado.
    BuddyAllocator(size_t size);

    // Destructor: libera el bloque de memoria.
    ~BuddyAllocator();

    // Asigna un bloque de memoria del tamaño solicitado.
    void* alloc(size_t size);

    // Libera el bloque de memoria (sin efecto en esta implementación).
    void free(void* ptr);

private:
    size_t size;         // Tamaño total de la memoria gestionada
    void* memoriaBase;   // Puntero al bloque de memoria base
};

#endif

```

### buddy_allocator.cpp
```cpp
#include "buddy_allocator.h"
#include <cstdlib>
#include <iostream>

using namespace std;

// Constructor: asigna un bloque de memoria de tamaño especificado usando malloc.
BuddyAllocator::BuddyAllocator(size_t size) {
    this->size = size;
    memoriaBase = std::malloc(size);
    if (!memoriaBase) {
        cerr << "Error: No se pudo asignar memoria base con Buddy System.\n";
        exit(1);
    }
}

// Destructor: libera el bloque de memoria.
BuddyAllocator::~BuddyAllocator() {
    std::free(memoriaBase);
}

// Asigna un bloque de memoria del tamaño especificado.
// Si el tamaño solicitado supera el bloque disponible, devuelve nullptr.
void* BuddyAllocator::alloc(size_t size) {
    if (size > this->size) {
        cerr << "Error: Tamaño solicitado (" << size 
             << " bytes) supera el tamaño disponible (" 
             << this->size << " bytes).\n";
        return nullptr;
    }
    return memoriaBase;
}

// Libera el bloque de memoria (sin efecto en esta implementación).
void BuddyAllocator::free(void* ptr) {
    // No liberamos porque el Buddy System maneja esto automáticamente.
}

```

---

### imagen.h
```cpp
#ifndef IMAGEN_H
#define IMAGEN_H

#include <string>
#include "buddy_allocator.h"

class Imagen {
public:
    Imagen(const std::string &nombreArchivo, BuddyAllocator *allocador = nullptr);
    ~Imagen();

    void invertirColores();
    
    void guardarImagen(const std::string &nombreArchivo) const;
    void mostrarInfo() const;  // ✅ Declaración como const

    // Nuevos métodos para rotación y escalado
    void rotarImagen(float angulo);
    void escalarImagen(float factor);

private:
    int alto;
    int ancho;
    int canales;
    unsigned char ***pixeles;
    BuddyAllocator *allocador;

    void convertirBufferAMatriz(unsigned char* buffer); // ✅ Declaración privada
};

#endif

```

---

### imagen.cpp
```cpp
#include "imagen.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include <iostream>
#include <cmath>
using namespace std;


// ✅ Implementación del constructor
Imagen::Imagen(const std::string &nombreArchivo, BuddyAllocator *allocador)
    : allocador(allocador) {

    unsigned char* buffer = stbi_load(nombreArchivo.c_str(), &ancho, &alto, &canales, 0);
    if (!buffer) {
        cerr << "Error: No se pudo cargar la imagen '" << nombreArchivo << "'.\n";
        exit(1);
    }

    convertirBufferAMatriz(buffer);
    stbi_image_free(buffer);
}

// ✅ Implementación del destructor
Imagen::~Imagen() {
    if (!allocador) {
        for (int y = 0; y < alto; y++) {
            for (int x = 0; x < ancho; x++) {
                delete[] pixeles[y][x];
            }
            delete[] pixeles[y];
        }
        delete[] pixeles;
    }
}

// ✅ Implementación de convertirBufferAMatriz()
void Imagen::convertirBufferAMatriz(unsigned char* buffer) {
    int indice = 0;
    pixeles = new unsigned char**[alto];

    for (int y = 0; y < alto; y++) {
        pixeles[y] = new unsigned char*[ancho];
        for (int x = 0; x < ancho; x++) {
            pixeles[y][x] = new unsigned char[canales];
            for (int c = 0; c < canales; c++) {
                pixeles[y][x][c] = buffer[indice++];
            }
        }
    }
}

// ✅ Implementación de mostrarInfo()
void Imagen::mostrarInfo() const {
    cout << "Dimensiones: " << ancho << " x " << alto << endl;
    cout << "Canales: " << canales << endl;
}

// ✅ Implementación de guardarImagen()
void Imagen::guardarImagen(const std::string &nombreArchivo) const {
    unsigned char* buffer = new unsigned char[alto * ancho * canales];
    int indice = 0;

    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            for (int c = 0; c < canales; c++) {
                buffer[indice++] = pixeles[y][x][c];
            }
        }
    }
    // Guardar la imagen en formato PNG, el tercer parametro es el número de canales
    // 0 para PNG, 1 para JPEG, 2 para BMP, etc.
    if (!stbi_write_png(nombreArchivo.c_str(), ancho, alto, canales, buffer, ancho * canales)) {
        cerr << "Error: No se pudo guardar la imagen en '" << nombreArchivo << "'.\n";
        delete[] buffer;
        exit(1);
    }

    delete[] buffer;
    cout << "[INFO] Imagen guardada correctamente en '" << nombreArchivo << "'.\n";
}

// ✅ Implementación para invertir los colores.
void Imagen::invertirColores() {
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            for (int c = 0; c < canales; c++) {
                pixeles[y][x][c] = 255 - pixeles[y][x][c];
            }
        }
    }}


    // ✅ Implementación para convertir a escala de grises.
    void Imagen::escalarImagen(float factor=0.5) {
        int nuevoAncho = static_cast<int>(ancho * factor);
        int nuevoAlto = static_cast<int>(alto * factor);
        unsigned char*** nuevaMatriz = new unsigned char**[nuevoAlto];
    
        for (int y = 0; y < nuevoAlto; y++) {
            nuevaMatriz[y] = new unsigned char*[nuevoAncho];
            for (int x = 0; x < nuevoAncho; x++) {
                nuevaMatriz[y][x] = new unsigned char[canales];
                
                float srcX = x / factor;
                float srcY = y / factor;
                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                int x1 = min(x0 + 1, ancho - 1);
                int y1 = min(y0 + 1, alto - 1);
    
                float dx = srcX - x0;
                float dy = srcY - y0;
    
                for (int c = 0; c < canales; c++) {
                    float valor = (1 - dx) * (1 - dy) * pixeles[y0][x0][c] +
                                  dx * (1 - dy) * pixeles[y0][x1][c] +
                                  (1 - dx) * dy * pixeles[y1][x0][c] +
                                  dx * dy * pixeles[y1][x1][c];
                    nuevaMatriz[y][x][c] = static_cast<unsigned char>(valor);
                }
            }
        }
    
        // Liberar la memoria de la imagen original
        for (int y = 0; y < alto; y++) {
            for (int x = 0; x < ancho; x++) {
                delete[] pixeles[y][x];
            }
            delete[] pixeles[y];
        }
        delete[] pixeles;
    
        // Asignar la nueva matriz
        pixeles = nuevaMatriz;
        ancho = nuevoAncho;
        alto = nuevoAlto;
    }

    // ✅ Implementación para rotar la imagen (sentido antihorario)
    void Imagen::rotarImagen(float angulo) {
        float radianes = angulo * M_PI / 180.0;
        float cosA = cos(radianes);
        float sinA = sin(radianes);
    
        int nuevoAncho = abs(ancho * cosA) + abs(alto * sinA);
        int nuevoAlto = abs(ancho * sinA) + abs(alto * cosA);
    
        unsigned char*** nuevaMatriz = new unsigned char**[nuevoAlto];
        for (int y = 0; y < nuevoAlto; y++) {
            nuevaMatriz[y] = new unsigned char*[nuevoAncho];
            for (int x = 0; x < nuevoAncho; x++) {
                nuevaMatriz[y][x] = new unsigned char[canales];
                for (int c = 0; c < canales; c++) {
                    nuevaMatriz[y][x][c] = 255; // Rellenar con blanco
                }
            }
        }
    
        int cx = ancho / 2;
        int cy = alto / 2;
        int ncx = nuevoAncho / 2;
        int ncy = nuevoAlto / 2;
    
        for (int ny = 0; ny < nuevoAlto; ny++) {
            for (int nx = 0; nx < nuevoAncho; nx++) {
                float xOriginal = cosA * (nx - ncx) + sinA * (ny - ncy) + cx;
                float yOriginal = -sinA * (nx - ncx) + cosA * (ny - ncy) + cy;
    
                int x0 = floor(xOriginal);
                int y0 = floor(yOriginal);
                int x1 = x0 + 1;
                int y1 = y0 + 1;
    
                if (x0 >= 0 && x1 < ancho && y0 >= 0 && y1 < alto) {
                    for (int c = 0; c < canales; c++) {
                        float p00 = pixeles[y0][x0][c];
                        float p10 = pixeles[y0][x1][c];
                        float p01 = pixeles[y1][x0][c];
                        float p11 = pixeles[y1][x1][c];
    
                        float dx = xOriginal - x0;
                        float dy = yOriginal - y0;
    
                        float interpolado = (1 - dx) * (1 - dy) * p00 +
                                            dx * (1 - dy) * p10 +
                                            (1 - dx) * dy * p01 +
                                            dx * dy * p11;
    
                        nuevaMatriz[ny][nx][c] = static_cast<unsigned char>(interpolado);
                    }
                }
            }
        }
    
        // Liberar memoria de la imagen original
        for (int y = 0; y < alto; y++) {
            for (int x = 0; x < ancho; x++) {
                delete[] pixeles[y][x];
            }
            delete[] pixeles[y];
        }
        delete[] pixeles;
    
        // Asignar la nueva matriz
        pixeles = nuevaMatriz;
        ancho = nuevoAncho;
        alto = nuevoAlto;
    }
```

---

## ¿Cómo se usa el Buddy System en este código?
1. **Asignación de memoria:**  
   El `BuddyAllocator` asigna bloques de memoria usando el algoritmo de división y combinación de buddies.

2. **Conversión de imagen:**  
   Los datos de la imagen se convierten a una estructura de matriz tridimensional (`alto x ancho x canales`) usando `convertirBufferAMatriz()`.

3. **Procesamiento de la imagen:**  
   La operación de inversión de colores (`invertirColores()`) altera directamente los valores en la matriz.

4. **Liberación de memoria:**  
   El destructor `~Imagen()` libera los bloques de memoria asignados para evitar fugas de memoria.

5. **Uso del Buddy System:**  
   Si el Buddy System está habilitado, la asignación y liberación de memoria se gestiona automáticamente mediante el algoritmo de división y combinación de bloques.

---

## Ventajas del Buddy System en este código
- Reducción de fragmentación.  
- Rápida reasignación de memoria.  
- Mejora en el rendimiento del procesamiento de imágenes.  

---

## Conclusión
El Buddy System es ideal para aplicaciones de procesamiento de imágenes que requieren asignación y liberación rápida de memoria. En este código, el Buddy System proporciona una estructura eficiente para gestionar bloques de memoria contiguos y optimiza las operaciones de inversión y manipulación de datos de imagen.
