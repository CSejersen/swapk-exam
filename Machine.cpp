#include "Machine.hpp"
#include <iostream>

namespace Factory::Machinery{

    void Machine::EmergencyStop() {
        std::cout << "[MACHINE] emergency stop" << std::endl;
    }

    void Machine::DoMaintenance() {
        std::cout << "[MACHINE] performing maintenance" << std::endl;
    }

    void Machine::ReceiveMaterial(Data::RawMaterial &&material) {
        availableMaterials[material.getType()].push_back(std::move(material));
    }
}
