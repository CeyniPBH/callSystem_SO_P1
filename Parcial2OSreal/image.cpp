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