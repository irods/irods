#include "zmq.hpp"
#include "avro/Encoder.hh"
#include "avro/Decoder.hh"
#include "avro/Specific.hh"

#include <string>
#include <sstream>
#include <iostream>

#include "boost/unordered_map.hpp"
#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

#include "rodsClient.h"
#include "irods_server_control_plane.hpp"
#include "irods_buffer_encryption.hpp"
#include "server_control_plane_command.hpp"
#include "irods_buffer_encryption.hpp"
#include "irods_exception.hpp"

#include <nlohmann/json.hpp>

template <typename T>
irods::error usage(T& ostream) {
    ostream << "Usage:  'irods-grid action [ option ] target'" << std::endl;
    ostream << " " << std::endl;
    ostream << "Sends a message on control channel for possible grid-wide operations" << std::endl;
    ostream << " " << std::endl;
    ostream << "action: ( required ) status, ping, pause, resume, shutdown" << std::endl;
    ostream << " " << std::endl;
    ostream << "option: --force-after=seconds or --wait-forever" << std::endl;
    ostream << " " << std::endl;
    ostream << "target: ( required ) --all, --hosts=<fqdn1>,<fqdn2>,..." << std::endl;
    ostream << " " << std::endl;
    ostream << "Examples:" << std::endl;
    ostream << " " << std::endl;
    ostream << "Status - returns a status of the server (or servers) requested in a validated" << std::endl;
    ostream << "json document which includes iRODS Server PID, iRODS Server hostname," << std::endl;
    ostream << "Rule Execution server PID, Agent PIDs and their age," << std::endl;
    ostream << "XMessage server PID, Server status" << std::endl;
    ostream << " irods-grid status --all" << std::endl;
    ostream << " " << std::endl;
    ostream << "Ping - attempt a connection to a server or servers" << std::endl;
    ostream << " irods-grid ping --all" << std::endl;
    ostream << " " << std::endl;
    ostream << "Pause - suspend all incoming connections while allowing existing client connections to complete" << std::endl;
    ostream << " irods-grid pause --all" << std::endl;
    ostream << " " << std::endl;
    ostream << "Resume - allow incoming connections from new clients" << std::endl;
    ostream << " irods-grid resume --all" << std::endl;
    ostream << " " << std::endl;
    ostream << "Shutdown - gracefully shutdown an iRODS server or servers after allowing existing client connections to complete" << std::endl;
    ostream << " irods-grid shutdown --force-after=5 --all" << std::endl;
    ostream << " " << std::endl;
    ostream << " irods-grid shutdown --wait-forever --all" << std::endl;

    return ERROR( SYS_INVALID_INPUT_PARAM, "usage" );

} // usage

irods::error parse_program_options(
    int   _argc,
    char* _argv[],
    irods::control_plane_command& _cmd ) {

    namespace po = boost::program_options;
    po::options_description opt_desc( "options" );
    opt_desc.add_options()
    ( "action", "either 'status', 'ping', 'shutdown', 'pause', or 'resume'" )
    ( "help,h", "show command usage" )
    ( "all", "operation applies to all servers in the grid" )
    ( "hosts", po::value<std::string>(), "operation applies to a list of hosts in the grid" )
    ( "force-after", po::value<size_t>(), "force shutdown after N seconds" )
    ( "wait-forever", "wait indefinitely for a graceful shutdown" )
    ( "ping", "attempt a connection to a server(s)" )
    ( "status", "display status a server(s)" )
    ( "shutdown", "gracefully shutdown a server(s)" )
    ( "pause", "refuse new client connections" )
    ( "resume", "allow new client connections" );

    po::positional_options_description pos_desc;
    pos_desc.add( "action", 1 );

    po::variables_map vm;
    try {
        po::store(
            po::command_line_parser(
                _argc, _argv ).options(
                opt_desc ).positional(
                pos_desc ).run(), vm );
        po::notify( vm );
    }
    catch ( po::error& _e ) {
        std::cerr << std::endl
                  << "Error: "
                  << _e.what()
                  << std::endl
                  << std::endl;
        return usage(std::cerr);

    }

    // capture the 'subcommand' or 'action' to perform on the grid
    irods::control_plane_command cmd;
    if ( vm.count( "action" ) ) {
        try {
            const std::string& action = vm["action"].as<std::string>();
            boost::unordered_map< std::string, std::string > cmd_map;
            cmd_map[ "status"   ] = irods::SERVER_CONTROL_STATUS;
            cmd_map[ "ping"     ] = irods::SERVER_CONTROL_PING;
            cmd_map[ "pause"    ] = irods::SERVER_CONTROL_PAUSE;
            cmd_map[ "resume"   ] = irods::SERVER_CONTROL_RESUME;
            cmd_map[ "shutdown" ] = irods::SERVER_CONTROL_SHUTDOWN;

            if ( cmd_map.end() == cmd_map.find( action ) ) {
                std::cerr << "invalid subcommand ["
                          << action
                          << "]"
                          << std::endl;
                return usage(std::cerr);

            }

            _cmd.command = cmd_map[ action ];
        }
        catch ( const boost::bad_any_cast& ) {
            return ERROR( INVALID_ANY_CAST, "Attempt to cast vm[\"action\"] to std::string failed." );
        }
    }
    else {
        return usage(std::cout);

    }

    if ( vm.count( "force-after" ) ) {
        try {
            size_t secs = vm[ "force-after" ].as<size_t>();
            std::stringstream ss; ss << secs;
            _cmd.options[ irods::SERVER_CONTROL_FORCE_AFTER_KW ] =
                ss.str();
        }
        catch ( const boost::bad_any_cast& ) {
            return ERROR( INVALID_ANY_CAST, "Attempt to cast vm[\"force-after\"] to std::string failed." );
        }
    }
    else if ( vm.count( "wait-forever" ) ) {
        _cmd.options[ irods::SERVER_CONTROL_WAIT_FOREVER_KW ] =
            "keep_waiting";

    }

    // capture either the 'all' servers or the hosts list
    if ( vm.count( "all" ) ) {
        _cmd.options[ irods::SERVER_CONTROL_OPTION_KW ] =
            irods::SERVER_CONTROL_ALL_OPT;

    }
    else if ( vm.count( "hosts" ) ) {
        _cmd.options[ irods::SERVER_CONTROL_OPTION_KW ] =
            irods::SERVER_CONTROL_HOSTS_OPT;

        std::vector< std::string > hosts;
        try {
            boost::split(
                hosts,
                vm[ "hosts" ].as<std::string>(),
                boost::is_any_of( ", " ),
                boost::token_compress_on );
        }
        catch ( const boost::bad_function_call& ) {
            return ERROR( BAD_FUNCTION_CALL, "Boost threw bad_function_call." );
        }
        catch ( const boost::bad_any_cast& ) {
            return ERROR( INVALID_ANY_CAST, "Attempt to cast vm[\"hosts\"] to std::string failed." );
        }

        for ( size_t i = 0;
                i < hosts.size();
                ++i ) {
            std::stringstream ss; ss << i;
            _cmd.options[ irods::SERVER_CONTROL_HOST_KW + ss.str() ] =
                hosts[ i ];

        } // for i

    }
    else {
        return usage(std::cerr);

    }

    return SUCCESS();

} // parse_program_options

irods::error prepare_command_for_transport(
    const rodsEnv&                      _env,
    const irods::control_plane_command& _cmd,
    irods::buffer_crypt::array_t&       _data_to_send ) {

    // build a encryption context
    std::string encryption_key( _env.irodsCtrlPlaneKey );
    irods::buffer_crypt crypt(
        encryption_key.size(), // key size
        0,                     // salt size ( we dont send a salt )
        _env.irodsCtrlPlaneEncryptionNumHashRounds,
        _env.irodsCtrlPlaneEncryptionAlgorithm );

    // serialize using the generated avro class
    auto out = avro::memoryOutputStream();
    avro::EncoderPtr e = avro::binaryEncoder();
    e->init( *out );
    avro::encode( *e, _cmd );
    std::shared_ptr<std::vector<uint8_t>> data = avro::snapshot(*out);

    // encrypt outgoing request
    std::vector< unsigned char > enc_data(
        data->begin(),
        data->end() );

    irods::buffer_crypt::array_t iv;
    irods::buffer_crypt::array_t in_buf(
        enc_data.begin(),
        enc_data.end() );
    irods::buffer_crypt::array_t shared_secret(
        encryption_key.begin(),
        encryption_key.end() );
    irods::error ret = crypt.encrypt(
                           shared_secret,
                           iv,
                           in_buf,
                           _data_to_send );
    if ( !ret.ok() ) {
        return PASS( ret );

    }

    return SUCCESS();

} // prepare_command_for_transport

irods::error decrypt_response(
    const rodsEnv& _env,
    const uint8_t* _data_ptr,
    const size_t   _data_size,
    std::string&   _rep_str ) {

    irods::buffer_crypt::array_t in_buf;
    in_buf.assign(
        _data_ptr,
        _data_ptr + _data_size );

    std::string encryption_key( _env.irodsCtrlPlaneKey );
    irods::buffer_crypt crypt(
        encryption_key.size(), // key size
        0,                     // salt size ( we dont send a salt )
        _env.irodsCtrlPlaneEncryptionNumHashRounds,
        _env.irodsCtrlPlaneEncryptionAlgorithm );


    irods::buffer_crypt::array_t iv;
    irods::buffer_crypt::array_t shared_secret(
        encryption_key.begin(),
        encryption_key.end() );
    irods::buffer_crypt::array_t decoded_data;
    irods::error ret = crypt.decrypt(
                           shared_secret,
                           iv,
                           in_buf,
                           decoded_data );
    if ( !ret.ok() ) {
        return PASS( ret );

    }

    _rep_str.assign(
        decoded_data.begin(),
        decoded_data.begin() + decoded_data.size() );

    return SUCCESS();

} // decrypt_response

std::string format_grid_message( const std::string& _status)
{
    std::string status = "{\n    \"hosts\": [\n";
    status += _status;
    status += "]    \n}";

    std::string::size_type pos = status.find_last_of( "," );
    if ( std::string::npos != pos ) {
        status.erase( pos, 1 );
    }
    else {
        // possible error message
        THROW( -1, std::string("server responded with an error\n") + _status );
    }

    using json = nlohmann::json;

    json obj;

    try {
        obj = json::parse(static_cast<const char*>(status.data()));
    }
    catch (const json::parse_error& e) {
        if (std::string::npos != _status.find(irods::SERVER_PAUSED_ERROR)) {
            THROW(-1, std::string{"server paused error\n"} + _status);
        }

        THROW(-1, boost::format("json::parse failed with error: %s\n") % e.what());
    }

    status = obj.dump(4);

    return status;

} // format_grid_message

irods::error get_and_verify_client_environment(
    rodsEnv& _env ) {
    _getRodsEnv( _env );

    bool have_error = false;
    std::string msg = "missing environment entries: ";
    if( 0 == strlen( _env.irodsCtrlPlaneKey ) ) {
        have_error = true;
        msg += "irods_server_control_plane_key";
    }

    if( 0 == _env.irodsCtrlPlaneEncryptionNumHashRounds ) {
        have_error = true;
        msg += ", irods_server_control_plane_encryption_num_hash_rounds";
    }

    if( 0 == strlen( _env.irodsCtrlPlaneEncryptionAlgorithm ) ) {
        have_error = true;
        msg += ", irods_server_control_plane_encryption_algorithm";
    }

    if( 0 == _env.irodsCtrlPlanePort ) {
        have_error = true;
        msg += ", irods_server_control_plane_port";
    }

    if( have_error ) {
        return ERROR(
                   USER_INVALID_CLIENT_ENVIRONMENT,
                   msg );
    }

    return SUCCESS();

} // verify_client_environment

int main(
    int   _argc,
    char* _argv[] ) {

    irods::control_plane_command cmd;
    irods::error ret = parse_program_options(
                           _argc,
                           _argv,
                           cmd );
    if ( !ret.ok() ) {
        return 0;

    }

    rodsEnv env;
    irods::error err = get_and_verify_client_environment( env );
    if( !err.ok() ) {
        std::cerr << err.result();
        return 1;
    }

    irods::buffer_crypt::array_t data_to_send;
    ret = prepare_command_for_transport(
              env,
              cmd,
              data_to_send );
    if ( !ret.ok() ) {
        std::cerr << ret.result()
                  << std::endl;
        return ret.code();

    }

    // this is the client so we connect rather than bind
    std::stringstream port_sstr;
    port_sstr << env.irodsCtrlPlanePort;
    std::string bind_str( "tcp://localhost:" );
    bind_str += port_sstr.str();
    try {
        // standard zmq rep-req communication pattern
        zmq::context_t zmq_ctx( 1 );
        zmq::socket_t  zmq_skt( zmq_ctx, ZMQ_REQ );
        try {
            zmq_skt.connect(bind_str.c_str());
        }
        catch ( const zmq::error_t& ) {
            std::cerr << "ZeroMQ encountered an error connecting to a socket.\n";
            return 1;
        }

        // copy binary encoding into a zmq message for transport
        zmq::message_t req( data_to_send.size() );
        memcpy(
            req.data(),
            data_to_send.data(),
            data_to_send.size() );
        try {
            if (!zmq_skt.send(req, zmq::send_flags::none)) {
                std::cerr << "ZeroMQ encountered an error sending a message.\n";
                return errno;
            }
        }
        catch ( const zmq::error_t& ) {
            std::cerr << "ZeroMQ encountered an error sending a message.\n";
            return 1;
        }

        zmq::message_t rep;
        // wait for the server response
        try {
            if (!zmq_skt.recv(rep)) {
                std::cerr << "ZeroMQ encountered an error receiving a message.\n";
                return errno;
            }
        }
        catch ( const zmq::error_t& ) {
            std::cerr << "ZeroMQ encountered an error receiving a message.\n";
            return 1;
        }

        // decrypt the response
        std::string rep_str;
        ret = decrypt_response(
                  env,
                  static_cast< const uint8_t* >( rep.data() ),
                  rep.size(),
                  rep_str );
        if ( !ret.ok() ) {
            irods::error err = PASS( ret );
            std::cerr << err.result()
                      << std::endl;
            return 1;

        }

        if ( irods::SERVER_CONTROL_SUCCESS != rep_str ) {
            try {
                rep_str = format_grid_message( rep_str );
                std::cout << rep_str << std::endl;
            } catch ( const irods::exception& e_ ) {
                std::cerr << e_.message_stack()[0];
            }
        }

    }
    catch ( const zmq::error_t& ) {
        std::cerr << "ZeroMQ encountered an error.\n";
        return 1;

    }

    return 0;

} // main
