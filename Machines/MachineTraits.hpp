#pragma once

#include "Core/MachineBase.hpp"
#include "Core/Mover.hpp"

#include <type_traits>

namespace Factory::Machinery {

    // Primary template: unknown machine type
    template<typename T, typename = void>
    struct MachineTraits {
        static constexpr bool is_mover = false;
        static constexpr bool is_producer = false;
    };

    // Explicit specialization for Mover
    template<>
    struct MachineTraits<Mover> {
        static constexpr bool is_mover = true;
        static constexpr bool is_producer = false;
    };

    // SFINAE specialization: detect any type that has EnqueueCommand(ProcessCommand)
    // This catches all Producer<T> derived types (like CNCMachine<T>)
    template<typename T>
    struct MachineTraits<T, std::void_t<
        decltype(std::declval<T>().EnqueueCommand(std::declval<ProcessCommand>()))
    >> {
        static constexpr bool is_mover = false;
        static constexpr bool is_producer = true;
    };

    // Helper variable templates for cleaner usage
    template<typename T>
    inline constexpr bool is_mover_v = MachineTraits<T>::is_mover;

    template<typename T>
    inline constexpr bool is_producer_v = MachineTraits<T>::is_producer;

} // namespace Factory::Machinery

