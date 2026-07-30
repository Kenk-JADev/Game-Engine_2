/**
 * @file thread_pool.cpp
 * @brief Worker-Pool-Implementierung.
 */
#include <aether/core/thread_pool.hpp>
#include <aether/core/logger.hpp>

#include <stdexcept>
#include <string>

namespace aether::core {
namespace {

u32 resolve_thread_count(u32 requested) {
    if (requested > 0) {
        return requested;
    }
    const unsigned hc = std::thread::hardware_concurrency();
    if (hc <= 1) {
        return 1;
    }
    return static_cast<u32>(hc - 1);
}

} // namespace

ThreadPool::ThreadPool(u32 thread_count) {
    const u32 n = resolve_thread_count(thread_count);
    workers_.reserve(n);
    for (u32 i = 0; i < n; ++i) {
        workers_.emplace_back([this, i] { worker_loop(i); });
    }
    log_info("ThreadPool", "Started with " + std::to_string(n) + " worker(s)");
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::worker_loop(u32 /*index*/) {
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
            ++active_;
        }

        try {
            task();
        } catch (const std::exception& ex) {
            log_error("ThreadPool", std::string("Task exception: ") + ex.what());
        } catch (...) {
            log_error("ThreadPool", "Task exception: unknown");
        }

        {
            std::lock_guard lock(mutex_);
            --active_;
            if (tasks_.empty() && active_ == 0) {
                idle_cv_.notify_all();
            }
        }
    }
}

usize ThreadPool::pending_tasks() const {
    std::lock_guard lock(mutex_);
    return tasks_.size();
}

void ThreadPool::wait_idle() {
    std::unique_lock lock(mutex_);
    idle_cv_.wait(lock, [this] { return tasks_.empty() && active_ == 0; });
}

void ThreadPool::shutdown() {
    {
        std::lock_guard lock(mutex_);
        if (stopping_) {
            return;
        }
        stopping_ = true;
    }
    cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) {
            w.join();
        }
    }
    workers_.clear();
    log_info("ThreadPool", "Shutdown complete");
}

} // namespace aether::core
