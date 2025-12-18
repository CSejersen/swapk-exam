#pragma once

#include <future>
#include <memory>
#include <vector>
#include "Machines/MachineTraits.hpp"
#include "Machines/Core/Mover.hpp"
#include "Machines/Core/Producer.hpp"
#include "Materials/AnyMaterial.hpp"
#include "Job.hpp"
#include <boost/signals2.hpp>

namespace Factory {

    class Controller {
    public:
        Controller() = default;

        // Template method for type-safe machine registration
        // Uses SFINAE via MachineTraits to dispatch to correct signal wiring
        template<typename MachineT, typename... Args>
        MachineT& AddMachine(Args&&... args) {
            auto machine = std::make_unique<MachineT>(std::forward<Args>(args)...);
            MachineT* ptr = machine.get();

            // Compile-time dispatch based on machine type traits
            if constexpr (Machinery::is_mover_v<MachineT>) {
                ConnectMoverSignals(ptr);
            } else if constexpr (Machinery::is_producer_v<MachineT>) {
                ConnectProducerSignals(ptr);
            }

            ownedMachines_.push_back(std::move(machine));
            ptr->StartThread();
            return *ptr;
        }

        // Signals
        boost::signals2::signal<void (
                Machinery::Mover* mover,
                Data::MaterialKind material,
                Machinery::MachineBase& destination,
                std::promise<bool>& cmdCompleted
        )> onTransportRequested;

        boost::signals2::signal<void (
                Data::MaterialKind material,
                Machinery::MachineBase& target,
                std::promise<bool>& cmdCompleted
        )> onProcessRequested;

        void executeJob(Job job);

    private:
        // Signal connection helpers using std::bind
        void ConnectMoverSignals(Machinery::Mover* mover);
        void ConnectProducerSignals(Machinery::MachineBase* producer);

        // Signal handlers (bound via std::bind)
        void HandleTransport(
            Machinery::Mover* targetMover,
            Machinery::Mover* requestedMover,
            Data::MaterialKind kind,
            Machinery::MachineBase& destination,
            std::promise<bool>& cmdCompleted
        );

        void HandleProcess(
            Machinery::MachineBase* targetProducer,
            Data::MaterialKind kind,
            Machinery::MachineBase& requestedTarget,
            std::promise<bool>& cmdCompleted
        );

        void executeJobStep(const JobStep& step, std::promise<bool>& promise);

        // Owned machines with unique_ptr for proper lifetime management
        std::vector<std::unique_ptr<Machinery::MachineBase>> ownedMachines_;
    };

} // namespace Factory
