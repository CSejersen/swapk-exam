#pragma once
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

        // Accessors for demo/tests (instances are created in Setup()).
        // Valid after calling Setup().
        Factory::Machinery::Mover& Arm1();
        Factory::Machinery::MachineBase& CNC1();

        // Signals
        boost::signals2::signal<void (
                Factory::Machinery::Mover* mover,
                Factory::Data::MaterialKind material,
                Factory::Machinery::MachineBase& destination
        )> onTransportRequested;

        boost::signals2::signal<void (
                Factory::Data::MaterialKind material,
                Factory::Machinery::MachineBase& target
        )> onProcessRequested;

        void executeJob(Factory::Job);

    private:
        void executeJobStep(const Factory::JobStep& step);
        std::vector<std::unique_ptr<Factory::Machinery::MachineBase>> machines;

        Factory::Machinery::Mover* arm1_{nullptr};
        Factory::Machinery::MachineBase* cnc1_{nullptr};
    };
}



