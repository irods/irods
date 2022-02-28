// =-=-=-=-=-=-=-
// irods includes
#include "irods/rodsDef.h"
#include "irods/msParam.h"
#include "irods/rcConnect.h"
#include "irods/sockComm.h"

// =-=-=-=-=-=-=-
#include "irods/irods_network_plugin.hpp"
#include "irods/irods_network_constants.hpp"
#include "irods/irods_tcp_object.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/sockCommNetworkInterface.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>

// =-=-=-=-=-=-=-
// local function to read a buffer from a socket
irods::error tcp_socket_read(
    int             _socket,
    void*           _buffer,
    int             _length,
    int&            _bytes_read,
    struct timeval* _time_value ) {
    // =-=-=-=-=-=-=-
    // Initialize the file descriptor set
    fd_set set;
    FD_ZERO( &set );
    FD_SET( _socket, &set );

    // =-=-=-=-=-=-=-
    // local copy of time value?
    struct timeval timeout;
    if ( _time_value != NULL ) {
        timeout = ( *_time_value );
    }

    // =-=-=-=-=-=-=-
    // local working variables
    int   len_to_read = _length;
    char* read_ptr    = static_cast<char*>( _buffer );

    // =-=-=-=-=-=-=-
    // reset bytes read
    _bytes_read = 0;

    while ( len_to_read > 0 ) {
        if ( nullptr != _time_value ) {
            const int status = select( _socket + 1, &set, NULL, NULL, &timeout );
            if ( status == 0 ) { // the select has timed out
                return ERROR( SYS_SOCK_READ_TIMEDOUT, boost::format("socket timeout with [%d] bytes read") % _bytes_read);
            } else if ( status < 0 ) {
                if ( errno == EINTR ) {
                    continue;
                } else {
                    return ERROR( SYS_SOCK_READ_ERR - errno, boost::format("error on select after [%d] bytes read") % _bytes_read);
                }
            } // else
        } // if tv

        int num_bytes = read( _socket, ( void * ) read_ptr, len_to_read );
        if ( num_bytes < 0 ) {
            if ( EINTR == errno ) {
                errno = 0;
                num_bytes = 0;
            } else {
                return ERROR(SYS_SOCK_READ_ERR - errno, boost::format("error reading from socket after [%d] bytes read") % _bytes_read);
            }
        } else if ( num_bytes == 0 ) {
            break;
        }

        len_to_read -= num_bytes;
        read_ptr    += num_bytes;
        _bytes_read += num_bytes;
    } // while

    return CODE( _length - len_to_read );
} // tcp_socket_read

// =-=-=-=-=-=-=-
// local function to write a buffer to a socket
irods::error tcp_socket_write(
    int         _socket,
    const void* _buffer,
    int         _length,
    int&        _bytes_written ) {
    // =-=-=-=-=-=-=-
    // local variables for write
    int   len_to_write = _length;
    const char* write_ptr    = static_cast<const char*>( _buffer );

    // =-=-=-=-=-=-=-
    // reset bytes written
    _bytes_written = 0;

    // =-=-=-=-=-=-=-
    // loop while there is data to read
    while ( len_to_write > 0 ) {
        int num_bytes = write( _socket,
                               static_cast<const void*>( write_ptr ),
                               len_to_write );
        // =-=-=-=-=-=-=-
        // error trapping the write
        if ( num_bytes <= 0 ) {
            // =-=-=-=-=-=-=-
            // gracefully handle an interrupt
            if ( errno == EINTR ) {
                errno     = 0;
                num_bytes = 0;

            }
            else {
                break;

            }
        }

        // =-=-=-=-=-=-=-
        // increment working variables
        len_to_write   -= num_bytes;
        write_ptr      += num_bytes;
        _bytes_written += num_bytes;

    }

    // =-=-=-=-=-=-=-
    // and were done? report length not written
    return CODE( _length - len_to_write );

} // tcp_socket_write

// =-=-=-=-=-=-=-
//
irods::error tcp_start(
    irods::plugin_context& ) {
    return SUCCESS();

} // tcp_start

// =-=-=-=-=-=-=-
//
irods::error tcp_end(
    irods::plugin_context& ) {
    return SUCCESS();

} // tcp_end

// =-=-=-=-=-=-=-
//
irods::error tcp_shutdown(
    irods::plugin_context& ) {
    return SUCCESS();

} // tcp_end

// =-=-=-=-=-=-=-
//
irods::error tcp_read_msg_header(
    irods::plugin_context& _ctx,
    void*                  _buffer,
    struct timeval*        _time_val ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid< irods::tcp_object >();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // extract the useful bits from the context
    irods::tcp_object_ptr tcp = boost::dynamic_pointer_cast< irods::tcp_object >( _ctx.fco() );
    int socket_handle = tcp->socket_handle();

    // =-=-=-=-=-=-=-
    // read the header length packet */
    int header_length = 0;
    int bytes_read    = 0;
    ret = tcp_socket_read(
              socket_handle,
              static_cast<void*>( &header_length ),
              sizeof( header_length ),
              bytes_read,
              _time_val );
    if (!ret.ok()) {
        return PASS(ret);
    }

    if (bytes_read != sizeof(header_length)) {
        return ERROR(SYS_HEADER_READ_LEN_ERR, boost::format("only read [%d] of [%d]") % bytes_read % sizeof(header_length));
    }

    header_length = ntohl( header_length );

    // check head length against expected size range
    if ( header_length >  MAX_NAME_LEN ||
            header_length <= 0 ) {
        std::stringstream msg;
        msg << "header length is out of range: "
            << header_length
            << " expected >= 0 and < "
            << MAX_NAME_LEN;
        return ERROR( SYS_HEADER_READ_LEN_ERR, msg.str() );
    }

    // now read the actual header
    ret = tcp_socket_read(
              socket_handle,
              _buffer,
              header_length,
              bytes_read,
              _time_val );

    if (!ret.ok()) {
        return PASS(ret);
    }

    if (bytes_read != header_length) {
        return ERROR(SYS_HEADER_READ_LEN_ERR, boost::format("only read [%d] of [%d]") % bytes_read % header_length);
    }

    rodsLog(LOG_DEBUG8, "received header: len = %d\n%.*s\n", header_length, bytes_read, static_cast<const char*>( _buffer ) );

    return SUCCESS();
} // tcp_read_msg_header

// =-=-=-=-=-=-=-
//
irods::error tcp_write_msg_header(
    irods::plugin_context& _ctx,
    const bytesBuf_t*      _header ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid< irods::tcp_object >();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // log debug information if appropriate
    if ( getRodsLogLevel() >= LOG_DEBUG8 ) {
        printf( "sending header: len = %d\n%.*s\n",
                _header->len,
                _header->len,
                ( const char * ) _header->buf );
    }

    // =-=-=-=-=-=-=-
    // extract the useful bits from the context
    irods::tcp_object_ptr tcp = boost::dynamic_pointer_cast< irods::tcp_object >( _ctx.fco() );
    int socket_handle = tcp->socket_handle();

    // =-=-=-=-=-=-=-
    // convert host byte order to network byte order
    int header_length = htonl( _header->len );

    // =-=-=-=-=-=-=-
    // write the length of the header to the socket
    int bytes_written = 0;
    ret = tcp_socket_write(
              socket_handle,
              &header_length,
              sizeof( header_length ),
              bytes_written );
    if ( !ret.ok() ||
            bytes_written != sizeof( header_length ) ) {
        std::stringstream msg;
        msg << "wrote "
            << bytes_written
            << " expected " << header_length;
        return ERROR( SYS_HEADER_WRITE_LEN_ERR - errno,
                      msg.str() );
    }

    // =-=-=-=-=-=-=-
    // now send the actual header
    ret = tcp_socket_write(
              socket_handle,
              _header->buf,
              _header->len,
              bytes_written );
    if ( !ret.ok() ||
            bytes_written != _header->len ) {
        std::stringstream msg;
        msg << "wrote "
            << bytes_written
            << " expected " << _header->len;
        return ERROR( SYS_HEADER_WRITE_LEN_ERR - errno,
                      msg.str() );
    }

    return SUCCESS();

} // tcp_write_msg_header

// =-=-=-=-=-=-=-
//
irods::error tcp_send_rods_msg(
    irods::plugin_context& _ctx,
    const char*            _msg_type,
    const bytesBuf_t*      _msg_buf,
    const bytesBuf_t*      _stream_bbuf,
    const bytesBuf_t*      _error_buf,
    int                    _int_info,
    irodsProt_t            _protocol ) {
    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid< irods::tcp_object >();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // check the params
    if ( !_msg_type ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null msg type" );
    }


    // =-=-=-=-=-=-=-
    // extract the useful bits from the context
    irods::tcp_object_ptr tcp = boost::dynamic_pointer_cast< irods::tcp_object >( _ctx.fco() );
    int socket_handle = tcp->socket_handle();

    // =-=-=-=-=-=-=-
    // initialize a new header
    msgHeader_t msg_header;
    memset( &msg_header, 0, sizeof( msg_header ) );

    snprintf( msg_header.type, HEADER_TYPE_LEN, "%s", _msg_type );
    msg_header.intInfo = _int_info;

    // =-=-=-=-=-=-=-
    // initialize buffer lengths
    if ( _msg_buf ) {
        msg_header.msgLen = _msg_buf->len;
    }
    if ( _stream_bbuf ) {
        msg_header.bsLen = _stream_bbuf->len;
    }
    if ( _error_buf ) {
        msg_header.errorLen = _error_buf->len;
    }

    // =-=-=-=-=-=-=-
    // send the header
    ret = writeMsgHeader(
              tcp,
              &msg_header );
    if ( !ret.ok() ) {
        return PASSMSG( "writeMsgHeader failed", ret );
    }

    // =-=-=-=-=-=-=-
    // send the message buffer
    int bytes_written = 0;
    if ( _msg_buf && _msg_buf->len > 0 ) {
        if ( XML_PROT == _protocol &&
                getRodsLogLevel() >= LOG_DEBUG8 ) {
            printf( "sending msg: \n%.*s\n", _msg_buf->len, ( const char* ) _msg_buf->buf );
        }
        ret = tcp_socket_write(
                  socket_handle,
                  _msg_buf->buf,
                  _msg_buf->len,
                  bytes_written );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

    } // if msgLen > 0

    // =-=-=-=-=-=-=-
    // send the error buffer
    if ( _error_buf && _error_buf->len > 0 ) {
        if ( XML_PROT == _protocol &&
                getRodsLogLevel() >= LOG_DEBUG8 ) {
            printf( "sending msg: \n%.*s\n", _error_buf->len, ( const char* ) _error_buf->buf );

        }

        ret = tcp_socket_write(
                  socket_handle,
                  _error_buf->buf,
                  _error_buf->len,
                  bytes_written );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

    } // if errorLen > 0

    // =-=-=-=-=-=-=-
    // send the stream buffer
    if ( _stream_bbuf && _stream_bbuf->len > 0 ) {
        if ( XML_PROT == _protocol &&
                getRodsLogLevel() >= LOG_DEBUG8 ) {
            printf( "sending msg: \n%.*s\n", _stream_bbuf->len, ( const char* ) _stream_bbuf->buf );
        }

        ret = tcp_socket_write(
                  socket_handle,
                  _stream_bbuf->buf,
                  _stream_bbuf->len,
                  bytes_written );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

    } // if bsLen > 0

    return SUCCESS();

} // tcp_send_rods_msg

// =-=-=-=-=-=-=-
// helper fcn to read a bytes buf
irods::error read_bytes_buf(
    int             _socket_handle,
    int             _length,
    bytesBuf_t*     _buffer,
    irodsProt_t     _protocol,
    struct timeval* _time_val ) {
    // =-=-=-=-=-=-=-
    // trap input buffer ptr
    if ( !_buffer || !_buffer->buf ) {
        return ERROR( SYS_READ_MSG_BODY_INPUT_ERR,
                      "null buffer ptr" );
    }

    int bytes_read = 0;

    // read buffer
    irods::error ret = tcp_socket_read(
                           _socket_handle,
                           _buffer->buf,
                           _length,
                           bytes_read,
                           _time_val );
    _buffer->len = bytes_read;

    // log transaction if requested
    if ( _protocol == XML_PROT ) {
        rodsLog(LOG_DEBUG8, "received msg: \n%.*s\n", _buffer->len, ( char* )_buffer->buf );
    }

    if (!ret.ok()) {
        free(_buffer->buf);
        return PASS(ret);
    }

    if (bytes_read != _length) {
        free(_buffer->buf);
        return ERROR(SYS_READ_MSG_BODY_LEN_ERR, boost::format("only read [%d] of [%d]") % bytes_read % _length);
    }

    return SUCCESS();

} // read_bytes_buf

// =-=-=-=-=-=-=-
// read a message body off of the socket
irods::error tcp_read_msg_body(
    irods::plugin_context& _ctx,
    msgHeader_t*            _header,
    bytesBuf_t*             _input_struct_buf,
    bytesBuf_t*             _bs_buf,
    bytesBuf_t*             _error_buf,
    irodsProt_t             _protocol,
    struct timeval*         _time_val ) {

    // =-=-=-=-=-=-=-
    // client make the assumption that we clear the error
    // buffer for them
    if ( _error_buf ) {
        memset( _error_buf, 0, sizeof( bytesBuf_t ) );

    }

    // =-=-=-=-=-=-=-
    // check the context
    irods::error ret = _ctx.valid< irods::tcp_object >();
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // extract the useful bits from the context
    irods::tcp_object_ptr tcp = boost::dynamic_pointer_cast< irods::tcp_object >( _ctx.fco() );
    int socket_handle = tcp->socket_handle();

    // =-=-=-=-=-=-=-
    // trap header ptr
    if ( !_header ) {
        return ERROR( SYS_READ_MSG_BODY_INPUT_ERR,
                      "null header ptr" );
    }

    // =-=-=-=-=-=-=-
    // reset error buf
    // NOTE :: do not reset bs buf as it can be reused
    //         on the client side
    if ( _error_buf ) {
    }

    // =-=-=-=-=-=-=-
    // read input buffer
    if ( 0 != _input_struct_buf ) {
        if ( _header->msgLen > 0 ) {
            _input_struct_buf->buf = malloc( _header->msgLen + 1 );

            ret = read_bytes_buf(
                      socket_handle,
                      _header->msgLen,
                      _input_struct_buf,
                      _protocol,
                      _time_val );
            if ( !ret.ok() ) {
                return PASS( ret );
            }

        }
        else {
            // =-=-=-=-=-=-=-
            // ensure msg len is 0 as this can cause issues
            // in the agent
            _input_struct_buf->len = 0;

        }

    } // input buffer

    // =-=-=-=-=-=-=-
    // read error buffer
    if ( 0 != _error_buf ) {
        if ( _header->errorLen > 0 ) {
            _error_buf->buf = malloc( _header->errorLen + 1 );
            ret = read_bytes_buf(
                      socket_handle,
                      _header->errorLen,
                      _error_buf,
                      _protocol,
                      _time_val );
            if ( !ret.ok() ) {
                return PASS( ret );
            }
        }
        else {
            _error_buf->len = 0;

        }

    } // error buffer

    // =-=-=-=-=-=-=-
    // read bs buffer
    if ( 0 != _bs_buf ) {
        if ( _header->bsLen > 0 ) {
            // do not repave bs buf as it can be
            // reused by the client
            if ( _bs_buf->buf == NULL ) {
                _bs_buf->buf = malloc( _header->bsLen + 1 );

            }
            else if ( _header->bsLen > _bs_buf->len ) {
                free( _bs_buf->buf );
                _bs_buf->buf = malloc( _header->bsLen + 1 );

            }

            ret = read_bytes_buf(
                      socket_handle,
                      _header->bsLen,
                      _bs_buf,
                      _protocol,
                      _time_val );
            if ( !ret.ok() ) {
                return PASS( ret );
            }
        }
        else {
            _bs_buf->len = 0;

        }

    } // bs buffer

    return SUCCESS();

} // tcp_read_msg_body

// =-=-=-=-=-=-=-
// stubs for ops that the tcp plug does
// not need to support - accept etc
irods::error tcp_client_stop(
    irods::plugin_context&,
    rodsEnv* ) {
    return SUCCESS();

} // tcp_success_stub

irods::error tcp_client_start(
    irods::plugin_context& _ctx,
    rodsEnv*                _env ) {
    return SUCCESS();
}

irods::error tcp_agent_stop(
    irods::plugin_context& ) {
    return SUCCESS();

} // tcp_success_stub

irods::error tcp_agent_start(
    irods::plugin_context& ) {
    return SUCCESS();

} // tcp_success_stub

// =-=-=-=-=-=-=-
// derive a new tcp network plugin from
// the network plugin base class for handling
// tcp communications
class tcp_network_plugin : public irods::network {
    public:
        tcp_network_plugin(
            const std::string& _nm,
            const std::string& _ctx ) :
            irods::network(
                _nm,
                _ctx ) {
        } // ctor

        ~tcp_network_plugin() {
        }

}; // class tcp_network_plugin



// =-=-=-=-=-=-=-
// factory function to provide instance of the plugin
extern "C"
irods::network* plugin_factory(
    const std::string& _inst_name,
    const std::string& _context ) {
    // =-=-=-=-=-=-=-
    // create a tcp network object
    tcp_network_plugin* tcp = new tcp_network_plugin(
        _inst_name,
        _context );

    // =-=-=-=-=-=-=-
    // fill in the operation table mapping call
    // names to function names
    using namespace irods;
    using namespace std;
    tcp->add_operation(
        NETWORK_OP_CLIENT_START,
        function<error(plugin_context&,rodsEnv*)>(
            tcp_client_start ) );
    tcp->add_operation(
        NETWORK_OP_CLIENT_STOP,
        function<error(plugin_context&,rodsEnv*)>(
            tcp_client_stop ) );
    tcp->add_operation(
        NETWORK_OP_AGENT_START,
        function<error(plugin_context&)>(
            tcp_agent_start ) );
    tcp->add_operation(
        NETWORK_OP_AGENT_STOP,
        function<error(plugin_context&)>(
            tcp_agent_stop ) );
    tcp->add_operation(
        NETWORK_OP_READ_HEADER,
        function<error(plugin_context&,void*, struct timeval*)>(
            tcp_read_msg_header ) );
    tcp->add_operation(
        NETWORK_OP_READ_BODY,
        function<error(plugin_context&,msgHeader_t*,bytesBuf_t*,bytesBuf_t*,bytesBuf_t*,irodsProt_t,struct timeval*)>(
            tcp_read_msg_body ) );
    tcp->add_operation(
        NETWORK_OP_WRITE_HEADER,
        function<error(plugin_context&,const bytesBuf_t*)>(
            tcp_write_msg_header ) );
    tcp->add_operation(
        NETWORK_OP_WRITE_BODY,
        function<error(plugin_context&,const char*,const bytesBuf_t*,const bytesBuf_t*,const bytesBuf_t*,int,irodsProt_t)>(
            tcp_send_rods_msg ) );

    irods::network* net = dynamic_cast< irods::network* >( tcp );

    return net;

} // plugin_factory
