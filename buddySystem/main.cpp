#include "imagen.h"
#include "buddy_allocator.h"
#include <iostream>
#include <chrono>
#include <cstring>

using namespace std;
using namespace std::chrono;

// Muestra cómo se usa el programa desde la línea de comandos
void mostrarUso() {
    cout << "Uso: ./main <archivo_entrada> <archivo_salida> <-buddy|-no-buddy>" << endl;
    cout << "  <archivo_entrada>   Archivo de imagen de entrada (PNG, BMP, JPG)" << endl;
    cout << "  <archivo_salida>    Archivo de salida para la imagen procesada" << endl;
    cout << "  <angulo>            Angulo de rotacion" <<endl;
    cout << "  <escala>            Factor de escala "  <<endl;
    cout << "  -buddy              Usa Buddy System para la asignación de memoria" << endl;
    cout << "  -no-buddy           Usa new/delete para la asignación de memoria" << endl;
}

// Muestra una lista de chequeo para verificar que los parámetros son correctos
void mostrarListaChequeo(const string &archivoEntrada, const string &archivoSalida, bool usarBuddy,const string &angulo, const string &escala) {
    cout << "\n=== PROCESAMIENTO DE IMAGEN ===" << endl;
    cout << "Archivo de entrada: " << archivoEntrada << endl;
    cout << "Archivo de salida: " << archivoSalida << endl;
    cout << "Angulo:           " << angulo <<endl;
    cout << "Escala:           " << escala <<endl;
    cout << "Modo de asignación: " << (usarBuddy ? "Buddy System" : "new/delete") << endl;
    cout << "------------------------" << endl;
}

int main(int argc, char* argv[]) {
    // Verificar número de argumentos
    if (argc != 5) {
        cerr << "Error: Número incorrecto de argumentos." << endl;
        mostrarUso();
        return 1;
    }

    // Parámetros de línea de comandos
    string archivoEntrada = argv[1];
    string archivoSalida = argv[2];
    float angulo = atof(argv[3]);
    float escala = 2;
    string modoAsignacion = argv[4];
    
    // Verifica si el modo de asignación es válido
    bool usarBuddy = false;
    if (modoAsignacion == "-buddy") {
        usarBuddy = true;
    } else if (modoAsignacion == "-no-buddy") {
        usarBuddy = false;
    } else {
        cerr << "Error: Opción de modo inválida." << endl;
        mostrarUso();
        return 1;
    }

    // Mostrar lista de chequeo
    mostrarListaChequeo(archivoEntrada, archivoSalida, usarBuddy, argv[3], argv[4]);

    // Medir el tiempo de ejecución
    auto inicio = high_resolution_clock::now();

    if (usarBuddy) {
        cout << "\n[INFO] Usando Buddy System para la asignación de memoria." << endl;

        // Crear el allocador Buddy System de 32 MB
        BuddyAllocator allocador(32 * 1024 * 1024);

        // Cargar imagen usando Buddy System
        Imagen img(archivoEntrada, &allocador);

        // Mostrar información de la imagen
        img.mostrarInfo();

        // Invertir colores
        img.rotarImagen(angulo);
        
        

        // Guardar imagen procesada
        img.guardarImagen(archivoSalida);
    } else {
        cout << "\n[INFO] Usando new/delete para la asignación de memoria." << endl;

        // Cargar imagen usando new/delete
        Imagen img(archivoEntrada);

        // Mostrar información de la imagen
        img.mostrarInfo();

        
        img.rotarImagen(angulo);

        // Guardar imagen procesada
        img.guardarImagen(archivoSalida);
    }

    // Medir tiempo de finalización
    auto fin = high_resolution_clock::now();
    auto duracion = duration_cast<milliseconds>(fin - inicio).count();

    cout << "\nTiempo total de procesamiento: " << duracion << " ms" << endl;

    cout << "\n[INFO] Proceso completado con éxito." << endl;

    return 0;
}
