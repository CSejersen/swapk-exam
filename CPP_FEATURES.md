# C++ Features Documentation

This document catalogs the C++ features demonstrated in the Factory Machinery codebase, referencing the best examples for each topic from the curriculum.

---

## Namespaces

The codebase uses nested namespaces to organize the factory simulation into logical domains.

**Best Example: `Factory::Machinery` and `Factory::Data`**

```cpp
// From Materials/AnyMaterial.hpp
namespace Factory::Data {
    enum class MaterialKind { ... };
    struct MetalPipe { ... };
    using AnyMaterial = std::variant<MetalPipe, Gravel, TitaniumSlab, MetalPipeHalf>;
}

// From Machines/Core/MachineBase.hpp
namespace Factory::Machinery {
    class MachineBase { ... };
}
```

The codebase demonstrates C++17 nested namespace syntax (`Factory::Machinery` instead of `namespace Factory { namespace Machinery { ... }}`). The namespaces provide a clear separation: `Factory::Data` contains material types and their metadata, while `Factory::Machinery` contains all machine-related classes. This organization prevents name collisions and makes the codebase's architecture immediately apparent from the namespace structure.

---

## Move Semantics & Rule of 5 (Manual Resource Management)

Move semantics are used throughout the codebase to transfer ownership of materials and commands without unnecessary copies.

**Best Example: Material transfer in `Mover::TryReceive` and `Mover::OnTransport`**

```cpp
// From Machines/Core/Mover.cpp
bool Mover::TryReceive(Factory::Data::AnyMaterial&& material) {
    std::cout << "[MOVER] " << Name() << " received material" << std::endl;
    inventory_.push(std::move(material));
    return true;
}

bool Mover::OnTransport(const TransportCommand& cmd) {
    // ...
    while (!inventory_.empty()) {
        auto item = std::move(inventory_.front());
        inventory_.pop();

        if (!moved && Factory::Data::kind_of(item) == cmd.kind) {
            moved = cmd.destination.TryReceive(std::move(item));
            continue;
        }
        rest.push(std::move(item));
    }
    inventory_ = std::move(rest);
    // ...
}
```

The `TryReceive` function takes an rvalue reference (`AnyMaterial&&`) signaling that it intends to take ownership of the material. The `OnTransport` function demonstrates proper move semantics when transferring items between queues—using `std::move` to avoid copying potentially large material objects.

**Rule of 5 Consideration:**

```cpp
// From Machines/Core/MachineBase.hpp
class MachineBase {
public:
    explicit MachineBase(std::string name) : name_(std::move(name)) {}
    virtual ~MachineBase() { StopThread(); }
    // ...
private:
    std::thread workerThread_;
    std::mutex workMutex_;
    // ...
};
```

`MachineBase` has a custom destructor that calls `StopThread()` to ensure the worker thread is properly joined before destruction. However, **the class does not explicitly delete or define the copy/move constructors and assignment operators**. Since `std::thread` and `std::mutex` are not copyable, the compiler will implicitly delete the copy operations, but move operations will also be deleted because of the user-defined destructor. For a production codebase, we should explicitly follow the Rule of 5:

```cpp
// What could be added for completeness:
MachineBase(const MachineBase&) = delete;
MachineBase& operator=(const MachineBase&) = delete;
MachineBase(MachineBase&&) = delete;
MachineBase& operator=(MachineBase&&) = delete;
```

This would make the intent explicit and prevent accidental misuse.

---

## Concepts & Constraints

C++20 concepts are used to constrain template parameters, providing clear compile-time requirements.

**Best Example: `CNCCompatible` concept and `HasMaterialKind` concept**

```cpp
// From MachineConepts.hpp
namespace Factory::Data {
    template<class T>
    concept CNCCompatible = requires(T t) {
        t.cutInHalf();
    };
}

// From Machines/Core/Producer.hpp
template<class T>
concept HasMaterialKind = requires {
    { T::kind } -> std::convertible_to<Data::MaterialKind>;
};

template<HasMaterialKind T>
class Producer : public MachineBase { ... };
```

The `CNCCompatible` concept enforces that any material type used with `CNCMachine` must have a `cutInHalf()` method. The `HasMaterialKind` concept ensures that material types have a static `kind` member that is convertible to `MaterialKind`.

**Usage in CNCMachine:**

```cpp
// From Machines/CNCMachine.hpp
template<Data::CNCCompatible T>
class CNCMachine : public Producer<T> {
private:
    void ProcessOne(T&& item) override {
        // We can call CNC-only operations, guaranteed by the concept.
        auto out = item.cutInHalf();
        this->Emit(Factory::Data::AnyMaterial{std::move(out)});
    }
};
```

By constraining `CNCMachine` with `CNCCompatible`, the compiler guarantees at instantiation time that `item.cutInHalf()` is a valid call. This provides better error messages than traditional SFINAE and documents the requirements directly in the template signature.

---

## Tagging, Traits, & SFINAE

The codebase uses type traits with SFINAE to enable compile-time dispatch based on machine types.

**Best Example: `MachineTraits` for compile-time machine categorization**

```cpp
// From Machines/MachineTraits.hpp
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
}
```

**Usage in Controller:**

```cpp
// From Controller.hpp
template<typename MachineT, typename... Args>
MachineT& AddMachine(Args&&... args) {
    auto machine = std::make_unique<MachineT>(std::forward<Args>(args)...);
    MachineT* ptr = machine.get();

    // Compile-time dispatch based on machine type traits
    if constexpr (Machinery::is_mover_v<MachineT>) {
        ConnectMoverSignal(ptr);
    } else if constexpr (Machinery::is_producer_v<MachineT>) {
        ConnectProducerSignal(ptr);
    }
    // ...
}
```

This demonstrates the classic SFINAE pattern using `std::void_t` to detect whether a type has a particular member function. The `MachineTraits` template provides a unified interface for querying machine capabilities at compile time.

**Note on usage for demonstration purposes:** The SFINAE-based detection for producers (checking for `EnqueueCommand(ProcessCommand)`) is somewhat fragile because `Mover` also inherits `EnqueueCommand` from `MachineBase`. It works in this codebase because the explicit `Mover` specialization takes precedence. A cleaner approach might be to use a tag type or base class check:

```cpp
// Alternative approach using inheritance detection:
template<typename T>
inline constexpr bool is_producer_v = std::is_base_of_v<ProducerBase, T>;
```

---

## Literals

**Note:** This codebase does not demonstrate **user-defined literals**. The codebase uses standard library literals such as:

```cpp
// From Controller.cpp and main.cpp
std::this_thread::sleep_for(std::chrono::milliseconds(500));
std::this_thread::sleep_for(std::chrono::milliseconds(100));
```

For a complete demonstration of user-defined literals, one could add custom literals for domain-specific concepts. For example:

```cpp
// Example of what could be added:
namespace Factory::Literals {
    constexpr Data::MaterialKind operator""_material(const char* name, size_t) {
        // Parse material name...
    }
    
    // Or for quantities:
    constexpr int operator""_kg(unsigned long long weight) {
        return static_cast<int>(weight);
    }
}

// Usage: auto mat = "MetalPipe"_material;
// Usage: auto weight = 50_kg;
```

This feature could be incorporated into the `AnyMaterial` module for more expressive material specifications.

---

## PMR (Polymorphic Memory Resources)

**Note:** This codebase does not currently demonstrate **Polymorphic Memory Resources (PMR)**. The codebase uses standard `std::queue` containers with the default allocator.

PMR could be beneficial in this codebase for scenarios like:

```cpp
// Example of what could be added:
#include <memory_resource>

class MachineBase {
    // Use a monotonic buffer for command queue to avoid heap fragmentation
    std::array<std::byte, 4096> buffer_;
    std::pmr::monotonic_buffer_resource pool_{buffer_.data(), buffer_.size()};
    std::pmr::queue<Command> workQueue_{&pool_};
};
```

This would be particularly useful in a real-time factory control system where predictable memory allocation is important. The monotonic buffer resource provides fast, deterministic allocation for the command queue, with all memory coming from a pre-allocated buffer.

---

## Threading, Synchronization & Futures

The codebase demonstrates a comprehensive threading model with worker threads, synchronization primitives, and future-based async completion.

**Best Example: Worker thread pattern in `MachineBase`**

```cpp
// From Machines/Core/MachineBase.hpp
class MachineBase {
public:
    void StartThread() {
        if (running_.exchange(true)) {
            return;  // Already running
        }
        shouldStop_.store(false);
        workerThread_ = std::thread(&MachineBase::WorkerLoop, this);
    }

    void StopThread() {
        if (!running_.exchange(false)) {
            return;  // Not running
        }
        shouldStop_.store(true);
        workCondition_.notify_all();
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }

private:
    void WorkerLoop() {
        while (!shouldStop_.load()) {
            std::unique_lock<std::mutex> lock(workMutex_);
            workCondition_.wait(lock, [this] {
                return shouldStop_.load() || !workQueue_.empty();
            });
            if (shouldStop_.load()) break;

            Command cmd = std::move(workQueue_.front());
            workQueue_.pop();
            lock.unlock();

            // Process command...
        }
    }

    std::thread workerThread_;
    std::mutex workMutex_;
    std::condition_variable workCondition_;
    std::atomic_bool shouldStop_{false};
    std::atomic_bool running_{false};
};
```

This demonstrates:
- **`std::thread`**: Each machine runs its own worker thread
- **`std::mutex` + `std::unique_lock`**: Protects the work queue
- **`std::condition_variable`**: Efficient wait/notify pattern for the producer-consumer queue
- **`std::atomic_bool`**: Lock-free flags for thread control

**Best Example: Promise/Future for async command completion**

```cpp
// From Controller.cpp
void Controller::executeJob(Job job) {
    while (!job.stepsEmpty()) {
        const auto& currentStep = job.getNextStep();
        std::promise<bool> promise;
        auto future = promise.get_future();

        executeJobStep(currentStep, promise);
        future.wait();

        bool success = future.get();
        // Handle success/failure...
    }
}

// From Machines/Core/MachineBase.hpp (WorkerLoop)
std::visit([this](auto&& c) {
    using C = std::decay_t<decltype(c)>;
    if constexpr (std::is_same_v<C, ProcessCommand>) {
        bool success = OnProcess(c);
        c.cmdCompleted.set_value(success);  // Fulfill the promise
    }
}, cmd);
```

The `std::promise<bool>` is passed with the command and fulfilled by the worker thread when processing completes. The controller waits on the corresponding `std::future` to synchronize with command completion. This pattern allows the controller to coordinate multi-step jobs across asynchronous machine operations.

---

## STD Variant & Visit

`std::variant` is used extensively as a type-safe union for commands, job steps, and materials.

**Best Example: Command variant with `std::visit` dispatch**

```cpp
// From Machines/Core/MachineBase.hpp
struct TransportCommand {
    Factory::Data::MaterialKind kind;
    MachineBase& destination;
    std::promise<bool>& cmdCompleted;
};

struct ProcessCommand {
    Factory::Data::MaterialKind kind;
    std::promise<bool>& cmdCompleted;
};

using Command = std::variant<TransportCommand, ProcessCommand>;
```

**Compile-time dispatch with `std::visit` and `if constexpr`:**

```cpp
// From Machines/Core/MachineBase.hpp
void EnqueueCommand(Command const& cmd) {
    std::visit([this](const auto& c) {
        using C = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<C, TransportCommand>) {
            std::cout << "[MOVER] " << Name() << " enqueued transport command\n";
        } else if constexpr (std::is_same_v<C, ProcessCommand>) {
            std::cout << "[PRODUCER] " << Name() << " enqueued process command\n";
        }
    }, cmd);
    // ...
}
```

**Material variant with runtime kind detection:**

```cpp
// From Materials/AnyMaterial.hpp
using AnyMaterial = std::variant<MetalPipe, Gravel, TitaniumSlab, MetalPipeHalf>;

inline MaterialKind kind_of(const AnyMaterial& m) {
    return std::visit([](const auto& v) { 
        return std::decay_t<decltype(v)>::kind; 
    }, m);
}
```

The `kind_of` function demonstrates using `std::visit` with a generic lambda to extract the `kind` static member from whatever type is currently held in the variant. The `std::decay_t<decltype(v)>` pattern is used to get the actual type without reference/const qualifiers.

**JobStep variant for polymorphic steps:**

```cpp
// From Job.hpp
using JobStep = std::variant<MoveStep, ProcessStep>;

// From Controller.cpp
void Controller::executeJobStep(const Factory::JobStep& step, std::promise<bool>& promise) {
    std::visit([this, &promise](const auto& s) {
        using S = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<S, Factory::MoveStep>) {
            onTransportRequested(&s.mover.get(), s.material, s.destination.get(), promise);
        } else if constexpr (std::is_same_v<S, Factory::ProcessStep>) {
            onProcessRequested(s.material, s.executor.get(), promise);
        }
    }, step);
}
```

This pattern replaces traditional OOP polymorphism (virtual functions) with compile-time dispatch, providing type safety and avoiding the overhead of virtual function calls.

---

## Exceptions & Guarantees

The codebase uses exceptions for error handling in specific scenarios.

**Best Example: `Job::getNextStep()` throwing on empty queue**

```cpp
// From Job.cpp
const JobStep& Job::getNextStep() const {
    if (steps_.empty()) {
        throw std::out_of_range("no steps left in the job");
    }
    return steps_.front();
}
```

This demonstrates using a standard exception type (`std::out_of_range`) for a precondition violation. The exception provides a clear error message and follows the principle of failing fast when invariants are broken.

**Exception Safety Analysis:**

The codebase could be improved with more explicit exception guarantees. For example:

```cpp
// From Machines/Core/Mover.cpp
bool Mover::OnTransport(const TransportCommand& cmd) {
    std::queue<Factory::Data::AnyMaterial> rest;
    bool moved = false;

    while (!inventory_.empty()) {
        auto item = std::move(inventory_.front());
        inventory_.pop();
        // If TryReceive throws, 'item' is lost!
        if (!moved && Factory::Data::kind_of(item) == cmd.kind) {
            moved = cmd.destination.TryReceive(std::move(item));
            continue;
        }
        rest.push(std::move(item));
    }

    inventory_ = std::move(rest);  // Strong guarantee here
    return moved;
}
```

**Note on usage for demonstration purposes:** The current exception handling is minimal. For a production system, we would want to:

1. **Document exception guarantees** (no-throw, basic, strong) for each function
2. **Use RAII** more extensively to ensure cleanup on exceptions
3. **Consider `noexcept`** specifications for move operations and destructors

For example, the destructor and move operations should be `noexcept`:

```cpp
virtual ~MachineBase() noexcept {
    StopThread();  // Should also be noexcept
}
```

And functions that cannot fail should be marked:

```cpp
std::string_view Name() const noexcept { return name_; }
bool stepsEmpty() const noexcept { return steps_.empty(); }
```

---

## Summary

| Feature | Demonstrated | Notes |
|---------|--------------|-------|
| Namespaces | ✅ Yes | C++17 nested namespace syntax |
| Move Semantics & Rule of 5 | ✅ Partial | Move semantics used; Rule of 5 not explicitly followed |
| Concepts & Constraints | ✅ Yes | `CNCCompatible`, `HasMaterialKind` concepts |
| Tagging, Traits, & SFINAE | ✅ Yes | `MachineTraits` with `std::void_t` SFINAE |
| Literals | ❌ No | Only standard library literals used |
| PMR | ❌ No | Could benefit command queues |
| Threading & Synchronization | ✅ Yes | Full worker thread pattern with futures |
| STD Variant & Visit | ✅ Yes | Extensive use for type-safe unions |
| Exceptions & Guarantees | ✅ Partial | Basic exceptions; guarantees not documented |

