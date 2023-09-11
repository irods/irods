#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/connection_pool.hpp"
#include "irods/data_object_modify_info.h"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_client_api_table.hpp"
#include "irods/irods_pack_table.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"
#include "irods/transport/default_transport.hpp"

TEST_CASE("data_object_modify_info")
{
    namespace fs = irods::experimental::filesystem;
    namespace io = irods::experimental::io;

    auto& api_table = irods::get_client_api_table();
    auto& pack_table = irods::get_pack_table();
    init_api_table(api_table, pack_table);

    auto conn_pool = irods::make_connection_pool();
    auto conn = conn_pool->get_connection();

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "irods_unit_tests_sandbox";
    const auto path = sandbox / "dstream_data_object.txt";

    fs::client::create_collection(conn, sandbox);

    irods::at_scope_exit cleanup{[&] {
        fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash);
    }};

    // Create a data object in iRODS.
    {
        io::client::default_transport tp{conn};
        io::odstream out{tp, path};
    }

    dataObjInfo_t info{};
    std::strcpy(info.objPath, path.c_str());

    const auto invalid_keywords = {
        CHKSUM_KW,
        COLL_ID_KW,
        //DATA_COMMENTS_KW,
        DATA_CREATE_KW,
        //DATA_EXPIRY_KW,
        DATA_ID_KW,
        DATA_MAP_ID_KW,
        DATA_MODE_KW,
        //DATA_MODIFY_KW,
        DATA_NAME_KW,
        DATA_OWNER_KW,
        DATA_OWNER_ZONE_KW,
        //DATA_RESC_GROUP_NAME_KW, // Not defined.
        DATA_SIZE_KW,
        //DATA_TYPE_KW,
        FILE_PATH_KW,
        REPL_NUM_KW,
        REPL_STATUS_KW,
        RESC_HIER_STR_KW,
        RESC_ID_KW,
        RESC_NAME_KW,
        STATUS_STRING_KW,
        VERSION_KW
    };

    for (auto&& kw : invalid_keywords) {
        DYNAMIC_SECTION("error on invalid keyword [" << kw << ']')
        {
            keyValPair_t reg_params{};
            addKeyVal(&reg_params, kw, "");

            modDataObjMeta_t input{};
            input.dataObjInfo = &info;
            input.regParam = &reg_params;

            REQUIRE(rc_data_object_modify_info(static_cast<rcComm_t*>(conn), &input) == USER_BAD_KEYWORD_ERR);
        }
    }
}

TEST_CASE("#7338")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn{irods::experimental::defer_authentication};

    modDataObjMeta_t input{};
    CHECK(rc_data_object_modify_info(static_cast<RcComm*>(conn), &input) == SYS_NO_API_PRIV);
}
