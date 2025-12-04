#pragma once
#include "Machine.hpp"
#include "RawMaterial.hpp"

namespace Factory::Machinery {
    class RoboticArm : Machine {
    public:
        RoboticArm();

        void Run();
        void Process(Factory::Data::RawMaterial &&m);

    private:
        Machine *destination;
    };
}
