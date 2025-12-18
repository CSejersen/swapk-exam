#pragma once
// Typed machine layer: enforces a single input type `T` and gives derived machines typed processing.
//
// Transport/routing can target `MachineBase` to allow destinations with different `T`.

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
        bool CanAccept(Data::MaterialKind kind) const override {
            return kind == T::kind;
        }

        bool TryReceive(Data::AnyMaterial&& material) override {
            if (auto* p = std::get_if<T>(&material)) {
                input_.push(std::move(*p));
                return true;
            }
            return false;
        }

    protected:
        bool OnProcess(const ProcessCommand& cmd) override {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (cmd.kind != T::kind) {
                std::cout << "[PRODUCER] " << Name() << " material kind mismatch for processing: got="
                          << Data::toString(cmd.kind) << " expected: " << Data::toString(T::kind) << "\n";
                return false;
            }
            if (input_.empty()) {
                std::cout << "[PRODUCER] " << Name() << " has no material of kind " << Data::toString(T::kind) << "\n";
                return false;
            }

            T item = std::move(input_.front());
            input_.pop();
            ProcessOne(std::move(item));
            return true;
        }

        // Optional helper for derived producers: store output material for later pickup.
        void Emit(Data::AnyMaterial&& out) {
            outputs_.push(std::move(out));
        }

        bool TryPopOutput(Data::MaterialKind kind, Data::AnyMaterial& out) {
            if (outputs_.empty()) return false;
            std::queue<Data::AnyMaterial> rest;
            bool found = false;

            while (!outputs_.empty()) {
                auto item = std::move(outputs_.front());
                outputs_.pop();
                if (!found && Data::kind_of(item) == kind) {
                    out = std::move(item);
                    found = true;
                    continue;
                }
                rest.push(std::move(item));
            }
            outputs_ = std::move(rest);
            return found;
        }

        virtual void ProcessOne(T&& item) = 0;

    private:
        std::queue<T> input_;
        std::queue<Data::AnyMaterial> outputs_;
    };
}

