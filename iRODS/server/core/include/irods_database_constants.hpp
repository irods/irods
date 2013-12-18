


#ifndef __EIRODS_DATABASE_CONSTANTS_H__
#define __EIRODS_DATABASE_CONSTANTS_H__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace irods {
    /// =-=-=-=-=-=-=-
    /// @brief constants for network object indexing
    const std::string DATABASE_OP_START( "database_start" );
    const std::string DATABASE_OP_STOP( "database_stop" );

    const std::string DATABASE_OP_DEBUG( "database_debug" );
    const std::string DATABASE_OP_OPEN( "database_open" );
    const std::string DATABASE_OP_CLOSE( "database_close" );
    const std::string DATABASE_OP_GET_LOCAL_ZONE( "database_get_local_zone" );

}; // namespace irods

#endif // __EIRODS_DATABASE_CONSTANTS_H__



