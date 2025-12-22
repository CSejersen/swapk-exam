#pragma once
#include "MachineBase.hpp"
#include "../../Shared.hpp"

namespace Factory::Machinery {
    class Mover : public MachineBase {
    public:
        using MachineBase::MachineBase;

        // Receiver API (via MachineBase)
        /**
         *
         * @param material
         * @throws std::runtime_error for all materials
         * guarantee: strong
         * no changes to machine state on throws
         */
        void TryReceive(Data::AnyMaterial&& material) override;
        bool CanAccept(Data::MaterialKind) const noexcept override { return false; }

    protected:
        StepStatus OnTransport(const TransportCommand& cmd) override;
    };
}
