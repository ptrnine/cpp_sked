#include "catch2/catch_test_macros.hpp"

#include "fake_clock.hpp"

using namespace std::chrono_literals;
using namespace skd::literals;

struct once_func {
    once_func(std::atomic_bool* idestroyed, std::atomic_uint* icalled): destroyed(idestroyed), called(icalled) {}

    once_func(once_func&& f) noexcept: destroyed(f.destroyed), called(f.called) {
        f.moved = true;
    }

    once_func(const once_func& f): destroyed(f.destroyed), called(f.called) {}

    once_func& operator=(once_func&& f) noexcept {
        destroyed = f.destroyed;
        called = f.called;
        f.moved = true;
        return *this;
    }

    once_func& operator=(const once_func& f) noexcept {
        destroyed = f.destroyed;
        called = f.called;
        return *this;
    }

    ~once_func() {
        if (!moved)
            *destroyed = true;
    }

    void operator()() {
        ++(*called);
    }

    std::atomic_bool* destroyed;
    std::atomic_uint* called;
    bool moved = false;
};

TEST_CASE("every_second") {
    skd::sked sked;
    std::atomic_uint c = 0;
    sked.every().second([&] { ++c; });
    std::this_thread::sleep_for(1s);
    REQUIRE(c == 1);
}

TEST_CASE("once") {
    skd::sked sked;

    std::atomic_bool destroyed1 = false;
    std::atomic_uint called1 = 0;
    std::atomic_bool destroyed2 = false;
    std::atomic_uint called2 = 0;

    sked.once().second(once_func{&destroyed1, &called1});
    sked.once().second(once_func{&destroyed2, &called2});
    std::this_thread::sleep_for(3s);

    REQUIRE(destroyed1);
    REQUIRE(called1 == 1);
    REQUIRE(destroyed2);
    REQUIRE(called2 == 1);
}

TEST_CASE("specific_time") {
    skd::sked<adjusted_clock> sked;

    adjusted_clock::set_date(skd::week_e::wednesday, 13, 45, 34);

    std::atomic_uint called = 0;
    sked.once().friday().at("13:45:35"_dtm)([&] { called += 1; });
    sked.once().wednesday().at("13:45:35"_dtm)([&] { called += 10; });
    sked.once().day("13:45:35"_dtm)([&] { called += 100; });
    sked.once().hour(45, 36)([&] { called += 1000; });
    sked.once().minute(36)([&] { called += 10000; });

    std::this_thread::sleep_for(3s);

    REQUIRE(called == 11110);
}
