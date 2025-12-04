#pragma once
#include "RawMaterial.hpp"

namespace Factory::Machinery {
    class Machine {
    public:
        virtual void Process(Factory::Data::RawMaterial &material) = 0;
        virtual void Run() = 0;
        virtual void EmergencyStop();
        virtual void DoMaintenance();
        virtual void ReceiveMaterial(Data::RawMaterial &&material);

    protected:
        bool isBroken;
        std::unordered_map<std::string, std::vector<Factory::Data::RawMaterial>> availableMaterials;
    };

}

