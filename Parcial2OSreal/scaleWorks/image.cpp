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