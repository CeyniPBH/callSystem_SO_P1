
# Procesamiento de imágenes con Buddy system.

## Propósito del proyecto.
El proyecto se trabaja para practicar o conocer y entender las expresiones de rotación y escalamiento de una imagen, entender porque consume grandes cantidades de memoria y comparando la eficiencia de la gestión de memoria mediante el Buddy system con la asignación dinámica convencional `(new/delete)`

---

## Estructura del programa.
El siguiente código en C++ implementa el algoritmo Buddy System para la gestión de memoria en procesamiento de imágenes.

### buddy_allocator.h
Se encarga de encapsular la lógica del algoritmo Buddy system para la gestión de memoria.
```cpp
#ifndef BUDDYALLOCATOR_H
#define BUDDYALLOCATOR_H

#include <vector>
#include <map>
#include <cstddef>
#include <iostream>

class BuddyAllocator {
private:
    size_t totalSize;
    std::vector<char> memoryBlocks;
    std::map<size_t, size_t> allocatedBlocks; // Bloques asignados: índice -> tamaño
    std::map<size_t, size_t> freeBlocks;      // Bloques libres: índice -> tamaño

    size_t nextPowerOfTwo(size_t n);
    void mergeBuddies(size_t index, size_t size);

public:
    BuddyAllocator(size_t size);
    void* allocate(size_t size);
    void deallocate(void* ptr);
    
    // Funciones de monitoreo
    size_t getTotalMemory() const;
    size_t getUsedMemory() const;
    size_t getFreeMemory() const;
    void printMemoryStatus() const;
};

#endif // BUDDYALLOCATOR_H

```

### buddy_allocator.cpp
Se encarga de implementar los métodos declarados en `buddy_allocator.h`, es decir, definir como funciona realmente el buddy system.
```cpp
#include "BuddyAllocator.h"

size_t BuddyAllocator::nextPowerOfTwo(size_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

BuddyAllocator::BuddyAllocator(size_t size) 
    : totalSize(nextPowerOfTwo(size)), memoryBlocks(totalSize, 0) {}

void* BuddyAllocator::allocate(size_t size) {
    if (size == 0) return nullptr;
    
    size = nextPowerOfTwo(size);
    if (size > totalSize) return nullptr;

    // Buscar el primer bloque libre del tamaño adecuado
    for (size_t i = 0; i <= totalSize - size; i += size) {
        if (memoryBlocks[i] == 0) {
            bool blockFree = true;
            // Verificar que todo el bloque esté libre
            for (size_t j = i; j < i + size; ++j) {
                if (memoryBlocks[j] != 0) {
                    blockFree = false;
                    break;
                }
            }
            
            if (blockFree) {
                // Marcar como ocupado
                for (size_t j = i; j < i + size; ++j) {
                    memoryBlocks[j] = 1;
                }
                allocatedBlocks[i] = size;
                return &memoryBlocks[i];
            }
        }
    }
    return nullptr;
}

void BuddyAllocator::deallocate(void* ptr) {
    if (!ptr) return;

    size_t index = static_cast<char*>(ptr) - &memoryBlocks[0];
    if (index >= totalSize || !allocatedBlocks.count(index)) return;

    size_t size = allocatedBlocks[index];
    // Marcar como libre
    for (size_t i = index; i < index + size; ++i) {
        memoryBlocks[i] = 0;
    }
    allocatedBlocks.erase(index);
    
    // Intentar fusionar con buddies
    mergeBuddies(index, size);
}

void BuddyAllocator::mergeBuddies(size_t index, size_t size) {
    size_t buddyIndex = index ^ size;
    
    while (size < totalSize) {
        // Verificar si el buddy está libre y es del mismo tamaño
        if (buddyIndex < totalSize && memoryBlocks[buddyIndex] == 0) {
            bool buddyCompletelyFree = true;
            for (size_t i = buddyIndex; i < buddyIndex + size; ++i) {
                if (memoryBlocks[i] != 0) {
                    buddyCompletelyFree = false;
                    break;
                }
            }
            
            if (buddyCompletelyFree) {
                // Fusionar bloques
                size *= 2;
                index = std::min(index, buddyIndex);
                buddyIndex = index ^ size;
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

// Funciones de monitoreo
size_t BuddyAllocator::getTotalMemory() const {
    return totalSize;
}

size_t BuddyAllocator::getUsedMemory() const {
    size_t used = 0;
    for (const auto& block : allocatedBlocks) {
        used += block.second;
    }
    return used;
}

size_t BuddyAllocator::getFreeMemory() const {
    return totalSize - getUsedMemory();
}

void BuddyAllocator::printMemoryStatus() const {
    std::cout << "Estado de memoria:\n";
    std::cout << "Total: " << getTotalMemory() << " bytes\n";
    std::cout << "En uso: " << getUsedMemory() << " bytes\n";
    std::cout << "Libres: " << getFreeMemory() << " bytes\n";
    
    std::cout << "Bloques asignados:\n";
    for (const auto& block : allocatedBlocks) {
        std::cout << " - Dirección: " << block.first 
                  << ", Tamaño: " << block.second << " bytes\n";
    }
}
```
---

### imagen.h
Se encarga de encapsular la lógica para cargar, manipular y guardar las imágenes.
```cpp
// image.h
#ifndef IMAGE_H
#define IMAGE_H

#include <opencv2/opencv.hpp>
#include <string>
#include <cmath>

class ImageProcessor {
public:
    cv::Mat loadImage(const std::string& filepath);
    cv::Mat scaleImage(const cv::Mat& image, double scaleFactor);
    void scaleImageToBuddy(const cv::Mat& src, cv::Mat& dst, double scaleFactor);
    cv::Mat rotateImage(const cv::Mat& image, double angle);
    void rotateImageToBuddy(const cv::Mat& src, cv::Mat& dst, double angle);
    cv::Vec3b bilinearInterpolate(const cv::Mat& img, float x, float y);
    
private:
    
};

#endif // IMAGE_H
```

---

### imagen.cpp
Implementa la lógica propuesta en `imagen.h`, contiene la lógica para cargar y manipular los pixeles de las imagenes (rotar, escalar e invertir colores) y guardarla.
```cpp
#include "image.h"
#include <iostream>

cv::Mat ImageProcessor::loadImage(const std::string& filepath) {
    cv::Mat image = cv::imread(filepath);
    if (image.empty()) {
        std::cerr << "Error al cargar la imagen: " << filepath << std::endl;
        exit(1);
    }
    return image;
}

cv::Mat ImageProcessor::scaleImage(const cv::Mat& image, double scaleFactor) {
    int newRows = static_cast<int>(image.rows * scaleFactor);
    int newCols = static_cast<int>(image.cols * scaleFactor);
    cv::Mat scaledImage(newRows, newCols, image.type());

    for (int y = 0; y < newRows; ++y) {
        for (int x = 0; x < newCols; ++x) {
            float srcX = x / scaleFactor;
            float srcY = y / scaleFactor;

            int x1 = static_cast<int>(srcX);
            int y1 = static_cast<int>(srcY);
            int x2 = std::min(x1 + 1, image.cols - 1);
            int y2 = std::min(y1 + 1, image.rows - 1);

            float dx = srcX - x1;
            float dy = srcY - y1;

            cv::Vec3b p1 = image.at<cv::Vec3b>(y1, x1);
            cv::Vec3b p2 = image.at<cv::Vec3b>(y1, x2);
            cv::Vec3b p3 = image.at<cv::Vec3b>(y2, x1);
            cv::Vec3b p4 = image.at<cv::Vec3b>(y2, x2);

            cv::Vec3b interpolatedPixel;
            for (int c = 0; c < 3; ++c) {
                float interpolatedValue =
                    (1 - dx) * (1 - dy) * p1[c] +
                    dx * (1 - dy) * p2[c] +
                    (1 - dx) * dy * p3[c] +
                    dx * dy * p4[c];

                interpolatedPixel[c] = static_cast<uchar>(interpolatedValue);
            }

            scaledImage.at<cv::Vec3b>(y, x) = interpolatedPixel;
        }
    }

    return scaledImage;
}

void ImageProcessor::scaleImageToBuddy(const cv::Mat& src, cv::Mat& dst, double scaleFactor) {
    int newRows = static_cast<int>(src.rows * scaleFactor);
    int newCols = static_cast<int>(src.cols * scaleFactor);
    
    CV_Assert(dst.rows == newRows && dst.cols == newCols && dst.type() == src.type());

    for (int y = 0; y < newRows; ++y) {
        for (int x = 0; x < newCols; ++x) {
            float srcX = x / scaleFactor;
            float srcY = y / scaleFactor;

            int x1 = static_cast<int>(srcX);
            int y1 = static_cast<int>(srcY);
            int x2 = std::min(x1 + 1, src.cols - 1);
            int y2 = std::min(y1 + 1, src.rows - 1);

            float dx = srcX - x1;
            float dy = srcY - y1;

            cv::Vec3b p1 = src.at<cv::Vec3b>(y1, x1);
            cv::Vec3b p2 = src.at<cv::Vec3b>(y1, x2);
            cv::Vec3b p3 = src.at<cv::Vec3b>(y2, x1);
            cv::Vec3b p4 = src.at<cv::Vec3b>(y2, x2);

            cv::Vec3b interpolatedPixel;
            for (int c = 0; c < 3; ++c) {
                float interpolatedValue =
                    (1 - dx) * (1 - dy) * p1[c] +
                    dx * (1 - dy) * p2[c] +
                    (1 - dx) * dy * p3[c] +
                    dx * dy * p4[c];

                interpolatedPixel[c] = static_cast<uchar>(interpolatedValue);
            }

            dst.at<cv::Vec3b>(y, x) = interpolatedPixel;
        }
    }
}

cv::Vec3b ImageProcessor::bilinearInterpolate(const cv::Mat& img, float x, float y) {
    int x1 = static_cast<int>(x);
    int y1 = static_cast<int>(y);
    int x2 = std::min(x1 + 1, img.cols - 1);
    int y2 = std::min(y1 + 1, img.rows - 1);

    float dx = x - x1;
    float dy = y - y1;

    cv::Vec3b p1 = img.at<cv::Vec3b>(y1, x1);
    cv::Vec3b p2 = img.at<cv::Vec3b>(y1, x2);
    cv::Vec3b p3 = img.at<cv::Vec3b>(y2, x1);
    cv::Vec3b p4 = img.at<cv::Vec3b>(y2, x2);

    cv::Vec3b interpolatedPixel;
    for (int c = 0; c < 3; ++c) {
        float interpolatedValue = 
            (1 - dx) * (1 - dy) * p1[c] +
            dx * (1 - dy) * p2[c] +
            (1 - dx) * dy * p3[c] +
            dx * dy * p4[c];

        interpolatedPixel[c] = static_cast<uchar>(interpolatedValue);
    }

    return interpolatedPixel;
}

cv::Mat ImageProcessor::rotateImage(const cv::Mat& image, double angle) {
    // Convertir ángulo a radianes
    double radians = angle * M_PI / 180.0;
    double cos_theta = cos(radians);
    double sin_theta = sin(radians);

    // Calcular dimensiones de la nueva imagen
    double new_width = abs(image.cols * cos_theta) + abs(image.rows * sin_theta);
    double new_height = abs(image.cols * sin_theta) + abs(image.rows * cos_theta);

    // Crear imagen de destino
    cv::Mat rotatedImage(static_cast<int>(new_height), static_cast<int>(new_width), image.type());

    // Centro de la imagen original y nueva
    double original_center_x = image.cols / 2.0;
    double original_center_y = image.rows / 2.0;
    double new_center_x = new_width / 2.0;
    double new_center_y = new_height / 2.0;

    for (int y = 0; y < rotatedImage.rows; ++y) {
        for (int x = 0; x < rotatedImage.cols; ++x) {
            // Convertir coordenadas al sistema centrado
            double x_offset = x - new_center_x;
            double y_offset = y - new_center_y;

            // Rotación inversa (de destino a origen)
            double original_x = x_offset * cos_theta + y_offset * sin_theta + original_center_x;
            double original_y = -x_offset * sin_theta + y_offset * cos_theta + original_center_y;

            // Si el punto está dentro de la imagen original
            if (original_x >= 0 && original_x < image.cols && original_y >= 0 && original_y < image.rows) {
                rotatedImage.at<cv::Vec3b>(y, x) = bilinearInterpolate(image, original_x, original_y);
            } else {
                // Poner negro si está fuera de los límites
                rotatedImage.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
            }
        }
    }

    return rotatedImage;
}

void ImageProcessor::rotateImageToBuddy(const cv::Mat& src, cv::Mat& dst, double angle) {
    // Convertir ángulo a radianes
    double radians = angle * M_PI / 180.0;
    double cos_theta = cos(radians);
    double sin_theta = sin(radians);

    // Centro de la imagen original y nueva
    double original_center_x = src.cols / 2.0;
    double original_center_y = src.rows / 2.0;
    double new_center_x = dst.cols / 2.0;
    double new_center_y = dst.rows / 2.0;

    for (int y = 0; y < dst.rows; ++y) {
        for (int x = 0; x < dst.cols; ++x) {
            // Convertir coordenadas al sistema centrado
            double x_offset = x - new_center_x;
            double y_offset = y - new_center_y;

            // Rotación inversa (de destino a origen)
            double original_x = x_offset * cos_theta + y_offset * sin_theta + original_center_x;
            double original_y = -x_offset * sin_theta + y_offset * cos_theta + original_center_y;

            // Si el punto está dentro de la imagen original
            if (original_x >= 0 && original_x < src.cols && original_y >= 0 && original_y < src.rows) {
                dst.at<cv::Vec3b>(y, x) = bilinearInterpolate(src, original_x, original_y);
            } else {
                // Poner negro si está fuera de los límites
                dst.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
            }
        }
    }
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
## Video
Link del video explicativo del proyecto: 
https://youtu.be/3p0XQpRNwT0

---
## Conclusión
El Buddy System es ideal para aplicaciones de procesamiento de imágenes que requieren asignación y liberación rápida de memoria. Este código proprciona una base funcional para el procesamiento de imágenes, demonstrando la rotación y el escalamiento con la `interpolación bilineal` y la organización de datos de imagenes en una matriz tridimensional. Se buscó simplificar el pograma, pero sobre todo trabajar la practicidad de cada funcionalidad.
