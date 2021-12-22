#include <catch2/catch.hpp>

#include "irods_linked_list_iterator.hpp"
#include "objInfo.h"
#include "dataObjOpr.hpp"

#include <memory>
#include <iterator>
#include <functional>

using deleter_t = std::function<void(dataObjInfo_t*)>;

auto make_c_linked_list(int _list_length) noexcept -> std::unique_ptr<dataObjInfo_t, deleter_t>;

TEST_CASE("irods_linked_list_iterator", "[iterator]")
{
    const int list_length = 5;
    auto list = make_c_linked_list(list_length);
    irods::linked_list_iterator<dataObjInfo_t> pos{list.get()};

    REQUIRE(list.get() != nullptr);
    REQUIRE(list.get() == &*pos);

    SECTION("copy_assignment")
    {
        const auto copy = pos;
        REQUIRE(copy == pos);
    }

    SECTION("pre_fix_increment")
    {
        const auto copy = ++pos;
        REQUIRE(copy == pos);
    }

    SECTION("post_fix_increment")
    {
        auto previous_pos = pos++;
        REQUIRE(previous_pos != pos);
        REQUIRE(previous_pos->next == &*pos);
        REQUIRE(++previous_pos == pos);
    }

    SECTION("standard_algorithms")
    {
        REQUIRE(std::distance(begin(list.get()), end(list.get())) == list_length);
    }

    SECTION("range_based_for_loop")
    {
        int count = 0;

        for (const auto& _ : list.get())
        {
            (void) _;
            ++count;
        }

        REQUIRE(count == list_length);
    }
}

auto make_c_linked_list(int _list_length) noexcept -> std::unique_ptr<dataObjInfo_t, deleter_t>
{
    auto deleter = [](auto* _list)
    {
        auto* head = _list;
        auto* tail = _list->next;

        while (head)
        {
            delete head;
            if (!tail) break;
            head = tail;
            tail = tail->next;
        }
    };

    std::unique_ptr<dataObjInfo_t, deleter_t> head{new dataObjInfo_t{}, deleter};
    auto* list = head.get();

    for (int i = 1; i < _list_length; ++i)
    {
        list->next = new dataObjInfo_t{};
        list = list->next;
    }

    return head;
}

