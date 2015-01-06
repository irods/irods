

#include "avro/Encoder.hh"
#include "avro/Decoder.hh"
#include "avro/Specific.hh"

#include "genQuery.hpp"
#include "rcMisc.hpp"
#include "irods_log.hpp"
#include "irods_server_control_plane.hpp"
#include "irods_server_properties.hpp"
#include "irods_buffer_encryption.hpp"

#include "irods_server_state.hpp"
#include "irods_stacktrace.hpp"

namespace irods {


    static error get_server_properties( 
        const std::string&     _port_keyword,
        int&                   _port,
        int&                   _num_rounds,
        buffer_crypt::array_t& _key,
        std::string&           _algorithm ) {

        error ret = get_server_property< int > (
                        _port_keyword,
                        _port );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

        std::string key;
        ret = get_server_property <
              std::string > (
                  CFG_SERVER_CONTROL_PLANE_KEY,
                  key );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

        ret = get_server_property <
              int > (
                  CFG_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW,
                  _num_rounds );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

        std::string encryption_algorithm;
        ret = get_server_property <
              std::string > (
                  CFG_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM_KW,
                  _algorithm );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

        // convert string to array_t 
        _key.assign( 
            key.begin(),
            key.end() );

        return SUCCESS();

    } // get_server_properties

    static error forward_server_control_command(
        const std::string& _name,
        const std::string& _host,
        const std::string& _port_keyword,
        std::string&       _output ) {
        int time_out = 0;
        error ret = get_server_property <
                        int > (
                            CFG_SERVER_CONTROL_PLANE_TIMEOUT,
                            time_out );
        if ( !ret.ok() ) {
            return PASS( ret );

        }
        
        int port = 0, num_hash_rounds = 0;
        std::string encryption_algorithm;
        buffer_crypt::array_t shared_secret;

        ret = get_server_properties( 
                  _port_keyword,
                  port, 
                  num_hash_rounds,
                  shared_secret,
                  encryption_algorithm );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

        // stringify the port
        std::stringstream port_sstr;
        port_sstr << port;

        // standard zmq rep-req communication pattern
        zmq::context_t zmq_ctx( 1 );
        zmq::socket_t  zmq_skt( zmq_ctx, ZMQ_REQ );

        zmq_skt.setsockopt( ZMQ_RCVTIMEO, &time_out, sizeof( time_out ) );
        zmq_skt.setsockopt( ZMQ_SNDTIMEO, &time_out, sizeof( time_out ) );

        // this is the client so we connect rather than bind
        std::string conn_str( "tcp://" );
        conn_str += _host;
        conn_str += ":";
        conn_str += port_sstr.str();
        zmq_skt.connect( conn_str.c_str() );

        // build the command to forward
        control_plane_command cmd;
        cmd.command = _name;
        cmd.options[ SERVER_CONTROL_OPTION_KW ] = SERVER_CONTROL_HOSTS_OPT;
        cmd.options[ SERVER_CONTROL_HOST_KW ]   = _host;

        // serialize using the generated avro class
        std::auto_ptr< avro::OutputStream > out = avro::memoryOutputStream();
        avro::EncoderPtr e = avro::binaryEncoder();
        e->init( *out );
        avro::encode( *e, cmd );
        boost::shared_ptr< std::vector< uint8_t > > data = avro::snapshot( *out );

        buffer_crypt crypt(
                shared_secret.size(),  // key size
                0,                     // salt size ( we dont send a salt )
                num_hash_rounds,       // num hash rounds
                encryption_algorithm.c_str());

        buffer_crypt::array_t iv;
        buffer_crypt::array_t data_to_send;
        buffer_crypt::array_t data_to_encrypt( 
                                  data->data(),
                                  data->data()+data->size() );
        ret = crypt.encrypt(
                  shared_secret,
                  iv,
                  data_to_encrypt,
                  data_to_send );
        if( !ret.ok() ) {
            return PASS( ret );

        }

        // copy binary encoding into a zmq message for transport
        zmq::message_t rep( data_to_send.size() );
        memcpy(
            rep.data(),
            data_to_send.data(),
            data_to_send.size() );
        zmq_skt.send( rep );

        // wait for the server reponse
        zmq::message_t req;
        zmq_skt.recv( &req );

        std::string rep_str(
            static_cast< char* >( req.data() ),
            req.size() );
        if ( SERVER_CONTROL_SUCCESS != rep_str ) {
            // check if the result is really an error or a status
            if ( std::string::npos == rep_str.find( "[-]" ) ) {
                _output += rep_str;

            }
            else {
                return ERROR(
                           CONTROL_PLANE_MESSAGE_ERROR,
                           rep_str );

            }
        }

        return SUCCESS();

    } // forward_server_control_command

    static error server_operation_shutdown(
        std::string& _output ) {
        rodsEnv my_env;
        getRodsEnv( &my_env );
        _output += "[ shutting down ";
        _output += my_env.rodsHost;
        _output += " ]";
        _output += "\n";
        // rule engine server only runs on IES
#ifdef RODS_CAT
        std::string output;
        error ret = forward_server_control_command(
                        SERVER_CONTROL_SHUTDOWN,
                        my_env.rodsHost,
                        CFG_RULE_ENGINE_CONTROL_PLANE_PORT,
                        output );
        if ( !ret.ok() ) {
            error msg = PASS( ret );
            log( msg );
            _output = msg.result();
        }
#endif

        server_state& s = server_state::instance();
        s( server_state::STOPPED );
        return SUCCESS();

    } // server_operation_shutdown

    static error rule_engine_operation_shutdown(
        std::string& _output ) {
        rodsEnv my_env;
        getRodsEnv( &my_env );
        _output += "[ shutting down ";
        _output += my_env.rodsHost;
        _output += " ]";
        _output += "\n";

        server_state& s = server_state::instance();
        s( server_state::STOPPED );
        return SUCCESS();

    } // rule_engine_server_operation_shutdown

    static error operation_pause(
        std::string& _output ) {
        rodsEnv my_env;
        getRodsEnv( &my_env );
        _output += "[ pausing ";
        _output += my_env.rodsHost;
        _output += " ]";
        _output += "\n";

        server_state& s = server_state::instance();
        s( server_state::PAUSED );

        return SUCCESS();

    } // operation_pause

    static error operation_resume(
        std::string& _output ) {
        rodsEnv my_env;
        getRodsEnv( &my_env );
        _output += "[ resuming ";
        _output += my_env.rodsHost;
        _output += " ]";
        _output += "\n";

        server_state& s = server_state::instance();
        s( server_state::RUNNING );
        return SUCCESS();

    } // operation_resume

    static error operation_status(
        std::string& _output ) {
        rodsEnv my_env;
        getRodsEnv( &my_env );

        _output += "[ status for ";
        _output += my_env.rodsHost;
        _output += " ]";
        _output += "\n";

        return SUCCESS();

    } // operation_status

    server_control_plane::server_control_plane(
        const std::string& _prop ) :
        control_executor_( _prop ),
        control_thread_( boost::ref( control_executor_ ) ) {


    } // ctor

    server_control_plane::~server_control_plane() {
        control_thread_.join();

    } // dtor

    server_control_executor::server_control_executor(
        const std::string& _prop ) : port_prop_( _prop )  {
        if ( port_prop_.empty() ) {
            log( ERROR(
                     SYS_INVALID_INPUT_PARAM,
                     "control_plane_port key is empty" ) );
            // TODO :: throw fancy exception
            return;
        }

        op_map_[ SERVER_CONTROL_PAUSE ]    = operation_pause;
        op_map_[ SERVER_CONTROL_RESUME ]   = operation_resume;
        op_map_[ SERVER_CONTROL_STATUS ]   = operation_status;
        if ( _prop == CFG_RULE_ENGINE_CONTROL_PLANE_PORT ) {
            op_map_[ SERVER_CONTROL_SHUTDOWN ] = rule_engine_operation_shutdown;
        }
        else {
            op_map_[ SERVER_CONTROL_SHUTDOWN ] = server_operation_shutdown;

        }

        // get our hostname for ordering
        rodsEnv my_env;
        getRodsEnv( &my_env );
        my_host_name_ = my_env.rodsHost;

        // get the IES host for ordereing
        error ret = get_server_property <
                    std::string > (
                        CFG_ICAT_HOST_KW,
                        icat_host_name_ );
        if ( !ret.ok() ) {
            log( PASS( ret ) );

        }

        // repave icat_host_name_ as we do not want to process 'localhost'
        if ( "localhost" == icat_host_name_ ) {
            icat_host_name_ = my_host_name_;
            rodsLog(
                LOG_ERROR,
                "server_control_executor - icat_host_name is localhost, not a fqdn" );
            // TODO :: throw fancy exception here
        }

    } // ctor

    error server_control_executor::forward_command(
        const std::string& _name,
        const std::string& _host,
        const std::string& _port_keyword,
        std::string&       _output ) {
        // if this is forwarded to us, just perform the operation
        if ( _host == my_host_name_ ) {
            host_list_t hosts;
            hosts.push_back( _host );
            return process_host_list(
                       _name,
                       hosts,
                       _output );

        }
        else {
            return forward_server_control_command(
                       _name,
                       _host,
                       _port_keyword,
                       _output );

        }

    } // forward_command

    error server_control_executor::get_resource_host_names(
        host_list_t& _host_names ) {
        rodsEnv my_env;
        getRodsEnv( &my_env );
        rcComm_t* comm = rcConnect(
                             my_env.rodsHost,
                             my_env.rodsPort,
                             my_env.rodsUserName,
                             my_env.rodsZone,
                             RECONN_TIMEOUT, 0 );
        if ( !comm ) {
            return ERROR(
                       LOG_ERROR,
                       "rcConnect failed" );
        }

        int status = clientLogin(
                         comm,
                         0,
                         my_env.rodsAuthScheme );
        if ( status != 0 ) {
            rcDisconnect( comm );
            return ERROR(
                       status,
                       "client login failed" );
        }

        genQueryInp_t  gen_inp;
        genQueryOut_t* gen_out = NULL;
        memset( &gen_inp, 0, sizeof( gen_inp ) );

        addInxIval( &gen_inp.selectInp, COL_R_LOC, 1 );
        gen_inp.maxRows = MAX_SQL_ROWS;

        int cont_idx = 1;
        while ( cont_idx ) {
            int status = rcGenQuery(
                             comm,
                             &gen_inp,
                             &gen_out );
            if ( status < 0 ) {
                rcDisconnect( comm );
                freeGenQueryOut( &gen_out );
                clearGenQueryInp( &gen_inp );
                return ERROR(
                           status,
                           "genQuery failed." );

            } // if

            sqlResult_t* resc_loc = getSqlResultByInx(
                                        gen_out,
                                        COL_R_LOC );
            if ( !resc_loc ) {
                rcDisconnect( comm );
                freeGenQueryOut( &gen_out );
                clearGenQueryInp( &gen_inp );
                return ERROR(
                           UNMATCHED_KEY_OR_INDEX,
                           "getSqlResultByInx for COL_R_LOC failed" );
            }

            for ( int i = 0;
                    i < gen_out->rowCnt;
                    ++i ) {
                const std::string hn( &resc_loc->value[ resc_loc->len * i ] );
                if ( "localhost"    != hn ) {
                    _host_names.push_back( hn );

                }

            } // for i

            cont_idx = gen_out->continueInx;

        } // while

        rcDisconnect( comm );
        freeGenQueryOut( &gen_out );
        clearGenQueryInp( &gen_inp );

        return SUCCESS();

    } // get_resource_host_names

    void server_control_executor::operator()() {

        int port = 0, num_hash_rounds = 0;
        buffer_crypt::array_t shared_secret; 
        std::string encryption_algorithm;
        error ret = get_server_properties( 
                        port_prop_,
                        port, 
                        num_hash_rounds,
                        shared_secret,
                        encryption_algorithm );
        if( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return;

        }

        zmq::context_t zmq_ctx( 1 );
        zmq::socket_t  zmq_skt( zmq_ctx, ZMQ_REP );

        int time_out = SERVER_CONTROL_POLLING_TIME;
        zmq_skt.setsockopt( ZMQ_RCVTIMEO, &time_out, sizeof( time_out ) );
        zmq_skt.setsockopt( ZMQ_SNDTIMEO, &time_out, sizeof( time_out ) );

        std::stringstream port_sstr;
        port_sstr << port;
        std::string conn_str( "tcp://*:" );
        conn_str += port_sstr.str();
        zmq_skt.bind( conn_str.c_str() );

        rodsLog(
            LOG_NOTICE,
            ">>> control plane :: listening on port %d\n",
            port );

        server_state& s = server_state::instance();
        while ( server_state::STOPPED != s() ) {
            zmq::message_t req;
            zmq_skt.recv( &req );
            if ( 0 == req.size() ) {
                continue;

            }

            // process the message
            std::string output;
            std::string rep_msg( SERVER_CONTROL_SUCCESS );
            error ret = process_operation( req, output );
            if ( !ret.ok() ) {
                log( PASS( ret ) );
                rep_msg = ret.result();

            }
            else if ( !output.empty() ) {
                rep_msg = output;

            }

            buffer_crypt crypt(
                    shared_secret.size(), // key size
                    0,                    // salt size ( we dont send a salt )
                    num_hash_rounds,      // num hash rounds
                    encryption_algorithm.c_str());

            buffer_crypt::array_t iv;
            buffer_crypt::array_t data_to_send;
            buffer_crypt::array_t data_to_encrypt( 
                                      rep_msg.begin(),
                                      rep_msg.end() );
            ret = crypt.encrypt(
                      shared_secret,
                      iv,
                      data_to_encrypt,
                      data_to_send );
            if( !ret.ok() ) {
                irods::log( PASS( ret ) );

            }

            zmq::message_t rep( data_to_send.size() );
            memcpy(
                rep.data(),
                data_to_send.data(),
                data_to_send.size() );

            zmq_skt.send( rep );

        } // while

    } // control operation

    error server_control_executor::notify_icat_and_local_servers_preop(
        const std::string& _cmd_name,
        const std::string& _cmd_option,
        const host_list_t& _cmd_hosts,
        std::string&       _output ) {

        if ( SERVER_CONTROL_RESUME != _cmd_name ) {
            return SUCCESS();

        }

        error ret = SUCCESS();
        const bool found_my_host = ( std::find(
                                         _cmd_hosts.begin(),
                                         _cmd_hosts.end(),
                                         my_host_name_ )
                                     != _cmd_hosts.end() );
        const bool found_icat_host = ( std::find(
                                           _cmd_hosts.begin(),
                                           _cmd_hosts.end(),
                                           icat_host_name_ )
                                       != _cmd_hosts.end() );
        const bool is_all_opt  = ( SERVER_CONTROL_ALL_OPT == _cmd_option );
        const bool is_icat_host = ( my_host_name_ == icat_host_name_ );
        // for pause or shutdown: pre-op forwards to the ies first,
        // then to myself, then others
        // for resume: we skip doing work here (we'll go last in post-op)
        if ( found_icat_host || is_all_opt ) {
            ret = forward_command(
                      _cmd_name,
                      icat_host_name_,
                      CFG_SERVER_CONTROL_PLANE_PORT,
                      _output );
            sleep( SERVER_CONTROL_FWD_SLEEP_TIME );
        }

        // pre-op forwards to the local server second
        // such as for resume
        if ( !is_icat_host && ( found_my_host || is_all_opt ) ) {
            ret = forward_command(
                      _cmd_name,
                      my_host_name_,
                      CFG_SERVER_CONTROL_PLANE_PORT,
                      _output );
        }

        return ret;

    } // notify_icat_and_local_servers_preop

    error server_control_executor::notify_icat_and_local_servers_postop(
        const std::string& _cmd_name,
        const std::string& _cmd_option,
        const host_list_t& _cmd_hosts,
        std::string&       _output ) {
        error ret = SUCCESS();
        if ( SERVER_CONTROL_RESUME == _cmd_name ) {
            return SUCCESS();

        }

        bool found_my_host = ( std::find(
                                   _cmd_hosts.begin(),
                                   _cmd_hosts.end(),
                                   my_host_name_ )
                               != _cmd_hosts.end() );
        bool found_icat_host = ( std::find(
                                     _cmd_hosts.begin(),
                                     _cmd_hosts.end(),
                                     icat_host_name_ )
                                 != _cmd_hosts.end() );
        bool is_all_opt  = ( SERVER_CONTROL_ALL_OPT == _cmd_option );
        bool is_icat_host = ( my_host_name_ == icat_host_name_ );

        // post-op forwards to the local server first
        // then the icat such as for shutdown
        if ( !is_icat_host && ( found_my_host || is_all_opt ) ) {
            ret = forward_command(
                      _cmd_name,
                      my_host_name_,
                      CFG_SERVER_CONTROL_PLANE_PORT,
                      _output );
        }

        // post-op forwards to the ies last
        if ( found_icat_host || is_all_opt ) {
            ret = forward_command(
                      _cmd_name,
                      icat_host_name_,
                      CFG_SERVER_CONTROL_PLANE_PORT,
                      _output );
        }

        return ret;

    } // notify_icat_server_postop

    error server_control_executor::validate_host_list(
        const host_list_t&  _irods_hosts,
        const host_list_t&  _cmd_hosts,
        host_list_t&        _valid_hosts ) {

        host_list_t::const_iterator itr;
        for ( itr  = _cmd_hosts.begin();
                itr != _cmd_hosts.end();
                ++itr ) {
            // check host value against list from the icat
            if ( _irods_hosts.end() == std::find(
                        _irods_hosts.begin(),
                        _irods_hosts.end(),
                        *itr ) ) {
                std::string msg( "invalid server hostname [" );
                msg += *itr;
                msg += "]";
                return ERROR(
                           SYS_INVALID_INPUT_PARAM,
                           msg );

            }

            // skip the IES since it is a special case
            // and handled elsewhere
            if ( icat_host_name_ == *itr ) {
                continue;
            }

            // skip the local server since it is also a
            // special case and handled elsewhere
            if ( my_host_name_ == *itr ) {
                continue;

            }

            // add the host to our newly ordered list
            _valid_hosts.push_back( *itr );

        } // for itr

        return SUCCESS();

    } // validate_host_list

    error server_control_executor::extract_command_parameters(
        const control_plane_command& _cmd,
        std::string&                 _name,
        std::string&                 _option,
        host_list_t&                 _hosts ) {
        // capture and validate the command parameter
        _name = _cmd.command;
        if ( SERVER_CONTROL_SHUTDOWN != _name &&
                SERVER_CONTROL_PAUSE    != _name &&
                SERVER_CONTROL_RESUME   != _name &&
                SERVER_CONTROL_STATUS   != _name ) {
            std::string msg( "invalid command [" );
            msg += _name;
            msg += "]";
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       msg );
        }

        // capture and validate the option parameter
        std::map<std::string, std::string>::const_iterator itr =
            _cmd.options.find( SERVER_CONTROL_OPTION_KW );
        if ( _cmd.options.end() == itr ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "option parameter is empty" );

        }

        _option = itr->second;
        if ( SERVER_CONTROL_ALL_OPT   != _option &&
                SERVER_CONTROL_HOSTS_OPT != _option ) {
            std::string msg( "invalid command option [" );
            msg += _option;
            msg += "]";
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       msg );
        }

        // capture and validate the server hosts, skip the option key
        for ( itr  = _cmd.options.begin();
                itr != _cmd.options.end();
                ++itr ) {
            if ( itr->first == SERVER_CONTROL_OPTION_KW ) {
                continue;

            }
            else if ( itr->first.find(
                          SERVER_CONTROL_HOST_KW )
                      != std::string::npos ) {
                _hosts.push_back( itr->second );

            }
            else {
                std::string msg( "invalid option key [" );
                msg += itr->first;
                msg += "]";
                return ERROR(
                           SYS_INVALID_INPUT_PARAM,
                           msg );

            }

        } // for itr

        return SUCCESS();

    } // extract_command_parameters

    error server_control_executor::process_host_list(
        const std::string& _cmd_name,
        const host_list_t& _hosts,
        std::string&       _output ) {
        if ( _hosts.empty() ) {
            return SUCCESS();

        }

        error fwd_err;
        host_list_t::const_iterator itr;
        for ( itr  = _hosts.begin();
                itr != _hosts.end();
                ++itr ) {
            if ( "localhost" == *itr ) {
                continue;

            }

            std::string output;
            if ( *itr == my_host_name_ ) {
                error ret = op_map_[ _cmd_name ]( _output );
                if ( !ret.ok() ) {
                    fwd_err = PASS( ret );

                }

                continue;
            }

            error ret = forward_command(
                            _cmd_name,
                            *itr,
                            port_prop_,
                            _output );
            if ( !ret.ok() ) {
                log( PASS( ret ) );

            }

        } // for itr

        return SUCCESS();

    } // process_host_list

    error server_control_executor::process_operation(
        const zmq::message_t& _msg,
        std::string&          _output ) {
        if ( _msg.size() <= 0 ) {
            return SUCCESS();

        }

        int port = 0, num_hash_rounds = 0;
        buffer_crypt::array_t shared_secret; 
        std::string encryption_algorithm;
        error ret = get_server_properties( 
                        port_prop_,
                        port, 
                        num_hash_rounds,
                        shared_secret,
                        encryption_algorithm );
        if( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return PASS( ret );

        }

        // decrypt the message before passing to avro
        buffer_crypt crypt(
                shared_secret.size(), // key size
                0,                    // salt size ( we dont send a salt )
                num_hash_rounds,      // num hash rounds
                encryption_algorithm.c_str());

        buffer_crypt::array_t iv;
        buffer_crypt::array_t data_to_process;

        const uint8_t* data_ptr = static_cast< const uint8_t* >( _msg.data() );
        buffer_crypt::array_t data_to_decrypt( 
                                  data_ptr, 
                                  data_ptr + _msg.size() );
        ret = crypt.decrypt(
                  shared_secret,
                  iv,
                  data_to_decrypt,
                  data_to_process );
        if( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return PASS( ret );

        }


        std::auto_ptr<avro::InputStream> in = avro::memoryInputStream(
                static_cast<const uint8_t*>(
                    data_to_process.data() ),
                    data_to_process.size() );
        avro::DecoderPtr dec = avro::binaryDecoder();
        dec->init( *in );

        control_plane_command cmd;
        avro::decode( *dec, cmd );

        std::string cmd_name, cmd_option;
        host_list_t cmd_hosts;
        ret = extract_command_parameters(
                  cmd,
                  cmd_name,
                  cmd_option,
                  cmd_hosts );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return PASS( ret );

        }

        // the icat needs to be notified first in certain
        // cases such as RESUME where it is needed to capture
        // the host list for validation, etc
        ret = notify_icat_and_local_servers_preop(
                  cmd_name,
                  cmd_option,
                  cmd_hosts,
                  _output );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return PASS( ret );

        }

        host_list_t irods_hosts;
        ret = get_resource_host_names(
                  irods_hosts );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return PASS( ret );

        }

        if ( SERVER_CONTROL_ALL_OPT == cmd_option ) {
            cmd_hosts = irods_hosts;

        }

        host_list_t valid_hosts;
        ret = validate_host_list(
                  irods_hosts,
                  cmd_hosts,
                  valid_hosts );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return PASS( ret );

        }

        ret = process_host_list(
                  cmd_name,
                  valid_hosts,
                  _output );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return PASS( ret );

        }

        // the icat needs to be notified last in certain
        // cases such as SHUTDOWN or PAUSE  where it is
        // needed to capture the host list for validation
        ret = notify_icat_and_local_servers_postop(
                  cmd_name,
                  cmd_option,
                  cmd_hosts, // dont want sanitized
                  _output );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return PASS( ret );

        }

        return ret;

    } // process_operation

}; // namespace irods
































