#pragma once

#include <iostream>
#include <string>
#include <variant>

namespace Factory::Data {

    class DataBuffer {
    private:
        char* data_ = nullptr;
        size_t size_ = 0;

    public:
        DataBuffer(size_t size) : data_(new char[size]), size_(size) {}
        ~DataBuffer() { delete[] data_; }

        DataBuffer(const DataBuffer&) = delete;
        DataBuffer& operator=(const DataBuffer&) = delete;

        DataBuffer(DataBuffer&& other) noexcept : data_(other.data_), size_(other.size_) {
            other.data_ = nullptr;
            other.size_ = 0;
        }

        DataBuffer& operator=(DataBuffer&& other) noexcept {
            if (this != &other) {
                delete[] data_;
                data_ = other.data_;
                size_ = other.size_;
                other.data_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }
    };

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
        DataBuffer data;
        static constexpr MaterialKind kind = MaterialKind::MetalPipeHalf;
    };

    struct MetalPipe {
        DataBuffer data;
        static constexpr MaterialKind kind = MaterialKind::MetalPipe;

        MetalPipeHalf cutInHalf() {
            return MetalPipeHalf{Data::DataBuffer{512}};
        }
    };

    struct Gravel {
        DataBuffer data;
        static constexpr MaterialKind kind = MaterialKind::Gravel;
    };

    struct TitaniumSlab {
        DataBuffer data;
        static constexpr MaterialKind kind = MaterialKind::TitaniumSlab;

        bool requiresCoolant() const { return true; }
    };

    using AnyMaterial = std::variant<MetalPipe, Gravel, TitaniumSlab, MetalPipeHalf>;

    inline MaterialKind kind_of(const AnyMaterial& m) {
        return std::visit([](const auto& v) { return std::decay_t<decltype(v)>::kind; }, m);
    }
}


