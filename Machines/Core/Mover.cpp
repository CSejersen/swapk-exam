#include "Mover.hpp"
#include <exception>
#include <iostream>

#include "../../MachineConepts.hpp"

namespace Factory::Machinery{
    void Mover::TryReceive(Data::AnyMaterial&& material) {
        std::cout << "[MOVER] " << Name() << " received material" << std::endl;
        if (!CanAccept(Data::kind_of(material))) {
            throw std::invalid_argument("Mover received material of non-compatible type");
        }
        inventory_[Data::kind_of(material)].push(std::move(material));
    }

    StepStatus Mover::OnTransport(const TransportCommand& cmd) {
        // Find first matching item in inventory
        if (inventory_[cmd.material_kind].empty()) {
            std::cout << "[MOVER] " << Name() << " has no materials of kind: "<< toString(cmd.material_kind) << std::endl;
            return RETRY;
        }

        const auto it = &inventory_[cmd.material_kind].front();

        try {
            cmd.destination.TryReceive(std::move(*it));
        } catch (std::exception& e) {
            std::cerr << "[MOVER] " << Name() << " the destination: " << cmd.destination.Name()
                      << " failed to receive material_kind=" << Data::toString(cmd.material_kind)
                      << " with error: " << e.what() << std::endl;
            throw;
        }
        inventory_[cmd.material_kind].pop();
        std::cout << "[MOVER] " << Name() << " moved material_kind=" << Data::toString(cmd.material_kind) << " to " << cmd.destination.Name() << std::endl;
        return SUCCESS;
    }

}
