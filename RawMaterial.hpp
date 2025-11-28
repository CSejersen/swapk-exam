#pragma once

#include <cstdio>
#include <string>
#include <iostream>

constexpr int operator"" _kg(long double weight) {
    return static_cast<int>(weight * 1000);
}

constexpr int operator"" _g(long double weight) {
    return static_cast<int>(weight);
}

namespace Factory::Data {
    class RawMaterial {
    private:
        std::string type_;
        int weight_;
        int* resource_buffer_;
        size_t buffer_size_;

    public:
        RawMaterial(std::string type, int weight, size_t buffer_size);
        ~RawMaterial();

        RawMaterial(const RawMaterial&) = delete;
        RawMaterial& operator=(const RawMaterial&) = delete;
        RawMaterial(RawMaterial&& other) noexcept;
        RawMaterial& operator=(RawMaterial&& other) noexcept;

        const std::string& getType() const;
        const int getWeight() const;
    };
}