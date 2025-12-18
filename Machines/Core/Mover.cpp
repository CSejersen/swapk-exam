#include "Mover.hpp"
#include <exception>
#include <iostream>

namespace Factory::Machinery{
    bool Mover::TryReceive(Factory::Data::AnyMaterial&& material) {
        std::cout << "[MACHINE] " << Name() << " received material" << std::endl;
        inventory_.push(std::move(material));
        return true;
    }

    void Mover::OnTransport(const TransportCommand& cmd) {
        // Find first matching item in inventory
        if (inventory_.empty()) {
            std::cout << "[MOVER] " << Name() << " has no inventory\n";
            return;
        }

        std::queue<Factory::Data::AnyMaterial> rest;
        bool moved = false;

        while (!inventory_.empty()) {
            auto item = std::move(inventory_.front());
            inventory_.pop();

            if (!moved && Factory::Data::kind_of(item) == cmd.kind) {
                moved = cmd.destination.TryReceive(std::move(item));
                continue;
            }
            rest.push(std::move(item));
        }

        inventory_ = std::move(rest);

        if (!moved) {
            std::cout << "[MOVER] " << Name() << " could not move kind=" << Factory::Data::toString(cmd.kind) << "\n";
        }
    }

}