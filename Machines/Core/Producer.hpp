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
        { T::kind } -> std::convertible_to<Factory::Data::MaterialKind>;
    };

    template<HasMaterialKind T>
    class Producer : public MachineBase {
    public:
        using MachineBase::MachineBase;

        // Receiver API (via MachineBase)
        bool CanAccept(Factory::Data::MaterialKind kind) const override {
            return kind == T::kind;
        }

        bool TryReceive(Factory::Data::AnyMaterial&& material) override {
            if (auto* p = std::get_if<T>(&material)) {
                input_.push(std::move(*p));
                return true;
            }
            return false;
        }

        void EnqueueCommand(ProcessCommand command) {
            Enqueue(std::move(command));
        }

    protected:
        void OnProcess(const ProcessCommand& cmd) override {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (cmd.kind != T::kind) {
                std::cout << "[MACHINE] " << Name() << " ignoring process request for kind="
                          << Factory::Data::toString(cmd.kind) << "\n";
                return;
            }
            if (input_.empty()) {
                std::cout << "[MACHINE] " << Name() << " has no input for kind=" << Factory::Data::toString(T::kind) << "\n";
                return;
            }

            T item = std::move(input_.front());
            input_.pop();
            ProcessOne(std::move(item));
        }

        // Optional helper for derived producers: store output material for later pickup.
        void Emit(Factory::Data::AnyMaterial&& out) {
            outputs_.push(std::move(out));
        }

        bool TryPopOutput(Factory::Data::MaterialKind kind, Factory::Data::AnyMaterial& out) {
            if (outputs_.empty()) return false;
            std::queue<Factory::Data::AnyMaterial> rest;
            bool found = false;

            while (!outputs_.empty()) {
                auto item = std::move(outputs_.front());
                outputs_.pop();
                if (!found && Factory::Data::kind_of(item) == kind) {
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
        std::queue<Factory::Data::AnyMaterial> outputs_;
    };
}

