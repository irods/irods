#include "catch.hpp"

#include "fixed_buffer_resource.hpp"

#include <boost/container/pmr/vector.hpp>

#include <cstddef>
#include <sstream>
#include <stdexcept>

namespace pmr = irods::experimental::pmr;

TEST_CASE("fixed_buffer_resource")
{
    std::byte buffer[1000];
    pmr::fixed_buffer_resource mem_resc{buffer, sizeof(buffer), 100};

    {
        boost::container::pmr::vector<int> ints{&mem_resc};

        for (int i = 0; i < 20; ++i) {
            ints.push_back(i);
        }

        CHECK(mem_resc.allocated() > 0);

        // Show that the allocation table can be printed out.
        std::ostringstream os;
        CHECK_NOTHROW([&mem_resc, &os] { mem_resc.print(os); }());

        const auto alloc_table = os.str();
        CHECK(alloc_table.find("  1. Header Info [") != std::string::npos);
        CHECK(alloc_table.find("previous=") != std::string::npos);
        CHECK(alloc_table.find("next=") != std::string::npos);
        CHECK(alloc_table.find("used=") != std::string::npos);
        CHECK(alloc_table.find("data_size=") != std::string::npos);
        CHECK(alloc_table.find("data=") != std::string::npos);

        // Show that the fixed_buffer_resource throws a std::bad_alloc exception
        // when the memory is exhausted.
        CHECK_THROWS_AS([&ints] {
            for (int i = 0; i < 100000; ++i) {
                ints.push_back(i);
            }
        }(), std::bad_alloc);
    }

    CHECK(mem_resc.allocated() == 0);
    CHECK(mem_resc.allocation_overhead() == 25);
}

TEST_CASE("fixed_buffer_resource throws on invalid arguments")
{
    SECTION("throws on invalid buffer")
    {
        CHECK_THROWS_AS(([] {
            pmr::fixed_buffer_resource<char>{nullptr, 1, 1};
        }()), std::invalid_argument);
    }

    SECTION("throws on invalid buffer size")
    {
        CHECK_THROWS_AS(([] {
            char buffer;
            pmr::fixed_buffer_resource<char>{&buffer, 0, 1};
        }()), std::invalid_argument);

        CHECK_THROWS_AS(([] {
            char buffer;
            pmr::fixed_buffer_resource<char>{&buffer, -1, 1};
        }()), std::invalid_argument);
    }

    SECTION("throws on invalid freelist size")
    {
        CHECK_THROWS_AS(([] {
            char buffer;
            pmr::fixed_buffer_resource<char>{&buffer, 1, 0};
        }()), std::invalid_argument);

        CHECK_THROWS_AS(([] {
            char buffer;
            pmr::fixed_buffer_resource<char>{&buffer, 1, -1};
        }()), std::invalid_argument);
    }
}

