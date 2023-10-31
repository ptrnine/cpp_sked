#include "catch2/catch_test_macros.hpp"

#include "fake_clock.hpp"

using namespace std::chrono_literals;

TEST_CASE("every_second") {
    skd::sked<fake_clock> sked;

    fake_clock::set_date(skd::week_e::monday, 0, 0, 0);
    std::atomic_uint c = 0;
    sked.every().second([&] { ++c; });
    fake_clock::set_date(skd::week_e::monday, 0, 0, 1);
    std::this_thread::sleep_for(1s);
    REQUIRE(c == 1);
}
