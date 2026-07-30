/**
 * @file thread_pool.hpp
 * @brief Fester Worker-Pool für Asset-Loading und Hintergrundarbeit.
 */
#pragma once

#include <aether/core/types.hpp>

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace aether::core {

/**
 * @brief Einfacher, RAII-basierter Thread-Pool.
 *
 * Tasks sollten keine OpenGL-Aufrufe tätigen (GL-Kontext ist Main-Thread).
 */
class ThreadPool : public NonMovable {
public:
    /**
     * @param thread_count Anzahl Worker. 0 → max(1, hardware_concurrency-1)
     */
    explicit ThreadPool(u32 thread_count = 0);
    ~ThreadPool();

    /**
     * @brief Stellt eine Task in die Warteschlange.
     * @return std::future mit dem Rückgabewert der Funktion
     */
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using R = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<R()>>(
            [fn = std::forward<F>(f),
             tup = std::make_tuple(std::forward<Args>(args)...)]() mutable -> R {
                return std::apply(std::move(fn), std::move(tup));
            });

        std::future<R> future = task->get_future();
        {
            std::lock_guard lock(mutex_);
            if (stopping_) {
                throw std::runtime_error("ThreadPool: submit on stopped pool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return future;
    }

    /** @brief Anzahl Worker-Threads. */
    [[nodiscard]] u32 thread_count() const noexcept { return static_cast<u32>(workers_.size()); }

    /** @brief Grobe Schätzung offener Tasks. */
    [[nodiscard]] usize pending_tasks() const;

    /** @brief Blockiert bis die Queue leer ist (laufende Tasks enden). */
    void wait_idle();

    /** @brief Stoppt Worker nach Abschluss aktueller Tasks. */
    void shutdown();

private:
    void worker_loop(u32 index);

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable idle_cv_;
    bool stopping_ = false;
    u32 active_ = 0;
};

} // namespace aether::core
