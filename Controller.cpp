#include "Controller.hpp"
#include "Machines/CNCMachine.hpp"
#include <type_traits>
#include <thread>
#include <chrono>
#include <stdexcept>

namespace Factory {
    Controller::Controller() = default;

    void Controller::Setup() {
        auto arm1 = std::make_unique<Factory::Machinery::Mover>("arm-1");
        arm1_ = arm1.get();
        onTransportRequested.connect(
            [arm1_ptr = arm1.get()](
                    Machinery::Mover* arm,
                    Factory::Data::MaterialKind kind,
                    Machinery::MachineBase& destination) {
                if (arm != arm1_ptr) {
                    return;
                }
                Machinery::TransportCommand command {kind, destination};
                arm1_ptr->EnqueueCommand(std::move(command));
            }
        );
        arm1->StartThread();
        machines.push_back(std::move(arm1));

        auto cnc1 = std::make_unique<Factory::Machinery::CNCMachine<Factory::Data::MetalPipe>>("CNC-1");
        cnc1_ = cnc1.get();
        onProcessRequested.connect(
            [cnc1_ptr = cnc1.get()](
                    Factory::Data::MaterialKind kind,
                    Machinery::MachineBase& target) {
                if (&target != cnc1_ptr) {
                    return;
                }
                Machinery::ProcessCommand command {kind};
                cnc1_ptr->EnqueueCommand(std::move(command));
            }
        );
        cnc1->StartThread();
        machines.push_back(std::move(cnc1));
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

    void Controller::executeJob(Factory::Job job) {
        int stepCounter = 1;
        while (!job.stepsEmpty()) {
            const auto& currentStep = job.getNextStep();
            executeJobStep(currentStep);

            job.popStep();
            std::cout << "job: " << job.name() << " executing step number " << stepCounter << std::endl;
            stepCounter++;
        }
        std::cout << "job: " << job.name() << " finished" << std::endl;
    }

    void Controller::executeJobStep(const Factory::JobStep& step) {
        std::visit([this](const auto& s) {
            using S = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<S, Factory::MoveStep>) {
                onTransportRequested(&s.mover.get(), s.material, s.destination.get());
            } else if constexpr (std::is_same_v<S, Factory::ProcessStep>) {
                onProcessRequested(s.material, s.executor.get());
            }
        }, step);
    }
}

