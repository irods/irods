#include "catch.hpp"

#include "irods_error_enum_matcher.hpp"
#include "irods_exception.hpp"
#include "irods_re_serialization.hpp"
#include "lifetime_manager.hpp"
#include "key_value_proxy.hpp"

namespace res = irods::re_serialization;

const std::string null_out = "null_value";

TEST_CASE("serialize_bytesBuf_ptr", "[pointer][serialization]")
{
    bytesBuf_t* bbuf{};
    bbuf = static_cast<bytesBuf_t*>(std::malloc(sizeof(bytesBuf_t)));
    std::memset(bbuf, 0, sizeof(bytesBuf_t));

    auto lm = irods::experimental::lifetime_manager{*bbuf};

    SECTION("nullptr") {
        auto out = res::serialized_parameter_t{{},{}};

        const auto e = res::serialize_parameter(static_cast<bytesBuf_t*>(nullptr), out);
        CHECK(e.status());

        CHECK(null_out == out.at(null_out));
    }

    SECTION("empty") {

        auto out = res::serialized_parameter_t{{},{}};

        const auto e = res::serialize_parameter(bbuf, out);
        CHECK(e.status());

        CHECK(std::to_string(0) == out.at("len"));
        CHECK(std::string{} == out.at("buf"));
    }

    SECTION("basic_string") {
        const std::string cs = "wheeeee";
        const int len = cs.size() + 1;

        bbuf->len = len;
        bbuf->buf = static_cast<void*>(std::malloc(len));
        std::memset(bbuf->buf, 0, sizeof(len));
        std::memcpy(bbuf->buf, cs.data(), len);

        auto out = res::serialized_parameter_t{{},{}};

        const auto e = res::serialize_parameter(bbuf, out);
        CHECK(e.status());

        CHECK(std::to_string(len) == out.at("len"));
        CHECK(cs + "\\x00" == out.at("buf"));
    }
}

TEST_CASE("serialize_openedDataObjInp_ptr", "[pointer][serialization]")
{
    openedDataObjInp_t* odoi{};
    odoi = static_cast<openedDataObjInp_t*>(std::malloc(sizeof(openedDataObjInp_t)));
    std::memset(odoi, 0, sizeof(openedDataObjInp_t));

    auto lm = irods::experimental::lifetime_manager{*odoi};

    SECTION("nullptr") {
        auto out = res::serialized_parameter_t{{},{}};

        const auto e = res::serialize_parameter(static_cast<openedDataObjInp_t*>(nullptr), out);
        CHECK(e.status());

        CHECK(null_out == out.at(null_out));
    }

    SECTION("empty") {
        auto out = res::serialized_parameter_t{{},{}};

        const auto e = res::serialize_parameter(odoi, out);
        CHECK(e.status());

        CHECK(std::to_string(0)      == out.at("l1descInx"));
        CHECK(std::to_string(0)      == out.at("len"));
        CHECK(std::to_string(0)      == out.at("whence"));
        CHECK(std::to_string(0)      == out.at("oprType"));
        CHECK(std::to_string(0)      == out.at("offset"));
        CHECK(std::to_string(0)      == out.at("bytesWritten"));
        CHECK(std::string("nullptr") == out.at("keyValPair_t"));
    }

    SECTION("basic_struct") {
        odoi->l1descInx = 3;
        odoi->len = 42;
        odoi->whence = -1;
        odoi->oprType = PUT_OPR;
        odoi->offset = 20;
        odoi->bytesWritten = 9;

        auto cond_input = irods::experimental::make_key_value_proxy(odoi->condInput);
        cond_input["key"] = "value";

        auto out = res::serialized_parameter_t{{},{}};

        const auto e = res::serialize_parameter(odoi, out);
        CHECK(e.status());

        CHECK(std::to_string(odoi->l1descInx)    == out.at("l1descInx"));
        CHECK(std::to_string(odoi->len)          == out.at("len"));
        CHECK(std::to_string(odoi->whence)       == out.at("whence"));
        CHECK(std::to_string(odoi->oprType)      == out.at("oprType"));
        CHECK(std::to_string(odoi->offset)       == out.at("offset"));
        CHECK(std::to_string(odoi->bytesWritten) == out.at("bytesWritten"));
        CHECK(cond_input.at("key").value()       == out.at("key"));
    }
}
