#include "RawMaterial.hpp"

Factory::Data::RawMaterial::RawMaterial(std::string type, int weight, size_t buffer_size)
: type_(std::move(type)), weight_(weight), buffer_size_(buffer_size) {
    resource_buffer_ = new int[buffer_size_];
    std::fill_n(resource_buffer_, buffer_size_, 0);
    std::cout << "[Material] Created: " << type_ << std::endl;
}

Factory::Data::RawMaterial::~RawMaterial() {
    delete[] resource_buffer_;
}

Factory::Data::RawMaterial::RawMaterial(RawMaterial&& other) noexcept
    : type_(std::move(other.type_)),
    weight_(other.weight_),
    resource_buffer_(other.resource_buffer_),
    buffer_size_(other.buffer_size_) {
    other.resource_buffer_ = nullptr;
    other.buffer_size_ = 0;
    other.type_ = "Moved_Empty";
    std::cout << "[Material] Moved Constructed " << type_ << std::endl;
}

Factory::Data::RawMaterial &Factory::Data::RawMaterial::operator=(Factory::Data::RawMaterial &&other) noexcept {
    if (this != &other) {
        delete[] resource_buffer_;

        type_ = std::move(other.type_);
        weight_ = other.weight_;
        resource_buffer_ = other.resource_buffer_;
        buffer_size_ = other.buffer_size_;

        other.resource_buffer_ = nullptr;
        other.buffer_size_ = 0;
        other.type_ = "Moved_Empty";
    }
    std::cout << "[Material] Moved Assigned " << type_ << std::endl;
    return *this;
}


const std::string& Factory::Data::RawMaterial::getType() const {return type_;}
const int Factory::Data::RawMaterial::getWeight() const {return weight_;}
