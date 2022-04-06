#ifndef TASKER_H_
#define TASKER_H_

#include <algorithm>
#include <atomic>
#include <functional>
#include <future>
#include <thread>
#include <unordered_map>
#include <vector>

#include "concurrent_queue.h"

template <typename T, std::size_t N = 0>
class Tasker {
public:
    template <typename Function>
    Tasker(Function function)
        : function_ { std::move(function) }
    {
        taskers_.reserve(count_);
        for (std::size_t n = 0; n < count_; ++n)
            taskers_.emplace_back(std::async(std::launch::async, &Tasker::Run, this, n));
    }

    ~Tasker()
    {
        Stop();
    }

    void Stop()
    {
        for (auto& queue : queues_)
            queue.Stop();

        for (auto& tasker : taskers_) {
            if (tasker.valid())
                tasker.get();
        }
    }

    template <typename... Args>
    void Post(Args&&... args)
    {
        // Atomic modulo operation
        auto index = index_.load(std::memory_order_relaxed);
        auto next = std::size_t { 0 };

        do {
            next = (index + 1 == count_) ? 0 : index + 1;
        } while (!index_.compare_exchange_weak(index, next, std::memory_order_release, std::memory_order_relaxed));

        // Schedule task
        for (std::size_t n = 0; n < count_; ++n) {
            next = (index + n >= count_) ? (index + n - count_) : (index + n);
            if (queues_[next].TryEmplace(std::forward<Args>(args)...))
                return;
        }

        queues_[index].Emplace(std::forward<Args>(args)...);
    }

    void Clear()
    {
        for (auto& queue : queues_)
            queue.Clear();
    }

private:
    void Run(std::size_t index)
    {
        std::vector<std::reference_wrapper<ConcurrentQueue<T>>> queues_ref { std::begin(queues_), std::end(queues_) };

        std::rotate(std::begin(queues_ref), std::next(std::begin(queues_ref), index), std::end(queues_ref));

        while (true) {
            // Steal task
            for (auto& queue : queues_ref) {
                if (auto value = queue.get().TryPop()) {
                    function_(*std::move(value));
                    continue;
                }
            }

            if (auto value = queues_ref.front().get().Pop()) {
                function_(*std::move(value));
            } else {
                break;
            }
        }
    }

    std::size_t count_ { N != 0 ? N : std::max(1u, std::thread::hardware_concurrency()) };
    std::atomic_size_t index_ { 0 };
    std::function<void(T)> function_;
    std::vector<ConcurrentQueue<T>> queues_ { count_ };
    std::vector<std::future<void>> taskers_;
};

template <typename T>
class Tasker<T, 1> {
public:
    template <typename Function>
    Tasker(Function function)
        : function_ { std::move(function) }
        , tasker_ { std::async(std::launch::async, &Tasker::Run, this) }
    {
    }

    ~Tasker()
    {
        Stop();
    }

    void Stop()
    {
        queue_.Stop();

        if (tasker_.valid())
            tasker_.get();
    }

    template <typename... Args>
    void Post(Args&&... args)
    {
        queue_.Emplace(std::forward<Args>(args)...);
    }

    void Clear()
    {
        queue_.Clear();
    }

private:
    void Run()
    {
        auto wait_task = [this]() {
            if (auto value = queue_.Pop()) {
                function_(*std::move(value));
                return true;
            }

            return false;
        };

        while (wait_task()) { }
    }

    std::function<void(T)> function_;
    ConcurrentQueue<T> queue_;
    std::future<void> tasker_;
};

template <typename T>
using Looper = Tasker<T, 1>;

#endif // TASKER_H_