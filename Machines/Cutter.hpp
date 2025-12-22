#pragma once
#include "Core/Producer.hpp"
#include "../MachineConepts.hpp"

#include <concepts>
#include <iostream>

namespace Factory::Machinery {
    template<Data::Cuttable T>
    class Cutter : public Producer<T> {
    public:
        using Producer<T>::Producer;

    private:
        void ProcessOne(T&& item) override {
            auto out = item.cutInHalf();
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            // Store output for potential later pickup/transport
            this->Emit(Data::AnyMaterial{std::move(out)});
            std::cout << "[PRODUCER] " << this->Name() << " processed material_kind=" << Data::toString(T::kind) << std::endl;
        }
    };
}


