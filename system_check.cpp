#include <iostream>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <filesystem>
#include <iomanip>
#ifndef system_check_h
#define system_check_h


/*
* @file system_check.cpp
* @brief Checks system properties such as disk space, RAM, and WSL status.
*
* This program provides functions to check if the system is running in WSL,
* check available disk space, available RAM, save images in various formats,
* and calculate disk write speed for different image formats.

* It also provides a function to choose the best image format based on disk write speed
* and available disk space.
*/


bool isWSL() {
    /**
     * @brief Checks if the program is running in Windows Subsystem for Linux (WSL).
     * 
     * This function reads the /proc/version file to determine if the system is WSL.
     * It checks for the presence of "Microsoft" or "WSL" in the version string.
     * 
     * @return true if running in WSL, false otherwise.
     */
    std::ifstream f("/proc/version");
    std::string line;
    std::getline(f, line);
    return line.find("Microsoft") != std::string::npos || line.find("WSL") != std::string::npos;
}

long long checkDiskSpace(const char* path){
    /**
     * @brief Checks the available disk space in the specified path.
     * 
     * This function retrieves the file system statistics for the given path
     * and prints the total and free disk space in megabytes (MB).
     * 
     * @param path The path to check the disk space.
     * @return unsigned long long The free disk space in bytes.
     */
    struct statvfs stat;
    statvfs(path, &stat);
    unsigned long long totalSpace = stat.f_bsize * stat.f_blocks; // Total space in bytes
    unsigned long long freeSpace = stat.f_bsize * stat.f_bavail; // Free space in bytes
    std::cout << "Total disk space: " << totalSpace / (1024 * 1024) << " MB" << std::endl;
    std::cout << "Free disk space: " << freeSpace / (1024 * 1024) << " MB" << std::endl;
    return freeSpace; // Return free space in bytes
}

long long checkAvailableRam(){
    /**
     * @brief Checks the available RAM in the system.
     * 
     * This function retrieves the total and free physical memory in the system
     * and prints it in megabytes (MB).
     * @param None
     * @return long long The free physical memory in bytes.
     */
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long long totalPhysMem = memInfo.totalram;
    totalPhysMem *= memInfo.mem_unit;
    long long freePhysMem = memInfo.freeram;
    freePhysMem *= memInfo.mem_unit;

    std::cout << "Total RAM: " << totalPhysMem / (1024 * 1024) << " MB" << std::endl;
    std::cout << "Free RAM: " << freePhysMem / (1024 * 1024) << " MB" << std::endl;
    return freePhysMem; // Return free RAM in bytes
}

float saveMatToRaw(const cv::Mat& image, const std::string& filename) {
    /**
     * @brief Saves a cv::Mat image to a raw binary file.
     * 
     * This function opens a binary file for writing and writes the image data to it.
     * The image is saved in raw format, which is not compressed and contains pixel data only.
     * 
     * @param image The cv::Mat image to be saved.
     * @param filename The name of the file where the image will be saved.
     * @return float The write speed in MB/s.
     */
    auto start = std::chrono::steady_clock::now();
    // Open binary file for writing
    std::ofstream file(filename, std::ios::out | std::ios::binary);

    // write the image data to the file
    file.write(reinterpret_cast<const char*>(image.data), image.total() * image.elemSize());
    auto end = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    file.close();
    return static_cast<float>(elapsed_time); // MB/s
}

long long getFileSize(const std::string& filePath) {
    /**
     * @brief Gets the size of a file in bytes.
     * 
     * This function opens a file in binary mode and retrieves its size using tellg().
     * 
     * @param filePath The path to the file whose size is to be determined.
     * @return long long The size of the file in bytes.
     */
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    return file.tellg();
}
struct imageInfo {
    /**
     * @brief Structure to hold information about an image format.
     * 
     * This structure contains the format of the image, the time taken to write it,
     * and the size of the file in bytes.
     */
    std::string format;
    float writing_time;
    long long file_size;
};
std::vector<imageInfo> calculateDiskWriteSpeed(int height, int width) {
    /**
     * @brief Calculates the disk write speed for various image formats.
     * 
     * This function creates a test image and saves it in different formats,
     * measuring the time taken to write each format and the resulting file size.
     * 
     * @param height The height of the test image.
     * @param width The width of the test image.
     * @return std::vector<imageInfo> A vector containing information about each image format,
     */

    std::cout << "Calculating disk write speed for various image formats..." << std::endl;

    std::filesystem::create_directory("./test");
    
    cv::Mat test_image(height, width, CV_8UC3);
    cv::randu(test_image, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));

    std::vector<imageInfo> image_info;
    image_info.reserve(13); // Reserve space for 12 formats

    std::vector<std::string> formats = {
        ".bmp",".dib",".jpeg",".jpg",".jpe",".png",
        ".ppm",".sr",".ras",".tiff",".tif",".hdr", ".raw"
    };

    for (int i = 0; i < formats.size();i++){
        if (i<formats.size()-1) {
            auto start = std::chrono::steady_clock::now();
            cv::imwrite("./test/test_image" + formats[i], test_image);
            auto end = std::chrono::steady_clock::now();
            auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            image_info.push_back({formats[i], static_cast<float>(elapsed_time), getFileSize("./test/test_image" + formats[i])});
        } else {
            // For raw format, we handle it separately
            image_info.push_back({formats[i], saveMatToRaw(test_image, "./test/test_image.raw"), getFileSize("./test/test_image" + formats[i])});
        }
    }
    
    //sort the vector by writing time
    std::sort(image_info.begin(), image_info.end(), [](const imageInfo& a, const imageInfo& b) {
        return a.writing_time < b.writing_time;
    });
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Format | Writing time (ms) | File Size (MB)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    for (auto element : image_info){
        std::cout << std::fixed << std::setprecision(2) << element.format << " | " 
        <<  element.writing_time/1000 << " milliseconds | " << static_cast<float>(element.file_size)/(1024*1024) << " MB " << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;

    return image_info;
}
void chooseFormat(int duration, int minimunImages){
    /**
     * @brief Chooses the best image format based on disk write speed and available disk space.
     * 
     * This function calculates the disk write speed for various image formats, checks if the
     * available disk space is sufficient for saving a specified number of images and if it is posible 
     * to save enough images per second.
     * 
     * @param duration The duration in seconds for which the images will be saved.
     * @param minimunImages The minimum number of images to be saved per second.
     */
    std::vector<imageInfo> image_info = calculateDiskWriteSpeed(1080, 1920);
    long long freeSpace = isWSL() ? checkDiskSpace("/mnt/c") : checkDiskSpace("./");
    long long freeRam = checkAvailableRam();
    for (int i = 0; i < image_info.size(); i++){
        if (minimunImages*duration*image_info[i].file_size < freeSpace
            && 1000000/image_info[i].writing_time > minimunImages) { // Check if the format is suitable
            std::cout << "You can use the format: " << image_info[i].format << std::endl
            << "Your maximum queue size is: " << freeRam/image_info[i].file_size << std::endl;
            
            return; // Exit after finding the first suitable format
        }
    }
    std::cout << "No suitable format found for the given duration and minimum images per second." << std::endl;
}
#endif // system_check_h

int main(int argc, char **argv) {
    chooseFormat(60, 50); // Example usage with 60 seconds duration and 50 images per second
    return 0;
}