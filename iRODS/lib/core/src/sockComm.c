/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sockComm.c - sock communication routines 
 */

#include "sockComm.h"
#include "rcMisc.h"
#include "rcGlobalExtern.h"
#include "miscServerFunct.h"
#include "getHostForPut.h"
#include "getHostForGet.h"
#ifdef RBUDP_TRANSFER
#include "QUANTAnet_rbudpBase_c.h"
#endif  /* RBUDP_TRANSFER */

#ifdef windows_platform
#include "irodsntutil.h"
#endif

#ifdef _WIN32
#include <mmsystem.h>
int win_connect_timeout;
MMRESULT win_connect_timer_id;
#endif

#ifndef _WIN32

#include <setjmp.h>
jmp_buf Jcenv;

#if 0 // JMC - UNUSED
void
connToutHandler (int sig)
{
    alarm (0);
    longjmp (Jcenv, 1);
}
#endif // JMC - UNUSED
#endif  /* _WIN32 */

#include "eirods_stacktrace.h"

#ifdef USE_BOOST_ASIO

// =-=-=-=-=-=-=-
// open socket for incoming connection
int sockOpenForInConn ( rsComm_t* _rsComm, int* _portNum, char** _addr, int _proto ) {
    // =-=-=-=-=-=-=-
    // get rid of alot of namespace references
    using namespace boost::asio::ip;

    // =-=-=-=-=-=-=-
    // return value
    int status = 0;
 
    // =-=-=-=-=-=-=-
    // value check incoming protocol
    if( proto != SOCK_DGRAM && proto != SOCK_STREAM ) {
        rodsLog( LOG_ERROR, "sockOpenForInConn() -- invalid input protocol %d", proto );
        return SYS_INVALID_PROTOCOL_TYPE;
    } // if proto

    // =-=-=-=-=-=-=-
    // based on protocol, create either tcp or udp flavored socket wrapper
    if( SOCK_STREAM == _proto ) {
        _rsComm->sock =  new socket_wrapper_tcp;
    } else if( SOCK_DGRAM == _proto ) {
        _rsComm->sock = new socket_wrapper_udp;
        // FIXME rodsSetSockOpt (sock, rsComm->windowSize);
    } // if/else proto
    
    // =-=-=-=-=-=-=-
    // trap failed socket creation
    if( !_rsComm->sock ) {
        status = SYS_SOCK_OPEN_ERR - errno;
        rodsLogError( LOG_NOTICE, status, "sockOpenForInConn: open socket error. status = %d", status );
        return status;
    } // if !sock

    // =-=-=-=-=-=-=-
    // if portNum is <= 0 and env svrPortRangeStart is set, pick a port
    // in the range.
    char* tmpPtr = 0;
    int svrPortRangeStart = 0;
    int svrPortRangeEnd   = 0;

    if( *portNum <= 0 && ( tmpPtr = getenv( "svrPortRangeStart" ) ) != NULL ) {
        int portRangeCount = 0;
        int bindCnt        = 0;
        int myPortNum      = 0;

        svrPortRangeStart = atoi( tmpPtr );

        // =-=-=-=-=-=-=-
        // test for env vars which describe the valid port range to scan
        if( ( tmpPtr = getenv( "svrPortRangeEnd" ) ) != NULL ) { 
            svrPortRangeEnd = atoi( tmpPtr );
            if ( svrPortRangeEnd < svrPortRangeStart ) {
                rodsLog( LOG_ERROR, "sockOpenForInConn: PortRangeStart %d > PortRangeEnd %d", 
                         svrPortRangeStart, svrPortRangeEnd);
                svrPortRangeEnd = svrPortRangeStart + DEF_NUMBER_SVR_PORT - 1;
            }
        } else {
            svrPortRangeEnd = svrPortRangeStart + DEF_NUMBER_SVR_PORT - 1;
        }
        
        portRangeCount = svrPortRangeEnd   - svrPortRangeStart + 1;
        myPortNum      = svrPortRangeStart + random() % portRangeCount;

        // =-=-=-=-=-=-=-
        // loop through port range and find one that sticks
        while( bindCnt < portRangeCount ) {
            if( myPortNum > svrPortRangeEnd ) {
                myPortNum = svrPortRangeStart;
            }

            if( _rsComm->sock->open( myPortNum, "" ) )  {
                *portNum = myPortNum;
                rodsLog( LOG_DEBUG, "sockOpenForInConn: port number = %d", myPortNum );
                break;
            } // if success
            bindCnt ++;
            myPortNum ++;
        } // while bindCnd < portRangeCount
    } else {
        // =-=-=-=-=-=-=-
        // else use the provided port number parameter
        status = _rsComm->sock->open( _port, "" );
    } // else port > 0

    // =-=-=-=-=-=-=-
    // error out if unsuccessful
    if( status < 0 ) { 
        status = SYS_SOCK_BIND_ERR - errno;
        rodsLog( LOG_NOTICE, "sockOpenForInConn: bind socket error. portNum = %d, errno = %d", 
                 *portNum, errno );
        return ( status );
    } // if status

    if( addr != NULL ) {
        struct sockaddr_in sin;
#if defined(aix_platform)
        socklen_t  length = sizeof (sin);
#elif defined(windows_platform)
        int length;
#else
        uint length = sizeof (sin);
#endif
        if (getsockname (sock, (struct sockaddr *) &sin, &length)) {
            rodsLog (LOG_NOTICE,
                     "sockOpenForInConn() -- getsockname() failed: errno=%d", errno);
            return SYS_SOCK_BIND_ERR - errno;
        }

        *portNum = ntohs (sin.sin_port);
        *addr =  strdup (rods_inet_ntoa (sin.sin_addr));
    }

    return (sock);

} // sockOpenForInConn




#else

/* open sock for incoming connection */
int 
sockOpenForInConn (rsComm_t *rsComm, int *portNum, char **addr, int proto)
{
    struct sockaddr_in  mySockAddr;
    int sock;
    int status;
    int svrPortRangeStart, svrPortRangeEnd;
    char *tmpPtr;
 
    if (proto != SOCK_DGRAM && proto != SOCK_STREAM) {
        rodsLog (LOG_ERROR,
                 "sockOpenForInConn() -- invalid input protocol %d", proto);
        return SYS_INVALID_PROTOCOL_TYPE;
    }

    memset((char *) &mySockAddr, 0, sizeof mySockAddr);

    sock = socket (AF_INET, proto, 0);

    if (sock < 0) {
        status = SYS_SOCK_OPEN_ERR - errno;
        rodsLogError (LOG_NOTICE, status,
                      "sockOpenForInConn: open socket error. status = %d", status);
        return (status);
    }

    /* For SOCK_DGRAM, done in checkbuf */
    if (proto == SOCK_STREAM) {
        rodsSetSockOpt (sock, rsComm->windowSize);
    }

    mySockAddr.sin_family = AF_INET;

    /* if portNum is <= 0 and env svrPortRangeStart is set, pick a port
     * in the range.
     */

    if (*portNum <= 0 && (tmpPtr = getenv ("svrPortRangeStart")) != NULL) {
        int portRangeCount;
        int bindCnt = 0;
        int myPortNum;

        svrPortRangeStart = atoi (tmpPtr);
        if ((tmpPtr = getenv ("svrPortRangeEnd")) != NULL) { 
            svrPortRangeEnd = atoi (tmpPtr);
            if (svrPortRangeEnd < svrPortRangeStart) {
                rodsLog (LOG_ERROR,
                         "sockOpenForInConn: PortRangeStart %d > PortRangeEnd %d",
                         svrPortRangeStart, svrPortRangeEnd);
                svrPortRangeEnd = svrPortRangeStart + DEF_NUMBER_SVR_PORT - 1;
            }
        } else {
            svrPortRangeEnd = svrPortRangeStart + DEF_NUMBER_SVR_PORT - 1;
        }
        portRangeCount = svrPortRangeEnd - svrPortRangeStart + 1;

        myPortNum = svrPortRangeStart + random() % portRangeCount;
        while (bindCnt < portRangeCount) {
            if (myPortNum > svrPortRangeEnd) {
                myPortNum = svrPortRangeStart;
            }
            mySockAddr.sin_port = htons(myPortNum);

            if ((status = bind (sock, (struct sockaddr *) &mySockAddr,
                                sizeof mySockAddr)) >= 0) {
                *portNum = myPortNum;
                rodsLog (LOG_DEBUG,
                         "sockOpenForInConn: port number = %d", myPortNum);
                break;
            }
            bindCnt ++;
            myPortNum ++;
        }
 
    } else {
        mySockAddr.sin_port = htons(*portNum);
        status = bind (sock, (struct sockaddr *) &mySockAddr, 
                       sizeof mySockAddr);
    }

    if (status < 0) { 
        status = SYS_SOCK_BIND_ERR - errno;
        rodsLog (LOG_NOTICE,
                 "sockOpenForInConn: bind socket error. portNum = %d, errno = %d", 
                 *portNum, errno);
        return (status);   
    }

    if (addr != NULL) {
        struct sockaddr_in sin;
#if defined(aix_platform)
        socklen_t  length = sizeof (sin);
#elif defined(windows_platform)
        int length;
#else
        uint length = sizeof (sin);
#endif
        if (getsockname (sock, (struct sockaddr *) &sin, &length)) {
            rodsLog (LOG_NOTICE,
                     "sockOpenForInConn() -- getsockname() failed: errno=%d", errno);
            return SYS_SOCK_BIND_ERR - errno;
        }
        *portNum = ntohs (sin.sin_port);
        *addr =  strdup (rods_inet_ntoa (sin.sin_addr));
    }

    return (sock);
}

/* rsAcceptConn - Server accept connection */

int 
rsAcceptConn (rsComm_t *svrComm)
{
    socklen_t len;
    int newSock;
    int status;

    len = sizeof (svrComm->remoteAddr);
    newSock = accept (svrComm->sock, 
                      (struct sockaddr *) &svrComm->remoteAddr, &len);

    if (newSock < 0) {
        status = SYS_SOCK_ACCEPT_ERR - errno;
        rodsLogError (LOG_NOTICE, status,
                      "rsAcceptConn: accept error for socket %d, status = %d", 
                      svrComm->sock, status);
    }
    rodsSetSockOpt (newSock, svrComm->windowSize);

    return (newSock);
}

int
readMsgHeader (int sock, msgHeader_t *myHeader, struct timeval *tv)
{
    int nbytes;
    int myLen;
    char tmpBuf[MAX_NAME_LEN]; 
    msgHeader_t *outHeader;
    int status;
    
    /* read the header length packet */

    nbytes = myRead (sock, (void *) &myLen, sizeof (myLen), 
                     SOCK_TYPE, NULL, tv);

    if (nbytes != sizeof (myLen)) {
        if (nbytes < 0) {
            status = nbytes - errno;
        } else {
            status = SYS_HEADER_READ_LEN_ERR - errno;
        }
        rodsLog (LOG_ERROR,
                 "readMsgHeader:header read- read %d bytes, expect %d, status = %d",
                 nbytes, sizeof (myLen), status);
        return (status);
    }

    myLen =  ntohl (myLen);

    if (myLen > MAX_NAME_LEN || myLen <= 0) {
        rodsLog (LOG_ERROR,
                 "readMsgHeader: header length %d out of range",
                 myLen);
        return (SYS_HEADER_READ_LEN_ERR);
    }

    nbytes = myRead (sock, (void *) tmpBuf, myLen, SOCK_TYPE, NULL, tv);

    if (nbytes != myLen) {
        if (nbytes < 0) {
            status = nbytes - errno;
        } else {
            status = SYS_HEADER_READ_LEN_ERR - errno;
        }
        rodsLog (LOG_ERROR,
                 "readMsgHeader:header read- read %d bytes, expect %d, status = %d",
                 nbytes, myLen, status);
        return (status);
    }

    if (getRodsLogLevel () >= LOG_DEBUG3) {
        printf ("received header: len = %d\n%s\n", myLen, tmpBuf);
    }

    /* always use XML_PROT for the startup pack */
    status = unpackStruct ((void *) tmpBuf, (void **) &outHeader,
                           "MsgHeader_PI", RodsPackTable, XML_PROT);

    if (status < 0) {
        rodsLogError (LOG_ERROR,  status,
                      "readMsgHeader:unpackStruct error. status = %d",
                      status);
        return (status);
    }

    *myHeader = *outHeader;

    free (outHeader);

    return (0);
}

int
writeMsgHeader (int sock, msgHeader_t *myHeader)
{
    int nbytes;
    int status;
    int myLen;
    bytesBuf_t *headerBBuf = NULL;

    /* always use XML_PROT for the Header */
    status = packStruct ((void *) myHeader, &headerBBuf,
                         "MsgHeader_PI", RodsPackTable, 0, XML_PROT);

    if (status < 0 || NULL == headerBBuf ) { // JMC cppcheck - nullptr
        rodsLogError (LOG_ERROR, status,
                      "writeMsgHeader: packStruct error, status = %d", status);
        return status;
    }

    if (getRodsLogLevel () >= LOG_DEBUG3) {
        printf ("sending header: len = %d\n%s\n", headerBBuf->len, 
                (char *) headerBBuf->buf);
    }

    myLen = htonl (headerBBuf->len);

    nbytes = myWrite (sock, (void *) &myLen, sizeof (myLen), SOCK_TYPE, NULL);

    if (nbytes != sizeof (myLen)) {
        rodsLog (LOG_ERROR,
                 "writeMsgHeader: wrote %d bytes for myLen , expect %d, status = %d",
                 nbytes, sizeof (myLen), SYS_HEADER_WRITE_LEN_ERR - errno);
        return (SYS_HEADER_WRITE_LEN_ERR - errno);
    }

    /* now send the header */

    nbytes = myWrite (sock, headerBBuf->buf, headerBBuf->len, SOCK_TYPE, NULL);

    if (headerBBuf->len != nbytes) {
        rodsLog (LOG_ERROR,
                 "writeMsgHeader: wrote %d bytes, expect %d, status = %d",
                 nbytes, headerBBuf->len, SYS_HEADER_WRITE_LEN_ERR - errno);
        freeBBuf (headerBBuf);
        return (SYS_HEADER_WRITE_LEN_ERR - errno);
    }

    freeBBuf (headerBBuf);

    return (0);
}

int 
myRead (int sock, void *buf, int len, irodsDescType_t irodsDescType,
        int *bytesRead, struct timeval *tv)
{
    int nbytes;
    int toRead;
    char *tmpPtr;
    fd_set set;
    struct timeval timeout;
    int status;

    /* Initialize the file descriptor set. */
    FD_ZERO (&set);
    FD_SET (sock, &set);
    if (tv != NULL) timeout = *tv;

    toRead = len;
    tmpPtr = (char *) buf;

    if (bytesRead != NULL)
        *bytesRead = 0;

    while (toRead > 0) {
#ifdef _WIN32
        if (irodsDescType == SOCK_TYPE) {
            nbytes = recv(sock, tmpPtr, toRead, 0);
        } else {
            nbytes = read (sock, (void *) tmpPtr, toRead);
        }
#else
        if (tv != NULL) {
            status = select (sock + 1, &set, NULL, NULL, &timeout);
            if (status == 0) {
                /* timedout */
                if (len - toRead > 0) {
                    return (len - toRead);
                } else {
                    return SYS_SOCK_READ_TIMEDOUT;
                }
            } else if (status < 0) {
                if ( errno == EINTR) {
                    continue;
                } else {
                    return SYS_SOCK_READ_ERR - errno;
                }
            }
        }
        nbytes = read (sock, (void *) tmpPtr, toRead);
#endif
        if (nbytes <= 0) {
            if (errno == EINTR) {
                /* interrupted */
                errno = 0;
                nbytes = 0;
            } else {
                break;
            }
        }

        toRead -= nbytes;
        tmpPtr += nbytes;
        if (bytesRead != NULL)
            *bytesRead += nbytes;
    }
    return (len - toRead);
}

int 
myWrite (int sock, void *buf, int len, irodsDescType_t irodsDescType,
         int *bytesWritten)
{
    int nbytes;
    int toWrite;
    char *tmpPtr;

    toWrite = len;
    tmpPtr = (char *) buf;

    if (bytesWritten != NULL)
        *bytesWritten = 0;

    while (toWrite > 0) {
#ifdef _WIN32
        if (irodsDescType == SOCK_TYPE) {
            nbytes = send (sock, tmpPtr, toWrite, 0);
        } else {
            nbytes = write (sock, (void *) tmpPtr, toWrite);
        }
#else
        nbytes = write (sock, (void *) tmpPtr, toWrite);
#endif
        if (nbytes <= 0) {
            if (errno == EINTR) {
                /* interrupted */
                errno = 0;
                nbytes = 0;
            } else {
                break;
            }
        }
        toWrite -= nbytes;
        tmpPtr += nbytes;
        if (bytesWritten != NULL)
            *bytesWritten += nbytes;
    }
    return (len - toWrite);
}

int
readVersion (int sock, version_t **myVersion)
{
    int status;
    msgHeader_t myHeader;
    bytesBuf_t inputStructBBuf, bsBBuf, errorBBuf;
    struct timeval tv;

    tv.tv_sec = READ_VERSION_TOUT_SEC;
    tv.tv_usec = 0;

    status = readMsgHeader (sock, &myHeader, &tv);

    if (status < 0) {
        rodsLogError (LOG_NOTICE, status,
                      "readVersion: readMsgHeader error. status = %d", status);
        return (status);
    }

    memset (&bsBBuf, 0, sizeof (bytesBuf_t));
    status = readMsgBody (sock, &myHeader, &inputStructBBuf, &bsBBuf,
                          &errorBBuf, XML_PROT, NULL);
    if (status < 0) {
        rodsLogError (LOG_NOTICE, status,
                      "readVersion: readMsgBody error. status = %d", status);
        return (status);
    }

    /* some sanity check */

    if (strcmp (myHeader.type, RODS_VERSION_T) != 0) {
        if (inputStructBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        if (bsBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        if (errorBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE,
                 "readVersion: wrong mag type - %s, expect %s",
                 myHeader.type, RODS_VERSION_T);
        return (SYS_HEADER_TPYE_LEN_ERR);
    }
 
    if (myHeader.bsLen != 0) {
        if (bsBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE, "readVersion: myHeader.bsLen = %d is not 0",
                 myHeader.bsLen);
    }

    if (myHeader.errorLen != 0) {
        if (errorBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE, "readVersion: myHeader.errorLen = %d is not 0",
                 myHeader.errorLen);
    }

    if (myHeader.msgLen > (int) sizeof (version_t) * 2 || myHeader.msgLen <= 0) {
        if (inputStructBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE, 
                 "readVersion: problem with myHeader.msgLen = %d",
                 myHeader.msgLen);
        return (SYS_HEADER_READ_LEN_ERR);
    }

    /* alway use XML for version */
    status = unpackStruct (inputStructBBuf.buf, (void **) myVersion, 
                           "Version_PI", RodsPackTable, XML_PROT);

    free (inputStructBBuf.buf);

    if (status < 0) {
        rodsLogError (LOG_NOTICE, status,
                      "readVersion:unpackStruct error. status = %d",
                      status);
    } 
    return (status);
}

int
rodsSetSockOpt (int sock, int windowSize)
{
    int status;
    int savedStatus = 0;
    int temp;
#ifndef _WIN32
    struct linger linger;
#endif

    if (windowSize <= 0) {
        windowSize = SOCK_WINDOW_SIZE;
    } else if (windowSize < MIN_SOCK_WINDOW_SIZE) {
        rodsLog (LOG_NOTICE,
                 "rodsSetSockOpt: the input windowSize %d is too small, default to %d",
                 windowSize, MIN_SOCK_WINDOW_SIZE);
        windowSize = MIN_SOCK_WINDOW_SIZE;
    } else if (windowSize > MAX_SOCK_WINDOW_SIZE) {
        rodsLog (LOG_NOTICE, 
                 "rodsSetSockOpt: the input windowSize %d is too large, default to %d",
                 windowSize, MAX_SOCK_WINDOW_SIZE);
        windowSize = MAX_SOCK_WINDOW_SIZE;
    }

#ifdef _WIN32
    status = setsockopt (sock, SOL_SOCKET, SO_SNDBUF, 
                         (char*)&windowSize, sizeof (windowSize));
    if (status < 0) 
        savedStatus = status;

    status = setsockopt (sock, SOL_SOCKET, SO_RCVBUF, 
                         (char*)&windowSize, sizeof (windowSize));
    if (status < 0) 
        savedStatus = status;

    temp = 1;
    status = setsockopt (sock, IPPROTO_TCP, TCP_NODELAY,
                         (char*)&temp, sizeof (temp));
    if (status < 0)
        savedStatus = status;
#else
    status = setsockopt (sock, SOL_SOCKET, SO_SNDBUF, 
                         &windowSize, sizeof (windowSize));
    if (status < 0) 
        savedStatus = status;

    status = setsockopt (sock, SOL_SOCKET, SO_RCVBUF, 
                         &windowSize, sizeof (windowSize));
    if (status < 0) 
        savedStatus = status;

    temp = 1;
    status = setsockopt (sock, IPPROTO_TCP, TCP_NODELAY,
                         &temp, sizeof (temp));
    if (status < 0)
        savedStatus = status;

    /* reuse the socket. Otherwise will be kept for 2-4 minutes */
    status = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, 
                         &temp, sizeof (temp));
    if (status < 0)
        savedStatus = status;

    /* keep connection alive */
    temp = 1;
    status = setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, &temp, sizeof (temp));
    if (status < 0)
        savedStatus = status;

    linger.l_onoff = 1;
    linger.l_linger = 5;
    status = setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
    if (status < 0)
        savedStatus = status;
#endif

    return (savedStatus);
}

int 
connectToRhostPortal (char *rodsHost, int rodsPort, 
                      int cookie, int windowSize)
{
    int status, nbytes;
    struct sockaddr_in remoteAddr;
    int sock, myCookie;

    status = setSockAddr (&remoteAddr, rodsHost, rodsPort);
    if (status < 0) {
        rodsLog (LOG_NOTICE,
                 "connectToRhostPortal: setSockAddr error for %s, errno = %d",
                 rodsHost, errno); 
        return (status);
    }
    /* set timeout 11/13/2009 */
    sock = connectToRhostWithRaddr (&remoteAddr, windowSize, 1);

    if (sock < 0) {
        rodsLog (LOG_ERROR,
                 "connectToRhostPortal: connectTo Rhost %s port %d error, status = %d",
                 rodsHost, rodsPort, sock);
        return (sock);
    }

    myCookie = htonl (cookie);
    nbytes = myWrite (sock, &myCookie, sizeof (myCookie), SOCK_TYPE, NULL);

    if (nbytes != sizeof (myCookie)) {
        CLOSE_SOCK (sock);
        return (SYS_PORT_COOKIE_ERR);
    }
    
    return (sock);
}
 
int
connectToRhost (rcComm_t *conn, int connectCnt, int reconnFlag)
{
    int status;

    conn->sock = connectToRhostWithRaddr (&conn->remoteAddr, 
                                          conn->windowSize, 1);

    if (conn->sock < 0) {
        rodsLogError (LOG_NOTICE, conn->sock,
                      "connectToRhost: connect to host %s on port %d failed, status = %d",
                      conn->host, conn->portNum, conn->sock);
        return conn->sock;
    }

    setConnAddr (conn);

    status = sendStartupPack (conn, connectCnt, reconnFlag);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
                      "connectToRhost: sendStartupPack to %s failed, status = %d",
                      conn->host, status);
        close (conn->sock);
        return status;
    }

    status = readVersion (conn->sock, &conn->svrVersion);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
                      "connectToRhost: readVersion to %s failed, status = %d",
                      conn->host, status);
        close (conn->sock);
        return status;
    }

    if (conn->svrVersion->status < 0) {
        rodsLogError (LOG_ERROR, conn->svrVersion->status,
                      "connectToRhost: error returned from host %s status = %d",
                      conn->host, conn->svrVersion->status);
        if (conn->svrVersion->status == SYS_EXCEED_CONNECT_CNT)
            rodsLog (LOG_ERROR,
                     "It is likely %s is a localhost but not recognized by this server. A line can be added to the server/config/irodsHost file to fix the problem", conn->host);
        close (conn->sock);
        return conn->svrVersion->status;
    }

    return 0;
}

int
connectToRhostWithRaddr (struct sockaddr_in *remoteAddr, int windowSize, 
                         int timeoutFlag)
{
    int sock;
    int status;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock <= 0) {    /* try again */
        sock = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (sock <= 0) {
        rodsLog (LOG_NOTICE,
                 "connectToRhostWithRaddr() - socket() failed: errno=%d",
                 errno);
        return (USER_SOCK_OPEN_ERR - errno);
    }

    if (timeoutFlag > 0) {
        status = connectToRhostWithTout (sock, (struct sockaddr *) remoteAddr);
    } else {
        status = connect (sock, (struct sockaddr *) remoteAddr, 
                          sizeof (struct sockaddr));
    }

    if (status < 0) {
        if (status == -1) status = USER_SOCK_CONNECT_ERR - errno;
#ifdef _WIN32
        closesocket (sock);
#else
        close (sock);
#endif /* WIN32 */
        return (status);
    }

    rodsSetSockOpt (sock, windowSize);

#ifdef PORTNAME_solaris
    flag = fcntl (sock, F_GETFL);
    if (flag & O_NONBLOCK)
        fcntl (sock, F_SETFL, (flag & (~O_NONBLOCK)));
#endif

    return (sock);

}

#ifdef _WIN32
void CALLBACK my_timeout_handler(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
    win_connect_timeout = 1;
}
#endif

int
connectToRhostWithTout (int sock, struct sockaddr *sin)
{
    int timeoutCnt = 0;
    int status;
    long arg;
    fd_set myset;
    struct timeval tv;

#ifdef _WIN32
    /* A Windows console app has very limited timeout functionality.
     * An pseudo timeout is implemented.
     */
    /*
      int connectCnt;
      int win_connect_timeout_cb;
      int win_connect_timeout = 0;
      int win_connect_timer_id;
      win_connect_timeout_cb = 0;
      win_connect_timer_id = 
      timeSetEvent(CONNECT_TIMEOUT*1000, 0, my_timeout_handler, 0, 
      TIME_ONESHOT);
      connectCnt = 0;
    */

    status = 0;

    while ((timeoutCnt < MAX_CONN_RETRY_CNT) && (!win_connect_timeout)) {
        if ((status = connect (sock, sin, sizeof (struct sockaddr))) < 0) {
            timeoutCnt ++;
            rodsSleep (0, 200000);
        } else {
            break;
        }
    }
    if(status != 0)
    {
        return USER_SOCK_CONNECT_TIMEDOUT;
    }

    return 0;

    /*
      if(win_connect_timeout) {
      fprintf(stderr,
      "portalConnect: connect msg timed out for pid %d\n", getpid ());
      status = USER_SOCK_CONNECT_ERR;
      } else {
      timeKillEvent(win_connect_timer_id);
      }
    */

#else
    /* redo the timeout using select */
    /* Set non-blocking */
    if((arg = fcntl (sock, F_GETFL, NULL)) < 0) {
        rodsLog (LOG_ERROR,
                 "connectToRhostWithTout: fcntl F_GETFL error, errno = %d", errno);
        return (USER_SOCK_CONNECT_ERR);
    }
    arg |= O_NONBLOCK;
    if (fcntl (sock, F_SETFL, arg) < 0) {
        rodsLog (LOG_ERROR,
                 "connectToRhostWithTout: fcntl F_SETFL error, errno = %d", errno);
        return (USER_SOCK_CONNECT_ERR);
    }

    while (timeoutCnt < MAX_CONN_RETRY_CNT) {
        status = connect (sock, sin, sizeof (struct sockaddr));
        if (status >= 0) break;
        if (errno == EISCONN) {
            /* already connected. seen this error on AIX */
            status = 0;
            break;
        }
        if (errno == EINPROGRESS || errno == EINTR) {
            tv.tv_sec = CONNECT_TIMEOUT_TIME;
            tv.tv_usec = 0;
            FD_ZERO (&myset);
            FD_SET (sock, &myset);
            status = select (sock + 1, NULL, &myset, NULL, &tv);
            if (status < 0) {
                if (errno != EINTR) {
                    rodsLog (LOG_NOTICE,
                             "connectToRhostWithTout: connect error, errno = %d", 
                             errno);
                    timeoutCnt++;
                }
                continue;
            } else if (status > 0) {
                int myval;
#if defined(aix_platform)
                socklen_t mylen = sizeof (int);
#else
                uint mylen = sizeof (int);
#endif
                if (getsockopt (sock, SOL_SOCKET, SO_ERROR, (void*) (&myval), 
                                &mylen) < 0) {
                    rodsLog (LOG_ERROR,
                             "connectToRhostWithTout: getsockopt error, errno = %d", 
                             errno);
                    return (USER_SOCK_CONNECT_ERR - errno);
                }
                /* Check the returned value */
                if (myval) {
                    rodsLog (LOG_NOTICE,
                             "connectToRhostWithTout: connect error, errno = %d", 
                             myval);
                    timeoutCnt++;
                    status = USER_SOCK_CONNECT_ERR - myval;
                    continue;
                } else {
                    break;
                }
            } else {
                /* timed out */
                status = USER_SOCK_CONNECT_TIMEDOUT;
                break;
            }
        } else {
            rodsLog (LOG_NOTICE,
                     "connectToRhostWithTout: connect error, errno = %d", errno);
            timeoutCnt++;
            status = USER_SOCK_CONNECT_ERR - errno;
            continue;
        }
    }

    if (status < 0) {
        if (status == -1) 
            return USER_SOCK_CONNECT_ERR;
        else 
            return status;
    }
                
    /* Set to blocking again */
    if((arg = fcntl (sock, F_GETFL, NULL)) < 0) {
        rodsLog (LOG_ERROR,
                 "connectToRhostWithTout: fcntl F_GETFL error, errno = %d", errno);
        return (USER_SOCK_CONNECT_ERR);
    }

    arg &= (~O_NONBLOCK);
    if( fcntl(sock, F_SETFL, arg) < 0) {
        rodsLog (LOG_ERROR,
                 "connectToRhostWithTout: fcntl F_SETFL error, errno = %d", errno);
        return (USER_SOCK_CONNECT_ERR);
    }

#endif
    if (status < 0) {
        rodsLog (LOG_NOTICE,
                 "connectToRhostWithTout: connect failed. errno = %d \n",
                 errno);
        status = USER_SOCK_CONNECT_ERR - errno;
    }
    return (status);
}

int
setConnAddr (rcComm_t *conn)
{
    int status1, status2;

    status1 = setLocalAddr (conn->sock, &conn->localAddr);

    status2 = setRemoteAddr (conn->sock, &conn->remoteAddr);

    if (status1 < 0) {
        return status1;
    } else if (status2 < 0) {
        return status2;
    } else {
        return 0;
    }
}

int
setRemoteAddr (int sock, struct sockaddr_in *remoteAddr)
{
#if defined(aix_platform)
    socklen_t       laddrlen = sizeof(struct sockaddr);
#elif defined(windows_platform)
    int laddrlen = sizeof(struct sockaddr);
#else
    uint         laddrlen = sizeof(struct sockaddr);
#endif

    /* fill in the server address. This is for case where the conn->host
     * is not a real host but an address that select a host from a pool
     * of hosts */
    if (getpeername(sock, (struct sockaddr *) remoteAddr,
                    &laddrlen) < 0) {
        rodsLog (LOG_NOTICE,
                 "setLocalAddr() -- getpeername() failed: errno=%d", errno);
        return (USER_RODS_HOSTNAME_ERR);
    }

    return (0);
}

int
setLocalAddr (int sock, struct sockaddr_in *localAddr)
{
#if defined(aix_platform)
    socklen_t       laddrlen = sizeof(struct sockaddr);
#elif defined(windows_platform)
    int         laddrlen = sizeof(struct sockaddr);
#else
    uint         laddrlen = sizeof(struct sockaddr);
#endif


    /* fill in the client address */
    if (getsockname (sock, (struct sockaddr *) localAddr,
                     &laddrlen) < 0) {
        rodsLog (LOG_NOTICE,
                 "setLocalAddr() -- getsockname() failed: errno=%d",
                 errno);
        return USER_RODS_HOSTNAME_ERR;
    }
    return ntohs (localAddr->sin_port);
}

int
sendStartupPack (rcComm_t *conn, int connectCnt, int reconnFlag)
{
    startupPack_t startupPack;
    int status;
    char *tmpStr;
    bytesBuf_t *startupPackBBuf = NULL;
    

    /* setup the startup pack */

    startupPack.irodsProt = conn->irodsProt;
    startupPack.connectCnt = connectCnt;
    startupPack.reconnFlag = reconnFlag;

    rstrcpy (startupPack.proxyUser, conn->proxyUser.userName, NAME_LEN);
    rstrcpy (startupPack.proxyRodsZone, conn->proxyUser.rodsZone, NAME_LEN);
    rstrcpy (startupPack.clientUser, conn->clientUser.userName, NAME_LEN);
    rstrcpy (startupPack.clientRodsZone, conn->clientUser.rodsZone, 
             NAME_LEN);
    rstrcpy (startupPack.relVersion, RODS_REL_VERSION,  NAME_LEN);
    rstrcpy (startupPack.apiVersion, RODS_API_VERSION,  NAME_LEN);

    if ((tmpStr = getenv (SP_OPTION)) != NULL) {
        rstrcpy (startupPack.option, tmpStr, NAME_LEN);
    } else {
        startupPack.option[0] = '\0';
    }

    /* always use XML_PROT for the startupPack */
    status = packStruct ((void *) &startupPack, &startupPackBBuf,
                         "StartupPack_PI", RodsPackTable, 0, XML_PROT);

    if (status < 0) {
        rodsLogError (LOG_NOTICE, status,
                      "sendStartupPack: packStruct error, status = %d", status);
        return status;
    }

    status = sendRodsMsg (conn->sock, RODS_CONNECT_T, startupPackBBuf, 
                          NULL, NULL, 0, XML_PROT);

    freeBBuf (startupPackBBuf);

    if (status < 0) {
        rodsLogError (LOG_NOTICE, status,
                      "sendStartupPack: sendRodsMsg error, status = %d", status);
        return status;
    }

    return (status);
}

int
sendVersion (int sock, int versionStatus, int reconnPort, 
             char *reconnAddr, int cookie)
{
    version_t myVersion;
    int status;
    bytesBuf_t *versionBBuf = NULL;


    /* setup the version struct */

    memset (&myVersion, 0, sizeof (myVersion));
    myVersion.status = versionStatus;
    rstrcpy (myVersion.relVersion, RODS_REL_VERSION,  NAME_LEN);
    rstrcpy (myVersion.apiVersion, RODS_API_VERSION,  NAME_LEN);
    if (reconnAddr != NULL) {
        myVersion.reconnPort = reconnPort;
        rstrcpy (myVersion.reconnAddr, reconnAddr, LONG_NAME_LEN);
        myVersion.cookie = cookie;
    }

    /* alway use XML for version */
    status = packStruct ((char *) &myVersion, &versionBBuf,
                         "Version_PI", RodsPackTable, 0, XML_PROT);

    if (status < 0) {
        rodsLogError (LOG_NOTICE, status,
                      "sendVersion: packStruct error, status = %d", status);
        return status;
    }

    status = sendRodsMsg (sock, RODS_VERSION_T, versionBBuf, NULL, NULL, 0,
                          XML_PROT);

    freeBBuf (versionBBuf);

    if (status < 0) {
        rodsLogError (LOG_NOTICE, status,
                      "sendVersion: sendRodsMsg error, status = %d", status);
        return status;
    }

    return (status);
}


int
sendRodsMsg (int sock, char *msgType, bytesBuf_t *msgBBuf, 
             bytesBuf_t *byteStreamBBuf, bytesBuf_t *errorBBuf, int intInfo, 
             irodsProt_t irodsProt)
{
    int status;
    msgHeader_t msgHeader;
    int bytesWritten;

    memset (&msgHeader, 0, sizeof (msgHeader));

    rstrcpy (msgHeader.type, msgType, HEADER_TYPE_LEN);

    if (msgBBuf == NULL) {
        msgHeader.msgLen = 0;
    } else {
        msgHeader.msgLen = msgBBuf->len;
    }

    if (byteStreamBBuf == NULL) {
        msgHeader.bsLen = 0;
    } else {
        msgHeader.bsLen = byteStreamBBuf->len;
    }
 
    if (errorBBuf == NULL) {
        msgHeader.errorLen = 0;
    } else { 
        msgHeader.errorLen = errorBBuf->len;
    }

    msgHeader.intInfo = intInfo;

    status = writeMsgHeader (sock, &msgHeader);

    if (status < 0)
        return (status);

    /* send the rest */

    if (msgHeader.msgLen > 0) {
        if (irodsProt == XML_PROT && getRodsLogLevel () >= LOG_DEBUG3) {
            printf ("sending msg: \n%s\n", (char *) msgBBuf->buf);
        }
        status = myWrite (sock, msgBBuf->buf, msgBBuf->len, SOCK_TYPE, NULL);
        if (status < 0) 
            return (status);
    }

    if (msgHeader.errorLen > 0) {
        if (irodsProt == XML_PROT && getRodsLogLevel () >= LOG_DEBUG3) {
            printf ("sending error msg: \n%s\n", (char *) errorBBuf->buf);
        }
        status = myWrite (sock, errorBBuf->buf, errorBBuf->len, SOCK_TYPE, 
                          NULL);
        if (status < 0)
            return (status);
    }
    if (msgHeader.bsLen > 0) {
        status = myWrite (sock, byteStreamBBuf->buf,
                          byteStreamBBuf->len, SOCK_TYPE, &bytesWritten);
        if (status < 0)
            return (status);
    }

    return (0);
}

int
rodsSleep (int sec, int microSec)
{
    struct timeval sleepTime;

    sleepTime.tv_sec = sec;
    sleepTime.tv_usec = microSec;

    select (0, NULL, NULL, NULL, &sleepTime);

    return 0;
}

int
readMsgBody (int sock, msgHeader_t *myHeader, bytesBuf_t *inputStructBBuf, 
             bytesBuf_t *bsBBuf, bytesBuf_t *errorBBuf, irodsProt_t irodsProt,
             struct timeval *tv)
{
    int nbytes;
    int bytesRead;

    if (myHeader == NULL) {
        return (SYS_READ_MSG_BODY_INPUT_ERR);
    }
    if (inputStructBBuf != NULL)
        memset (inputStructBBuf, 0, sizeof (bytesBuf_t));

    /* Don't memset bsBBuf because bsBBuf can be reused on the client side */

    if (errorBBuf != NULL)
        memset (errorBBuf, 0, sizeof (bytesBuf_t));

    if (myHeader->msgLen > 0) {
        if (inputStructBBuf == NULL) {
            return (SYS_READ_MSG_BODY_INPUT_ERR);
        }

        inputStructBBuf->buf = malloc (myHeader->msgLen);

        nbytes = myRead (sock, inputStructBBuf->buf, myHeader->msgLen, 
                         SOCK_TYPE, NULL, tv);

        if (irodsProt == XML_PROT && getRodsLogLevel () >= LOG_DEBUG3) {
            printf ("received msg: \n%s\n", (char *) inputStructBBuf->buf);
        }

        if (nbytes != myHeader->msgLen) {
            rodsLog (LOG_NOTICE, 
                     "readMsgBody: inputStruct read error, read %d bytes, expect %d",
                     nbytes, myHeader->msgLen);
            free (inputStructBBuf->buf);
            return (SYS_HEADER_READ_LEN_ERR);
        }
        inputStructBBuf->len = myHeader->msgLen;
    }
 
    if (myHeader->errorLen > 0) {
        if (errorBBuf == NULL) {
            return (SYS_READ_MSG_BODY_INPUT_ERR);
        }

        errorBBuf->buf = malloc (myHeader->errorLen);

        nbytes = myRead (sock, errorBBuf->buf, myHeader->errorLen,
                         SOCK_TYPE, NULL, tv);

        if (irodsProt == XML_PROT && getRodsLogLevel () >= LOG_DEBUG3) {
            printf ("received error msg: \n%s\n", (char *) errorBBuf->buf);
        }

        if (nbytes != myHeader->errorLen) {
            rodsLog (LOG_NOTICE,
                     "readMsgBody: errorBbuf read error, read %d bytes, expect %d, errno = %d",
                     nbytes, myHeader->msgLen, errno);
            free (errorBBuf->buf);
            return (SYS_READ_MSG_BODY_LEN_ERR - errno);
        }
        errorBBuf->len = myHeader->errorLen;
    }

    if (myHeader->bsLen > 0) {
        if (bsBBuf == NULL) {
            return (SYS_READ_MSG_BODY_INPUT_ERR);
        }

        if (bsBBuf->buf == NULL) {
            bsBBuf->buf = malloc (myHeader->bsLen);
        } else if (myHeader->bsLen > bsBBuf->len) {
            free (bsBBuf->buf);
            bsBBuf->buf = malloc (myHeader->bsLen);
        }

        nbytes = myRead (sock, bsBBuf->buf, myHeader->bsLen, SOCK_TYPE,
                         &bytesRead, tv);

        if (nbytes != myHeader->bsLen) {
            rodsLog (LOG_NOTICE, 
                     "readMsgBody: bsBBuf read error, read %d bytes, expect %d, errno = %d",
                     nbytes, myHeader->bsLen, errno);
            free (bsBBuf->buf);
            return (SYS_READ_MSG_BODY_INPUT_ERR - errno);
        }
        bsBBuf->len = myHeader->bsLen;
    }

    return (0);
}

char *
rods_inet_ntoa (struct in_addr in)
{
    char *clHostAddr;

    clHostAddr = inet_ntoa (in);

    if (strcmp (clHostAddr, "127.0.0.1") == 0 ||
        strcmp (clHostAddr, "0.0.0.0") == 0) { /* localhost */
        char sb[LONG_NAME_LEN];
        struct hostent *phe;

        if (gethostname (sb, sizeof (sb)) != 0)
            return(clHostAddr);
        if ((phe = gethostbyname (sb)) == NULL)
            return(clHostAddr);
        clHostAddr = inet_ntoa (*(struct in_addr*) phe->h_addr);
    }

    return (clHostAddr);
}
#if 0 // JMC - UNUSED 
int
irodsCloseSock (int sock)
{
#ifdef _WIN32
    return (closesocket (sock));
#else
    return (close (sock));
#endif /* WIN32 */
}
#endif // JMC - UNUSED 
int
readReconMsg (int sock, reconnMsg_t **reconnMsg)
{
    int status;
    msgHeader_t myHeader;
    bytesBuf_t inputStructBBuf, bsBBuf, errorBBuf;

    status = readMsgHeader (sock, &myHeader, NULL);

    if (status < 0) {
        rodsLogError (LOG_NOTICE, status,
                      "readReconMsg: readMsgHeader error. status = %d", status);
        return (status);
    }

    memset (&bsBBuf, 0, sizeof (bytesBuf_t));  
    status = readMsgBody (sock, &myHeader, &inputStructBBuf, &bsBBuf, 
                          &errorBBuf, XML_PROT, NULL);
    if (status < 0) {
        rodsLogError (LOG_NOTICE, status,
                      "readReconMsg: readMsgBody error. status = %d", status);
        return (status);
    }

    /* some sanity check */

    if (strcmp (myHeader.type, RODS_RECONNECT_T) != 0) {
        if (inputStructBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        if (bsBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        if (errorBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE,
                 "readReconMsg: wrong mag type - %s, expect %s",
                 myHeader.type, RODS_CONNECT_T);
        return (SYS_HEADER_TPYE_LEN_ERR);
    }
 
    if (myHeader.bsLen != 0) {
        if (bsBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE, "readReconMsg: myHeader.bsLen = %d is not 0",
                 myHeader.bsLen);
    }

    if (myHeader.errorLen != 0) {
        if (errorBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE, 
                 "readReconMsg: myHeader.errorLen = %d is not 0",
                 myHeader.errorLen);
    }

    if (myHeader.msgLen <= 0) {
        if (inputStructBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE, 
                 "readReconMsg: problem with myHeader.msgLen = %d",
                 myHeader.msgLen);
        return (SYS_HEADER_READ_LEN_ERR);
    }

    /* always use XML_PROT for the startup pack */
    status = unpackStruct (inputStructBBuf.buf, (void **) reconnMsg, 
                           "ReconnMsg_PI", RodsPackTable, XML_PROT);

    clearBBuf (&inputStructBBuf);

    if (status < 0) {
        rodsLogError (LOG_NOTICE,  status,
                      "readReconMsg:unpackStruct error. status = %d",
                      status);
    } 
    return (status);
}

int
sendReconnMsg (int sock, reconnMsg_t *reconnMsg)
{
    int status;
    bytesBuf_t *reconnMsgBBuf = NULL;

    if (reconnMsg == NULL) return (USER__NULL_INPUT_ERR);

    /* alway use XML for version */
    status = packStruct ((char *) reconnMsg, &reconnMsgBBuf,
                         "ReconnMsg_PI", RodsPackTable, 0, XML_PROT);

    status = sendRodsMsg (sock, RODS_RECONNECT_T, reconnMsgBBuf,
                          NULL, NULL, 0, XML_PROT);

    freeBBuf (reconnMsgBBuf);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
                      "sendReconnMsg: sendRodsMsg of reconnect msg failed, status = %d",
                      status);
    }
    return (status);
}

int svrSwitchConnect (rsComm_t *rsComm)
{
    if (rsComm->reconnectedSock > 0) {
        if (rsComm->clientState == RECEIVING_STATE) {
            reconnMsg_t reconnMsg;
            bzero (&reconnMsg, sizeof (reconnMsg));
            sendReconnMsg (rsComm->sock, &reconnMsg);
            rsComm->clientState = PROCESSING_STATE;
        }
        close (rsComm->sock); 
        rsComm->sock = rsComm->reconnectedSock;
        rsComm->reconnectedSock = 0;
        rodsLog (LOG_NOTICE,
                 "svrSwitchConnect: Switch connection");
        return 1;
    } else {
        return 0;
    }
}

int cliSwitchConnect (rcComm_t *conn)
{
    if (conn->reconnectedSock > 0) {
        if (conn->agentState == RECEIVING_STATE) {
            reconnMsg_t reconnMsg;
            bzero (&reconnMsg, sizeof (reconnMsg));
            sendReconnMsg (conn->sock, &reconnMsg);
            conn->agentState = PROCESSING_STATE;
        }
        close (conn->sock);
        conn->sock = conn->reconnectedSock;
        conn->reconnectedSock = 0;
        printf ("The client/server socket connection has been renewed\n");
        return 1;
    } else {
        return 0;
    }
}

int
addUdpPortToPortList (portList_t *thisPortList, int udpport)
{
    /* put udpport in the upper 16 bits of portNum */
    thisPortList->portNum |= udpport << 16;
    return 0;
}

int
getUdpPortFromPortList (portList_t *thisPortList)
{
    int udpport = 0;
    udpport = (thisPortList->portNum & 0xffff0000) >> 16;
    return (udpport);
}

int
getTcpPortFromPortList (portList_t *thisPortList)
{
    return (thisPortList->portNum & 0xffff);
}

int
addUdpSockToPortList (portList_t *thisPortList, int udpsock)
{
    /* put udpport in the upper 16 bits of portNum */
    thisPortList->sock |= udpsock << 16;
    return 0;
}

int
getUdpSockFromPortList (portList_t *thisPortList)
{
    int udpsock = 0;
    udpsock = (thisPortList->sock & 0xffff0000) >> 16;
    return (udpsock);
}

int
getTcpSockFromPortList (portList_t *thisPortList)
{
    return (thisPortList->sock & 0xffff);
}

int
isReadMsgError (int status)
{
    int irodsErr = getIrodsErrno (status);

    if (irodsErr == SYS_READ_MSG_BODY_LEN_ERR ||
        irodsErr == SYS_HEADER_READ_LEN_ERR ||
        irodsErr == SYS_HEADER_WRITE_LEN_ERR) {
        return 1;
    } else {
        return 0;
    }
}

int 
redirectConnToRescSvr (rcComm_t **conn, dataObjInp_t *dataObjInp, 
                       rodsEnv *myEnv, int reconnFlag)
{
    int status;
    char *outHost = NULL;

    if (dataObjInp->oprType == PUT_OPR) {
        status = rcGetHostForPut (*conn, dataObjInp, &outHost);
    } else if (dataObjInp->oprType == GET_OPR) {
        status = rcGetHostForGet (*conn, dataObjInp, &outHost);
    } else {
        rodsLog (LOG_NOTICE,
                 "redirectConnToRescSvr: Unknown oprType %d\n",
                 dataObjInp->oprType);
        return 0;
    }

    if (status < 0 || outHost == NULL || strcmp (outHost, THIS_ADDRESS) == 0)
        return status;

#if 0
    newConn =  rcConnect (outHost, myEnv->rodsPort, myEnv->rodsUserName,
                          myEnv->rodsZone, reconnFlag, &errMsg);

    if (newConn != NULL) {
        status = clientLogin(newConn);
        if (status != 0) {
            rcDisconnect(newConn);
            return status;
        }
        rcDisconnect (*conn);
        *conn = newConn;
    }
    return 0;
#else
    status = rcReconnect (conn, outHost, myEnv, reconnFlag);
    return status;
#endif
}

int
rcReconnect (rcComm_t **conn, char *newHost, rodsEnv *myEnv, int reconnFlag)
{
    int status;
    rcComm_t *newConn = NULL;
    rErrMsg_t errMsg;

    bzero (&errMsg, sizeof (errMsg));

    newConn =  rcConnect (newHost, myEnv->rodsPort, myEnv->rodsUserName,
                          myEnv->rodsZone, reconnFlag, &errMsg);

    if (newConn != NULL) {
        status = clientLogin(newConn);
        if (status != 0) {
            rcDisconnect(newConn);
            return status;
        }
        rcDisconnect (*conn);
        *conn = newConn;
        return 0;
    } else {
        return errMsg.status;
    }
}

int
mySockClose (int sock)
{
    int status;
#ifdef _WIN32
    status = closesocket (sock);
#else   /* _WIN32 */
#if defined(solaris_platform) || defined(linux_platform) || defined(osx_platform)
    /* For reason I do not completely understand, if I do a socket write and
     * then a socket close immediately, the receiver at the other end can
     * get a errno 104 (reset by peer) and does no always get the sent msg
     * even though setting SO_LINGER. Making a shutdown call to shutdown
     * the send channel seems to do the job.
     */
    shutdown (sock, SHUT_WR);
#endif
    status = close (sock);
#endif
    return status;
}

#endif // USE_BOOST_ASIO 
