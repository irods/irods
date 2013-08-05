


#ifndef __EIRODS_NETWORK_CONSTANTS_H__
#define __EIRODS_NETWORK_CONSTANTS_H__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace eirods {
    /// =-=-=-=-=-=-=-
    /// @brief constants for network object indexing
    const std::string NETWORK_OP_START( "start" );
    const std::string NETWORK_OP_END( "end" );
    const std::string NETWORK_OP_ACCEPT( "accept" );
    const std::string NETWORK_OP_SHUTDOWN( "shutdown" );
    const std::string NETWORK_OP_READ_HEADER( "read_header" );
    const std::string NETWORK_OP_READ_BODY( "read_body" );
    const std::string NETWORK_OP_WRITE_HEADER( "write_header" );
    const std::string NETWORK_OP_WRITE_BODY( "write_body" );

}; // namespace eirods

#endif // __EIRODS_NETWORK_CONSTANTS_H__



