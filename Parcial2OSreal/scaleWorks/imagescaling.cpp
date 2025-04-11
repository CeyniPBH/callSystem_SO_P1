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

BuddyAllocator buddySystem(1024 * 1024 * 100); // 100 MB de memoria pre-asignada

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
    if (argc != 5) {
        cerr << "Uso: " << argv[0] << " <imagen_entrada> <imagen_salida> <factor_escala> <buddy_system (0/1)>" << endl;
        return 1;
    }

    string inputFile = argv[1];
    string outputFile = argv[2];
    double scaleFactor = atof(argv[3]);
    bool useBuddySystem = atoi(argv[4]);

    ImageProcessor processor;
    Mat image = processor.loadImage(inputFile);

    size_t memBefore = getMemoryUsage();
    cout << "Memoria antes de escalar: " << memBefore << " KB" << endl;

    auto startTime = chrono::high_resolution_clock::now();

    Mat scaledImage;
    if (useBuddySystem) {
        int newRows = max(static_cast<int>(image.rows * scaleFactor), 1);
        int newCols = max(static_cast<int>(image.cols * scaleFactor), 1);

        scaledImage = buddyAllocate(newRows, newCols, image.type());

        if (scaledImage.rows != newRows || scaledImage.cols != newCols || scaledImage.type() != image.type()) {
            cerr << "Error: La imagen pre-asignada no coincide con las dimensiones esperadas." << endl;
            return 1;
        }

        processor.scaleImageToBuddy(image, scaledImage, scaleFactor); // Llamada corregida
    } else {
        scaledImage = processor.scaleImage(image, scaleFactor);
    }

    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = endTime - startTime;

    size_t memAfter = getMemoryUsage();
    cout << "Memoria después de escalar: " << memAfter << " KB" << endl;
    cout << "Diferencia de memoria: " << (memAfter - memBefore) << " KB" << endl;

    if (!imwrite(outputFile, scaledImage)) {
        cerr << "Error al guardar la imagen: " << outputFile << endl;
        return 1;
    }

    if (useBuddySystem) {
        buddyDeallocate(scaledImage);
    }

    size_t memFinal = getMemoryUsage();
    cout << "Tiempo de ejecución: " << elapsed.count() << " segundos" << endl;
    cout << "Memoria después de liberar: " << memFinal << " KB" << endl;
    cout << "Memoria total utilizada: " << (memFinal - memBefore) << " KB" << endl;

    return 0;
}