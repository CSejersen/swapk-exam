#pragma once

#include "../../Materials/AnyMaterial.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <variant>
#include <iostream>
#include <future>
#include "../../Shared.hpp"

namespace Factory::Machinery {

    class MachineBase;

    struct TransportCommand {
        Data::MaterialKind material_kind;
        MachineBase& source;
        MachineBase& destination;
        std::promise<StepStatus> &cmdCompleted;
    };

    struct ProcessCommand {
        Data::MaterialKind material_kind;
        std::promise<StepStatus> &cmdCompleted;
    };

    struct GenerateResourceCommand {
        Data::MaterialKind material_kind;
    };

    using Command = std::variant<TransportCommand, ProcessCommand, GenerateResourceCommand>;

    class MachineBase {
    public:
        explicit MachineBase(std::string name) noexcept
            : name_(std::move(name)) {}

        virtual ~MachineBase() {
            StopThread();
        }

        // Rule of 5: non-copyable and non-movable due to thread and synchronization primitives
        MachineBase(const MachineBase&) = delete;
        MachineBase& operator=(const MachineBase&) = delete;
        MachineBase(MachineBase&&) = delete;
        MachineBase& operator=(MachineBase&&) = delete;

        std::string_view Name() const noexcept { return name_; };

        /**
         * Attempts to deliver material to this machine.
         * @throws std::invalid_argument if material type is not compatible
         * @param material
         * Exception guarantee: strong
         * No changes to inventory on throws.
         */
        virtual void TryReceive(Data::AnyMaterial&& material) = 0;
        virtual bool CanAccept(Data::MaterialKind kind) const noexcept = 0;

        /**
         * Attempts to take a material of the specified kind from this machine.
         * Override in machines that can provide materials (e.g., ResourceStation).
         * @return The material if available, std::nullopt otherwise
         */
        virtual std::optional<Data::AnyMaterial> TakeMaterial(Data::MaterialKind) {
            return std::nullopt;
        }

        void EmergencyStop() noexcept{
            shouldStop_.store(true);
            workCondition_.notify_all();
        }

        void DoMaintenance() {
            // Placeholder
        }

        /** Starts the internal worker thread. No-op if already running.
         *
         * @throws std::system_error if thread creation fails
         * Exception guarantee: strong
         * Running flag remains false on throws.
         */
        void StartThread() {
            if (running_.exchange(true)) {
                return;
            }
            shouldStop_.store(false);
            try {
                workerThread_ = std::thread(&MachineBase::WorkerLoop, this);
            } catch (...) {
                running_.store(false);
                throw;
            }

        }

        /** Stops the internal worker thread. No-op if not running.
         *
         * Exception guarantee: no-throw
         */
        void StopThread() noexcept {
            if (!running_.exchange(false)) {
                return;
            }
            shouldStop_.store(true);
            workCondition_.notify_all();
            if (workerThread_.joinable()) {
                try {
                    workerThread_.join();
                } catch (...) {
                    std::cerr << "[ERROR] Failed to join worker thread for machine: " << Name() << std::endl;
                }

            }
        }
        /** Enqueues a command for processing by the internal worker thread.
         *
         * @throws std::exception on enqueue failure
         * Exception guarantee: strong
         * No changes to work queue on throws.
         */
        void EnqueueCommand(Command const &cmd) {
            try {
                std::visit([this](const auto& c) {
                using C = std::decay_t<decltype(c)>;
                if constexpr (std::is_same_v<C, TransportCommand>) {
                    std::cout << "[MOVER] " << Name() << " enqueued transport command" << std::endl;
                } else if constexpr (std::is_same_v<C, ProcessCommand>) {
                    std::cout << "[PRODUCER] " << Name() << " enqueued process command" << std::endl;
                }
            }, cmd);
            } catch (...) {
                std::cout << "[MACHINE] Failed to deduce machine type proceeding to enqueue command" << std::endl;
            }
            {
                try {
                    std::lock_guard<std::mutex> lock(workMutex_);
                    workQueue_.push(cmd);
                } catch (std::exception &e) {
                    std::cerr << "[ERROR] Failed to enqueue command with error: " << e.what() << std::endl;
                    throw;
                }

            }
            workCondition_.notify_one();
        }

    protected:

        /** Override to handle transport commands.
         *
         * @param cmd TransportCommand to process
         * @return true on success, false on failure
         * Exception guarantee: strong
         * No changes to machine state on throws.
         */
        virtual StepStatus OnTransport(const TransportCommand&) {return ERROR;}

        /** Override to handle process commands.
         *
         * @param cmd ProcessCommand to process
         * @return true on success, false on failure
         * @throws std::invalid_argument if the material is not compatible
         * Exception guarantee: strong
         * No changes to machine state on throws.
         */
        virtual StepStatus OnProcess(const ProcessCommand&) {return ERROR;}

        /** Override to handle resource generation commands.
         *
         * @param cmd GenerateResourceCommand to process
         * @return StepStatus indicating success or failure
         * Exception guarantee: strong
         * No changes to inventory on throws.
         */
        virtual StepStatus OnGenerate(const GenerateResourceCommand&) {return ERROR;}

    private:
        /** Internal worker loop processing commands from the queue.
         *
         * Exception guarantee: Basic
         * Commands may be lost if the unique_lock throws on unlock.
         */
        void WorkerLoop() {
            while (!shouldStop_.load()) {
                std::unique_lock<std::mutex> lock(workMutex_);
                workCondition_.wait(lock, [this] {
                    return shouldStop_.load() || !workQueue_.empty();
                });
                if (shouldStop_.load()) {
                    break;
                }

                Command cmd = std::move(workQueue_.front());
                workQueue_.pop();
                lock.unlock(); // if this throws the command is lost. But this should never happen as mutex gets locked above.

                std::visit([this](auto&& c) {
                    using C = std::decay_t<decltype(c)>;
                    StepStatus success;
                    if constexpr (std::is_same_v<C, TransportCommand>) {
                        std::cout << "[MOVER] " << Name() << " picking up transport command from queue"  << std::endl;
                        try {
                            success = OnTransport(c);
                        } catch (std::exception &e) {
                            std::cerr << "[ERROR] Failed to execute the transport command with error: " << e.what() << std::endl;
                            success = ERROR;
                        }
                        c.cmdCompleted.set_value(success);
                    } else if constexpr (std::is_same_v<C, ProcessCommand>) {
                        std::cout << "[PRODUCER] " << name_ << " picking up process command from queue" << std::endl;
                        try {
                            success = OnProcess(c);
                        } catch (std::exception &e) {
                            std::cerr << "[ERROR] Failed to execute the process command with error: " << e.what() << std::endl;
                            success = ERROR;
                        }
                        c.cmdCompleted.set_value(success);
                    } else if constexpr (std::is_same_v<C, GenerateResourceCommand>) {
                        try {
                            OnGenerate(c);
                        } catch (std::exception &e) {
                            std::cerr << "[ERROR] Failed to execute the generate_material command with error: " << e.what() << std::endl;
                        }
                    }
                }, cmd);
            }
        }

        std::string name_;
        std::thread workerThread_;
        std::mutex workMutex_;
        std::condition_variable workCondition_;
        std::queue<Command> workQueue_;
        std::atomic_bool shouldStop_{false};
        std::atomic_bool running_{false};
    };
}


