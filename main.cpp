#include <iostream>
#include <thread>
#include <chrono>

#include "Controller.hpp"
#include "Machines/CNCMachine.hpp"

using namespace Factory;

int main() {
    Controller controller;

    // Type-safe machine registration using AddMachine<T>()
    // The template dispatches to correct signal wiring via MachineTraits + SFINAE
    auto& arm1 = controller.AddMachine<Machinery::Mover>("Arm-1");
    auto& cnc1 = controller.AddMachine<Machinery::CNCMachine<Data::MetalPipe>>("CNC-1");

    // Give machines time to start their worker threads
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Preload material so the move step has something to transport
    arm1.TryReceive(Data::AnyMaterial{Data::MetalPipe{Data::DataBuffer(1024)}});

    // Create a job with move and process steps
    Job job("demo-job");
    job.addStep(MoveStep{arm1, Data::MaterialKind::MetalPipe,, cnc1});
    job.addStep(ProcessStep{cnc1, Data::MaterialKind::MetalPipe, Data::MaterialKind::MetalPipeHalf});
    
    // Extra step to demonstrate retry behavior when there's nothing left to process
    job.addStep(ProcessStep{cnc1, Data::MaterialKind::MetalPipe, Data::MaterialKind::MetalPipeHalf});

    try {
      controller.executeJob(job);
    }
    catch (std::exception &e) {
        std::cerr << "[ERROR] An exception occured! with error: " << e.what()<< std::endl;
    }
    // Give worker threads time to execute enqueued commands before program exits
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    return 0;
}
