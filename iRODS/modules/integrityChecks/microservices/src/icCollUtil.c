/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* icCollUtil.c
 */

#include "icCollUtil.h"
#include "icutils.h"

int	icCollOps (char* collname, char* operation, char* oplist, bytesBuf_t* mybuf, int status) {

	rsComm_t* rsComm;
	genQueryInp_t gqin;
	genQueryOut_t* gqout=NULL;
	char condStr[MAX_NAME_LEN];
	char tmpstr[MAX_NAME_LEN];

	/* init stuff */
	memset (&gqin, 0, sizeof(genQueryInp_t));
	gqin.maxRows = MAX_SQL_ROWS;
	gqout = (genQueryOut_t*) malloc (sizeof (genQueryOut_t));
	memset (gqout, 0, sizeof (genQueryOut_t));

	/* Generate a query - we only want subcollection data objects */
    addInxIval (&gqin.selectInp, COL_COLL_NAME, 1);
    addInxIval (&gqin.selectInp, COL_COLL_ID, 1); 
	genAllInCollQCond (collname, condStr);
    addInxVal (&gqin.sqlCondInp, COL_COLL_NAME, condStr);

	/* Determine which data we want to receive - ownerstuff, ACL stuff or AVU stuff */
	if (!(strcmp(operation,"owner"))) {
		addInxIval (&gqin.selectInp, COL_COLL_OWNER_NAME, 1);
	} else if (!(strcmp(operation, "AVU"))) {
		addInxIval (&gqin.selectInp, COL_META_COLL_ATTR_NAME, 1);
		addInxIval (&gqin.selectInp, COL_META_COLL_ATTR_VALUE, 1);
		addInxIval (&gqin.selectInp, COL_META_COLL_ATTR_UNITS, 1);
	} else if (!(strcmp(operation, "ACL"))) {
		addInxIval (&gqin.selectInp, COL_COLL_ACCESS_TYPE, 1);
		addInxIval (&gqin.selectInp, COL_COLL_ACCESS_NAME, 1);
	} else {
		rodsLog (LOG_ERROR, "icCollOps: ERROR");
		free( gqout ); // JMC cppcheck - leak
		return (-1); 
	}

	/* This is effectively a recursive query because of the condStr */
    status = rsGenQuery (rsComm, &gqin, &gqout);
	fprintf (stderr, "status=%d\n", status);

	if ((status==CAT_NO_ROWS_FOUND) || (status < 0)) {
		snprintf (tmpstr, MAX_NAME_LEN, "No rows found matching input criteria.\n");
		appendToByteBuf (mybuf, tmpstr);

	/* assume at this point we have valid data for all three possible query types */
	} else if (!(strcmp(operation,"owner"))) {
		// process owners query results
		verifyCollOwners (gqout, oplist, mybuf);
	} else if (!(strcmp(operation, "AVU"))) {
		// process AVU query results
		//verifyCollAVU (gqout, oplist, mybuf);
	} else if (!(strcmp(operation, "ACL"))) {
		// process ACL query results
		//verifyCollACL (gqout, oplist, mybuf);
	}

	printGenQueryOut(stderr, NULL, NULL, gqout);

	free( gqout ); // JMC cppcheck - leak
	return (status);

}
	

/* the following functions are wrappers for icCollOps function */
int msiVerifySubCollOwner (msParam_t* collinp, msParam_t* ownerinp, 
	msParam_t *bufout, msParam_t* statout) {

	bytesBuf_t* mybuf=NULL;
	char* collname;
	char* ownerlist;
	int status;

    mybuf = (bytesBuf_t *) malloc (sizeof (bytesBuf_t));
    memset (mybuf, 0, sizeof (bytesBuf_t));

	collname = strdup ((char*)collinp->inOutStruct);
	ownerlist = strdup ((char*)ownerinp->inOutStruct);
	
	status = icCollOps (collname, "owner", ownerlist, mybuf, status);

	fillStrInMsParam (bufout, (char*)mybuf->buf);
	fillIntInMsParam (statout, status);
	return (status);
}

int msiVerifySubCollAVU (msParam_t* collinp, msParam_t* avunameinp, msParam_t* avuvalueinp, 
	msParam_t* avuattrinp, msParam_t *bufout, msParam_t* statout) {

	rsComm_t* rsComm;
	genQueryInp_t gqin;
	genQueryOut_t* gqout=NULL;
	char condStr[MAX_NAME_LEN];
	char tmpstr[MAX_NAME_LEN];
	bytesBuf_t* mybuf=NULL;
	char* collname;
	char* myavuname;
	char* myavuvalue;
	char* myavuattr;;
	int status;

	/* init stuff */
	memset (&gqin, 0, sizeof(genQueryInp_t));
	gqin.maxRows = MAX_SQL_ROWS;
	gqout = (genQueryOut_t*) malloc (sizeof (genQueryOut_t));
	memset (gqout, 0, sizeof (genQueryOut_t));
    mybuf = (bytesBuf_t *) malloc (sizeof (bytesBuf_t));
    memset (mybuf, 0, sizeof (bytesBuf_t));

	collname = strdup ((char*)collinp->inOutStruct);
	myavuname = strdup ((char*)avunameinp->inOutStruct);
	myavuvalue = strdup ((char*)avuvalueinp->inOutStruct);
	myavuattr = strdup ((char*)avuattrinp->inOutStruct);
	
	/* Generate a query - we only want subcollection data objects */
    addInxIval (&gqin.selectInp, COL_COLL_NAME, 1);
    addInxIval (&gqin.selectInp, COL_COLL_ID, 1); 
	genAllInCollQCond (collname, condStr);
    addInxVal (&gqin.sqlCondInp, COL_COLL_NAME, condStr);

	/* get the AVU fields */
	addInxIval (&gqin.selectInp, COL_META_COLL_ATTR_NAME, 1);
	addInxIval (&gqin.selectInp, COL_META_COLL_ATTR_VALUE, 1);
	addInxIval (&gqin.selectInp, COL_META_COLL_ATTR_UNITS, 1);

	/* This is effectively a recursive query because of the condStr */
    status = rsGenQuery (rsComm, &gqin, &gqout);
	fprintf (stderr, "status=%d\n", status);

	if ((status==CAT_NO_ROWS_FOUND) || (status < 0)) {
		snprintf (tmpstr, MAX_NAME_LEN, "No rows found matching input criteria.\n");
		appendToByteBuf (mybuf, tmpstr);
	} else
		verifyCollAVU (gqout, myavuname, myavuvalue, myavuattr, mybuf);

	fillStrInMsParam (bufout, (char*)mybuf->buf);
	fillIntInMsParam (statout, status);
	return (status);
}

int msiVerifySubCollACL (msParam_t* collinp, msParam_t* acltypeinp, msParam_t* aclnameinp, msParam_t *bufout, msParam_t* statout) {

	rsComm_t* rsComm;
	genQueryInp_t gqin;
	genQueryOut_t* gqout=NULL;
	bytesBuf_t* mybuf;
	char condStr[MAX_NAME_LEN];
	char tmpstr[MAX_NAME_LEN];
	char* collname;
	char* myaclname;
	char* myacltype;
	int status;

	/* init stuff */
	memset (&gqin, 0, sizeof(genQueryInp_t));
	gqin.maxRows = MAX_SQL_ROWS;
	gqout = (genQueryOut_t*) malloc (sizeof (genQueryOut_t));
	memset (gqout, 0, sizeof (genQueryOut_t));
	mybuf = (bytesBuf_t *) malloc (sizeof (bytesBuf_t));
	memset (mybuf, 0, sizeof (bytesBuf_t));

	collname = strdup ((char*)collinp->inOutStruct);
	myaclname = strdup ((char*)aclnameinp->inOutStruct);
	myacltype = strdup ((char*)acltypeinp->inOutStruct);

	/* Generate a query - we only want subcollection data objects */
    addInxIval (&gqin.selectInp, COL_COLL_NAME, 1);
    addInxIval (&gqin.selectInp, COL_COLL_ID, 1); 
	genAllInCollQCond (collname, condStr);
    addInxVal (&gqin.sqlCondInp, COL_COLL_NAME, condStr);

	/* Determine which data we want to receive - ownerstuff, ACL stuff or AVU stuff */
	addInxIval (&gqin.selectInp, COL_COLL_ACCESS_TYPE, 1);
	addInxIval (&gqin.selectInp, COL_COLL_ACCESS_NAME, 1);

	/* This is effectively a recursive query because of the condStr */
    status = rsGenQuery (rsComm, &gqin, &gqout);
	fprintf (stderr, "status=%d\n", status);

	if ((status==CAT_NO_ROWS_FOUND) || (status < 0)) {
		snprintf (tmpstr, MAX_NAME_LEN, "No rows found matching input criteria.\n");
		appendToByteBuf (mybuf, tmpstr);
	} else
		verifyCollACL (gqout, myaclname, myacltype, mybuf);

	printGenQueryOut(stderr, NULL, NULL, gqout);

	fillStrInMsParam (bufout, (char*)mybuf->buf);
	fillIntInMsParam (statout, status);
	return (status);
}





int	msiListColl (msParam_t* collectionname, msParam_t* buf, ruleExecInfo_t* rei) {

	rsComm_t* rsComm;
	sqlResult_t *collectionName;
	sqlResult_t *collectionID;
	bytesBuf_t* mybuf=NULL;
	genQueryInp_t gqin;
	genQueryOut_t* gqout=NULL;
	int i,status;
	char condStr[MAX_NAME_LEN];
	char* collname;

	RE_TEST_MACRO ("    Calling msiListColl")

	/* Sanity check */
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiListColl: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	rsComm = rei->rsComm;

	/* need to turn parameter 1 into a collInp_t struct */
	collname = strdup ((char*)collectionname->inOutStruct);
	
	/* init stuff */
	memset (&gqin, 0, sizeof(genQueryInp_t));
	gqin.maxRows = MAX_SQL_ROWS;
	gqout = (genQueryOut_t*) malloc (sizeof (genQueryOut_t));
	memset (gqout, 0, sizeof (genQueryOut_t));
	mybuf = (bytesBuf_t *) malloc (sizeof (bytesBuf_t));
	memset (mybuf, 0, sizeof (bytesBuf_t));

	/* Generate a query - we only want subcollection data objects */
    addInxIval (&gqin.selectInp, COL_COLL_NAME, 1);
    addInxIval (&gqin.selectInp, COL_COLL_ID, 1);
	genAllInCollQCond (collname, condStr);
    addInxVal (&gqin.sqlCondInp, COL_COLL_NAME, condStr);

	/* This is effectively a recursive query */
    status = rsGenQuery (rsComm, &gqin, &gqout);

	if (status < 0) {
		free( mybuf ); // JMC cppcheck - leak
		return (status);
	} else if (status != CAT_NO_ROWS_FOUND)  {

		collectionName = getSqlResultByInx (gqout, COL_COLL_NAME);
		collectionID = getSqlResultByInx (gqout, COL_COLL_ID);
		for (i=1; i<gqout->rowCnt; i++) {

			char* subcoll;
			char tmpstr[MAX_NAME_LEN];

			subcoll = &collectionName->value[collectionName->len*i];
			sprintf (tmpstr, "%s\n", subcoll );
			appendToByteBuf (mybuf, tmpstr);
		}

	} else {
		return (status); /* something bad happened */
	}

    fillBufLenInMsParam (buf, mybuf->len, mybuf);	
	return (rei->status);

}

