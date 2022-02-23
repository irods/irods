#ifndef __IRODS_RESOURCE_REDIRECT_HPP__
#define __IRODS_RESOURCE_REDIRECT_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "irods/rsGlobalExtern.hpp"

// =-=-=-=-=-=-=-
#include "irods/irods_file_object.hpp"
#include "irods/irods_log.hpp"

namespace irods
{
    // clang-format off
    using file_object_factory_result    = std::tuple<irods::file_object_ptr, irods::error>;
    using resolve_hierarchy_result_type = std::tuple<irods::file_object_ptr, std::string>;
    // clang-format on

    extern const std::string CREATE_OPERATION;
    extern const std::string WRITE_OPERATION;
    extern const std::string OPEN_OPERATION;
    extern const std::string UNLINK_OPERATION;

    error resource_redirect(
        const std::string&, // requested operation to consider
        rsComm_t*,          // current agent connection
        dataObjInp_t*,      // data inp struct for data object in question
        std::string&,       // out going selected resource hierarchy
        rodsServerHost_t*&, // selected host for redirection if necessary
        int&,                // flag stating LOCAL_HOST or REMOTE_HOST
        dataObjInfo_t**    _data_obj_info = nullptr );

    irods::resolve_hierarchy_result_type resolve_resource_hierarchy(
        const std::string& oper,
        rsComm_t*          comm,
        dataObjInp_t&      data_obj_inp,
        dataObjInfo_t**    data_obj_info = nullptr);

    irods::resolve_hierarchy_result_type resolve_resource_hierarchy(
        rsComm_t*            comm,
        const std::string&   oper_in,
        dataObjInp_t&        data_obj_inp,
        irods::file_object_factory_result& file_obj_result);
} // namespace irods

#endif // __IRODS_RESOURCE_REDIRECT_HPP__
