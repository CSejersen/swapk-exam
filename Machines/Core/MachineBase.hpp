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

namespace Factory::Machinery {

    class MachineBase;

    struct TransportCommand {
        Factory::Data::MaterialKind kind;
        MachineBase& destination;
    };

    struct ProcessCommand {
        Factory::Data::MaterialKind kind;
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

    protected:
        void Enqueue(Command cmd) {
            {
                std::lock_guard<std::mutex> lock(workMutex_);
                workQueue_.push(cmd);
            }
            std::cout << "[MACHINE] " << name_ << " added command to queue" << std::endl;
            workCondition_.notify_one();
        }

        virtual void OnTransport(const TransportCommand&) {}
        virtual void OnProcess(const ProcessCommand&) {}

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
                        std::cout << "[MACHINE] " << name_ << " received transport command" << " calling OnTransport()" << std::endl;
                        OnTransport(c);
                    } else if constexpr (std::is_same_v<C, ProcessCommand>) {
                        std::cout << "[MACHINE] " << name_ << " received process command" << " calling OnProcess()" << std::endl;
                        OnProcess(c);
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


