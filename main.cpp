#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

#include "Controller.hpp"
#include "Machines/Cutter.hpp"

using namespace Factory;

int main() {
    Controller controller;

    // Type-safe machine registration using AddMachine<T>()
    // The template dispatches to correct signal wiring via MachineTraits + SFINAE
    auto& resourceStation = controller.AddMachine<Machinery::ResourceStation>("Resource_Station");
    controller.StartResourceGeneration();

    auto& arm1 = controller.AddMachine<Machinery::Mover>("Arm-1");
    auto& cutter1 = controller.AddMachine<Machinery::Cutter<Data::MetalPipe>>("Cutter-1");

    // Give machines time to start their worker threads
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Job counter for unique job names
    std::atomic<int> jobCounter{0};

    // Job factory lambda - creates demo jobs with unique names
    auto jobFactory = [&]() -> Job {
        int id = jobCounter.fetch_add(1);
        Job job("job-" + std::to_string(id));
        
        // Each job: move material to cutter, process, move back
        job.addStep(MoveStep{arm1, Data::MaterialKind::MetalPipe, resourceStation, cutter1});
        job.addStep(ProcessStep{cutter1, Data::MaterialKind::MetalPipe, Data::MaterialKind::MetalPipeHalf});

        return job;
    };

    // Start worker pool with 2 workers
    controller.StartWorkers(2);

    // Start job spawner - creates a new job every 2 seconds
    controller.StartJobSpawner(jobFactory, 2000);

    std::cout << "\n=== Factory simulation running ===" << std::endl;
    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

    std::cout << "\n=== Shutting down ===" << std::endl;

    // Destructor handles graceful shutdown of:
    // - Job spawner
    // - Worker pool  
    // - Resource generation

    return 0;
}
