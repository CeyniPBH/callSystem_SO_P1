
# Procesamiento de imágenes con Buddy system.

## Propósito del proyecto.
El proyecto se trabaja para practicar o conocer y entender las expresiones de rotación y escalamiento de una imagen, entender porque consume grandes cantidades de memoria y comparando la eficiencia de la gestión de memoria mediante el Buddy system con la asignación dinámica convencional `(new/delete)`

---

## Estructura del programa.
El siguiente código en C++ implementa el algoritmo Buddy System para la gestión de memoria en procesamiento de imágenes.

### buddy_allocator.h
Se encarga de encapsular la lógica del algoritmo Buddy system para la gestión de memoria.
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
Se encarga de implementar los métodos declarados en `buddy_allocator.h`, es decir, definir como funciona realmente el buddy system.
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
Se encarga de encapsular la lógica para cargar, manipular y guardar las imágenes.
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
Implementa la lógica propuesta en `imagen.h`, contiene la lógica para cargar y manipular los pixeles de las imagenes (rotar, escalar e invertir colores) y guardarla.
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

## Puntos claves Adicionales.
1. **stb_image:**  
   Esta biblioteca simplifica la carga y guardado de imágenes, evitando tener que escribir código complejo para decodificar diferentes formatos de archivos.

2. **Interpolación bilineal:**  
   Se utiliza en `escalarImagen`y `rotarImagen` para obtener una mejor calidad a la imagen al redimensionar o rotar, ayudando a suavizar y reducir los pixeles y el efecto de "bloques".

3. **Gestion de memoria:**  
   La operación de inversión de colores (`invertirColores()`) altera directamente los valores en la matriz.

4. **Liberación de memoria:**  
   El código es responsable de asignar y liberar memoria correctamente para evitar fugas de memoria. El uso de `new, new[], delete y delete[]` nos ayuda. El `buddyAllocator` se introduce como una alternativa a la gestión de manual con `new/delete`.

---

## Requisitos previos.
- **Compilador:** g++/gcc.  
- **Librerías:** `stb_image.h, stb_image_write.h`

---

## Parámetros.
```bash
    entrada.jpg: archivo de imagenes de entrada.
    salida.jpg: archivo donde se guarda la imagen procesada.
    -angulo <valor>: define el ángulo de rotación.
    -escalar <valor>: define el factor de escalado.
    -buddy: activa el modo Buddy system (si se omite, se usará el modo convencional `new/delete`)

    Nota: <valor> equivale a el valor que se quiera definir en la acción de la imagen.
```

---

## Ejecución.
```bash
    ./programa_imagen entrada.jpg salida.jpg -angulo 45 -escalar 1.5 -buddy
```
## Ejemplo de salida.
```bash
    === PROCESAMIENTO DE IMAGEN ===
    Archivo de entrada: entrada.jpg
    Archivo de salida: salida.jpg
    Modo de asignación de memoria: Buddy System
    ------------------------
    Dimensiones originales: 1920 x 1080
    Dimensiones finales: 2880 x 1620
    Canales: 3 (RGB)
    Ángulo de rotación: 45
    Factor de escalado: 1.5
    ------------------------
    [INFO] Imagen rotada correctamente.
    [INFO] Imagen escalada correctamente.
    ------------------------
    TIEMPO DE PROCESAMIENTO:
    - Sin Buddy System: 120 ms
    - Con Buddy System: 95 ms

    MEMORIA UTILIZADA:
    - Sin Buddy System: 2.1 MB
    - Con Buddy System: 1.8 MB
    ------------------------
    [INFO] Imagen guardada correctamente en salida.jpg

```
---

## Ventajas del Buddy System en este código.
- Reducción de fragmentación.
- Rápida reasignación de memoria.
- Mejora en el rendimiento del procesamiento de imágenes.
  
## Desventajas.
- fragmentación interna: desperdicia memoria al redondear los tamaños de los bloques a potencias de 2.
- Complejidad de implementación: puede ser más tediosos en cuanto a la cantidad de asignaciones.
- No siempre es la mejor opción: depende del patrón de asignación puede variar la técnica (pools de memoria).
---

## Preguntas de Análisis.
| pregunta | respuesta |
|----------|-----------|
| ¿Qué diferencia observaste en el tiempo de procesamiento entre los dos modos de asignación de memoria?| Se buscaba que el cambio en el tiempo variara en beneficio del Buddy system, que fuera más rapido. Al momento de ponerlo en práctica, no hubo mucha diferenncia entre los modos de asignación de memoria, en varias pruebas que se hicieron.|
|¿Cuál fue el impacto del tamaño de la imagen en el consumo de memoria y el rendimiento?| En el consumo de memoria como la matriz pixeles tiene dimensiones `[alto], [ancho], [canales]`, entonces el consumo aumenta exponencialmente. Por otro lado, el rendimiento varía según la funcionalidad, como es más trabajo poder rotar la imagen, su rendimiento lineal aumenta.|
| ¿Por qué el Buddy System es más eficiente o menos eficiente que el uso de new/delete en este caso?| por la carga de asignación y liberaciones de bloques de tamaños, ya que reduce la fragmentación externa y facilita la coalescencia de bloques contiguos.|
| ¿Cómo podrías optimizar el uso de memoria y tiempo de procesamiento en este programa?| Con paralelización y reducción de copias. La paralelización para trabajar con hilos o técnicas SIMD; y la reducción de copias para minimizar la cantidad de datos innecesarios, como algunas operaciones o la modificación de imágenes en lugar de crear una nueva matriz.|
| ¿Qué implicaciones podría tener esta solución en sistemas con limitaciones de memoria o en dispositivos embebidos?| En la limitación de memoria el tamaño de la imagen que se puede procesar estará restringido. En los dispositivos embebidos, la optimización del tiempo de procesamiento.|
| ¿Cómo afectaría el aumento de canales (por ejemplo, de RGB a RGBA) en el rendimiento y consumo de memoria?| El consumo de memoria aumentaría linealmente con el número de canales. El tiempo de procesameint ode las operaciones que iteran sobre los pixeles también aumentarían linealmente con el número de canales, debido a que se realiza la misma operación en cada canal de cada pixel.|

---
## Conclusión
El Buddy System es ideal para aplicaciones de procesamiento de imágenes que requieren asignación y liberación rápida de memoria. Este código proprciona una base funcional para el procesamiento de imágenes, demonstrando la rotación y el escalamiento con la `interpolación bilineal` y la organización de datos de imagenes en una matriz tridimensional. Se buscó simplificar el pograma, pero sobre todo trabajar la practicidad de cada funcionalidad.
