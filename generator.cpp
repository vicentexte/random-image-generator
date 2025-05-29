#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <chrono>
#include <pthread.h>
#include <list>
#include <unistd.h> 
#include <filesystem>
namespace fs = std::filesystem;

#define NUM_THREADS 8
#define MAX_FPS 50

// Global variables and mutex
int accumulatedImages = 0;
int previousFPS = 0;
int counter = 0;
int frames = 0;
int totalFrames = 0;
std::chrono::system_clock::time_point start;

//std::chrono::_V2::system_clock::time_point start;
/*std::chrono::_V2::system_clock::time_point end;*/
std::chrono::system_clock::time_point end;
std::chrono::seconds duration;
std::chrono::seconds globalDuration;
int inputDuration;
bool isTimerRunning = false;
std::list<cv::Mat> imagesList;
long long totalBytesWritten = 0;
pthread_mutex_t timeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t counterMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t framesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;


/**
 * @brief Generates a random color image of the specified dimensions.
 *
 * Creates an 8-bit, 3-channel (BGR format by default in OpenCV) image
 * and fills each pixel's channels with random values between 0 and 255.
 *
 * @param width The desired width of the image in pixels. Must be positive.
 * @param height The desired height of the image in pixels. Must be positive.
 * @return cv::Mat An OpenCV matrix representing the generated random image.
 * Returns an empty cv::Mat if width or height are not positive.
 */
cv::Mat generateRandomImage(int width, int height) {
  if (width <= 0 || height <= 0) {
    std::cerr << "Error: Image dimensions must be positive." << std::endl;
    return cv::Mat(); // Return an empty Mat to indicate failure
  }
  
  // Create an 8-bit 3-channel BGR image (Height x Width)
  cv::Mat randomImage(height, width, CV_8UC3);

  // Fill the image with random values (0-255 for each channel)
  cv::randu(randomImage, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));

  return randomImage;
}

/**
 * @brief Holds the properties of an image.
 * 
 * This structure defines the basic dimensions of an image, 
 * including its width and height in pixels.
 */
struct imageProperties
{
  int width;
  int height;
};

/**
 * @brief Generates randoms images and counts FPS.
 * 
 * Creates a primary while loop that sets the timer then a secondary while loop creates the images,
 * adds it to imagesList, increments frames counter and checks if timer is 1 second, if it is true, secondary loop breaks and prints FPS.
 * 
 * @param width The desired width of the image in pixels. Must be positive.
 * @param height The desired height of the image in pixels. Must be positive.
 * @return void*
 */
void* generateLoop(void* args) {
  //variables
  imageProperties* prop = static_cast<imageProperties*>(args);
  int width = prop -> width;
  int height = prop -> height;
  
  while (1){
    pthread_mutex_lock(&timeMutex);
    int temp1 = globalDuration.count();
    pthread_mutex_unlock(&timeMutex);
    if (temp1 >= inputDuration) {
      break;
    }
    
    pthread_mutex_lock(&timeMutex);
    if (isTimerRunning == false){
      start = std::chrono::system_clock::now();
      isTimerRunning = true;
    }
    pthread_mutex_unlock(&timeMutex);
    
    while (1){
      cv::Mat myRandomImage = generateRandomImage(width, height); //create Image

      pthread_mutex_lock(&listMutex);
      imagesList.push_back(myRandomImage); //add image to list
      pthread_mutex_unlock(&listMutex);

      pthread_mutex_lock(&framesMutex);
      frames += 1;                          //+1 frame
      pthread_mutex_unlock(&framesMutex);
      
      pthread_mutex_lock(&timeMutex);
      end = std::chrono::system_clock::now();
      duration = std::chrono::duration_cast<std::chrono::seconds> (end - start);
      float temp2 = duration.count();
      pthread_mutex_unlock(&timeMutex);

      if (temp2 >= 1) {
        pthread_mutex_lock(&timeMutex);
        globalDuration += duration;
        pthread_mutex_unlock(&timeMutex);
        break;
      }
    }
    pthread_mutex_lock(&framesMutex);
    pthread_mutex_lock(&timeMutex);
    if (isTimerRunning == true){
      accumulatedImages += frames;
      std::cout << "→ Time: " << globalDuration.count() << "s | "
            << "FPS: " << frames << " | "
            << "Acumulated frames: " << accumulatedImages << std::endl;
      totalFrames += frames;
      frames = 0;
      isTimerRunning = false;
    }
    pthread_mutex_unlock(&timeMutex);
    pthread_mutex_unlock(&framesMutex);
    
  }
  return NULL;
}

/**
 * @brief Saves images from imagesList.
 * 
 * While loop that writes a file named {$counter}.png wich includes the first image from imagesLists,
 * increments counter, and pop the image from the list.
 * 
 * @return void*
 */
void* saveImage(void* args) {
  while (true) {
    cv::Mat image;

    pthread_mutex_lock(&listMutex);
    if (imagesList.size() > 0) {
    image = imagesList.front().clone();
    imagesList.pop_front();  //delete image from list
    }
    pthread_mutex_unlock(&listMutex);

    if (!image.empty()){
      try {
        std::string filename = "./images/" + std::to_string(counter) + ".png";
        if (cv::imwrite(filename, image)) { // This line won't throw but we use try-catch for safety
          pthread_mutex_lock(&counterMutex);
          counter++;
          totalBytesWritten += fs::file_size(filename);
          pthread_mutex_unlock(&counterMutex);
        }
      } catch (const cv::Exception& ex) {
        std::cerr << "Failed to save image: " << ex.what() << std::endl;
      }
    }
    
    pthread_mutex_lock(&timeMutex);
    float temp = globalDuration.count();
    pthread_mutex_unlock(&timeMutex);
    if (temp >= inputDuration) {
    break;
    }
  }
  return NULL;
}


int main(int argc, char **argv) {
  imageProperties properties = {1920,1080};
  int threadsNumber;
  //set input time
  if (argc >= 2){
    try {

      if (argc >= 4){
        try
        {
          threadsNumber = std::stoi(argv[3]);
          std::cout <<  "Selected " << threadsNumber << " threads" << std::endl;
        }
        catch(const std::exception& e)
        {
          std::cerr << e.what() << '\n';
        }
      }
      // m minutes, s seconds, h hours
      inputDuration = std::stoi(argv[2]);
      switch (*argv[1])
      {
      case 's':
        std::cout << "Set " << inputDuration << " seconds." << std::endl;
        break;
      case 'm':
        std::cout << "Set " << inputDuration << " minutes." << std::endl;
        inputDuration *= 60;
        break;
      case 'h':
        std::cout << "Set " << inputDuration << " hours." << std::endl;
        inputDuration *= 3600;
        break;
      default:
        std::cout << "Set 5 seconds.\n";
        inputDuration = 5;
        break;
      }
    }
    catch(...){
      std::cerr << "Invalid input. Set 5 seconds.\n";
      inputDuration = 5;
    }
  }
  else{
    std::cout << "Selected 2 threads and set 5 seconds.\n";
    threadsNumber = 2;
    inputDuration = 5;
  }
  
  fs::create_directory("./images");
  
  pthread_t threads[threadsNumber];
  std::cout << "Generating a " << properties.width << "x" << properties.height << " random image..." << std::endl;
  pthread_create(&threads[0],nullptr,generateLoop,&properties);  
  for (int i = 1; i < threadsNumber; i++){
    pthread_create(&threads[i],nullptr,saveImage,nullptr); 
  }
  for (int i = 0; i < threadsNumber; i++){
    pthread_join(threads[i],nullptr);
  }

  std::cout << "\n--- SUMMARY ---\n";
  std::cout << "→ Total frames generated: " << accumulatedImages << "\n";
  std::cout << "→ Total time: " << inputDuration << " seconds\n";
  std::cout << "→ FPS estimated mean: "
          << (inputDuration > 0 ? accumulatedImages / inputDuration : 0) << "\n";

}