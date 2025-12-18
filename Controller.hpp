#pragma once
#include <future>
#include <vector>
#include "Machines/Core/Mover.hpp"
#include "Machines/Core/Producer.hpp"
#include "Materials/AnyMaterial.hpp"
#include "Job.hpp"
#include <boost/signals2.hpp>

namespace Factory {

    class Controller {
    public:
        Controller();
        void Setup();

        void AddMachine(Machinery::MachineBase* machine);

        // Accessors for demo/tests (instances are created in Setup()).
        // Valid after calling Setup().
        Machinery::Mover& Arm1();
        Machinery::MachineBase& CNC1();

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

        void executeJob(Job);

    private:
        void executeJobStep(const JobStep& step, std::promise<bool>& promise);
        std::vector<Machinery::MachineBase*> machines;

        Machinery::Mover* arm1_{nullptr};
        Machinery::MachineBase* cnc1_{nullptr};
    };
}



