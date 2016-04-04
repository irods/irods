#ifndef __IRODS_CLIENT_SERVER_NEGOTIATION_HPP__
#define __IRODS_CLIENT_SERVER_NEGOTIATION_HPP__

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_network_object.hpp"
#include "irods_server_properties.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief key for use of ssl or not
    const char RODS_CS_NEG      [] = {"RODS_CS_NEG"};
    const char CS_NEG_USE_SSL_KW[] = {"cs_neg_ssl_kw"};

/// =-=-=-=-=-=-=-
/// @brief constants for sucess / failure status
    const int CS_NEG_STATUS_SUCCESS = 1;
    const int CS_NEG_STATUS_FAILURE = 0;

/// =-=-=-=-=-=-=-
/// @brief struct to hold the negotiation message
    struct cs_neg_t {
        int  status_;
        char result_[MAX_NAME_LEN];
    };

/// =-=-=-=-=-=-=-
/// @brief constants for connection choices made by the negotiation
    const std::string CS_NEG_FAILURE( "CS_NEG_FAILURE" );
    const std::string CS_NEG_USE_SSL( "CS_NEG_USE_SSL" );
    const std::string CS_NEG_USE_TCP( "CS_NEG_USE_TCP" );

    const std::string CS_NEG_REQUIRE( "CS_NEG_REQUIRE" );     // index 0
    const std::string CS_NEG_REFUSE( "CS_NEG_REFUSE" );       // index 1
    const std::string CS_NEG_DONT_CARE( "CS_NEG_DONT_CARE" ); // index 2

    const std::string CS_NEG_SID_KW( "cs_neg_sid_kw" );
    const std::string CS_NEG_RESULT_KW( "cs_neg_result_kw" );

/// =-=-=-=-=-=-=-
/// @brief function which determines if a client/server negotiation is needed
///        on the server side
    bool do_client_server_negotiation_for_server( );

/// =-=-=-=-=-=-=-
/// @brief function which determines if a client/server negotiation is needed
///        on the client side
    bool do_client_server_negotiation_for_client( );

/// =-=-=-=-=-=-=-
/// @brief function which manages the TLS and Auth negotiations with the client
    error client_server_negotiation_for_server(
        irods::network_object_ptr, // server connection handle
        std::string& );             // results of negotiation

/// =-=-=-=-=-=-=-
/// @brief function which manages the TLS and Auth negotiations with the client
    error client_server_negotiation_for_client(
        irods::network_object_ptr, // client connection handle
        const std::string&,        // host name
        std::string& );            // results of the negotiation

/// =-=-=-=-=-=-=-
/// @brief function which sends the negotiation message
    error send_client_server_negotiation_message(
        irods::network_object_ptr, // socket
        cs_neg_t& );                // message payload

/// =-=-=-=-=-=-=-
/// @brief function which sends the negotiation message
    error read_client_server_negotiation_message(
        irods::network_object_ptr,       // socket
        boost::shared_ptr< cs_neg_t >& ); // message payload

/// =-=-=-=-=-=-=-
/// @brief given a buffer encrypt and hash it for negotiation
    error sign_server_sid(
        const std::string,   // incoming SID
        const std::string,   // encryption key
        std::string& );      // signed buffer

/// =-=-=-=-=-=-=-
/// @brief check the incoming signed SID against all locals SIDs
    error check_sent_sid(
        const std::string );  // incoming signed SID

}; // namespace irods

#endif // __IRODS_CLIENT_SERVER_NEGOTIATION_HPP__



