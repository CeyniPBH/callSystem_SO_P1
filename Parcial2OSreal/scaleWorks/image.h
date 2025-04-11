#ifndef IMAGE_H
#define IMAGE_H

#include <opencv2/opencv.hpp>
#include <string>

class ImageProcessor {
    public:
        cv::Mat loadImage(const std::string& filepath);
        cv::Mat scaleImage(const cv::Mat& image, double scaleFactor);
        void scaleImageToBuddy(const cv::Mat& src, cv::Mat& dst, double scaleFactor); // Nombre correcto
    };
#endif // IMAGE_H