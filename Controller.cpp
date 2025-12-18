#include "Controller.hpp"
#include "Machines/CNCMachine.hpp"

#include <functional>
#include <iostream>

namespace Factory {

    void Controller::ConnectMoverSignals(Machinery::Mover* mover) {
        using namespace std::placeholders;

        // Use std::bind to create a callable that captures the target mover
        // and forwards the signal arguments to HandleTransport
        onTransportRequested.connect(
            std::bind(
                &Controller::HandleTransport,
                this,
                mover,      // targetMover - bound at connection time
                _1,         // requestedMover - from signal
                _2,         // kind - from signal
                _3,         // destination - from signal
                _4          // cmdCompleted - from signal
            )
        );

        std::cout << "[CONTROLLER] Connected transport signals for mover: " 
                  << mover->Name() << std::endl;
    }

    void Controller::ConnectProducerSignals(Machinery::MachineBase* producer) {
        using namespace std::placeholders;

        // Use std::bind to create a callable that captures the target producer
        // and forwards the signal arguments to HandleProcess
        onProcessRequested.connect(
            std::bind(
                &Controller::HandleProcess,
                this,
                producer,   // targetProducer - bound at connection time
                _1,         // kind - from signal
                _2,         // requestedTarget - from signal
                _3          // cmdCompleted - from signal
            )
        );

        std::cout << "[CONTROLLER] Connected process signals for producer: " 
                  << producer->Name() << std::endl;
    }

    void Controller::HandleTransport(
        Machinery::Mover* targetMover,
        Machinery::Mover* requestedMover,
        Data::MaterialKind kind,
        Machinery::MachineBase& destination,
        std::promise<bool>& cmdCompleted)
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
        std::promise<bool>& cmdCompleted)
    {
        // Only handle if this is the producer we're bound to
        if (&requestedTarget != targetProducer) {
            return;
        }

        Machinery::ProcessCommand command{kind, cmdCompleted};
        // We need to cast to the correct producer type to call EnqueueCommand
        // Since we verified this is a producer at registration time via traits,
        // we use the base class Enqueue mechanism
        // Note: EnqueueCommand is on Producer<T>, but we store MachineBase*
        // The signal system will route to the correct handler
        
        // For now, we'll use a dynamic approach since Producer<T> is templated
        // In a real system, you might use a type-erased command interface
        if (auto* producer = dynamic_cast<Machinery::Producer<Data::MetalPipe>*>(targetProducer)) {
            producer->EnqueueCommand(std::move(command));
        }
        // Add other material types as needed
    }

    void Controller::executeJob(Job job) {
        int stepCounter = 1;
        int retries = 0;

        while (!job.stepsEmpty()) {
            std::cout << "[CONTROLLER] job: " << job.name() 
                      << " executing step number " << stepCounter 
                      << ". Attempt number: " << retries + 1 << std::endl;

            const auto& currentStep = job.getNextStep();
            std::promise<bool> promise;
            auto future = promise.get_future();

            executeJobStep(currentStep, promise);
            future.wait();

            bool success = future.get();
            if (!success) {
                std::cout << "[CONTROLLER] job: " << job.name() 
                          << " step number " << stepCounter << " failed" << std::endl;
                if (retries >= 2) {
                    std::cout << "[CONTROLLER] job: " << job.name() 
                              << " step number " << stepCounter 
                              << " exceeded max retries, aborting job" << std::endl;
                    break;
                }
                retries++;
                continue;
            }

            std::cout << "[CONTROLLER] job: " << job.name() 
                      << " step number " << stepCounter 
                      << " completed successfully" << std::endl;
            job.popStep();
            stepCounter++;
            retries = 0;
        }

        std::cout << "[CONTROLLER] job: " << job.name() << " finished" << std::endl;
    }

    void Controller::executeJobStep(const Factory::JobStep& step, std::promise<bool>& promise) {
        std::visit([this, &promise](const auto& s) {
            using S = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<S, Factory::MoveStep>) {
                onTransportRequested(&s.mover.get(), s.material, s.destination.get(), promise);
            } else if constexpr (std::is_same_v<S, Factory::ProcessStep>) {
                onProcessRequested(s.material, s.executor.get(), promise);
            }
        }, step);
    }

} // namespace Factory
