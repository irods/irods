


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_client_server_negotiation.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "index.h"
#include "reFuncDefs.h"

// =-=-=-=-=-=-=-
// stl includes
#include <map>


namespace eirods {
    /// =-=-=-=-=-=-=-
    /// @brief convenience class to initialize the table and index map for negotiations
    class client_server_negotiations_context {
        typedef std::map < std::string, int > negotiation_map_t;
        typedef std::pair< std::string, int > negotiation_pair_t;
    public:
        client_server_negotiations_context() {
            // =-=-=-=-=-=-=-
            // initialize the neogitation context
            cs_neg_param_map.insert( negotiation_pair_t( CS_NEG_REQUIRE,   0 ) );
            cs_neg_param_map.insert( negotiation_pair_t( CS_NEG_DONT_CARE, 1 ) );
            cs_neg_param_map.insert( negotiation_pair_t( CS_NEG_REFUSE,    2 ) );

            // =-=-=-=-=-=-=-
            // table is indexed as[ CLIENT ][ SERVER ] 
            client_server_negotiations_table[ 0 ][ 0 ] = CS_NEG_USE_SSL;      // REQ, REQ
            client_server_negotiations_table[ 0 ][ 1 ] = CS_NEG_USE_SSL;      // REQ, DC
            client_server_negotiations_table[ 0 ][ 2 ] = CS_NEG_FAILURE;      // REQ, REF
            client_server_negotiations_table[ 1 ][ 0 ] = CS_NEG_USE_SSL;      // DC,  REQ
            client_server_negotiations_table[ 1 ][ 1 ] = CS_NEG_USE_SSL;      // DC,  DC
            client_server_negotiations_table[ 1 ][ 2 ] = CS_NEG_DONT_USE_SSL; // DC,  REF
            client_server_negotiations_table[ 2 ][ 0 ] = CS_NEG_FAILURE;      // REF, REQ
            client_server_negotiations_table[ 2 ][ 1 ] = CS_NEG_DONT_USE_SSL; // REF, DC
            client_server_negotiations_table[ 2 ][ 2 ] = CS_NEG_DONT_USE_SSL; // REF, REF
           
        } // ctor

        error operator()( 
            const std::string& _cli_policy,
            const std::string& _svr_policy,
            std::string&       _result ) {
            // =-=-=-=-=-=-=-
            // convert client policy to an index
            // in order to reference the negotiation table
            int cli_idx = cs_neg_param_map[ _cli_policy ];
            if( cli_idx > 2 || cli_idx < 0 ) {
                return ERROR( SYS_INVALID_INPUT_PARAM, 
                              "client policy index is out of bounds" );
            
            }
            
            // =-=-=-=-=-=-=-
            // convert server policy to an index
            // in order to reference the negotiation table
            int svr_idx = cs_neg_param_map[ _svr_policy ];
            if( svr_idx > 2 || svr_idx < 0 ) {
                return ERROR( SYS_INVALID_INPUT_PARAM, 
                              "server policy index is out of bounds" );

            }

            // =-=-=-=-=-=-=-
            // politely ask for the SSL usage results
            _result = client_server_negotiations_table[ cli_idx ][ svr_idx ];
std::cout << "client_server_negotiations_context - result [" << _result << "]" << std::endl;
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
    bool do_client_server_negotiation(  ) {
        // =-=-=-=-=-=-=-
        // check the SP_OPTION for the string stating a negotiation is requested
        char* opt_ptr = getenv( RODS_CS_NEG );

        // =-=-=-=-=-=-=-
        // if it is not set then move on
        if( !opt_ptr || strlen( opt_ptr ) == 0 ) {
            return false;
        }

        // =-=-=-=-=-=-=-
        // if it is set then check for our magic token which requests 
        // the negotiation, if its not there then return success
        std::string opt_str( opt_ptr );
        if( std::string::npos == opt_str.find( REQ_SVR_NEG ) ) { 
            return false;
        }

        // =-=-=-=-=-=-=-
        // otherwise, its a go.
        return true;

    } // do_client_server_negotiation
 
    /// =-=-=-=-=-=-=-
    /// @brief function which manages the TLS and Auth negotiations with the client
    error client_server_negotiation_for_client( 
        eirods::network_object_ptr _ptr,
        std::string&               _result ) {
        // =-=-=-=-=-=-=-
        // prep the out variable
        _result.clear();

        // =-=-=-=-=-=-=-
        // we requested a negotiation, wait for the response from CS_NEG_SVR_1_MSG
        boost::shared_ptr< cs_neg_t > cs_neg;
        error err = read_client_server_negotiation_message( _ptr, cs_neg );
        if( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // get the server requested policy
        std::string svr_policy( cs_neg->result_ );
        if( svr_policy.empty() || cs_neg->status_ != CS_NEG_STATUS_SUCCESS ) {
            std::stringstream msg;
            msg << "invalid result [" << cs_neg->result_ << "]  or status: " << cs_neg->status_;
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the irods environment so we can compare the 
        // policy in the .irodsEnv file
        rodsEnv rods_env;
        int status = getRodsEnv( &rods_env );
        if( status < 0 ) {
            return ERROR( status, "failed in getRodsEnv" );
        }

        // =-=-=-=-=-=-=-
        // if the policy is empty, then default to DONT_CARE
        std::string cli_policy( rods_env.rodsClientServerPolicy );
        if( cli_policy.empty() ) {
            cli_policy = CS_NEG_DONT_CARE; 
        
        }

        // =-=-=-=-=-=-=-
        // perform the negotation
        client_server_negotiations_context negotiate;
        std::string result;
        error neg_err = negotiate( cli_policy, svr_policy, result );

        // =-=-=-=-=-=-=-
        // aggregate the error stack if necessary
        error ret = SUCCESS();
        if( !neg_err.ok() ) {
           ret = PASSMSG( "failed in negotation context", neg_err ); 
        }

        // =-=-=-=-=-=-=-
        // handle failure - send a failure msg back to client
        if( !err.ok() || CS_NEG_FAILURE == result ) {
            // =-=-=-=-=-=-=-
            // send CS_NEG_CLI_1_MSG, failure message to the server
            cs_neg_t send_cs_neg;
            send_cs_neg.status_ = CS_NEG_STATUS_FAILURE;
            strncpy( send_cs_neg.result_, CS_NEG_FAILURE.c_str(), CS_NEG_FAILURE.size() );
            
            error send_err = send_client_server_negotiation_message( 
                                 _ptr, 
                                 send_cs_neg );
            if( !send_err.ok() ) {
                ret = PASSMSG( "failed to send CS_NEG_CLI1_MSG Failure Messsage", send_err );
            }

            std::stringstream msg;
            msg << "client-server negoations failed for server request [";
            msg << svr_policy << "] and client request [" << cli_policy << "]";
            ret = PASSMSG( msg.str(), ret );
            return ret;
        }

        // =-=-=-=-=-=-=-
        // send CS_NEG_CLI_1_MSG, success message to the server with our choice 
        cs_neg_t send_cs_neg;
        send_cs_neg.status_ = CS_NEG_STATUS_SUCCESS;
        strncpy( send_cs_neg.result_, result.c_str(), result.size() );
        err = send_client_server_negotiation_message( 
                  _ptr, 
                  send_cs_neg );
        if( !err.ok() ) {
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
        eirods::network_object_ptr _ptr, 
        cs_neg_t&                  _cs_neg_msg ) {
        // =-=-=-=-=-=-=-
        // pack the negotiation message
        bytesBuf_t* cs_neg_buf = 0;
        int status = packStruct( &_cs_neg_msg, 
                                 &cs_neg_buf,
                                 "CS_NEG_PI", 
                                 RodsPackTable, 
                                 0, XML_PROT );
        if( status < 0 ) {
            return ERROR( status, "failed to pack client-server message" );
        }

        // =-=-=-=-=-=-=-
        // pack the negotiation message
        eirods::error ret = sendRodsMsg( _ptr, 
                              RODS_CS_NEG_T, 
                              cs_neg_buf, 
                              0, 0, 0,
                              XML_PROT );
        freeBBuf( cs_neg_buf );
        if( !ret.ok() ) {
            return PASSMSG( "failed to send client-server negotiation message", ret );
        
        }

        return SUCCESS();

    } // send_client_server_negotiation_message

    /// =-=-=-=-=-=-=-
    /// @brief function which sends the negotiation message
    error read_client_server_negotiation_message( 
        eirods::network_object_ptr      _ptr, 
        boost::shared_ptr< cs_neg_t >&  _cs_neg_msg ) {
        // =-=-=-=-=-=-=-
        // read the message header 
        struct timeval tv;
        tv.tv_sec = READ_VERSION_TOUT_SEC;
        tv.tv_usec = 0;

        msgHeader_t msg_header;
        eirods::error ret = readMsgHeader( _ptr, &msg_header, &tv );
        if( !ret.ok() ) {
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
        if( !ret.ok() ) {
            return PASS( ret );
        
        }

        // =-=-=-=-=-=-=-
        // check that we did in fact get the right type of message
        if( strcmp( msg_header.type, RODS_CS_NEG_T ) != 0 ) {
            if( struct_buf.buf )
                free( struct_buf.buf );
            if( data_buf.buf )
                free( data_buf.buf );
            if( error_buf.buf )
                free( error_buf.buf );
            std::stringstream msg;
            msg << "wrong message type [" << msg_header.type << "] ";
            msg << "expected [" << RODS_CS_NEG_T << "]";
            return ERROR( SYS_HEADER_TYPE_LEN_ERR, msg.str() );
        }
     
        // =-=-=-=-=-=-=-
        // check that we did not get any data with the message
        if( msg_header.bsLen != 0 ) {
            if( data_buf.buf != NULL )
                free ( data_buf.buf );
            rodsLog( LOG_NOTICE, 
                     "read_client_server_negotiation_message: msg_header.bsLen = %d is not 0",
                     msg_header.bsLen );
        }

        // =-=-=-=-=-=-=-
        // check that we did not get anything in the error buffer
        if( msg_header.errorLen != 0 ) {
            if( error_buf.buf )
                free ( error_buf.buf );
            rodsLog( LOG_NOTICE, 
                     "read_client_server_negotiation_message: msg_header.errorLen = %d is not 0",
                     msg_header.errorLen );
        }

        // =-=-=-=-=-=-=-
        // check that we did get an appropriately sized message
        if( msg_header.msgLen > (int) sizeof( eirods::cs_neg_t ) * 2 || 
            msg_header.msgLen <= 0 ) {
            if ( struct_buf.buf != NULL)
                free ( struct_buf.buf);
            std::stringstream msg;
            msg << "message length is invalid: " << msg_header.msgLen << " vs " << sizeof( eirods::cs_neg_t );
            return ERROR( SYS_HEADER_READ_LEN_ERR, msg.str() );
        }
        
        // =-=-=-=-=-=-=-
        // do an unpack into our out variable using the xml protocol
        cs_neg_t* tmp_cs_neg = 0;
        int status = unpackStruct( struct_buf.buf, 
                         (void **) &tmp_cs_neg,
                         "CS_NEG_PI", 
                         RodsPackTable, 
                         XML_PROT );
        free( struct_buf.buf );
        if( status < 0 ) {
            rodsLog( LOG_ERROR, "read_client_server_negotiation_message :: unpackStruct FAILED" );
            return ERROR( status, "unpackStruct failed" );
                          
        } 

         _cs_neg_msg.reset( tmp_cs_neg );

        // =-=-=-=-=-=-=-
        // win!!!111one
        return SUCCESS();

    } // read_client_server_negotiation_message

}; // namespace eirods









































