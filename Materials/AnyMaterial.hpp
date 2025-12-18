#pragma once

#include <string>
#include <variant>

namespace Factory::Data {

    enum class MaterialKind {
        MetalPipe,
        Gravel,
        TitaniumSlab,
        MetalPipeHalf,
        Invalid,
    };

    inline std::string toString(MaterialKind kind) {
        switch (kind) {
            case MaterialKind::MetalPipe: return "MetalPipe";
            case MaterialKind::Gravel: return "Gravel";
            case MaterialKind::TitaniumSlab: return "TitaniumSlab";
            case MaterialKind::MetalPipeHalf: return "MetalPipeHalf";
            case MaterialKind::Invalid: return "Invalid";
        }
        return "Unknown";
    }

    struct MetalPipeHalf {
        static constexpr MaterialKind kind = MaterialKind::MetalPipeHalf;
    };

    struct MetalPipe {
        static constexpr MaterialKind kind = MaterialKind::MetalPipe;

        MetalPipeHalf cutInHalf() {
            return {};
        }
    };

    struct Gravel {
        static constexpr MaterialKind kind = MaterialKind::Gravel;
    };

    struct TitaniumSlab {
        static constexpr MaterialKind kind = MaterialKind::TitaniumSlab;

        bool requiresCoolant() const { return true; }
    };

    using AnyMaterial = std::variant<MetalPipe, Gravel, TitaniumSlab, MetalPipeHalf>;

    inline MaterialKind kind_of(const AnyMaterial& m) {
        return std::visit([](const auto& v) { return std::decay_t<decltype(v)>::kind; }, m);
    }
}


