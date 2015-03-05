/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sockComm.c - sock communication routines
 */

#include "sockComm.hpp"
#include "rcMisc.hpp"
#include "rcGlobalExtern.hpp"
#include "miscServerFunct.hpp"
#include "getHostForPut.hpp"
#include "getHostForGet.hpp"
#include "QUANTAnet_rbudpBase_c.hpp"
#include "rcConnect.hpp"

#ifdef windows_platform
#include "irodsntutil.hpp"
#endif

#ifdef _WIN32
#include <mmsystem.h>
int win_connect_timeout;
MMRESULT win_connect_timer_id;
#endif

#ifndef _WIN32

#include <setjmp.h>
jmp_buf Jcenv;

#endif  /* _WIN32 */

// =-=-=-=-=-=-=-
#include "irods_stacktrace.hpp"
#include "irods_client_server_negotiation.hpp"
#include "irods_network_plugin.hpp"
#include "irods_network_manager.hpp"
#include "irods_network_factory.hpp"
#include "irods_network_constants.hpp"
#include "irods_environment_properties.hpp"
#include "sockCommNetworkInterface.hpp"

// =-=-=-=-=-=-=-
//
irods::error sockClientStart(
    irods::network_object_ptr _ptr,
    rodsEnv*                   _env ) {
    // =-=-=-=-=-=-=-
    // resolve a network interface plugin from the
    // network object
    irods::plugin_ptr p_ptr;
    irods::error ret_err = _ptr->resolve( irods::NETWORK_INTERFACE, p_ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve network interface", ret_err );

    }

    // =-=-=-=-=-=-=-
    // make the call to the "read" interface
    irods::network_ptr net = boost::dynamic_pointer_cast< irods::network >( p_ptr );
    ret_err = net->call< rodsEnv* >( irods::NETWORK_OP_CLIENT_START, _ptr, _env );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'client start'", ret_err );

    }
    else {
        return CODE( ret_err.code() );

    }

} // sockClientStart

// =-=-=-=-=-=-=-
//
irods::error sockClientStop(
    irods::network_object_ptr _ptr,
    rodsEnv*                   _env ) {
    // =-=-=-=-=-=-=-
    // resolve a network interface plugin from the
    // network object
    irods::plugin_ptr p_ptr;
    irods::error ret_err = _ptr->resolve( irods::NETWORK_INTERFACE, p_ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve network interface", ret_err );

    }

    // =-=-=-=-=-=-=-
    // make the call to the "read" interface
    irods::network_ptr net = boost::dynamic_pointer_cast< irods::network >( p_ptr );
    ret_err = net->call< rodsEnv* >( irods::NETWORK_OP_CLIENT_STOP, _ptr, _env );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'client stop'", ret_err );

    }
    else {
        return CODE( ret_err.code() );

    }

} // sockClientStop

// =-=-=-=-=-=-=-
//
irods::error sockAgentStart(
    irods::network_object_ptr _ptr ) {
    // =-=-=-=-=-=-=-
    // resolve a network interface plugin from the
    // network object
    irods::plugin_ptr p_ptr;
    irods::error ret_err = _ptr->resolve( irods::NETWORK_INTERFACE, p_ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve network interface", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "read" interface
    irods::network_ptr net = boost::dynamic_pointer_cast< irods::network >( p_ptr );
    ret_err = net->call( irods::NETWORK_OP_AGENT_START, _ptr );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'agent start'", ret_err );

    }
    else {
        return CODE( ret_err.code() );

    }

} // sockAgentStart

// =-=-=-=-=-=-=-
//
irods::error sockAgentStop(
    irods::network_object_ptr _ptr ) {
    // =-=-=-=-=-=-=-
    // resolve a network interface plugin from the
    // network object
    irods::plugin_ptr p_ptr;
    irods::error ret_err = _ptr->resolve( irods::NETWORK_INTERFACE, p_ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve network interface", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "read" interface
    irods::network_ptr net = boost::dynamic_pointer_cast< irods::network >( p_ptr );
    ret_err = net->call( irods::NETWORK_OP_AGENT_STOP, _ptr );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'agent stop'", ret_err );
    }
    else {
        return CODE( ret_err.code() );
    }

} // sockAgentStop

// =-=-=-=-=-=-=-
//
irods::error readMsgHeader(
    irods::network_object_ptr _ptr,
    msgHeader_t*               _header,
    struct timeval*            _time_val ) {
    // =-=-=-=-=-=-=-
    // resolve a network interface plugin from the
    // network object
    irods::plugin_ptr p_ptr;
    irods::error ret_err = _ptr->resolve( irods::NETWORK_INTERFACE, p_ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve network interface", ret_err );
    }

    // =-=-=-=-=-=-=-
    // make the call to the "read" interface
    char tmp_buf[ MAX_NAME_LEN ];
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast< irods::first_class_object >( _ptr );
    irods::network_ptr            net = boost::dynamic_pointer_cast< irods::network >( p_ptr );
    ret_err = net->call< void*, struct timeval* >(
                  irods::NETWORK_OP_READ_HEADER,
                  ptr,
                  tmp_buf,
                  _time_val );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'read header'", ret_err );
    }

    // =-=-=-=-=-=-=-
    // unpack the header message, always use XML_PROT for the header
    msgHeader_t* out_header = 0;
    int status = unpackStruct(
                     static_cast<void*>( tmp_buf ),
                     ( void ** )( static_cast< void * >( &out_header ) ),
                     "MsgHeader_PI",
                     RodsPackTable,
                     XML_PROT );
    if ( status < 0 ) {
        return ERROR( status, "unpackStruct error" );
    }

    if ( !out_header ) {
        return ERROR( -1, "" );
    }

    // =-=-=-=-=-=-=-
    // need to do an assignment due to something potentially going out
    // of scope from unpackStruct.
    // NOTE :: look into why this is necessary
    *_header = *out_header;
    free( out_header );

    // =-=-=-=-=-=-=-
    // win!
    return SUCCESS();

} // readMsgHeader

// =-=-=-=-=-=-=-
//
irods::error readMsgBody(
    irods::network_object_ptr _ptr,
    msgHeader_t*        _header,
    bytesBuf_t*         _input_struct_buf,
    bytesBuf_t*         _bs_buf,
    bytesBuf_t*         _error_buf,
    irodsProt_t         _protocol,
    struct timeval*     _time_val ) {
    // =-=-=-=-=-=-=-
    // resolve a network interface plugin from the
    // network object
    irods::plugin_ptr p_ptr;
    irods::error ret_err = _ptr->resolve( irods::NETWORK_INTERFACE, p_ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve network interface", ret_err );

    }

    // =-=-=-=-=-=-=-
    // make the call to the "read" interface
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast< irods::first_class_object >( _ptr );
    irods::network_ptr            net = boost::dynamic_pointer_cast< irods::network >( p_ptr );
    ret_err = net->call < msgHeader_t*,
    bytesBuf_t*,
    bytesBuf_t*,
    bytesBuf_t*,
    irodsProt_t,
    struct timeval* > (
        irods::NETWORK_OP_READ_BODY,
        ptr,
        _header,
        _input_struct_buf,
        _bs_buf,
        _error_buf,
        _protocol,
        _time_val );
    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'read message body'", ret_err );

    }
    else {
        return CODE( ret_err.code() );

    }

} // readMsgBody


/* open sock for incoming connection */
int
sockOpenForInConn( rsComm_t *rsComm, int *portNum, char **addr, int proto ) {
    int status = 0;
    char *tmpPtr;

    if ( proto != SOCK_DGRAM && proto != SOCK_STREAM ) {
        rodsLog( LOG_ERROR,
                 "sockOpenForInConn() -- invalid input protocol %d", proto );
        return SYS_INVALID_PROTOCOL_TYPE;
    }

    struct sockaddr_in  mySockAddr;
    memset( ( char * ) &mySockAddr, 0, sizeof mySockAddr );

    const int sock = socket( AF_INET, proto, 0 );

    if ( sock < 0 ) {
        status = SYS_SOCK_OPEN_ERR - errno;
        rodsLogError( LOG_NOTICE, status,
                      "sockOpenForInConn: open socket error. status = %d", status );
        return status;
    }

    /* For SOCK_DGRAM, done in checkbuf */
    if ( proto == SOCK_STREAM ) {
        rodsSetSockOpt( sock, rsComm->windowSize );
    }

    mySockAddr.sin_family = AF_INET;

    /* if portNum is <= 0 and env svrPortRangeStart is set, pick a port
     * in the range.
     */

    if ( *portNum <= 0 && ( tmpPtr = getenv( "svrPortRangeStart" ) ) != NULL ) {

        const int svrPortRangeStart = atoi( tmpPtr );
        if ( svrPortRangeStart < 1 || svrPortRangeStart > 65535 ) {
            rodsLog( LOG_ERROR, "port %d not in between 1 and 65535, inclusive.", svrPortRangeStart );
            close( sock );
            return SYS_INVALID_INPUT_PARAM;
        }

        int svrPortRangeEnd = 0;
        if ( ( tmpPtr = getenv( "svrPortRangeEnd" ) ) != NULL ) {
            svrPortRangeEnd = atoi( tmpPtr );
            if ( svrPortRangeEnd < svrPortRangeStart ) {
                rodsLog( LOG_ERROR,
                         "sockOpenForInConn: PortRangeStart %d > PortRangeEnd %d",
                         svrPortRangeStart, svrPortRangeEnd );
                svrPortRangeEnd = svrPortRangeStart + DEF_NUMBER_SVR_PORT - 1;
            }
            if ( svrPortRangeEnd > 65535 ) {
                rodsLog( LOG_ERROR,
                         "sockOpenForInConn: PortRangeEnd %d > 65535",
                         svrPortRangeStart, svrPortRangeEnd );
                svrPortRangeEnd = 65535;
            }
        }
        else {
            svrPortRangeEnd = svrPortRangeStart + DEF_NUMBER_SVR_PORT - 1;
        }
        svrPortRangeEnd = svrPortRangeEnd < 65535 ? svrPortRangeEnd : 65535;
        int portRangeCount = svrPortRangeEnd - svrPortRangeStart + 1;

        int myPortNum = svrPortRangeStart + random() % portRangeCount;
        int bindCnt = 0;
        while ( bindCnt < portRangeCount ) {
            if ( myPortNum > svrPortRangeEnd ) {
                myPortNum = svrPortRangeStart;
            }
            mySockAddr.sin_port = htons( myPortNum );

            if ( ( status = bind( sock, ( struct sockaddr * ) &mySockAddr,
                                  sizeof mySockAddr ) ) >= 0 ) {
                *portNum = myPortNum;
                rodsLog( LOG_DEBUG,
                         "sockOpenForInConn: port number = %d", myPortNum );
                break;
            }
            bindCnt ++;
            myPortNum ++;
        }

    }
    else {
        mySockAddr.sin_port = htons( *portNum );
        status = bind( sock, ( struct sockaddr * ) &mySockAddr,
                       sizeof mySockAddr );
    }

    if ( status < 0 ) {
        status = SYS_SOCK_BIND_ERR - errno;
        rodsLog( LOG_NOTICE,
                 "sockOpenForInConn: bind socket error. portNum = %d, errno = %d",
                 *portNum, errno );
        close( sock );
        return status;
    }

    if ( addr != NULL ) {
        *addr = ( char * )malloc( sizeof( char ) * LONG_NAME_LEN );
        gethostname( *addr, LONG_NAME_LEN );
    }

    return sock;
}

/* rsAcceptConn - Server accept connection */

int
rsAcceptConn( rsComm_t *svrComm ) {
    socklen_t len = sizeof( svrComm->remoteAddr );

    const int saved_socket_flags = fcntl( svrComm->sock, F_GETFL );
    int status = fcntl( svrComm->sock, F_SETFL, saved_socket_flags | O_NONBLOCK );
    if ( status < 0 ) {
        rodsLogError( LOG_NOTICE, status, "failed to set flags with nonblock on fnctl with status %d", status );
    }
    const int newSock = accept( svrComm->sock, ( struct sockaddr * ) &svrComm->remoteAddr, &len );
    status = fcntl( svrComm->sock, F_SETFL, saved_socket_flags );
    if ( status < 0 ) {
        rodsLogError( LOG_NOTICE, status, "failed to revert flags on fnctl with status %d", status );
    }

    if ( newSock < 0 ) {
        const int status = SYS_SOCK_ACCEPT_ERR - errno;
        rodsLogError( LOG_NOTICE, status,
                      "rsAcceptConn: accept error for socket %d, status = %d",
                      svrComm->sock, status );
        return newSock;
    }
    rodsSetSockOpt( newSock, svrComm->windowSize );

    return newSock;
}

// =-=-=-=-=-=-=-
//
irods::error writeMsgHeader(
    irods::network_object_ptr _ptr,
    msgHeader_t*               _header ) {
    // =-=-=-=-=-=-=-
    // always use XML_PROT for the Header
    bytesBuf_t* header_buf = 0;
    int status = packStruct(
                     static_cast<void *>( _header ),
                     &header_buf,
                     "MsgHeader_PI",
                     RodsPackTable,
                     0, XML_PROT );
    if ( status < 0 ||
            0 == header_buf ) {
        return ERROR( status, "packstruct error" );
    }

    // =-=-=-=-=-=-=-
    // resolve a network interface plugin from the
    // network object
    irods::plugin_ptr p_ptr;
    irods::error ret = _ptr->resolve( irods::NETWORK_INTERFACE, p_ptr );
    if ( !ret.ok() ) {
        freeBBuf( header_buf );
        return PASSMSG( "failed to resolve network interface", ret );
    }

    // =-=-=-=-=-=-=-
    // make the call to the plugin interface
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast< irods::first_class_object >( _ptr );
    irods::network_ptr            net = boost::dynamic_pointer_cast< irods::network >( p_ptr );
    ret = net->call< bytesBuf_t* >(
              irods::NETWORK_OP_WRITE_HEADER,
              ptr,
              header_buf );

    freeBBuf( header_buf );

    if ( !ret.ok() ) {
        return PASS( ret );
    }

    return SUCCESS();

} // writeMsgHeader

int
myRead( int sock, void *buf, int len,
        int *bytesRead, struct timeval *tv ) {
    int nbytes;
    int toRead;
    char *tmpPtr;
    fd_set set;
    struct timeval timeout;
    int status;

    /* Initialize the file descriptor set. */
    FD_ZERO( &set );
    FD_SET( sock, &set );
    if ( tv != NULL ) {
        timeout = *tv;
    }

    toRead = len;
    tmpPtr = ( char * ) buf;

    if ( bytesRead != NULL ) {
        *bytesRead = 0;
    }

    while ( toRead > 0 ) {
#ifdef _WIN32
        if ( irodsDescType == SOCK_TYPE ) {
            nbytes = recv( sock, tmpPtr, toRead, 0 );
        }
        else {
            nbytes = read( sock, ( void * ) tmpPtr, toRead );
        }
#else
        if ( tv != NULL ) {
            status = select( sock + 1, &set, NULL, NULL, &timeout );
            if ( status == 0 ) {
                /* timedout */
                if ( len - toRead > 0 ) {
                    return len - toRead;
                }
                else {
                    return SYS_SOCK_READ_TIMEDOUT;
                }
            }
            else if ( status < 0 ) {
                if ( errno == EINTR ) {
                    continue;
                }
                else {
                    return SYS_SOCK_READ_ERR - errno;
                }
            }
        }
        nbytes = read( sock, ( void * ) tmpPtr, toRead );
#endif
        if ( nbytes <= 0 ) {
            if ( errno == EINTR ) {
                /* interrupted */
                errno = 0;
                nbytes = 0;
            }
            else {
                break;
            }
        }

        toRead -= nbytes;
        tmpPtr += nbytes;
        if ( bytesRead != NULL ) {
            *bytesRead += nbytes;
        }
    }
    return len - toRead;
}

int
myWrite( int sock, void *buf, int len,
         int *bytesWritten ) {
    int nbytes;
    int toWrite;
    char *tmpPtr;

    toWrite = len;
    tmpPtr = ( char * ) buf;

    if ( bytesWritten != NULL ) {
        *bytesWritten = 0;
    }

    while ( toWrite > 0 ) {
#ifdef _WIN32
        if ( irodsDescType == SOCK_TYPE ) {
            nbytes = send( sock, tmpPtr, toWrite, 0 );
        }
        else {
            nbytes = write( sock, ( void * ) tmpPtr, toWrite );
        }
#else
        nbytes = write( sock, ( void * ) tmpPtr, toWrite );
#endif
        if ( nbytes <= 0 ) {
            if ( errno == EINTR ) {
                /* interrupted */
                errno = 0;
                nbytes = 0;
            }
            else {
                break;
            }
        }
        toWrite -= nbytes;
        tmpPtr += nbytes;
        if ( bytesWritten != NULL ) {
            *bytesWritten += nbytes;
        }
    }
    return len - toWrite;
}

// =-=-=-=-=-=-=-
//
irods::error readVersion(
    irods::network_object_ptr _ptr,
    version_t**         _version ) {
    // =-=-=-=-=-=-=-
    // init timval struct for header call
    struct timeval tv;
    tv.tv_sec = READ_VERSION_TOUT_SEC;
    tv.tv_usec = 0;

    // =-=-=-=-=-=-=-
    // call interface to read message header
    msgHeader_t myHeader;
    irods::error ret = readMsgHeader( _ptr, &myHeader, &tv );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // call interface to read message body
    bytesBuf_t inputStructBBuf, bsBBuf, errorBBuf;
    memset( &bsBBuf, 0, sizeof( bytesBuf_t ) );
    ret = readMsgBody( _ptr, &myHeader, &inputStructBBuf, &bsBBuf,
                       &errorBBuf, XML_PROT, NULL );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // basic error checking of message type
    if ( strcmp( myHeader.type, RODS_VERSION_T ) != 0 ) {
        if ( inputStructBBuf.buf != NULL ) {
            free( inputStructBBuf.buf );
        }
        if ( bsBBuf.buf != NULL ) {
            free( bsBBuf.buf );
        }
        if ( errorBBuf.buf != NULL ) {
            free( errorBBuf.buf );
        }
        std::stringstream msg;
        msg << "wrong msg type ["
            << myHeader.type
            << " expected ["
            << RODS_VERSION_T
            << "]";
        return ERROR( SYS_HEADER_TYPE_LEN_ERR, msg.str() );
    }

    // =-=-=-=-=-=-=-
    // check length of byte stream buffer, should be 0
    if ( myHeader.bsLen != 0 ) {
        if ( bsBBuf.buf != NULL ) {
            free( bsBBuf.buf );
        }
        rodsLog( LOG_NOTICE, "readVersion: myHeader.bsLen = %d is not 0",
                 myHeader.bsLen );
    }

    // =-=-=-=-=-=-=-
    // check length of error buffer, should be 0
    if ( myHeader.errorLen != 0 ) {
        if ( errorBBuf.buf != NULL ) {
            free( errorBBuf.buf );
        }
        rodsLog( LOG_NOTICE, "readVersion: myHeader.errorLen = %d is not 0",
                 myHeader.errorLen );
    }

    // =-=-=-=-=-=-=-
    // bounds check message size
    if ( myHeader.msgLen > ( int ) sizeof( version_t ) * 2 || myHeader.msgLen <= 0 ) {
        if ( inputStructBBuf.buf != NULL ) {
            free( inputStructBBuf.buf );
        }
        std::stringstream msg;
        msg << "header length is not within bounds: "
            << myHeader.msgLen;
        return ERROR( SYS_HEADER_READ_LEN_ERR, msg.str() );
    }

    // =-=-=-=-=-=-=-
    // unpack the message, always use XML for this message type
    int status = unpackStruct(
                     inputStructBBuf.buf,
                     ( void** )( _version ),
                     "Version_PI",
                     RodsPackTable,
                     XML_PROT );
    free( inputStructBBuf.buf );
    if ( status < 0 ) {
        rodsLogError( LOG_NOTICE, status,
                      "readVersion:unpackStruct error. status = %d",
                      status );
    }

    return CODE( status );

} // readVersion

int
rodsSetSockOpt( int sock, int windowSize ) {
    int status;
    int savedStatus = 0;
    int temp;
#ifndef _WIN32
    struct linger linger;
#endif

    if ( windowSize <= 0 ) {
        windowSize = SOCK_WINDOW_SIZE;
    }
    else if ( windowSize < MIN_SOCK_WINDOW_SIZE ) {
        rodsLog( LOG_NOTICE,
                 "rodsSetSockOpt: the input windowSize %d is too small, default to %d",
                 windowSize, MIN_SOCK_WINDOW_SIZE );
        windowSize = MIN_SOCK_WINDOW_SIZE;
    }
    else if ( windowSize > MAX_SOCK_WINDOW_SIZE ) {
        rodsLog( LOG_NOTICE,
                 "rodsSetSockOpt: the input windowSize %d is too large, default to %d",
                 windowSize, MAX_SOCK_WINDOW_SIZE );
        windowSize = MAX_SOCK_WINDOW_SIZE;
    }

#ifdef _WIN32
    status = setsockopt( sock, SOL_SOCKET, SO_SNDBUF,
                         ( char* )&windowSize, sizeof( windowSize ) );
    if ( status < 0 ) {
        savedStatus = status;
    }

    status = setsockopt( sock, SOL_SOCKET, SO_RCVBUF,
                         ( char* )&windowSize, sizeof( windowSize ) );
    if ( status < 0 ) {
        savedStatus = status;
    }

    temp = 1;
    status = setsockopt( sock, IPPROTO_TCP, TCP_NODELAY,
                         ( char* )&temp, sizeof( temp ) );
    if ( status < 0 ) {
        savedStatus = status;
    }
#else
    status = setsockopt( sock, SOL_SOCKET, SO_SNDBUF,
                         &windowSize, sizeof( windowSize ) );
    if ( status < 0 ) {
        savedStatus = status;
    }

    status = setsockopt( sock, SOL_SOCKET, SO_RCVBUF,
                         &windowSize, sizeof( windowSize ) );
    if ( status < 0 ) {
        savedStatus = status;
    }

    temp = 1;
    status = setsockopt( sock, IPPROTO_TCP, TCP_NODELAY,
                         &temp, sizeof( temp ) );
    if ( status < 0 ) {
        savedStatus = status;
    }

    /* reuse the socket. Otherwise will be kept for 2-4 minutes */
    status = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR,
                         &temp, sizeof( temp ) );
    if ( status < 0 ) {
        savedStatus = status;
    }

    /* keep connection alive */
    temp = 1;
    status = setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE, &temp, sizeof( temp ) );
    if ( status < 0 ) {
        savedStatus = status;
    }

    linger.l_onoff = 1;
    linger.l_linger = 5;
    status = setsockopt( sock, SOL_SOCKET, SO_LINGER, &linger, sizeof( linger ) );
    if ( status < 0 ) {
        savedStatus = status;
    }
#endif

    return savedStatus;
}

int
connectToRhostPortal( char *rodsHost, int rodsPort,
                      int cookie, int windowSize ) {
    int status, nbytes;
    struct sockaddr_in remoteAddr;
    int sock, myCookie;

    status = setSockAddr( &remoteAddr, rodsHost, rodsPort );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "connectToRhostPortal: setSockAddr error for %s, errno = %d",
                 rodsHost, errno );
        return status;
    }
    /* set timeout 11/13/2009 */
    sock = connectToRhostWithRaddr( &remoteAddr, windowSize, 1 );

    if ( sock < 0 ) {
        rodsLog( LOG_ERROR,
                 "connectToRhostPortal: connectTo Rhost %s port %d error, status = %d",
                 rodsHost, rodsPort, sock );
        return sock;
    }

    myCookie = htonl( cookie );
    nbytes = myWrite( sock, &myCookie, sizeof( myCookie ), NULL );

    if ( nbytes != sizeof( myCookie ) ) {
        CLOSE_SOCK( sock );
        return SYS_PORT_COOKIE_ERR;
    }

    return sock;
}

int
connectToRhost( rcComm_t *conn, int connectCnt, int reconnFlag ) {
    int status;
    conn->sock = connectToRhostWithRaddr( &conn->remoteAddr,
                                          conn->windowSize, 1 );
    if ( conn->sock < 0 ) {
        rodsLogError( LOG_NOTICE, conn->sock,
                      "connectToRhost: connect to host %s on port %d failed, status = %d",
                      conn->host, conn->portNum, conn->sock );
        return conn->sock;
    }

    setConnAddr( conn );
    status = sendStartupPack( conn, connectCnt, reconnFlag );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "connectToRhost: sendStartupPack to %s failed, status = %d",
                      conn->host, status );
        close( conn->sock );
        return status;
    }

    // =-=-=-=-=-=-=-
    // create a network object
    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( conn, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // if the client requests the connection negotiation then wait for a
    // response here from the Agent
    if ( irods::do_client_server_negotiation_for_client() ) {
        // =-=-=-=-=-=-=-
        // politely do the negotiation
        std::string results;
        ret = irods::client_server_negotiation_for_client(
                  net_obj,
                  results );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }


        // =-=-=-=-=-=-=-
        // enable SSL if requested
        // NOTE:: this is disabled in rcDisconnect if the conn->ssl_on flag is set
        if ( irods::CS_NEG_USE_SSL == irods::CS_NEG_FAILURE ) {
            printf( "connectToRhost - failed in client-server negotiations\n" );
        }

        // =-=-=-=-=-=-=-
        // copy results to connection for network object factory
        snprintf( conn->negotiation_results, MAX_NAME_LEN, "%s", results.c_str() );
    }

    ret = readVersion( net_obj, &conn->svrVersion );
    if ( !ret.ok() ) {
        rodsLogError( LOG_ERROR, ret.code(),
                      "connectToRhost: readVersion to %s failed, status = %d",
                      conn->host, ret.code() );
        close( conn->sock );
        return ret.code();
    }

    if ( conn->svrVersion->status < 0 ) {
        rodsLogError( LOG_ERROR, conn->svrVersion->status,
                      "connectToRhost: error returned from host %s status = %d",
                      conn->host, conn->svrVersion->status );
        if ( conn->svrVersion->status == SYS_EXCEED_CONNECT_CNT )
            rodsLog( LOG_ERROR,
                     "It is likely %s is a localhost but not recognized by this server. A line can be added to the server/config/irodsHost file to fix the problem", conn->host );
        close( conn->sock );
        return conn->svrVersion->status;
    }

    // =-=-=-=-=-=-=-
    // call initialization for network plugin as negotiated
    irods::network_object_ptr new_net_obj;
    ret = irods::network_factory( conn, new_net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // get rods env to pass to client start for policy decisions
    rodsEnv rods_env;
    status = getRodsEnv( &rods_env );

    ret = sockClientStart( new_net_obj, &rods_env );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    new_net_obj->to_client( conn );

    return 0;

} // connectToRhost

int
connectToRhostWithRaddr( struct sockaddr_in *remoteAddr, int windowSize,
                         int timeoutFlag ) {
    int sock;
    int status;

    sock = socket( AF_INET, SOCK_STREAM, 0 );

    if ( sock < 0 ) {  /* the ol' one-two */
        sock = socket( AF_INET, SOCK_STREAM, 0 );
    }

    if ( sock < 0 ) {
        rodsLog( LOG_NOTICE,
                 "connectToRhostWithRaddr() - socket() failed: errno=%d",
                 errno );
        return USER_SOCK_OPEN_ERR - errno;
    }

    if ( timeoutFlag > 0 ) {
        status = connectToRhostWithTout( sock, ( struct sockaddr * ) remoteAddr );
    }
    else {
        status = connect( sock, ( struct sockaddr * ) remoteAddr,
                          sizeof( struct sockaddr ) );
    }

    if ( status < 0 ) {
        if ( status == -1 ) {
            status = USER_SOCK_CONNECT_ERR - errno;
        }
#ifdef _WIN32
        closesocket( sock );
#else
        close( sock );
#endif /* WIN32 */
        return status;
    }

    rodsSetSockOpt( sock, windowSize );

#ifdef PORTNAME_solaris
    flag = fcntl( sock, F_GETFL );
    if ( flag & O_NONBLOCK ) {
        fcntl( sock, F_SETFL, ( flag & ( ~O_NONBLOCK ) ) );
    }
#endif

    return sock;

}

#ifdef _WIN32
void CALLBACK my_timeout_handler( UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2 ) {
    win_connect_timeout = 1;
}
#endif

int
connectToRhostWithTout( int sock, struct sockaddr *sin ) {
    int timeoutCnt = 0;

#ifdef _WIN32
    // A Windows console app has very limited timeout functionality.
    // An pseudo timeout is implemented.
    int status = 0;

    while ( ( timeoutCnt < MAX_CONN_RETRY_CNT ) && ( !win_connect_timeout ) ) {
        if ( ( status = connect( sock, sin, sizeof( struct sockaddr ) ) ) < 0 ) {
            timeoutCnt ++;
            rodsSleep( 0, 200000 );
        }
        else {
            break;
        }
    }
    if ( status != 0 ) {
        return USER_SOCK_CONNECT_TIMEDOUT;
    }

    return 0;

#else
    /* redo the timeout using select */
    /* Set non-blocking */
    long arg = fcntl( sock, F_GETFL, NULL );
    if ( arg < 0 ) {
        rodsLog( LOG_ERROR,
                 "connectToRhostWithTout: fcntl F_GETFL error, errno = %d", errno );
        return USER_SOCK_CONNECT_ERR;
    }
    arg |= O_NONBLOCK;
    if ( fcntl( sock, F_SETFL, arg ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "connectToRhostWithTout: fcntl F_SETFL error, errno = %d", errno );
        return USER_SOCK_CONNECT_ERR;
    }

    int status = 0;
    while ( timeoutCnt < MAX_CONN_RETRY_CNT ) {
        status = connect( sock, sin, sizeof( struct sockaddr ) );
        if ( status >= 0 ) {
            break;
        }
        if ( errno == EISCONN ) {
            /* already connected. seen this error on AIX */
            status = 0;
            break;
        }
        if ( errno == EINPROGRESS || errno == EINTR ) {
            struct timeval tv;
            tv.tv_sec = CONNECT_TIMEOUT_TIME;
            tv.tv_usec = 0;
            fd_set myset;
            FD_ZERO( &myset );
            FD_SET( sock, &myset );
            status = select( sock + 1, NULL, &myset, NULL, &tv );
            if ( status < 0 ) {
                if ( errno != EINTR ) {
                    rodsLog( LOG_NOTICE,
                             "connectToRhostWithTout: connect error, errno = %d",
                             errno );
                    timeoutCnt++;
                }
                continue;
            }
            else if ( status > 0 ) {
                int myval;
#if defined(aix_platform)
                socklen_t mylen = sizeof( int );
#else
                uint mylen = sizeof( int );
#endif
                if ( getsockopt( sock, SOL_SOCKET, SO_ERROR, ( void* )( &myval ),
                                 &mylen ) < 0 ) {
                    rodsLog( LOG_ERROR,
                             "connectToRhostWithTout: getsockopt error, errno = %d",
                             errno );
                    return USER_SOCK_CONNECT_ERR - errno;
                }
                /* Check the returned value */
                if ( myval ) {
                    rodsLog( LOG_NOTICE,
                             "connectToRhostWithTout: connect error, errno = %d",
                             myval );
                    timeoutCnt++;
                    status = USER_SOCK_CONNECT_ERR - myval;
                    continue;
                }
                else {
                    break;
                }
            }
            else {
                /* timed out */
                status = USER_SOCK_CONNECT_TIMEDOUT;
                break;
            }
        }
        else {
            rodsLog( LOG_NOTICE,
                     "connectToRhostWithTout: connect error, errno = %d", errno );
            timeoutCnt++;
            status = USER_SOCK_CONNECT_ERR - errno;
            continue;
        }
    }

    if ( status < 0 ) {
        if ( status == -1 ) {
            return USER_SOCK_CONNECT_ERR;
        }
        else {
            return status;
        }
    }

    /* Set to blocking again */
    if ( ( arg = fcntl( sock, F_GETFL, NULL ) ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "connectToRhostWithTout: fcntl F_GETFL error, errno = %d", errno );
        return USER_SOCK_CONNECT_ERR;
    }

    arg &= ( ~O_NONBLOCK );
    if ( fcntl( sock, F_SETFL, arg ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "connectToRhostWithTout: fcntl F_SETFL error, errno = %d", errno );
        return USER_SOCK_CONNECT_ERR;
    }
    return status;

#endif
}

int
setConnAddr( rcComm_t *conn ) {
    int status1, status2;

    status1 = setLocalAddr( conn->sock, &conn->localAddr );

    status2 = setRemoteAddr( conn->sock, &conn->remoteAddr );

    if ( status1 < 0 ) {
        return status1;
    }
    else if ( status2 < 0 ) {
        return status2;
    }
    else {
        return 0;
    }
}

int
setRemoteAddr( int sock, struct sockaddr_in *remoteAddr ) {
#if defined(aix_platform)
    socklen_t       laddrlen = sizeof( struct sockaddr );
#elif defined(windows_platform)
    int laddrlen = sizeof( struct sockaddr );
#else
    uint         laddrlen = sizeof( struct sockaddr );
#endif

    /* fill in the server address. This is for case where the conn->host
     * is not a real host but an address that select a host from a pool
     * of hosts */
    if ( getpeername( sock, ( struct sockaddr * ) remoteAddr,
                      &laddrlen ) < 0 ) {
        rodsLog( LOG_NOTICE,
                 "setLocalAddr() -- getpeername() failed: errno=%d", errno );
        return USER_RODS_HOSTNAME_ERR;
    }

    return 0;
}

int
setLocalAddr( int sock, struct sockaddr_in *localAddr ) {
#if defined(aix_platform)
    socklen_t       laddrlen = sizeof( struct sockaddr );
#elif defined(windows_platform)
    int         laddrlen = sizeof( struct sockaddr );
#else
    uint         laddrlen = sizeof( struct sockaddr );
#endif


    /* fill in the client address */
    if ( getsockname( sock, ( struct sockaddr * ) localAddr,
                      &laddrlen ) < 0 ) {
        rodsLog( LOG_NOTICE,
                 "setLocalAddr() -- getsockname() failed: errno=%d",
                 errno );
        return USER_RODS_HOSTNAME_ERR;
    }
    return ntohs( localAddr->sin_port );
}

int
sendStartupPack( rcComm_t *conn, int connectCnt, int reconnFlag ) {
    startupPack_t startupPack;
    int status;
    char *tmpStr;
    bytesBuf_t *startupPackBBuf = NULL;


    /* setup the startup pack */

    startupPack.irodsProt  = conn->irodsProt;
    startupPack.connectCnt = connectCnt;
    startupPack.reconnFlag = reconnFlag;

    rstrcpy( startupPack.proxyUser,      conn->proxyUser.userName,  NAME_LEN );
    rstrcpy( startupPack.proxyRodsZone,  conn->proxyUser.rodsZone,  NAME_LEN );
    rstrcpy( startupPack.clientUser,     conn->clientUser.userName, NAME_LEN );
    rstrcpy( startupPack.clientRodsZone, conn->clientUser.rodsZone, NAME_LEN );

    rstrcpy( startupPack.relVersion, RODS_REL_VERSION,  NAME_LEN );
    rstrcpy( startupPack.apiVersion, RODS_API_VERSION,  NAME_LEN );

    if ( ( tmpStr = getenv( SP_OPTION ) ) != NULL ) {
        rstrcpy( startupPack.option, tmpStr, NAME_LEN );
    }
    else {
        startupPack.option[0] = '\0';
    }

    // =-=-=-=-=-=-=-
    // if the advanced negotiation is requested from the irodsEnv,
    // tack those results onto the startup pack option string
    rodsEnv rods_env;
    status = getRodsEnv( &rods_env );

    if ( status >= 0 && strlen( rods_env.rodsClientServerNegotiation ) > 0 ) {
        strncat( startupPack.option,
                 rods_env.rodsClientServerNegotiation,
                 strlen( rods_env.rodsClientServerNegotiation ) );
    }

    /* always use XML_PROT for the startupPack */
    status = packStruct( ( void * ) &startupPack, &startupPackBBuf,
                         "StartupPack_PI", RodsPackTable, 0, XML_PROT );
    if ( status < 0 ) {
        rodsLogError( LOG_NOTICE, status,
                      "sendStartupPack: packStruct error, status = %d", status );
        return status;
    }

    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( conn, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        freeBBuf( startupPackBBuf );
        return ret.code();
    }

    ret = sendRodsMsg(
              net_obj,
              RODS_CONNECT_T,
              startupPackBBuf,
              NULL, NULL, 0,
              XML_PROT );
    freeBBuf( startupPackBBuf );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    return ret.code();

} // sendStartupPack


irods::error sendVersion(
    irods::network_object_ptr _ptr,
    int                 versionStatus,
    int                 reconnPort,
    char*               reconnAddr,
    int                 cookie ) {
    version_t myVersion;
    int status;
    bytesBuf_t *versionBBuf = NULL;


    /* setup the version struct */

    memset( &myVersion, 0, sizeof( myVersion ) );
    myVersion.status = versionStatus;
    rstrcpy( myVersion.relVersion, RODS_REL_VERSION,  NAME_LEN );
    rstrcpy( myVersion.apiVersion, RODS_API_VERSION,  NAME_LEN );
    if ( reconnAddr != NULL ) {
        myVersion.reconnPort = reconnPort;
        rstrcpy( myVersion.reconnAddr, reconnAddr, LONG_NAME_LEN );
        myVersion.cookie = cookie;
    }
    else {
        // =-=-=-=-=-=-=-
        // super secret jargon irods detection
        // sshhhhhhh...
        myVersion.cookie = 400;

    }

    /* alway use XML for version */
    status = packStruct( ( char * ) &myVersion, &versionBBuf,
                         "Version_PI", RodsPackTable, 0, XML_PROT );
    if ( status < 0 ) {
        return ERROR( status, "packStruct error" );
    }

    irods::error ret = sendRodsMsg(
                           _ptr,
                           RODS_VERSION_T,
                           versionBBuf,
                           NULL, NULL, 0,
                           XML_PROT );
    freeBBuf( versionBBuf );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    return SUCCESS();

} // sendVersion


irods::error sendRodsMsg(
    irods::network_object_ptr _ptr,
    char*               _msg_type,
    bytesBuf_t*         _msg_buf,
    bytesBuf_t*         _bs_buf,
    bytesBuf_t*         _error_buf,
    int                 _int_info,
    irodsProt_t         _protocol ) {
    // =-=-=-=-=-=-=-
    // resolve a network interface plugin from the
    // network object
    irods::plugin_ptr p_ptr;
    irods::error ret_err = _ptr->resolve( irods::NETWORK_INTERFACE, p_ptr );
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to resolve network interface", ret_err );

    }

    // =-=-=-=-=-=-=-
    // make the call to the "write body" interface
    irods::first_class_object_ptr ptr = boost::dynamic_pointer_cast< irods::first_class_object >( _ptr );
    irods::network_ptr            net = boost::dynamic_pointer_cast< irods::network >( p_ptr );
    ret_err = net->call< char*, bytesBuf_t*, bytesBuf_t*, bytesBuf_t*, int, irodsProt_t >(
                  irods::NETWORK_OP_WRITE_BODY,
                  ptr,
                  _msg_type,
                  _msg_buf,
                  _bs_buf,
                  _error_buf,
                  _int_info,
                  _protocol );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        return PASSMSG( "failed to call 'write body'", ret_err );

    }
    else {
        return CODE( ret_err.code() );

    }

    return SUCCESS();

} // sendRodsMsg

int
rodsSleep( int sec, int microSec ) {
    struct timeval sleepTime;

    sleepTime.tv_sec = sec;
    sleepTime.tv_usec = microSec;

    select( 0, NULL, NULL, NULL, &sleepTime );

    return 0;
}


char *
rods_inet_ntoa( struct in_addr in ) {
    char *clHostAddr;

    clHostAddr = inet_ntoa( in );

    if ( isLoopbackAddress( clHostAddr ) ||
            strcmp( clHostAddr, "0.0.0.0" ) == 0 ) { /* localhost */
        char sb[LONG_NAME_LEN];
        struct hostent *phe;

        if ( gethostname( sb, sizeof( sb ) ) != 0 ) {
            return clHostAddr;
        }
        if ( ( phe = gethostbyname( sb ) ) == NULL ) {
            return clHostAddr;
        }
        clHostAddr = inet_ntoa( *( struct in_addr* ) phe->h_addr );
    }

    return clHostAddr;
}

// =-=-=-=-=-=-=-
//
irods::error readReconMsg(
    irods::network_object_ptr _ptr,
    reconnMsg_t**       _msg ) {
    int status;
    msgHeader_t myHeader;
    bytesBuf_t inputStructBBuf, bsBBuf, errorBBuf;
    // =-=-=-=-=-=-=-
    //
    irods::error ret = readMsgHeader( _ptr, &myHeader, NULL );
    if ( !ret.ok() ) {
        return PASSMSG( "read msg header error", ret );
    }

    memset( &bsBBuf, 0, sizeof( bytesBuf_t ) );
    ret = readMsgBody(
              _ptr,
              &myHeader,
              &inputStructBBuf,
              &bsBBuf,
              &errorBBuf,
              XML_PROT,
              NULL );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    /* some sanity check */

    if ( strcmp( myHeader.type, RODS_RECONNECT_T ) != 0 ) {
        if ( inputStructBBuf.buf != NULL ) {
            free( inputStructBBuf.buf );
        }
        if ( bsBBuf.buf != NULL ) {
            free( bsBBuf.buf );
        }
        if ( errorBBuf.buf != NULL ) {
            free( errorBBuf.buf );
        }
        std::stringstream msg;
        msg << "wrong msg type ["
            << myHeader.type
            << "] expected ["
            << RODS_CONNECT_T
            << "]";
        return ERROR( SYS_HEADER_TYPE_LEN_ERR, msg.str() );
    }

    if ( myHeader.bsLen != 0 ) {
        if ( bsBBuf.buf != NULL ) {
            free( bsBBuf.buf );
        }
        rodsLog( LOG_NOTICE, "readReconMsg: myHeader.bsLen = %d is not 0",
                 myHeader.bsLen );
    }

    if ( myHeader.errorLen != 0 ) {
        if ( errorBBuf.buf != NULL ) {
            free( errorBBuf.buf );
        }
        rodsLog( LOG_NOTICE,
                 "readReconMsg: myHeader.errorLen = %d is not 0",
                 myHeader.errorLen );
    }

    if ( myHeader.msgLen <= 0 ) {
        if ( inputStructBBuf.buf != NULL ) {
            free( inputStructBBuf.buf );
        }
        rodsLog( LOG_NOTICE,
                 "readReconMsg: problem with myHeader.msgLen = %d",
                 myHeader.msgLen );
        std::stringstream msg;
        msg << "message length is invalid: "
            << myHeader.msgLen;
        return ERROR( SYS_HEADER_READ_LEN_ERR, msg.str() );
    }

    /* always use XML_PROT for the startup pack */
    status = unpackStruct(
                 inputStructBBuf.buf,
                 ( void** )( _msg ),
                 "ReconnMsg_PI",
                 RodsPackTable,
                 XML_PROT );
    clearBBuf( &inputStructBBuf );
    if ( status < 0 ) {
        rodsLogError( LOG_NOTICE,  status,
                      "readReconMsg:unpackStruct error. status = %d",
                      status );
    }

    return CODE( status );
}

// =-=-=-=-=-=-=-
// interface for requesting a reconnection
irods::error sendReconnMsg(
    irods::network_object_ptr _ptr,
    reconnMsg_t*        _msg ) {
    // =-=-=-=-=-=-=-
    // trap invalid param
    if ( _msg == NULL ) {
        return ERROR( USER__NULL_INPUT_ERR, "null msg buf" );
    }

    // =-=-=-=-=-=-=-
    // pack outgoing message - alway use XML for version
    bytesBuf_t* recon_buf = NULL;
    int status = packStruct(
                     static_cast<void*>( _msg ),
                     &recon_buf,
                     "ReconnMsg_PI",
                     RodsPackTable,
                     0, XML_PROT );
    if ( status < 0 ) {
        return ERROR( status, "failed to pack struct" );
    }

    // =-=-=-=-=-=-=-
    // pack outgoing message - alway use XML for version
    irods::error ret = sendRodsMsg(
                           _ptr,
                           RODS_RECONNECT_T,
                           recon_buf,
                           NULL,
                           NULL,
                           0,
                           XML_PROT );
    freeBBuf( recon_buf );
    if ( !ret.ok() ) {
        rodsLogError( LOG_ERROR, status,
                      "sendReconnMsg: sendRodsMsg of reconnect msg failed, status = %d",
                      status );
    }

    return CODE( status );

} // sendReconnMsg

int svrSwitchConnect(
    rsComm_t *rsComm ) {
    // =-=-=-=-=-=-=-
    // construct a network object from the comm
    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( rsComm, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    if ( rsComm->reconnectedSock > 0 ) {
        if ( rsComm->clientState == RECEIVING_STATE ) {
            reconnMsg_t reconnMsg;
            bzero( &reconnMsg, sizeof( reconnMsg ) );
            sendReconnMsg( net_obj, &reconnMsg );
            rsComm->clientState = PROCESSING_STATE;
        }
        close( rsComm->sock );
        rsComm->sock = rsComm->reconnectedSock;
        rsComm->reconnectedSock = 0;
        rodsLog( LOG_NOTICE,
                 "svrSwitchConnect: Switch connection" );
        return 1;
    }
    else {
        return 0;
    }
}

int cliSwitchConnect( rcComm_t *conn ) {
    // =-=-=-=-=-=-=-
    // construct a network object from the comm
    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( conn, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    if ( conn->reconnectedSock > 0 ) {
        if ( conn->agentState == RECEIVING_STATE ) {
            reconnMsg_t reconnMsg;
            bzero( &reconnMsg, sizeof( reconnMsg ) );
            sendReconnMsg( net_obj, &reconnMsg );
            conn->agentState = PROCESSING_STATE;
        }
        close( conn->sock );
        conn->sock = conn->reconnectedSock;
        conn->reconnectedSock = 0;
        fprintf( stderr,
                 "The client/server socket connection has been renewed\n" );
        return 1;
    }
    else {
        return 0;
    }
}

int
addUdpPortToPortList( portList_t *thisPortList, int udpport ) {
    /* put udpport in the upper 16 bits of portNum */
    thisPortList->portNum |= udpport << 16;
    return 0;
}

int
getUdpPortFromPortList( portList_t *thisPortList ) {
    int udpport = 0;
    udpport = ( thisPortList->portNum & 0xffff0000 ) >> 16;
    return udpport;
}

int
getTcpPortFromPortList( portList_t *thisPortList ) {
    return thisPortList->portNum & 0xffff;
}

int
addUdpSockToPortList( portList_t *thisPortList, int udpsock ) {
    /* put udpport in the upper 16 bits of portNum */
    thisPortList->sock |= udpsock << 16;
    return 0;
}

int
getUdpSockFromPortList( portList_t *thisPortList ) {
    int udpsock = 0;
    udpsock = ( thisPortList->sock & 0xffff0000 ) >> 16;
    return udpsock;
}

int
getTcpSockFromPortList( portList_t *thisPortList ) {
    return thisPortList->sock & 0xffff;
}

int
isReadMsgError( int status ) {
    int irodsErr = getIrodsErrno( status );

    if ( irodsErr == SYS_READ_MSG_BODY_LEN_ERR ||
            irodsErr == SYS_HEADER_READ_LEN_ERR ||
            irodsErr == SYS_HEADER_WRITE_LEN_ERR ) {
        return 1;
    }
    else {
        return 0;
    }
}

int
redirectConnToRescSvr( rcComm_t **conn, dataObjInp_t *dataObjInp,
                       rodsEnv *myEnv, int reconnFlag ) {
    int status;
    char *outHost = NULL;

    if ( dataObjInp->oprType == PUT_OPR ) {
        status = rcGetHostForPut( *conn, dataObjInp, &outHost );
    }
    else if ( dataObjInp->oprType == GET_OPR ) {
        status = rcGetHostForGet( *conn, dataObjInp, &outHost );
    }
    else {
        rodsLog( LOG_NOTICE,
                 "redirectConnToRescSvr: Unknown oprType %d\n",
                 dataObjInp->oprType );
        return 0;
    }

    if ( status < 0 || outHost == NULL || strcmp( outHost, THIS_ADDRESS ) == 0 ) {
        return status;
    }

    status = rcReconnect( conn, outHost, myEnv, reconnFlag );
    return status;
}

int
rcReconnect( rcComm_t **conn, char *newHost, rodsEnv *myEnv, int reconnFlag ) {
    int status;
    rcComm_t *newConn = NULL;
    rErrMsg_t errMsg;

    bzero( &errMsg, sizeof( errMsg ) );

    newConn =  rcConnect( newHost, myEnv->rodsPort, myEnv->rodsUserName,
                          myEnv->rodsZone, reconnFlag, &errMsg );

    if ( newConn != NULL ) {
        status = clientLogin( newConn );
        if ( status != 0 ) {
            rcDisconnect( newConn );
            return status;
        }
        rcDisconnect( *conn );
        *conn = newConn;
        return 0;
    }
    else {
        return errMsg.status;
    }
}

int
mySockClose( int sock ) {
    int status;
#ifdef _WIN32
    status = closesocket( sock );
#else   /* _WIN32 */
#if defined(solaris_platform) || defined(linux_platform) || defined(osx_platform)
    /* For reason I do not completely understand, if I do a socket write and
     * then a socket close immediately, the receiver at the other end can
     * get a errno 104 (reset by peer) and does no always get the sent msg
     * even though setting SO_LINGER. Making a shutdown call to shutdown
     * the send channel seems to do the job.
     */
    shutdown( sock, SHUT_WR );
#endif
    status = close( sock );
#endif
    return status;
}
