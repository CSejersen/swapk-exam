#pragma once
#include "MachineBase.hpp"
#include "../../Shared.hpp"

#include <mutex>
#include <optional>

namespace Factory::Machinery {
    class ResourceStation : public MachineBase {
    private:
        std::unordered_map<Data::MaterialKind, std::queue<Data::AnyMaterial>> inventory_;
        mutable std::mutex inventoryMutex_;

    public:
        using MachineBase::MachineBase;

        // ResourceStation does not accept incoming materials
        void TryReceive(Data::AnyMaterial&& /*material*/) override {
            throw std::invalid_argument("ResourceStation does not accept materials");
        }

        bool CanAccept(Data::MaterialKind /*kind*/) const noexcept override {
            return false;
        }

        // Retrieve a material from inventory (thread-safe)
        std::optional<Data::AnyMaterial> TakeMaterial(Data::MaterialKind kind) override {
            std::lock_guard<std::mutex> lock(inventoryMutex_);
            auto it = inventory_.find(kind);
            if (it == inventory_.end() || it->second.empty()) {
                return std::nullopt;
            }
            auto material = std::move(it->second.front());
            it->second.pop();
            std::cout << "[RESOURCE_STATION] " << Name() << " dispensed " 
                      << Data::toString(kind) << std::endl;
            return material;
        }

        // Check if material is available (thread-safe)
        bool HasMaterial(Data::MaterialKind kind) const noexcept {
            std::lock_guard<std::mutex> lock(inventoryMutex_);
            auto it = inventory_.find(kind);
            return it != inventory_.end() && !it->second.empty();
        }

           StepStatus OnGenerate(const GenerateResourceCommand &c) override {
            std::lock_guard<std::mutex> lock(inventoryMutex_);
            switch(c.material_kind) {
                case Data::MaterialKind::MetalPipe: {
                    auto material = Data::MetalPipe{Data::DataBuffer(1024)};
                    inventory_[c.material_kind].emplace(std::move(material));
//                    std::cout << "[RESOURCE_STATION] " << Name() << " created MetalPipe" << std::endl;
                    break;
                }
                case Data::MaterialKind::Gravel: {
                    auto material = Data::Gravel{Data::DataBuffer(4096)};
                    inventory_[c.material_kind].emplace(std::move(material));
//                    std::cout << "[RESOURCE_STATION] " << Name() << " created Gravel" << std::endl;
                    break;
                }
                case Data::MaterialKind::TitaniumSlab: {
                    auto material = Data::TitaniumSlab{Data::DataBuffer(2048)};
                    inventory_[c.material_kind].emplace(std::move(material));
//                    std::cout << "[RESOURCE_STATION] " << Name() << " created TitaniumSlab" << std::endl;
                    break;
                }
                case Data::MaterialKind::MetalPipeHalf: {
                    throw std::invalid_argument("[RESOURCE_STATION] cannot create halved item, use the cutter");
                }
                default: {
                    throw std::invalid_argument("[RESOURCE_STATION] invalid material kind");
                }
            }
            return SUCCESS;
        }

    };
}


