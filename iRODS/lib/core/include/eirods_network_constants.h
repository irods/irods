


#ifndef __EIRODS_NETWORK_CONSTANTS_H__
#define __EIRODS_NETWORK_CONSTANTS_H__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace eirods {
    /// =-=-=-=-=-=-=-
    /// @brief constants for network object indexing
    const std::string NETWORK_OP_CLIENT_START( "network_client_start" );
    const std::string NETWORK_OP_CLIENT_STOP( "network_client_stop" );
    const std::string NETWORK_OP_AGENT_START( "network_agent_start" );
    const std::string NETWORK_OP_AGENT_STOP( "network_agent_stop" );
    const std::string NETWORK_OP_READ_HEADER( "network_read_header" );
    const std::string NETWORK_OP_READ_BODY( "network_read_body" );
    const std::string NETWORK_OP_WRITE_HEADER( "network_write_header" );
    const std::string NETWORK_OP_WRITE_BODY( "network_write_body" );

}; // namespace eirods

#endif // __EIRODS_NETWORK_CONSTANTS_H__



