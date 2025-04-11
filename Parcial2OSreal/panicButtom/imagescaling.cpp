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

BuddyAllocator buddySystem(1024 * 1024 * 100);

Mat buddyAllocate(int rows, int cols, int type) {
    size_t requiredSize = rows * cols * CV_ELEM_SIZE(type);

    void* memory = buddySystem.allocate(requiredSize);
    if (!memory) {
        cerr << "Error: No se pudo asignar memoria con el Buddy System" << endl;
        exit(1);
    }

    size_t step = cols * CV_ELEM_SIZE(type); // Tamaño en bytes por fila
    return Mat(rows, cols, type, memory, step);
}

void buddyDeallocate(Mat& mat) {
    if (mat.data) {
        buddySystem.deallocate(mat.data);
        mat.data = nullptr; // Evitar uso después de liberación
    }
}

// Función para escalar utilizando la memoria pre-asignada con Buddy
void scaleImageToBuddy(const Mat& src, Mat& dst, double scaleFactor) {
    int newRows = static_cast<int>(src.rows * scaleFactor);
    int newCols = static_cast<int>(src.cols * scaleFactor);
    
    // Asegurarse que dst tenga el tamaño correcto
    CV_Assert(dst.rows == newRows && dst.cols == newCols && dst.type() == src.type());

    for (int y = 0; y < newRows; ++y) {
        for (int x = 0; x < newCols; ++x) {
            // Coordenadas originales en la imagen fuente
            float srcX = x / scaleFactor;
            float srcY = y / scaleFactor;

            int x1 = static_cast<int>(srcX);
            int y1 = static_cast<int>(srcY);
            int x2 = min(x1 + 1, src.cols - 1);
            int y2 = min(y1 + 1, src.rows - 1);

            float dx = srcX - x1;
            float dy = srcY - y1;

            // Obtener los valores de los píxeles vecinos
            Vec3b p1 = src.at<Vec3b>(y1, x1);
            Vec3b p2 = src.at<Vec3b>(y1, x2);
            Vec3b p3 = src.at<Vec3b>(y2, x1);
            Vec3b p4 = src.at<Vec3b>(y2, x2);

            // Interpolación bilineal en cada canal (BGR)
            Vec3b interpolatedPixel;
            for (int c = 0; c < 3; ++c) {
                float interpolatedValue =
                    (1 - dx) * (1 - dy) * p1[c] +
                    dx * (1 - dy) * p2[c] +
                    (1 - dx) * dy * p3[c] +
                    dx * dy * p4[c];

                interpolatedPixel[c] = static_cast<uchar>(interpolatedValue);
            }

            dst.at<Vec3b>(y, x) = interpolatedPixel;
        }
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

    // Registrar memoria antes de la operación de escalado
    size_t memBefore = getMemoryUsage();
    cout << "Memoria antes de escalar: " << memBefore << " KB" << endl;
    
    auto startTime = chrono::high_resolution_clock::now();

    Mat scaledImage;
    if (useBuddySystem) {
        int newRows = cvRound(image.rows * scaleFactor);
        int newCols = cvRound(image.cols * scaleFactor);
    
        // Pre-asignar con Buddy
        scaledImage = buddyAllocate(newRows, newCols, image.type());
        
        // Usar la función personalizada de escalado con la memoria pre-asignada
        scaleImageToBuddy(image, scaledImage, scaleFactor);
    } else {
        scaledImage = processor.scaleImage(image, scaleFactor);
    }

    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = endTime - startTime;

    // Registrar memoria después de la operación de escalado
    size_t memAfter = getMemoryUsage();
    cout << "Memoria después de escalar: " << memAfter << " KB" << endl;
    cout << "Diferencia de memoria: " << (memAfter - memBefore) << " KB" << endl;

    imwrite(outputFile, scaledImage);

    // Liberar memoria si se usó Buddy System
    if (useBuddySystem) {
        buddyDeallocate(scaledImage);
    }

    cout << "Tiempo de ejecución: " << elapsed.count() << " segundos" << endl;
    
    // Memoria final después de liberar
    size_t memFinal = getMemoryUsage();
    cout << "Memoria después de liberar: " << memFinal << " KB" << endl;
    cout << "Memoria total utilizada: " << (memFinal - memBefore) << " KB" << endl;

    return 0;
}