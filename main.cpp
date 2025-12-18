#include <iostream>
#include "Controller.hpp"
#include <thread>
#include <chrono>

using namespace Factory;

int main() {
    // Setup controller and machines
    Controller controller;
    controller.Setup();
    
    // Give machines time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Small test/demo: create a job with a few steps and execute it.
    auto& arm = controller.Arm1();
    auto& cnc = controller.CNC1();

    // Preload material so the move step has something to transport.
    arm.TryReceive(Factory::Data::AnyMaterial{Factory::Data::MetalPipe{}});

    Factory::Job job("demo-job");
    job.addStep(Factory::MoveStep{arm, Factory::Data::MaterialKind::MetalPipe, cnc});
    job.addStep(Factory::ProcessStep{cnc, Factory::Data::MaterialKind::MetalPipe, Factory::Data::MaterialKind::MetalPipeHalf});
    // Extra step to show behavior when there's nothing left to process.
    job.addStep(Factory::ProcessStep{cnc, Factory::Data::MaterialKind::MetalPipe, Factory::Data::MaterialKind::MetalPipeHalf});

    controller.executeJob(job);

    // Give worker threads time to execute enqueued commands before program exits.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    return 0;
}
