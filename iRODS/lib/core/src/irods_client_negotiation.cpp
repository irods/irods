// =-=-=-=-=-=-=-
#include "irods_client_server_negotiation.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_properties.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_buffer_encryption.hpp"
#include "irods_hasher_factory.hpp"
#include "MD5Strategy.hpp"
#include "sockComm.hpp"
#include "sockCommNetworkInterface.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "rodsConnect.h"
#include "index.hpp"
#include "reFuncDefs.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <map>


namespace irods {

/// =-=-=-=-=-=-=-
/// @brief given a buffer encrypt and hash it for negotiation
    error sign_server_sid(
        const std::string _svr_sid,
        const std::string _enc_key,
        std::string&      _signed_sid ) {
        // =-=-=-=-=-=-=-
        // create an encryption object
        // 32 byte key, 8 byte iv, 16 rounds encryption
        irods::buffer_crypt          crypt;
        irods::buffer_crypt::array_t key;

        // leverage iteration to copy from std::string to a std::vector<>
        key.assign( _enc_key.begin(), _enc_key.end() );

        irods::buffer_crypt::array_t in_buf;
        // leverage iteration to copy from std::string to a std::vector<>
        in_buf.assign( _svr_sid.begin(), _svr_sid.end() );

        irods::buffer_crypt::array_t out_buf;
        irods::error err = crypt.encrypt(
                               key,
                               key, // reuse key as iv
                               in_buf,
                               out_buf );
        if ( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // hash the encrypted sid
        Hasher hasher;
        err = getHasher( MD5_NAME, hasher );
        hasher.update( std::string( reinterpret_cast<char*>( out_buf.data() ), out_buf.size() ) );
        hasher.digest( _signed_sid );

        return SUCCESS();

    } // sign_server_sid

/// =-=-=-=-=-=-=-
/// @brief convenience class to initialize the table and index map for negotiations
    class client_server_negotiations_context {
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
        // the negotiation, if its not there then return success
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
        std::string&              _result ) {

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
            return ERROR( -1, msg.str() );
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
                ret = PASSMSG( "failed to send CS_NEG_CLI1_MSG Failure Messsage", send_err );
            }

            std::stringstream msg;
            msg << "client-server negoations failed for server request [";
            msg << svr_policy << "] and client request [" << cli_policy << "]";
            ret = PASSMSG( msg.str(), ret );
            return ret;
        }

        // =-=-=-=-=-=-=-
        // attempt to get the server config, if we can then we must be an
        // Agent.  sign the SID and send it showing that we are a trusted
        // Agent and not an actual Client ( icommand, jargon connection etc )
        std::string cli_msg;

        // =-=-=-=-=-=-=-
        // if we cannot read a server config file, punt
        // as this must be a client-side situation
        server_properties& props = server_properties::getInstance();
        err = props.capture_if_needed();
        if ( err.ok() ) {
            // =-=-=-=-=-=-=-
            // get our local zone SID
            std::string sid;
            err = props.get_property <
                  std::string > (
                      LOCAL_ZONE_SID_KW,
                      sid );
            if ( err.ok() ) {
                std::string enc_key;
                err = props.get_property <
                      std::string > (
                          AGENT_KEY_KW,
                          enc_key );
                if ( err.ok() ) {
                    // =-=-=-=-=-=-=-
                    // sign the SID
                    std::string signed_sid;
                    err = sign_server_sid(
                              sid,
                              enc_key,
                              signed_sid );
                    if ( err.ok() ) {
                        // =-=-=-=-=-=-=-
                        // add the SID to the returning client message
                        cli_msg += CS_NEG_SID_KW             +
                                   irods::kvp_association()  +
                                   signed_sid                +
                                   irods::kvp_delimiter();
                    }
                    else {
                        rodsLog(
                            LOG_DEBUG,
                            "%s",
                            PASS( err ).result().c_str() );
                    }
                }
                else {
                    rodsLog(
                        LOG_DEBUG,
                        "failed to get agent key" );
                }
            }
            else {
                rodsLog(
                    LOG_DEBUG,
                    "failed to get local zone SID" );
            }

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
        cs_neg_t&                  _cs_neg_msg ) {
        // =-=-=-=-=-=-=-
        // pack the negotiation message
        bytesBuf_t* cs_neg_buf = 0;
        int status = packStruct( &_cs_neg_msg,
                                 &cs_neg_buf,
                                 "CS_NEG_PI",
                                 RodsPackTable,
                                 0, XML_PROT );
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
        boost::shared_ptr< cs_neg_t >&  _cs_neg_msg ) {
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
        bytesBuf_t struct_buf, data_buf, error_buf;
        memset( &data_buf, 0, sizeof( bytesBuf_t ) );
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
                int status = unpackStruct(
                                 struct_buf.buf,
                                 ( void ** )( static_cast<void *>( &version ) ),
                                 "Version_PI",
                                 RodsPackTable,
                                 XML_PROT );

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
                    // if no negoation is allowed then provide a readable
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
        int status = unpackStruct( struct_buf.buf,
                                   ( void ** )( static_cast<void *>( &tmp_cs_neg ) ),
                                   "CS_NEG_PI",
                                   RodsPackTable,
                                   XML_PROT );
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

}; // namespace irods









































