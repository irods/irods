#ifndef __IRODS_RESOURCE_REDIRECT_HPP__
#define __IRODS_RESOURCE_REDIRECT_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "rsGlobalExtern.hpp"

// =-=-=-=-=-=-=-
#include "irods_file_object.hpp"
#include "irods_log.hpp"

namespace irods {

    const std::string CREATE_OPERATION( "CREATE" );
    const std::string WRITE_OPERATION( "WRITE" );
    const std::string OPEN_OPERATION( "OPEN" );
    const std::string UNLINK_OPERATION( "UNLINK" );


// check C++11 support
#if __cplusplus > 199711L
    error resolve_resource_hierarchy(
        const std::string&, // requested operation to consider
        rsComm_t*,          // current agent connection
        dataObjInp_t*,      // data inp struct for data object in question
        std::string&,        // out going selected resource hierarchy
        dataObjInfo_t** _data_obj_info = nullptr );

    error resource_redirect(
        const std::string&, // requested operation to consider
        rsComm_t*,          // current agent connection
        dataObjInp_t*,      // data inp struct for data object in question
        std::string&,       // out going selected resource hierarchy
        rodsServerHost_t*&, // selected host for redirection if necessary
        int&,                // flag stating LOCAL_HOST or REMOTE_HOST
        dataObjInfo_t**    _data_obj_info = nullptr );

#else
    error resolve_resource_hierarchy(
        const std::string&, // requested operation to consider
        rsComm_t*,          // current agent connection
        dataObjInp_t*,      // data inp struct for data object in question
        std::string&,        // out going selected resource hierarchy
        dataObjInfo_t** _data_obj_info = NULL );

    error resource_redirect(
        const std::string&, // requested operation to consider
        rsComm_t*,          // current agent connection
        dataObjInp_t*,      // data inp struct for data object in question
        std::string&,       // out going selected resource hierarchy
        rodsServerHost_t*&, // selected host for redirection if necessary
        int&,                // flag stating LOCAL_HOST or REMOTE_HOST
        dataObjInfo_t**    _data_obj_info = NULL );
#endif

bool is_hier_in_obj_info_list(const std::string&, dataObjInfo_t*);

}; // namespace irods

#endif // __IRODS_RESOURCE_REDIRECT_HPP__



