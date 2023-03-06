#include "irods_client_server_negotiation.hpp"

#include "irods_stacktrace.hpp"
#include "irods_exception.hpp"
#include "irods_server_properties.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_buffer_encryption.hpp"
#include "irods_hasher_factory.hpp"
#include "irods_configuration_parser.hpp"
#include "MD5Strategy.hpp"
#include "sockComm.h"
#include "sockCommNetworkInterface.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rsGlobalExtern.hpp"
#include "rodsDef.h"
#include "rodsConnect.h"
#include "rcMisc.h"
//#include "index.hpp"
//#include "reFuncDefs.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <algorithm>
#include <cstdlib>
#include <map>
#include <regex>
#include <vector>

#include "fmt/format.h"

extern const packInstruct_t RodsPackTable[];

namespace irods
{
    auto negotiation_key_is_valid(const std::string_view _key) -> bool
    {
        static const auto negotiation_key_regex = std::regex{R"_(^[A-Za-z0-9_]+$)_"};

        return _key.length() == negotiation_key_length_in_bytes &&
            std::regex_match(_key.data(), negotiation_key_regex);
    } // negotiation_key_is_valid

    /// =-=-=-=-=-=-=-
    /// @brief given a property map and the target host name decide between a federated key and a local key
    auto determine_negotiation_key(const std::string& _host_name) -> std::string
    {
        try {
            for (const auto& el : irods::get_server_property<const std::vector<boost::any>&>(irods::CFG_FEDERATION_KW))
            {
                try {
                    const auto& item = boost::any_cast<const std::unordered_map<std::string, boost::any>&>(el);

                    const auto e = std::cend(item);
                    const auto provider_hosts_itr = item.find(irods::CFG_CATALOG_PROVIDER_HOSTS_KW);
                    const auto negotiation_key = item.find(irods::CFG_NEGOTIATION_KEY_KW);
                    if (e == provider_hosts_itr || e == negotiation_key) {
                        irods::log(
                            LOG_WARNING,
                            fmt::format(
                                "[{}] federation entry missing required properties - check configuration", __func__));
                        continue;
                    }

                    const auto& provider_hosts =
                        boost::any_cast<const std::vector<boost::any>&>(provider_hosts_itr->second);

                    const auto hostname_in_provider_hosts_list = std::any_of(
                        std::cbegin(provider_hosts), std::cend(provider_hosts), [&_host_name](const auto& _host) {
                            return _host_name == boost::any_cast<const std::string&>(_host);
                        });

                    if (hostname_in_provider_hosts_list) {
                        return boost::any_cast<const std::string&>(negotiation_key->second);
                    }
                }
                catch (const boost::bad_any_cast& e) {
                    irods::log(LOG_ERROR,
                               fmt::format(
                                   "[{}] boost::any_cast failed for federation entry - check configuration", __func__));
                    continue;
                }
            }
        }
        catch (const irods::exception&) {
        }

        // if not, it must be in our zone
        return irods::get_server_property<const std::string>(CFG_NEGOTIATION_KEY_KW);
    } // determine_negotiation_key

    error sign_server_sid(const std::string& _zone_key,
                          const std::string& _encryption_key,
                          std::string&       _signed_zone_key)
    {
        if (_encryption_key.empty()) {
            return ERROR(CLIENT_NEGOTIATION_ERROR, "encryption key for signing is empty");
        }

        irods::buffer_crypt::array_t key;
        key.assign(_encryption_key.begin(), _encryption_key.end());

        irods::buffer_crypt::array_t in_buf;
        in_buf.assign(_zone_key.begin(), _zone_key.end());

        // create an encryption object
        // 32 byte key, 8 byte iv, 16 rounds encryption
        irods::buffer_crypt          crypt;
        irods::buffer_crypt::array_t out_buf;
        if (const auto err = crypt.encrypt(key, key, in_buf, out_buf); !err.ok()) {
            return PASS(err);
        }

        Hasher hasher;
        if (const auto err = getHasher(MD5_NAME, hasher); !err.ok()) {
            return PASS(err);
        }
        hasher.update( std::string( reinterpret_cast<char*>( out_buf.data() ), out_buf.size() ) );
        hasher.digest( _signed_zone_key );

        return SUCCESS();
    } // sign_server_sid

    /// =-=-=-=-=-=-=-
    /// @brief convenience class to initialize the table and index map for negotiations
    class client_server_negotiations_context
    {
        typedef std::map < std::string, int > negotiation_map_t;
        typedef std::pair< std::string, int > negotiation_pair_t;
    public:
        client_server_negotiations_context() {
            // =-=-=-=-=-=-=-
            // initialize the negotiation context
            cs_neg_param_map.insert( negotiation_pair_t( CS_NEG_REQUIRE,   0 ) );
            cs_neg_param_map.insert( negotiation_pair_t( CS_NEG_DONT_CARE, 1 ) );
            cs_neg_param_map.insert( negotiation_pair_t( CS_NEG_REFUSE,    2 ) );

            // =-=-=-=-=-=-=-
            // table is indexed as[ CLIENT ][ SERVER ]
            client_server_negotiations_table[ 0 ][ 0 ] = CS_NEG_USE_SSL; // REQ, REQ
            client_server_negotiations_table[ 0 ][ 1 ] = CS_NEG_USE_SSL; // REQ, DC
            client_server_negotiations_table[ 0 ][ 2 ] = CS_NEG_FAILURE; // REQ, REF
            client_server_negotiations_table[ 1 ][ 0 ] = CS_NEG_USE_SSL; // DC,  REQ
            client_server_negotiations_table[ 1 ][ 1 ] = CS_NEG_USE_SSL; // DC,  DC
            client_server_negotiations_table[ 1 ][ 2 ] = CS_NEG_USE_TCP; // DC,  REF
            client_server_negotiations_table[ 2 ][ 0 ] = CS_NEG_FAILURE; // REF, REQ
            client_server_negotiations_table[ 2 ][ 1 ] = CS_NEG_USE_TCP; // REF, DC
            client_server_negotiations_table[ 2 ][ 2 ] = CS_NEG_USE_TCP; // REF, REF

        } // ctor

        error operator()(
            const std::string& _cli_policy,
            const std::string& _svr_policy,
            std::string&       _result ) {
            // =-=-=-=-=-=-=-
            // convert client policy to an index
            // in order to reference the negotiation table
            int cli_idx = cs_neg_param_map[ _cli_policy ];
            if ( cli_idx > 2 || cli_idx < 0 ) {
                return ERROR( SYS_INVALID_INPUT_PARAM,
                              "client policy index is out of bounds" );

            }

            // =-=-=-=-=-=-=-
            // convert server policy to an index
            // in order to reference the negotiation table
            int svr_idx = cs_neg_param_map[ _svr_policy ];
            if ( svr_idx > 2 || svr_idx < 0 ) {
                return ERROR( SYS_INVALID_INPUT_PARAM,
                              "server policy index is out of bounds" );

            }

            // =-=-=-=-=-=-=-
            // politely ask for the SSL usage results
            _result = client_server_negotiations_table[ cli_idx ][ svr_idx ];

            return SUCCESS();

        } // operator()

    private:
        /// =-=-=-=-=-=-=-
        /// @brief table which describes the negotiation choices
        std::string client_server_negotiations_table[3][3];

        /// =-=-=-=-=-=-=-
        /// @brief map from policy to a table index
        negotiation_map_t cs_neg_param_map;
    }; // class client_server_negotiations_context

    /// =-=-=-=-=-=-=-
    /// @brief function which determines if a client/server negotiation is needed
    ///        on the client side
    bool do_client_server_negotiation_for_client( ) {
        // =-=-=-=-=-=-=-
        // get the irods environment so we can compare the
        // flag for negotiation of policy
        rodsEnv rods_env;
        int status = getRodsEnv( &rods_env );
        if ( status < 0 ) {
            return false;
        }

        // =-=-=-=-=-=-=-
        // if it is not set then move on
        std::string neg_policy( rods_env.rodsClientServerNegotiation );
        if ( neg_policy.empty() ) {
            return false;
        }

        // =-=-=-=-=-=-=-
        // if it is set then check for our magic token which requests
        // the negotiation, if its not there then return success
        if ( std::string::npos == neg_policy.find( REQ_SVR_NEG ) ) {
            return false;
        }

        // =-=-=-=-=-=-=-
        // otherwise, its a failure.
        return true;
    } // do_client_server_negotiation_for_client

    /// =-=-=-=-=-=-=-
    /// @brief function which determines if a client/server negotiation is needed
    ///        on the server side
    bool do_client_server_negotiation_for_server( ) {
        // =-=-=-=-=-=-=-
        // check the SP_OPTION for the string stating a negotiation is requested
        char* opt_ptr = getenv( RODS_CS_NEG );

        // =-=-=-=-=-=-=-
        // if it is not set then move on
        if ( !opt_ptr || strlen( opt_ptr ) == 0 ) {
            return false;
        }

        // =-=-=-=-=-=-=-
        // if it is set then check for our magic token which requests
        // the negotiation, if it is not the magic token, move on
        std::string opt_str( opt_ptr );
        if ( std::string::npos == opt_str.find( REQ_SVR_NEG ) ) {
            return false;
        }

        // =-=-=-=-=-=-=-
        // otherwise, its a go.
        return true;
    } // do_client_server_negotiation_for_server

    /// =-=-=-=-=-=-=-
    /// @brief function which manages the TLS and Auth negotiations with the client
    error client_server_negotiation_for_client(
        irods::network_object_ptr _ptr,
        const std::string&        _host_name,
        std::string&              _result )
    {
        // =-=-=-=-=-=-=-
        // we requested a negotiation, wait for the response from CS_NEG_SVR_1_MSG
        boost::shared_ptr< cs_neg_t > cs_neg;
        error err = read_client_server_negotiation_message( _ptr, cs_neg );
        if ( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // get the server requested policy
        std::string svr_policy( cs_neg->result_ );
        if ( svr_policy.empty() || cs_neg->status_ != CS_NEG_STATUS_SUCCESS ) {
            std::stringstream msg;
            msg << "invalid result [" << cs_neg->result_ << "]  or status: " << cs_neg->status_;
            return ERROR(
                       CLIENT_NEGOTIATION_ERROR,
                       msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the irods environment so we can compare the
        // policy in the irods_environment.json file
        rodsEnv rods_env;
        int status = getRodsEnv( &rods_env );
        if ( status < 0 ) {
            return ERROR( status, "failed in getRodsEnv" );
        }

        // =-=-=-=-=-=-=-
        // if the policy is empty, then default to DONT_CARE
        std::string cli_policy( rods_env.rodsClientServerPolicy );
        if ( cli_policy.empty() ) {
            cli_policy = CS_NEG_DONT_CARE;
        }

        // =-=-=-=-=-=-=-
        // perform the negotiation
        client_server_negotiations_context negotiate;
        std::string result;
        error neg_err = negotiate( cli_policy, svr_policy, result );

        // =-=-=-=-=-=-=-
        // aggregate the error stack if necessary
        error ret = SUCCESS();
        if ( !neg_err.ok() ) {
            ret = PASSMSG( "failed in negotiation context", neg_err );
        }

        // =-=-=-=-=-=-=-
        // handle failure - send a failure msg back to client
        if ( !err.ok() || CS_NEG_FAILURE == result ) {
            // =-=-=-=-=-=-=-
            // send CS_NEG_CLI_1_MSG, failure message to the server
            cs_neg_t send_cs_neg;
            send_cs_neg.status_ = CS_NEG_STATUS_FAILURE;
            snprintf( send_cs_neg.result_, sizeof( send_cs_neg.result_ ),
                      "%s", CS_NEG_FAILURE.c_str() );
            error send_err = send_client_server_negotiation_message(
                                 _ptr,
                                 send_cs_neg );
            if ( !send_err.ok() ) {
                ret = PASSMSG( "failed to send CS_NEG_CLI1_MSG Failure Message", send_err );
            }

            std::stringstream msg;
            msg << "client-server negotiations failed for server request [";
            msg << svr_policy << "] and client request [" << cli_policy << "]";
            ret = ERROR(
                      CLIENT_NEGOTIATION_ERROR,
                      msg.str() );
            return ret;
        }

        // =-=-=-=-=-=-=-
        // attempt to get the server config, if we can then we must be an
        // Agent.  sign the SID and send it showing that we are a trusted
        // Agent and not an actual Client ( icommand, jargon connection etc )
        std::string cli_msg;

        try {
            // If server_config cannot be read, this must be a "pure" client. An
            // irods::exception will be thrown in that case.
            server_properties::instance();

            try {
                boost::optional<const std::string&> zone_key;
                try {
                    zone_key.reset(irods::get_server_property<const std::string>(irods::CFG_ZONE_KEY_KW));
                } catch (const irods::exception&) {
                    zone_key.reset(irods::get_server_property<const std::string>(LOCAL_ZONE_SID_KW));
                }

                const std::string& neg_key = determine_negotiation_key(_host_name);
                if (!negotiation_key_is_valid(neg_key)) {
                    // The communication with the remote server should be continued so that
                    // it can be torn down cleanly, even if there is something wrong with
                    // the negotiation_key. The remote server will detect this and shut
                    // down the connection. Issue a warning in the log here as a hint to
                    // the Zone administrator.
                    irods::log(LOG_WARNING, fmt::format(
                        "[{}:{}] - negotiation_key is invalid",
                        __func__, __LINE__));
                }

                std::string signed_zone_key;
                if (const auto err = sign_server_sid(*zone_key, neg_key, signed_zone_key);
                    !err.ok()) {
                    // Even if the signing of the zone_key fails, we continue because the
                    // other side of the connection will reject any further communications
                    // due to the missing keyword. This will result in a clean disconnect.
                    // That is why we do not return an error here.
                    rodsLog(LOG_WARNING, "%s", PASS(err).result().c_str());
                }

                // Add the signed zone_key to the message to be sent to the server, even if the
                // signing failed due to invalid negotiation_key.
                cli_msg += CS_NEG_SID_KW +
                    irods::kvp_association() +
                    signed_zone_key +
                    irods::kvp_delimiter();
            } catch (const irods::exception& e) {
                irods::log(LOG_WARNING, fmt::format(
                    "[{}:{}] - failed to retrieve and sign local zone_key [{}]",
                    __func__, __LINE__, e.client_display_what()));
                return irods::error(e);
            }
        } catch (const irods::exception& e) {
            // This is a pure client, just continue.
        }

        // =-=-=-=-=-=-=-
        // tack on the rest of the result
        cli_msg += CS_NEG_RESULT_KW         +
                   irods::kvp_association() +
                   result                   +
                   irods::kvp_delimiter();

        // =-=-=-=-=-=-=-
        // send CS_NEG_CLI_1_MSG, success message to the server with our choice
        cs_neg_t send_cs_neg;
        send_cs_neg.status_ = CS_NEG_STATUS_SUCCESS;
        snprintf( send_cs_neg.result_, sizeof( send_cs_neg.result_ ), "%s", cli_msg.c_str() );
        err = send_client_server_negotiation_message(
                  _ptr,
                  send_cs_neg );
        if ( !err.ok() ) {
            return PASSMSG( "failed to send CS_NEG_CLI_1_MSG Success Message", err );
        }

        // =-=-=-=-=-=-=-
        // set the out variable and return
        _result = result;

        return SUCCESS();
    } // client_server_negotiation_for_client

    /// =-=-=-=-=-=-=-
    /// @brief function which sends the negotiation message
    error send_client_server_negotiation_message(
        irods::network_object_ptr _ptr,
        cs_neg_t&                  _cs_neg_msg )
    {
        // =-=-=-=-=-=-=-
        // pack the negotiation message
        bytesBuf_t* cs_neg_buf = 0;
        int status = pack_struct(&_cs_neg_msg,
                                 &cs_neg_buf,
                                 "CS_NEG_PI",
                                 RodsPackTable,
                                 0, XML_PROT, nullptr);
        if ( status < 0 ) {
            return ERROR( status, "failed to pack client-server message" );
        }

        // =-=-=-=-=-=-=-
        // pack the negotiation message
        irods::error ret = sendRodsMsg( _ptr,
                                        RODS_CS_NEG_T,
                                        cs_neg_buf,
                                        0, 0, 0,
                                        XML_PROT );
        freeBBuf( cs_neg_buf );
        if ( !ret.ok() ) {
            return PASSMSG( "failed to send client-server negotiation message", ret );
        }

        return SUCCESS();
    } // send_client_server_negotiation_message

    /// =-=-=-=-=-=-=-
    /// @brief function which sends the negotiation message
    error read_client_server_negotiation_message(
        irods::network_object_ptr      _ptr,
        boost::shared_ptr< cs_neg_t >&  _cs_neg_msg )
    {
        // =-=-=-=-=-=-=-
        // read the message header
        struct timeval tv;
        tv.tv_sec = READ_VERSION_TOUT_SEC;
        tv.tv_usec = 0;

        msgHeader_t msg_header;
        irods::error ret = readMsgHeader( _ptr, &msg_header, &tv );
        if ( !ret.ok() ) {
            return PASSMSG( "read message header failed", ret );
        }

        // =-=-=-=-=-=-=-
        // read the message body
        bytesBuf_t struct_buf{}, data_buf{}, error_buf{};
        ret = readMsgBody(
                  _ptr,
                  &msg_header,
                  &struct_buf,
                  &data_buf,
                  &error_buf,
                  XML_PROT, 0 );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // check that we did in fact get the right type of message
        if ( strcmp( msg_header.type, RODS_CS_NEG_T ) != 0 ) {
            // =-=-=-=-=-=-=-
            // trap potential case where server does not support
            // advanced negotiation.  a version msg would be sent
            // back instead.
            if ( strcmp( msg_header.type, RODS_VERSION_T ) == 0 ) {
                // =-=-=-=-=-=-=-
                // unpack the version struct to check the status
                version_t* version = 0;
                int status = unpack_struct(
                                 struct_buf.buf,
                                 ( void ** )( static_cast<void *>( &version ) ),
                                 "Version_PI",
                                 RodsPackTable,
                                 XML_PROT, nullptr);

                if ( struct_buf.buf ) {
                    free( struct_buf.buf );
                }
                if ( data_buf.buf ) {
                    free( data_buf.buf );
                }
                if ( error_buf.buf ) {
                    free( error_buf.buf );
                }

                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "read_client_server_negotiation_message :: unpackStruct FAILED" );
                    return ERROR( status, "unpackStruct failed" );
                }

                if ( version->status < 0 ) {
                    rodsLog( LOG_ERROR, "read_client_server_negotiation_message :: received error message %d", version->status );
                    return ERROR( version->status, "negotiation failed" );
                }
                else {
                    // =-=-=-=-=-=-=-
                    // if no negotiation is allowed then provide a readable
                    // error for the client
                    std::stringstream msg;
                    msg << "received [" << msg_header.type << "] ";
                    msg << "but expected [" << RODS_CS_NEG_T << "]\n\n";
                    msg << "\t*** Advanced negotiation is enabled in this iRODS environment   ***\n";
                    msg << "\t*** which is most likely not supported by the server.           ***\n";
                    msg << "\t*** Comment out irods_client_server_negotiation in the          ***\n";
                    msg << "\t*** irods_environment.json file to disable.                     ***\n";
                    return ERROR( ADVANCED_NEGOTIATION_NOT_SUPPORTED, msg.str() );
                }
            }
            else {
                // =-=-=-=-=-=-=-
                // something entirely unexpected happened
                std::stringstream msg;
                msg << "wrong message type [" << msg_header.type << "] ";
                msg << "expected [" << RODS_CS_NEG_T << "]";
                return ERROR( SYS_HEADER_TYPE_LEN_ERR, msg.str() );
            }
        }

        // =-=-=-=-=-=-=-
        // check that we did not get any data with the message
        if ( msg_header.bsLen != 0 ) {
            if ( data_buf.buf != NULL ) {
                free( data_buf.buf );
            }
            rodsLog( LOG_NOTICE,
                     "read_client_server_negotiation_message: msg_header.bsLen = %d is not 0",
                     msg_header.bsLen );
        }

        // =-=-=-=-=-=-=-
        // check that we did not get anything in the error buffer
        if ( msg_header.errorLen != 0 ) {
            if ( error_buf.buf ) {
                free( error_buf.buf );
            }
            rodsLog( LOG_NOTICE,
                     "read_client_server_negotiation_message: msg_header.errorLen = %d is not 0",
                     msg_header.errorLen );
        }

        // =-=-=-=-=-=-=-
        // check that we did get an appropriately sized message
        if ( msg_header.msgLen > ( int ) sizeof( irods::cs_neg_t ) * 2 ||
                msg_header.msgLen <= 0 ) {
            if ( struct_buf.buf != NULL ) {
                free( struct_buf.buf );
            }
            std::stringstream msg;
            msg << "message length is invalid: " << msg_header.msgLen << " vs " << sizeof( irods::cs_neg_t );
            return ERROR( SYS_HEADER_READ_LEN_ERR, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // do an unpack into our out variable using the xml protocol
        cs_neg_t* tmp_cs_neg = 0;
        int status = unpack_struct( struct_buf.buf,
                                   ( void ** )( static_cast<void *>( &tmp_cs_neg ) ),
                                   "CS_NEG_PI",
                                   RodsPackTable,
                                   XML_PROT, nullptr );
        free( struct_buf.buf );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR, "read_client_server_negotiation_message :: unpackStruct FAILED" );
            return ERROR( status, "unpackStruct failed" );
        }

        _cs_neg_msg.reset( tmp_cs_neg, free );

        // =-=-=-=-=-=-=-
        // win!!!111one
        return SUCCESS();
    } // read_client_server_negotiation_message
} // namespace irods

