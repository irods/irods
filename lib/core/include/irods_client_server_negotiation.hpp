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
#include <string_view>

namespace irods
{
    /// =-=-=-=-=-=-=-
    /// @brief key for use of ssl or not
    inline const char RODS_CS_NEG[]       = {"RODS_CS_NEG"};
    inline const char CS_NEG_USE_SSL_KW[] = {"cs_neg_ssl_kw"};

    /// =-=-=-=-=-=-=-
    /// @brief constants for success / failure status
    inline const int CS_NEG_STATUS_SUCCESS = 1;
    inline const int CS_NEG_STATUS_FAILURE = 0;

    /// \brief Constant for the length of the negotiation_key in bytes
    inline const std::size_t negotiation_key_length_in_bytes = 32;

    /// =-=-=-=-=-=-=-
    /// @brief struct to hold the negotiation message
    struct cs_neg_t {
        int  status_;
        char result_[MAX_NAME_LEN];
    };

    /// =-=-=-=-=-=-=-
    /// @brief constants for connection choices made by the negotiation
    inline const std::string CS_NEG_FAILURE( "CS_NEG_FAILURE" );
    inline const std::string CS_NEG_USE_SSL( "CS_NEG_USE_SSL" );
    inline const std::string CS_NEG_USE_TCP( "CS_NEG_USE_TCP" );

    inline const std::string CS_NEG_REQUIRE( "CS_NEG_REQUIRE" );     // index 0
    inline const std::string CS_NEG_REFUSE( "CS_NEG_REFUSE" );       // index 1
    inline const std::string CS_NEG_DONT_CARE( "CS_NEG_DONT_CARE" ); // index 2

    inline const std::string CS_NEG_SID_KW( "cs_neg_sid_kw" );
    inline const std::string CS_NEG_RESULT_KW( "cs_neg_result_kw" );

    /// =-=-=-=-=-=-=-
    /// @brief function which determines if a client/server negotiation is needed
    ///        on the server side
    bool do_client_server_negotiation_for_server();

    /// =-=-=-=-=-=-=-
    /// @brief function which determines if a client/server negotiation is needed
    ///        on the client side
    bool do_client_server_negotiation_for_client();

    /// =-=-=-=-=-=-=-
    /// @brief function which manages the TLS and Auth negotiations with the client
    error client_server_negotiation_for_server(
        irods::network_object_ptr,  // server connection handle
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
        irods::network_object_ptr,  // socket
        cs_neg_t& );                // message payload

    /// =-=-=-=-=-=-=-
    /// @brief function which sends the negotiation message
    error read_client_server_negotiation_message(
        irods::network_object_ptr,        // socket
        boost::shared_ptr< cs_neg_t >& ); // message payload

    /// =-=-=-=-=-=-=-
    /// @brief given a buffer encrypt and hash it for negotiation
    auto sign_server_sid(const std::string& _zone_key,
                         const std::string& _encryption_key,
                         std::string&       _signed_zone_key) -> irods::error;

    /// \brief check the incoming signed zone_key against local and remote zone_keys
    auto check_sent_sid(const std::string& _zone_key) -> irods::error;

    /// \brief Return whether the configured negotiation_key meets a set of requirements.
    ///
    /// \param[in] _key negotiation_key to check.
    ///
    /// \retval true If the negotiation_key has exactly 32 valid characters.
    /// \retval false If the negotiation_key does not have exactly 32 valid characters.
    ///
    /// \since 4.2.11
    auto negotiation_key_is_valid(const std::string_view _key) -> bool;
} // namespace irods

#endif // __IRODS_CLIENT_SERVER_NEGOTIATION_HPP__

