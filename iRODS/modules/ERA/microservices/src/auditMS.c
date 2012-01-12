/**
 * @file auditMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "auditMS.h"
#include "auditUtil.h"


/**
 * \fn msiGetAuditTrailInfoByUserID (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief   This microservice gets audit trail information by the user identifier
 *
 * \module ERA  
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date   10/2007
 *
 * \usage See clients/icommands/test/rules3.0/
 * 
 * \param[in] inpParam1 - a msParam of type STR_MS_T
 * \param[in] inpParam2 - a msParam of type BUF_LEN_MS_T
 * \param[out] outParam - a msParam of operation status INT_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiGetAuditTrailInfoByUserID(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
{
	rsComm_t *rsComm; 
	char *userID;
	bytesBuf_t *mybuf;
	
	
	RE_TEST_MACRO ("    Calling msiGetAuditTrailInfoByUserID")
	
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiGetAuditTrailInfoByUserID: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	rsComm = rei->rsComm;
	
	/* buffer init */
	mybuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	memset (mybuf, 0, sizeof (bytesBuf_t));


	/* parse inpParam1 (user ID input string) */
	if ((userID = parseMspForStr (inpParam1)) == NULL) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			"msiGetAuditTrailInfoByUserID: parseMspForStr error for param 1.");
		free( mybuf ); // JMC cppcheck - leak
		return (rei->status);
	}

	
	/* call getAuditTrailInfoByUserID() */
	rei->status = getAuditTrailInfoByUserID(userID, mybuf, rsComm);

	/* failure? */
	if (rei->status < 0) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			"msiGetAuditTrailInfoByUserID: getAuditTrailInfoByUserID failed for user ID %s, status = %d", userID, rei->status);
	}


	/* return operation status through outParam */
	fillIntInMsParam (outParam, rei->status);


	/* send result buffer, even if length is 0, to inParam2 */
	if (!mybuf->buf) {
		mybuf->buf = strdup("");
	}
	fillBufLenInMsParam (inpParam2, strlen((char*)mybuf->buf), mybuf);

	free( mybuf ); // JMC cppcheck - leak
	
	return (rei->status);

}



/**
 * \fn msiGetAuditTrailInfoByObjectID (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief   This function gets audit trail information by the object identifier
 *  
 * \module ERA
 *  
 * \since pre-2.1
 *  
 * \author  Antoine de Torcy
 * \date   10/2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *  
 * \param[in] inpParam1 - a msParam of type STR_MS_T
 * \param[in] inpParam2 - a msParam of type BUF_LEN_MS_T
 * \param[out] outParam - a msParam of operation status INT_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiGetAuditTrailInfoByObjectID(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
{
	rsComm_t *rsComm; 
	char *objectID;
	bytesBuf_t *mybuf;
	
	
	RE_TEST_MACRO ("    Calling msiGetAuditTrailInfoByObjectID")
	
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiGetAuditTrailInfoByObjectID: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	rsComm = rei->rsComm;
	
	/* buffer init */
	mybuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	memset (mybuf, 0, sizeof (bytesBuf_t));


	/* parse inpParam1 (object ID input string) */
	if ((objectID = parseMspForStr (inpParam1)) == NULL) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			"msiGetAuditTrailInfoByObjectID: parseMspForStr error for param 1.");
	    free( mybuf ); // JMC cppcheck - leak
		return (rei->status);
	}

	
	/* call getAuditTrailInfoByObjectID() */
	rei->status = getAuditTrailInfoByObjectID(objectID, mybuf, rsComm);

	/* failure? */
	if (rei->status < 0) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			"msiGetAuditTrailInfoByObjectID: getAuditTrailInfoByObjectID failed for object ID %s, status = %d", objectID, rei->status);
	}


	/* return operation status through outParam */
	fillIntInMsParam (outParam, rei->status);


	/* send result buffer, even if length is 0, to inParam2 */
	if (!mybuf->buf) {
		mybuf->buf = strdup("");
	}
	fillBufLenInMsParam (inpParam2, strlen((char*)mybuf->buf), mybuf);

	free( mybuf ); // JMC cppcheck - leak
	
	return (rei->status);

}



/**
 * \fn msiGetAuditTrailInfoByActionID (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief   This function gets audit trail information by the action identifier
 *
 * \module ERA
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date   10/2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *  
 * \param[in] inpParam1 - a msParam of type STR_MS_T
 * \param[in] inpParam2 - a msParam of type BUF_LEN_MS_T
 * \param[out] outParam - a msParam of operation status INT_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiGetAuditTrailInfoByActionID(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
{
	rsComm_t *rsComm; 
	char *actionID;
	bytesBuf_t *mybuf;
	
	
	RE_TEST_MACRO ("    Calling msiGetAuditTrailInfoByActionID")
	
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiGetAuditTrailInfoByActionID: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	rsComm = rei->rsComm;
	
	/* buffer init */
	mybuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	memset (mybuf, 0, sizeof (bytesBuf_t));


	/* parse inpParam1 (action ID input string) */
	if ((actionID = parseMspForStr (inpParam1)) == NULL) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			"msiGetAuditTrailInfoByActionID: parseMspForStr error for param 1.");
	    free( mybuf ); // JMC cppcheck - leak
		return (rei->status);
	}

	
	/* call getAuditTrailInfoByActionID() */
	rei->status = getAuditTrailInfoByActionID(actionID, mybuf, rsComm);

	/* failure? */
	if (rei->status < 0) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			"msiGetAuditTrailInfoByActionID: getAuditTrailInfoByActionID failed for action ID %s, status = %d", actionID, rei->status);
	}


	/* return operation status through outParam */
	fillIntInMsParam (outParam, rei->status);


	/* send result buffer, even if length is 0, to inParam2 */
	if (!mybuf->buf) {
		mybuf->buf = strdup("");
	}
	fillBufLenInMsParam (inpParam2, strlen((char*)mybuf->buf), mybuf);

	free( mybuf ); // JMC cppcheck - leak
	
	return (rei->status);

}



/**
 * \fn msiGetAuditTrailInfoByKeywords (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief   This function gets audit trail information by keywords in the comment field
 *  
 * \module ERA
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2007-10
 *
 * \usage See clients/icommands/test/rules3.0/
 *  
 * \param[in] inpParam1 - a msParam of type STR_MS_T
 * \param[in] inpParam2 - a msParam of type BUF_LEN_MS_T
 * \param[out] outParam - a msParam of operation status INT_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiGetAuditTrailInfoByKeywords(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei)
{
	rsComm_t *rsComm; 
	char *commentStr;
	bytesBuf_t *mybuf;
	
	
	RE_TEST_MACRO ("    Calling msiGetAuditTrailInfoByKeywords")
	
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiGetAuditTrailInfoByKeywords: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	rsComm = rei->rsComm;
	
	/* buffer init */
	mybuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	memset (mybuf, 0, sizeof (bytesBuf_t));


	/* parse inpParam1 (comment input string) */
	if ((commentStr = parseMspForStr (inpParam1)) == NULL) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			"msiGetAuditTrailInfoByKeywords: parseMspForStr error for param 1.");
	    free( mybuf ); // JMC cppcheck - leak
		return (rei->status);
	}

	
	/* call getAuditTrailInfoByKeywords() */
	rei->status = getAuditTrailInfoByKeywords(commentStr, mybuf, rsComm);

	/* failure? */
	if (rei->status < 0) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			"msiGetAuditTrailInfoByKeywords: getAuditTrailInfoByKeywords failed for comment string %s, status = %d", commentStr, rei->status);
	}


	/* return operation status through outParam */
	fillIntInMsParam (outParam, rei->status);


	/* send result buffer, even if length is 0, to inParam2 */
	if (!mybuf->buf) {
		mybuf->buf = strdup("");
	}
	fillBufLenInMsParam (inpParam2, strlen((char*)mybuf->buf), mybuf);

	free( mybuf ); // JMC cppcheck - leak
	
	return (rei->status);

}



/**
 * \fn msiGetAuditTrailInfoByTimeStamp (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief   This microservice gets audit trail information by the timestamp
 *
 * \module ERA   
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date   2007-10
 *
 * \usage See clients/icommands/test/rules3.0/
 *  
 * \param[in] inpParam1 - a msParam of type STR_MS_T
 * \param[in] inpParam2 - a msParam of type STR_MS_T
 * \param[in] inpParam3 - a msParam of type BUF_LEN_MS_T
 * \param[out] outParam - a msParam of operation status INT_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
**/
int
msiGetAuditTrailInfoByTimeStamp(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei)
{
	rsComm_t *rsComm; 
	char *begTS;
	char *endTS;
	bytesBuf_t *mybuf;
	
	
	RE_TEST_MACRO ("    Calling msiGetAuditTrailInfoByTimeStamp")
	
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiGetAuditTrailInfoByTimeStamp: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	rsComm = rei->rsComm;
	
	/* buffer init */
	mybuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	memset (mybuf, 0, sizeof (bytesBuf_t));


	/* NULL values for timestamps don't generate errors */
	/* parse inpParam1 (beginning timestamp) */
	if ((begTS = parseMspForStr (inpParam1)) == NULL) {
		begTS = strdup("");
	}

	/* parse inpParam2 (end timestamp) */
	if ((endTS = parseMspForStr (inpParam2)) == NULL) {
		endTS = strdup("");
	}

	
	/* call getAuditTrailInfoByTimeStamp() */
	rei->status = getAuditTrailInfoByTimeStamp(begTS, endTS, mybuf, rsComm);

	/* failure? */
	if (rei->status < 0) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
			"msiGetAuditTrailInfoByTimeStamp: getAuditTrailInfoByTimeStamp failed for values between %s and %s, status = %d", begTS, endTS, rei->status);
	}


	/* return operation status through outParam */
	fillIntInMsParam (outParam, rei->status);

	/* send result buffer, even if length is 0, to inParam3 */
	if (!mybuf->buf) {
		mybuf->buf = strdup("");
	}
	fillBufLenInMsParam (inpParam3, strlen((char*)mybuf->buf), mybuf);
	
	return (rei->status);

}
