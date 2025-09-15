#include <catch2/catch_all.hpp>

#include "irods/apiNumber.h"
#include "irods/client_connection.hpp"
#include "irods/dataObjInpOut.h"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/packStruct.h"
#include "irods/procApiRequest.h"
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"

#include <array>
#include <cstring>
#include <string_view>

TEST_CASE("#8653: procApiRequest_raw allows overriding output packing instructions")
{
    load_client_api_plugins();

    // This is a modified version of the output structure used by rcGetMiscSvrInfo.
    // It purposefully removes the member variables for storing the zone name and
    // certificate information to prove procApiRequest_raw is capable of changing the
    // expectations around input and output data for an API.
    struct api_output
    {
        int serverType;
        uint serverBootTime;
        char relVersion[NAME_LEN];
        char apiVersion[NAME_LEN];
    };

    // We use an unknown packing instruction name for the output structure to show
    // how deserialization is decoupled from the API number.
    constexpr const char* pi_name = "api_output_PI";

    // This is the packing instruction definition. It tells the PackStruct API how
    // to interpret the received bytes and fill in the given output structure.
    constexpr const char* pi_instruction = "int serverType; "
                                           "int serverBootTime; "
                                           "str relVersion[NAME_LEN]; "
                                           "str apiVersion[NAME_LEN];";

    // clang-format off
    // The custom packing instruction table to use for deserialization of the output
    // structure. Notice only the new type is defined. The PackStruct API will use
    // the global packing instruction table for any types which are not defined here.
    const auto pi_table = std::to_array<PackingInstruction>({
        {pi_name, pi_instruction, nullptr},
        {PACK_TABLE_END_PI, nullptr, nullptr}
    });
    // clang-format on

    api_output* output{};
    // NOLINTNEXTLINE(*-owning-memory, *-no-malloc)
    irods::at_scope_exit free_output{[&output] { std::free(output); }};

    irods::experimental::client_connection conn;
    auto* conn_ptr = static_cast<RcComm*>(conn);

    const auto ec = procApiRequest_raw(conn_ptr,
                                       GET_MISC_SVR_INFO_AN,
                                       pi_table.data(),
                                       nullptr,
                                       nullptr,
                                       nullptr,
                                       pi_name,
                                       static_cast<void**>(static_cast<void*>(&output)),
                                       nullptr);
    REQUIRE(ec == 0);
    CHECK(output->relVersion == std::string_view{conn_ptr->svrVersion->relVersion});
    CHECK(output->apiVersion == std::string_view{conn_ptr->svrVersion->apiVersion});
}

TEST_CASE("#8653: procApiRequest_raw allows overriding packing instructions")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    DataObjInp input{};
    std::strncpy(input.objPath, env.rodsHome, sizeof(DataObjInp::objPath) - 1);

    rodsObjStat* output{};
    // NOLINTNEXTLINE(*-owning-memory, *-no-malloc)
    irods::at_scope_exit free_output{[&output] { std::free(output); }};

    irods::experimental::client_connection conn;
    auto* conn_ptr = static_cast<RcComm*>(conn);

    // The names associated with custom packing instructions.
    constexpr const char* pi_name_in = "api_input_PI";
    constexpr const char* pi_name_out = "api_output_PI";

    SECTION("truncated input packing instruction")
    {
        constexpr const char* pi_instruction_in = "str objPath[MAX_NAME_LEN];";

        // clang-format off
        const auto pi_table = std::to_array<PackingInstruction>({
            {pi_name_in, pi_instruction_in, nullptr},
            {PACK_TABLE_END_PI, nullptr, nullptr}
        });
        // clang-format on

        const auto ec = procApiRequest_raw(conn_ptr,
                                           OBJ_STAT_AN,
                                           pi_table.data(),
                                           pi_name_in,
                                           &input,
                                           nullptr,
                                           "RodsObjStat_PI", // For deserialization.
                                           static_cast<void**>(static_cast<void*>(&output)),
                                           nullptr);
        REQUIRE(ec == COLL_OBJ_T);
    }

    SECTION("bad input packing instruction")
    {
        constexpr const char* pi_instruction_in = "bad_type objPath[MAX_NAME_LEN];";

        // clang-format off
        const auto pi_table = std::to_array<PackingInstruction>({
            {pi_name_in, pi_instruction_in, nullptr},
            {PACK_TABLE_END_PI, nullptr, nullptr}
        });
        // clang-format on

        const auto ec = procApiRequest_raw(conn_ptr,
                                           OBJ_STAT_AN,
                                           pi_table.data(),
                                           pi_name_in,
                                           &input,
                                           nullptr,
                                           "RodsObjStat_PI", // For deserialization.
                                           static_cast<void**>(static_cast<void*>(&output)),
                                           nullptr);
        REQUIRE(ec == SYS_PACK_INSTRUCT_FORMAT_ERR);
    }

    SECTION("bad output packing instruction")
    {
        constexpr const char* pi_instruction_out = "double objSize; "
                                                   "int objType; "
                                                   "bad_type dataMode; "
                                                   "str dataId[NAME_LEN]; "
                                                   "str chksum[NAME_LEN]; "
                                                   "str ownerName[NAME_LEN]; "
                                                   "str ownerZone[NAME_LEN]; "
                                                   "str createTime[TIME_LEN]; "
                                                   "str modifyTime[TIME_LEN]; "
                                                   "struct *SpecColl_PI; "
                                                   "int unknown_field;";

        // clang-format off
        const auto pi_table = std::to_array<PackingInstruction>({
            {pi_name_out, pi_instruction_out, nullptr},
            {PACK_TABLE_END_PI, nullptr, nullptr}
        });
        // clang-format on

        const auto ec = procApiRequest_raw(conn_ptr,
                                           OBJ_STAT_AN,
                                           pi_table.data(),
                                           "DataObjInp_PI", // For serialization.
                                           &input,
                                           nullptr,
                                           pi_name_out,
                                           static_cast<void**>(static_cast<void*>(&output)),
                                           nullptr);
        REQUIRE(ec == SYS_PACK_INSTRUCT_FORMAT_ERR);
    }
}
