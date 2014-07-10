#ifndef __IRODS_AUTH_CONSTANTS_HPP__
#define __IRODS_AUTH_CONSTANTS_HPP__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief constants for auth object indexing
    const std::string AUTH_CLIENT_START( "auth_client_start" );
    const std::string AUTH_AGENT_START( "auth_agent_start" );
    const std::string AUTH_ESTABLISH_CONTEXT( "auth_establish_context" );
    const std::string AUTH_CLIENT_AUTH_REQUEST( "auth_agent_client_request" );
    const std::string AUTH_AGENT_AUTH_REQUEST( "auth_agent_auth_request" );
    const std::string AUTH_CLIENT_AUTH_RESPONSE( "auth_agent_client_response" );
    const std::string AUTH_AGENT_AUTH_RESPONSE( "auth_agent_auth_response" );
    const std::string AUTH_AGENT_AUTH_VERIFY( "auth_agent_auth_verify" );

/// =-=-=-=-=-=-=-
/// @brief keys for key-value association
    const std::string AUTH_USER_KEY( "a_user" );
    const std::string AUTH_SCHEME_KEY( "a_scheme" );
    const std::string AUTH_TTL_KEY( "a_ttl" );
    const std::string AUTH_PASSWORD_KEY( "a_pw" );
    const std::string AUTH_RESPONSE_KEY( "a_resp" );

}; // namespace irods


#endif // __IRODS_AUTH_CONSTANTS_HPP__

