#include "RoboticArm.hpp"

namespace Factory::Machinery {
    void RoboticArm::Process(Data::RawMaterial &&material) {
        std::cout << "[ROBOTIC ARM] moving material to destination" << std::endl;
        destination->ReceiveMaterial(std::move(material));
    }

    void RoboticArm::Run() {
        while (true) {
            // TODO: use signals

        }
    }

}


