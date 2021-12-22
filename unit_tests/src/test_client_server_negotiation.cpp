#include <catch2/catch.hpp>

#include "irods_client_server_negotiation.hpp"

TEST_CASE("negotiation_key_is_valid")
{
    CHECK(irods::negotiation_key_is_valid("32_byte_server_negotiation_key__"));
    CHECK(irods::negotiation_key_is_valid("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    CHECK(irods::negotiation_key_is_valid("a_a_a_a_a_a_a_a_a_a_a_a_a_a_a_a_"));
    CHECK(irods::negotiation_key_is_valid("aAaAaAaAaAaAaAaAaAaAaAaAaAaAaAaA"));
    CHECK(irods::negotiation_key_is_valid("a_A_a_A_a_A_a_A_a_A_a_A_a_A_a_A_"));
    CHECK(irods::negotiation_key_is_valid("0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_"));
    CHECK(irods::negotiation_key_is_valid("a0A_a0A_a0A_a0A_a0A_a0A_a0A_a0A_"));
    CHECK(irods::negotiation_key_is_valid("ABCDEFGHIJKLMNOPQRSTUVWXYZ012345"));
    CHECK(irods::negotiation_key_is_valid("abcdefghijklmnopqrstuvwxyz012345"));
    CHECK(irods::negotiation_key_is_valid("01234567890123456789012345678900"));
    CHECK(irods::negotiation_key_is_valid("________________________________"));

    CHECK(!irods::negotiation_key_is_valid(""));
    CHECK(!irods::negotiation_key_is_valid("31_byte_server_negotiation_key_"));

    CHECK(!irods::negotiation_key_is_valid("33_byte_server_negotiation_key___"));
    CHECK(!irods::negotiation_key_is_valid("48_byte_server_negotiation_key__________________"));

    CHECK(!irods::negotiation_key_is_valid("32-byte_server_negotiation_key__"));
    CHECK(!irods::negotiation_key_is_valid("33_byte_server_negotiation_key__-"));

    // Leading underscore makes the string 32 bytes long, couldn't find any others to use.
    CHECK(!irods::negotiation_key_is_valid("_`~!@#$%^&*()-+=[{}]|;:',<>.?/\"\\"));
}
