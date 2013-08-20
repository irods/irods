/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* procApiRequest.c - process API request
 */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_network_factory.h"

// =-=-=-=-=-=-=-
// irods includes
#include "procApiRequest.h"
#include "rcGlobalExtern.h"
#include "rcMisc.h"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

#ifdef USE_BOOST
#else
//#ifndef windows_platform
#include <pthread.h>
//#endif
#endif

/* procApiRequest - This is the main function used by the client API
 * function to issue API requests and recieve output returned from 
 * the server.
 * rcComm_t *conn - the client communication handle.
 * int apiNumber - the API number of this call defined in apiNumber.h.
 * void *inputStruct - pointer to the input struct of this API. If there
 *     is no input struct, a NULL should be entered
 * bytesBuf_t *inputBsBBuf - pointer to the input byte stream. If there
 *     is no input byte stream, a NULL should be entered
 * void **outStruct - Pointer to pointer to the output struct. The outStruct 
 *     will be allocated by this function and the pointer to this struct is
 *     passed back to the caller through this pointer. If there
 *     is no output struct, a NULL should be entered
 * bytesBuf_t *outBsBBuf - pointer to the output byte stream. If there
 *     is no output byte stream, a NULL should be entered
 * 
 */

int
procApiRequest (rcComm_t *conn, int apiNumber, void *inputStruct,
                bytesBuf_t *inputBsBBuf, void **outStruct, bytesBuf_t *outBsBBuf)
{
    int status;
    int apiInx;

    if (conn == NULL) {
        return (USER__NULL_INPUT_ERR);
    }

    freeRError (conn->rError);
    conn->rError = NULL;
    
    apiInx = apiTableLookup (apiNumber);
    if (apiInx < 0) {
        rodsLog (LOG_ERROR,
                 "procApiRequest: apiTableLookup of apiNumber %d failed", apiNumber);
        return (apiInx);
    }

    status = sendApiRequest (conn, apiInx, inputStruct, inputBsBBuf);
    if (status < 0) {
        rodsLogError (LOG_DEBUG, status,
                      "procApiRequest: sendApiRequest failed. status = %d", status);
        return (status);
    }

    conn->apiInx = apiInx;

    status = readAndProcApiReply (conn, apiInx, outStruct, outBsBBuf);
    if (status < 0) {
        rodsLogError (LOG_DEBUG, status,
                      "procApiRequest: readAndProcApiReply failed. status = %d", status);
    }

    return (status);
}

int
branchReadAndProcApiReply (rcComm_t *conn, int apiNumber,
                           void **outStruct, bytesBuf_t *outBsBBuf)
{
    int status;
    int apiInx;

    if (conn == NULL) {
        return (USER__NULL_INPUT_ERR);
    }

    apiInx = apiTableLookup (apiNumber);
    if (apiInx < 0) {
        rodsLog (LOG_ERROR,
                 "branchReadAndProcApiReply: apiTableLookup of apiNum %d failed", 
                 apiNumber);
        return (apiInx);
    }

    conn->apiInx = apiInx;

    status = readAndProcApiReply (conn, apiInx, outStruct, outBsBBuf);
    if (status < 0) {
        rodsLogError (LOG_DEBUG, status,
                      "branchReadAndProcApiReply: readAndProcApiReply failed. status = %d",
                      status);
    }
    return (status);
}

int
sendApiRequest (rcComm_t *conn, int apiInx, void *inputStruct,
                bytesBuf_t *inputBsBBuf)
{
    int status;
    bytesBuf_t *inputStructBBuf = NULL;
    bytesBuf_t *myInputStructBBuf;

//#ifndef windows_platform
    cliChkReconnAtSendStart (conn);
//#endif

    if (RcApiTable[apiInx].inPackInstruct != NULL) {
        if (inputStruct == NULL) {
//#ifndef windows_platform
            cliChkReconnAtSendEnd (conn);
//#endif
            return (USER_API_INPUT_ERR);
        }
        status = packStruct ((void *) inputStruct, &inputStructBBuf,
                             RcApiTable[apiInx].inPackInstruct, RodsPackTable, 0, conn->irodsProt);

        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
                          "sendApiRequest: packStruct error, status = %d", status);
//#ifndef windows_platform
            cliChkReconnAtSendEnd (conn);
//#endif
            return status;
        }

        myInputStructBBuf = inputStructBBuf;
    } else {
        myInputStructBBuf = NULL;
    };


    if (RcApiTable[apiInx].inBsFlag <= 0) {
        inputBsBBuf = NULL;
    }
#if 0 // JMC - network interface
#ifdef USE_SSL
    if (conn->ssl_on)
        status = sslSendRodsMsg (conn->sock, RODS_API_REQ_T, myInputStructBBuf,
                              inputBsBBuf, NULL, RcApiTable[apiInx].apiNumber, 
                              conn->irodsProt, conn->ssl);
    else
#endif
        status = sendRodsMsg( conn->sock, RODS_API_REQ_T, myInputStructBBuf,
                              inputBsBBuf, NULL, RcApiTable[apiInx].apiNumber, conn->irodsProt);
#endif

    eirods::network_object_ptr net_obj;
    eirods::error ret = eirods::network_factory( conn, net_obj );
    if( !ret.ok() ) {
        eirods::log( PASS( ret ) );
        return ret.code();
    }
 
    ret = sendRodsMsg( 
              net_obj, 
              RODS_API_REQ_T, 
              myInputStructBBuf,
              inputBsBBuf, 
              NULL, 
              RcApiTable[apiInx].apiNumber, 
              conn->irodsProt );
    if( !ret.ok() ) {
        eirods::log( PASS( ret ) ); 
//#ifndef windows_platform
        if( conn->svrVersion != NULL && 
            conn->svrVersion->reconnPort > 0 ) {
            int status1;
            int savedStatus = ret.code() ;
#ifdef USE_BOOST
            conn->lock->lock();
#else
            pthread_mutex_lock (&conn->lock);
#endif
            status1 = cliSwitchConnect (conn);
            rodsLog (LOG_DEBUG,
                     "sendApiRequest: svrSwitchConnect. cliState = %d,agState=%d",
                     conn->clientState, conn->agentState);
#ifdef USE_BOOST
            conn->lock->unlock();
#else
            pthread_mutex_unlock (&conn->lock);
#endif
            if (status1 > 0) {
                /* should not be here */
                rodsLog (LOG_NOTICE,
                         "sendApiRequest: Switch connection and retry sendRodsMsg");
#if 0 // JMC - network interface
#ifdef USE_SSL
                if (conn->ssl_on) 
                    status = sslSendRodsMsg (conn->sock, RODS_API_REQ_T, 
                                          myInputStructBBuf, inputBsBBuf, NULL, 
                                             RcApiTable[apiInx].apiNumber, conn->irodsProt,
                                             conn->ssl);
                else
#endif
                    status = sendRodsMsg( conn->sock, RODS_API_REQ_T, 
                                          myInputStructBBuf, inputBsBBuf, NULL, 
                                          RcApiTable[apiInx].apiNumber, conn->irodsProt);
#endif
                ret = sendRodsMsg( 
                          net_obj, 
                          RODS_API_REQ_T, 
                          myInputStructBBuf,
                          inputBsBBuf, 
                          NULL, 
                          RcApiTable[apiInx].apiNumber, 
                          conn->irodsProt );
                if( !ret.ok() ) {
                    eirods::log( PASS( ret ) ); 
                } else {
                    status = savedStatus;
                }
            
            } // if status1 > 0

        } // if svrVersion != NULL ...
//#endif

    } // if !ret.ok

    freeBBuf (inputStructBBuf);

    return (status);
}

int
readAndProcApiReply (rcComm_t *conn, int apiInx, void **outStruct, 
                     bytesBuf_t *outBsBBuf)
{
    int status;
    msgHeader_t myHeader;
    /* bytesBuf_t outStructBBuf, errorBBuf, myOutBsBBuf; */
    bytesBuf_t outStructBBuf, errorBBuf;

#ifndef windows_platform
    cliChkReconnAtReadStart (conn);
#endif

    memset (&outStructBBuf, 0, sizeof (bytesBuf_t));
    memset (&outStructBBuf, 0, sizeof (bytesBuf_t));
    /* memset (&myOutBsBBuf, 0, sizeof (bytesBuf_t)); */

    /* some sanity check */

    if (RcApiTable[apiInx].outPackInstruct != NULL && outStruct == NULL) {
        rodsLog (LOG_ERROR,
                 "readAndProcApiReply: outStruct error for A apiNumber %d", 
                 RcApiTable[apiInx].apiNumber);
#ifndef windows_platform
        cliChkReconnAtReadEnd (conn);
#endif
        return (USER_API_INPUT_ERR);
    }

    if (RcApiTable[apiInx].outBsFlag > 0 && outBsBBuf == NULL) {
        rodsLog (LOG_ERROR,
                 "readAndProcApiReply: outBsBBuf error for B apiNumber %d",
                 RcApiTable[apiInx].apiNumber);
#ifndef windows_platform
        cliChkReconnAtReadEnd (conn);
#endif
        return (USER_API_INPUT_ERR);
    }

#if 0 // JMC - network interface
#ifdef USE_SSL
    if (conn->ssl_on) 
        status = sslReadMsgHeader (conn->sock, &myHeader, NULL, conn->ssl);
    else
#endif
        status = readMsgHeader( conn->sock, &myHeader, NULL );
#endif

    eirods::network_object_ptr net_obj;
    eirods::error ret = eirods::network_factory( conn, net_obj );
    if( !ret.ok() ) {
        eirods::log( PASS( ret ) );
        return ret.code();
    }

    ret = readMsgHeader( net_obj, &myHeader, NULL );
    if( !ret.ok() ) {
        eirods::log( PASS( ret ) );
        //rodsLogError (LOG_ERROR, status,
        //              "readAndProcApiReply: readMsgHeader error. status = %d", status);
#ifndef windows_platform
        //rodsLog (LOG_DEBUG,
        //         "readAndProcClientMsg: readMsgHeader error. status = %d", ret.code() );
        if (conn->svrVersion != NULL && conn->svrVersion->reconnPort > 0) {
            int savedStatus = ret.code();
            /* try again. the socket might have changed */
#ifdef USE_BOOST
            conn->lock->lock();
#else
            pthread_mutex_lock (&conn->lock);
#endif
            rodsLog (LOG_DEBUG,
                     "readAndProcClientMsg:svrSwitchConnect.cliState = %d,agState=%d",
                     conn->clientState, conn->agentState);
            cliSwitchConnect (conn);
#ifdef USE_BOOST
            conn->lock->unlock();
#else
            pthread_mutex_unlock (&conn->lock);
#endif
#if 0 // JMC - network interface
#ifdef USE_SSL
            if (conn->ssl_on)
                status = sslReadMsgHeader (conn->sock, &myHeader, NULL, conn->ssl);
            else
#endif
                status = readMsgHeader (conn->sock, &myHeader, NULL);
#endif
                eirods::error ret = readMsgHeader( net_obj, &myHeader, NULL );

            if( !ret.ok() ) {
                cliChkReconnAtReadEnd (conn);
                return (savedStatus);
            }
        } else {
            cliChkReconnAtReadEnd (conn);
            return ( ret.code() );
        }
#else
        return ( ret.code() );
#endif
    } // if !ret.ok

#if 0 // JMC - network interface
#ifdef USE_SSL
    if (conn->ssl_on)
        status = sslReadMsgBody (conn->sock, &myHeader, &outStructBBuf, outBsBBuf,
                              &errorBBuf, conn->irodsProt, NULL, conn->ssl);
    else
#endif
        status = readMsgBody( conn->sock, &myHeader, &outStructBBuf, outBsBBuf,
                              &errorBBuf, conn->irodsProt, NULL);
#endif

    ret = readMsgBody( net_obj, &myHeader, &outStructBBuf, outBsBBuf,
                       &errorBBuf, conn->irodsProt, NULL );
    if( !ret.ok() ) {
        eirods::log( PASS( ret ) );
        //rodsLogError(LOG_ERROR, status,
        //              "readAndProcApiReply: readMsgBody error. status = %d", status);
#ifndef windows_platform
        cliChkReconnAtReadEnd (conn);
#endif
        return (status);
    } // if !ret.ok

#ifndef windows_platform
    cliChkReconnAtReadEnd (conn);
#endif

    if (strcmp (myHeader.type, RODS_API_REPLY_T) == 0) {
        status = procApiReply (conn, apiInx, outStruct, outBsBBuf,
                               &myHeader, &outStructBBuf, NULL, &errorBBuf); 
    }

    clearBBuf (&outStructBBuf);
    /* clearBBuf (&myOutBsBBuf); */
    clearBBuf (&errorBBuf);

    return (status);
}

int
procApiReply (rcComm_t *conn, int apiInx, void **outStruct,
              bytesBuf_t *outBsBBuf,
              msgHeader_t *myHeader, bytesBuf_t *outStructBBuf, bytesBuf_t *myOutBsBBuf, 
              bytesBuf_t *errorBBuf)
{
    int status;
    int retVal;

    if (errorBBuf->len > 0) {
        status = unpackStruct (errorBBuf->buf, (void **) &conn->rError,
                               "RError_PI", RodsPackTable, conn->irodsProt);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
                          "readAndProcApiReply:unpackStruct error. status = %d",
                          status);
        }
    }

    retVal = myHeader->intInfo;

    /* some sanity check */

    if (RcApiTable[apiInx].outPackInstruct != NULL && outStruct == NULL) {
        rodsLog (LOG_ERROR,
                 "readAndProcApiReply: outStruct error for C apiNumber %d",
                 RcApiTable[apiInx].apiNumber);
        if (retVal < 0)
            return retVal;
        else {
            return (USER_API_INPUT_ERR);
        }
    }

    if (RcApiTable[apiInx].outBsFlag > 0 && outBsBBuf == NULL) {
        rodsLog (LOG_ERROR,
                 "readAndProcApiReply: outBsBBuf error for D apiNumber %d",
                 RcApiTable[apiInx].apiNumber);
        if (retVal < 0)
            return retVal;
        else {
            return (USER_API_INPUT_ERR);
        }
    }

    /* handle outStruct */
    if (outStructBBuf->len > 0) {
        if (outStruct != NULL) {
            status = unpackStruct (outStructBBuf->buf, (void **) outStruct,
                                   RcApiTable[apiInx].outPackInstruct, RodsPackTable, 
                                   conn->irodsProt);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                              "readAndProcApiReply:unpackStruct error. status = %d",
                              status);
                if (retVal < 0)
                    return retVal;
                else
                    return (status);
            }
        } else {
            rodsLog (LOG_ERROR,
                     "readAndProcApiReply: got unneeded outStruct for apiNumber %d",
                     RcApiTable[apiInx].apiNumber);
        }
    }

    if (myOutBsBBuf != NULL && myOutBsBBuf->len > 0) {
        if (outBsBBuf != NULL) {
            /* copy to out */
            *outBsBBuf = *myOutBsBBuf;
            memset (myOutBsBBuf, 0, sizeof (bytesBuf_t));
        } else {
            rodsLog (LOG_ERROR,
                     "readAndProcApiReply: got unneeded outBsBBuf for apiNumber %d",
                     RcApiTable[apiInx].apiNumber);
        }
    }

    return retVal;
}

int 
cliGetCollOprStat (rcComm_t *conn, collOprStat_t *collOprStat, int vFlag, 
                   int retval)
{
    int status = retval;

    while (status == SYS_SVR_TO_CLI_COLL_STAT) {
        /* more to come */
        if (collOprStat != NULL) {
            if (vFlag != 0) {
                printf ("num files done = %d, ", collOprStat->filesCnt);
                if (collOprStat->totalFileCnt <= 0) {
                    printf ("totalFileCnt = UNKNOWN, ");
                } else {
                    printf ("totalFileCnt = %d, ", collOprStat->totalFileCnt);
                }
                printf ("bytesWritten = %lld, last file done: %s\n",
                        collOprStat->bytesWritten, collOprStat->lastObjPath);
            }
            free (collOprStat);
            collOprStat = NULL;
        }
        status = _cliGetCollOprStat (conn, &collOprStat);
    }

    if (collOprStat != NULL) {
        free (collOprStat);
    }

    return (status);
}

int
_cliGetCollOprStat (rcComm_t *conn, collOprStat_t **collOprStat)
{
    int myBuf;
    int status;

    myBuf = htonl (SYS_CLI_TO_SVR_COLL_STAT_REPLY);
    status = myWrite (conn->sock, (void *) &myBuf, 4, SOCK_TYPE, NULL);
    status = readAndProcApiReply (conn, conn->apiInx,
                                  (void **) collOprStat, NULL);

    return (status);
}

