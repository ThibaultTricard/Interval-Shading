# Interval-Shading

Source code for the research report "Interval Shading: using Mesh Shaders to generate shading intervals for volume rendering" available at https://hal.science/hal-04561269

## Getting Started 

Make sure all the submodules have been pulled with the following command
```
git submodule update --init --recursive
```

Then use CMake to compile.

## Compiling

first use cmake to configure the project

```
cmake -S . -B build
```

Then go to the build folder 
```
cd build
```

and compile with :
```
make
```

## Running

the Six example :
  - armadillo
  - asteroids
  - bunny
  - crystal
  - jet
  - singleTet

can be run by using
```
./[nameOfTheExample]
```

In all example, the mouse can be used to move around the objects and the keyboard's arrow can be use to move the camera.

In the jet example the 'o' key start the animation and the 'p' key pauses it. 

