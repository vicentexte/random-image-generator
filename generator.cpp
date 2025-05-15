#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <chrono>
#include <pthread.h>
#include <list>
#include <unistd.h> 
#define NUM_THREADS 10
#define MAX_FPS 10

// Global variables and mutex
int counter = 0;
int frames = 0;
std::list<cv::Mat> imagesList;
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
 * Creates a primary while loop that sets the timer and frames count to 0 then a secondary while loop creates the images,
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
  auto start = std::chrono::system_clock::now();

  //set frames to 0
  pthread_mutex_lock(&framesMutex);
  frames = 0;
  pthread_mutex_unlock(&framesMutex);

  std::cout << "Generating a " << width << "x" << height << " random image..." << std::endl;

  //create images in loop and print fps 
  while (1)
  {
    pthread_mutex_lock(&framesMutex);
    frames = 0;                          //set frame to 0
    pthread_mutex_unlock(&framesMutex);
    start = std::chrono::system_clock::now();
    while (1) {
      pthread_mutex_lock(&framesMutex);
      int temp = frames;                          //+1 frame
      pthread_mutex_unlock(&framesMutex);
      while (temp >= MAX_FPS - 1)
      {
        //duration check(1s)
        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);
        if (duration.count() >= 1000){
          break;
        }
        usleep(1000);
      }
      
      cv::Mat myRandomImage = generateRandomImage(width, height); //create Image
      pthread_mutex_lock(&listMutex);
      imagesList.push_back(myRandomImage);  //add image to list
      pthread_mutex_unlock(&listMutex);

      pthread_mutex_lock(&framesMutex);
      frames += 1;                          //+1 frame
      pthread_mutex_unlock(&framesMutex);

      //duration check(1s)
      auto end = std::chrono::system_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start); 
      if (duration.count() >= 1000) {
        //std::cout << duration.count() << std::endl << std::flush;
        break;
      }

    }
    std::cout << "FPS: " << frames << std::endl;
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
    
    pthread_mutex_lock(&listMutex);
    if (imagesList.size() > 0) {
    cv::Mat image = imagesList.front().clone();
    imagesList.pop_front();  //delete image from list
    pthread_mutex_unlock(&listMutex);
    pthread_mutex_lock(&counterMutex);
    cv::imwrite("./images/" + std::to_string(counter) + ".png",image);//write image   
    counter++; // counter +1
    pthread_mutex_unlock(&counterMutex);
    } else {usleep(1000);}
  }
  return NULL;
}


int main(int argc, char **argv) {
  imageProperties properties = {100,100};

  pthread_t threads[NUM_THREADS];

  pthread_create(&threads[0],NULL,generateLoop,&properties);  
  pthread_create(&threads[1],NULL,saveImage,NULL);
  pthread_create(&threads[2],NULL,saveImage,NULL);
  pthread_join(threads[0],NULL);
  pthread_join(threads[1],NULL);
  pthread_join(threads[2],NULL);
}
