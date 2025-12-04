cmake_minimum_required(VERSION 3.26)
project(swapk_exam)

set(CMAKE_CXX_STANDARD 20)

add_executable(swapk_exam main.cpp
        RawMaterial.hpp
        RawMaterial.cpp
        MachineConepts.hpp
        Machine.hpp
        RoboticArm.hpp
        Machine.cpp
        RoboticArm.cpp
        Controller.cpp
        Controller.hpp)
