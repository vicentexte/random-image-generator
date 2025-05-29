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
./random-image-generator
```
## Opciones
### Ajustar tiempo de procesamiento
```
./random-image-generator m 5
```
En este caso se estableció 5 minutos de duración (s: segundos, m: minutos, h: horas).
### Ajustar hilos usados
```
./random-image-generator m 5 5 //Se establecen 5 hilos
```
El tercer argumento corresponde a la cantidad de hilos que desea usar.
