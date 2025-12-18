#include "Controller.hpp"
#include "Machines/CNCMachine.hpp"

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
                _3,         // destination - from signal
                _4          // cmdCompleted - from signal
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
        Machinery::MachineBase& destination,
        std::promise<StepStatus>& cmdCompleted)
    {
        // Only handle if this is the mover we're bound to
        if (requestedMover != targetMover) {
            return;
        }

        Machinery::TransportCommand command{kind, destination, cmdCompleted};
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
                onTransportRequested(&s.mover.get(), s.material, s.destination.get(), promise);
            } else if constexpr (std::is_same_v<S, ProcessStep>) {
                onProcessRequested(s.material, s.executor.get(), promise);
            }
        }, step);
    }

} // namespace Factory
