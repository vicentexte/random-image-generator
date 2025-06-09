#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <chrono>
#include <pthread.h>
#include <queue>
#include <unistd.h> 
#include <filesystem>
#include <fstream>
#include <cstring>
/**
 * @file generator.cpp
 * @brief Generates random images and saves them to disk using multiple threads.
 *
 * This program generates random images of specified dimensions, counts the frames per second (FPS),
 * and saves the images to a directory. It uses multithreading to handle image generation and saving concurrently.
 *
 * Usage: ./generator [time_unit] [duration] [threads_number]
 * - time_unit: 's' for seconds, 'm' for minutes, 'h' for hours
 * - duration: number of time units to run the program
 * - threads_number: number of threads to use for image generation and saving (default is 3)
 */

/*Global variables
*/
  const int MAX_QUEUE_SIZE = 500; // Maximum size of the imagesList queue

  int totalFrames = 0; // Total frames generated
  int counter = 0; // Counter for saved images
  int frames = 0; // Frames generated in the last second
  int lostFrames = 0; // Frames that were not saved due to timing issues
  bool isTimerRunning = false; // Timer status
  bool isTimelimitReached = false; // Timer limit status
  const char* imageFormat;// Image format for saving

  // Input parameters
  int inputDuration; // Duration for which the program will run, (default 5 seconds)
  int threadsNumber; // Number of threads to be used for image generation and saving

  // Time variables
  std::chrono::system_clock::time_point start; // Start time of the timer
  std::chrono::system_clock::time_point end; // End time of the timer
  std::chrono::seconds duration; // Duration of the last second of image generation
  std::chrono::seconds nowTime; // Current time

  // Image list to hold generated images
  std::queue<cv::Mat> imagesList;

/*Mutexes
*/
  pthread_mutex_t globalTimeMutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t timeMutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t framesMutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t counterMutex = PTHREAD_MUTEX_INITIALIZER;


struct imageProperties{
  /**
 * @brief Holds the properties of an image.
 * 
 * This structure defines the basic dimensions of an image, 
 * including its width and height in pixels.
 */
  int width;
  int height;
};

cv::Mat generateRandomImage(int width, int height) {
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

void* generateLoop(void* args) {
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

  imageProperties* prop = static_cast<imageProperties*>(args);
  int width = prop -> width;
  int height = prop -> height;
  
  while (1){
    // Check if the global duration has reached the input duration
    pthread_mutex_lock(&globalTimeMutex);
    bool limit_reached = isTimelimitReached;
    pthread_mutex_unlock(&globalTimeMutex);
    if (limit_reached) {
      break;
    }

    // Start the timer if it is not running
    pthread_mutex_lock(&timeMutex);
    if (isTimerRunning == false){
      start = std::chrono::system_clock::now();
      isTimerRunning = true;
    }
    pthread_mutex_unlock(&timeMutex);
    
    while (1){
      cv::Mat myRandomImage = generateRandomImage(width, height); //create Image

      pthread_mutex_lock(&listMutex);
      if (imagesList.size() < MAX_QUEUE_SIZE) { // Check if the queue is not full
        imagesList.push(myRandomImage); //add image to list
      } else {
        pthread_mutex_lock(&framesMutex);
        lostFrames += 1; //+1 lost frame
        pthread_mutex_unlock(&framesMutex);
      }
      pthread_mutex_unlock(&listMutex);

      pthread_mutex_lock(&framesMutex);
      frames += 1; //+1 frame
      pthread_mutex_unlock(&framesMutex);
      
      pthread_mutex_lock(&timeMutex);
      end = std::chrono::system_clock::now();
      duration = std::chrono::duration_cast<std::chrono::seconds> (end - start);
      float frameGen_duration = duration.count();
      pthread_mutex_unlock(&timeMutex);

      if (frameGen_duration >= 1) {
        pthread_mutex_lock(&timeMutex);
        nowTime += duration; //update time
        pthread_mutex_unlock(&timeMutex);
        usleep(100000); // Sleep for 100 ms to avoid busy waiting
        break;
      }
    }
    pthread_mutex_lock(&framesMutex);
    pthread_mutex_lock(&timeMutex);
    if (isTimerRunning == true){
      totalFrames += frames;
      std::cout << "→ Time: " << nowTime.count() << "s | "
            << "FPS: " << frames << " | "
            << "Acumulated frames: " << totalFrames << " | "
            << "Saved frames: " << counter << " | "
            << "Frames in queue: " << imagesList.size() << " | "
            << "Losted frames: " << lostFrames << std::endl;
      frames = 0;
      isTimerRunning = false;
    }
    pthread_mutex_unlock(&timeMutex);
    pthread_mutex_unlock(&framesMutex);
    
  }
  return NULL;
}

void* saveImage(void* args) {
  /**
 * @brief Saves images from imagesList.
 * 
 * While loop that writes a file named {$counter}.png wich includes the first image from imagesLists,
 * increments counter, and pop the image from the list.
 * 
 * @return void*
 */
  while (true) {
    cv::Mat image;

    pthread_mutex_lock(&listMutex);
    if (imagesList.size() > 0) {
    image = imagesList.front().clone();
    imagesList.pop();  //delete image from list
    }
    pthread_mutex_unlock(&listMutex);

    if (!image.empty()){
      try {
        pthread_mutex_lock(&counterMutex);
        int this_counter = counter; // Get the current counter value
        counter++;
        pthread_mutex_unlock(&counterMutex);
        std::string filename = "./images/" + std::to_string(this_counter) + imageFormat; // Use the specified image format
        if (!cv::imwrite(filename, image)) { // This line won't throw but we use try-catch for safety
          std::cerr << "Failed to save image: " << filename << std::endl;
        }
      } catch (const cv::Exception& ex) {
        std::cerr << "Failed to save image: " << ex.what() << std::endl;
      }
    } else {usleep(1000); // If no image is available, wait a bit before checking again
    }
    
    pthread_mutex_lock(&globalTimeMutex);
    bool limit_reached = isTimelimitReached;
    pthread_mutex_unlock(&globalTimeMutex);
    if (limit_reached) {
      break;
    }
    
  }
  return NULL;
}

void* saveImageRaw(void* args) {
  /**
 * @brief Saves images from imagesList in raw format.
 * 
 * While loop that writes a file named {$counter}.raw wich includes the first image from imagesLists,
 * increments counter, and pop the image from the list.
 * 
 * @return void*
 */
  while (true) {
    cv::Mat image;

    pthread_mutex_lock(&listMutex);
    if (imagesList.size() > 0) {
      image = imagesList.front().clone();
      imagesList.pop();  //delete image from list
    }
    pthread_mutex_unlock(&listMutex);

    if (!image.empty()){
      try {
        pthread_mutex_lock(&counterMutex);
        int this_counter = counter; // Get the current counter value
        counter++;
        pthread_mutex_unlock(&counterMutex);
        std::string filename = "./images/" + std::to_string(this_counter) + ".raw";
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
          std::cerr << "Failed to open file for writing: " << filename << std::endl;
          continue;
        }
        file.write(reinterpret_cast<const char*>(image.data), image.total() * image.elemSize());
        file.close();
      } catch (const std::exception& ex) {
        std::cerr << "Failed to save image: " << ex.what() << std::endl;
      }
    } else {usleep(1000); // If no image is available, wait a bit before checking again
    }
    pthread_mutex_lock(&globalTimeMutex);
    bool limit_reached = isTimelimitReached;
    pthread_mutex_unlock(&globalTimeMutex);
    if (limit_reached) {
      break;
    }
    
  }
  return NULL;
}

void* controller(void* args) {
  /**
 * @brief Controls the time limit for the image generation.
 *
 * This function runs in a separate thread and checks the elapsed time against the input duration.
 * If the elapsed time exceeds the input duration, it sets a flag to stop the image generation loop.
 */

  // Start the timer
  auto globalStart = std::chrono::system_clock::now();
  while (true) {
    int globalDuration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - globalStart).count();
    if (globalDuration >= inputDuration) {
      pthread_mutex_lock(&globalTimeMutex);
      isTimelimitReached = true; // Set the flag to stop the generation loop
      pthread_mutex_unlock(&globalTimeMutex);
      break;
    }
    usleep(100000); // Sleep for 100 milliseconds to avoid busy waiting
  } 
  return NULL;
}

int main(int argc, char **argv) {
  imageProperties properties = {1920,1080};
  std::filesystem::create_directory("./images");

  //Console inputs
  if (argc >= 5){
    try{
      threadsNumber = std::stoi(argv[3]);
        if (threadsNumber < 3){
          std::cout << "Threads number must be at least 3, setting to 3.\n";
          threadsNumber = 3;
        } else {
          std::cout <<  "Selected " << threadsNumber << " threads" << std::endl;
        }
      imageFormat = argv[4];
        if (
            (strcmp(imageFormat, ".bmp") != 0 &&
            strcmp(imageFormat, ".dib") != 0 &&
            strcmp(imageFormat, ".jpeg") != 0 &&
            strcmp(imageFormat, ".jpg") != 0 &&
            strcmp(imageFormat, ".jpe") != 0 &&
            strcmp(imageFormat, ".png") != 0 &&
            strcmp(imageFormat, ".ppm") != 0 &&
            strcmp(imageFormat, ".sr") != 0 &&
            strcmp(imageFormat, ".ras") != 0 &&
            strcmp(imageFormat, ".tiff") != 0 &&
            strcmp(imageFormat, ".tif") != 0 &&
            strcmp(imageFormat, ".hdr") != 0 &&
            strcmp(imageFormat, ".raw") != 0) || 
            (imageFormat[0] != '.')
        ){
        std::cout << "Invalid image format, closing program.\n"
                  << "Valid formats: .bmp, .dib, .jpeg, .jpg, .jpe, .png, .ppm, .sr, .ras, .tiff, .tif, .hdr, .raw\n";
        close(0); // Exit the program if an invalid image format is provided
        return 1;
      } else {
        std::cout << "Selected image format: " << imageFormat << std::endl;
      }
    }catch(const std::exception& e){
      std::cerr << e.what() << '\n';
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
  } else {
    std::cout << "Insufficient arguments provided.\n"
              << "Usage: ./generator [time_unit] [duration] [threads_number] [image_format]\n"
              << "Example: ./generator s 5 3 .png\n";
    close(0); // Exit the program if not enough arguments are provided
    return 1;
  }

  pthread_t threads[threadsNumber];

  //Start the program
  std::cout << "Generating a " << properties.width << "x" << properties.height << " random image..." << std::endl;

  pthread_create(&threads[0],nullptr,generateLoop,&properties);  
  pthread_create(&threads[1],nullptr,controller,nullptr);  
  for (int i = 2; i < threadsNumber; i++){
    pthread_create(&threads[i],nullptr, strcmp(imageFormat, ".raw") ? saveImage : saveImageRaw ,nullptr); 
  }
  for (int i = 0; i < threadsNumber; i++){
    pthread_join(threads[i],nullptr);
  }

  // End of the program
  std::cout << "\n--- SUMMARY ---\n"
        << "→ Total frames generated: " << totalFrames << "\n"
        << "→ Total time: " << inputDuration << " seconds\n"
        << "→ Total frames saved: " << counter << "\n"
        << "→ Total frames in queue: " << imagesList.size() << "\n"
        << "→ Total frames not queued: " << lostFrames << "\n"
        << "Timer might not be accurate due to the multithreading nature of the program.\n";
}
