#include "irods/client_api_allowlist.hpp"

#include "irods/apiNumber.h"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/rodsErrorTable.h"

#include <exception>
#include <vector>

namespace
{
    using log_api = irods::experimental::log::api;

    std::vector<int> g_api_numbers; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    auto is_client_to_agent_connection() -> bool
    {
        return !irods::server_property_exists(irods::AGENT_CONN_KW);
    } // is_client_to_agent_connection
} // anonymous namespace

namespace irods::client_api_allowlist
{
    auto init() -> void
    {
        // clang-format off
        g_api_numbers = {
            //FILE_CREATE_AN,
            //FILE_OPEN_AN,
            //FILE_WRITE_AN,
            //FILE_CLOSE_AN,
            //FILE_LSEEK_AN,
            //FILE_READ_AN,
            //FILE_UNLINK_AN,
            //FILE_MKDIR_AN,
            //FILE_CHMOD_AN,
            //FILE_RMDIR_AN,
            //FILE_STAT_AN,
            //FILE_FSTAT_AN,
            //FILE_FSYNC_AN,

            //FILE_STAGE_AN,
            //FILE_GET_FS_FREE_SPACE_AN,
            //FILE_OPENDIR_AN,
            //FILE_CLOSEDIR_AN,
            //FILE_READDIR_AN,
            //FILE_PUT_AN,
            //FILE_GET_AN,
            //FILE_CHKSUM_AN,
            //CHK_N_V_PATH_PERM_AN,
            //FILE_RENAME_AN,
            //FILE_TRUNCATE_AN,
            //FILE_STAGE_TO_CACHE_AN,
            //FILE_SYNC_TO_ARCH_AN,

            DATA_OBJ_CREATE_AN,
            DATA_OBJ_OPEN_AN,
            DATA_OBJ_PUT_AN,
            DATA_PUT_AN,
            DATA_OBJ_GET_AN,
            DATA_GET_AN,
            //DATA_COPY_AN,
            DATA_OBJ_UNLINK_AN,
            REG_DATA_OBJ_AN,
            UNREG_DATA_OBJ_AN,
            REG_REPLICA_AN,
            //MOD_DATA_OBJ_META_AN,
            RULE_EXEC_SUBMIT_AN,
            RULE_EXEC_DEL_AN,
            EXEC_MY_RULE_AN,
            OPR_COMPLETE_AN,
            DATA_OBJ_RENAME_AN,
            DATA_OBJ_RSYNC_AN,
            DATA_OBJ_CHKSUM_AN,
            PHY_PATH_REG_AN,
            DATA_OBJ_TRIM_AN,
            OBJ_STAT_AN,
            SUB_STRUCT_FILE_CREATE_AN,
            SUB_STRUCT_FILE_OPEN_AN,
            SUB_STRUCT_FILE_READ_AN,
            SUB_STRUCT_FILE_WRITE_AN,
            SUB_STRUCT_FILE_CLOSE_AN,
            SUB_STRUCT_FILE_UNLINK_AN,
            SUB_STRUCT_FILE_STAT_AN,
            SUB_STRUCT_FILE_FSTAT_AN,
            SUB_STRUCT_FILE_LSEEK_AN,
            SUB_STRUCT_FILE_RENAME_AN,
            QUERY_SPEC_COLL_AN,
            SUB_STRUCT_FILE_MKDIR_AN,
            SUB_STRUCT_FILE_RMDIR_AN,
            SUB_STRUCT_FILE_OPENDIR_AN,
            SUB_STRUCT_FILE_READDIR_AN,
            SUB_STRUCT_FILE_CLOSEDIR_AN,
            DATA_OBJ_TRUNCATE_AN,
            SUB_STRUCT_FILE_TRUNCATE_AN,
            //GET_XMSG_TICKET_AN,
            //SEND_XMSG_AN,
            //RCV_XMSG_AN,
            SUB_STRUCT_FILE_GET_AN,
            SUB_STRUCT_FILE_PUT_AN,
            SYNC_MOUNTED_COLL_AN,
            STRUCT_FILE_SYNC_AN,
            CLOSE_COLLECTION_AN,
            STRUCT_FILE_EXTRACT_AN,
            STRUCT_FILE_EXT_AND_REG_AN,
            STRUCT_FILE_BUNDLE_AN,
            CHK_OBJ_PERM_AND_STAT_AN,
            GET_REMOTE_ZONE_RESC_AN,
            DATA_OBJ_OPEN_AND_STAT_AN,
            //L3_FILE_GET_SINGLE_BUF_AN,
            L3_FILE_PUT_SINGLE_BUF_AN,
            DATA_OBJ_CREATE_AND_STAT_AN,
            DATA_OBJ_CLOSE_AN,
            DATA_OBJ_LSEEK_AN,
            DATA_OBJ_READ_AN,
            DATA_OBJ_WRITE_AN,
            COLL_REPL_AN,
            OPEN_COLLECTION_AN,
            RM_COLL_AN,
            MOD_COLL_AN,
            COLL_CREATE_AN,
            REG_COLL_AN,
            PHY_BUNDLE_COLL_AN,
            UNBUN_AND_REG_PHY_BUNFILE_AN,
            GET_HOST_FOR_PUT_AN,
            GET_RESC_QUOTA_AN,
            BULK_DATA_OBJ_REG_AN,
            BULK_DATA_OBJ_PUT_AN,
            PROC_STAT_AN,
            //STREAM_READ_AN,
            //EXEC_CMD_AN,
            //STREAM_CLOSE_AN,
            GET_HOST_FOR_GET_AN,
            DATA_OBJ_REPL_AN,
            DATA_OBJ_COPY_AN,
            DATA_OBJ_PHYMV_AN,
            DATA_OBJ_FSYNC_AN,

            GET_MISC_SVR_INFO_AN,
            GENERAL_ADMIN_AN,
            GEN_QUERY_AN,
            AUTH_REQUEST_AN,
            AUTH_RESPONSE_AN,
            AUTH_CHECK_AN,
            MOD_AVU_METADATA_AN,
            MOD_ACCESS_CONTROL_AN,
            RULE_EXEC_MOD_AN,
            GET_TEMP_PASSWORD_AN,
            READ_COLLECTION_AN,
            USER_ADMIN_AN,
            //GENERAL_ROW_INSERT_AN,
            //GENERAL_ROW_PURGE_AN,
            //END_TRANSACTION_AN,
            //DATABASE_RESC_OPEN_AN,
            //DATABASE_OBJ_CONTROL_AN,
            //DATABASE_RESC_CLOSE_AN,
            SPECIFIC_QUERY_AN,
            TICKET_ADMIN_AN,
            GET_TEMP_PASSWORD_FOR_OTHER_AN,
            PAM_AUTH_REQUEST_AN,
            GET_LIMITED_PASSWORD_AN,

            SSL_START_AN,
            SSL_END_AN,

            AUTH_PLUG_REQ_AN,
            AUTH_PLUG_RESP_AN,
            GET_HIER_FOR_RESC_AN,
            GET_HIER_FROM_LEAF_ID_AN,
            SET_RR_CTX_AN,
            EXEC_RULE_EXPRESSION_AN,

            SERVER_REPORT_AN,
            ZONE_REPORT_AN,
            CLIENT_HINTS_AN,

            GET_LIBRARY_FEATURES_AN,

            GET_RESOURCE_INFO_FOR_OPERATION_AN,
            REPLICA_TRUNCATE_AN,
            GENQUERY2_AN
        };
        // clang-format on
    } // init

    auto enforce(const RsComm& _comm) noexcept -> bool
    {
        try {
            if (irods::is_privileged_client(_comm)) {
                log_api::trace("Client has administrative privileges. Skipping client API allowlist.");
                return false;
            }

            if (!is_client_to_agent_connection()) {
                log_api::trace("Connection is a server-to-server connection. Skipping client API allowlist.");
                return false;
            }
        }
        catch (...) {
            // At this point, this function MUST NOT invoke anything that could result in an
            // exception being thrown. Failing to follow this rule can result in the application
            // being terminated immediately!
        }

        return true;
    } // enforce

    auto contains(int _api_number) noexcept -> bool
    {
        const auto end = std::cend(g_api_numbers);
        return std::find(std::cbegin(g_api_numbers), end, _api_number) != end;
    } // contains

    auto add(int api_number) -> void
    {
        if (contains(api_number)) {
            log_api::trace("API number [{}] has already been added to the client API allowlist", api_number);
            return;
        }

        try {
            g_api_numbers.push_back(api_number);
            log_api::trace("Added API number [{}] to the client API allowlist.", api_number);
        }
        catch (const std::exception& e) {
            log_api::error(
                "Could not add API number [{}] to client API allowlist [error_code={}, exception={}]",
                api_number,
                SYS_INTERNAL_ERR,
                e.what());
        }
    } // add
} // namespace irods::client_api_allowlist
