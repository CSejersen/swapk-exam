#pragma once

namespace Factory::Data {
    template<class T>
    concept Cuttable = requires(T t) {
        t.cutInHalf();
    };
}