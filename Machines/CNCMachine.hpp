#pragma once
#include "Core/Producer.hpp"
#include "../MachineConepts.hpp"

#include <concepts>
#include <iostream>

namespace Factory::Machinery {
    template<Data::CNCCompatible T>
    class CNCMachine : public Producer<T> {
    public:
        using Producer<T>::Producer;

    private:
        void ProcessOne(T&& item) override {
            // We can call CNC-only operations, guaranteed by the concept.
            auto out = item.cutInHalf();

            // Store output for potential later pickup/transport
            this->Emit(Data::AnyMaterial{std::move(out)});
            std::cout << "[PRODUCER] " << this->Name() << " processed material_kind=" << Data::toString(T::kind) << std::endl;
        }
    };
}

