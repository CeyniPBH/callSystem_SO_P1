#ifndef IMAGE_H
#define IMAGE_H

#include <opencv2/opencv.hpp>
#include <string>

class ImageProcessor {
public:
    cv::Mat loadImage(const std::string& filepath);
    cv::Mat scaleImage(const cv::Mat& image, double scaleFactor);
};

#endif // IMAGE_H
