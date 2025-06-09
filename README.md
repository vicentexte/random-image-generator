# Random Image Generator

Generador de imágenes aleatorias escrito en C++.

## Requisitos

- CMake 3.10 o superior
- Compilador C++ compatible con C++11 (g++, clang, etc.)

## Compilación

```bash
git clone https://github.com/vicentexte/random-image-generator.git
cd random-image-generator
mkdir build && cd build
cmake ..
make
```

## Uso

```
./random-image-generator [Unidad de tiempo] [Cantidad de tiempo] [Cantidad de hilos] [Formato]
```
- Unidades de tiempo -> s: segundos, m: minutos, h:horas
- Formatos disponibles -> .bmp, .dib, .jpeg, .jpg, .jpe, .png, .ppm, .sr, .ras, .tiff, .tif, .hdr, .raw

## Checkear el sistema
Para ejecutar el código principal, se recomienda utilizar el siguiente código con el fin de aplicar los ajustes necesarios al código principal
```
./system_check
```
