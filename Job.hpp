#pragma once

#include "Machines/Core/MachineBase.hpp"
#include "Machines/Core/Mover.hpp"

#include <functional>
#include <queue>
#include <string>
#include <variant>

namespace Factory {


    struct MoveStep {
        std::reference_wrapper<Machinery::Mover> mover;
        Data::MaterialKind material;
        std::reference_wrapper<Machinery::MachineBase> destination;
    };

    struct ProcessStep {
        std::reference_wrapper<Machinery::MachineBase> executor;
        Data::MaterialKind material;
        Data::MaterialKind product; // currently unused by Controller
    };

    using JobStep = std::variant<MoveStep, ProcessStep>;

    class Job {
    public:
        Job(std::string name);
        void addStep(JobStep step);
        const JobStep& getNextStep() const;
        void popStep();
        bool stepsEmpty();
        std::string name();

    private:
        std::string name_;
        std::queue<JobStep> steps_;
    };
}


