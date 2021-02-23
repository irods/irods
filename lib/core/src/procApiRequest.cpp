#include "procApiRequest.h"

#include "irods_network_factory.hpp"
#include "irods_client_api_table.hpp"

#if defined(RODS_SERVER) || defined(RODS_CLERVER)
    #include "irods_server_api_table.hpp"
#endif // RODS_SERVER || RODS_CLERVER

#include "rcGlobalExtern.h"
#include "rcMisc.h"
#include "sockComm.h"
#include "sockCommNetworkInterface.hpp"
#include "sslSockComm.h"
#include "irods_client_server_negotiation.hpp"

#include <iostream>

#include <boost/shared_ptr.hpp>

#include "irods_threads.hpp"

/* procApiRequest - This is the main function used by the client API
 * function to issue API requests and receive output returned from
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
int procApiRequest(rcComm_t *conn,
                   int apiNumber,
                   const void *inputStruct,
                   const bytesBuf_t *inputBsBBuf,
                   void **outStruct,
                   bytesBuf_t *outBsBBuf)
{
    if (!conn) {
        return USER__NULL_INPUT_ERR;
    }

    freeRError(conn->rError);
    conn->rError = nullptr;

    const int apiInx = apiTableLookup(apiNumber);

    if (apiInx < 0) {
        rodsLog(LOG_ERROR, "procApiRequest: apiTableLookup of apiNumber %d failed", apiNumber);
        return apiInx;
    }

    if (const auto ec = sendApiRequest(conn, apiInx, inputStruct, inputBsBBuf); ec < 0) {
        rodsLogError(LOG_DEBUG, ec, "procApiRequest: sendApiRequest failed. status = %d", ec);
        return ec;
    }

    conn->apiInx = apiInx;

    const auto ec = readAndProcApiReply(conn, apiInx, outStruct, outBsBBuf);
    if (ec < 0) {
        rodsLogError(LOG_DEBUG, ec, "procApiRequest: readAndProcApiReply failed. status = %d", ec);
    }

    return ec;
}

int branchReadAndProcApiReply( rcComm_t *conn, int apiNumber, void **outStruct, bytesBuf_t *outBsBBuf )
{
    if ( conn == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    int apiInx = apiTableLookup( apiNumber );
    if ( apiInx < 0 ) {
        rodsLog( LOG_ERROR,
                 "branchReadAndProcApiReply: apiTableLookup of apiNum %d failed",
                 apiNumber );
        return apiInx;
    }

    conn->apiInx = apiInx;

    int status = readAndProcApiReply( conn, apiInx, outStruct, outBsBBuf );
    if ( status < 0 ) {
        rodsLogError( LOG_DEBUG, status,
                      "branchReadAndProcApiReply: readAndProcApiReply failed. status = %d",
                      status );
    }

    return status;
}

int sendApiRequest( rcComm_t *conn, int apiInx, const void *inputStruct, const bytesBuf_t *inputBsBBuf )
{
    int status = 0;
    bytesBuf_t *inputStructBBuf = NULL;
    bytesBuf_t *myInputStructBBuf = NULL;

    cliChkReconnAtSendStart( conn );

    irods::api_entry_table& RcApiTable = irods::get_client_api_table();

    irods::api_entry_table::iterator itr = RcApiTable.find( apiInx );
    if ( itr == RcApiTable.end() ) {
        rodsLogError( LOG_ERROR, SYS_UNMATCHED_API_NUM, "API Entry not found at index %d", apiInx );
        return SYS_UNMATCHED_API_NUM;
    }


    if ( RcApiTable[apiInx]->inPackInstruct != NULL ) {
        if ( inputStruct == NULL ) {
            cliChkReconnAtSendEnd( conn );
            return USER_API_INPUT_ERR;
        }
        status = pack_struct(inputStruct,
                             &inputStructBBuf,
                             const_cast<char*>(RcApiTable[apiInx]->inPackInstruct),
                             RodsPackTable, 0, conn->irodsProt, conn->svrVersion->relVersion);
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "sendApiRequest: pack_struct error, status = %d", status );
            cliChkReconnAtSendEnd( conn );
            return status;
        }

        myInputStructBBuf = inputStructBBuf;
    }
    else {
        myInputStructBBuf = NULL;
    }

    if ( RcApiTable[apiInx]->inBsFlag <= 0 ) {
        inputBsBBuf = NULL;
    }

    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( conn, net_obj );
    if ( !ret.ok() ) {
        freeBBuf( inputStructBBuf );
        irods::log( PASS( ret ) );
        return ret.code();
    }

    ret = sendRodsMsg(
              net_obj,
              RODS_API_REQ_T,
              myInputStructBBuf,
              inputBsBBuf,
              NULL,
              RcApiTable[apiInx]->apiNumber,
              conn->irodsProt );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        if ( conn->svrVersion != NULL &&
                conn->svrVersion->reconnPort > 0 ) {
            int status1;
            int savedStatus = ret.code() ;
            conn->thread_ctx->lock->lock();
            status1 = cliSwitchConnect( conn );
            rodsLog( LOG_DEBUG,
                     "sendApiRequest: svrSwitchConnect. cliState = %d,agState=%d",
                     conn->clientState, conn->agentState );
            conn->thread_ctx->lock->unlock();
            if ( status1 > 0 ) {
                /* should not be here */
                rodsLog( LOG_NOTICE,
                         "sendApiRequest: Switch connection and retry sendRodsMsg" );
                ret = sendRodsMsg(
                          net_obj,
                          RODS_API_REQ_T,
                          myInputStructBBuf,
                          inputBsBBuf,
                          NULL,
                          RcApiTable[apiInx]->apiNumber,
                          conn->irodsProt );
                if ( !ret.ok() ) {
                    irods::log( PASS( ret ) );
                }
                else {
                    status = savedStatus;
                }

            } // if status1 > 0
        } // if svrVersion != NULL ...
    }
    else {
        // =-=-=-=-=-=-=-
        // be sure to pass along the return code from the
        // plugin call
        status = ret.code();
    }

    freeBBuf( inputStructBBuf );

    return status;
}

int
readAndProcApiReply( rcComm_t *conn, int apiInx, void **outStruct,
                     bytesBuf_t *outBsBBuf ) {
    int status = 0;
    msgHeader_t myHeader;
    /* bytesBuf_t outStructBBuf, errorBBuf, myOutBsBBuf; */
    bytesBuf_t outStructBBuf, errorBBuf;

    cliChkReconnAtReadStart( conn );

    memset( &outStructBBuf, 0, sizeof( bytesBuf_t ) );
    memset( &errorBBuf, 0, sizeof( bytesBuf_t ) );
    /* memset (&myOutBsBBuf, 0, sizeof (bytesBuf_t)); */

    /* some sanity check */

    irods::api_entry_table& RcApiTable = irods::get_client_api_table();
    if ( RcApiTable[apiInx]->outPackInstruct != NULL && outStruct == NULL ) {
        rodsLog( LOG_ERROR,
                 "readAndProcApiReply: outStruct error for A apiNumber %d",
                 RcApiTable[apiInx]->apiNumber );
        cliChkReconnAtReadEnd( conn );
        return USER_API_INPUT_ERR;
    }

    if ( RcApiTable[apiInx]->outBsFlag > 0 && outBsBBuf == NULL ) {
        rodsLog( LOG_ERROR,
                 "readAndProcApiReply: outBsBBuf error for B apiNumber %d",
                 RcApiTable[apiInx]->apiNumber );
        cliChkReconnAtReadEnd( conn );
        return USER_API_INPUT_ERR;
    }

    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( conn, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    ret = readMsgHeader( net_obj, &myHeader, NULL );
    if ( !ret.ok() ) {
#ifdef RODS_CLERVER
        irods::log( PASS( ret ) );
#else
        if (ret.code() == SYS_HEADER_READ_LEN_ERR) {
            ret = CODE(SYS_INTERNAL_ERR);
        }
#endif
        if ( conn->svrVersion != NULL && conn->svrVersion->reconnPort > 0 ) {
            int savedStatus = ret.code();
            /* try again. the socket might have changed */
            conn->thread_ctx->lock->lock();
            rodsLog( LOG_DEBUG,
                     "readAndProcClientMsg:svrSwitchConnect.cliState = %d,agState=%d",
                     conn->clientState, conn->agentState );
            cliSwitchConnect( conn );
            conn->thread_ctx->lock->unlock();
            ret = readMsgHeader( net_obj, &myHeader, NULL );

            if ( !ret.ok() ) {
                cliChkReconnAtReadEnd( conn );
                return savedStatus;
            }
        }
        else {
            cliChkReconnAtReadEnd( conn );
            return ret.code();
        }

    } // if !ret.ok

    ret = readMsgBody( net_obj, &myHeader, &outStructBBuf, outBsBBuf,
                       &errorBBuf, conn->irodsProt, NULL );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        cliChkReconnAtReadEnd( conn );
        return status;
    } // if !ret.ok

    cliChkReconnAtReadEnd( conn );

    if ( strcmp( myHeader.type, RODS_API_REPLY_T ) == 0 ) {
        status = procApiReply( conn, apiInx, outStruct, outBsBBuf,
                               &myHeader, &outStructBBuf, NULL, &errorBBuf );
    }

    clearBBuf( &outStructBBuf );
    /* clearBBuf (&myOutBsBBuf); */
    clearBBuf( &errorBBuf );

    return status;
}

int procApiReply(rcComm_t *conn,
                 int apiInx,
                 void **outStruct,
                 bytesBuf_t *outBsBBuf,
                 msgHeader_t *myHeader,
                 bytesBuf_t *outStructBBuf,
                 bytesBuf_t *myOutBsBBuf,
                 bytesBuf_t *errorBBuf)
{
    int status;
    int retVal;

    if ( errorBBuf->len > 0 ) {
        status = unpack_struct(errorBBuf->buf,
                               (void**) (static_cast<void*>(&conn->rError)),
                               "RError_PI", RodsPackTable, conn->irodsProt,
                               conn->svrVersion->relVersion);
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "readAndProcApiReply:unpack_struct error. status = %d",
                          status );
        }
    }

    retVal = myHeader->intInfo;

    /* some sanity check */

    irods::api_entry_table& RcApiTable = irods::get_client_api_table();
    if ( RcApiTable[apiInx]->outPackInstruct != NULL && outStruct == NULL ) {
        rodsLog( LOG_ERROR,
                 "readAndProcApiReply: outStruct error for C apiNumber %d",
                 RcApiTable[apiInx]->apiNumber );

        if ( retVal < 0 ) {
            return retVal;
        }

        return USER_API_INPUT_ERR;
    }

    if ( RcApiTable[apiInx]->outBsFlag > 0 && outBsBBuf == NULL ) {
        rodsLog( LOG_ERROR,
                 "readAndProcApiReply: outBsBBuf error for D apiNumber %d",
                 RcApiTable[apiInx]->apiNumber );
        if ( retVal < 0 ) {
            return retVal;
        }

        return USER_API_INPUT_ERR;
    }

    /* handle outStruct */
    if ( outStructBBuf->len > 0 ) {
        if ( outStruct != NULL ) {
            status = unpack_struct(outStructBBuf->buf, (void**) outStruct,
                                   (char*) RcApiTable[apiInx]->outPackInstruct, RodsPackTable,
                                   conn->irodsProt, conn->svrVersion->relVersion);
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "readAndProcApiReply:unpack_struct error. status = %d",
                              status );

                if ( retVal < 0 ) {
                    return retVal;
                }

                return status;
            }
        }
        else {
            rodsLog( LOG_ERROR,
                     "readAndProcApiReply: got unneeded outStruct for apiNumber %d",
                     RcApiTable[apiInx]->apiNumber );
        }
    }

    if ( myOutBsBBuf != NULL && myOutBsBBuf->len > 0 ) {
        if ( outBsBBuf != NULL ) {
            /* copy to out */
            *outBsBBuf = *myOutBsBBuf;
            memset( myOutBsBBuf, 0, sizeof( bytesBuf_t ) );
        }
        else {
            rodsLog( LOG_ERROR,
                     "readAndProcApiReply: got unneeded outBsBBuf for apiNumber %d",
                     RcApiTable[apiInx]->apiNumber );
        }
    }

    return retVal;
}

int cliGetCollOprStat( rcComm_t *conn, collOprStat_t *collOprStat, int vFlag, int retval )
{
    int status = retval;

    while ( status == SYS_SVR_TO_CLI_COLL_STAT ) {
        /* more to come */
        if ( collOprStat != NULL ) {
            if ( vFlag != 0 ) {
                printf( "num files done = %d, ", collOprStat->filesCnt );
                if ( collOprStat->totalFileCnt <= 0 ) {
                    printf( "totalFileCnt = UNKNOWN, " );
                }
                else {
                    printf( "totalFileCnt = %d, ", collOprStat->totalFileCnt );
                }
                printf( "bytesWritten = %lld, last file done: %s\n",
                        collOprStat->bytesWritten, collOprStat->lastObjPath );
            }
            free( collOprStat );
            collOprStat = NULL;
        }
        status = _cliGetCollOprStat( conn, &collOprStat );
    }

    if ( collOprStat != NULL ) {
        free( collOprStat );
    }

    return status;
}

int _cliGetCollOprStat( rcComm_t *conn, collOprStat_t **collOprStat )
{
    int myBuf = htonl( SYS_CLI_TO_SVR_COLL_STAT_REPLY );
    if (irods::CS_NEG_USE_SSL == conn->negotiation_results) {
        if (int bytes_left = 4 - sslWrite(static_cast<void*>(&myBuf), 4, NULL, conn->ssl)) {
            rodsLogError( LOG_ERROR, SYS_SOCK_WRITE_ERR, "sslWrite exited with %d bytes still left to write in %s.", bytes_left, __PRETTY_FUNCTION__ );
            return SYS_SOCK_WRITE_ERR;
        }
    }
    else {
        if (int bytes_left = 4 - myWrite( conn->sock, ( void * ) &myBuf, 4, NULL )) {
            rodsLogError( LOG_ERROR, SYS_SOCK_WRITE_ERR, "myWrite exited with %d bytes still left to write in %s.", bytes_left, __PRETTY_FUNCTION__ );
            return SYS_SOCK_WRITE_ERR;
        }
    }

    return readAndProcApiReply( conn, conn->apiInx, ( void ** ) collOprStat, NULL );
}

int apiTableLookup(int apiNumber)
{
    if (auto& api_table = irods::get_client_api_table();
        api_table.find(apiNumber) != std::end(api_table))
    {
        return apiNumber;
    }

#if defined(RODS_SERVER) || defined(RODS_CLERVER)
    if (auto& api_table = irods::get_server_api_table();
        api_table.find(apiNumber) != std::end(api_table))
    {
        return apiNumber;
    }
#endif // RODS_SERVER || RODS_CLERVER

    return SYS_UNMATCHED_API_NUM;
}

