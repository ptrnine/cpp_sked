#pragma once

#include "cpp_sked.hpp"

class fake_clock {
public:
    static fake_clock& instance() {
        static fake_clock clock;
        return clock;
    }

    static inline constexpr bool is_steady = std::chrono::system_clock::is_steady;
    using rep = std::chrono::system_clock::rep;
    using period = std::chrono::system_clock::period;
    using duration = std::chrono::system_clock::duration;
    using time_point = std::chrono::time_point<fake_clock>;

    static time_point now() {
        return instance().now_impl();
    }

    static void set_date(skd::week_e weekday, size_t hour, size_t minute, size_t second) {
        instance().set_date_impl(weekday, hour, minute, second);
    }

private:
    time_point now_impl() {
        std::lock_guard lock{mtx};
        return now_tp;
    }

    void set_date_impl(skd::week_e weekday, size_t hour, size_t minute, size_t second) {
        auto real_now = std::chrono::system_clock::now();
        auto week_now = time_point_cast<skd::weeks>(real_now);

        auto ws1 = real_now - week_now;
        auto ws2 = skd::days{int(weekday)} + skd::hours{hour} + skd::minutes{minute} + skd::seconds{second};

        std::lock_guard lock{mtx};
        now_tp = time_point{(real_now + (ws2 - ws1)).time_since_epoch()};
    }

    std::mutex mtx;
    time_point now_tp;
};

class adjusted_clock {
public:
    static adjusted_clock& instance() {
        static adjusted_clock clock;
        return clock;
    }

    static inline constexpr bool is_steady = std::chrono::system_clock::is_steady;
    using rep = std::chrono::system_clock::rep;
    using period = std::chrono::system_clock::period;
    using duration = std::chrono::system_clock::duration;
    using time_point = std::chrono::time_point<adjusted_clock>;

    static time_point now() {
        return instance().now_impl();
    }

    static void set_date(skd::week_e weekday, size_t hour, size_t minute, size_t second) {
        instance().set_date_impl(weekday, hour, minute, second);
    }

private:
    time_point now_impl() {
        std::lock_guard lock{mtx};
        auto t = std::chrono::system_clock::now() + shift;
        return time_point{t.time_since_epoch()};
    }

    void set_date_impl(skd::week_e weekday, size_t hour, size_t minute, size_t second) {
        auto real_now = std::chrono::system_clock::now();
        auto week_now = time_point_cast<skd::weeks>(real_now);

        auto ws1 = real_now - week_now;
        auto ws2 = skd::days{int(weekday)} + skd::hours{hour} + skd::minutes{minute} + skd::seconds{second};

        std::lock_guard lock{mtx};
        shift = ws2 - ws1;
    }

    std::mutex mtx;
    std::chrono::system_clock::duration shift;
};
