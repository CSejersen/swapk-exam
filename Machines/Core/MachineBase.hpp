#pragma once

#include "../../Materials/AnyMaterial.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>
#include <iostream>
#include <future>

namespace Factory::Machinery {

    class MachineBase;

    struct TransportCommand {
        Factory::Data::MaterialKind kind;
        MachineBase& destination;
        std::promise<bool> &cmdCompleted;
    };

    struct ProcessCommand {
        Factory::Data::MaterialKind kind;
        std::promise<bool> &cmdCompleted;
    };

    using Command = std::variant<TransportCommand, ProcessCommand>;

    class MachineBase {
    public:
        explicit MachineBase(std::string name)
            : name_(std::move(name)) {}

        virtual ~MachineBase() {
            StopThread();
        }

        std::string_view Name() const { return name_; }

        virtual bool TryReceive(Factory::Data::AnyMaterial&& material) = 0;
        virtual bool CanAccept(Factory::Data::MaterialKind kind) const = 0;

        void EmergencyStop() {
            shouldStop_.store(true);
            workCondition_.notify_all();
        }

        void DoMaintenance() {
            // Placeholder
        }

        void StartThread() {
            if (running_.exchange(true)) {
                return;
            }
            shouldStop_.store(false);
            workerThread_ = std::thread(&MachineBase::WorkerLoop, this);
        }

        void StopThread() {
            if (!running_.exchange(false)) {
                return;
            }
            shouldStop_.store(true);
            workCondition_.notify_all();
            if (workerThread_.joinable()) {
                workerThread_.join();
            }
        }

        void EnqueueCommand(Command const &cmd) {
            std::visit([this](const auto& c) {
                using C = std::decay_t<decltype(c)>;
                if constexpr (std::is_same_v<C, TransportCommand>) {
                    std::cout << "[MOVER] " << Name() << " enqueued transport command" << std::endl;
                } else if constexpr (std::is_same_v<C, ProcessCommand>) {
                    std::cout << "[PRODUCER] " << Name() << " enqueued process command" << std::endl;
                }
            }, cmd);
            {
                std::lock_guard<std::mutex> lock(workMutex_);
                workQueue_.push(cmd);
            }
            workCondition_.notify_one();
        }

    protected:

        virtual bool OnTransport(const TransportCommand&) {return false;}
        virtual bool OnProcess(const ProcessCommand&) {return false;}

    private:
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
                lock.unlock();

                std::visit([this](auto&& c) {
                    using C = std::decay_t<decltype(c)>;
                    if constexpr (std::is_same_v<C, TransportCommand>) {
                        std::cout << "[MOVER] " << Name() << " picking up transport command from queue"  << std::endl;
                        bool success = OnTransport(c);
                        c.cmdCompleted.set_value(success);
                    } else if constexpr (std::is_same_v<C, ProcessCommand>) {
                        std::cout << "[PRODUCER] " << name_ << " picking up process command from queue" << std::endl;
                        bool success = OnProcess(c);
                        c.cmdCompleted.set_value(success);
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


