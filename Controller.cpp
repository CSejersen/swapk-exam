#include "Controller.hpp"
#include "Machines/CNCMachine.hpp"
#include <type_traits>
#include <thread>
#include <chrono>
#include <stdexcept>

namespace Factory {
    Controller::Controller() = default;

    void Controller::Setup() {
        for (auto* machine : machines) {

        }
        arm1_ = arm1.get();
        onTransportRequested.connect(
            [arm1_ptr = arm1.get()](
                    Machinery::Mover* arm,
                    Factory::Data::MaterialKind kind,
                    Machinery::MachineBase& destination,
                    std::promise<bool> &cmdCompleted) {
                if (arm != arm1_ptr) {
                    return;
                }
                Machinery::TransportCommand command {kind, destination, cmdCompleted};
                arm1_ptr->EnqueueCommand(std::move(command));
            }
        );
        arm1->StartThread();

        auto cnc1 = std::make_unique<Factory::Machinery::CNCMachine<Factory::Data::MetalPipe>>("CNC-1");
        cnc1_ = cnc1.get();
        onProcessRequested.connect(
            [cnc1_ptr = cnc1.get()](
                    Factory::Data::MaterialKind kind,
                    Machinery::MachineBase& target,
                    std::promise<bool> &cmdCompleted) {
                if (&target != cnc1_ptr) {
                    return;
                }
                Machinery::ProcessCommand command {kind, cmdCompleted};
                cnc1_ptr->EnqueueCommand(std::move(command));
            }
        );
        cnc1->StartThread();
    }

    void Controller::AddMachine(Machinery::MachineBase *machine) {
        std::cout << "[CONTROLLER] Adding machine: " << machine->Name() << std::endl;
        machines.push_back(machine);
    }

    Factory::Machinery::Mover& Controller::Arm1() {
        if (!arm1_) {
            throw std::logic_error("Controller::Arm1() called before Setup()");
        }
        return *arm1_;
    }

    Factory::Machinery::MachineBase& Controller::CNC1() {
        if (!cnc1_) {
            throw std::logic_error("Controller::CNC1() called before Setup()");
        }
        return *cnc1_;
    }

    void Controller::executeJob(Job job) {
        int stepCounter = 1;
        int retries = 0;
        while (!job.stepsEmpty()) {
            std::cout << "[CONTROLLER] job: " << job.name() << " executing step number " << stepCounter << ". Attempt number: " << retries + 1<< std::endl;
            const auto& currentStep = job.getNextStep();
            std::promise<bool> promise;
            auto future = promise.get_future();

            executeJobStep(currentStep, promise);
            // Wait for step to complete
            future.wait();

            bool success = future.get();
            if (!success) {
                std::cout << "[CONTROLLER] job: " << job.name() << " step number " << stepCounter << " failed" << std::endl;
                if (retries >= 2) {
                    std::cout << "[CONTROLLER] job: " << job.name() << " step number " << stepCounter << " exceeded max retries, aborting job" << std::endl;
                    break;
                }
                retries++;
                continue;
            }
            std::cout << "[CONTROLLER] job: " << job.name() << " step number " << stepCounter << " completed successfully" << std::endl;
            job.popStep();
            stepCounter++;

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
}

