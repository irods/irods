#include <catch2/catch_all.hpp>

#include "irods/ADLER32Strategy.hpp"
#include "irods/CRC64NVMEStrategy.hpp"
#include "irods/Hasher.hpp"
#include "irods/MD5Strategy.hpp"
#include "irods/SHA1Strategy.hpp"
#include "irods/SHA256Strategy.hpp"
#include "irods/SHA512Strategy.hpp"
#include "irods/hash_types.hpp"
#include "irods/irods_hasher_factory.hpp"
#include "irods/rodsErrorTable.h"

#include <fmt/format.h>

#include <string>
#include <tuple>
#include <vector>

TEST_CASE("checksum hashers - strings")
{
    // clang-format off
    const std::tuple<const std::string, const std::string, const std::string> hash_test_vals = GENERATE(
        std::make_tuple("asdf1234ASDF!@#$", irods::MD5_NAME,       "70f597ce53373700ba5e5dfc892bac59"),
        std::make_tuple("asdf1234ASDF!@#$", irods::SHA1_NAME,      "sha1:w4pAayodnxhmStdLObUARZUQu68="),
        std::make_tuple("asdf1234ASDF!@#$", irods::SHA256_NAME,    "sha2:jwyFBi2ugt4geZKPMJzJlE8eQ/M8qLpAbKNzS0uUBG4="),
        std::make_tuple("asdf1234ASDF!@#$", irods::SHA512_NAME,    "sha512:oOcXC8A34DybRSivQjyGExYLzEmHXzh0KUtZZzE72EDQ3lTcWhOkF0XmLkzX85QrJT80Ral+v4/zDQthDvoj8A=="),
        std::make_tuple("asdf1234ASDF!@#$", irods::ADLER32_NAME,   "adler32:28b8042f"),
        std::make_tuple("asdf1234ASDF!@#$", irods::CRC64NVME_NAME, "crc64nvme:OTvEv/lA92k="),
        std::make_tuple("",                 irods::MD5_NAME,       "d41d8cd98f00b204e9800998ecf8427e"),
        std::make_tuple("",                 irods::SHA1_NAME,      "sha1:2jmj7l5rSw0yVb/vlWAYkK/YBwk="),
        std::make_tuple("",                 irods::SHA256_NAME,    "sha2:47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU="),
        std::make_tuple("",                 irods::SHA512_NAME,    "sha512:z4PhNX7vuL3xVChQ1m2AB9Yg5AULVxXcg/SpIdNs6c5H0NE8XYXysP+DGNKHfuwvY7kxvUdBeoGlODJ6+SfaPg=="),
        std::make_tuple("",                 irods::ADLER32_NAME,   "adler32:00000001"),
        std::make_tuple("",                 irods::CRC64NVME_NAME, "crc64nvme:AAAAAAAAAAA=")
    );
    // clang-format on

    const std::string& str_to_hash = std::get<0>(hash_test_vals);
    const std::string& hash_type_str = std::get<1>(hash_test_vals);
    const std::string& expected_hash_value = std::get<2>(hash_test_vals);

    SECTION(fmt::format("{} hash [{}]", hash_type_str, str_to_hash))
    {
        irods::Hasher hasher;
        REQUIRE(irods::getHasher(hash_type_str, hasher).ok());

        SECTION("all-at-once")
        {
            REQUIRE(hasher.update(str_to_hash).ok());

            std::string hash_value;
            REQUIRE(hasher.digest(hash_value).ok());

            REQUIRE(hash_value == expected_hash_value);
        }

        SECTION("byte-by-byte")
        {
            for (const char& byte : str_to_hash) {
                const std::string byte_s(1, byte);
                REQUIRE(hasher.update(byte_s).ok());
            }

            std::string hash_value;
            REQUIRE(hasher.digest(hash_value).ok());

            REQUIRE(hash_value == expected_hash_value);
        }
    }
}

const std::vector<unsigned char> test_nonstring_1{0x00, 0x00, 0x00, 0x00, 0xF0, 0xA4, 0xAD, 0xA2, 0xC3, 0x28, 0xA0,
                                                  0xA1, 0xF0, 0x90, 0x28, 0xBC, 0xF0, 0x28, 0x8C, 0x28, 0xFF};

TEST_CASE("checksum hashers - nonstrings")
{
    // clang-format off
    const std::tuple<const std::vector<unsigned char>, const std::string, const std::string> hash_test_vals = GENERATE(
        std::make_tuple(test_nonstring_1, irods::MD5_NAME,       "47146e5abac11e150e8f5b0c64574e51"),
        std::make_tuple(test_nonstring_1, irods::SHA1_NAME,      "sha1:6VDLSmD0EtUrxhB5PLpAH1jAAxs="),
        std::make_tuple(test_nonstring_1, irods::SHA256_NAME,    "sha2:Vv+8sjSakZcB2PPoVQZUaVIU5EDD4DjA3mNSB4bB7u4="),
        std::make_tuple(test_nonstring_1, irods::SHA512_NAME,    "sha512:dnPmYhU+7FrWj8hdMRjFEiXu11cQYVItfjGMweCcETgY8NWX5VhIoEk4Stka7RDifH0mz/L7tpIF2H7+YcKkxA=="),
        std::make_tuple(test_nonstring_1, irods::ADLER32_NAME,   "adler32:60e80a3f"),
        std::make_tuple(test_nonstring_1, irods::CRC64NVME_NAME, "crc64nvme:LmR4qrQyCGM=")
    );
    // clang-format on

    const std::vector<unsigned char>& data_to_hash_v = std::get<0>(hash_test_vals);
    const std::string& hash_type_str = std::get<1>(hash_test_vals);
    const std::string& expected_hash_value = std::get<2>(hash_test_vals);

    const std::string data_to_hash(reinterpret_cast<const char*>(data_to_hash_v.data()), data_to_hash_v.size());

    SECTION(fmt::format("{} hash nonstring", hash_type_str))
    {
        irods::Hasher hasher;
        REQUIRE(irods::getHasher(hash_type_str, hasher).ok());

        SECTION("all-at-once")
        {
            REQUIRE(hasher.update(data_to_hash).ok());

            std::string hash_value;
            REQUIRE(hasher.digest(hash_value).ok());

            REQUIRE(hash_value == expected_hash_value);
        }

        SECTION("byte-by-byte")
        {
            for (const unsigned char& byte : data_to_hash_v) {
                const std::string byte_s(1, static_cast<char>(byte));
                REQUIRE(hasher.update(byte_s).ok());
            }

            std::string hash_value;
            REQUIRE(hasher.digest(hash_value).ok());

            REQUIRE(hash_value == expected_hash_value);
        }
    }
}

TEST_CASE("HashStrategy::digest options")
{
    using om = irods::hash::output_mode;

    constexpr std::string_view str_to_hash = "asdf1234ASDF!@#$";

    // clang-format off
    const std::tuple<const irods::hash::output_mode, const bool, const std::string, const std::string> hash_test_vals = GENERATE(
        // std::make_tuple(om::base64_encoded_string, false, irods::ADLER32_NAME,   "adler32:KLgELw=="), // not implemented
        std::make_tuple(om::base64_encoded_string, false, irods::CRC64NVME_NAME, "OTvEv/lA92k="),
        std::make_tuple(om::base64_encoded_string, false, irods::MD5_NAME,       "cPWXzlM3NwC6Xl38iSusWQ=="),
        std::make_tuple(om::base64_encoded_string, false, irods::SHA1_NAME,      "w4pAayodnxhmStdLObUARZUQu68="),
        std::make_tuple(om::base64_encoded_string, false, irods::SHA256_NAME,    "jwyFBi2ugt4geZKPMJzJlE8eQ/M8qLpAbKNzS0uUBG4="),
        std::make_tuple(om::base64_encoded_string, false, irods::SHA512_NAME,    "oOcXC8A34DybRSivQjyGExYLzEmHXzh0KUtZZzE72EDQ3lTcWhOkF0XmLkzX85QrJT80Ral+v4/zDQthDvoj8A=="),
        // std::make_tuple(om::base64_encoded_string, true,  irods::ADLER32_NAME,   "adler32:KLgELw=="), // not implemented
        std::make_tuple(om::base64_encoded_string, true,  irods::CRC64NVME_NAME, "crc64nvme:OTvEv/lA92k="), // default
        std::make_tuple(om::base64_encoded_string, true,  irods::MD5_NAME,       "md5:cPWXzlM3NwC6Xl38iSusWQ=="),
        std::make_tuple(om::base64_encoded_string, true,  irods::SHA1_NAME,      "sha1:w4pAayodnxhmStdLObUARZUQu68="), // default
        std::make_tuple(om::base64_encoded_string, true,  irods::SHA256_NAME,    "sha2:jwyFBi2ugt4geZKPMJzJlE8eQ/M8qLpAbKNzS0uUBG4="), // default
        std::make_tuple(om::base64_encoded_string, true,  irods::SHA512_NAME,    "sha512:oOcXC8A34DybRSivQjyGExYLzEmHXzh0KUtZZzE72EDQ3lTcWhOkF0XmLkzX85QrJT80Ral+v4/zDQthDvoj8A=="), // default
        std::make_tuple(om::hex_string,            false, irods::ADLER32_NAME,   "28b8042f"),
        std::make_tuple(om::hex_string,            false, irods::CRC64NVME_NAME, "393bc4bff940f769"),
        std::make_tuple(om::hex_string,            false, irods::MD5_NAME,       "70f597ce53373700ba5e5dfc892bac59"), // default
        std::make_tuple(om::hex_string,            false, irods::SHA1_NAME,      "c38a406b2a1d9f18664ad74b39b500459510bbaf"),
        std::make_tuple(om::hex_string,            false, irods::SHA256_NAME,    "8f0c85062dae82de2079928f309cc9944f1e43f33ca8ba406ca3734b4b94046e"),
        std::make_tuple(om::hex_string,            false, irods::SHA512_NAME,    "a0e7170bc037e03c9b4528af423c8613160bcc49875f3874294b5967313bd840d0de54dc5a13a41745e62e4cd7f3942b253f3445a97ebf8ff30d0b610efa23f0"),
        std::make_tuple(om::hex_string,            true,  irods::ADLER32_NAME,   "adler32:28b8042f"), // default
        std::make_tuple(om::hex_string,            true,  irods::CRC64NVME_NAME, "crc64nvme:393bc4bff940f769"),
        std::make_tuple(om::hex_string,            true,  irods::MD5_NAME,       "md5:70f597ce53373700ba5e5dfc892bac59"),
        std::make_tuple(om::hex_string,            true,  irods::SHA1_NAME,      "sha1:c38a406b2a1d9f18664ad74b39b500459510bbaf"),
        std::make_tuple(om::hex_string,            true,  irods::SHA256_NAME,    "sha2:8f0c85062dae82de2079928f309cc9944f1e43f33ca8ba406ca3734b4b94046e"),
        std::make_tuple(om::hex_string,            true,  irods::SHA512_NAME,    "sha512:a0e7170bc037e03c9b4528af423c8613160bcc49875f3874294b5967313bd840d0de54dc5a13a41745e62e4cd7f3942b253f3445a97ebf8ff30d0b610efa23f0")
    );
    // clang-format on

    const auto options = irods::hash::options{std::get<0>(hash_test_vals), std::get<1>(hash_test_vals)};
    const auto& hash_type_str = std::get<2>(hash_test_vals);
    const auto& expected_hash_value = std::get<3>(hash_test_vals);

    SECTION(fmt::format("hash:[{}],mode:[{}],checksum_prefix:[{}]",
                        hash_type_str,
                        static_cast<int>(options.output_mode),
                        options.include_checksum_prefix))
    {
        irods::Hasher hasher;
        REQUIRE(irods::getHasher(hash_type_str, hasher).ok());

        SECTION("all-at-once")
        {
            REQUIRE(hasher.update(str_to_hash.data()).ok());

            std::string hash_value;
            REQUIRE(hasher.digest(options, hash_value).ok());

            REQUIRE(hash_value == expected_hash_value);
        }

        SECTION("byte-by-byte")
        {
            for (const char& byte : str_to_hash) {
                const std::string byte_s(1, byte);
                REQUIRE(hasher.update(byte_s).ok());
            }

            std::string hash_value;
            REQUIRE(hasher.digest(options, hash_value).ok());

            REQUIRE(hash_value == expected_hash_value);
        }
    }
}

TEST_CASE("ADLER32 does not support base64-encoded string output")
{
    constexpr std::string_view str_to_hash = "asdf1234ASDF!@#$";

    // Set up ADLER32 hasher.
    irods::Hasher hasher;
    REQUIRE(irods::getHasher(irods::ADLER32_NAME, hasher).ok());
    REQUIRE(hasher.update(str_to_hash.data()).ok());

    std::string hash_value;

    // digest() should fail with checksum prefix.
    REQUIRE(SYS_NOT_IMPLEMENTED ==
            hasher.digest({irods::hash::output_mode::base64_encoded_string, true}, hash_value).code());
    REQUIRE(hash_value.empty());

    // ...and without checksum prefix.
    REQUIRE(SYS_NOT_IMPLEMENTED ==
            hasher.digest({irods::hash::output_mode::base64_encoded_string, false}, hash_value).code());
    REQUIRE(hash_value.empty());
}
