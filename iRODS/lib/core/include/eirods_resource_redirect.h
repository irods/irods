


#ifndef __EIRODS_RESOURCE_REDIRECT_H__
#define __EIRODS_RESOURCE_REDIRECT_H__

// =-=-=-=-=-=-=-
// irods includes
#include "rsGlobalExtern.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_file_object.h"
#include "eirods_log.h"

namespace eirods {

    const std::string EIRODS_GET_OPERATION ( "get" );
    const std::string EIRODS_PUT_OPERATION ( "put" );
    const std::string EIRODS_REPL_OPERATION( "repl" );

    error resource_redirect( const std::string&, // requested operation to consider
                             rsComm_t*,          // current agent connection
                             dataObjInp_t*,      // data inp struct for data object in question 
                             std::string&,       // out going selected resource hierarchy
                             rodsServerHost_t*&, // selected host for redirection if necessary 
                             int& );             // flag stating LOCAL_HOST or REMOTE_HOST
 
}; // namespace eirods

#endif // __EIRODS_RESOURCE_REDIRECT_H__



