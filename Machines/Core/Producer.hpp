#pragma once

#include "MachineBase.hpp"

#include <concepts>
#include <iostream>
#include <queue>
#include <type_traits>
#include <string>

namespace Factory::Machinery {
    template<class T>
    concept HasMaterialKind = requires {
        { T::kind } -> std::convertible_to<Data::MaterialKind>;
    };

    template<HasMaterialKind T>
    class Producer : public MachineBase {
    public:
        using MachineBase::MachineBase;

        // Receiver API (via MachineBase)
        bool CanAccept(Data::MaterialKind kind) const noexcept override {
            return kind == T::kind;
        }

        void TryReceive(Data::AnyMaterial&& material) override {
            if (!CanAccept(Data::kind_of(material))) {
                throw std::invalid_argument("Producer received material of non-compatible type");
            }
            inventory_.push(std::get<T>(std::move(material)));
        }

    protected:
        StepStatus OnProcess(const ProcessCommand& cmd) override {
            if (cmd.material_kind != T::kind) {
                throw std::invalid_argument(std::format("[PRODUCER] {} material material_kind mismatch for processing: got={} expected: {}\n",
                    Name(), Data::toString(cmd.material_kind), Data::toString(T::kind)));
            }
            if (inventory_.empty()) {
                std::cout << "[PRODUCER] " << Name() << " has no material of material_kind " << Data::toString(T::kind) << ". Retrying!" << std::endl;
                return RETRY;
            }
            auto item = &inventory_.front();
            try {
                ProcessOne(std::move(*item));
            } catch (std::exception& e) {
                std::cerr << "[PRODUCER] " << Name() << " failed to process material of material_kind " << Data::toString(T::kind) << " with error: " << e.what() << std::endl;
                return ERROR;
            }
            inventory_.pop();
            return SUCCESS;
        }

        // Optional helper for derived producers: store output material for later pickup.
        void StoreProduct(Data::AnyMaterial&& out) {
            outputs_.push(std::move(out));
        }

        virtual void ProcessOne(T&& item) = 0;

    private:
        std::queue<T> inventory_;
        std::queue<Data::AnyMaterial> outputs_;
    };
}

