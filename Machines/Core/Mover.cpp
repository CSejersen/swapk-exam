#include "Mover.hpp"
#include <exception>
#include <iostream>

#include "../../MachineConepts.hpp"

namespace Factory::Machinery{
    void Mover::TryReceive(Data::AnyMaterial&&) {
        throw std::runtime_error("Mover does not accept materials directly");
    }

    StepStatus Mover::OnTransport(const TransportCommand& cmd) {
        // Take material from the source
        auto material = cmd.source.TakeMaterial(cmd.material_kind);
        if (!material.has_value()) {
            std::cout << "[MOVER] " << Name() << " source " << cmd.source.Name() 
                      << " has no materials of kind: " << Data::toString(cmd.material_kind) << std::endl;
            return RETRY;
        }
        
        // Simulates time to move
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        try {
            cmd.destination.TryReceive(std::move(*material));
        } catch (std::exception& e) {
            std::cerr << "[MOVER] " << Name() << " the destination: " << cmd.destination.Name()
                      << " failed to receive material_kind=" << Data::toString(cmd.material_kind)
                      << " with error: " << e.what() << std::endl;
            throw;
        }
        std::cout << "[MOVER] " << Name() << " moved material_kind=" << Data::toString(cmd.material_kind) 
                  << " from " << cmd.source.Name() << " to " << cmd.destination.Name() << std::endl;
        return SUCCESS;
    }

}
