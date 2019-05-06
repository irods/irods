/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsErrorTable.h - common header file for rods server and agents
 */

/**
 * @file  rodsErrorTable.h
 *
 * @brief Defines ERRORS for iRODS server and agents
 */

#ifndef RODS_ERROR_TABLE_H__
#define RODS_ERROR_TABLE_H__

/**
 * IMPORTANT - END OF LIFE ERROR CODES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Error codes having a prefix of "END_OF_LIFE_" must NEVER be used
 * in new code.
 *
 * This prefix is an indicator to the implementer that the error code
 * is no longer used by iRODS.
 *
 * These error codes only exist as placeholders.
 */

/**
 * @defgroup error_codes iRODS ERROR Codes
 * @note ERROR code format:
 *
 *      -mmmmnnn
 *
 * Example:    SYS_SOCK_OPEN_ERR -1000
 *
 * Where -mmmm000 is an iRODS ERROR Code
 *
 * This error (-1000) is the error that occurs when a
 * socket open call failed. Here mmmm = 1.
 *
 * nnn is the errno associated with the socket open call.
 *
 * If the errno is 34, then the error returned to the user
 * is -1034. iRODS uses 3 digits for nnn because the errno
 * is less than 1000.
 *
 */

#ifdef MAKE_IRODS_ERROR_MAP
#include <map>
#include <string>
namespace {
    namespace irods_error_map_construction {
        std::map<const int, const std::string> irods_error_map;
        std::map<const std::string, const int> irods_error_name_map;

        //We pass the variable as a const reference here to silence
        //unused variable warnings in a controlled manner
        int create_error( const std::string& err_name, const int err_code, const int& ) {
            irods_error_map.insert( std::pair<const int, const std::string>( err_code, err_name ) );
            irods_error_name_map.insert( std::pair<const std::string, const int>( err_name, err_code ) );
            return err_code;
        }
    }
}
#define NEW_ERROR(err_name, err_code) const int err_name = irods_error_map_construction::create_error( #err_name, err_code, err_name );
#else
#define NEW_ERROR(err_name, err_code) err_name = err_code,
enum IRODS_ERROR_ENUM
{
#endif

// clang-format off

/* 1,000 - 299,000 - system type */
/** @defgroup system_errors System ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 1,000 - 299,000
 * @{
 */
NEW_ERROR(SYS_SOCK_OPEN_ERR,                           -1000)
NEW_ERROR(SYS_SOCK_LISTEN_ERR,                         -1100)
NEW_ERROR(SYS_SOCK_BIND_ERR,                           -2000)
NEW_ERROR(SYS_SOCK_ACCEPT_ERR,                         -3000)
NEW_ERROR(SYS_HEADER_READ_LEN_ERR,                     -4000)
NEW_ERROR(SYS_HEADER_WRITE_LEN_ERR,                    -5000)
NEW_ERROR(SYS_HEADER_TYPE_LEN_ERR,                     -6000)
NEW_ERROR(SYS_CAUGHT_SIGNAL,                           -7000)
NEW_ERROR(SYS_GETSTARTUP_PACK_ERR,                     -8000)
NEW_ERROR(SYS_EXCEED_CONNECT_CNT,                      -9000)
NEW_ERROR(SYS_USER_NOT_ALLOWED_TO_CONN,                -10000)
NEW_ERROR(SYS_READ_MSG_BODY_INPUT_ERR,                 -11000)
NEW_ERROR(SYS_UNMATCHED_API_NUM,                       -12000)
NEW_ERROR(SYS_NO_API_PRIV,                             -13000)
NEW_ERROR(SYS_API_INPUT_ERR,                           -14000)
NEW_ERROR(SYS_PACK_INSTRUCT_FORMAT_ERR,                -15000)
NEW_ERROR(SYS_MALLOC_ERR,                              -16000)
NEW_ERROR(SYS_GET_HOSTNAME_ERR,                        -17000)
NEW_ERROR(SYS_OUT_OF_FILE_DESC,                        -18000)
NEW_ERROR(SYS_FILE_DESC_OUT_OF_RANGE,                  -19000)
NEW_ERROR(SYS_UNRECOGNIZED_REMOTE_FLAG,                -20000)
NEW_ERROR(SYS_INVALID_SERVER_HOST,                     -21000)
NEW_ERROR(SYS_SVR_TO_SVR_CONNECT_FAILED,               -22000)
NEW_ERROR(SYS_BAD_FILE_DESCRIPTOR,                     -23000)
NEW_ERROR(SYS_INTERNAL_NULL_INPUT_ERR,                 -24000)
NEW_ERROR(SYS_CONFIG_FILE_ERR,                         -25000)
NEW_ERROR(SYS_INVALID_ZONE_NAME,                       -26000)
NEW_ERROR(SYS_COPY_LEN_ERR,                            -27000)
NEW_ERROR(SYS_PORT_COOKIE_ERR,                         -28000)
NEW_ERROR(SYS_KEY_VAL_TABLE_ERR,                       -29000)
NEW_ERROR(SYS_INVALID_RESC_TYPE,                       -30000)
NEW_ERROR(SYS_INVALID_FILE_PATH,                       -31000)
NEW_ERROR(SYS_INVALID_RESC_INPUT,                      -32000)
NEW_ERROR(SYS_INVALID_PORTAL_OPR,                      -33000)
NEW_ERROR(SYS_INVALID_OPR_TYPE,                        -35000)
NEW_ERROR(SYS_NO_PATH_PERMISSION,                      -36000)
NEW_ERROR(SYS_NO_ICAT_SERVER_ERR,                      -37000)
NEW_ERROR(SYS_AGENT_INIT_ERR,                          -38000)
NEW_ERROR(SYS_PROXYUSER_NO_PRIV,                       -39000)
NEW_ERROR(SYS_NO_DATA_OBJ_PERMISSION,                  -40000)
NEW_ERROR(SYS_DELETE_DISALLOWED,                       -41000)
NEW_ERROR(SYS_OPEN_REI_FILE_ERR,                       -42000)
NEW_ERROR(SYS_NO_RCAT_SERVER_ERR,                      -43000)
NEW_ERROR(SYS_UNMATCH_PACK_INSTRUCTI_NAME,             -44000)
NEW_ERROR(SYS_SVR_TO_CLI_MSI_NO_EXIST,                 -45000)
NEW_ERROR(SYS_COPY_ALREADY_IN_RESC,                    -46000)
NEW_ERROR(SYS_RECONN_OPR_MISMATCH,                     -47000)
NEW_ERROR(SYS_INPUT_PERM_OUT_OF_RANGE,                 -48000)
NEW_ERROR(SYS_FORK_ERROR,                              -49000)
NEW_ERROR(SYS_PIPE_ERROR,                              -50000)
NEW_ERROR(SYS_EXEC_CMD_STATUS_SZ_ERROR,                -51000)
NEW_ERROR(SYS_PATH_IS_NOT_A_FILE,                      -52000)
NEW_ERROR(SYS_UNMATCHED_SPEC_COLL_TYPE,                -53000)
NEW_ERROR(SYS_TOO_MANY_QUERY_RESULT,                   -54000)
NEW_ERROR(SYS_SPEC_COLL_NOT_IN_CACHE,                  -55000)
NEW_ERROR(SYS_SPEC_COLL_OBJ_NOT_EXIST,                 -56000)
NEW_ERROR(SYS_REG_OBJ_IN_SPEC_COLL,                    -57000)
NEW_ERROR(SYS_DEST_SPEC_COLL_SUB_EXIST,                -58000)
NEW_ERROR(SYS_SRC_DEST_SPEC_COLL_CONFLICT,             -59000)
NEW_ERROR(SYS_UNKNOWN_SPEC_COLL_CLASS,                 -60000)
NEW_ERROR(END_OF_LIFE_SYS_DUPLICATE_XMSG_TICKET,       -61000) // EOL since iRODS v4.3.0
NEW_ERROR(END_OF_LIFE_SYS_UNMATCHED_XMSG_TICKET,       -62000) // EOL since iRODS v4.3.0
NEW_ERROR(END_OF_LIFE_SYS_NO_XMSG_FOR_MSG_NUMBER,      -63000) // EOL since iRODS v4.3.0
NEW_ERROR(SYS_COLLINFO_2_FORMAT_ERR,                   -64000)
NEW_ERROR(SYS_CACHE_STRUCT_FILE_RESC_ERR,              -65000)
NEW_ERROR(SYS_NOT_SUPPORTED,                           -66000)
NEW_ERROR(SYS_TAR_STRUCT_FILE_EXTRACT_ERR,             -67000)
NEW_ERROR(SYS_STRUCT_FILE_DESC_ERR,                    -68000)
NEW_ERROR(SYS_TAR_OPEN_ERR,                            -69000)
NEW_ERROR(SYS_TAR_EXTRACT_ALL_ERR,                     -70000)
NEW_ERROR(SYS_TAR_CLOSE_ERR,                           -71000)
NEW_ERROR(SYS_STRUCT_FILE_PATH_ERR,                    -72000)
NEW_ERROR(SYS_MOUNT_MOUNTED_COLL_ERR,                  -73000)
NEW_ERROR(SYS_COLL_NOT_MOUNTED_ERR,                    -74000)
NEW_ERROR(SYS_STRUCT_FILE_BUSY_ERR,                    -75000)
NEW_ERROR(SYS_STRUCT_FILE_INMOUNTED_COLL,              -76000)
NEW_ERROR(SYS_COPY_NOT_EXIST_IN_RESC,                  -77000)
NEW_ERROR(SYS_RESC_DOES_NOT_EXIST,                     -78000)
NEW_ERROR(SYS_COLLECTION_NOT_EMPTY,                    -79000)
NEW_ERROR(SYS_OBJ_TYPE_NOT_STRUCT_FILE,                -80000)
NEW_ERROR(SYS_WRONG_RESC_POLICY_FOR_BUN_OPR,           -81000)
NEW_ERROR(SYS_DIR_IN_VAULT_NOT_EMPTY,                  -82000)
NEW_ERROR(SYS_OPR_FLAG_NOT_SUPPORT,                    -83000)
NEW_ERROR(SYS_TAR_APPEND_ERR,                          -84000)
NEW_ERROR(SYS_INVALID_PROTOCOL_TYPE,                   -85000)
NEW_ERROR(SYS_UDP_CONNECT_ERR,                         -86000)
NEW_ERROR(SYS_UDP_TRANSFER_ERR,                        -89000)
NEW_ERROR(SYS_UDP_NO_SUPPORT_ERR,                      -90000)
NEW_ERROR(SYS_READ_MSG_BODY_LEN_ERR,                   -91000)
NEW_ERROR(CROSS_ZONE_SOCK_CONNECT_ERR,                 -92000)
NEW_ERROR(SYS_NO_FREE_RE_THREAD,                       -93000)
NEW_ERROR(SYS_BAD_RE_THREAD_INX,                       -94000)
NEW_ERROR(SYS_CANT_DIRECTLY_ACC_COMPOUND_RESC,         -95000)
NEW_ERROR(SYS_SRC_DEST_RESC_COMPOUND_TYPE,             -96000)
NEW_ERROR(SYS_CACHE_RESC_NOT_ON_SAME_HOST,             -97000)
NEW_ERROR(SYS_NO_CACHE_RESC_IN_GRP,                    -98000)
NEW_ERROR(SYS_UNMATCHED_RESC_IN_RESC_GRP,              -99000)
NEW_ERROR(SYS_CANT_MV_BUNDLE_DATA_TO_TRASH,            -100000)
NEW_ERROR(SYS_CANT_MV_BUNDLE_DATA_BY_COPY,             -101000)
NEW_ERROR(SYS_EXEC_TAR_ERR,                            -102000)
NEW_ERROR(SYS_CANT_CHKSUM_COMP_RESC_DATA,              -103000)
NEW_ERROR(SYS_CANT_CHKSUM_BUNDLED_DATA,                -104000)
NEW_ERROR(SYS_RESC_IS_DOWN,                            -105000)
NEW_ERROR(SYS_UPDATE_REPL_INFO_ERR,                    -106000)
NEW_ERROR(SYS_COLL_LINK_PATH_ERR,                      -107000)
NEW_ERROR(SYS_LINK_CNT_EXCEEDED_ERR,                   -108000)
NEW_ERROR(SYS_CROSS_ZONE_MV_NOT_SUPPORTED,             -109000)
NEW_ERROR(SYS_RESC_QUOTA_EXCEEDED,                     -110000)
NEW_ERROR(SYS_RENAME_STRUCT_COUNT_EXCEEDED,            -111000)
NEW_ERROR(SYS_BULK_REG_COUNT_EXCEEDED,                 -112000)
NEW_ERROR(SYS_REQUESTED_BUF_TOO_LARGE,                 -113000)
NEW_ERROR(SYS_INVALID_RESC_FOR_BULK_OPR,               -114000)
NEW_ERROR(SYS_SOCK_READ_TIMEDOUT,                      -115000)
NEW_ERROR(SYS_SOCK_READ_ERR,                           -116000)
NEW_ERROR(SYS_CONNECT_CONTROL_CONFIG_ERR,              -117000)
NEW_ERROR(SYS_MAX_CONNECT_COUNT_EXCEEDED,              -118000)
NEW_ERROR(SYS_STRUCT_ELEMENT_MISMATCH,                 -119000)
NEW_ERROR(SYS_PHY_PATH_INUSE,                          -120000)
NEW_ERROR(SYS_USER_NO_PERMISSION,                      -121000)
NEW_ERROR(SYS_USER_RETRIEVE_ERR,                       -122000)
NEW_ERROR(SYS_FS_LOCK_ERR,                             -123000) // JMC - backport 4598
NEW_ERROR(SYS_LOCK_TYPE_INP_ERR,                       -124000) // JMC - backport 4599
NEW_ERROR(SYS_LOCK_CMD_INP_ERR,                        -125000) // JMC - backport 4599
NEW_ERROR(SYS_ZIP_FORMAT_NOT_SUPPORTED,                -126000) // JMC - backport 4635
NEW_ERROR(SYS_ADD_TO_ARCH_OPR_NOT_SUPPORTED,           -127000) // JMC - backport 4643
NEW_ERROR(CANT_REG_IN_VAULT_FILE,                      -128000) // JMC - backport 4774
NEW_ERROR(PATH_REG_NOT_ALLOWED,                        -129000) // JMC - backport 4774
NEW_ERROR(SYS_INVALID_INPUT_PARAM,                     -130000) // JMC - backport 4774
NEW_ERROR(SYS_GROUP_RETRIEVE_ERR,                      -131000)
NEW_ERROR(SYS_MSSO_APPEND_ERR,                         -132000)
NEW_ERROR(SYS_MSSO_STRUCT_FILE_EXTRACT_ERR,            -133000)
NEW_ERROR(SYS_MSSO_EXTRACT_ALL_ERR,                    -134000)
NEW_ERROR(SYS_MSSO_OPEN_ERR,                           -135000)
NEW_ERROR(SYS_MSSO_CLOSE_ERR,                          -136000)
NEW_ERROR(SYS_RULE_NOT_FOUND,                          -144000) // JMC
NEW_ERROR(SYS_NOT_IMPLEMENTED,                         -146000) // JMC
NEW_ERROR(SYS_SIGNED_SID_NOT_MATCHED,                  -147000) // JMC
NEW_ERROR(SYS_HASH_IMMUTABLE,                          -148000)
NEW_ERROR(SYS_UNINITIALIZED,                           -149000)
NEW_ERROR(SYS_NEGATIVE_SIZE,                           -150000)
NEW_ERROR(SYS_ALREADY_INITIALIZED,                     -151000)
NEW_ERROR(SYS_SETENV_ERR,                              -152000)
NEW_ERROR(SYS_GETENV_ERR,                              -153000)
NEW_ERROR(SYS_INTERNAL_ERR,                            -154000)
NEW_ERROR(SYS_SOCK_SELECT_ERR,                         -155000)
NEW_ERROR(SYS_THREAD_ENCOUNTERED_INTERRUPT,            -156000)
NEW_ERROR(SYS_THREAD_RESOURCE_ERR,                     -157000)
NEW_ERROR(SYS_BAD_INPUT,                               -158000)
NEW_ERROR(SYS_PORT_RANGE_EXHAUSTED,                    -159000)
NEW_ERROR(SYS_SERVICE_ROLE_NOT_SUPPORTED,              -160000)
NEW_ERROR(SYS_SOCK_WRITE_ERR,                          -161000)
NEW_ERROR(SYS_SOCK_CONNECT_ERR,                        -162000)
NEW_ERROR(SYS_OPERATION_IN_PROGRESS,                   -163000)
NEW_ERROR(SYS_REPLICA_DOES_NOT_EXIST,                  -164000)
NEW_ERROR(SYS_UNKNOWN_ERROR,                           -165000)
/** @} */

/* 300,000 - 499,000 - user input type error */
/** @defgroup user_input_errors User Input ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 300,000 - 499,000
 * @{
 */
NEW_ERROR(USER_AUTH_SCHEME_ERR,                        -300000)
NEW_ERROR(USER_AUTH_STRING_EMPTY,                      -301000)
NEW_ERROR(USER_RODS_HOST_EMPTY,                        -302000)
NEW_ERROR(USER_RODS_HOSTNAME_ERR,                      -303000)
NEW_ERROR(USER_SOCK_OPEN_ERR,                          -304000)
NEW_ERROR(USER_SOCK_CONNECT_ERR,                       -305000)
NEW_ERROR(USER_STRLEN_TOOLONG,                         -306000)
NEW_ERROR(USER_API_INPUT_ERR,                          -307000)
NEW_ERROR(USER_PACKSTRUCT_INPUT_ERR,                   -308000)
NEW_ERROR(USER_NO_SUPPORT_ERR,                         -309000)
NEW_ERROR(USER_FILE_DOES_NOT_EXIST,                    -310000)
NEW_ERROR(USER_FILE_TOO_LARGE,                         -311000)
NEW_ERROR(OVERWRITE_WITHOUT_FORCE_FLAG,                -312000)
NEW_ERROR(UNMATCHED_KEY_OR_INDEX,                      -313000)
NEW_ERROR(USER_CHKSUM_MISMATCH,                        -314000)
NEW_ERROR(USER_BAD_KEYWORD_ERR,                        -315000)
NEW_ERROR(USER__NULL_INPUT_ERR,                        -316000)
NEW_ERROR(USER_INPUT_PATH_ERR,                         -317000)
NEW_ERROR(USER_INPUT_OPTION_ERR,                       -318000)
NEW_ERROR(USER_INVALID_USERNAME_FORMAT,                -319000)
NEW_ERROR(USER_DIRECT_RESC_INPUT_ERR,                  -320000)
NEW_ERROR(USER_NO_RESC_INPUT_ERR,                      -321000)
NEW_ERROR(USER_PARAM_LABEL_ERR,                        -322000)
NEW_ERROR(USER_PARAM_TYPE_ERR,                         -323000)
NEW_ERROR(BASE64_BUFFER_OVERFLOW,                      -324000)
NEW_ERROR(BASE64_INVALID_PACKET,                       -325000)
NEW_ERROR(USER_MSG_TYPE_NO_SUPPORT,                    -326000)
NEW_ERROR(USER_RSYNC_NO_MODE_INPUT_ERR,                -337000)
NEW_ERROR(USER_OPTION_INPUT_ERR,                       -338000)
NEW_ERROR(SAME_SRC_DEST_PATHS_ERR,                     -339000)
NEW_ERROR(USER_RESTART_FILE_INPUT_ERR,                 -340000)
NEW_ERROR(RESTART_OPR_FAILED,                          -341000)
NEW_ERROR(BAD_EXEC_CMD_PATH,                           -342000)
NEW_ERROR(EXEC_CMD_OUTPUT_TOO_LARGE,                   -343000)
NEW_ERROR(EXEC_CMD_ERROR,                              -344000)
NEW_ERROR(BAD_INPUT_DESC_INDEX,                        -345000)
NEW_ERROR(USER_PATH_EXCEEDS_MAX,                       -346000)
NEW_ERROR(USER_SOCK_CONNECT_TIMEDOUT,                  -347000)
NEW_ERROR(USER_API_VERSION_MISMATCH,                   -348000)
NEW_ERROR(USER_INPUT_FORMAT_ERR,                       -349000)
NEW_ERROR(USER_ACCESS_DENIED,                          -350000)
NEW_ERROR(CANT_RM_MV_BUNDLE_TYPE,                      -351000)
NEW_ERROR(NO_MORE_RESULT,                              -352000)
NEW_ERROR(NO_KEY_WD_IN_MS_INP_STR,                     -353000)
NEW_ERROR(CANT_RM_NON_EMPTY_HOME_COLL,                 -354000)
NEW_ERROR(CANT_UNREG_IN_VAULT_FILE,                    -355000)
NEW_ERROR(NO_LOCAL_FILE_RSYNC_IN_MSI,                  -356000)
NEW_ERROR(BULK_OPR_MISMATCH_FOR_RESTART,               -357000)
NEW_ERROR(OBJ_PATH_DOES_NOT_EXIST,                     -358000)
NEW_ERROR(SYMLINKED_BUNFILE_NOT_ALLOWED,               -359000) // JMC - backport 4833
NEW_ERROR(USER_INPUT_STRING_ERR,                       -360000)
NEW_ERROR(USER_INVALID_RESC_INPUT,                     -361000)
NEW_ERROR(USER_NOT_ALLOWED_TO_EXEC_CMD,                -370000)
NEW_ERROR(USER_HASH_TYPE_MISMATCH,                     -380000)
NEW_ERROR(USER_INVALID_CLIENT_ENVIRONMENT,             -390000)
NEW_ERROR(USER_INSUFFICIENT_FREE_INODES,               -400000)
NEW_ERROR(USER_FILE_SIZE_MISMATCH,                     -401000)
NEW_ERROR(USER_INCOMPATIBLE_PARAMS,                    -402000)
NEW_ERROR(USER_INVALID_REPLICA_INPUT,                  -403000)
/** @} */

/* 500,000 to 800,000 - file driver error */
/** @defgroup file_driver_errors File Driver ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 500,000 to 800,000
 * @{
 */
NEW_ERROR(FILE_INDEX_LOOKUP_ERR,                       -500000)
NEW_ERROR(UNIX_FILE_OPEN_ERR,                          -510000)
NEW_ERROR(UNIX_FILE_CREATE_ERR,                        -511000)
NEW_ERROR(UNIX_FILE_READ_ERR,                          -512000)
NEW_ERROR(UNIX_FILE_WRITE_ERR,                         -513000)
NEW_ERROR(UNIX_FILE_CLOSE_ERR,                         -514000)
NEW_ERROR(UNIX_FILE_UNLINK_ERR,                        -515000)
NEW_ERROR(UNIX_FILE_STAT_ERR,                          -516000)
NEW_ERROR(UNIX_FILE_FSTAT_ERR,                         -517000)
NEW_ERROR(UNIX_FILE_LSEEK_ERR,                         -518000)
NEW_ERROR(UNIX_FILE_FSYNC_ERR,                         -519000)
NEW_ERROR(UNIX_FILE_MKDIR_ERR,                         -520000)
NEW_ERROR(UNIX_FILE_RMDIR_ERR,                         -521000)
NEW_ERROR(UNIX_FILE_OPENDIR_ERR,                       -522000)
NEW_ERROR(UNIX_FILE_CLOSEDIR_ERR,                      -523000)
NEW_ERROR(UNIX_FILE_READDIR_ERR,                       -524000)
NEW_ERROR(UNIX_FILE_STAGE_ERR,                         -525000)
NEW_ERROR(UNIX_FILE_GET_FS_FREESPACE_ERR,              -526000)
NEW_ERROR(UNIX_FILE_CHMOD_ERR,                         -527000)
NEW_ERROR(UNIX_FILE_RENAME_ERR,                        -528000)
NEW_ERROR(UNIX_FILE_TRUNCATE_ERR,                      -529000)
NEW_ERROR(UNIX_FILE_LINK_ERR,                          -530000)
NEW_ERROR(UNIX_FILE_OPR_TIMEOUT_ERR,                   -540000)

/* universal MSS driver error */
NEW_ERROR(UNIV_MSS_SYNCTOARCH_ERR,                     -550000)
NEW_ERROR(UNIV_MSS_STAGETOCACHE_ERR,                   -551000)
NEW_ERROR(UNIV_MSS_UNLINK_ERR,                         -552000)
NEW_ERROR(UNIV_MSS_MKDIR_ERR,                          -553000)
NEW_ERROR(UNIV_MSS_CHMOD_ERR,                          -554000)
NEW_ERROR(UNIV_MSS_STAT_ERR,                           -555000)
NEW_ERROR(UNIV_MSS_RENAME_ERR,                         -556000)

NEW_ERROR(HPSS_AUTH_NOT_SUPPORTED,                     -600000)
NEW_ERROR(HPSS_FILE_OPEN_ERR,                          -610000)
NEW_ERROR(HPSS_FILE_CREATE_ERR,                        -611000)
NEW_ERROR(HPSS_FILE_READ_ERR,                          -612000)
NEW_ERROR(HPSS_FILE_WRITE_ERR,                         -613000)
NEW_ERROR(HPSS_FILE_CLOSE_ERR,                         -614000)
NEW_ERROR(HPSS_FILE_UNLINK_ERR,                        -615000)
NEW_ERROR(HPSS_FILE_STAT_ERR,                          -616000)
NEW_ERROR(HPSS_FILE_FSTAT_ERR,                         -617000)
NEW_ERROR(HPSS_FILE_LSEEK_ERR,                         -618000)
NEW_ERROR(HPSS_FILE_FSYNC_ERR,                         -619000)
NEW_ERROR(HPSS_FILE_MKDIR_ERR,                         -620000)
NEW_ERROR(HPSS_FILE_RMDIR_ERR,                         -621000)
NEW_ERROR(HPSS_FILE_OPENDIR_ERR,                       -622000)
NEW_ERROR(HPSS_FILE_CLOSEDIR_ERR,                      -623000)
NEW_ERROR(HPSS_FILE_READDIR_ERR,                       -624000)
NEW_ERROR(HPSS_FILE_STAGE_ERR,                         -625000)
NEW_ERROR(HPSS_FILE_GET_FS_FREESPACE_ERR,              -626000)
NEW_ERROR(HPSS_FILE_CHMOD_ERR,                         -627000)
NEW_ERROR(HPSS_FILE_RENAME_ERR,                        -628000)
NEW_ERROR(HPSS_FILE_TRUNCATE_ERR,                      -629000)
NEW_ERROR(HPSS_FILE_LINK_ERR,                          -630000)
NEW_ERROR(HPSS_AUTH_ERR,                               -631000)
NEW_ERROR(HPSS_WRITE_LIST_ERR,                         -632000)
NEW_ERROR(HPSS_READ_LIST_ERR,                          -633000)
NEW_ERROR(HPSS_TRANSFER_ERR,                           -634000)
NEW_ERROR(HPSS_MOVER_PROT_ERR,                         -635000)

/* Amazon S3 error */
NEW_ERROR(S3_INIT_ERROR,                               -701000)
NEW_ERROR(S3_PUT_ERROR,                                -702000)
NEW_ERROR(S3_GET_ERROR,                                -703000)
NEW_ERROR(S3_FILE_UNLINK_ERR,                          -715000)
NEW_ERROR(S3_FILE_STAT_ERR,                            -716000)
NEW_ERROR(S3_FILE_COPY_ERR,                            -717000)

/* DDN WOS error */
NEW_ERROR(WOS_PUT_ERR,                                 -750000)
NEW_ERROR(WOS_STREAM_PUT_ERR,                          -751000)
NEW_ERROR(WOS_STREAM_CLOSE_ERR,                        -752000)
NEW_ERROR(WOS_GET_ERR,                                 -753000)
NEW_ERROR(WOS_STREAM_GET_ERR,                          -754000)
NEW_ERROR(WOS_UNLINK_ERR,                              -755000)
NEW_ERROR(WOS_STAT_ERR,                                -756000)
NEW_ERROR(WOS_CONNECT_ERR,                             -757000)

/* Hadoop HDFS error */

NEW_ERROR(HDFS_FILE_OPEN_ERR,                          -730000)
NEW_ERROR(HDFS_FILE_CREATE_ERR,                        -731000)
NEW_ERROR(HDFS_FILE_READ_ERR,                          -732000)
NEW_ERROR(HDFS_FILE_WRITE_ERR,                         -733000)
NEW_ERROR(HDFS_FILE_CLOSE_ERR,                         -734000)
NEW_ERROR(HDFS_FILE_UNLINK_ERR,                        -735000)
NEW_ERROR(HDFS_FILE_STAT_ERR,                          -736000)
NEW_ERROR(HDFS_FILE_FSTAT_ERR,                         -737000)
NEW_ERROR(HDFS_FILE_LSEEK_ERR,                         -738000)
NEW_ERROR(HDFS_FILE_FSYNC_ERR,                         -739000)
NEW_ERROR(HDFS_FILE_MKDIR_ERR,                         -741000)
NEW_ERROR(HDFS_FILE_RMDIR_ERR,                         -742000)
NEW_ERROR(HDFS_FILE_OPENDIR_ERR,                       -743000)
NEW_ERROR(HDFS_FILE_CLOSEDIR_ERR,                      -744000)
NEW_ERROR(HDFS_FILE_READDIR_ERR,                       -745000)
NEW_ERROR(HDFS_FILE_STAGE_ERR,                         -746000)
NEW_ERROR(HDFS_FILE_GET_FS_FREESPACE_ERR,              -746000)
NEW_ERROR(HDFS_FILE_CHMOD_ERR,                         -748000)
NEW_ERROR(HDFS_FILE_RENAME_ERR,                        -749000)
NEW_ERROR(HDFS_FILE_TRUNCATE_ERR,                      -760000)
NEW_ERROR(HDFS_FILE_LINK_ERR,                          -761000)
NEW_ERROR(HDFS_FILE_OPR_TIMEOUT_ERR,                   -762000)

/* Direct Access vault error */
NEW_ERROR(DIRECT_ACCESS_FILE_USER_INVALID_ERR,         -770000)
/** @} */

/* 800,000 to 880,000 - Catalog library errors  */
/** @defgroup catalog_library_errors Catalog Library ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 800,000 to 880,000
 * @{
 */
NEW_ERROR(CATALOG_NOT_CONNECTED,                       -801000)
NEW_ERROR(CAT_ENV_ERR,                                 -802000)
NEW_ERROR(CAT_CONNECT_ERR,                             -803000)
NEW_ERROR(CAT_DISCONNECT_ERR,                          -804000)
NEW_ERROR(CAT_CLOSE_ENV_ERR,                           -805000)
NEW_ERROR(CAT_SQL_ERR,                                 -806000)
NEW_ERROR(CAT_GET_ROW_ERR,                             -807000)
NEW_ERROR(CAT_NO_ROWS_FOUND,                           -808000)
NEW_ERROR(CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME,       -809000)
NEW_ERROR(CAT_INVALID_RESOURCE_TYPE,                   -810000)
NEW_ERROR(CAT_INVALID_RESOURCE_CLASS,                  -811000)
NEW_ERROR(CAT_INVALID_RESOURCE_NET_ADDR,               -812000)
NEW_ERROR(CAT_INVALID_RESOURCE_VAULT_PATH,             -813000)
NEW_ERROR(CAT_UNKNOWN_COLLECTION,                      -814000)
NEW_ERROR(CAT_INVALID_DATA_TYPE,                       -815000)
NEW_ERROR(CAT_INVALID_ARGUMENT,                        -816000)
NEW_ERROR(CAT_UNKNOWN_FILE,                            -817000)
NEW_ERROR(CAT_NO_ACCESS_PERMISSION,                    -818000)
NEW_ERROR(CAT_SUCCESS_BUT_WITH_NO_INFO,                -819000)
NEW_ERROR(CAT_INVALID_USER_TYPE,                       -820000)
NEW_ERROR(CAT_COLLECTION_NOT_EMPTY,                    -821000)
NEW_ERROR(CAT_TOO_MANY_TABLES,                         -822000)
NEW_ERROR(CAT_UNKNOWN_TABLE,                           -823000)
NEW_ERROR(CAT_NOT_OPEN,                                -824000)
NEW_ERROR(CAT_FAILED_TO_LINK_TABLES,                   -825000)
NEW_ERROR(CAT_INVALID_AUTHENTICATION,                  -826000)
NEW_ERROR(CAT_INVALID_USER,                            -827000)
NEW_ERROR(CAT_INVALID_ZONE,                            -828000)
NEW_ERROR(CAT_INVALID_GROUP,                           -829000)
NEW_ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL,            -830000)
NEW_ERROR(CAT_INVALID_RESOURCE,                        -831000)
NEW_ERROR(CAT_INVALID_CLIENT_USER,                     -832000)
NEW_ERROR(CAT_NAME_EXISTS_AS_COLLECTION,               -833000)
NEW_ERROR(CAT_NAME_EXISTS_AS_DATAOBJ,                  -834000)
NEW_ERROR(CAT_RESOURCE_NOT_EMPTY,                      -835000)
NEW_ERROR(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION,      -836000)
NEW_ERROR(CAT_RECURSIVE_MOVE,                          -837000)
NEW_ERROR(CAT_LAST_REPLICA,                            -838000)
NEW_ERROR(CAT_OCI_ERROR,                               -839000)
NEW_ERROR(CAT_PASSWORD_EXPIRED,                        -840000)
NEW_ERROR(CAT_PASSWORD_ENCODING_ERROR,                 -850000)
NEW_ERROR(CAT_TABLE_ACCESS_DENIED,                     -851000)
NEW_ERROR(CAT_UNKNOWN_RESOURCE,                        -852000)
NEW_ERROR(CAT_UNKNOWN_SPECIFIC_QUERY,                  -853000)
NEW_ERROR(CAT_PSEUDO_RESC_MODIFY_DISALLOWED,           -854000) // JMC - backport 4629
NEW_ERROR(CAT_HOSTNAME_INVALID,                        -855000)
NEW_ERROR(CAT_BIND_VARIABLE_LIMIT_EXCEEDED,            -856000) // JMC - backport 484
NEW_ERROR(CAT_INVALID_CHILD,                           -857000)
NEW_ERROR(CAT_INVALID_OBJ_COUNT,                       -858000) // hcj
NEW_ERROR(CAT_INVALID_RESOURCE_NAME,                   -859000) // JMC
NEW_ERROR(CAT_STATEMENT_TABLE_FULL,                    -860000) // JMC
NEW_ERROR(CAT_RESOURCE_NAME_LENGTH_EXCEEDED,           -861000)
/** @} */

/* 880,000 to 889,000  Deprecated  */

/* 890,000 to 899,000  Ticket errors  */
/** @defgroup ticket_errors Ticket ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 890,000 to 899,000
 * @{
 */
NEW_ERROR(CAT_TICKET_INVALID,                          -890000)
NEW_ERROR(CAT_TICKET_EXPIRED,                          -891000)
NEW_ERROR(CAT_TICKET_USES_EXCEEDED,                    -892000)
NEW_ERROR(CAT_TICKET_USER_EXCLUDED,                    -893000)
NEW_ERROR(CAT_TICKET_HOST_EXCLUDED,                    -894000)
NEW_ERROR(CAT_TICKET_GROUP_EXCLUDED,                   -895000)
NEW_ERROR(CAT_TICKET_WRITE_USES_EXCEEDED,              -896000)
NEW_ERROR(CAT_TICKET_WRITE_BYTES_EXCEEDED,             -897000)
/** @} */

/* 900,000 to 920,000 - Misc errors (used by obf library, etc)  */
/** @defgroup miscellaneous_errors Miscellaneous ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 900,000 to 920,000
 * @{
 */
NEW_ERROR(FILE_OPEN_ERR,                               -900000)
NEW_ERROR(FILE_READ_ERR,                               -901000)
NEW_ERROR(FILE_WRITE_ERR,                              -902000)
NEW_ERROR(PASSWORD_EXCEEDS_MAX_SIZE,                   -903000)
NEW_ERROR(ENVIRONMENT_VAR_HOME_NOT_DEFINED,            -904000)
NEW_ERROR(UNABLE_TO_STAT_FILE,                         -905000)
NEW_ERROR(AUTH_FILE_NOT_ENCRYPTED,                     -906000)
NEW_ERROR(AUTH_FILE_DOES_NOT_EXIST,                    -907000)
NEW_ERROR(UNLINK_FAILED,                               -908000)
NEW_ERROR(NO_PASSWORD_ENTERED,                         -909000)
NEW_ERROR(REMOTE_SERVER_AUTHENTICATION_FAILURE,        -910000)
NEW_ERROR(REMOTE_SERVER_AUTH_NOT_PROVIDED,             -911000)
NEW_ERROR(REMOTE_SERVER_AUTH_EMPTY,                    -912000)
NEW_ERROR(REMOTE_SERVER_SID_NOT_DEFINED,               -913000)
/** @} */

/* 921,000 to 999,000 - GSI, KRB, OSAUTH, and PAM-AUTH errors  */
/** @defgroup authentication_errors Auth ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 921,000 to 999,000
 * @{
 */
NEW_ERROR(GSI_NOT_COMPILED_IN,                         -921000)
NEW_ERROR(GSI_NOT_BUILT_INTO_CLIENT,                   -922000)
NEW_ERROR(GSI_NOT_BUILT_INTO_SERVER,                   -923000)
NEW_ERROR(GSI_ERROR_IMPORT_NAME,                       -924000)
NEW_ERROR(GSI_ERROR_INIT_SECURITY_CONTEXT,             -925000)
NEW_ERROR(GSI_ERROR_SENDING_TOKEN_LENGTH,              -926000)
NEW_ERROR(GSI_ERROR_READING_TOKEN_LENGTH,              -927000)
NEW_ERROR(GSI_ERROR_TOKEN_TOO_LARGE,                   -928000)
NEW_ERROR(GSI_ERROR_BAD_TOKEN_RCVED,                   -929000)
NEW_ERROR(GSI_SOCKET_READ_ERROR,                       -930000)
NEW_ERROR(GSI_PARTIAL_TOKEN_READ,                      -931000)
NEW_ERROR(GSI_SOCKET_WRITE_ERROR,                      -932000)
NEW_ERROR(GSI_ERROR_FROM_GSI_LIBRARY,                  -933000)
NEW_ERROR(GSI_ERROR_IMPORTING_NAME,                    -934000)
NEW_ERROR(GSI_ERROR_ACQUIRING_CREDS,                   -935000)
NEW_ERROR(GSI_ACCEPT_SEC_CONTEXT_ERROR,                -936000)
NEW_ERROR(GSI_ERROR_DISPLAYING_NAME,                   -937000)
NEW_ERROR(GSI_ERROR_RELEASING_NAME,                    -938000)
NEW_ERROR(GSI_DN_DOES_NOT_MATCH_USER,                  -939000)
NEW_ERROR(GSI_QUERY_INTERNAL_ERROR,                    -940000)
NEW_ERROR(GSI_NO_MATCHING_DN_FOUND,                    -941000)
NEW_ERROR(GSI_MULTIPLE_MATCHING_DN_FOUND,              -942000)

NEW_ERROR(KRB_NOT_COMPILED_IN,                         -951000)
NEW_ERROR(KRB_NOT_BUILT_INTO_CLIENT,                   -952000)
NEW_ERROR(KRB_NOT_BUILT_INTO_SERVER,                   -953000)
NEW_ERROR(KRB_ERROR_IMPORT_NAME,                       -954000)
NEW_ERROR(KRB_ERROR_INIT_SECURITY_CONTEXT,             -955000)
NEW_ERROR(KRB_ERROR_SENDING_TOKEN_LENGTH,              -956000)
NEW_ERROR(KRB_ERROR_READING_TOKEN_LENGTH,              -957000)
NEW_ERROR(KRB_ERROR_TOKEN_TOO_LARGE,                   -958000)
NEW_ERROR(KRB_ERROR_BAD_TOKEN_RCVED,                   -959000)
NEW_ERROR(KRB_SOCKET_READ_ERROR,                       -960000)
NEW_ERROR(KRB_PARTIAL_TOKEN_READ,                      -961000)
NEW_ERROR(KRB_SOCKET_WRITE_ERROR,                      -962000)
NEW_ERROR(KRB_ERROR_FROM_KRB_LIBRARY,                  -963000)
NEW_ERROR(KRB_ERROR_IMPORTING_NAME,                    -964000)
NEW_ERROR(KRB_ERROR_ACQUIRING_CREDS,                   -965000)
NEW_ERROR(KRB_ACCEPT_SEC_CONTEXT_ERROR,                -966000)
NEW_ERROR(KRB_ERROR_DISPLAYING_NAME,                   -967000)
NEW_ERROR(KRB_ERROR_RELEASING_NAME,                    -968000)
NEW_ERROR(KRB_USER_DN_NOT_FOUND,                       -969000)
NEW_ERROR(KRB_NAME_MATCHES_MULTIPLE_USERS,             -970000)
NEW_ERROR(KRB_QUERY_INTERNAL_ERROR,                    -971000)

NEW_ERROR(OSAUTH_NOT_BUILT_INTO_CLIENT,                -981000)
NEW_ERROR(OSAUTH_NOT_BUILT_INTO_SERVER,                -982000)

NEW_ERROR(PAM_AUTH_NOT_BUILT_INTO_CLIENT,              -991000)
NEW_ERROR(PAM_AUTH_NOT_BUILT_INTO_SERVER,              -992000)
NEW_ERROR(PAM_AUTH_PASSWORD_FAILED,                    -993000)
NEW_ERROR(PAM_AUTH_PASSWORD_INVALID_TTL,               -994000)
/** @} */

/* 1,000,000 to 1,500,000  - Rule Engine errors */
/** @defgroup rule_engine_errors Rule Engine ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 1,000,000 to 1,500,000
 * @{
 */
NEW_ERROR(OBJPATH_EMPTY_IN_STRUCT_ERR,                 -1000000)
NEW_ERROR(RESCNAME_EMPTY_IN_STRUCT_ERR,                -1001000)
NEW_ERROR(DATATYPE_EMPTY_IN_STRUCT_ERR,                -1002000)
NEW_ERROR(DATASIZE_EMPTY_IN_STRUCT_ERR,                -1003000)
NEW_ERROR(CHKSUM_EMPTY_IN_STRUCT_ERR,                  -1004000)
NEW_ERROR(VERSION_EMPTY_IN_STRUCT_ERR,                 -1005000)
NEW_ERROR(FILEPATH_EMPTY_IN_STRUCT_ERR,                -1006000)
NEW_ERROR(REPLNUM_EMPTY_IN_STRUCT_ERR,                 -1007000)
NEW_ERROR(REPLSTATUS_EMPTY_IN_STRUCT_ERR,              -1008000)
NEW_ERROR(DATAOWNER_EMPTY_IN_STRUCT_ERR,               -1009000)
NEW_ERROR(DATAOWNERZONE_EMPTY_IN_STRUCT_ERR,           -1010000)
NEW_ERROR(DATAEXPIRY_EMPTY_IN_STRUCT_ERR,              -1011000)
NEW_ERROR(DATACOMMENTS_EMPTY_IN_STRUCT_ERR,            -1012000)
NEW_ERROR(DATACREATE_EMPTY_IN_STRUCT_ERR,              -1013000)
NEW_ERROR(DATAMODIFY_EMPTY_IN_STRUCT_ERR,              -1014000)
NEW_ERROR(DATAACCESS_EMPTY_IN_STRUCT_ERR,              -1015000)
NEW_ERROR(DATAACCESSINX_EMPTY_IN_STRUCT_ERR,           -1016000)
NEW_ERROR(NO_RULE_FOUND_ERR,                           -1017000)
NEW_ERROR(NO_MORE_RULES_ERR,                           -1018000)
NEW_ERROR(UNMATCHED_ACTION_ERR,                        -1019000)
NEW_ERROR(RULES_FILE_READ_ERROR,                       -1020000)
NEW_ERROR(ACTION_ARG_COUNT_MISMATCH,                   -1021000)
NEW_ERROR(MAX_NUM_OF_ARGS_IN_ACTION_EXCEEDED,          -1022000)
NEW_ERROR(UNKNOWN_PARAM_IN_RULE_ERR,                   -1023000)
NEW_ERROR(DESTRESCNAME_EMPTY_IN_STRUCT_ERR,            -1024000)
NEW_ERROR(BACKUPRESCNAME_EMPTY_IN_STRUCT_ERR,          -1025000)
NEW_ERROR(DATAID_EMPTY_IN_STRUCT_ERR,                  -1026000)
NEW_ERROR(COLLID_EMPTY_IN_STRUCT_ERR,                  -1027000)
NEW_ERROR(RESCGROUPNAME_EMPTY_IN_STRUCT_ERR,           -1028000)
NEW_ERROR(STATUSSTRING_EMPTY_IN_STRUCT_ERR,            -1029000)
NEW_ERROR(DATAMAPID_EMPTY_IN_STRUCT_ERR,               -1030000)
NEW_ERROR(USERNAMECLIENT_EMPTY_IN_STRUCT_ERR,          -1031000)
NEW_ERROR(RODSZONECLIENT_EMPTY_IN_STRUCT_ERR,          -1032000)
NEW_ERROR(USERTYPECLIENT_EMPTY_IN_STRUCT_ERR,          -1033000)
NEW_ERROR(HOSTCLIENT_EMPTY_IN_STRUCT_ERR,              -1034000)
NEW_ERROR(AUTHSTRCLIENT_EMPTY_IN_STRUCT_ERR,           -1035000)
NEW_ERROR(USERAUTHSCHEMECLIENT_EMPTY_IN_STRUCT_ERR,    -1036000)
NEW_ERROR(USERINFOCLIENT_EMPTY_IN_STRUCT_ERR,          -1037000)
NEW_ERROR(USERCOMMENTCLIENT_EMPTY_IN_STRUCT_ERR,       -1038000)
NEW_ERROR(USERCREATECLIENT_EMPTY_IN_STRUCT_ERR,        -1039000)
NEW_ERROR(USERMODIFYCLIENT_EMPTY_IN_STRUCT_ERR,        -1040000)
NEW_ERROR(USERNAMEPROXY_EMPTY_IN_STRUCT_ERR,           -1041000)
NEW_ERROR(RODSZONEPROXY_EMPTY_IN_STRUCT_ERR,           -1042000)
NEW_ERROR(USERTYPEPROXY_EMPTY_IN_STRUCT_ERR,           -1043000)
NEW_ERROR(HOSTPROXY_EMPTY_IN_STRUCT_ERR,               -1044000)
NEW_ERROR(AUTHSTRPROXY_EMPTY_IN_STRUCT_ERR,            -1045000)
NEW_ERROR(USERAUTHSCHEMEPROXY_EMPTY_IN_STRUCT_ERR,     -1046000)
NEW_ERROR(USERINFOPROXY_EMPTY_IN_STRUCT_ERR,           -1047000)
NEW_ERROR(USERCOMMENTPROXY_EMPTY_IN_STRUCT_ERR,        -1048000)
NEW_ERROR(USERCREATEPROXY_EMPTY_IN_STRUCT_ERR,         -1049000)
NEW_ERROR(USERMODIFYPROXY_EMPTY_IN_STRUCT_ERR,         -1050000)
NEW_ERROR(COLLNAME_EMPTY_IN_STRUCT_ERR,                -1051000)
NEW_ERROR(COLLPARENTNAME_EMPTY_IN_STRUCT_ERR,          -1052000)
NEW_ERROR(COLLOWNERNAME_EMPTY_IN_STRUCT_ERR,           -1053000)
NEW_ERROR(COLLOWNERZONE_EMPTY_IN_STRUCT_ERR,           -1054000)
NEW_ERROR(COLLEXPIRY_EMPTY_IN_STRUCT_ERR,              -1055000)
NEW_ERROR(COLLCOMMENTS_EMPTY_IN_STRUCT_ERR,            -1056000)
NEW_ERROR(COLLCREATE_EMPTY_IN_STRUCT_ERR,              -1057000)
NEW_ERROR(COLLMODIFY_EMPTY_IN_STRUCT_ERR,              -1058000)
NEW_ERROR(COLLACCESS_EMPTY_IN_STRUCT_ERR,              -1059000)
NEW_ERROR(COLLACCESSINX_EMPTY_IN_STRUCT_ERR,           -1060000)
NEW_ERROR(COLLMAPID_EMPTY_IN_STRUCT_ERR,               -1062000)
NEW_ERROR(COLLINHERITANCE_EMPTY_IN_STRUCT_ERR,         -1063000)
NEW_ERROR(RESCZONE_EMPTY_IN_STRUCT_ERR,                -1065000)
NEW_ERROR(RESCLOC_EMPTY_IN_STRUCT_ERR,                 -1066000)
NEW_ERROR(RESCTYPE_EMPTY_IN_STRUCT_ERR,                -1067000)
NEW_ERROR(RESCTYPEINX_EMPTY_IN_STRUCT_ERR,             -1068000)
NEW_ERROR(RESCCLASS_EMPTY_IN_STRUCT_ERR,               -1069000)
NEW_ERROR(RESCCLASSINX_EMPTY_IN_STRUCT_ERR,            -1070000)
NEW_ERROR(RESCVAULTPATH_EMPTY_IN_STRUCT_ERR,           -1071000)
NEW_ERROR(NUMOPEN_ORTS_EMPTY_IN_STRUCT_ERR,            -1072000)
NEW_ERROR(PARAOPR_EMPTY_IN_STRUCT_ERR,                 -1073000)
NEW_ERROR(RESCID_EMPTY_IN_STRUCT_ERR,                  -1074000)
NEW_ERROR(GATEWAYADDR_EMPTY_IN_STRUCT_ERR,             -1075000)
NEW_ERROR(RESCMAX_BJSIZE_EMPTY_IN_STRUCT_ERR,          -1076000)
NEW_ERROR(FREESPACE_EMPTY_IN_STRUCT_ERR,               -1077000)
NEW_ERROR(FREESPACETIME_EMPTY_IN_STRUCT_ERR,           -1078000)
NEW_ERROR(FREESPACETIMESTAMP_EMPTY_IN_STRUCT_ERR,      -1079000)
NEW_ERROR(RESCINFO_EMPTY_IN_STRUCT_ERR,                -1080000)
NEW_ERROR(RESCCOMMENTS_EMPTY_IN_STRUCT_ERR,            -1081000)
NEW_ERROR(RESCCREATE_EMPTY_IN_STRUCT_ERR,              -1082000)
NEW_ERROR(RESCMODIFY_EMPTY_IN_STRUCT_ERR,              -1083000)
NEW_ERROR(INPUT_ARG_NOT_WELL_FORMED_ERR,               -1084000)
NEW_ERROR(INPUT_ARG_OUT_OF_ARGC_RANGE_ERR,             -1085000)
NEW_ERROR(INSUFFICIENT_INPUT_ARG_ERR,                  -1086000)
NEW_ERROR(INPUT_ARG_DOES_NOT_MATCH_ERR,                -1087000)
NEW_ERROR(RETRY_WITHOUT_RECOVERY_ERR,                  -1088000)
NEW_ERROR(CUT_ACTION_PROCESSED_ERR,                    -1089000)
NEW_ERROR(ACTION_FAILED_ERR,                           -1090000)
NEW_ERROR(FAIL_ACTION_ENCOUNTERED_ERR,                 -1091000)
NEW_ERROR(VARIABLE_NAME_TOO_LONG_ERR,                  -1092000)
NEW_ERROR(UNKNOWN_VARIABLE_MAP_ERR,                    -1093000)
NEW_ERROR(UNDEFINED_VARIABLE_MAP_ERR,                  -1094000)
NEW_ERROR(NULL_VALUE_ERR,                              -1095000)
NEW_ERROR(DVARMAP_FILE_READ_ERROR,                     -1096000)
NEW_ERROR(NO_RULE_OR_MSI_FUNCTION_FOUND_ERR,           -1097000)
NEW_ERROR(FILE_CREATE_ERROR,                           -1098000)
NEW_ERROR(FMAP_FILE_READ_ERROR,                        -1099000)
NEW_ERROR(DATE_FORMAT_ERR,                             -1100000)
NEW_ERROR(RULE_FAILED_ERR,                             -1101000)
NEW_ERROR(NO_MICROSERVICE_FOUND_ERR,                   -1102000)
NEW_ERROR(INVALID_REGEXP,                              -1103000)
NEW_ERROR(INVALID_OBJECT_NAME,                         -1104000)
NEW_ERROR(INVALID_OBJECT_TYPE,                         -1105000)
NEW_ERROR(NO_VALUES_FOUND,                             -1106000)
NEW_ERROR(NO_COLUMN_NAME_FOUND,                        -1107000)
NEW_ERROR(BREAK_ACTION_ENCOUNTERED_ERR,                -1108000)
NEW_ERROR(CUT_ACTION_ON_SUCCESS_PROCESSED_ERR,         -1109000)
NEW_ERROR(MSI_OPERATION_NOT_ALLOWED,                   -1110000)
NEW_ERROR(MAX_NUM_OF_ACTION_IN_RULE_EXCEEDED,          -1111000)
NEW_ERROR(MSRVC_FILE_READ_ERROR,                       -1112000)
NEW_ERROR(MSRVC_VERSION_MISMATCH,                      -1113000)
NEW_ERROR(MICRO_SERVICE_OBJECT_TYPE_UNDEFINED,         -1114000)
NEW_ERROR(MSO_OBJ_GET_FAILED,                          -1115000)
NEW_ERROR(REMOTE_IRODS_CONNECT_ERR,                    -1116000)
NEW_ERROR(REMOTE_SRB_CONNECT_ERR,                      -1117000)
NEW_ERROR(MSO_OBJ_PUT_FAILED,                          -1118000)
/* parser error -1201000 */
#ifndef RE_PARSER_ERROR
NEW_ERROR(RE_PARSER_ERROR,                             -1201000)
NEW_ERROR(RE_UNPARSED_SUFFIX,                          -1202000)
NEW_ERROR(RE_POINTER_ERROR,                            -1203000)
/* runtime error -1205000 */
NEW_ERROR(RE_RUNTIME_ERROR,                            -1205000)
NEW_ERROR(RE_DIVISION_BY_ZERO,                         -1206000)
NEW_ERROR(RE_BUFFER_OVERFLOW,                          -1207000)
NEW_ERROR(RE_UNSUPPORTED_OP_OR_TYPE,                   -1208000)
NEW_ERROR(RE_UNSUPPORTED_SESSION_VAR,                  -1209000)
NEW_ERROR(RE_UNABLE_TO_WRITE_LOCAL_VAR,                -1210000)
NEW_ERROR(RE_UNABLE_TO_READ_LOCAL_VAR,                 -1211000)
NEW_ERROR(RE_UNABLE_TO_WRITE_SESSION_VAR,              -1212000)
NEW_ERROR(RE_UNABLE_TO_READ_SESSION_VAR,               -1213000)
NEW_ERROR(RE_UNABLE_TO_WRITE_VAR,                      -1214000)
NEW_ERROR(RE_UNABLE_TO_READ_VAR,                       -1215000)
NEW_ERROR(RE_PATTERN_NOT_MATCHED,                      -1216000)
NEW_ERROR(RE_STRING_OVERFLOW,                          -1217000)
/* system error -1220000 */
NEW_ERROR(RE_UNKNOWN_ERROR,                            -1220000)
NEW_ERROR(RE_OUT_OF_MEMORY,                            -1221000)
NEW_ERROR(RE_SHM_UNLINK_ERROR,                         -1222000)
NEW_ERROR(RE_FILE_STAT_ERROR,                          -1223000)
NEW_ERROR(RE_UNSUPPORTED_AST_NODE_TYPE,                -1224000)
NEW_ERROR(RE_UNSUPPORTED_SESSION_VAR_TYPE,             -1225000)
/* type error -1230000 */
NEW_ERROR(RE_TYPE_ERROR,                               -1230000)
NEW_ERROR(RE_FUNCTION_REDEFINITION,                    -1231000)
NEW_ERROR(RE_DYNAMIC_TYPE_ERROR,                       -1232000)
NEW_ERROR(RE_DYNAMIC_COERCION_ERROR,                   -1233000)
NEW_ERROR(RE_PACKING_ERROR,                            -1234000)
#endif
/** @} */



/* 1,600,000 to 1,700,000  - PHP scripting error */
/** @defgroup php_errors PHP ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 1,600,000 to 1,700,000
 * @{
 */
NEW_ERROR(PHP_EXEC_SCRIPT_ERR,                         -1600000)
NEW_ERROR(PHP_REQUEST_STARTUP_ERR,                     -1601000)
NEW_ERROR(PHP_OPEN_SCRIPT_FILE_ERR,                    -1602000)
/** @} */

/* 1,701,000 to 1,899,000  deprecated */

// new irods errors
/** @defgroup 4x_errors iRODS 4.x ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 1,800,000 to 1,899,000
 * @{
 */
NEW_ERROR(KEY_NOT_FOUND,                        -1800000)
NEW_ERROR(KEY_TYPE_MISMATCH,                    -1801000)
NEW_ERROR(CHILD_EXISTS,                         -1802000)
NEW_ERROR(HIERARCHY_ERROR,                      -1803000)
NEW_ERROR(CHILD_NOT_FOUND,                      -1804000)
NEW_ERROR(NO_NEXT_RESC_FOUND,                   -1805000)
NEW_ERROR(NO_PDMO_DEFINED,                      -1806000)
NEW_ERROR(INVALID_LOCATION,                     -1807000)
NEW_ERROR(PLUGIN_ERROR,                         -1808000)
NEW_ERROR(INVALID_RESC_CHILD_CONTEXT,           -1809000)
NEW_ERROR(INVALID_FILE_OBJECT,                  -1810000)
NEW_ERROR(INVALID_OPERATION,                    -1811000)
NEW_ERROR(CHILD_HAS_PARENT,                     -1812000)
NEW_ERROR(FILE_NOT_IN_VAULT,                    -1813000)
NEW_ERROR(DIRECT_ARCHIVE_ACCESS,                -1814000)
NEW_ERROR(ADVANCED_NEGOTIATION_NOT_SUPPORTED,   -1815000)
NEW_ERROR(DIRECT_CHILD_ACCESS,                  -1816000)
NEW_ERROR(INVALID_DYNAMIC_CAST,                 -1817000)
NEW_ERROR(INVALID_ACCESS_TO_IMPOSTOR_RESOURCE,  -1818000)
NEW_ERROR(INVALID_LEXICAL_CAST,                 -1819000)
NEW_ERROR(CONTROL_PLANE_MESSAGE_ERROR,          -1820000)
NEW_ERROR(REPLICA_NOT_IN_RESC,                  -1821000)
NEW_ERROR(INVALID_ANY_CAST,                     -1822000)
NEW_ERROR(BAD_FUNCTION_CALL,                    -1823000)
NEW_ERROR(CLIENT_NEGOTIATION_ERROR,             -1824000)
NEW_ERROR(SERVER_NEGOTIATION_ERROR,             -1825000)
NEW_ERROR(INVALID_KVP_STRING,                   -1826000)
NEW_ERROR(PLUGIN_ERROR_MISSING_SHARED_OBJECT,   -1827000)
NEW_ERROR(RULE_ENGINE_ERROR,                    -1828000)
NEW_ERROR(REBALANCE_ALREADY_ACTIVE_ON_RESOURCE, -1829000)
/** @} */


/* NetCDF error code */
/** @defgroup netcdf_errors NetCDF ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 2,000,000 to 2,099,000
 * @{
 */
NEW_ERROR(NETCDF_OPEN_ERR,                             -2000000)
NEW_ERROR(NETCDF_CREATE_ERR,                           -2001000)
NEW_ERROR(NETCDF_CLOSE_ERR,                            -2002000)
NEW_ERROR(NETCDF_INVALID_PARAM_TYPE,                   -2003000)
NEW_ERROR(NETCDF_INQ_ID_ERR,                           -2004000)
NEW_ERROR(NETCDF_GET_VARS_ERR,                         -2005000)
NEW_ERROR(NETCDF_INVALID_DATA_TYPE,                    -2006000)
NEW_ERROR(NETCDF_INQ_VARS_ERR,                         -2007000)
NEW_ERROR(NETCDF_VARS_DATA_TOO_BIG,                    -2008000)
NEW_ERROR(NETCDF_DIM_MISMATCH_ERR,                     -2009000)
NEW_ERROR(NETCDF_INQ_ERR,                              -2010000)
NEW_ERROR(NETCDF_INQ_FORMAT_ERR,                       -2011000)
NEW_ERROR(NETCDF_INQ_DIM_ERR,                          -2012000)
NEW_ERROR(NETCDF_INQ_ATT_ERR,                          -2013000)
NEW_ERROR(NETCDF_GET_ATT_ERR,                          -2014000)
NEW_ERROR(NETCDF_VAR_COUNT_OUT_OF_RANGE,               -2015000)
NEW_ERROR(NETCDF_UNMATCHED_NAME_ERR,                   -2016000)
NEW_ERROR(NETCDF_NO_UNLIMITED_DIM,                     -2017000)
NEW_ERROR(NETCDF_PUT_ATT_ERR,                          -2018000)
NEW_ERROR(NETCDF_DEF_DIM_ERR,                          -2019000)
NEW_ERROR(NETCDF_DEF_VAR_ERR,                          -2020000)
NEW_ERROR(NETCDF_PUT_VARS_ERR,                         -2021000)
NEW_ERROR(NETCDF_AGG_INFO_FILE_ERR,                    -2022000)
NEW_ERROR(NETCDF_AGG_ELE_INX_OUT_OF_RANGE,             -2023000)
NEW_ERROR(NETCDF_AGG_ELE_FILE_NOT_OPENED,              -2024000)
NEW_ERROR(NETCDF_AGG_ELE_FILE_NO_TIME_DIM,             -2025000)
/** @} */

/* SSL protocol error codes */
/** @defgroup ssl_errors SSL ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 2,100,000 to 2,199,000
 * @{
 */
NEW_ERROR(SSL_NOT_BUILT_INTO_CLIENT,                   -2100000)
NEW_ERROR(SSL_NOT_BUILT_INTO_SERVER,                   -2101000)
NEW_ERROR(SSL_INIT_ERROR,                              -2102000)
NEW_ERROR(SSL_HANDSHAKE_ERROR,                         -2103000)
NEW_ERROR(SSL_SHUTDOWN_ERROR,                          -2104000)
NEW_ERROR(SSL_CERT_ERROR,                              -2105000)
/** @} */

/* OOI CI error codes */
/** @defgroup ooi_errors OOI ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 2,200,000 to 2,299,000
 * @{
 */
NEW_ERROR(OOI_CURL_EASY_INIT_ERR,                      -2200000)
NEW_ERROR(OOI_JSON_OBJ_SET_ERR,                        -2201000)
NEW_ERROR(OOI_DICT_TYPE_NOT_SUPPORTED,                 -2202000)
NEW_ERROR(OOI_JSON_PACK_ERR,                           -2203000)
NEW_ERROR(OOI_JSON_DUMP_ERR,                           -2204000)
NEW_ERROR(OOI_CURL_EASY_PERFORM_ERR,                   -2205000)
NEW_ERROR(OOI_JSON_LOAD_ERR,                           -2206000)
NEW_ERROR(OOI_JSON_GET_ERR,                            -2207000)
NEW_ERROR(OOI_JSON_NO_ANSWER_ERR,                      -2208000)
NEW_ERROR(OOI_JSON_TYPE_ERR,                           -2209000)
NEW_ERROR(OOI_JSON_INX_OUT_OF_RANGE,                   -2210000)
NEW_ERROR(OOI_REVID_NOT_FOUND,                         -2211000)
/** @} */

/* Deprecation error codes */
/** @defgroup deprecation_errors DEPRECATION ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 3,000,000 to 3,099,000
 * @{
 */
NEW_ERROR(DEPRECATED_PARAMETER,                        -3000000)
/** @} */

/* XML parsing and TDS error */
/** @defgroup xml_errors XML ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 2,300,000 to 2,399,000
 * @{
 */
NEW_ERROR(XML_PARSING_ERR,                             -2300000)
NEW_ERROR(OUT_OF_URL_PATH,                             -2301000)
NEW_ERROR(URL_PATH_INX_OUT_OF_RANGE,                   -2302000)
/** @} */

/* The following are handler protocol type msg. These are not real errors */
/** @defgroup handler_errors Handler Protocol ERRORs
 *  @ingroup error_codes
 *  ERROR Code Range 9,999,000 to 9,999,999
 * @{
 */
NEW_ERROR(SYS_NULL_INPUT,                              -99999996)
NEW_ERROR(SYS_HANDLER_DONE_WITH_ERROR,                 -99999997)
NEW_ERROR(SYS_HANDLER_DONE_NO_ERROR,                   -99999998)
NEW_ERROR(SYS_NO_HANDLER_REPLY_MSG,                    -99999999)
/** @} */
#ifndef MAKE_IRODS_ERROR_MAP
};
#endif

// clang-format on

#endif /* RODS_ERROR_TABLE_H__ */
