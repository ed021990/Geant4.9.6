MPI/Examples : exMPI01
======================

Description
-----------
A simple application

### Configuration:

- Geometry     : chamber / calorimeter
- Primary      : particle gun (200 MeV electron as default)
- Physics List : standard EM

The environment variable *G4LEDATA* for low energy EM data is required.

### Features:

- Particles are transported in a geometry without any scoring.
- Learn how to parallelized your G4 session.


How to build
------------
Use CMake on Geant4 library installed with CMake build.

Check `CMakeList.txt`, especially the following two variables
should be taken care to match your MPI library.

    eg.
    set(CMAKE_CXX_COMPILER mpicxx)
    # set(CMAKE_CXX_INCLUDE_PATH ) set if necessary

Follow these commands,

    > mkdir build
    > cd build
    > cmake -DGeant4_DIR=<your Geant4 install path>/lib64/Geant4-V.m.n ..
      (V.m.n is the version of Geant4, eg. Geant4-9.6.0)
    > make
    > make install

