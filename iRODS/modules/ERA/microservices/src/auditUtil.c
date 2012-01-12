/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "auditUtil.h"
#include "eraUtil.h"

/*
 * getAuditTrailInfoByUserID() - Extracts audit trail information from the iCAT for a given user ID
 *
 */
int 
getAuditTrailInfoByUserID(char *userID, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	char condStr[MAX_NAME_LEN];
	int status;
	char *attrs[] = {"audit_item", "object_id", "user_id", "action_id", "r_comment", "create_ts", "modify_ts"};

	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByUserID: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* check for proper input */
	if (userID == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByUserID: userID input is NULL");
		return (USER__NULL_INPUT_ERR);
	}


	/* Prepare query */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_OBJ_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_USER_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_ACTION_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_COMMENT, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_CREATE_TIME, ORDER_BY);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_MODIFY_TIME, 1);


	/* make the condition */
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", userID);
	addInxVal (&genQueryInp.sqlCondInp, COL_AUDIT_USER_ID, condStr);

	genQueryInp.maxRows = MAX_SQL_ROWS;

	
	/* ICAT query for user info */
	status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	/* print results out to buffer */
	if (status == 0) {
		genQueryOutToXML(genQueryOut, mybuf, attrs);
	
		/* not done? */
		while (status==0 && genQueryOut->continueInx > 0) {
			genQueryInp.continueInx=genQueryOut->continueInx;
			status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);

			genQueryOutToXML(genQueryOut, mybuf, attrs);
		}
	}


	return (0);
}



/*
 * getAuditTrailInfoByObjectID() - Extracts audit trail information from the iCAT for a given object ID
 *
 */
int 
getAuditTrailInfoByObjectID(char *objectID, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	char condStr[MAX_NAME_LEN];
	int status;
	char *attrs[] = {"audit_item", "object_id", "user_id", "action_id", "r_comment", "create_ts", "modify_ts"};

	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByObjectID: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* check for proper input */
	if (objectID == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByObjectID: objectID input is NULL");
		return (USER__NULL_INPUT_ERR);
	}


	/* Prepare query */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_OBJ_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_USER_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_ACTION_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_COMMENT, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_CREATE_TIME, ORDER_BY);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_MODIFY_TIME, 1);


	/* make the condition */
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", objectID);
	addInxVal (&genQueryInp.sqlCondInp, COL_AUDIT_OBJ_ID, condStr);

	genQueryInp.maxRows = MAX_SQL_ROWS;

	
	/* ICAT query for user info */
	status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	/* print results out to buffer */
	if (status == 0) {
		genQueryOutToXML(genQueryOut, mybuf, attrs);
	
		/* not done? */
		while (status==0 && genQueryOut->continueInx > 0) {
			genQueryInp.continueInx=genQueryOut->continueInx;
			status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);

			genQueryOutToXML(genQueryOut, mybuf, attrs);
		}
	}


	return (0);
}



/*
 * getAuditTrailInfoByActionID() - Extracts audit trail information from the iCAT for a given action ID
 *
 */
int 
getAuditTrailInfoByActionID(char *actionID, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	char condStr[MAX_NAME_LEN];
	int status;
	char *attrs[] = {"audit_item", "object_id", "user_id", "action_id", "r_comment", "create_ts", "modify_ts"};

	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByActionID: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* check for proper input */
	if (actionID == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByActionID: actionID input is NULL");
		return (USER__NULL_INPUT_ERR);
	}


	/* Prepare query */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_OBJ_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_USER_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_ACTION_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_COMMENT, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_CREATE_TIME, ORDER_BY);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_MODIFY_TIME, 1);


	/* make the condition */
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", actionID);
	addInxVal (&genQueryInp.sqlCondInp, COL_AUDIT_ACTION_ID, condStr);

	genQueryInp.maxRows = MAX_SQL_ROWS;

	
	/* ICAT query for user info */
	status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	/* print results out to buffer */
	if (status == 0) {
		genQueryOutToXML(genQueryOut, mybuf, attrs);
	
		/* not done? */
		while (status==0 && genQueryOut->continueInx > 0) {
			genQueryInp.continueInx=genQueryOut->continueInx;
			status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);

			genQueryOutToXML(genQueryOut, mybuf, attrs);
		}
	}


	return (0);
}



/*
 * getAuditTrailInfoByKeywords() - Extracts audit trail information from the iCAT for given keywords in the comment field
 *
 */
int 
getAuditTrailInfoByKeywords(char *keywordStr, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	char condStr[MAX_NAME_LEN];
	int status;
	char *attrs[] = {"audit_item", "object_id", "user_id", "action_id", "r_comment", "create_ts", "modify_ts"};

	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByKeywords: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* check for proper input */
	if (keywordStr == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByKeywords: keywordStr input is NULL");
		return (USER__NULL_INPUT_ERR);
	}


	/* Prepare query */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_OBJ_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_USER_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_ACTION_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_COMMENT, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_CREATE_TIME, ORDER_BY);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_MODIFY_TIME, 1);


	/* make the condition */
	/* '%' in keyword string is treated as a wildcard */
	if (strstr(keywordStr, "%")) {
		snprintf (condStr, MAX_NAME_LEN, " like '%s'", keywordStr);
	}
	else {
		snprintf (condStr, MAX_NAME_LEN, " = '%s'", keywordStr);
	}

	addInxVal (&genQueryInp.sqlCondInp, COL_AUDIT_COMMENT, condStr);




	genQueryInp.maxRows = MAX_SQL_ROWS;

	
	/* ICAT query for user info */
	status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	/* print results out to buffer */
	if (status == 0) {
		genQueryOutToXML(genQueryOut, mybuf, attrs);
	
		/* not done? */
		while (status==0 && genQueryOut->continueInx > 0) {
			genQueryInp.continueInx=genQueryOut->continueInx;
			status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);

			genQueryOutToXML(genQueryOut, mybuf, attrs);
		}
	}


	return (0);
}



/*
 * getAuditTrailInfoByTimeStamp() - Extracts audit trail information from the iCAT between two dates
 *
 */
int 
getAuditTrailInfoByTimeStamp(char *begTS, char *endTS, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	char condStr[MAX_NAME_LEN];
	int status;
	char *attrs[] = {"audit_item", "object_id", "user_id", "action_id", "r_comment", "create_ts", "modify_ts"};

	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByTimeStamp: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* check for proper input timestamp values */
	if (begTS == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByTimeStamp: begTS input is NULL");
		return (USER__NULL_INPUT_ERR);
	}

	if (endTS == NULL) {
		rodsLog (LOG_ERROR, "getAuditTrailInfoByTimeStamp: endTS input is NULL");
		return (USER__NULL_INPUT_ERR);
	}


	/* Prepare query */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_OBJ_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_USER_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_ACTION_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_COMMENT, 1);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_CREATE_TIME, ORDER_BY);
	addInxIval (&genQueryInp.selectInp, COL_AUDIT_MODIFY_TIME, 1);


	/* make the conditions */
	if (strlen(begTS)) {
		snprintf (condStr, MAX_NAME_LEN, " >= '%s'", begTS);
		addInxVal (&genQueryInp.sqlCondInp, COL_AUDIT_MODIFY_TIME, condStr);
	}

	if (strlen(endTS)) {
		snprintf (condStr, MAX_NAME_LEN, " < '%s'", endTS);
		addInxVal (&genQueryInp.sqlCondInp, COL_AUDIT_MODIFY_TIME, condStr);
	}



	genQueryInp.maxRows = MAX_SQL_ROWS;

	
	/* ICAT query for user info */
	status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	/* print results out to buffer */
	if (status == 0) {
		genQueryOutToXML(genQueryOut, mybuf, attrs);
	
		/* not done? */
		while (status==0 && genQueryOut->continueInx > 0) {
			genQueryInp.continueInx=genQueryOut->continueInx;
			status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);

			genQueryOutToXML(genQueryOut, mybuf, attrs);
		}
	}


	return (0);
}





