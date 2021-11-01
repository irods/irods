#include "catch.hpp"

#include "irods_error_enum_matcher.hpp"
#include "user_validation_utilities.hpp"

TEST_CASE("irods_user_get_type", "[user_validation][requires_connection]")
{
    // TODO
}

TEST_CASE("irods_user_type_is_valid", "[user_validation]")
{
    namespace iu = irods::user;

    CHECK(iu::type_is_valid("rodsuser"));
    CHECK(iu::type_is_valid("rodsadmin"));
    CHECK(iu::type_is_valid("groupadmin"));
    CHECK(iu::type_is_valid("rodsgroup"));

    //CHECK(!iu::type_is_valid(nullptr)); // crashes the agent...
    CHECK(!iu::type_is_valid(""));
    CHECK(!iu::type_is_valid("nope"));
    CHECK(!iu::type_is_valid("rodsusers"));
    CHECK(!iu::type_is_valid("odsuser"));
    CHECK(!iu::type_is_valid("roduser"));
}

TEST_CASE("irods_validate_name", "[user_validation]")
{
    namespace iu = irods::user;

    // pathological
    //CHECK(!iu::validate_name(nullptr)); // crashes the agent...
    CHECK(!iu::validate_name(""));
    CHECK(!iu::validate_name(" "));
    CHECK(!iu::validate_name("\t"));
    CHECK(!iu::validate_name("\n"));

    CHECK(!iu::validate_name("."));
    CHECK(!iu::validate_name(".."));
    CHECK(iu::validate_name("..."));

    CHECK(iu::validate_name("_"));
    CHECK(iu::validate_name("-"));
    CHECK(iu::validate_name("@"));

    CHECK(!iu::validate_name("#"));
    CHECK(!iu::validate_name("##"));
    CHECK(!iu::validate_name("#zone"));
    CHECK(!iu::validate_name("user#zone#"));
    CHECK(!iu::validate_name("user#zon#e"));
    CHECK(!iu::validate_name("u#ser#zone"));
    CHECK(iu::validate_name("user#zone"));

    // no zone provided assumes local zone
    CHECK(iu::validate_name("user"));
    CHECK(iu::validate_name("user#"));

    CHECK(iu::validate_name("user-name.department@location.domain#zone-name@location.domain"));

    CHECK(iu::validate_name("this-user-name-is-63-characters-longggggggggggggggggggggggggggg"));
    CHECK(!iu::validate_name("this-username-is-64-characters-longggggggggggggggggggggggggggggg"));

    CHECK(iu::validate_name("user-name-and-zone-name-will-be-63-characters-long#yes-yes-yeah"));
    CHECK(iu::validate_name("user-name-and-zone-name-will-be-64-characters-long#yes-yeah-yeah"));

    CHECK(iu::validate_name("this-username-is-63-characters-longgggggggggggggggggggggggggggg#and-this-zonename-is-63-characters-longgggggggggggggggggggggggg"));
    CHECK(!iu::validate_name("this-username-is-64-characters-longggggggggggggggggggggggggggggg#and-this-zonename-is-63-characters-longgggggggggggggggggggggggg"));
    CHECK(!iu::validate_name("this-username-is-63-characters-longgggggggggggggggggggggggggggg#and-this-zonename-is-64-characters-longggggggggggggggggggggggggg"));
}
