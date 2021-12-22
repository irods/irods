#include <catch2/catch.hpp>

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

TEST_CASE("serialize_rsComm_ptr", "[pointer][serialization]")
{
    RsComm* rscp{};
    rscp = static_cast<RsComm*>(std::malloc(sizeof(RsComm)));
    std::memset(rscp, 0, sizeof(RsComm));

    auto lm = irods::experimental::lifetime_manager{*rscp};

    SECTION("nullptr") {
        auto out = res::serialized_parameter_t{{},{}};

        const auto e = res::serialize_parameter(static_cast<RsComm*>(nullptr), out);
        CHECK(e.status());

        CHECK("nullptr" == out.at("rsComm_ptr"));
    }

    SECTION("empty") {
        auto out = res::serialized_parameter_t{{},{}};

        const auto e = res::serialize_parameter(rscp, out);
        CHECK(e.status());

        CHECK_THROWS(out.at("auth_scheme"));

        CHECK(out.at("client_addr") == rscp->clientAddr);

        const auto& pu = rscp->proxyUser;
        CHECK(out.at("proxy_user_name")                         == pu.userName);
        CHECK(out.at("proxy_rods_zone")                         == pu.rodsZone);
        CHECK(out.at("proxy_user_type")                         == pu.userType);
        CHECK(out.at("proxy_sys_uid")                           == std::to_string(pu.sysUid));

        const auto& puai = pu.authInfo;
        CHECK(out.at("proxy_auth_info_auth_scheme")             == puai.authScheme);
        CHECK(out.at("proxy_auth_info_auth_flag")               == std::to_string(puai.authFlag));
        CHECK(out.at("proxy_auth_info_flag")                    == std::to_string(puai.flag));
        CHECK(out.at("proxy_auth_info_ppid")                    == std::to_string(puai.ppid));
        CHECK(out.at("proxy_auth_info_host")                    == puai.host);
        CHECK(out.at("proxy_auth_info_auth_str")                == puai.authStr);

        const auto& puuoi = pu.userOtherInfo;
        CHECK(out.at("proxy_user_other_info_user_info")         == puuoi.userInfo);
        CHECK(out.at("proxy_user_other_info_user_comments")     == puuoi.userComments);
        CHECK(out.at("proxy_user_other_info_user_create")       == puuoi.userCreate);
        CHECK(out.at("proxy_user_other_info_user_modify")       == puuoi.userModify);

        const auto& cu = rscp->clientUser;
        CHECK(out.at("user_user_name")                         == cu.userName);
        CHECK(out.at("user_rods_zone")                         == cu.rodsZone);
        CHECK(out.at("user_user_type")                         == cu.userType);
        CHECK(out.at("user_sys_uid")                           == std::to_string(cu.sysUid));

        const auto& cuai = cu.authInfo;
        CHECK(out.at("user_auth_info_auth_scheme")             == cuai.authScheme);
        CHECK(out.at("user_auth_info_auth_flag")               == std::to_string(cuai.authFlag));
        CHECK(out.at("user_auth_info_flag")                    == std::to_string(cuai.flag));
        CHECK(out.at("user_auth_info_ppid")                    == std::to_string(cuai.ppid));
        CHECK(out.at("user_auth_info_host")                    == cuai.host);
        CHECK(out.at("user_auth_info_auth_str")                == cuai.authStr);

        const auto& cuuoi = cu.userOtherInfo;
        CHECK(out.at("user_user_other_info_user_info")         == cuuoi.userInfo);
        CHECK(out.at("user_user_other_info_user_comments")     == cuuoi.userComments);
        CHECK(out.at("user_user_other_info_user_create")       == cuuoi.userCreate);
        CHECK(out.at("user_user_other_info_user_modify")       == cuuoi.userModify);

        CHECK(out.at("socket")          == std::to_string(rscp->sock));
        CHECK(out.at("connect_count")   == std::to_string(rscp->connectCnt));
        CHECK(out.at("status")          == std::to_string(rscp->status));
        CHECK(out.at("api_index")       == std::to_string(rscp->apiInx));
        CHECK(out.at("option")          == rscp->option);
    }

    SECTION("basic_struct") {
        const std::string as = "myauthscheme";

        std::strncpy(rscp->clientAddr, "127.0.0.1", sizeof(rscp->clientAddr));
        rscp->auth_scheme = const_cast<char*>(as.data());

        auto& pu = rscp->proxyUser;
        std::strncpy(pu.userName, "rods", sizeof(pu.userName));
        std::strncpy(pu.rodsZone, "tempZone", sizeof(pu.rodsZone));
        std::strncpy(pu.userType, "rodsadmin", sizeof(pu.userType));
        pu.sysUid = 999;

        auto& puai = pu.authInfo;
        std::strncpy(puai.authScheme, "myauthscheme", sizeof(puai.authScheme));
        puai.authFlag = 1;
        puai.flag = 2;
        puai.ppid = 3;
        std::strncpy(puai.host, "here.example.org", sizeof(puai.host));
        std::strncpy(puai.authStr, "myauthstr", sizeof(puai.authStr));

        auto& puuoi = pu.userOtherInfo;
        std::strncpy(puuoi.userInfo, "userinfo", sizeof(puuoi.userInfo));
        std::strncpy(puuoi.userComments, "jimbo", sizeof(puuoi.userComments));
        std::strncpy(puuoi.userCreate, "123456789", sizeof(puuoi.userCreate));
        std::strncpy(puuoi.userModify, "987654321", sizeof(puuoi.userModify));

        auto& cu = rscp->clientUser;
        std::strncpy(cu.userName, "alice", sizeof(cu.userName));
        std::strncpy(cu.rodsZone, "otherZone", sizeof(cu.rodsZone));
        std::strncpy(cu.userType, "rodsuser", sizeof(cu.userType));
        cu.sysUid = 111;

        auto& cuai = cu.authInfo;
        std::strncpy(cuai.authScheme, "authschemezzz", sizeof(cuai.authScheme));
        cuai.authFlag = 4;
        cuai.flag = 5;
        cuai.ppid = 6;
        std::strncpy(cuai.host, "other.example.org", sizeof(cuai.host));
        std::strncpy(cuai.authStr, "anotherauthstr", sizeof(cuai.authStr));

        auto& cuuoi = cu.userOtherInfo;
        std::strncpy(cuuoi.userInfo, "aaaaaaaa", sizeof(cuuoi.userInfo));
        std::strncpy(cuuoi.userComments, "kimbo", sizeof(cuuoi.userComments));
        std::strncpy(cuuoi.userCreate, "987654321", sizeof(cuuoi.userCreate));
        std::strncpy(cuuoi.userModify, "123456789", sizeof(cuuoi.userModify));

        rscp->sock = 3;
        rscp->connectCnt = 1;
        rscp->status = -808000;
        rscp->apiInx = 701;
        std::strncpy(rscp->option, "option", sizeof(rscp->option));

        auto out = res::serialized_parameter_t{{},{}};

        const auto e = res::serialize_parameter(rscp, out);
        CHECK(e.status());

        CHECK(out.at("client_addr") == rscp->clientAddr);
        CHECK(out.at("auth_scheme") == as);

        CHECK(out.at("proxy_user_name")                         == pu.userName);
        CHECK(out.at("proxy_rods_zone")                         == pu.rodsZone);
        CHECK(out.at("proxy_user_type")                         == pu.userType);
        CHECK(out.at("proxy_sys_uid")                           == std::to_string(pu.sysUid));

        CHECK(out.at("proxy_auth_info_auth_scheme")             == puai.authScheme);
        CHECK(out.at("proxy_auth_info_auth_flag")               == std::to_string(puai.authFlag));
        CHECK(out.at("proxy_auth_info_flag")                    == std::to_string(puai.flag));
        CHECK(out.at("proxy_auth_info_ppid")                    == std::to_string(puai.ppid));
        CHECK(out.at("proxy_auth_info_host")                    == puai.host);
        CHECK(out.at("proxy_auth_info_auth_str")                == puai.authStr);

        CHECK(out.at("proxy_user_other_info_user_info")         == puuoi.userInfo);
        CHECK(out.at("proxy_user_other_info_user_comments")     == puuoi.userComments);
        CHECK(out.at("proxy_user_other_info_user_create")       == puuoi.userCreate);
        CHECK(out.at("proxy_user_other_info_user_modify")       == puuoi.userModify);

        CHECK(out.at("user_user_name")                         == cu.userName);
        CHECK(out.at("user_rods_zone")                         == cu.rodsZone);
        CHECK(out.at("user_user_type")                         == cu.userType);
        CHECK(out.at("user_sys_uid")                           == std::to_string(cu.sysUid));

        CHECK(out.at("user_auth_info_auth_scheme")             == cuai.authScheme);
        CHECK(out.at("user_auth_info_auth_flag")               == std::to_string(cuai.authFlag));
        CHECK(out.at("user_auth_info_flag")                    == std::to_string(cuai.flag));
        CHECK(out.at("user_auth_info_ppid")                    == std::to_string(cuai.ppid));
        CHECK(out.at("user_auth_info_host")                    == cuai.host);
        CHECK(out.at("user_auth_info_auth_str")                == cuai.authStr);

        CHECK(out.at("user_user_other_info_user_info")         == cuuoi.userInfo);
        CHECK(out.at("user_user_other_info_user_comments")     == cuuoi.userComments);
        CHECK(out.at("user_user_other_info_user_create")       == cuuoi.userCreate);
        CHECK(out.at("user_user_other_info_user_modify")       == cuuoi.userModify);

        CHECK(out.at("socket")          == std::to_string(rscp->sock));
        CHECK(out.at("connect_count")   == std::to_string(rscp->connectCnt));
        CHECK(out.at("status")          == std::to_string(rscp->status));
        CHECK(out.at("api_index")       == std::to_string(rscp->apiInx));
        CHECK(out.at("option")          == rscp->option);
    }
}
