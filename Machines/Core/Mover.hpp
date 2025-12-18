#pragma once
#include "MachineBase.hpp"

namespace Factory::Machinery {
    class Mover : public MachineBase {
    public:
        using MachineBase::MachineBase;

        // Receiver API (via MachineBase)
        bool TryReceive(Factory::Data::AnyMaterial&& material) override;
        bool CanAccept(Factory::Data::MaterialKind) const override { return true; }

        // Mover-specific public API for scheduling work
        void EnqueueCommand(TransportCommand command) {
            Enqueue(std::move(command));
        }

    protected:
        void OnTransport(const TransportCommand& cmd) override;

    private:
        std::queue<Factory::Data::AnyMaterial> inventory_;
    };
}
