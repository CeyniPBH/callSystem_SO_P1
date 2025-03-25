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

    if (!stbi_write_png(nombreArchivo.c_str(), ancho, alto, canales, buffer, ancho * canales)) {
        cerr << "Error: No se pudo guardar la imagen en '" << nombreArchivo << "'.\n";
        delete[] buffer;
        exit(1);
    }

    delete[] buffer;
    cout << "[INFO] Imagen guardada correctamente en '" << nombreArchivo << "'.\n";
}

void Imagen::invertirColores() {
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            for (int c = 0; c < canales; c++) {
                pixeles[y][x][c] = 255 - pixeles[y][x][c];
            }
        }
    }}


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