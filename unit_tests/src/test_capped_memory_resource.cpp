#include <catch2/catch.hpp>

#include "capped_memory_resource.hpp"

#include "boost/container/pmr/vector.hpp"

#include <cstddef>
#include <stdexcept>

namespace pmr = irods::experimental::pmr;

TEST_CASE("capped_memory_resource allocation and deallocation")
{
    pmr::capped_memory_resource allocator{100};

    // No bytes allocated yet.
    CHECK(allocator.allocated() == 0);

    // Show that allocations and deallocations are tracked correctly.
    auto* p = allocator.allocate(10);
    CHECK(allocator.allocated() == 10);

    allocator.deallocate(p, 10);
    CHECK(allocator.allocated() == 0);

    // Show that the capped_memory_resource throws a std::bad_alloc exception
    // when the memory cap will be exceeded.
    CHECK_THROWS_AS([&allocator] { allocator.allocate(101); }(), std::bad_alloc);

    // Show that the allocator's state is not corrupted following a
    // std::bad_alloc exception.
    CHECK(allocator.allocated() == 0);
}

TEST_CASE("capped_memory_resource throws on invalid cap")
{
    CHECK_THROWS_AS([] { pmr::capped_memory_resource{ 0}; }(), std::invalid_argument);
    CHECK_THROWS_AS([] { pmr::capped_memory_resource{-1}; }(), std::invalid_argument);
}

TEST_CASE("capped_memory_resource works with pmr containers")
{
    pmr::capped_memory_resource allocator{64};

    {
        boost::container::pmr::vector<int> ints{&allocator};

        ints.push_back(1);
        ints.push_back(2);
        ints.push_back(3);

        CHECK_THROWS_AS([&ints] {
            for (int i = 0; i < 100; ++i) {
                ints.push_back(i + 4);
            }
        }(), std::bad_alloc);
    }

    CHECK(allocator.allocated() == 0);
}

