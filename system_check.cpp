#include <iostream>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <iomanip>
#include <filesystem>

void checkDiskSpace(const char* path){
    struct statvfs stat;
    statvfs(path, &stat);
}

void checkAvailableRam(){
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long long totalPhysMem = memInfo.totalram;
    totalPhysMem *= memInfo.mem_unit;
    long long freePhysMem = memInfo.freeram;
    freePhysMem *= memInfo.mem_unit;

    std::cout << "Total RAM: " << totalPhysMem / (1024 * 1024) << " MB" << std::endl;
    std::cout << "Free RAM: " << freePhysMem / (1024 * 1024) << " MB" << std::endl;
}

void calculateDiskWriteSpeed(int height, int width) {
    std::filesystem::create_directory("./test");
    
    cv::Mat test_image(height, width, CV_8UC3);
    cv::randu(test_image, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));

    std::vector<std::string> formats = {
        ".bmp",".dib",".jpeg",".jpg",".jpe",".png",
        ".ppm",".sr",".ras",".tiff",".tif",".hdr"
    };
    std::vector<float> speeds(formats.size(), 0.0f);
    for (int i = 0; i < formats.size();i++){
        auto start = std::chrono::steady_clock::now();
        cv::imwrite("./test/test_image" + formats[i], test_image);
        auto end = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        speeds[i] = static_cast<float>(test_image.total() * test_image.elemSize() * 1000000) / (elapsed_seconds * 1024 * 1024); // MB/s
    }

    for (int i = 0; i < formats.size();i++){
        std::cout << std::fixed << std::setprecision(2) << "Write speed for " << formats[i] << ": " << speeds[i] << " MB/s" << std::endl;
    }
}

int main() {
    // Calculate disk write speed
    calculateDiskWriteSpeed(1080, 1920);

    return 0;
}