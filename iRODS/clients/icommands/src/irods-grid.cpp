
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

#include "jansson.h"

#include "rodsClient.hpp"
#include "irods_server_control_plane.hpp"
#include "irods_buffer_encryption.hpp"
#include "server_control_plane_command.hpp"
#include "irods_buffer_encryption.hpp"

irods::error usage() {
    std::cout << "usage:  'irods-grid action [ option ] target'" << std::endl;
    std::cout << "action: ( required ) status, pause, resume, shutdown" << std::endl;
    std::cout << "option: --force-after=seconds or --wait-forever" << std::endl;
    std::cout << "target: ( required ) --all, --hosts=\"<fqdn1>, <fqdn2>, ...\"" << std::endl;

    return ERROR(
               SYS_INVALID_INPUT_PARAM,
               "usage" );

} // usage

irods::error parse_program_options(
    int   _argc,
    char* _argv[],
    irods::control_plane_command& _cmd ) {

    namespace po = boost::program_options;
    po::options_description opt_desc( "options" );
    opt_desc.add_options()
    ( "action", "either 'status', 'shutdown', 'pause', or 'resume'" )
    ( "help", "show command usage" )
    ( "all", "operation applies to all servers in the grid" )
    ( "hosts", po::value<std::string>(), "operation applies to a list of hosts in the grid" )
    ( "force-after", po::value<size_t>(), "force shutdown after N seconds" )
    ( "wait-forever", "wait indefinitely for a graceful shutdown" )
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
        std::cout << std::endl
                  << "Error: "
                  << _e.what()
                  << std::endl
                  << std::endl;
        return usage();

    }

    // capture the 'subcommand' or 'action' to perform on the grid
    irods::control_plane_command cmd;
    if ( vm.count( "action" ) ) {
        try {
            const std::string& action = vm["action"].as<std::string>();
            boost::unordered_map< std::string, std::string > cmd_map;
            cmd_map[ "status"   ] = irods::SERVER_CONTROL_STATUS;
            cmd_map[ "pause"    ] = irods::SERVER_CONTROL_PAUSE;
            cmd_map[ "resume"   ] = irods::SERVER_CONTROL_RESUME;
            cmd_map[ "shutdown" ] = irods::SERVER_CONTROL_SHUTDOWN;

            if ( cmd_map.end() == cmd_map.find( action ) ) {
                std::cout << "invalid subcommand ["
                          << action
                          << "]"
                          << std::endl;
                return usage();

            }

            _cmd.command = cmd_map[ action ];
        }
        catch ( const boost::bad_any_cast& ) {
            return ERROR( INVALID_ANY_CAST, "Attempt to cast vm[\"action\"] to std::string failed." );
        }
    }
    else {
        return usage();

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
                boost::is_any_of( "," ),
                boost::token_compress_on );
        }
        catch ( const boost::bad_function_call& ) {
            return ERROR( BAD_FUNCTION_CALL, "Boost threw bad_function_call." );
        }
        catch ( const boost::bad_any_cast& ) {
            return ERROR( INVALID_ANY_CAST, "Attempt to cast vm[\"force-after\"] to std::string failed." );
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
        return usage();

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
    std::auto_ptr< avro::OutputStream > out = avro::memoryOutputStream();
    avro::EncoderPtr e = avro::binaryEncoder();
    e->init( *out );
    avro::encode( *e, _cmd );
    boost::shared_ptr< std::vector< uint8_t > > data = avro::snapshot( *out );

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

std::string format_grid_status(
    const std::string& _status ) {


    std::string status = "{\n    \"hosts\": [\n";
    status += _status;
    status += "]    \n}";

    std::string::size_type pos = status.find_last_of( "," );
    if( std::string::npos != pos ) {
        status.erase( pos, 1 );
    } else {
        // possible error message
        return _status;
    
    }

    json_error_t j_err;
    json_t* obj = json_loads(
                      ( char* )status.data(),
                      status.size(),
                      &j_err );
    if ( !obj ) {
        if ( std::string::npos != _status.find( irods::SERVER_PAUSED_ERROR ) ) {
            return _status;
        }

        std::string msg( "json_loads failed [" );
        msg += j_err.text;
        msg += "]";
        std::cout << msg << std::endl;
        return "";
    }

    char* tmp_buf = json_dumps( obj, JSON_INDENT( 4 ) );
    json_decref( obj );

    return tmp_buf;

} // format_grid_status

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

    // fetch client environment for proper port
    rodsEnv env;
    _getRodsEnv( env );

    irods::buffer_crypt::array_t data_to_send;
    ret = prepare_command_for_transport(
              env,
              cmd,
              data_to_send );
    if ( !ret.ok() ) {
        std::cout << ret.result()
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
            zmq_skt.connect( bind_str.c_str() );
        }
        catch ( const zmq::error_t& ) {
            printf( "ZeroMQ encountered an error connecting to a socket." );
            return -1;
        }

        // copy binary encoding into a zmq message for transport
        zmq::message_t rep( data_to_send.size() );
        memcpy(
            rep.data(),
            data_to_send.data(),
            data_to_send.size() );
        try {
            zmq_skt.send( rep );
        }
        catch ( const zmq::error_t& ) {
            printf( "ZeroMQ encountered an error receiving a message." );
            return -1;
        }


        zmq::message_t req;
        // wait for the server reponse
        try {
            zmq_skt.recv( &req );
        }
        catch ( const zmq::error_t& ) {
            printf( "ZeroMQ encountered an error receiving a message." );
            return -1;
        }

        // decrypt the response
        std::string rep_str;
        ret = decrypt_response(
                  env,
                  static_cast< const uint8_t* >( req.data() ),
                  req.size(),
                  rep_str );
        if ( !ret.ok() ) {
            irods::error err = PASS( ret );
            std::cout << err.result()
                      << std::endl;
            return -1;

        }

        if ( irods::SERVER_CONTROL_SUCCESS != rep_str ) {
            if ( irods::SERVER_CONTROL_STATUS == cmd.command ) {
                rep_str = format_grid_status( rep_str );

            }

            std::cout << rep_str << std::endl;

        }

    }
    catch ( const zmq::error_t& ) {
        printf( "ZeroMQ encountered an error." );
        return -1;

    }

    return 0;

} // main


