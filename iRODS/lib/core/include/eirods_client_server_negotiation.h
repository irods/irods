


#ifndef __EIRODS_CLIENT_SERVER_NEGOTIATION_H__
#define __EIRODS_CLIENT_SERVER_NEGOTIATION_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace eirods {
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
    const std::string CS_NEG_DONT_USE_SSL( "CS_NEG_DONT_USE_SSL" );

    const std::string CS_NEG_REQUIRE( "CS_NEG_REQUIRE" );     // index 0
    const std::string CS_NEG_REFUSE( "CS_NEG_REFUSE" );       // index 1
    const std::string CS_NEG_DONT_CARE( "CS_NEG_DONT_CARE" ); // index 2

    /// =-=-=-=-=-=-=-
    /// @brief function which determines if a client/server negotiation is needed
    bool do_client_server_negotiation(  );

    /// =-=-=-=-=-=-=-
    /// @brief function which manages the TLS and Auth negotiations with the client
    error client_server_negotiation_for_server( rsComm_t&,      // server connection handle
                                                std::string& ); // results of negotiation
 
    /// =-=-=-=-=-=-=-
    /// @brief function which manages the TLS and Auth negotiations with the client
    error client_server_negotiation_for_client( rcComm_t&,      // client connection handle
                                                std::string& ); // results of the negotiation
   
    /// =-=-=-=-=-=-=-
    /// @brief function which sends the negotiation message
    error send_client_server_negotiation_message( int,         // socket
                                                  cs_neg_t& ); // message payload
 
    /// =-=-=-=-=-=-=-
    /// @brief function which sends the negotiation message
    error read_client_server_negotiation_message( int,                              // socket
                                                  boost::shared_ptr< cs_neg_t >& ); // message payload
}; // namespace eirods

#endif // __EIRODS_CLIENT_SERVER_NEGOTIATION_H__



