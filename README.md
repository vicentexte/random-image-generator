# Random Image Generator

Random image generator written in C++.

## Requirements

* CMake 3.10 or higher
* C++11 compatible compiler (g++, clang, etc.)

## Build

```bash
git clone https://github.com/vicentexte/random-image-generator.git
cd random-image-generator
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
./random-image-generator [Time unit] [Time amount] [Thread count] [Format]
```

* Time units -> s: seconds, m: minutes, h: hours
* Available formats -> .bmp, .dib, .jpeg, .jpg, .jpe, .png, .ppm, .sr, .ras, .tiff, .tif, .hdr, .raw

## System Check

To run the main program, it is recommended to execute the following command first in order to apply the necessary adjustments for the main code:

```bash
./system_check
```
