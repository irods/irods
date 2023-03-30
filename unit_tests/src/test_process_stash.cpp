#include "catch.hpp"

#include "process_stash.hpp"

#include <boost/any.hpp>

#include <algorithm>
#include <iterator>
#include <string>

TEST_CASE("process_stash basic operations")
{
    // A POD type which helps to demonstrate the stash's ability to store heterogeneous data.
    struct pod
    {
        int x = 0;
        int y = 0;
    };

    // Const data used throughout the test.
    const int an_integer = 100;
    const std::string a_string = "some data";
    const pod a_pod{320, 240};

    // Insert some data into the stash.
    const auto h1 = irods::process_stash::insert(an_integer);
    const auto h2 = irods::process_stash::insert(a_string);
    const auto h3 = irods::process_stash::insert(a_pod);

    // Show that the size of the stash is equivalent to the number of handles.
    auto handles = irods::process_stash::handles();
    REQUIRE(handles.size() == 3);

    // Show that the handles returned by the insertion exist in the handles container.
    const auto handle_exists = [](const auto& _h) noexcept {
        return [&_h](const auto& _j) noexcept {
            return _j == _h;
        };
    };

    CHECK(std::any_of(std::begin(handles), std::end(handles), handle_exists(h1)));
    CHECK(std::any_of(std::begin(handles), std::end(handles), handle_exists(h2)));
    CHECK(std::any_of(std::begin(handles), std::end(handles), handle_exists(h3)));

    // Show that the handles map to the data which generated them.
    auto* p = irods::process_stash::find(h1);
    CHECK(boost::any_cast<int&>(*p) == an_integer);

    p = irods::process_stash::find(h2);
    CHECK(boost::any_cast<std::string&>(*p) == a_string);

    p = irods::process_stash::find(h3);
    CHECK(boost::any_cast<pod&>(*p).x == a_pod.x);
    CHECK(boost::any_cast<pod&>(*p).y == a_pod.y);

    // Show that the handles can be used to erase data in the stash.
    irods::process_stash::erase(h1);
    CHECK_FALSE(irods::process_stash::find(h1));

    irods::process_stash::erase(h2);
    CHECK_FALSE(irods::process_stash::find(h2));

    irods::process_stash::erase(h3);
    CHECK_FALSE(irods::process_stash::find(h3));

    // Show that the stash is now empty.
    REQUIRE(irods::process_stash::handles().empty());
}
