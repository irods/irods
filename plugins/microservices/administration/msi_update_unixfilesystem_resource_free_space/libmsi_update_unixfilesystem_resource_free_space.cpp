#include "msParam.h"
#include "rcMisc.h"
#include "rsGeneralAdmin.hpp"
#include "irods_ms_plugin.hpp"
#include "irods_resource_plugin.hpp"
#include "irods_resource_manager.hpp"
#include "irods_plugin_name_generator.hpp"
#include "generalAdmin.h"
#include "irods_log.hpp"
#include <sys/statvfs.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

extern irods::resource_manager resc_mgr;

int
msi_update_unixfilesystem_resource_free_space(msParam_t *resource_name_msparam, ruleExecInfo_t *rei) {
    if (rei == NULL) {
        rodsLog(LOG_ERROR, "[%s]: input rei is NULL", __FUNCTION__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if (resource_name_msparam == NULL) {
        rodsLog(LOG_ERROR, "[%s]: resource_name_msparam is NULL", __FUNCTION__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    if (resource_name_msparam->type == NULL) {
        rodsLog(LOG_ERROR, "[%s]: resource_name_msparam->type is NULL", __FUNCTION__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    if (strcmp(resource_name_msparam->type, STR_MS_T)) {
        rodsLog(LOG_ERROR, "[%s]: first argument should be STR_MS_T, was [%s]", __FUNCTION__, resource_name_msparam->type);
        return USER_PARAM_TYPE_ERR;
    }

    const char* resource_name_cstring = static_cast<char*>(resource_name_msparam->inOutStruct);
    if (resource_name_cstring == NULL) {
        rodsLog(LOG_ERROR, "[%s]: input resource_name_msparam->inOutStruct is NULL", __FUNCTION__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    irods::resource_ptr resource_ptr;
    const irods::error resource_manager_resolve_error = resc_mgr.resolve(resource_name_cstring, resource_ptr );
    if (!resource_manager_resolve_error.ok()) {
        rodsLog(LOG_ERROR, "[%s]: failed to resolve resource [%s]", __FUNCTION__, resource_name_cstring);
        irods::log(resource_manager_resolve_error);
        return resource_manager_resolve_error.status();
    }

    std::string resource_type_unnormalized;
    const irods::error get_resource_type_error = resource_ptr->get_property<std::string>(irods::RESOURCE_TYPE, resource_type_unnormalized);
    if (!get_resource_type_error.ok()) {
        rodsLog(LOG_ERROR, "[%s]: failed to get resource type for [%s]", __FUNCTION__, resource_name_cstring);
        irods::log(get_resource_type_error);
        return get_resource_type_error.status();
    }

    std::string resource_type_normalized = irods::normalize_resource_type(resource_type_unnormalized);
    if (resource_type_normalized != "unixfilesystem") {
        return rei->status;
    }

    std::string resource_vault_path;
    const irods::error get_resource_vault_path_error = resource_ptr->get_property<std::string>(irods::RESOURCE_PATH, resource_vault_path);
    if (!get_resource_vault_path_error.ok()) {
        rodsLog(LOG_ERROR, "[%s]: failed to get resource path for [%s]", __FUNCTION__, resource_name_cstring);
        irods::log(get_resource_vault_path_error);
        return get_resource_vault_path_error.status();
    }

    boost::filesystem::path path_to_stat{resource_vault_path};
    const auto absolute_vault_path{boost::filesystem::absolute(path_to_stat)};
    while(!boost::filesystem::exists(path_to_stat)) {
        rodsLog(LOG_NOTICE, "[%s]: path to stat [%s] for resource [%s] doesn't exist, moving to parent", __FUNCTION__, path_to_stat.string().c_str(), resource_name_cstring);
        path_to_stat = path_to_stat.parent_path();
        if (path_to_stat.empty()) {
            rodsLog(LOG_ERROR, "[%s]: could not find existing path from vault path [%s] for resource [%s]", __FUNCTION__, resource_vault_path.c_str(), resource_name_cstring);
            return SYS_INVALID_RESC_INPUT;
        }
    }

    // Using the root directory as a vault path is bad - do not allow it to be used for updating free space
    if (absolute_vault_path.root_path() == path_to_stat) {
        rodsLog(LOG_ERROR, "[%s]: could not find existing non-root path from vault path [%s] for resource [%s]", __FUNCTION__, resource_vault_path.c_str(), resource_name_cstring);
        return SYS_INVALID_RESC_INPUT;
    }

    struct statvfs statvfs_buf;
    const int statvfs_ret = statvfs(path_to_stat.string().c_str(), &statvfs_buf);
    if (statvfs_ret != 0) {
        rodsLog(LOG_ERROR, "[%s]: statvfs() of [%s] for resource [%s] failed with return %d and errno %d", __FUNCTION__, path_to_stat.string().c_str(), resource_name_cstring, statvfs_ret, errno);
        return SYS_INVALID_RESC_INPUT;
    }

    uintmax_t free_space_in_bytes;
    if (statvfs_buf.f_bavail > ULONG_MAX / statvfs_buf.f_frsize) {
        rodsLog(LOG_NOTICE, "[%s]: statvfs() of resource [%s] reports free space in excess of ULONG_MAX f_bavail %ju f_frsize %ju failed with return %d and errno %d", __FUNCTION__, resource_name_cstring, static_cast<uintmax_t>(statvfs_buf.f_bavail), static_cast<uintmax_t>(statvfs_buf.f_frsize));
        free_space_in_bytes = ULONG_MAX;
    } else {
        free_space_in_bytes = statvfs_buf.f_bavail * statvfs_buf.f_frsize;
    }
    const std::string free_space_in_bytes_string = boost::lexical_cast<std::string>(free_space_in_bytes);

    if (rei->rsComm == NULL) {
        rodsLog(LOG_ERROR, "[%s]: input rei->rsComm is NULL", __FUNCTION__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    generalAdminInp_t admin_in;
    memset(&admin_in, 0, sizeof(admin_in));
    admin_in.arg0 = "modify";
    admin_in.arg1 = "resource";
    admin_in.arg2 = resource_name_cstring;
    admin_in.arg3 = "freespace";
    admin_in.arg4 = free_space_in_bytes_string.c_str();


    rodsEnv service_account_environment;
    const int getRodsEnv_ret = getRodsEnv(&service_account_environment);
    if (getRodsEnv_ret < 0) {
        rodsLog(LOG_ERROR, "[%s]: getRodsEnv failure [%d]", __FUNCTION__, getRodsEnv_ret);
        return getRodsEnv_ret;
    }

    rErrMsg_t errMsg;
    memset(&errMsg, 0, sizeof(errMsg));
    rcComm_t *admin_connection = rcConnect(service_account_environment.rodsHost, service_account_environment.rodsPort,
                                           service_account_environment.rodsUserName, service_account_environment.rodsZone, 0, &errMsg);
    if (admin_connection == NULL) {
        char *mySubName = NULL;
        const char *myName = rodsErrorName(errMsg.status, &mySubName);
        rodsLog(LOG_ERROR, "[%s]: rcConnect failure [%s] [%s] [%d] [%s]", __FUNCTION__,
                myName, mySubName, errMsg.status, errMsg.msg);
        return errMsg.status;
    }

    const int clientLogin_ret = clientLogin(admin_connection);
    if (clientLogin_ret != 0) {
        rodsLog(LOG_ERROR, "[%s]: clientLogin failure [%d]", __FUNCTION__, clientLogin_ret);
        return clientLogin_ret;
    }

    const int rcGeneralAdmin_ret = rcGeneralAdmin(admin_connection, &admin_in);
    rcDisconnect(admin_connection);
    if (rcGeneralAdmin_ret < 0) {
        printErrorStack(admin_connection->rError);
        rodsLog(LOG_ERROR, "[%s]: rcGeneralAdmin failure [%d]", __FUNCTION__, rcGeneralAdmin_ret);
    }
    return rcGeneralAdmin_ret;
}

extern "C"
irods::ms_table_entry* plugin_factory() {
    irods::ms_table_entry* msvc = new irods::ms_table_entry(1);
    msvc->add_operation("msi_update_unixfilesystem_resource_free_space",
                         std::function<int(
                             msParam_t*,
                             ruleExecInfo_t*)>(msi_update_unixfilesystem_resource_free_space));
    return msvc;
}
