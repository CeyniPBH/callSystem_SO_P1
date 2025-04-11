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