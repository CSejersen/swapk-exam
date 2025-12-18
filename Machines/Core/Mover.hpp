#pragma once
#include "MachineBase.hpp"
#include "../../Shared.hpp"

namespace Factory::Machinery {
    class Mover : public MachineBase {
    public:
        using MachineBase::MachineBase;

        // Receiver API (via MachineBase)
        void TryReceive(Data::AnyMaterial&& material) override;
        bool CanAccept(Data::MaterialKind) const noexcept override { return true; }

    protected:
        StepStatus OnTransport(const TransportCommand& cmd) override;

    private:
        std::unordered_map<Data::MaterialKind, std::queue<Data::AnyMaterial>> inventory_;
    };
}
