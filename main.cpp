#include <iostream>
#include "RawMaterial.hpp"

int main() {
    auto material = Factory::Data::RawMaterial("Titanium_slab", 100.0_kg, 1024);
    auto material2 = Factory::Data::RawMaterial(std::move(material));
    auto material3 = Factory::Data::RawMaterial("otherstuff", 100.0_kg, 1024);
    material3 = std::move(material2);
}
