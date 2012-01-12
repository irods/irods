/**
 * @file	printMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"

#ifdef ADDR_64BITS
#define CAST_PTR_INT (long int)
#else
#define CAST_PTR_INT
#endif

/**
 * \cond oldruleengine
 * \fn writeLine(msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes a given string and newline character into the given buffer.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \note   This microservice takes a given buffer string and appends it to the end of the buffer
 * (either stdout or stderr in ruleExecOut parameter) followed by a new line character. 
 * This may be extended later for writing into local log file 
 * or into an iRODS file also. The ruleExecOut is a system MS-parameter (*variable) that is automatically available.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] where - a msParam of type STR_MS_T which is the buffer name in ruleExecOut. Currently stdout and stderr.
 * \param[in] inString - a msParam of type STR_MS_T which is a string to be written into buffer.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect ruleExecOut structure in msParamArray gets modified
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa writeString
 * \endcond
**/
int writeLine(msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei)
{
  int i;
  char tmp[3];
  char *ptr;
  char *writeId = (char *) where->inOutStruct;

  if (writeId != NULL && strcmp (writeId, "serverLog") == 0 &&
   inString->inOutStruct != NULL) {
    rodsLog (LOG_NOTICE, "writeLine: inString = %s\n", inString->inOutStruct);
    return 0;
  }

  i = writeString(where, inString,rei);
  if (i < 0)
    return(i);
  ptr = (char*)inString->inOutStruct;
  sprintf(tmp,"%s\n","");
  inString->inOutStruct =  tmp;
  i = writeString(where, inString,rei);
  inString->inOutStruct = ptr;
  return(i);
  
}

/**
 * \cond oldruleengine
 * \fn writeString(msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes a given string into the target buffer
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date    2008
 * 
 * \note   This microservice takes a given buffer string and appends it to the end of the buffer
 * (either stdout or stderr in ruleExecOut parameter). This may be extended later for writing into local log file 
 * or into an iRODS file also. The ruleExecOut is a system MS-parameter (*variable) that is automatically available.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] where - where is a msParam of type STR_MS_T which is the buffer name in ruleExecOut. Currently stdout and stderr.
 * \param[in] inString - inString is a msParam of type STR_MS_T which is a string to be written into buffer
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect ruleExecOut structure in msParamArray gets modified.
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
 * \endcond
**/
int writeString(msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei)
{
  int i;
  char *writeId;
  char *writeStr;

  if (where->inOutStruct == NULL)
    writeId = where->label;
  else
    writeId = (char*)where->inOutStruct;

  if (inString->inOutStruct == NULL) {
    writeStr = strdup((char *) inString->label);
  }
  else {
    writeStr = strdup((char *) inString->inOutStruct);
  }
  i = _writeString(writeId, writeStr, rei);

  free(writeStr);
  return(i);
}

int _writeString(char *writeId, char *writeStr, ruleExecInfo_t *rei)
{
  msParamArray_t *inMsParamArray;
  msParam_t *mP;
  execCmdOut_t *myExecCmdOut;
    
  if (writeId != NULL && strcmp (writeId, "serverLog") == 0) {
    rodsLog (LOG_NOTICE, "writeString: inString = %s", writeStr);
    return 0;
  }
  mP = NULL;
  inMsParamArray = rei->msParamArray;
  if (((mP = getMsParamByLabel (inMsParamArray, "ruleExecOut")) != NULL) &&
      (mP->inOutStruct != NULL)) {
    if (!strcmp(mP->type,STR_MS_T)) {
      myExecCmdOut =  (execCmdOut_t*)malloc (sizeof (execCmdOut_t));
      memset (myExecCmdOut, 0, sizeof (execCmdOut_t));
      mP->inOutStruct = myExecCmdOut;
      mP->type = strdup(ExecCmdOut_MS_T);
    }
    else
      myExecCmdOut = (execCmdOut_t*)mP->inOutStruct;
  }
  else {
    myExecCmdOut =  (execCmdOut_t*)malloc (sizeof (execCmdOut_t));
    memset (myExecCmdOut, 0, sizeof (execCmdOut_t));
    if (mP == NULL)
      addMsParam(inMsParamArray,"ruleExecOut", ExecCmdOut_MS_T,myExecCmdOut,NULL);
    else {
      mP->inOutStruct = myExecCmdOut;
      mP->type = strdup(ExecCmdOut_MS_T);
    }
  }

  /***** Jun 27, 2007
  i  = replaceVariablesAndMsParams("",writeStr, rei->msParamArray, rei);
  if (i < 0)
    return(i);
  ****/

  if (!strcmp(writeId,"stdout")) 
    appendToByteBuf(&(myExecCmdOut->stdoutBuf),(char *) writeStr);
  else if (!strcmp(writeId,"stderr")) 
    appendToByteBuf(&(myExecCmdOut->stderrBuf),(char *) writeStr);



  return(0);
}


/**
 * \fn writePosInt(msParam_t* where, msParam_t* inInt, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes a positive integer into a buffer.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Antoine de Torcy
 * \date    2007
 * 
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] where - a msParam of type STR_MS_T which is the buffer name in ruleExecOut.
 * \param[in] inInt - the integer to write
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
 * \pre none
 * \post none
 * \sa none
**/
int writePosInt(msParam_t* where, msParam_t* inInt, ruleExecInfo_t *rei)
{
	char *writeId;
	char writeStr[LONG_NAME_LEN];
	int status;

	if (where->inOutStruct != NULL) {
		writeId = (char*)where->inOutStruct;	
	}
	else {
		writeId = where->label;
	}

	if (inInt->inOutStruct != NULL) {
		sprintf(writeStr, "%d", parseMspForPosInt (inInt));
	}
	else {
		snprintf(writeStr, LONG_NAME_LEN, "%s", inInt->label);
	}

	status = _writeString(writeId, writeStr, rei);

	return (status);
}


/**
 * \fn writeBytesBuf(msParam_t* where, msParam_t* inBuf, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes the buffer in an inOutStruct to stdout or stderr.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author  Antoine de Torcy
 * \date 2008
 * 
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] where - a msParam of type STR_MS_T which is the buffer name in ruleExecOut. It can be stdout or stderr.
 * \param[in] inBuf - a msParam of type STR_MS_T - related to the status output
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
 * \pre none
 * \post none
 * \sa none
**/
int writeBytesBuf(msParam_t* where, msParam_t* inBuf, ruleExecInfo_t *rei)
{
	char *writeId;
	char *writeStr;
	int status;
	
	if (where->inOutStruct != NULL) {
		writeId = (char*)where->inOutStruct;	
	}
	else {
		writeId = where->label;
	}
	
	if (inBuf->inpOutBuf != NULL) {
		writeStr = (char *) malloc(strlen((const char*)inBuf->inpOutBuf->buf) + MAX_COND_LEN);
		strcpy(writeStr , (const char*)inBuf->inpOutBuf->buf);
	}
	else {
		writeStr = (char *) malloc(strlen(inBuf->label) + MAX_COND_LEN);
		strcpy(writeStr , inBuf->label);
	}

	status = _writeString(writeId, writeStr, rei);
	
	if (writeStr != NULL) {
		free(writeStr);
	}

	return (status);
}

/**
 * \fn writeKeyValPairs(msParam_t *where, msParam_t *inKVPair, msParam_t *separator, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes keyword value pairs to stdout or stderr, using the given separator.
 * 
 * \module core
 * 
 * \since 2.1
 * 
 * \author  Antoine de Torcy
 * \date    2009
 * 
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] where - a msParam of type STR_MS_T which is the buffer name in ruleExecOut. It can be stdout or stderr.
 * \param[in] inKVPair - a msParam of type KeyValPair_MS_T
 * \param[in] separator - Optional - a msParam of type STR_MS_T, the desired parameter
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
 * \pre none
 * \post none
 * \sa none
**/
int writeKeyValPairs(msParam_t *where, msParam_t *inKVPair, msParam_t *separator, ruleExecInfo_t *rei)
{
	keyValPair_t *KVPairs;
	char *writeId;
	char *writeStr;
	char *sepStr;
	int i;
	size_t size;


	RE_TEST_MACRO ("    Calling writeKeyValPairs")

	
	/* sanity checks */
	if (!rei ) {
		rodsLog (LOG_ERROR, "writeKeyValPairs: input rei is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	if (!where) {
		rodsLog (LOG_ERROR, "writeKeyValPairs: No destination provided for writing.");
		return (USER__NULL_INPUT_ERR);
	}

	/* empty? */
	if (!inKVPair || !inKVPair->inOutStruct)
	{
		return 0;
	}

	/* check for proper input type and get keyValPair input */
	if (inKVPair->type && strcmp(inKVPair->type, KeyValPair_MS_T)) {
		rodsLog (LOG_ERROR, "writeKeyValPairs: input parameter is not of KeyValPair_MS_T type.");
		return(USER_PARAM_TYPE_ERR);
	}
	KVPairs = (keyValPair_t *)inKVPair->inOutStruct;


	/* where are we writing to? */
	if (where->inOutStruct != NULL) {
		writeId = (char*)where->inOutStruct;
	}
	else {
		writeId = where->label;
	}


	/* get separator string or use default */
	if ((sepStr = parseMspForStr(separator)) == NULL)  {
		sepStr = "\t|\t";
	}


	/* find out how much memory is needed for writeStr */
	size = 0;
	for (i=0; i < KVPairs->len; i++) {
		size += strlen(KVPairs->keyWord[i]) + strlen(sepStr) + strlen(KVPairs->value[i]) + strlen("\n");
	}

	/* allocate memory for writeStr and pad with null chars */
	writeStr = (char *)malloc(size + MAX_COND_LEN);
	memset(writeStr, '\0', size + MAX_COND_LEN);


	/* print each key-value pair to writeStr */
	for (i=0; i < KVPairs->len; i++)  {
		strcat(writeStr, KVPairs->keyWord[i]);
		strcat(writeStr, sepStr);
		strcat(writeStr, KVPairs->value[i]);
		strcat(writeStr, "\n");
	}


	/* call _writeString() routine */
	rei->status = _writeString(writeId, writeStr, rei);

	
	/* free writeStr since its content has been copied somewhere else */
	if (writeStr != NULL) {
		free(writeStr);
	}

	return (rei->status);
}



/**
 * \fn writeXMsg(msParam_t* inStreamId, msParam_t *inHdr, msParam_t *inMsg, ruleExecInfo_t *rei)
 *
 * \brief  This microservice writes a given string into an XMsgStream
 *
 * \module core
 *
 * \since 2.4
 *
 * \author  Arcot Rajasekar
 * \date    2010
 * 
 * \note   This microservice takes a given buffer string and sends it as a message packet to the XMsg Server 
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inStreamId - of type STR_MS_T or INT_MAS_T - the XMsg streamId number
 *    possibly generated by a  msiXmsgCreateStream micro-serice or a supported standard stream with ids 1 thru 5     
 * \param[in] inHdr - of type STR_MS_T - header string to be sent in the message packet
 * \param[in] inMsg - of type STR_MS_T - message string to be sent in the message packet
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect an Xmsg gets sent and queued in the Xmsg server 
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiXmsgCreateStream, readXMsg
**/
int 
writeXMsg(msParam_t* inStreamId, msParam_t *inHdr, msParam_t *inMsg, ruleExecInfo_t *rei)
{
  int i;
  int streamId;
  xmsgTicketInfo_t *xmsgTicketInfo;

  RE_TEST_MACRO ("    Calling writeXMsg")

    if (!strcmp(inStreamId->type,XmsgTicketInfo_MS_T)) {
      xmsgTicketInfo = (xmsgTicketInfo_t *) inStreamId->inOutStruct;
      streamId = (int) xmsgTicketInfo->rcvTicket;
    }
    else if (!strcmp(inStreamId->type,STR_MS_T)) {
      streamId = (int) atoi((char*)inStreamId->inOutStruct);
    }
    else
      streamId = (int) CAST_PTR_INT  inStreamId->inOutStruct;

  i = _writeXMsg(streamId, (char*)inHdr->inOutStruct, (char*)inMsg->inOutStruct);
  return(i);
}

/**
 * \fn readXMsg(msParam_t* inStreamId, msParam_t *inCondRead,
 *         msParam_t *outMsgNum, msParam_t *outSeqNum,
 *        msParam_t *outHdr, msParam_t *outMsg,
 *        msParam_t *outUser, msParam_t *outAddr, ruleExecInfo_t *rei)
 *
 * \brief  Reads a message packet from an XMsgStream
 *
 * \module core
 *
 * \since 2.4
 *
 * \author  Arcot Rajasekar
 * \date    2010
 * 
 * \note   Reads into buffer a message packet from the XMsg Server 
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inStreamId - of type STR_MS_T or INT_MAS_T - the XMsg streamId number
 *    possibly generated by a msiXmsgCreateStream microservice or a supported standard stream with ids 1 thru 5     
 * \param[in] inCondRead - of type STR_MS_T - boolean condition for a packet to satisfy and the first packet that satisfies
 *      the condition is read from the XMsg Stream
 * \param[out] outMsgNum - of type INT_MS_T - message number of the incoming packet (as given by message source)
 * \param[out] outSeqNum - of type INT_MS_T - sequence number of the incoming packet (as given by Xmsg Server)
 * \param[out] outHdr - of type STR_MS_T - header string of the incoming message packet
 * \param[out] outMsg - of type STR_MS_T - message string of the incoming message packet
 * \param[out] outUser - of type STR_MS_T - userName@Zone of the sender of the packet
 * \param[out] outAddr - of type STR_MS_T - address of the sending site of the packet (host address and process-id)
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect message packet may have its status changed in the XMsg Server
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiXmsgCreateStream, writeXMsg
**/

int
readXMsg(msParam_t* inStreamId, msParam_t *inCondRead,  
	 msParam_t *outMsgNum, msParam_t *outSeqNum, 
	 msParam_t *outHdr, msParam_t *outMsg, 
	 msParam_t *outUser, msParam_t *outAddr, ruleExecInfo_t *rei)
{
  int i;
  int sNum = 0;
  int mNum = 0;
  char *hdr = NULL;
  char *msg = NULL;
  char *user = NULL;
  char *addr = NULL;
  int streamId;
  xmsgTicketInfo_t *xmsgTicketInfo;
  char *condRead = NULL;
  RE_TEST_MACRO ("    Calling readXMsg");

  if (!strcmp(inStreamId->type,XmsgTicketInfo_MS_T)) {
    xmsgTicketInfo = (xmsgTicketInfo_t *) inStreamId->inOutStruct;
    streamId = xmsgTicketInfo->rcvTicket;
  }
  else if (!strcmp(inStreamId->type,STR_MS_T)) {
    streamId = (int) atoi((char*)inStreamId->inOutStruct);
  }
  else
    streamId = (int) CAST_PTR_INT  inStreamId->inOutStruct;
  condRead = (char *) inCondRead->inOutStruct;
  i = _readXMsg(streamId, condRead, &mNum, &sNum, &hdr, &msg, &user, &addr);
  if (i >= 0) {
    outHdr->inOutStruct = (void *) hdr;
    outHdr->type = strdup(STR_MS_T);
    outMsg->inOutStruct = (void *) msg;
    outMsg->type = strdup(STR_MS_T);
    fillIntInMsParam(outMsgNum, mNum);
    fillIntInMsParam(outSeqNum, sNum);
    outUser->inOutStruct = (void *) user;
    outUser->type = strdup(STR_MS_T);
    outAddr->inOutStruct = (void *) addr;
    outAddr->type = strdup(STR_MS_T);

  }
  return(i);
}

