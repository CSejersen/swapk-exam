#include "Controller.hpp"
#include "Machines/Cutter.hpp"

#include <array>
#include <functional>
#include <iostream>

namespace Factory {

    void Controller::ConnectMoverSignal(Machinery::Mover* mover) {
        using namespace std::placeholders;

        // Use std::bind to create a callable that captures the target mover
        // and forwards the signal arguments to HandleTransport
        onTransportRequested.connect(
            std::bind(
                &Controller::HandleTransport,
                this,
                mover,      // targetMover - bound at connection time
                _1,         // requestedMover - from signal
                _2,         // material_kind - from signal
                _3,         // source - from signal
                _4,         // destination - from signal
                _5          // cmdCompleted - from signal
            )
        );

        std::cout << "[CONTROLLER] Connected transport signal for mover: "
                  << mover->Name() << std::endl;
    }

    void Controller::ConnectProducerSignal(Machinery::MachineBase* producer) {
        using namespace std::placeholders;

        // Use std::bind to create a callable that captures the target producer
        // and forwards the signal arguments to HandleProcess
        onProcessRequested.connect(
            std::bind(
                &Controller::HandleProcess,
                this,
                producer,   // targetProducer - bound at connection time
                _1,         // material_kind - from signal
                _2,         // requestedTarget - from signal
                _3          // cmdCompleted - from signal
            )
        );

        std::cout << "[CONTROLLER] Connected process signal for producer: "
                  << producer->Name() << std::endl;
    }

    void Controller::HandleTransport(
        Machinery::Mover* targetMover,
        Machinery::Mover* requestedMover,
        Data::MaterialKind kind,
        Machinery::MachineBase& source,
        Machinery::MachineBase& destination,
        std::promise<StepStatus>& cmdCompleted)
    {
        // Only handle if this is the mover we're bound to
        if (requestedMover != targetMover) {
            return;
        }

        Machinery::TransportCommand command{kind, source, destination, cmdCompleted};
        targetMover->EnqueueCommand(std::move(command));
    }

    void Controller::HandleProcess(
        Machinery::MachineBase* targetProducer,
        Data::MaterialKind kind,
        Machinery::MachineBase& requestedTarget,
        std::promise<StepStatus>& cmdCompleted)
    {
        // Only handle if this is the producer we're bound to
        if (&requestedTarget != targetProducer) {
            return;
        }

        Machinery::ProcessCommand command{kind, cmdCompleted};
        targetProducer->EnqueueCommand(std::move(command));
    }



    void Controller::executeJob(Job job) {
        int stepCounter = 1;

        while (!job.stepsEmpty()) {
            std::cout << "[CONTROLLER] job: " << job.name()
                      << " executing step number " << stepCounter
                      <<  std::endl;

            const auto& currentStep = job.getNextStep();
            std::promise<StepStatus> promise;
            auto future = promise.get_future();

            executeJobStep(currentStep, promise);
            future.wait();

            StepStatus stepStatus = future.get();

            for (int retries = 0; stepStatus == RETRY && retries < 3; retries++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                std::cout << "[CONTROLLER] job: " << job.name()
                      << " retrying step number " << stepCounter
                      << " retry attempt " << (retries + 1)
                      <<  std::endl;
                promise = std::promise<StepStatus>();
                future = promise.get_future();
                executeJobStep(currentStep, promise);
                future.wait();
                stepStatus = future.get();
            }

            switch (stepStatus) {
                case SUCCESS:
                    std::cout << "[CONTROLLER] job: " << job.name()
                      << " step number " << stepCounter
                      << " completed successfully" << std::endl;
                    job.popStep();
                    stepCounter++;
                    break;
                case RETRY:
                    throw std::runtime_error("Job execution failed due to exceeding max retries");
                case ERROR:
                    throw std::runtime_error("Job execution failed due to critical error");
            }
        }
    }

    void Controller::executeJobStep(const JobStep& step, std::promise<StepStatus>& promise) {
        std::visit([this, &promise](const auto& s) {
            using S = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<S, MoveStep>) {
                onTransportRequested(&s.mover.get(), s.material, s.source.get(), s.destination.get(), promise);
            } else if constexpr (std::is_same_v<S, ProcessStep>) {
                onProcessRequested(s.material, s.executor.get(), promise);
            }
        }, step);
    }

    void Controller::StartResourceGeneration() {
        if (!resourceStation_) {
            std::cerr << "[CONTROLLER] Cannot start resource generation: no ResourceStation registered" << std::endl;
            return;
        }
        
        if (generationRunning_.exchange(true)) {
            std::cout << "[CONTROLLER] Resource generation already running" << std::endl;
            return;
        }
        
        stopGeneration_.store(false);
        resourceGenThread_ = std::thread(&Controller::ResourceGenerationLoop, this);
        std::cout << "[CONTROLLER] Started resource generation thread" << std::endl;
    }

    void Controller::StopResourceGeneration() noexcept {
        if (!generationRunning_.exchange(false)) {
            return;
        }
        
        stopGeneration_.store(true);
        if (resourceGenThread_.joinable()) {
            try {
                resourceGenThread_.join();
                std::cout << "[CONTROLLER] Stopped resource generation thread" << std::endl;
            } catch (...) {
                std::cerr << "[ERROR] Failed to join resource generation thread" << std::endl;
            }
        }
    }

    void Controller::ResourceGenerationLoop() {
        // Material types to generate in rotation
        constexpr std::array<Data::MaterialKind, 3> materials = {
            Data::MaterialKind::MetalPipe,
            Data::MaterialKind::Gravel,
            Data::MaterialKind::TitaniumSlab
        };
        
        size_t materialIndex = 0;
        
        while (!stopGeneration_.load()) {
            // Enqueue generation command for current material
            Machinery::GenerateResourceCommand cmd{materials[materialIndex]};
            resourceStation_->EnqueueCommand(cmd);
            
            // Rotate to next material
            materialIndex = (materialIndex + 1) % materials.size();
            
            // Wait for the configured interval
            std::this_thread::sleep_for(std::chrono::milliseconds(GENERATION_INTERVAL_MS));
        }
    }

    // ==================== Job Queue ====================

    void Controller::EnqueueJob(Job job) {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            jobQueue_.push(std::move(job));
            std::cout << "[CONTROLLER] Job enqueued. Queue size: " << jobQueue_.size() << std::endl;
        }
        queueCV_.notify_one();
    }

    // ==================== Worker Pool ====================

    void Controller::StartWorkers(size_t workerCount) {
        if (workersRunning_.exchange(true)) {
            std::cout << "[CONTROLLER] Workers already running" << std::endl;
            return;
        }

        stopWorkers_.store(false);
        workers_.reserve(workerCount);

        for (size_t i = 0; i < workerCount; ++i) {
            workers_.emplace_back(&Controller::WorkerLoop, this, i);
        }

        std::cout << "[CONTROLLER] Started " << workerCount << " worker threads" << std::endl;
    }

    void Controller::StopWorkers() noexcept {
        if (!workersRunning_.exchange(false)) {
            return;
        }

        stopWorkers_.store(true);
        queueCV_.notify_all();  // Wake up all waiting workers

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                try {
                    worker.join();
                } catch (...) {
                    std::cerr << "[ERROR] Failed to join worker thread" << std::endl;
                }
            }
        }
        workers_.clear();
        std::cout << "[CONTROLLER] Stopped all worker threads" << std::endl;
    }

    void Controller::WorkerLoop(size_t workerId) {
        std::cout << "[WORKER " << workerId << "] Started" << std::endl;

        while (!stopWorkers_.load()) {
            Job job("");
            bool hasJob = false;

            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                queueCV_.wait(lock, [this] {
                    return stopWorkers_.load() || !jobQueue_.empty();
                });

                if (stopWorkers_.load() && jobQueue_.empty()) {
                    break;
                }

                if (!jobQueue_.empty()) {
                    job = std::move(jobQueue_.front());
                    jobQueue_.pop();
                    hasJob = true;
                    std::cout << "[WORKER " << workerId << "] Picked up job: " 
                              << job.name() << std::endl;
                }
            }

            if (hasJob) {
                try {
                    executeJob(std::move(job));
                } catch (const std::exception& e) {
                    std::cerr << "[WORKER " << workerId << "] Job failed: " 
                              << e.what() << std::endl;
                }
            }
        }

        std::cout << "[WORKER " << workerId << "] Stopped" << std::endl;
    }

    // ==================== Job Spawner ====================

    void Controller::StartJobSpawner(std::function<Job()> jobFactory, int intervalMs) {
        if (spawnerRunning_.exchange(true)) {
            std::cout << "[CONTROLLER] Job spawner already running" << std::endl;
            return;
        }

        jobFactory_ = std::move(jobFactory);
        spawnerIntervalMs_ = intervalMs;
        stopSpawner_.store(false);

        spawnerThread_ = std::thread(&Controller::JobSpawnerLoop, this);
        std::cout << "[CONTROLLER] Started job spawner with interval " 
                  << intervalMs << "ms" << std::endl;
    }

    void Controller::StopJobSpawner() noexcept {
        if (!spawnerRunning_.exchange(false)) {
            return;
        }

        stopSpawner_.store(true);

        if (spawnerThread_.joinable()) {
            try {
                spawnerThread_.join();
                std::cout << "[CONTROLLER] Stopped job spawner thread" << std::endl;
            } catch (...) {
                std::cerr << "[ERROR] Failed to join job spawner thread" << std::endl;
            }
        }
    }

    void Controller::JobSpawnerLoop() {
        size_t jobCounter = 0;

        while (!stopSpawner_.load()) {
            if (jobFactory_) {
                Job newJob = jobFactory_();
                EnqueueJob(std::move(newJob));
                ++jobCounter;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(spawnerIntervalMs_));
        }

        std::cout << "[SPAWNER] Total jobs spawned: " << jobCounter << std::endl;
    }

} // namespace Factory
