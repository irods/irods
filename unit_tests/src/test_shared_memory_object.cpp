#include <catch2/catch.hpp>

#include "shared_memory_object.hpp"

#include <string_view>
#include <thread>
#include <chrono>

struct integer
{
    int value;
};

TEST_CASE("shared_memory_object")
{
    namespace ipc = irods::experimental::interprocess;
    using shared_memory_object = ipc::shared_memory_object<integer>;

    std::string_view shm_name = "shared_memory_object";

    SECTION("non-atomic execution")
    {
        {
            shared_memory_object smo{shm_name.data()};
            smo.exec([](auto& i) { i.value = 5; });
        }

        {
            shared_memory_object smo{shm_name.data()};
            REQUIRE(smo.exec([](auto& i) { return i.value; }) == 5);
        }

        {
            shared_memory_object smo{shm_name.data()};
            smo.remove();
        }

        {
            shared_memory_object smo{shm_name.data()};
            REQUIRE(smo.exec([](auto& i) { return i.value; }) == 0);
            smo.remove();
        }
    }

    SECTION("atomic execution")
    {
        constexpr int loop_count = 1000;

        shared_memory_object smo{shm_name.data()};

        const auto inc = [&smo] {
            using namespace std::chrono_literals;

            for (int i = 0; i < loop_count; ++i) {
                smo.atomic_exec([](auto& i) { ++i.value; });
                std::this_thread::sleep_for(1ms);
            }
        };

        std::thread t{inc};
        inc();
        t.join();

        REQUIRE(loop_count * 2 == smo.exec([](auto& i) { return i.value; }));

        smo.remove();
    }
}

