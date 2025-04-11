#include "BuddyAllocator.h"
#include "Image.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <sys/resource.h>

using namespace std;
using namespace cv;

size_t getMemoryUsage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

BuddyAllocator buddySystem(1024 * 1024 * 32);//100 MB de memoria pre-asignada

Mat buddyAllocate(int rows, int cols, int type) {
    size_t requiredSize = rows * cols * CV_ELEM_SIZE(type);
    void* memory = buddySystem.allocate(requiredSize);
    if (!memory) {
        cerr << "Error: Buddy System no pudo asignar memoria." << endl;
        exit(1);
    }
    return Mat(rows, cols, type, memory);
}

void buddyDeallocate(Mat& mat) {
    if (mat.data) {
        buddySystem.deallocate(mat.data);
        mat.data = nullptr;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        cerr << "Uso: " << argv[0] << " <imagen_entrada> <imagen_salida> <-rotar/-escalar> <factor> <buddy_system (0/1)>" << endl;
        cerr << "Ejemplo para escalar: " << argv[0] << " input.jpg output.jpg -escalar 1.5 1" << endl;
        cerr << "Ejemplo para rotar: " << argv[0] << " input.jpg output.jpg -rotar 45 0" << endl;
        return 1;
    }

    string inputFile = argv[1];
    string outputFile = argv[2];
    string operation = argv[3];
    double factor = atof(argv[4]);
    bool useBuddySystem = atoi(argv[5]);

    ImageProcessor processor;
    Mat image = processor.loadImage(inputFile);

    cout << "\n=== Estado inicial ===" << endl;
    size_t memBefore = getMemoryUsage();
    cout << "Memoria del sistema antes: " << memBefore << " KB" << endl;
    if (useBuddySystem) {
        buddySystem.printMemoryStatus();
    }

    auto startTime = chrono::high_resolution_clock::now();

    Mat resultImage;
    if (operation == "-escalar") {
        if (useBuddySystem) {
            int newRows = max(static_cast<int>(image.rows * factor), 1);
            int newCols = max(static_cast<int>(image.cols * factor), 1);

            cout << "\n=== Antes de asignar ===" << endl;
            buddySystem.printMemoryStatus();

            resultImage = buddyAllocate(newRows, newCols, image.type());

            cout << "\n=== Después de asignar ===" << endl;
            buddySystem.printMemoryStatus();

            processor.scaleImageToBuddy(image, resultImage, factor);
        } else {
            resultImage = processor.scaleImage(image, factor);
        }
    } 
    else if (operation == "-rotar") {
        if (useBuddySystem) {
            double radians = factor * M_PI / 180.0;
            double cos_theta = cos(radians);
            double sin_theta = sin(radians);
            
            int newWidth = static_cast<int>(abs(image.cols * cos_theta) + abs(image.rows * sin_theta));
            int newHeight = static_cast<int>(abs(image.cols * sin_theta) + abs(image.rows * cos_theta));

            cout << "\n=== Antes de asignar ===" << endl;
            buddySystem.printMemoryStatus();

            resultImage = buddyAllocate(newHeight, newWidth, image.type());

            cout << "\n=== Después de asignar ===" << endl;
            buddySystem.printMemoryStatus();

            processor.rotateImageToBuddy(image, resultImage, factor);
        } else {
            resultImage = processor.rotateImage(image, factor);
        }
    } 
    else {
        cerr << "Operación no válida. Use -rotar o -escalar." << endl;
        return 1;
    }

    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = endTime - startTime;

    size_t memAfter = getMemoryUsage();
    cout << "\n=== Resultados ===" << endl;
    cout << "Memoria del sistema después: " << memAfter << " KB" << endl;
    cout << "Diferencia de memoria del sistema: " << (memAfter - memBefore) << " KB" << endl;

    if (useBuddySystem) {
        cout << "\n=== Uso de Buddy Allocator ===" << endl;
        cout << "Memoria usada en Buddy: " << buddySystem.getUsedMemory() / 1024 << " KB" << endl;
        cout << "Memoria libre en Buddy: " << buddySystem.getFreeMemory() / 1024 << " KB" << endl;
    }

    if (!imwrite(outputFile, resultImage)) {
        cerr << "Error al guardar la imagen: " << outputFile << endl;
        return 1;
    }

    if (useBuddySystem) {
        cout << "\n=== Antes de liberar ===" << endl;
        buddySystem.printMemoryStatus();

        buddyDeallocate(resultImage);

        cout << "\n=== Después de liberar ===" << endl;
        buddySystem.printMemoryStatus();
    }

    size_t memFinal = getMemoryUsage();
    cout << "\n=== Resumen final ===" << endl;
    cout << "Tiempo de ejecución: " << elapsed.count() << " segundos" << endl;
    cout << "Memoria del sistema al final: " << memFinal << " KB" << endl;
    cout << "Memoria total utilizada: " << (memFinal - memBefore) << " KB" << endl;

    return 0;
}