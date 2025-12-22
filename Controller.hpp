#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "Machines/MachineTraits.hpp"
#include "Machines/Core/Mover.hpp"
#include "Machines/Core/Producer.hpp"
#include "Machines/Core/ResourceStation.h"
#include "Materials/AnyMaterial.hpp"
#include "Job.hpp"
#include <boost/signals2.hpp>

namespace Factory {

    // Compile-time configurable interval for resource generation (milliseconds)
    inline constexpr int GENERATION_INTERVAL_MS = 300;
    
    // Default worker pool size
    inline constexpr size_t DEFAULT_WORKER_COUNT = 2;

    class Controller {
    public:
        Controller() = default;
        
        ~Controller() {
            StopJobSpawner();
            StopWorkers();
            StopResourceGeneration();
        }

        // Template method for type-safe machine registration
        // Uses SFINAE via MachineTraits to dispatch to correct signal wiring
        template<typename MachineT, typename... Args>
        MachineT& AddMachine(Args&&... args) {
            auto machine = std::make_unique<MachineT>(std::forward<Args>(args)...);
            MachineT* ptr = machine.get();

            // Compile-time dispatch based on machine type traits
            if constexpr (Machinery::is_mover_v<MachineT>) {
                ConnectMoverSignal(ptr);
            } else if constexpr (Machinery::is_resource_station_v<MachineT>) {
                if (resourceStation_ != nullptr) {
                    throw std::runtime_error("Only one resource station can be added");
                }
                resourceStation_ = ptr;
            } else if constexpr (Machinery::is_producer_v<MachineT>) {
                ConnectProducerSignal(ptr);
            }

            ownedMachines_.push_back(std::move(machine));
            ptr->StartThread();
            return *ptr;
        }

        // Starts the resource generation thread
        void StartResourceGeneration();

        // Stops the resource generation thread
        void StopResourceGeneration() noexcept;

        // Signals
        boost::signals2::signal<void (
                Machinery::Mover* mover,
                Data::MaterialKind material,
                Machinery::MachineBase& source,
                Machinery::MachineBase& destination,
                std::promise<StepStatus>& cmdCompleted
        )> onTransportRequested;

        boost::signals2::signal<void (
                Data::MaterialKind material,
                Machinery::MachineBase& target,
                std::promise<StepStatus>& cmdCompleted
        )> onProcessRequested;

        void executeJob(Job job);

        // Job queue management
        void EnqueueJob(Job job);
        
        // Job spawner - spawns jobs at interval using factory function
        void StartJobSpawner(std::function<Job()> jobFactory, int intervalMs);
        void StopJobSpawner() noexcept;
        
        // Worker pool management
        void StartWorkers(size_t workerCount = DEFAULT_WORKER_COUNT);
        void StopWorkers() noexcept;

    private:
        // Signal connection helpers using std::bind
        void ConnectMoverSignal(Machinery::Mover* mover);
        void ConnectProducerSignal(Machinery::MachineBase* producer);

        // Signal handlers (bound via std::bind)
        void HandleTransport(
            Machinery::Mover* targetMover,
            Machinery::Mover* requestedMover,
            Data::MaterialKind kind,
            Machinery::MachineBase& source,
            Machinery::MachineBase& destination,
            std::promise<StepStatus>& cmdCompleted
        );

        void HandleProcess(
            Machinery::MachineBase* targetProducer,
            Data::MaterialKind kind,
            Machinery::MachineBase& requestedTarget,
            std::promise<StepStatus>& cmdCompleted
        );

        void executeJobStep(const JobStep& step, std::promise<StepStatus>& promise);

        // Resource generation thread loop
        void ResourceGenerationLoop();

        // Owned machines with unique_ptr for proper lifetime management
        std::vector<std::unique_ptr<Machinery::MachineBase>> ownedMachines_;

        // Resource generation thread members
        Machinery::ResourceStation* resourceStation_{nullptr};
        std::thread resourceGenThread_;
        std::atomic_bool stopGeneration_{false};
        std::atomic_bool generationRunning_{false};

        // Job queue members (thread-safe)
        std::queue<Job> jobQueue_;
        std::mutex queueMutex_;
        std::condition_variable queueCV_;

        // Worker pool members
        std::vector<std::thread> workers_;
        std::atomic_bool stopWorkers_{false};
        std::atomic_bool workersRunning_{false};
        void WorkerLoop(size_t workerId);

        // Job spawner members
        std::thread spawnerThread_;
        std::atomic_bool stopSpawner_{false};
        std::atomic_bool spawnerRunning_{false};
        std::function<Job()> jobFactory_;
        int spawnerIntervalMs_{1000};
        void JobSpawnerLoop();
    };

} // namespace Factory
