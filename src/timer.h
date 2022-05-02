#ifndef TIMER_H_
#define TIMER_H_

#include <cassert>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "message_router.h"
#include "singleton.h"

class Timer : public Singleton<Timer> {
public:
    Timer()
        : tick_ { 0 }
        , done_ { false }
        , task_ { std::async(std::launch::async, &Timer::Run, this) }
    {
    }

    ~Timer()
    {
        done_.store(true, std::memory_order_relaxed);

        if (task_.valid())
            task_.get();
    }

    template <typename... Args>
    void Register(Args&&... args)
    {
        const auto tick = tick_.load(std::memory_order_relaxed);
        std::lock_guard lock { mutex_ };

        schedule_.emplace_back(std::forward<Args>(args)..., tick);
    }

    void Unregister(std::string_view handler_id)
    {
        std::lock_guard lock { mutex_ };

        schedule_.erase(
            std::remove_if(
                std::begin(schedule_),
                std::end(schedule_),
                [handler_id](const auto& item) { return item.handler_id == handler_id; }),
            std::end(schedule_));
    }

    void Unregister(std::string_view handler_id, std::uint16_t message_id)
    {
        std::lock_guard lock { mutex_ };

        schedule_.erase(
            std::remove_if(
                std::begin(schedule_),
                std::end(schedule_),
                [handler_id, message_id](const auto& item) { return item.handler_id == handler_id && item.message_id == message_id; }),
            std::end(schedule_));
    }

private:
    struct Schedule {
        Schedule(std::string_view handler_id, std::uint16_t message_id, std::chrono::milliseconds period, std::uint64_t tick)
            : handler_id { handler_id }
            , message_id { message_id }
            , period { static_cast<std::uint32_t>(period.count() / kInterval.count()) }
            , offset { static_cast<std::uint32_t>(tick % this->period) }
        {
            assert(this->period != 0);
        }

        std::string handler_id;
        std::uint16_t message_id;
        std::uint32_t period;
        std::uint32_t offset;
    };

    void Run()
    {
        static constexpr auto kTimeDiff = std::chrono::seconds { 10 };
        // static constexpr auto kProcessingDelay = std::chrono::nanoseconds { 100 };
        // auto error = std::chrono::nanoseconds { 0 };
        auto start = std::chrono::high_resolution_clock::now();

        while (!done_.load(std::memory_order_relaxed)) {
            const auto tick = tick_.fetch_add(1, std::memory_order_relaxed) + 1;

            {
                std::shared_lock lock { mutex_ };

                for (const auto& schedule : schedule_) {
                    if (tick % schedule.period == schedule.offset) {
                        Message msg { schedule.message_id };
                        msg.to = schedule.handler_id;

                        MessageRouter::GetInstance().Post(std::move(msg));
                    }
                }
            }

            const auto end = std::chrono::high_resolution_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            auto error = elapsed - std::chrono::duration_cast<std::chrono::nanoseconds>(tick * kInterval);

            // Detect sleep mode, timezone change
            if (std::chrono::abs(error) > kTimeDiff) {
                tick_.store(0, std::memory_order_release);
                error = std::chrono::nanoseconds { 0 };
                start = end;
            }

            if (kInterval > error)
                std::this_thread::sleep_for(kInterval - error);
        }
    }

    static constexpr std::chrono::milliseconds kInterval { 100 };

    std::vector<Schedule> schedule_;
    std::shared_mutex mutex_;
    std::atomic_uint64_t tick_;
    std::atomic_bool done_;
    std::future<void> task_;
};

#endif // TIMER_H_