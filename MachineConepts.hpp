#pragma once

namespace Factory::Data {
    template<class T>
    concept CNCCompatible = requires(T t) {
        t.cutInHalf();
    };
}