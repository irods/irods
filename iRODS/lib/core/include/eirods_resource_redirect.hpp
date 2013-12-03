


#ifndef __EIRODS_RESOURCE_REDIRECT_H__
#define __EIRODS_RESOURCE_REDIRECT_H__

// =-=-=-=-=-=-=-
// irods includes
#include "rsGlobalExtern.hpp"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_file_object.hpp"
#include "eirods_log.hpp"

namespace eirods {

    const std::string EIRODS_CREATE_OPERATION( "CREATE" );
    const std::string EIRODS_WRITE_OPERATION( "WRITE" );
    const std::string EIRODS_OPEN_OPERATION( "OPEN" );

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
 
}; // namespace eirods

#endif // __EIRODS_RESOURCE_REDIRECT_H__



