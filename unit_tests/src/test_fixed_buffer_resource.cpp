#include <boost/version.hpp>

#if BOOST_VERSION >= 107200

#include <catch2/catch.hpp>

#include "irods/fixed_buffer_resource.hpp"

#include <boost/container/pmr/vector.hpp>

#include <cstddef>
#include <sstream>
#include <stdexcept>

namespace pmr = irods::experimental::pmr;

TEST_CASE("fixed_buffer_resource allocation and deallocation")
{
    char buffer[100];
    pmr::fixed_buffer_resource allocator{buffer, sizeof(buffer)};

    // No bytes allocated yet.
    CHECK(allocator.allocated() == 0);

    // Show that allocations and deallocations are tracked correctly.
    auto* p = allocator.allocate(10);
    CHECK(allocator.allocated() == 10);

    allocator.deallocate(p, 10);
    CHECK(allocator.allocated() == 0);

    // Show that the fixed_buffer_resource throws a std::bad_alloc exception
    // when the underlying buffer is exhausted.
    CHECK_THROWS_AS([&allocator] { allocator.allocate(101); }(), std::bad_alloc);

    // Show that the allocator's state is not corrupted following a
    // std::bad_alloc exception.
    CHECK(allocator.allocated() == 0);
}

TEST_CASE("fixed_buffer_resource throws on invalid arguments")
{
    SECTION("throws on invalid buffer")
    {
        CHECK_THROWS_AS(([] {
            pmr::fixed_buffer_resource<char>{nullptr, 1};
        }()), std::invalid_argument);
    }

    SECTION("throws on invalid buffer size")
    {
        CHECK_THROWS_AS(([] {
            char buffer;
            pmr::fixed_buffer_resource<char>{&buffer, 0};
        }()), std::invalid_argument);

        CHECK_THROWS_AS(([] {
            char buffer;
            pmr::fixed_buffer_resource<char>{&buffer, -1};
        }()), std::invalid_argument);
    }
}

TEST_CASE("fixed_buffer_resource works with pmr containers")
{
    char buffer[256];
    pmr::fixed_buffer_resource allocator{buffer, sizeof(buffer)};

    {
        boost::container::pmr::vector<int> ints{&allocator};

        ints.push_back(1);
        ints.push_back(2);
        ints.push_back(3);

        CHECK(allocator.allocated() == sizeof(int) * ints.size());

        // Show that the allocation table can be printed out.
        std::ostringstream os;
        CHECK_NOTHROW([&allocator, &os] { allocator.print(os); }());

        const auto alloc_table = os.str();
        CHECK(alloc_table.find("  1. Header Info [") != std::string::npos);
        CHECK(alloc_table.find("previous=") != std::string::npos);
        CHECK(alloc_table.find("next=") != std::string::npos);
        CHECK(alloc_table.find("used=") != std::string::npos);
        CHECK(alloc_table.find("data=") != std::string::npos);
        CHECK(alloc_table.find("data_size=") != std::string::npos);

        // Show that the fixed_buffer_resource throws a std::bad_alloc exception
        // when the memory is exhausted.
        CHECK_THROWS_AS([&ints] {
            for (int i = 0; i < 100; ++i) {
                ints.push_back(i + 4);
            }
        }(), std::bad_alloc);
    }

    CHECK(allocator.allocated() == 0);
}

#endif // BOOST_VERSION >= 107200

