/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rsApiHandler.c - The server API Handler. handle RODS_API_REQ_T type
 * messages
 */

#include "rsApiHandler.hpp"
#include "modDataObjMeta.hpp"
#include "rcMisc.hpp"
#include "miscServerFunct.hpp"
#include "regReplica.hpp"
#include "unregDataObj.hpp"
#include "modAVUMetadata.hpp"
#include "sockComm.hpp"

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <setjmp.h>
jmp_buf Jenv;

// =-=-=-=-=-=-=-
// irods includes
#include "irods_network_factory.hpp"
#include "irods_server_api_table.hpp"
#include "irods_threads.hpp"
#include "sockCommNetworkInterface.hpp"


int rsApiHandler(
    rsComm_t*   rsComm,
    int         apiNumber,
    bytesBuf_t* inputStructBBuf,
    bytesBuf_t* bsBBuf ) {
    int apiInx;
    int status = 0;
    char *myInStruct = NULL;
    funcPtr myHandler = NULL;
    void *myOutStruct = NULL;
    bytesBuf_t myOutBsBBuf;
    int retVal = 0;
    int numArg = 0;
    void *myArgv[4];
    memset( &myOutBsBBuf, 0, sizeof( bytesBuf_t ) );
    memset( &rsComm->rError, 0, sizeof( rError_t ) );

    apiInx = apiTableLookup( apiNumber );

    // =-=-=-=-=-=-=-
    // create a network object
    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( rsComm, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return apiInx;
    }

    if ( apiInx < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsApiHandler: apiTableLookup of apiNumber %d failed", apiNumber );
        /* cannot use sendApiReply because it does not know apiInx */
        sendRodsMsg( net_obj, RODS_API_REPLY_T, NULL, NULL, NULL,
                     apiInx, rsComm->irodsProt );
        return apiInx;
    }

    rsComm->apiInx = apiInx;

    status = chkApiVersion( apiInx );
    if ( status < 0 ) {
        sendApiReply( rsComm, apiInx, status, myOutStruct, &myOutBsBBuf );
        return status;
    }

    status = chkApiPermission( rsComm, apiInx );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsApiHandler: User has no permission for apiNumber %d", apiNumber );
        sendApiReply( rsComm, apiInx, status, myOutStruct, &myOutBsBBuf );
        return status;
    }

    irods::api_entry_table& RsApiTable = irods::get_server_api_table();

    /* some sanity check */
    if ( inputStructBBuf->len > 0 && RsApiTable[apiInx]->inPackInstruct == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsApiHandler: input struct error 1 for apiNumber %d", apiNumber );
        sendApiReply( rsComm, apiInx, SYS_API_INPUT_ERR, myOutStruct,
                      &myOutBsBBuf );
        return SYS_API_INPUT_ERR;
    }

    if ( inputStructBBuf->len <= 0 && RsApiTable[apiInx]->inPackInstruct != NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsApiHandler: input struct error 2 for apiNumber %d", apiNumber );
        sendApiReply( rsComm, apiInx, SYS_API_INPUT_ERR, myOutStruct,
                      &myOutBsBBuf );
        return SYS_API_INPUT_ERR;
    }

    if ( bsBBuf->len > 0 && RsApiTable[apiInx]->inBsFlag <= 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsApiHandler: input byte stream error for apiNumber %d", apiNumber );
        sendApiReply( rsComm, apiInx, SYS_API_INPUT_ERR, myOutStruct,
                      &myOutBsBBuf );
        return SYS_API_INPUT_ERR;
    }

    if ( inputStructBBuf->len > 0 ) {
        status = unpackStruct( inputStructBBuf->buf, ( void ** )( static_cast< void * >( &myInStruct ) ),
                               ( char* )RsApiTable[apiInx]->inPackInstruct, RodsPackTable, rsComm->irodsProt );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "rsApiHandler: unpackStruct error for apiNumber %d, status = %d",
                     apiNumber, status );
            sendApiReply( rsComm, apiInx, status, myOutStruct,
                          &myOutBsBBuf );
            return status;
        }
    }

    /* ready to call the handler functions */

    myHandler = RsApiTable[apiInx]->svrHandler;
    if ( !myHandler ) {
        rodsLog( LOG_ERROR, "Null handler encountered for api number %d in rsApiHandler.", apiNumber );
        return SYS_API_INPUT_ERR;
    }

    if ( RsApiTable[apiInx]->inPackInstruct != NULL ) {
        myArgv[numArg] = myInStruct;
        numArg++;
    };

    if ( RsApiTable[apiInx]->inBsFlag != 0 ) {
        myArgv[numArg] = bsBBuf;
        numArg++;
    };

    if ( RsApiTable[apiInx]->outPackInstruct != NULL ) {
        myArgv[numArg] = ( void * ) &myOutStruct;
        numArg++;
    };

    if ( RsApiTable[apiInx]->outBsFlag != 0 ) {
        myArgv[numArg] = ( void * ) &myOutBsBBuf;
        numArg++;
    };

    if ( numArg == 0 ) {
        retVal = ( *myHandler )( rsComm );
    }
    else if ( numArg == 1 ) {
        retVal = ( *myHandler )( rsComm, myArgv[0] );
    }
    else if ( numArg == 2 ) {
        retVal = ( *myHandler )( rsComm, myArgv[0], myArgv[1] );
    }
    else if ( numArg == 3 ) {
        retVal = ( *myHandler )( rsComm, myArgv[0], myArgv[1], myArgv[2] );
    }
    else if ( numArg == 4 ) {
        retVal = ( *myHandler )( rsComm, myArgv[0], myArgv[1], myArgv[2],
                                 myArgv[3] );
    }

    if ( retVal != SYS_NO_HANDLER_REPLY_MSG ) {
        status = sendAndProcApiReply
                 ( rsComm, apiInx, retVal, myOutStruct, &myOutBsBBuf );
    }

    // =-=-=-=-=-=-=-
    // clear the incoming packing instruction
    if ( myInStruct != NULL ) {
        if ( RsApiTable[apiInx]->clearInStruct ) {
            RsApiTable[apiInx]->clearInStruct( myInStruct );
        }

        free( myInStruct );
        myInStruct = NULL;
    }

    if ( retVal >= 0 && status < 0 ) {
        return status;
    }
    else {
        return retVal;
    }
}

int
sendAndProcApiReply( rsComm_t * rsComm, int apiInx, int status,
                     void * myOutStruct, bytesBuf_t * myOutBsBBuf ) {
    int retval;

    retval = sendApiReply( rsComm, apiInx, status, myOutStruct, myOutBsBBuf );

    clearBBuf( myOutBsBBuf );
    if ( myOutStruct != NULL ) {
        free( myOutStruct );
    }
    freeRErrorContent( &rsComm->rError );

    /* check for portal operation */

    if ( rsComm->portalOpr != NULL ) {
        handlePortalOpr( rsComm );
        clearKeyVal( &rsComm->portalOpr->dataOprInp.condInput );
        free( rsComm->portalOpr );
        rsComm->portalOpr = NULL;
    }

    return retval;
}

int
sendApiReply( rsComm_t * rsComm, int apiInx, int retVal,
              void * myOutStruct, bytesBuf_t * myOutBsBBuf ) {
    int status = 0;
    bytesBuf_t *outStructBBuf = NULL;
    bytesBuf_t *myOutStructBBuf;
    bytesBuf_t *rErrorBBuf = NULL;
    bytesBuf_t *myRErrorBBuf;

//#ifndef windows_platform
    svrChkReconnAtSendStart( rsComm );
//#endif

    if ( retVal == SYS_HANDLER_DONE_NO_ERROR ) {
        /* not actually an error */
        retVal = 0;
    }

    // =-=-=-=-=-=-=-
    // create a network object
    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( rsComm, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }


    irods::api_entry_table& RsApiTable = irods::get_server_api_table();
    if ( RsApiTable[apiInx]->outPackInstruct != NULL && myOutStruct != NULL ) {

        status = packStruct( ( char * ) myOutStruct, &outStructBBuf,
                             ( char* )RsApiTable[apiInx]->outPackInstruct, RodsPackTable, FREE_POINTER,
                             rsComm->irodsProt );

        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "sendApiReply: packStruct error, status = %d", status );
            sendRodsMsg( net_obj, RODS_API_REPLY_T, NULL,
                         NULL, NULL, status, rsComm->irodsProt );
            svrChkReconnAtSendEnd( rsComm );
            return status;
        }

        myOutStructBBuf = outStructBBuf;
    }
    else {
        myOutStructBBuf = NULL;
    }

    if ( RsApiTable[apiInx]->outBsFlag == 0 ) {
        myOutBsBBuf = NULL;
    }

    if ( rsComm->rError.len > 0 ) {
        status = packStruct( ( char * ) &rsComm->rError, &rErrorBBuf,
                             "RError_PI", RodsPackTable, 0, rsComm->irodsProt );

        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "sendApiReply: packStruct error, status = %d", status );
            sendRodsMsg( net_obj, RODS_API_REPLY_T, NULL,
                         NULL, NULL, status, rsComm->irodsProt );
            svrChkReconnAtSendEnd( rsComm );
            freeBBuf( outStructBBuf );
            freeBBuf( rErrorBBuf );
            return status;
        }

        myRErrorBBuf = rErrorBBuf;
    }
    else {
        myRErrorBBuf = NULL;
    }
    ret = sendRodsMsg( net_obj, RODS_API_REPLY_T, myOutStructBBuf,
                       myOutBsBBuf, myRErrorBBuf, retVal, rsComm->irodsProt );
    if ( !ret.ok() ) {
        int status1;
        irods::log( PASS( ret ) );

        if ( rsComm->reconnSock > 0 ) {
            int savedStatus = ret.code();
            boost::unique_lock< boost::mutex > boost_lock( *rsComm->thread_ctx->lock );
            rodsLog( LOG_DEBUG,
                     "sendApiReply: svrSwitchConnect. cliState = %d,agState=%d",
                     rsComm->clientState, rsComm->agentState );
            status1 = svrSwitchConnect( rsComm );
            boost_lock.unlock();
            if ( status1 > 0 ) {
                /* should not be here */
                rodsLog( LOG_NOTICE,
                         "sendApiReply: Switch connection and retry sendRodsMsg" );
                ret = sendRodsMsg( net_obj, RODS_API_REPLY_T,
                                   myOutStructBBuf, myOutBsBBuf, myRErrorBBuf,
                                   retVal, rsComm->irodsProt );

                if ( ret.code() >= 0 ) {
                    rodsLog( LOG_NOTICE,
                             "sendApiReply: retry sendRodsMsg succeeded" );
                }
                else {
                    status = savedStatus;
                }
            }
        }
    }

    svrChkReconnAtSendEnd( rsComm );

    freeBBuf( outStructBBuf );
    freeBBuf( rErrorBBuf );

    return status;
}

int
chkApiVersion( int apiInx ) {
    char *cliApiVersion;

    irods::api_entry_table& RsApiTable = irods::get_server_api_table();
    if ( ( cliApiVersion = getenv( SP_API_VERSION ) ) != NULL ) {
        if ( strcmp( cliApiVersion, RsApiTable[apiInx]->apiVersion ) != 0 ) {
            rodsLog( LOG_ERROR,
                     "chkApiVersion:Client's API Version %s does not match Server's %s",
                     cliApiVersion, RsApiTable[apiInx]->apiVersion );
            return USER_API_VERSION_MISMATCH;
        }
    }
    return 0;
}

int
chkApiPermission( rsComm_t * rsComm, int apiInx ) {
    int clientUserAuth;
    int proxyUserAuth;
    int xmsgSvrOnly;
    int xmsgSvrAlso;

    irods::api_entry_table& RsApiTable = irods::get_server_api_table();
    clientUserAuth = RsApiTable[apiInx]->clientUserAuth;

    xmsgSvrOnly = clientUserAuth & XMSG_SVR_ONLY;
    xmsgSvrAlso = clientUserAuth & XMSG_SVR_ALSO;

    if ( ProcessType == XMSG_SERVER_PT ) {
        if ( ( xmsgSvrOnly + xmsgSvrAlso ) == 0 ) {
            rodsLog( LOG_ERROR,
                     "chkApiPermission: xmsgServer not allowed to handle api %d",
                     RsApiTable[apiInx]->apiNumber );
            return SYS_NO_API_PRIV;
        }
    }
    else if ( xmsgSvrOnly != 0 ) {
        rodsLog( LOG_ERROR,
                 "chkApiPermission: non xmsgServer not allowed to handle api %d",
                 RsApiTable[apiInx]->apiNumber );
        return SYS_NO_API_PRIV;
    }

    if ( ( strcmp( rsComm->proxyUser.userType, STORAGE_ADMIN_USER_TYPE ) == 0 )
            && ( clientUserAuth & STORAGE_ADMIN_USER ) ) {
        return 0;
    }

    clientUserAuth = clientUserAuth & 0xfff;	/* take out XMSG_SVR_* flags */

    if ( clientUserAuth > rsComm->clientUser.authInfo.authFlag ) {
        return SYS_NO_API_PRIV;
    }

    proxyUserAuth = RsApiTable[apiInx]->proxyUserAuth & 0xfff;
    if ( proxyUserAuth > rsComm->proxyUser.authInfo.authFlag ) {
        return SYS_NO_API_PRIV;
    }
    return 0;
}

int
handlePortalOpr( rsComm_t * rsComm ) {
    int oprType;
    int status;

    if ( rsComm == NULL || rsComm->portalOpr == NULL ) {
        return 0;
    }

    oprType = rsComm->portalOpr->oprType;

    switch ( oprType ) {
    case PUT_OPR:
    case GET_OPR:
        status = svrPortalPutGet( rsComm );
        break;
    default:
        rodsLog( LOG_NOTICE,
                 "handlePortalOpr: Invalid portal oprType: %d", oprType );
        status = SYS_INVALID_PORTAL_OPR;
        break;
    }
    return status;
}

int
readAndProcClientMsg( rsComm_t * rsComm, int flags ) {
    int status = 0;
    msgHeader_t myHeader;
    bytesBuf_t inputStructBBuf, bsBBuf, errorBBuf;

    bzero( &inputStructBBuf, sizeof( inputStructBBuf ) );
    bzero( &bsBBuf, sizeof( bsBBuf ) );
    bzero( &errorBBuf, sizeof( errorBBuf ) );

    svrChkReconnAtReadStart( rsComm );
    /* everything else are set in readMsgBody */

    /* read the header */

    // =-=-=-=-=-=-=-
    // create a network object
    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( rsComm, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    if ( ( flags & READ_HEADER_TIMEOUT ) != 0 ) {
        int retryCnt = 0;
        struct timeval tv;
        tv.tv_sec = READ_HEADER_TIMEOUT_IN_SEC;
        tv.tv_usec = 0;

        while ( 1 ) {
            ret = readMsgHeader( net_obj, &myHeader, &tv );
            if ( !ret.ok() ) {
                if ( isL1descInuse() && retryCnt < MAX_READ_HEADER_RETRY ) {
                    rodsLogError( LOG_ERROR, status,
                                  "readAndProcClientMsg:readMsgHeader error. status = %d",  ret.code() );
                    retryCnt++;
                    continue;
                }
                if ( ret.code() == USER_SOCK_CONNECT_TIMEDOUT ) {
                    rodsLog( LOG_ERROR,
                             "readAndProcClientMsg: readMsgHeader by pid %d timedout",
                             getpid() );
                    return  ret.code();
                }
            }
            break;
        } // while 1
    }
    else {
        ret = readMsgHeader( net_obj, &myHeader, NULL );
    }

    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        /* attempt to accept reconnect. ENOENT result  from * user cntl-C */
        if ( rsComm->reconnSock > 0 ) {
            int savedStatus = ret.code();
            /* try again. the socket might have changed */
            boost::unique_lock< boost::mutex > boost_lock( *rsComm->thread_ctx->lock );
            rodsLog( LOG_DEBUG,
                     "readAndProcClientMsg: svrSwitchConnect. cliState = %d,agState=%d",
                     rsComm->clientState, rsComm->agentState );
            svrSwitchConnect( rsComm );
            boost_lock.unlock();
            ret = readMsgHeader( net_obj, &myHeader, NULL );
            if ( !ret.ok() ) {
                svrChkReconnAtReadEnd( rsComm );
                return savedStatus;
            }
        }
        else {
            svrChkReconnAtReadEnd( rsComm );
            return ret.code();
        }
    } // if !ret.ok()

#ifdef SYS_TIMING
    if ( strcmp( myHeader.type, RODS_API_REQ_T ) == 0 ) {
        /* Get the total time of AUTH_REQUEST_AN and AUTH_RESPONSE_AN */
        if ( myHeader.intInfo != AUTH_RESPONSE_AN ) {
            initSysTiming( "irodsAgent", "recv request", 0 );
        }
    }
#endif
    ret = readMsgBody( net_obj, &myHeader, &inputStructBBuf,
                       &bsBBuf, &errorBBuf, rsComm->irodsProt, NULL );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        svrChkReconnAtReadEnd( rsComm );
        return ret.code();
    }

    svrChkReconnAtReadEnd( rsComm );

    /* handler switch by msg type */

    if ( strcmp( myHeader.type, RODS_API_REQ_T ) == 0 ) {
        status = rsApiHandler( rsComm, myHeader.intInfo, &inputStructBBuf,
                               &bsBBuf );
#ifdef SYS_TIMING
        char tmpStr[NAME_LEN];
        snprintf( tmpStr, NAME_LEN, "handle API %d", myHeader.intInfo );
        printSysTiming( "irodsAgent", tmpStr, 0 );
#endif
        clearBBuf( &inputStructBBuf );
        clearBBuf( &bsBBuf );
        clearBBuf( &errorBBuf );

        if ( ( flags & RET_API_STATUS ) != 0 ) {
            return status;
        }
        else {
            return 0;
        }
    }
    else if ( strcmp( myHeader.type, RODS_DISCONNECT_T ) == 0 ) {
        rodsLog( LOG_NOTICE,
                 "readAndProcClientMsg: received disconnect msg from client" );

        return DISCONN_STATUS;
    }
    else if ( strcmp( myHeader.type, RODS_RECONNECT_T ) == 0 ) {
        rodsLog( LOG_NOTICE,
                 "readAndProcClientMsg: received reconnect msg from client" );
        /* call itself again. be careful */
        status = readAndProcClientMsg( rsComm, flags );
        return status;
    }
    else {
        rodsLog( LOG_NOTICE,
                 "agentMain: msg type %s not support by server",
                 myHeader.type );
        return USER_MSG_TYPE_NO_SUPPORT;
    }
}

/* sendAndRecvBranchMsg - Break out the normal mode of
 * clientReuest/serverReply protocol for handling API. Instead of returning
 * to rsApiHandler() and send a API reply, it sends a reply directly to
 * the client through sendAndProcApiReply.
 * Then it loops though readAndProcClientMsg() to process addtitional
 * clients requests until the status is SYS_HANDLER_DONE_NO_ERROR
 * which is generated by a rcOprComplete() call by the client. The client
 * must remember to send a rcOprComplete() call or the server will hang
 * in this loop.
 * The caller of this routine should return a SYS_NO_HANDLER_REPLY_MSG
 * status to rsApiHandler() if the client is not expecting any more
 * reply msg.
 */

int
sendAndRecvBranchMsg( rsComm_t * rsComm, int apiInx, int status,
                      void * myOutStruct, bytesBuf_t * myOutBsBBuf ) {
    int retval;
    int savedApiInx;

    savedApiInx = rsComm->apiInx;
    retval = sendAndProcApiReply( rsComm, apiInx, status,
                                  myOutStruct, myOutBsBBuf );
    if ( retval < 0 ) {
        rodsLog( LOG_ERROR,
                 "sendAndRecvBranchMsg: sendAndProcApiReply error. status = %d", retval );
        rsComm->apiInx = savedApiInx;
        return retval;
    }

    while ( 1 )  {
        retval = readAndProcClientMsg( rsComm, RET_API_STATUS );
        if ( retval >= 0 || retval == SYS_NO_HANDLER_REPLY_MSG ) {
            /* more to come */
            continue;
        }
        else {
            rsComm->apiInx = savedApiInx;
            if ( retval == SYS_HANDLER_DONE_NO_ERROR ) {
                return 0;
            }
            else {
                return retval;
            }
        }
    }
}

int
svrSendCollOprStat( rsComm_t * rsComm, collOprStat_t * collOprStat ) {
    int status;

    status = _svrSendCollOprStat( rsComm, collOprStat );

    if ( status != SYS_CLI_TO_SVR_COLL_STAT_REPLY ) {
        rodsLog( LOG_ERROR,
                 "svrSendCollOprStat: client reply %d != %d.",
                 status, SYS_CLI_TO_SVR_COLL_STAT_REPLY );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else {
        return 0;
    }
}

int
_svrSendCollOprStat( rsComm_t * rsComm, collOprStat_t * collOprStat ) {
    int myBuf;
    int status;

    status = sendAndProcApiReply( rsComm, rsComm->apiInx,
                                  SYS_SVR_TO_CLI_COLL_STAT, collOprStat, NULL );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "svrSendCollOprStat: sendAndProcApiReply failed. status = %d",
                      status );
        return status;
    }

    /* read 4 bytes */
    status = myRead( rsComm->sock, &myBuf, sizeof( myBuf ), NULL,
                     NULL );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "svrSendCollOprStat: read handshake failed. status = %d", status );
    }
    return ntohl( myBuf );
}

int
svrSendZoneCollOprStat( rsComm_t * rsComm, rcComm_t * conn,
                        collOprStat_t * collOprStat, int retval ) {
    int status = retval;

    while ( status == SYS_SVR_TO_CLI_COLL_STAT ) {
        status = _svrSendCollOprStat( rsComm, collOprStat );
        if ( status == SYS_CLI_TO_SVR_COLL_STAT_REPLY ) {
            status = _cliGetCollOprStat( conn, &collOprStat );
        }
        else {
            int myBuf = htonl( status );
            myWrite( conn->sock, ( void * ) &myBuf, 4, NULL );
            break;
        }
    }
    return status;
}

void
readTimeoutHandler( int ) {
    alarm( 0 );
    if ( isL1descInuse() ) {
        rodsLog( LOG_ERROR,
                 "readTimeoutHandler: read header by %d timed out. Lidesc is busy.",
                 getpid() );
        longjmp( Jenv, L1DESC_INUSE );
    }
    else {
        rodsLog( LOG_ERROR,
                 "readTimeoutHandler: read header by %d has timed out.",
                 getpid() );
        longjmp( Jenv, READ_HEADER_TIMED_OUT );
    }
}
