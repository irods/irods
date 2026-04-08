#include <catch2/catch_all.hpp>

#include "irods/irods_client_server_negotiation.hpp"

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

TEST_CASE("sign_zone_key")
{
    constexpr const char* zone_key = "zone_key_1";
    constexpr const char* encryption_key = "32_BYTE_SERVER_NEGOTIATION_KEY__";

    std::string signed_zone_key;

    constexpr const char* default_hash_scheme = "md5";

    // Invalid zone key
    CHECK(!irods::sign_zone_key("", encryption_key, default_hash_scheme, signed_zone_key).ok());

    // Invalid encryption keys
    CHECK(!irods::sign_zone_key(zone_key, "", default_hash_scheme, signed_zone_key).ok());
    CHECK(
        !irods::sign_zone_key(zone_key, "31_BYTE_SERVER_NEGOTIATION_KEY_", default_hash_scheme, signed_zone_key).ok());

    // Invalid hash scheme
    CHECK(!irods::sign_zone_key(zone_key, encryption_key, "", signed_zone_key).ok());
    CHECK(!irods::sign_zone_key(zone_key, encryption_key, "jimbo", signed_zone_key).ok());

    // md5
    CHECK(irods::sign_zone_key(zone_key, encryption_key, default_hash_scheme, signed_zone_key).ok());
    CHECK("ff2443be8ca197d5ecc2dc360244444e" == signed_zone_key);

    // sha256
    CHECK(irods::sign_zone_key(zone_key, encryption_key, "sha256", signed_zone_key).ok());
    CHECK("f9456c64c904cf104e5a436e06c274807b102fabe2e2b485fba1d3253df33d62" == signed_zone_key);

    // sha512
    CHECK(irods::sign_zone_key(zone_key, encryption_key, "sha512", signed_zone_key).ok());
    CHECK("7e2e38e24fd02f252b14a6fcd855c63d07b263b360f936b783e64bd56744f1c148a51d8388cd78b23883b132ccc8ef9de1856e8d2d3b"
          "123ce841d111d56a815a" == signed_zone_key);
}
