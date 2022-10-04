#include <catch2/catch.hpp>

#include "irods/packStruct.h"
#include "irods/rcGlobalExtern.h"
#include "irods/irods_at_scope_exit.hpp"

#include <cstring>
#include <string_view>

TEST_CASE("packstruct xml encoding")
{
    char data[] = R"_(aaa`'"<&test&>"'`_file)_";

    const auto protocol = XML_PROT;

    DataObjInp input{};
    std::strncpy(input.objPath, data, sizeof(data));

    BytesBuf* packed_result = nullptr;
    irods::at_scope_exit free_packed_result{[&packed_result] {
        std::free(packed_result->buf);
        std::free(packed_result);
    }};

    const auto is_equal = [](const BytesBuf& _bbuf, const std::string_view _expected) -> bool
    {
        const std::string_view xml(static_cast<const char*>(_bbuf.buf), _bbuf.len);
        const auto s = std::strlen("<DataObjInp_PI>\n<objPath>");
        return xml.substr(s, xml.find("</objPath>", s) - s) == _expected;
    };

    SECTION("4.2.8")
    {
        const char* peer_version = "rods4.2.8";

        REQUIRE(pack_struct(&input, &packed_result, "DataObjInp_PI", nullptr, 0, protocol, peer_version) == 0);

        REQUIRE(packed_result);
        REQUIRE(packed_result->len > 0);
        REQUIRE(is_equal(*packed_result, "aaa&apos;'&quot;&lt;&amp;test&amp;&gt;&quot;'&apos;_file"));

        DataObjInp* unpacked_result = nullptr;
        irods::at_scope_exit free_unpacked_result{[&unpacked_result] { std::free(unpacked_result); }};

        REQUIRE(unpack_struct(packed_result->buf, (void**) &unpacked_result, "DataObjInp_PI", nullptr, protocol, peer_version) == 0);

        REQUIRE(std::string_view((char*) unpacked_result->objPath) == data);
    }

    SECTION("client=4.2.8 server=4.2.9")
    {
        // A 4.2.8 client or server does not have logic for adjusting the packing instructions based on
        // the version of the peer. To simulate that, the version string is passed as a nullptr.
        const char* server_version = nullptr;
        REQUIRE(pack_struct(&input, &packed_result, "DataObjInp_PI", nullptr, 0, protocol, server_version) == 0);

        REQUIRE(packed_result);
        REQUIRE(packed_result->len > 0);
        REQUIRE(is_equal(*packed_result, "aaa&apos;'&quot;&lt;&amp;test&amp;&gt;&quot;'&apos;_file"));

        DataObjInp* unpacked_result = nullptr;
        irods::at_scope_exit free_unpacked_result{[&unpacked_result] { std::free(unpacked_result); }};

        const char* client_version = "rods4.2.8";
        REQUIRE(unpack_struct(packed_result->buf, (void**) &unpacked_result, "DataObjInp_PI", nullptr, protocol, client_version) == 0);

        REQUIRE(std::string_view((char*) unpacked_result->objPath) == data);
    }

    SECTION("client=4.2.9 server=4.2.8")
    {
        const char* server_version = "rods4.2.8";
        REQUIRE(pack_struct(&input, &packed_result, "DataObjInp_PI", nullptr, 0, protocol, server_version) == 0);

        REQUIRE(packed_result);
        REQUIRE(packed_result->len > 0);
        REQUIRE(is_equal(*packed_result, "aaa&apos;'&quot;&lt;&amp;test&amp;&gt;&quot;'&apos;_file"));

        DataObjInp* unpacked_result = nullptr;
        irods::at_scope_exit free_unpacked_result{[&unpacked_result] { std::free(unpacked_result); }};

        // A 4.2.8 client or server does not have logic for adjusting the packing instructions based on
        // the version of the peer. To simulate that, the version string is passed as a nullptr.
        const char* client_version = nullptr;
        REQUIRE(unpack_struct(packed_result->buf, (void**) &unpacked_result, "DataObjInp_PI", nullptr, protocol, client_version) == 0);

        REQUIRE(std::string_view((char*) unpacked_result->objPath) == data);
    }

    SECTION("4.2.9")
    {
        const char* peer_version = "rods4.2.9";

        REQUIRE(pack_struct(&input, &packed_result, "DataObjInp_PI", nullptr, 0, protocol, peer_version) == 0);

        REQUIRE(packed_result);
        REQUIRE(packed_result->len > 0);
        REQUIRE(is_equal(*packed_result, "aaa`&apos;&quot;&lt;&amp;test&amp;&gt;&quot;&apos;`_file"));

        DataObjInp* unpacked_result = nullptr;
        irods::at_scope_exit free_unpacked_result{[&unpacked_result] { std::free(unpacked_result); }};

        REQUIRE(unpack_struct(packed_result->buf, (void**) &unpacked_result, "DataObjInp_PI", nullptr, protocol, peer_version) == 0);

        REQUIRE(std::string_view((char*) unpacked_result->objPath) == data);
    }
}

