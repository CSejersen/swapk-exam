cmake_minimum_required(VERSION 3.26)
project(swapk_exam)

set(CMAKE_CXX_STANDARD 20)

set(BOOST_ROOT "/opt/homebrew")
set(BOOST_INCLUDEDIR "/opt/homebrew/include")
set(BOOST_LIBRARYDIR "/opt/homebrew/lib")

find_package(Boost REQUIRED COMPONENTS filesystem)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(swapk_exam main.cpp
        Materials/AnyMaterial.hpp
        MachineConepts.hpp
        Machines/Core/MachineBase.hpp
        Machines/Core/Producer.hpp
        Controller.cpp
        Controller.hpp
        Machines/CNCMachine.hpp
        Machines/Core/Producer.hpp
        Machines/Core/Mover.cpp
        Machines/Core/Mover.hpp
        Job.cpp
        Job.hpp
        Shared.hpp
        Machines/Core/ResourceStation.cpp
        Machines/Core/ResourceStation.h
)


target_link_libraries(swapk_exam ${BOOST_LIBRARIES})
