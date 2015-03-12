/**
 * @file xmsgMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "reGlobalsExtern.hpp"
#include "icatHighLevelRoutines.hpp"
#include "rodsXmsg.hpp"
#include "getXmsgTicket.hpp"
#include "sendXmsg.hpp"
#include "rcvXmsg.hpp"

#if defined(_LP64) || defined(__LP64__)
#define CAST_PTR_INT (long int)
#else
#define CAST_PTR_INT (uint)
#endif

static  rcComm_t *xmsgServerConn = NULL;
static   rodsEnv myRodsXmsgEnv;

/**
 * \fn msiXmsgServerConnect(msParam_t* outConnParam, ruleExecInfo_t *rei)
 *
 * \brief Connect to the XMessage server.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008-05
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[out] outConnParam - a msParam of type RcComm_MS_T which is a connection descriptor.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre needs the XMessage Server host name in irods_environment.json file
 * \post none
 * \sa none
**/
int msiXmsgServerConnect( msParam_t* outConnParam, ruleExecInfo_t *rei ) {

    rcComm_t *conn;
    rodsEnv myRodsEnv;
    rErrMsg_t errMsg;
    int status;

    RE_TEST_MACRO( "    Calling msiXmsgServerConnect" );

    status = getRodsEnv( &myRodsEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "msiXmsgServerConnect: getRodsEnv failed:%i", status );
        return status;
    }
    conn = rcConnectXmsg( &myRodsEnv, &errMsg );
    if ( conn == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiXmsgServerConnect: rcConnectXmsg failed:%i :%s\n", errMsg.status, errMsg.msg );
        return errMsg.status;
    }
    status = clientLogin( conn );
    if ( status != 0 ) {
        rodsLog( LOG_ERROR, "msiXmsgServerConnect: clientLogin failed:%i", status );
        return status;
    }

    outConnParam->inOutStruct = ( void * ) conn;
    outConnParam->type = strdup( "RcComm_MS_T" );

    return 0;

}

/**
 * \fn msiXmsgCreateStream(msParam_t* inConnParam, msParam_t* inGgetXmsgTicketInpParam, msParam_t* outXmsgTicketInfoParam, ruleExecInfo_t *rei)
 *
 * \brief  This microservice creates a new Message Stream.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008-05
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inConnParam - a msParam of type RcComm_MS_T which is a connection descriptor obtained by msiXmsgServerConnect.
 * \param[in] inGgetXmsgTicketInpParam - a msParam of type GetXmsgTicketInp_MS_T which is actually an integer giving expiration time.
 * \param[out] outXmsgTicketInfoParam - a msParam of type XmsgTicketInfo_MS_T which is an information struct for the ticket generated for this stream.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre msiXmsgServerConnect should have been done earlier to get the inConnParam.
 * \post none
 * \sa msiXmsgServerConnect
**/
int msiXmsgCreateStream( msParam_t* inConnParam,
                         msParam_t* inGgetXmsgTicketInpParam,
                         msParam_t* outXmsgTicketInfoParam,
                         ruleExecInfo_t *rei ) {

    rcComm_t *conn;
    getXmsgTicketInp_t *getXmsgTicketInp;
    xmsgTicketInfo_t *outXmsgTicketInfo = NULL;
    int status;
    int allocFlag = 0;

    RE_TEST_MACRO( "    Calling msiXmsgCreateStream" );

    if ( inConnParam->inOutStruct == NULL ) {
        rodsLog( LOG_ERROR,
                 "msiXmsgCreateStream: input inConnParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    conn = ( rcComm_t * ) inConnParam->inOutStruct;

    if ( inGgetXmsgTicketInpParam->inOutStruct != NULL && !( strcmp( inGgetXmsgTicketInpParam->type, STR_MS_T ) == 0 && strcmp( ( char * )inGgetXmsgTicketInpParam->inOutStruct, "" ) == 0 ) ) {
        getXmsgTicketInp = ( getXmsgTicketInp_t * ) inGgetXmsgTicketInpParam->inOutStruct;
    }
    else {
        getXmsgTicketInp = ( getXmsgTicketInp_t * )  malloc( sizeof( getXmsgTicketInp_t ) );
        memset( getXmsgTicketInp, 0, sizeof( getXmsgTicketInp_t ) );
        allocFlag = 1;
    }

    status = rcGetXmsgTicket( conn, getXmsgTicketInp, &outXmsgTicketInfo );
    if ( status != 0 ) {
        rodsLog( LOG_ERROR, "msiXmsgCreateStream: rcGetXmsgTicket failed:%i", status );
        free( getXmsgTicketInp );
        return status;
    }

    outXmsgTicketInfoParam->inOutStruct = ( void * ) outXmsgTicketInfo;
    outXmsgTicketInfoParam->type = strdup( XmsgTicketInfo_MS_T );
    if ( allocFlag == 1 ) {
        free( getXmsgTicketInp );
    }
    return 0;

}

/**
 * \fn msiCreateXmsgInp(msParam_t* inMsgNumber, msParam_t* inMsgType, msParam_t* inNumberOfReceivers, msParam_t* inMsg, msParam_t* inNumberOfDeliverySites,
 *                      msParam_t* inDeliveryAddressList, msParam_t* inDeliveryPortList, msParam_t* inMiscInfo, msParam_t* inXmsgTicketInfoParam,
 *                      msParam_t* outSendXmsgInpParam, ruleExecInfo_t *rei)
 *
 * \brief  Given all information values this microservice creates an Xmsg packet.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008-05
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inMsgNumber - a msParam of type uint or STR_MS_T which is a message serial number.
 * \param[in] inMsgType - a msParam of type uint or STR_MS_T which is currently 0 (SINGLE_MSG_TICKET) or 1 (MULTI_MSG_TICKET).
 * \param[in] inNumberOfReceivers - a msParam of type uint or STR_MS_T which is a number of receivers of the message.
 * \param[in] inMsg - a msParam of type  STR_MS_T which is a message body.
 * \param[in] inNumberOfDeliverySites - a msParam of type int or STR_MS_T which is a Number of Receiving Addresses.
 * \param[in] inDeliveryAddressList - a msParam of type STR_MS_T which is a List of Host Addresses (comma separated).
 * \param[in] inDeliveryPortList - a msParam of type STR_MS_T which is a List of Correspondng Ports (comma separated).
 * \param[in] inMiscInfo - a msParam of type STR_MS_T which is other Information.
 * \param[in] inXmsgTicketInfoParam - a msParam of type XmsgTicketInfo_MS_T which is outXmsgTicketInfoParam from msiXmsgCreateStream
 * \param[out] outSendXmsgInpParam - a msParam of type SendXmsgInp_MS_T which is a Xmsg packet.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre  none
 * \post none
 * \sa none
**/
int msiCreateXmsgInp( msParam_t* inMsgNumber,
                      msParam_t* inMsgType,
                      msParam_t* inNumberOfReceivers,
                      msParam_t* inMsg,
                      msParam_t* inNumberOfDeliverySites,
                      msParam_t* inDeliveryAddressList,
                      msParam_t* inDeliveryPortList,
                      msParam_t* inMiscInfo,
                      msParam_t* inXmsgTicketInfoParam,
                      msParam_t* outSendXmsgInpParam,
                      ruleExecInfo_t* ) {


    sendXmsgInp_t  *sendXmsgInp;
    xmsgTicketInfo_t *xmsgTicketInfo;

    if ( inXmsgTicketInfoParam == NULL ) {
        rodsLog( LOG_ERROR, "msiSendXmsg: input inXmsgTicketInfoParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    xmsgTicketInfo = ( xmsgTicketInfo_t * ) inXmsgTicketInfoParam->inOutStruct;

    sendXmsgInp = ( sendXmsgInp_t  * ) malloc( sizeof( sendXmsgInp_t ) );
    /*  memset (&sendXmsgInp, 0, sizeof (sendXmsgInp_t));*/

    sendXmsgInp->ticket.sendTicket =  xmsgTicketInfo->sendTicket;
    sendXmsgInp->ticket.rcvTicket =  xmsgTicketInfo->rcvTicket;
    sendXmsgInp->ticket.expireTime =  xmsgTicketInfo->expireTime;
    sendXmsgInp->ticket.flag = xmsgTicketInfo->flag;
    if ( !strcmp( inMsgNumber->type, STR_MS_T ) ) {
        sendXmsgInp->sendXmsgInfo.msgNumber = ( uint ) atoi( ( char* )inMsgNumber->inOutStruct );
    }
    else {
        sendXmsgInp->sendXmsgInfo.msgNumber = CAST_PTR_INT inMsgNumber->inOutStruct;
    }
    snprintf( sendXmsgInp->sendXmsgInfo.msgType, sizeof( sendXmsgInp->sendXmsgInfo.msgType ), "%s", ( char* )inMsgType->inOutStruct );
    if ( !strcmp( inNumberOfReceivers->type, STR_MS_T ) ) {
        sendXmsgInp->sendXmsgInfo.numRcv = ( uint ) atoi( ( char* )inNumberOfReceivers->inOutStruct );
    }
    else {
        sendXmsgInp->sendXmsgInfo.numRcv = CAST_PTR_INT inNumberOfReceivers->inOutStruct;
    }
    sendXmsgInp->sendXmsgInfo.msg = strdup( ( char * ) inMsg->inOutStruct );
    if ( !strcmp( inNumberOfDeliverySites->type, STR_MS_T ) ) {
        sendXmsgInp->sendXmsgInfo.numDeli = ( int ) atoi( ( char* )inNumberOfDeliverySites->inOutStruct );
    }
    else {
        sendXmsgInp->sendXmsgInfo.numDeli = CAST_PTR_INT inNumberOfDeliverySites->inOutStruct;
    }
    if ( sendXmsgInp->sendXmsgInfo.numDeli == 0 ) {
        sendXmsgInp->sendXmsgInfo.deliAddress = NULL;
        sendXmsgInp->sendXmsgInfo.deliPort = NULL;
    }
    else {
        sendXmsgInp->sendXmsgInfo.deliAddress = ( char** )inDeliveryAddressList->inOutStruct;
        sendXmsgInp->sendXmsgInfo.deliPort = ( uint* )inDeliveryPortList->inOutStruct;
    }
    sendXmsgInp->sendXmsgInfo.miscInfo = strdup( ( char * ) inMiscInfo->inOutStruct );

    outSendXmsgInpParam->inOutStruct = ( void * ) sendXmsgInp;
    outSendXmsgInpParam->type = strdup( SendXmsgInp_MS_T );
    return 0;


}


/**
 * \fn msiSendXmsg(msParam_t* inConnParam, msParam_t* inSendXmsgInpParam, ruleExecInfo_t *rei)
 *
 * \brief  This microservice sends an Xmsg packet.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008-05
 *
 * \note Sends an Xmsg packet created by #msiCreateXmsgInp using the connection made by #msiXmsgServerConnect
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inConnParam - a msParam of type RcComm_MS_T which is a connection descriptor obtained by msiXmsgServerConnect.
 * \param[in] inSendXmsgInpParam - a msParam of type SendXmsgInp_MS_T outSendXmsgInpParam from inSendXmsgInpParam.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre msiXmsgServerConnect should've been done earlier to get the inConnParam.
 * \post none
 * \sa none
**/
int msiSendXmsg( msParam_t* inConnParam,
                 msParam_t* inSendXmsgInpParam,
                 ruleExecInfo_t *rei ) {

    rcComm_t *conn;
    sendXmsgInp_t  *sendXmsgInp;
    int status;



    RE_TEST_MACRO( "    Calling msiSendXmsg" );

    if ( inConnParam->inOutStruct == NULL ) {
        rodsLog( LOG_ERROR, "msiSendXmsg: input inConnParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    conn = ( rcComm_t * ) inConnParam->inOutStruct;

    if ( inSendXmsgInpParam == NULL ) {
        rodsLog( LOG_ERROR, "msiSendXmsg: input inSendXmsgInpParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    sendXmsgInp = ( sendXmsgInp_t * ) inSendXmsgInpParam->inOutStruct;


    status = rcSendXmsg( conn, sendXmsgInp );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "msiSendXmsg: rcSendXmsg failed:%i", status );
        return status;
    }
    return status;
}



/**
 * \fn msiRcvXmsg(msParam_t* inConnParam, msParam_t* inTicketNumber, msParam_t* inMsgNumber, msParam_t* outMsgType, msParam_t* outMsg,
 *                msParam_t* outSendUser, ruleExecInfo_t *rei)
 *
 * \brief  This microservice receives an Xmsg packet.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008-05
 *
 * \note  Uses the connection made by #msiXmsgServerConnect.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inConnParam - a msParam of type RcComm_MS_T which is a connection descriptor obtained by msiXmsgServerConnect.
 * \param[in] inTicketNumber - a msParam of type XmsgTicketInfo_MS_T or STR_MS_T or unit which is outXmsgTicketInfoParam from msiXmsgCreateStream or outXmsgTicketInfoParam->rcvTicket (a string which the sender passes to the receiver).
 * \param[in] inMsgNumber - a msParam of type unit or STR_MS_T which is a message serial number to fetch.
 * \param[out] outMsgType - a msParam of type STR_MS_T which is a message type.
 * \param[out] outMsg - a msParam of type STR_MS_T which is a message body.
 * \param[out] outSendUser - a msParam of type STR_MS_T which is a sender information.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre msiXmsgServerConnect should've been done earlier to get the inConnParam.
 * \post none
 * \sa none
**/
int msiRcvXmsg( msParam_t* inConnParam,
                msParam_t* inTicketNumber,
                msParam_t* inMsgNumber,
                msParam_t* outMsgType,
                msParam_t* outMsg,
                msParam_t* outSendUser,
                ruleExecInfo_t *rei ) {

    rcComm_t *conn;
    rcvXmsgInp_t rcvXmsgInp;
    rcvXmsgOut_t *rcvXmsgOut = NULL;
    xmsgTicketInfo_t *xmsgTicketInfo = NULL;
    int status;


    RE_TEST_MACRO( "    Calling msiRcvXmsg" );

    if ( inConnParam->inOutStruct == NULL ) {
        rodsLog( LOG_ERROR, "msiRcvXmsg: input inConnParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    conn = ( rcComm_t * ) inConnParam->inOutStruct;

    memset( &rcvXmsgInp, 0, sizeof( rcvXmsgInp ) );
    if ( !strcmp( inTicketNumber->type, XmsgTicketInfo_MS_T ) ) {
        xmsgTicketInfo = ( xmsgTicketInfo_t * ) inTicketNumber->inOutStruct;
        rcvXmsgInp.rcvTicket = xmsgTicketInfo->rcvTicket;
    }
    else if ( !strcmp( inTicketNumber->type, STR_MS_T ) ) {
        rcvXmsgInp.rcvTicket = ( uint ) atoi( ( char* )inTicketNumber->inOutStruct );
    }
    else {
        rcvXmsgInp.rcvTicket = CAST_PTR_INT inTicketNumber->inOutStruct;
    }
    if ( !strcmp( inMsgNumber->type, STR_MS_T ) ) {
        rcvXmsgInp.msgNumber = ( uint ) atoi( ( char* )inMsgNumber->inOutStruct );
    }
    else {
        rcvXmsgInp.msgNumber = CAST_PTR_INT inMsgNumber->inOutStruct;
    }

    status = rcRcvXmsg( conn, &rcvXmsgInp, &rcvXmsgOut );
    if ( status < 0 || rcvXmsgOut == NULL ) { // JMC cppcheck
        rodsLog( LOG_ERROR, "msiRcvXmsg: rcRcvXmsg failed:%i", status );
        return status;
    }

    outMsgType->inOutStruct = ( void * ) strdup( rcvXmsgOut->msgType );
    outMsgType->type = strdup( STR_MS_T );
    outMsg->inOutStruct = ( void * ) rcvXmsgOut->msg;
    outMsg->type = strdup( STR_MS_T );
    outSendUser->inOutStruct = ( void * ) strdup( rcvXmsgOut->sendUserName );
    outSendUser->type = strdup( STR_MS_T );
    return status;
}

/**
 * \fn msiXmsgServerDisConnect(msParam_t* inConnParam, ruleExecInfo_t *rei)
 *
 * \brief  This microservice disconnects from the XMessage Server.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008-05
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inConnParam - a msParam of type RcComm_MS_T which is a connection descriptor obtained by msiXmsgServerConnect.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre msiXmsgServerConnect should've been done earlier to get the inConnParam.
 * \post none
 * \sa none
**/
int msiXmsgServerDisConnect( msParam_t* inConnParam, ruleExecInfo_t *rei ) {

    rcComm_t *conn;
    int status;

    RE_TEST_MACRO( "    Calling msiXmsgServerDisConnect" );

    if ( inConnParam->inOutStruct == NULL ) {
        rodsLog( LOG_ERROR, "msiSendXmsg: input inConnParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    conn = ( rcComm_t * ) inConnParam->inOutStruct;
    status = rcDisconnect( conn );
    if ( status == 0 ) {
        inConnParam->inOutStruct = NULL;
    }
    return status;

}


int _writeXMsg( int streamId, char *hdr, char *msg ) {
    int i;
    xmsgTicketInfo_t xmsgTicketInfo;
    sendXmsgInp_t sendXmsgInp;
    rcComm_t *conn;
    rErrMsg_t errMsg;
    char myHostName[MAX_NAME_LEN];


    if ( xmsgServerConn == NULL ) {
        i = getRodsEnv( &myRodsXmsgEnv );
        if ( i < 0 ) {
            rodsLog( LOG_ERROR, "_writeXMsg: getRodsEnv failed:%i", i );
            return i;
        }
        conn = rcConnectXmsg( &myRodsXmsgEnv, &errMsg );
        if ( conn == NULL ) {
            rodsLog( LOG_ERROR, "_writeXMsg: rcConnectXmsg failed:%i:%s", errMsg.status, errMsg.msg );
            return errMsg.status;
        }
        i = clientLogin( conn );
        if ( i != 0 ) {
            rodsLog( LOG_ERROR, "msiXmsgServerConnect: clientLogin failed:%i", i );
            rcDisconnect( conn );
            return i;
        }
        xmsgServerConn = conn;
    }
    else {
        conn = xmsgServerConn;
    }
    myHostName[0] = '\0';
    gethostname( myHostName, MAX_NAME_LEN );

    memset( &xmsgTicketInfo, 0, sizeof( xmsgTicketInfo ) );
    memset( &sendXmsgInp, 0, sizeof( sendXmsgInp ) );
    xmsgTicketInfo.sendTicket = streamId;
    xmsgTicketInfo.rcvTicket = streamId;
    xmsgTicketInfo.flag = 1;
    sendXmsgInp.ticket = xmsgTicketInfo;
    sendXmsgInp.sendXmsgInfo.numRcv = 1;
    sendXmsgInp.sendXmsgInfo.msgNumber = 0;
    snprintf( sendXmsgInp.sendXmsgInfo.msgType, HEADER_TYPE_LEN, "%s", hdr );
    snprintf( sendXmsgInp.sendAddr, NAME_LEN, "%s:%i", myHostName, getpid() );
    sendXmsgInp.sendXmsgInfo.msg = msg;
    i = rcSendXmsg( conn, &sendXmsgInp );
    if ( i < 0 ) {
        rodsLog( LOG_NOTICE, "_writeXmsg: Unable to send message to stream  %i\n", streamId );
    }
    /*   rcDisconnect(conn); */
    return i;
}

int _readXMsg( int streamId, char *condRead, int *msgNum, int *seqNum,
               char **hdr, char **msg, char **user, char **addr ) {
    int i;
    rcvXmsgInp_t rcvXmsgInp;
    rcvXmsgOut_t *rcvXmsgOut = NULL;
    rcComm_t *conn;
    rErrMsg_t errMsg;

    if ( xmsgServerConn == NULL ) {
        i = getRodsEnv( &myRodsXmsgEnv );
        if ( i < 0 ) {
            rodsLog( LOG_ERROR, "_readXMsg: getRodsEnv failed:%i", i );
            return i;
        }
        conn = rcConnectXmsg( &myRodsXmsgEnv, &errMsg );
        if ( conn == NULL ) {
            rodsLog( LOG_ERROR, "_readXMsg: rcConnectXmsg failed:%i:%s", errMsg.status, errMsg.msg );
            return errMsg.status;
        }
        i = clientLogin( conn );
        if ( i != 0 ) {
            rodsLog( LOG_ERROR, "msiXmsgServerConnect: clientLogin failed:%i", i );
            rcDisconnect( conn );
            return i;
        }
        xmsgServerConn = conn;
    }
    else {
        conn = xmsgServerConn;
    }
    memset( &rcvXmsgInp, 0, sizeof( rcvXmsgInp ) );
    rcvXmsgInp.rcvTicket = streamId;
    rcvXmsgInp.msgNumber = 0;
    snprintf( rcvXmsgInp.msgCondition, MAX_NAME_LEN, "%s", condRead );
    i = rcRcvXmsg( conn, &rcvXmsgInp, &rcvXmsgOut );
    if ( i < 0 || rcvXmsgOut == NULL ) { // JMC cppcheck
        /*  rcDisconnect(conn); */
        rodsLog( LOG_NOTICE, "_readXmsg: Unable to receive message from stream  %i\n", streamId );
        return i;
    }
    *msgNum = rcvXmsgOut->msgNumber;
    *seqNum = rcvXmsgOut->seqNumber;
    *hdr = strdup( rcvXmsgOut->msgType );
    *msg = strdup( rcvXmsgOut->msg );
    *user = strdup( rcvXmsgOut->sendUserName );
    *addr = strdup( rcvXmsgOut->sendAddr );
    /*  rcDisconnect(conn);*/
    return i;
}
