#pragma once
#include <vector>
#include <map>
#include "Machine.hpp"

class Controller {
public:
    Controller();
    void Start();
    std::vector<Factory::Machinery::Machine> machines;

private:
};



