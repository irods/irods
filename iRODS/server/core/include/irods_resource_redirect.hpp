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

    error resolve_resource_hierarchy(
        const std::string&, // requested operation to consider
        rsComm_t*,          // current agent connection
        dataObjInp_t*,      // data inp struct for data object in question
        std::string& );     // out going selected resource hierarchy

    error resource_redirect(
        const std::string&, // requested operation to consider
        rsComm_t*,          // current agent connection
        dataObjInp_t*,      // data inp struct for data object in question
        std::string&,       // out going selected resource hierarchy
        rodsServerHost_t*&, // selected host for redirection if necessary
        int& );             // flag stating LOCAL_HOST or REMOTE_HOST

}; // namespace irods

#endif // __IRODS_RESOURCE_REDIRECT_HPP__



