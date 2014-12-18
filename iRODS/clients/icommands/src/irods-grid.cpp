
#include "zmq.hpp"
#include "avro/Encoder.hh"
#include "avro/Decoder.hh"
#include "avro/Specific.hh"

#include <string>
#include <sstream>
#include <iostream>
#include "boost/unordered_map.hpp"

#include "rodsClient.hpp"
#include "irods_server_control_plane.hpp"
#include "server_control_plane_command.hpp"

int usage() {
    std::cout << "FIXME" << std::endl;
    return 0;

} // usage



int main(
    int   _argc,
    char* _argv[] ) {
    std::vector< std::string > cmd_line;
    for ( int i = 0;
            i < _argc;
            ++i ) {
        cmd_line.push_back( _argv[ i ] );

    }

    if ( 1 == cmd_line.size() ) {
        return usage();

    }

    const size_t sub_idx = 1;
    const size_t opt_idx = 2;

    boost::unordered_map< std::string, std::string > cmd_map;
    cmd_map[ "status"   ] = irods::SERVER_CONTROL_STATUS;
    cmd_map[ "pause"    ] = irods::SERVER_CONTROL_PAUSE;
    cmd_map[ "resume"   ] = irods::SERVER_CONTROL_RESUME;
    cmd_map[ "shutdown" ] = irods::SERVER_CONTROL_SHUTDOWN;

    if ( cmd_map.end() == cmd_map.find( cmd_line[ sub_idx ] ) ) {
        std::cout << "invalid subcommand ["
                  << cmd_line[ sub_idx ]
                  << "]"
                  << std::endl;
        return usage();

    }

    // fair to say we have a valid subcommand at least
    irods::control_plane_command cmd;
    cmd.command = cmd_map[ cmd_line[ sub_idx ] ];

    // serialize possible option which must be 'all' or 'hosts'
    if ( cmd_line.size() > 2 ) {
        if ( irods::SERVER_CONTROL_ALL_OPT   != cmd_line[ opt_idx ] &&
                irods::SERVER_CONTROL_HOSTS_OPT != cmd_line[ opt_idx ] ) {
            std::cout << "invalid option ["
                      << cmd_line[ opt_idx ]
                      << "]"
                      << std::endl;
            return usage();

        }
        else {
            cmd.options[ irods::SERVER_CONTROL_OPTION_KW ] = cmd_line[ opt_idx ];

        }
    }

    // serialize remaining command line parameters as hosts, using
    // numbered host keywords as the keys
    for ( size_t i = 3;
            i < cmd_line.size();
            ++i ) {
        std::stringstream ss; ss << i - 3;
        cmd.options[ irods::SERVER_CONTROL_HOST_KW + ss.str() ] = cmd_line[ i ];

    }

    // fetch client environment for proper port
    rodsEnv env;
    getRodsEnv( &env );

    // standard zmq rep-req communication pattern
    zmq::context_t zmq_ctx( 1 );
    zmq::socket_t  zmq_skt( zmq_ctx, ZMQ_REQ );

    // this is the client so we connect rather than bind
    std::stringstream port_sstr;
    port_sstr << env.irodsCtrlPlanePort;
    std::string bind_str( "tcp://localhost:" );
    bind_str += port_sstr.str();
    zmq_skt.connect( bind_str.c_str() );

    // serialize using the generated avro class
    std::auto_ptr< avro::OutputStream > out = avro::memoryOutputStream();
    avro::EncoderPtr e = avro::binaryEncoder();
    e->init( *out );
    avro::encode( *e, cmd );
    boost::shared_ptr< std::vector< uint8_t > > data = avro::snapshot( *out );

    // copy binary encoding into a zmq message for transport
    zmq::message_t rep( data->size() );
    memcpy(
        rep.data(),
        data->data(),
        data->size() );
    zmq_skt.send( rep );

    // wait for the server reponse
    zmq::message_t req;
    zmq_skt.recv( &req );

    std::string rep_str(
        static_cast< char* >( req.data() ),
        req.size() );
    if ( irods::SERVER_CONTROL_SUCCESS != rep_str ) {
        std::cout << rep_str
                  << std::endl;
    }

    return 0;

} // main


