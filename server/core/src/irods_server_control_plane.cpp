#include "avro/Encoder.hh"
#include "avro/Decoder.hh"
#include "avro/Specific.hh"

#include "stdio.h"

//#include <readproc.h>
//#include <sysinfo.h>

#include "genQuery.h"
#include "rcMisc.h"
#include "sockComm.h"
#include "miscServerFunct.hpp"
#include "rodsServer.hpp"

#include "irods_log.hpp"
#include "irods_server_control_plane.hpp"
#include "irods_server_properties.hpp"
#include "irods_buffer_encryption.hpp"
#include "irods_resource_manager.hpp"
#include "irods_server_state.hpp"
#include "irods_exception.hpp"
#include "irods_stacktrace.hpp"

#include "boost/lexical_cast.hpp"

#include "jansson.h"

#include <ctime>
#include <unistd.h>

namespace irods {
    static void ctrl_plane_sleep(
        int _s,
        int _ms ) {
        useconds_t us = ( _s * 1000000 ) + ( _ms * 1000 );
        usleep( us );
    }

    static error forward_server_control_command(
        const std::string& _name,
        const std::string& _host,
        const std::string& _port_keyword,
        std::string&       _output ) {
        if ( EMPTY_RESC_HOST == _host ) {
            return SUCCESS();

        }

        int time_out, port, num_hash_rounds;
        boost::optional<const std::string&> encryption_algorithm;
        buffer_crypt::array_t shared_secret;
        try {
            time_out = get_server_property<const int>(CFG_SERVER_CONTROL_PLANE_TIMEOUT);
            port = get_server_property<const int>(_port_keyword);
            num_hash_rounds = get_server_property<const int>(CFG_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW);
            encryption_algorithm.reset(get_server_property<const std::string>(CFG_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM_KW));
            const auto& key = get_server_property<const std::string>(CFG_SERVER_CONTROL_PLANE_KEY);
            shared_secret.assign(key.begin(), key.end());
        } catch ( const irods::exception& e ) {
            return irods::error(e);
        }

        // stringify the port
        std::stringstream port_sstr;
        port_sstr << port;
        // standard zmq rep-req communication pattern
        zmq::context_t zmq_ctx( 1 );
        zmq::socket_t  zmq_skt( zmq_ctx, ZMQ_REQ );
        zmq_skt.setsockopt( ZMQ_RCVTIMEO, &time_out, sizeof( time_out ) );
        zmq_skt.setsockopt( ZMQ_SNDTIMEO, &time_out, sizeof( time_out ) );
        zmq_skt.setsockopt( ZMQ_LINGER, 0 );

        // this is the client so we connect rather than bind
        std::string conn_str( "tcp://" );
        conn_str += _host;
        conn_str += ":";
        conn_str += port_sstr.str();

        try {
            zmq_skt.connect( conn_str.c_str() );
        }
        catch ( zmq::error_t& e_ ) {
            _output += "{\n    \"failed_to_connect\" : \"" + conn_str + "\"\n},\n";
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       _output );
        }


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
            encryption_algorithm->c_str() );

        buffer_crypt::array_t iv;
        buffer_crypt::array_t data_to_send;
        buffer_crypt::array_t data_to_encrypt(
            data->data(),
            data->data() + data->size() );
        irods::error ret = crypt.encrypt(
                  shared_secret,
                  iv,
                  data_to_encrypt,
                  data_to_send );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

        // copy binary encoding into a zmq message for transport
        zmq::message_t rep( data_to_send.size() );
        memcpy(
            rep.data(),
            data_to_send.data(),
            data_to_send.size() );
        zmq_skt.send( rep );

        // wait for the server response
        zmq::message_t req;
        zmq_skt.recv( &req );

        if ( 0 == req.size() ) {
            _output += "{\n    \"response_message_is_empty_from\" : \"" + conn_str + "\"\n},\n";
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "empty response string" );

        }

        // decrypt the message before passing to avro
        buffer_crypt::array_t data_to_process;
        const uint8_t* data_ptr = static_cast< const uint8_t* >( req.data() );
        buffer_crypt::array_t data_to_decrypt(
            data_ptr,
            data_ptr + req.size() );
        ret = crypt.decrypt(
                  shared_secret,
                  iv,
                  data_to_decrypt,
                  data_to_process );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            _output += "{\n    \"failed_to_decrpyt_message_from\" : \"" + conn_str + "\"\n},\n";
            rodsLog( LOG_ERROR, "Failed to decrpyt [%s]", req.data() );
            return PASS( ret );

        }

        std::string rep_str(
            reinterpret_cast< char* >( data_to_process.data() ),
            data_to_process.size() );
        if ( SERVER_CONTROL_SUCCESS != rep_str ) {
            // check if the result is really an error or a status
            if ( std::string::npos == rep_str.find( "[-]" ) ) {
                _output += rep_str;

            }
            else {
                _output += "{\n    \"invalid_message_format_from\" : \"" + conn_str + "\"\n},\n";
                return ERROR(
                           CONTROL_PLANE_MESSAGE_ERROR,
                           rep_str );

            }
        }

        return SUCCESS();

    } // forward_server_control_command

    static error kill_server(
        const std::string& _pid_prop ) {
        int svr_pid;
        // no error case, resource servers have no re server
        try {
            svr_pid = get_server_property<const int>(_pid_prop);
        } catch ( const irods::exception& e ) {
            if ( e.code() == KEY_NOT_FOUND ) {
                // if the property does not exist then the server
                // in question is not running
                return SUCCESS();
            }
            return irods::error(e);
        }

        std::stringstream pid_str; pid_str << svr_pid;
        std::vector<std::string> args;
        args.push_back( pid_str.str() );

        std::string output;
        irods::error ret = get_script_output_single_line(
                  "python",
                  "kill_pid.py",
                  args,
                  output );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        return SUCCESS();

    } // kill_server

    static error server_operation_shutdown(
        const std::string& _wait_option,
        const size_t       _wait_seconds,
        std::string&       _output ) {
        rodsEnv my_env;
        _reloadRodsEnv( my_env );
        _output += "{\n    \"shutting down\": \"";
        _output += my_env.rodsHost;
        _output += "\"\n},\n";

        error ret;
        int sleep_time_out_milli_sec = 0;
        try {
            sleep_time_out_milli_sec = get_server_property<const int>(CFG_SERVER_CONTROL_PLANE_TIMEOUT);
        } catch ( const irods::exception& e ) {
            return irods::error(e);
        }

        if ( SERVER_CONTROL_FORCE_AFTER_KW == _wait_option ) {
            // convert sec to millisec for comparison
            sleep_time_out_milli_sec = _wait_seconds * 1000;
        }

        int wait_milliseconds = SERVER_CONTROL_POLLING_TIME_MILLI_SEC;

        server_state& svr_state = server_state::instance();
        svr_state( server_state::PAUSED );

        int  sleep_time  = 0;
        bool timeout_flg = false;
        int  proc_cnt = getAgentProcCnt();

        while ( proc_cnt > 0 && !timeout_flg ) {
            // takes sec, millisec
            ctrl_plane_sleep(
                0,
                wait_milliseconds );

            if ( SERVER_CONTROL_WAIT_FOREVER_KW != _wait_option ) {
                sleep_time += SERVER_CONTROL_POLLING_TIME_MILLI_SEC;
                if ( sleep_time > sleep_time_out_milli_sec ) {
                    timeout_flg = true;
                }
            }

            proc_cnt = getAgentProcCnt();

        } // while

        // kill the rule engine server
        ret = kill_server( irods::RE_PID_KW );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
        }

        // kill the xmessage server
        ret = kill_server( irods::XMSG_PID_KW );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
        }

        // actually shut down the server
        svr_state( server_state::STOPPED );

        // block until server exits to return
        while( !timeout_flg ) {
            // takes sec, millisec
            ctrl_plane_sleep(
                0,
                wait_milliseconds );

            sleep_time += SERVER_CONTROL_POLLING_TIME_MILLI_SEC;
            if ( sleep_time > sleep_time_out_milli_sec ) {
                timeout_flg = true;
            }

            std::string the_server_state = svr_state();
            if ( irods::server_state::EXITED == the_server_state ) {
                break;
            }

        } // while

        return SUCCESS();

    } // server_operation_shutdown

    static error rule_engine_operation_shutdown(
        const std::string&, // _wait_option,
        const size_t, //       _wait_seconds,
        std::string& _output ) {
        rodsEnv my_env;
        _reloadRodsEnv( my_env );
        _output += "{\n    \"shutting down\": \"";
        _output += my_env.rodsHost;
        _output += "\"\n},\n";

        server_state& s = server_state::instance();
        s( server_state::STOPPED );
        return SUCCESS();

    } // rule_engine_server_operation_shutdown

    static error operation_pause(
        const std::string&, // _wait_option,
        const size_t, //       _wait_seconds,
        std::string& _output ) {
        rodsEnv my_env;
        _reloadRodsEnv( my_env );
        _output += "{\n    \"pausing\": \"";
        _output += my_env.rodsHost;
        _output += "\"\n},\n";
        server_state& s = server_state::instance();
        s( server_state::PAUSED );

        return SUCCESS();

    } // operation_pause

    static error operation_resume(
        const std::string&, // _wait_option,
        const size_t, //       _wait_seconds,
        std::string& _output ) {
        rodsEnv my_env;
        _reloadRodsEnv( my_env );
        _output += "{\n    \"resuming\": \"";
        _output += my_env.rodsHost;
        _output += "\"\n},\n";

        server_state& s = server_state::instance();
        s( server_state::RUNNING );
        return SUCCESS();

    } // operation_resume

    static int get_pid_age(
        pid_t _pid ) {
        std::stringstream pid_str; pid_str << _pid;
        std::vector<std::string> args;
        args.push_back( pid_str.str() );

        std::string pid_age;
        irods::error ret = get_script_output_single_line(
                               "python",
                               "pid_age.py",
                               args,
                               pid_age );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return 0;
        }

        double age = 0.0;
        try {
            age = boost::lexical_cast<double>( pid_age );
        }
        catch ( const boost::bad_lexical_cast& ) {
            rodsLog(
                LOG_ERROR,
                "get_pid_age bad lexical cast for [%s]",
                pid_age.c_str() );

        }

        return static_cast<int>( age );

    } // get_pid_age

    static error operation_status(
        const std::string&, // _wait_option,
        const size_t, //       _wait_seconds,
        std::string& _output ) {
        rodsEnv my_env;
        _reloadRodsEnv( my_env );

        int re_pid = 0;
        // no error case, resource servers have no re server
        try {
            re_pid = get_server_property<const int>(irods::RE_PID_KW);
        } catch ( const irods::exception& ) {}

        int xmsg_pid = 0;
        try {
            xmsg_pid = get_server_property<const int>(irods::XMSG_PID_KW);
        } catch ( const irods::exception& ) {}

        int my_pid = getpid();

        json_t* obj = json_object();
        if ( !obj ) {
            return ERROR(
                       SYS_MALLOC_ERR,
                       "allocation of json object failed" );
        }

        json_object_set_new( obj, "hostname", json_string( my_env.rodsHost ) );
        json_object_set_new( obj, "irods_server_pid", json_integer( my_pid ) );
        json_object_set_new( obj, "re_server_pid", json_integer( re_pid ) );
        json_object_set_new( obj, "xmsg_server_pid", json_integer( xmsg_pid ) );

        server_state& s = server_state::instance();
        json_object_set_new( obj, "status", json_string( s().c_str() ) );

        json_t* arr = json_array();
        if ( !arr ) {
            return ERROR(
                       SYS_MALLOC_ERR,
                       "allocation of json array failed" );
        }

        std::vector<int> pids;
        getAgentProcPIDs( pids );
        for ( size_t i = 0;
                i < pids.size();
                ++i ) {
            int  pid = pids[i];
            int  age = get_pid_age( pids[i] );

            json_t* agent_obj = json_object();
            if ( !agent_obj ) {
                return ERROR(
                           SYS_MALLOC_ERR,
                           "allocation of json object failed" );
            }

            json_object_set_new( agent_obj, "agent_pid", json_integer( pid ) );
            json_object_set_new( agent_obj, "age", json_integer( age ) );
            json_array_append_new( arr, agent_obj );
        }

        json_object_set_new( obj, "agents", arr );

        char* tmp_buf = json_dumps( obj, JSON_INDENT( 4 ) );
        json_decref( obj );

        _output += tmp_buf;
        free(tmp_buf);
        _output += ",";

        return SUCCESS();

    } // operation_status

    static error operation_ping(
        const std::string&, // _wait_option,
        const size_t, //       _wait_seconds,
        std::string& _output ) {

        _output += "{\n    \"status\": \"alive\"\n},\n";
        /*rodsEnv my_env;
        _reloadRodsEnv( my_env );
        _output += "{\n    \"pinging\": \"";
        _output += my_env.rodsHost;
        _output += "\"\n},\n";*/
        return SUCCESS();
    }

    bool server_control_executor::compare_host_names(
        const std::string& _hn1,
        const std::string& _hn2 ) {

        bool we_are_the_host = ( _hn1 == _hn2 );
        if ( !we_are_the_host ) {
            bool host_has_dots = ( std::string::npos != _hn1.find( "." ) );
            bool my_host_has_dots = ( std::string::npos != _hn2.find( "." ) );

            if ( host_has_dots && !my_host_has_dots ) {
                we_are_the_host = ( std::string::npos != _hn1.find( _hn2 ) );

            }
            else if ( !host_has_dots && my_host_has_dots ) {
                we_are_the_host = ( std::string::npos != _hn2.find( _hn1 ) );

            }

        }

        return we_are_the_host;

    } // compare_host_names

    bool server_control_executor::is_host_in_list(
        const std::string& _hn,
        const host_list_t& _hosts ) {
        for ( size_t i = 0;
                i < _hosts.size();
                ++i ) {
            if ( compare_host_names(
                        _hn,
                        _hosts[ i ] ) ) {
                return true;
            }

        } // for i

        return false;

    } // is_host_in_list

    server_control_plane::server_control_plane(
        const std::string& _prop ) :
        control_executor_( _prop ),
        control_thread_( boost::ref( control_executor_ ) ) {


    } // ctor

    server_control_plane::~server_control_plane() {
        try {
            control_thread_.join();
        }
        catch ( const boost::thread_resource_error& ) {
            rodsLog(
                LOG_ERROR,
                "boost encountered thread_resource_error on join in server_control_plane destructor." );
        }

    } // dtor

    server_control_executor::server_control_executor(
        const std::string& _prop ) : port_prop_( _prop )  {
        if ( port_prop_.empty() ) {
            THROW(
                SYS_INVALID_INPUT_PARAM,
                "control_plane_port key is empty" );
        }

        op_map_[ SERVER_CONTROL_PAUSE ]    = operation_pause;
        op_map_[ SERVER_CONTROL_RESUME ]   = operation_resume;
        op_map_[ SERVER_CONTROL_STATUS ]   = operation_status;
        op_map_[ SERVER_CONTROL_PING ]     = operation_ping;
        if ( _prop == CFG_RULE_ENGINE_CONTROL_PLANE_PORT ) {
            op_map_[ SERVER_CONTROL_SHUTDOWN ] = rule_engine_operation_shutdown;
        }
        else {
            op_map_[ SERVER_CONTROL_SHUTDOWN ] = server_operation_shutdown;

        }

        // get our hostname for ordering
        rodsEnv my_env;
        _reloadRodsEnv( my_env );
        my_host_name_ = my_env.rodsHost;

        // get the IES host for ordereing
        icat_host_name_ = boost::any_cast<const std::string&>(get_server_property<const std::vector<boost::any>>(CFG_CATALOG_PROVIDER_HOSTS_KW)[0]);

        // repave icat_host_name_ as we do not want to process 'localhost'
        if ( "localhost" == icat_host_name_ ) {
            icat_host_name_ = my_host_name_;
            rodsLog(
                LOG_ERROR,
                "server_control_executor - icat_host_name is localhost, not a fqdn" );
            // TODO :: throw fancy exception here when we disallow localhost
        }

    } // ctor

    error server_control_executor::forward_command(
        const std::string& _name,
        const std::string& _host,
        const std::string& _port_keyword,
        const std::string& _wait_option,
        const size_t&      _wait_seconds,
        std::string&       _output ) {
        bool we_are_the_host = compare_host_names(
                                   _host,
                                   my_host_name_ );

        irods::error ret = SUCCESS();
        // if this is forwarded to us, just perform the operation
        if ( we_are_the_host ) {
            host_list_t hosts;
            hosts.push_back( _host );
            ret = process_host_list(
                      _name,
                      _wait_option,
                      _wait_seconds,
                      hosts,
                      _output );

        }
        else {
            ret = forward_server_control_command(
                      _name,
                      _host,
                      _port_keyword,
                      _output );

        }

        return ret;

    } // forward_command

    error server_control_executor::get_resource_host_names(
        host_list_t& _host_names ) {
        rodsEnv my_env;
        _reloadRodsEnv( my_env );
        rcComm_t* comm = rcConnect(
                             my_env.rodsHost,
                             my_env.rodsPort,
                             my_env.rodsUserName,
                             my_env.rodsZone,
                             NO_RECONN, 0 );
        if ( !comm ) {
            return ERROR(
                       NULL_VALUE_ERR,
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

        freeGenQueryOut( &gen_out );
        clearGenQueryInp( &gen_inp );

        status = rcDisconnect( comm );
        if ( status < 0 ) {
            return ERROR(
                       status,
                       "failed in rcDisconnect" );
        }

        return SUCCESS();

    } // get_resource_host_names

    void server_control_executor::operator()() {

        int port, num_hash_rounds;
        boost::optional<const std::string&> encryption_algorithm;
        buffer_crypt::array_t shared_secret;
        try {
            port = get_server_property<const int>(port_prop_);
            num_hash_rounds = get_server_property<const int>(CFG_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW);
            encryption_algorithm.reset(get_server_property<const std::string>(CFG_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM_KW));
            const auto& key = get_server_property<const std::string>(CFG_SERVER_CONTROL_PLANE_KEY);
            shared_secret.assign(key.begin(), key.end());
        } catch ( const irods::exception& e ) {
            irods::log(e);
            return;
        }

        if ( shared_secret.empty() ||
                encryption_algorithm->empty() ||
                0 == port ||
                0 == num_hash_rounds ) {
            rodsLog(
                LOG_NOTICE,
                "control plane is not configured properly" );
            return;
        }

        zmq::context_t zmq_ctx( 1 );
        zmq::socket_t  zmq_skt( zmq_ctx, ZMQ_REP );

        int time_out = SERVER_CONTROL_POLLING_TIME_MILLI_SEC;
        zmq_skt.setsockopt( ZMQ_RCVTIMEO, &time_out, sizeof( time_out ) );
        zmq_skt.setsockopt( ZMQ_SNDTIMEO, &time_out, sizeof( time_out ) );
        zmq_skt.setsockopt( ZMQ_LINGER, 0 );

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
        while ( server_state::STOPPED != s() &&
                server_state::EXITED != s() ) {

            zmq::message_t req;
            zmq_skt.recv( &req );
            if ( 0 == req.size() ) {
                continue;

            }

            // process the message
            std::string output;
            std::string rep_msg( SERVER_CONTROL_SUCCESS );
            error ret = process_operation( req, output );

            rep_msg = output;
            if ( !ret.ok() ) {
                log( PASS( ret ) );
            }

            if ( !output.empty() ) {
                rep_msg = output;

            }

            buffer_crypt crypt(
                shared_secret.size(), // key size
                0,                    // salt size ( we dont send a salt )
                num_hash_rounds,      // num hash rounds
                encryption_algorithm->c_str() );

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
            if ( !ret.ok() ) {
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
        const std::string& _wait_option,
        const size_t&      _wait_seconds,
        const host_list_t& _cmd_hosts,
        std::string&       _output ) {

        if ( SERVER_CONTROL_RESUME != _cmd_name ) {
            return SUCCESS();

        }

        error ret = SUCCESS();
        const bool is_all_opt  = ( SERVER_CONTROL_ALL_OPT == _cmd_option );
        const bool found_my_host = is_host_in_list(
                                       my_host_name_,
                                       _cmd_hosts );
        const bool found_icat_host = is_host_in_list(
                                         icat_host_name_,
                                         _cmd_hosts );
        const bool is_icat_host = compare_host_names( my_host_name_, icat_host_name_ );
        // for pause or shutdown: pre-op forwards to the ies first,
        // then to myself, then others
        // for resume: we skip doing work here (we'll go last in post-op)
        if ( found_icat_host || is_all_opt ) {
            ret = forward_command(
                      _cmd_name,
                      icat_host_name_,
                      CFG_SERVER_CONTROL_PLANE_PORT,
                      _wait_option,
                      _wait_seconds,
                      _output );
            // takes sec, microsec
            ctrl_plane_sleep(
                0, SERVER_CONTROL_FWD_SLEEP_TIME_MILLI_SEC );
        }

        // pre-op forwards to the local server second
        // such as for resume
        if ( !is_icat_host && ( found_my_host || is_all_opt ) ) {
            ret = forward_command(
                      _cmd_name,
                      my_host_name_,
                      CFG_SERVER_CONTROL_PLANE_PORT,
                      _wait_option,
                      _wait_seconds,
                      _output );
        }

        return ret;

    } // notify_icat_and_local_servers_preop

    error server_control_executor::notify_icat_and_local_servers_postop(
        const std::string& _cmd_name,
        const std::string& _cmd_option,
        const std::string& _wait_option,
        const size_t&      _wait_seconds,
        const host_list_t& _cmd_hosts,
        std::string&       _output ) {
        error ret = SUCCESS();
        if ( SERVER_CONTROL_RESUME == _cmd_name ) {
            return SUCCESS();

        }

        bool is_all_opt  = ( SERVER_CONTROL_ALL_OPT == _cmd_option );
        const bool found_my_host = is_host_in_list(
                                       my_host_name_,
                                       _cmd_hosts );
        const bool found_icat_host = is_host_in_list(
                                         icat_host_name_,
                                         _cmd_hosts );
        const bool is_icat_host = compare_host_names( my_host_name_, icat_host_name_ );

        // post-op forwards to the local server first
        // then the icat such as for shutdown
        if ( !is_icat_host && ( found_my_host || is_all_opt ) ) {
            ret = forward_command(
                      _cmd_name,
                      my_host_name_,
                      CFG_SERVER_CONTROL_PLANE_PORT,
                      _wait_option,
                      _wait_seconds,
                      _output );
        }

        // post-op forwards to the ies last
        if ( found_icat_host || is_all_opt ) {
            ret = forward_command(
                      _cmd_name,
                      icat_host_name_,
                      CFG_SERVER_CONTROL_PLANE_PORT,
                      _wait_option,
                      _wait_seconds,
                      _output );
        }

        return ret;

    } // notify_icat_and_local_servers_postop

    error server_control_executor::validate_host_list(
        const host_list_t&  _irods_hosts,
        const host_list_t&  _cmd_hosts,
        host_list_t&        _valid_hosts ) {

        host_list_t::const_iterator itr;
        for ( itr  = _cmd_hosts.begin();
                itr != _cmd_hosts.end();
                ++itr ) {
            // check host value against list from the icat
            if ( !is_host_in_list(
                        *itr,
                        _irods_hosts ) &&
                 *itr != icat_host_name_ ) {
                std::string msg( "invalid server hostname [" );
                msg += *itr;
                msg += "]";
                return ERROR(
                           SYS_INVALID_INPUT_PARAM,
                           msg );

            }

            // skip the IES since it is a special case
            // and handled elsewhere
            if ( compare_host_names( icat_host_name_, *itr ) ) {
                continue;
            }

            // skip the local server since it is also a
            // special case and handled elsewhere
            if ( compare_host_names( my_host_name_, *itr ) ) {
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
        std::string&                 _wait_option,
        size_t&                      _wait_seconds,
        host_list_t&                 _hosts ) {
        // capture and validate the command parameter
        _name = _cmd.command;
        if ( op_map_.count(_name) == 0 ) {
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
            else if ( itr->first == SERVER_CONTROL_FORCE_AFTER_KW ) {
                _wait_option = SERVER_CONTROL_FORCE_AFTER_KW;
                _wait_seconds = boost::lexical_cast< size_t >( itr->second );

            }
            else if ( itr->first == SERVER_CONTROL_WAIT_FOREVER_KW ) {
                _wait_option = SERVER_CONTROL_WAIT_FOREVER_KW;
                _wait_seconds = 0;

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
        const std::string& _wait_option,
        const size_t&      _wait_seconds,
        const host_list_t& _hosts,
        std::string&       _output ) {
        if ( _hosts.empty() ) {
            return SUCCESS();

        }

        error fwd_err = SUCCESS();
        host_list_t::const_iterator itr;
        for ( itr  = _hosts.begin();
                itr != _hosts.end();
                ++itr ) {
            if ( "localhost" == *itr ) {
                continue;

            }

            std::string output;
            if ( compare_host_names(
                        *itr,
                        my_host_name_ ) ) {
                error ret = op_map_[ _cmd_name ](
                                _wait_option,
                                _wait_seconds,
                                output );
                if ( !ret.ok() ) {
                    fwd_err = PASS( ret );

                }

                _output += output;

            }
            else {
                error ret = forward_command(
                                _cmd_name,
                                *itr,
                                port_prop_,
                                _wait_option,
                                _wait_seconds,
                                output );
                if ( !ret.ok() ) {
                    _output += output;
                    log( PASS( ret ) );
                    fwd_err = PASS( ret );

                }
                else {
                    _output += output;

                }

            }

        } // for itr

        return fwd_err;

    } // process_host_list

    error server_control_executor::process_operation(
        const zmq::message_t& _msg,
        std::string&          _output ) {
        if ( _msg.size() <= 0 ) {
            return SUCCESS();

        }

        error final_ret = SUCCESS();

        int port, num_hash_rounds;
        boost::optional<const std::string&> encryption_algorithm;
        buffer_crypt::array_t shared_secret;
        try {
            port = get_server_property<const int>(port_prop_);
            num_hash_rounds = get_server_property<const int>(CFG_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW);
            encryption_algorithm.reset(get_server_property<const std::string>(CFG_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM_KW));
            const auto& key = get_server_property<const std::string>(CFG_SERVER_CONTROL_PLANE_KEY);
            shared_secret.assign(key.begin(), key.end());
        } catch ( const irods::exception& e ) {
            return irods::error(e);
        }

        // decrypt the message before passing to avro
        buffer_crypt crypt(
            shared_secret.size(), // key size
            0,                    // salt size ( we dont send a salt )
            num_hash_rounds,      // num hash rounds
            encryption_algorithm->c_str() );

        buffer_crypt::array_t iv;
        buffer_crypt::array_t data_to_process;

        const uint8_t* data_ptr = static_cast< const uint8_t* >( _msg.data() );
        buffer_crypt::array_t data_to_decrypt(
            data_ptr,
            data_ptr + _msg.size() );
        irods::error ret = crypt.decrypt(
                  shared_secret,
                  iv,
                  data_to_decrypt,
                  data_to_process );
        if ( !ret.ok() ) {
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

        std::string cmd_name, cmd_option, wait_option;
        host_list_t cmd_hosts;
        size_t wait_seconds = 0;
        ret = extract_command_parameters(
                  cmd,
                  cmd_name,
                  cmd_option,
                  wait_option,
                  wait_seconds,
                  cmd_hosts );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return PASS( ret );

        }

        // add safeguards - if server is paused only allow a resume call
        server_state& s = server_state::instance();
        std::string the_server_state = s();
        if ( server_state::PAUSED == the_server_state &&
                SERVER_CONTROL_RESUME != cmd_name ) {
            _output = SERVER_PAUSED_ERROR;
            return SUCCESS();
        }

        // the icat needs to be notified first in certain
        // cases such as RESUME where it is needed to capture
        // the host list for validation, etc
        ret = notify_icat_and_local_servers_preop(
                  cmd_name,
                  cmd_option,
                  wait_option,
                  wait_seconds,
                  cmd_hosts,
                  _output );
        if ( !ret.ok() ) {
            final_ret = PASS( ret );
            irods::log( final_ret );
        }

        host_list_t irods_hosts;
        ret = get_resource_host_names(
                  irods_hosts );
        if ( !ret.ok() ) {
            final_ret = PASS( ret );
            irods::log( final_ret );
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
            final_ret = PASS( ret );
            irods::log( final_ret );

        }

        ret = process_host_list(
                  cmd_name,
                  wait_option,
                  wait_seconds,
                  valid_hosts,
                  _output );
        if ( !ret.ok() ) {
            final_ret = PASS( ret );
            irods::log( final_ret );

        }

        // the icat needs to be notified last in certain
        // cases such as SHUTDOWN or PAUSE  where it is
        // needed to capture the host list for validation
        ret = notify_icat_and_local_servers_postop(
                  cmd_name,
                  cmd_option,
                  wait_option,
                  wait_seconds,
                  cmd_hosts, // dont want sanitized
                  _output );
        if ( !ret.ok() ) {
            final_ret = PASS( ret );
            irods::log( final_ret );
        }

        return final_ret;

    } // process_operation

}; // namespace irods
