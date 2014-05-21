/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rcConnect.c - client connect call to server
 *
 */

#include "rcConnect.hpp"
#include "rcGlobal.hpp"

#ifdef windows_platform
#include "startsock.hpp"
#endif

#include <boost/thread/thread_time.hpp>

// =-=-=-=-=-=-=-
#include "irods_stacktrace.hpp"
#include "irods_network_factory.hpp"

rcComm_t *
rcConnect( char *rodsHost, int rodsPort, char *userName, char *rodsZone,
           int reconnFlag, rErrMsg_t *errMsg ) {
    if ( strlen( rodsHost ) == 0 ) {
        irods::stacktrace st;
        st.trace();
        st.dump();
    }

    rcComm_t *conn;

#ifdef windows_platform
    if ( 0 != startWinsock() ) {
        conn = NULL;
        /*error -34*/
        return conn;
    }
#endif

#ifndef windows_platform
    if ( reconnFlag != RECONN_TIMEOUT && getenv( RECONNECT_ENV ) != NULL ) {
        reconnFlag = RECONN_TIMEOUT;
    }
#endif

    conn = _rcConnect( rodsHost, rodsPort, userName, rodsZone, NULL, NULL,
                       errMsg, 0, reconnFlag );

    return ( conn );
}

rcComm_t *
_rcConnect( char *rodsHost, int rodsPort,
            char *proxyUserName, char *proxyRodsZone,
            char *clientUserName, char *clientRodsZone, rErrMsg_t *errMsg, int connectCnt,
            int reconnFlag ) {
    rcComm_t *conn;
    int status;
    char *tmpStr;

#ifndef windows_platform
    if ( ProcessType == CLIENT_PT ) {
        signal( SIGPIPE, ( void ( * )( int ) ) rcPipSigHandler );
    }
#endif

    conn = ( rcComm_t* )malloc( sizeof( rcComm_t ) );

    memset( conn, 0, sizeof( rcComm_t ) );

    if ( errMsg != NULL ) {
        memset( errMsg, 0, sizeof( rErrMsg_t ) );
    }

    if ( ( tmpStr = getenv( IRODS_PROT ) ) != NULL ) {
        conn->irodsProt = ( irodsProt_t )atoi( tmpStr );
    }
    else {
        conn->irodsProt = NATIVE_PROT;
    }

    status = setUserInfo( proxyUserName, proxyRodsZone,
                          clientUserName, clientRodsZone, &conn->clientUser, &conn->proxyUser );

    if ( status < 0 ) {
        if ( errMsg != NULL ) {
            errMsg->status = status;
            snprintf( errMsg->msg, ERR_MSG_LEN - 1,
                      "_rcConnect: setUserInfo failed\n" );
        }
        free( conn );
        return NULL;
    }

    status = setRhostInfo( conn, rodsHost, rodsPort );

    if ( status < 0 ) {
        if ( errMsg != NULL ) {
            rodsLogError( LOG_ERROR, status,
                          "_rcConnect: setRhostInfo error, irodsHost is probably not set correctly" );
            errMsg->status = status;
            snprintf( errMsg->msg, ERR_MSG_LEN - 1,
                      "_rcConnect: setRhostInfo failed\n" );
        }
        free( conn );
        return NULL;
    }
    status = connectToRhost( conn, connectCnt, reconnFlag );

    if ( status < 0 ) {
        if ( getIrodsErrno( status ) == SYS_SOCK_READ_TIMEDOUT ) {
            /* timed out. try again */
            rodsLog( LOG_ERROR,
                     "_rcConnect: connectToRhost timedout retrying" );
            status = connectToRhost( conn, connectCnt, reconnFlag );
        }
    }

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "_rcConnect: connectToRhost error, server on %s:%d is probably down",
                      conn->host, conn->portNum );
        if ( errMsg != NULL ) {
            errMsg->status = status;
            snprintf( errMsg->msg, ERR_MSG_LEN - 1,
                      "_rcConnect: connectToRhost failed\n" );
        }
        free( conn );
        return NULL;
    }

#ifndef windows_platform
    if ( reconnFlag == RECONN_TIMEOUT &&
            conn->svrVersion != NULL &&
            conn->svrVersion->reconnPort > 0 ) {
        if ( strcmp( conn->svrVersion->reconnAddr, "127.0.0.1" ) == 0 ||
                strcmp( conn->svrVersion->reconnAddr , "0.0.0.0" ) == 0 ||
                strcmp( conn->svrVersion->reconnAddr , "localhost" ) ) {
            /* localhost. just use conn->host */
            rstrcpy( conn->svrVersion->reconnAddr, conn->host, NAME_LEN );
        }
        conn->exit_flg = false;
        conn->lock = new boost::mutex;
        conn->cond = new boost::condition_variable;
        conn->reconnThr = new boost::thread( cliReconnManager, conn );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR, "_rcConnect: pthread_create failed, stat=%d",
                     status );
        }
    }
#endif

    return ( conn );
}


int
setUserInfo(
    char *proxyUserName, char *proxyRodsZone,
    char *clientUserName, char *clientRodsZone,
    userInfo_t *clientUser, userInfo_t *proxyUser ) {
    char *myUserName;
    char *myRodsZone;

    rstrcpy( proxyUser->userName, proxyUserName, NAME_LEN );
    if ( clientUserName != NULL ) {
        rstrcpy( clientUser->userName, clientUserName, NAME_LEN );
    }
    else if ( ( myUserName = getenv( CLIENT_USER_NAME_KEYWD ) ) != NULL ) {
        rstrcpy( clientUser->userName, myUserName, NAME_LEN );
    }
    else {
        rstrcpy( clientUser->userName, proxyUserName, NAME_LEN );
    }

    rstrcpy( proxyUser->rodsZone, proxyRodsZone, NAME_LEN );
    if ( clientRodsZone != NULL ) {
        rstrcpy( clientUser->rodsZone, clientRodsZone, NAME_LEN );
    }
    else if ( ( myRodsZone = getenv( CLIENT_RODS_ZONE_KEYWD ) ) != NULL ) {
        rstrcpy( clientUser->rodsZone, myRodsZone, NAME_LEN );
    }
    else {
        rstrcpy( clientUser->rodsZone, proxyRodsZone, NAME_LEN );
    }

    return ( 0 );
}

int
setRhostInfo( rcComm_t *conn, char *rodsHost, int rodsPort ) {
    int status;

    if ( rodsHost == NULL || strlen( rodsHost ) == 0 ) {
        return ( USER_RODS_HOST_EMPTY );
    }

    rstrcpy( conn->host, rodsHost, NAME_LEN );
    conn->portNum = rodsPort;

    status = setSockAddr( &conn->remoteAddr, rodsHost, rodsPort );

    return ( status );
}

int
setSockAddr( struct sockaddr_in *remoteAddr, char *rodsHost, int rodsPort ) {
    struct hostent *myHostent;
    myHostent = gethostbyname( rodsHost );

    if ( myHostent == NULL || myHostent->h_addrtype != AF_INET ) {
        irods::stacktrace st;
        st.trace();
        st.dump();

        rodsLog( LOG_ERROR, "unknown hostname: [%s]", rodsHost );
        return ( USER_RODS_HOSTNAME_ERR - errno );
    }

    memcpy( &remoteAddr->sin_addr, myHostent->h_addr,
            myHostent->h_length );
    remoteAddr->sin_family = AF_INET;
    remoteAddr->sin_port = htons( ( unsigned short )( rodsPort ) );

    return ( 0 );
}

// =-=-=-=-=-=-=-
// request shut down client-server connection
int rcDisconnect(
    rcComm_t* _conn ) {
    // =-=-=-=-=-=-=-
    // check for invalid param
    if ( _conn == NULL ) {
        return ( 0 );
    }

    // =-=-=-=-=-=-=-
    // create network object to pass to plugin interface
    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( _conn, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // send disconnect msg to agent
    ret = sendRodsMsg(
              net_obj,
              RODS_DISCONNECT_T,
              NULL, NULL, NULL, 0,
              _conn->irodsProt );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    // =-=-=-=-=-=-=-
    // get rods env to pass to client start for policy decisions
    rodsEnv rods_env;
    int status = getRodsEnv( &rods_env );

    // =-=-=-=-=-=-=-
    // shut down any network plugin activity
    ret = sockClientStop( net_obj, &rods_env );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    net_obj->to_client( _conn );

    close( _conn->sock );

    _conn->exit_flg = true; //
    if ( _conn->reconnThr ) {
        // _conn->reconnThr->interrupt(); // terminate at next interruption point
        boost::system_time until = boost::get_system_time() + boost::posix_time::seconds( 2 );
        _conn->reconnThr->timed_join( until );    // force an interruption point
    }
    delete  _conn->reconnThr;
    delete  _conn->lock;
    delete  _conn->cond;

    status = freeRcComm( _conn );
    return ( status );

} // rcDisconnect

int
freeRcComm( rcComm_t *conn ) {
    int status;

    if ( conn == NULL ) {
        return ( 0 );
    }

    status = cleanRcComm( conn );
    free( conn );

    return status;
}

int
cleanRcComm( rcComm_t *conn ) {

    if ( conn == NULL ) {
        return ( 0 );
    }

    freeRError( conn->rError );
    conn->rError = NULL;

    if ( conn->svrVersion != NULL ) {
        free( conn->svrVersion );
        conn->svrVersion = NULL;
    }

    return ( 0 );
}
void rcPipSigHandler() {
    fprintf( stderr,
             "Client Caught broken pipe signal. Connection to server may be down\n" );
}

rcComm_t *
rcConnectXmsg( rodsEnv *myRodsEnv, rErrMsg_t *errMsg ) {
    rcComm_t *conn;

    if ( myRodsEnv == NULL ) {
        fprintf( stderr, "rcConnectXmsg: NULL myRodsEnv input\n" );
        return ( NULL );
    }

    conn = rcConnect( myRodsEnv->xmsgHost, myRodsEnv->xmsgPort,
                      myRodsEnv->rodsUserName, myRodsEnv->rodsZone, 0, errMsg );

    return ( conn );
}


void
cliReconnManager( rcComm_t *conn ) {
    struct sockaddr_in remoteAddr;
    struct hostent *myHostent;
    reconnMsg_t reconnMsg;
    reconnMsg_t *reconnMsgOut = NULL;

    if ( conn == NULL || conn->svrVersion == NULL ||
            conn->svrVersion->reconnPort <= 0 ) {
        return;
    }

    conn->reconnTime = time( 0 ) + RECONN_TIMEOUT_TIME;

    while ( !conn->exit_flg ) { /* JMC */
        time_t curTime = time( 0 );

        if ( curTime < conn->reconnTime ) {
            rodsSleep( conn->reconnTime - curTime, 0 );
        }
        boost::unique_lock<boost::mutex> boost_lock( *conn->lock );
        /* need to check clientState */
        while ( conn->clientState != PROCESSING_STATE ) {
            /* have to wait until the client stop sending */
            conn->reconnThrState = CONN_WAIT_STATE;
            rodsLog( LOG_DEBUG,
                     "cliReconnManager: clientState = %d", conn->clientState );
            conn->cond->wait( boost_lock );

        }
        rodsLog( LOG_DEBUG,
                 "cliReconnManager: Reconnecting clientState = %d",
                 conn->clientState );

        conn->reconnThrState = PROCESSING_STATE;
        /* connect to server's reconn thread */

        myHostent = gethostbyname( conn->svrVersion->reconnAddr );

        if ( myHostent == NULL || myHostent->h_addrtype != AF_INET ) {
            rodsLog( LOG_ERROR, "cliReconnManager: unknown hostname: %s",
                     conn->svrVersion->reconnAddr );
            return;
        }

        memcpy( &remoteAddr.sin_addr, myHostent->h_addr,
                myHostent->h_length );
        remoteAddr.sin_family = AF_INET;
        remoteAddr.sin_port =
            htons( ( unsigned short ) conn->svrVersion->reconnPort );

        conn->reconnectedSock =
            connectToRhostWithRaddr( &remoteAddr, conn->windowSize, 0 );

        if ( conn->reconnectedSock < 0 ) {
            conn->cond->notify_all();
            boost_lock.unlock();
            rodsLog( LOG_ERROR,
                     "cliReconnManager: connect to host %s failed, status = %d",
                     conn->svrVersion->reconnAddr, conn->reconnectedSock );
            rodsSleep( RECONNECT_SLEEP_TIME, 0 );
            continue;
        }

        bzero( &reconnMsg, sizeof( procState_t ) );
        reconnMsg.procState = conn->clientState;
        reconnMsg.cookie    = conn->svrVersion->cookie;

        // =-=-=-=-=-=-=-
        // create network object, need to override the socket
        // with the reconn socket.  no way to infer this in the
        // factory for the client comm
        irods::network_object_ptr net_obj;
        irods::error ret = irods::network_factory( conn, net_obj );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
        }

        net_obj->socket_handle( conn->reconnectedSock ); // repave w/ recon socket
        ret = sendReconnMsg( net_obj, &reconnMsg );
        if ( !ret.ok() ) {
            close( conn->reconnectedSock );
            conn->reconnectedSock = 0;
            conn->cond->notify_all();
            boost_lock.unlock();
            rodsLog( LOG_ERROR,
                     "cliReconnManager: sendReconnMsg to host %s failed, status = %d",
                     conn->svrVersion->reconnAddr, ret.code() );
            rodsSleep( RECONNECT_SLEEP_TIME, 0 );
            continue;
        }

        ret = readReconMsg( net_obj, &reconnMsgOut );
        if ( !ret.ok() ) {
            close( conn->reconnectedSock );
            conn->reconnectedSock = 0;
            conn->cond->notify_all();
            boost_lock.unlock();
            rodsLog( LOG_ERROR,
                     "cliReconnManager: readReconMsg to host %s failed, status = %d",
                     conn->svrVersion->reconnAddr, ret.code() );
            rodsSleep( RECONNECT_SLEEP_TIME, 0 );
            continue;
        }

        conn->agentState = reconnMsgOut->procState;
        free( reconnMsgOut );
        reconnMsgOut = NULL;
        conn->reconnTime = time( 0 ) + RECONN_TIMEOUT_TIME;
        if ( conn->clientState == PROCESSING_STATE ) {
            rodsLog( LOG_DEBUG,
                     "cliReconnManager: svrSwitchConnect. cliState = %d,agState=%d",
                     conn->clientState, conn->agentState );
            cliSwitchConnect( conn );
        }
        else {
            rodsLog( LOG_DEBUG,
                     "cliReconnManager: Not calling svrSwitchConnect,  clientState = %d",
                     conn->clientState );
        }
        conn->cond->notify_all();
        boost_lock.unlock();
    }
}

int
cliChkReconnAtSendStart( rcComm_t *conn ) {
    if ( conn->svrVersion != NULL && conn->svrVersion->reconnPort > 0 ) {
        /* handle reconn */
        boost::unique_lock<boost::mutex> boost_lock( *conn->lock );
        if ( conn->reconnThrState == CONN_WAIT_STATE ) {
            rodsLog( LOG_DEBUG,
                     "cliChkReconnAtSendStart:ThrState=CONN_WAIT_STATE,clientState=%d",
                     conn->clientState );
            conn->clientState = PROCESSING_STATE;

            conn->cond->notify_all();
            /* wait for reconnManager to get done */
            conn->cond->wait( boost_lock );

        }
        conn->clientState = SENDING_STATE;
        boost_lock.unlock();
    }
    return 0;
}

int
cliChkReconnAtSendEnd( rcComm_t *conn ) {
    if ( conn->svrVersion != NULL && conn->svrVersion->reconnPort > 0 ) {
        /* handle reconn */
        boost::unique_lock<boost::mutex> boost_lock( *conn->lock );
        conn->clientState = PROCESSING_STATE;
        if ( conn->reconnThrState == CONN_WAIT_STATE ) {
            conn->cond->notify_all();
        }

        boost_lock.unlock();
    }
    return 0;
}

int
cliChkReconnAtReadStart( rcComm_t *conn ) {
    if ( conn->svrVersion != NULL && conn->svrVersion->reconnPort > 0 ) {
        /* handle reconn */
        boost::unique_lock<boost::mutex> boost_lock( *conn->lock );
        conn->clientState = RECEIVING_STATE;
        boost_lock.unlock();
    }
    return 0;
}

int
cliChkReconnAtReadEnd( rcComm_t *conn ) {
    if ( conn->svrVersion != NULL && conn->svrVersion->reconnPort > 0 ) {
        /* handle reconn */
        boost::unique_lock<boost::mutex> boost_lock( *conn->lock );
        conn->clientState = PROCESSING_STATE;
        if ( conn->reconnThrState == CONN_WAIT_STATE ) {
            rodsLog( LOG_DEBUG,
                     "cliChkReconnAtReadEnd:ThrState=CONN_WAIT_STATE, clientState=%d",
                     conn->clientState );

            conn->cond->notify_all();
            /* wait for reconnManager to get done */
            conn->cond->wait( boost_lock );
        }
        boost_lock.unlock();
    }
    return 0;
}


