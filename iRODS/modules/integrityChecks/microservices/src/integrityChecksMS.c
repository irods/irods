/**
 * @file integrityChecksMS.c
 *
 */

#include "integrityChecksMS.h"
#include "icutils.h"

/* Check and see if the owner is in a comma separated list */

/**
 * \fn msiVerifyOwner (msParam_t* collinp, msParam_t* ownerinp, msParam_t* bufout, msParam_t* statout, ruleExecInfo_t *rei)
 *
 * \brief This microservice checks if files in a given collection have a consistent owner.
 *
 * \deprecated Since 3.0, the integrityChecks module microservices have been reproduced
 *    using rules.  These microservices only handled 256 files per collection.
 *    The example rules handle an arbitrary number of files.
 *
 * \module integrityChecks
 *
 * \since pre-2.1
 *
 * \author  Susan Lindsey
 * \date    August 2008
 *
 * \usage See clients/icommands/test/rules3.0/ 
 *
 * \param[in] collinp - a STR_MS_T containing the collection's name
 * \param[in] ownerinp - a STR_MS_T containing comma separated list of owner usernames
 * \param[out] bufout - a STR_MS_T containing the output string
 * \param[out] statout - the returned status
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
 * \retval rei->status
 * \pre none
 * \post none
 * \sa none
**/
int
msiVerifyOwner (msParam_t* collinp, msParam_t* ownerinp, msParam_t* bufout, msParam_t* statout, ruleExecInfo_t *rei)
{

	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut = NULL;
	char condStr[MAX_NAME_LEN];
	rsComm_t *rsComm = NULL;
	char* collname = NULL;
	char* ownerlist = NULL;
	int i,j;
	sqlResult_t *dataName;
	sqlResult_t *dataOwner;
	char delims[]=",";
	char* word = NULL;
	char** olist=NULL;
	bytesBuf_t*	stuff=NULL;
	
	RE_TEST_MACRO ("    Calling msiVerifyOwner")

	/* Sanity check */
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiVerifyOwner: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	rsComm = rei->rsComm;

	memset (&genQueryInp, 0, sizeof(genQueryInp_t));
	genQueryInp.maxRows = MAX_SQL_ROWS;

   /* buffer init */
    stuff = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
    memset (stuff, 0, sizeof (bytesBuf_t));

	/* construct an SQL query from the parameter list */
	collname = strdup ((char*)collinp->inOutStruct);

	/* this is the info we want returned from the query */
	addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_D_OWNER_NAME, 1);
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", collname);
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr);

	j = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	/* Now for each file retrieved in the query, determine if it's owner is in our list */

	if (j != CAT_NO_ROWS_FOUND) {

		printGenQueryOut(stderr, NULL, NULL, genQueryOut);

		dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME);
		dataOwner = getSqlResultByInx (genQueryOut, COL_D_OWNER_NAME);

		ownerlist = strdup ((char*)ownerinp->inOutStruct);
		//fprintf(stderr, "ownerlist: %s\n", ownerlist);

		if (strlen(ownerlist)>0) { /* our rule contains a list of owners we want to compare against */
			int ownercount=0;

			/* Construct a list of owners*/
			for (word=strtok(ownerlist, delims); word; word=strtok(NULL, delims)) {
				// JMC cppcheck - realloc failure case
				char** tmp_list = 0;
				tmp_list = (char**) realloc (olist, sizeof (char*) * (ownercount));  
				if( !tmp_list ) { 
					rodsLog( LOG_ERROR, "msiVerifyOwner - realloc failure" );
					continue;
				} else {
					olist = tmp_list;
				}
				olist[ownercount] = strdup (word);
				ownercount++;
			}

			/* Now compare each file's owner with our list */
			for (i=0; i<genQueryOut->rowCnt; i++) {
				int foundflag=0;
				for (j=0; j<ownercount; j++) {
					char* thisowner = strdup(&dataOwner->value[dataOwner->len*i]);

					//fprintf(stderr, "comparing %s and %s\n", thisowner, olist[j]);
					if (!(strcmp(thisowner, olist[j]))) {
						/* We only care about the ones that DON'T match */
						foundflag=1;
						break;
					}
                    free( thisowner ); // JMC cppcheck - leak 
				}

				if (!foundflag) {
					char tmpstr[80];
					sprintf (tmpstr, "File: %s with owner: %s does not match input list\n", 
						&dataName->value[dataName->len *i], &dataOwner->value[dataOwner->len * i]);
					appendToByteBuf (stuff, tmpstr);
				}
			}

		} else { /* input parameter for owner is not set */
			
			rodsLog (LOG_ERROR, "msiVerifyOwner: ownerlist is NULL");

			/* just check if the owner name (whatever it is) is consistent across all files */
			/* We'll just compare using the first file's owner - this can probably be done several ways */
			char* firstowner = strdup(&dataOwner->value[0]); 

			int matchflag=1;  /* Start off assuming all owners match */

			for (i=1; i<genQueryOut->rowCnt; i++) {
				char* thisowner = strdup( &dataOwner->value[dataOwner->len*i]);
				if (strcmp(firstowner, thisowner)) { /* the two strings are not equal */
					appendToByteBuf (stuff, "Owner is not consistent across this collection");
					matchflag=0;
					break;
				}
                free( thisowner ); // JMC cppcheck - leak 
			}

			if (matchflag) /* owner field was consistent across all data objects */
				appendToByteBuf (stuff, "Owner is consistent across this collection.\n");
			else
				appendToByteBuf (stuff, "Owner is not consistent across this collection.\n");
		
            free( firstowner ); // JMC cppcheck - leak 
		
		}	

		free( olist ); // JMC cppcheck - leak
	} 

	fillBufLenInMsParam (bufout, stuff->len, stuff);
	fillIntInMsParam (statout, rei->status);
	free( ownerlist ); // JMC cppcheck - leak 
    free( collname ); // JMC cppcheck - leak 
	return(rei->status);

}

/**
 * \fn msiVerifyACL (msParam_t* collinp, msParam_t* userinp, msParam_t* authinp, msParam_t* notflaginp, 
 *    msParam_t* bufout, msParam_t* statout, ruleExecInfo_t *rei)
 *
 * \brief Check the ACL on a collection
 *
 * \deprecated Since 3.0, the integrityChecks module microservices have been reproduced
 *    using rules.  These microservices only handled 256 files per collection.
 *    The example rules handle an arbitrary number of files.
 *
 * \module integrityChecks
 *
 * \since pre-2.1
 *
 * \author  Susan Lindsey
 * \date    August 2008
 *
 * \note
 * \code
 *    This function can perform three different checks, depending on flags set via input parameters:
 *    1. check that its ACL contains the same set of user-authorization pairs as others in its collection
 *    2. check that its ACL contains at least a given set of user-authorization pairs
 *    3. check that its ACL does not contain a given set of user-authorization pairs 
 *    
 *    We have four input parameters: Collection Name, User Name, Authorization Type & NOT flag
 *    For the above conditions, the following are examples of how to call the rule
 *    1. collname=/sdscZone/home/rods%*User=rods%*Auth=own  
 *    2. collname=/sdscZone/home/rods  
 *    3. collname=/sdscZone/home/rods%*User=rods%*Auth=own*Notflag=1  
 * \endcode
 *    
 * \usage See clients/icommands/test/rules3.0/ 
 *
 * \param[in] collinp - a STR_MS_T containing the collection's name
 * \param[in] userinp - Optional - a STR_MS_T containing comma separated list of owner usernames
 * \param[in] authinp - Optional - a STR_MS_T containing comma separated list 
 * \param[in] notflaginp - Optional - a STR_MS_T
 * \param[out] bufout - a STR_MS_T containing the output string
 * \param[out] statout - the returned status
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
 * \retval rei->status
 * \pre none
 * \post none
 * \sa none
**/
int
msiVerifyACL (msParam_t* collinp, msParam_t* userinp, msParam_t* authinp, msParam_t* notflaginp, 
	msParam_t* bufout, msParam_t* statout, ruleExecInfo_t *rei)
{

	int i,j;
	int querytype=0;
	char tmpstr[MAX_NAME_LEN];
	genQueryInp_t	gqin;
	genQueryOut_t*	gqout = NULL;
	char* collname=NULL;
	char* collid=NULL;
	char* username=NULL;
	char* accessauth=NULL;
	char* notflag=NULL;
	rsComm_t *rsComm;
	bytesBuf_t* mybuf=NULL;
	sqlResult_t* collCollId;
	sqlResult_t* dataName;
	sqlResult_t* dataAccessName;
	sqlResult_t* dataAccessType;
	sqlResult_t* dataAccessDataId;
	sqlResult_t* dataAccessUserId;
	//sqlResult_t* dataTokenNamespace;

	RE_TEST_MACRO ("    Calling msiVerifyACL")

	/* Sanity check */
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiVerifyACL: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	rsComm = rei->rsComm;

	/* 
		This function can perform three different checks, depending on flags set via input parameters:
		1. check that its ACL contains the same set of user-authorization pairs as others in its collection
		2. check that its ACL contains at least a given set of user-authorization pairs
		3. check that its ACL does not contain a given set of user-authorization pairs 

		We have four input parameters: Collection Name, User Name, Authorization Type & NOT flag
		For the above conditions, the following are examples of how to call the rule
		1. collname=/sdscZone/home/rods%*User=rods%*Auth=own  
		2. collname=/sdscZone/home/rods  
		3. collname=/sdscZone/home/rods%*User=rods%*Auth=own*Notflag=1  
	*/

	collname = strdup ((char*)collinp->inOutStruct);
	if (collname == NULL) return (USER__NULL_INPUT_ERR);
	/* These next three don't necessarily have to be set */
	username = strdup ((char*)userinp->inOutStruct);
	accessauth = strdup ((char*)authinp->inOutStruct);
	notflag = strdup ((char*)notflaginp->inOutStruct);
	
	if ((username==NULL) && (accessauth==NULL)) {
		querytype=1; /* Case #1. see above */
		free( notflag ); // JMC cppcheck - leak
		notflag=NULL; /* we don't care about this variable in this case */
	} 
	/* just an exclusive OR - both of these variables, username & accessauth HAVE to be set */
	else if (((username==NULL) && (accessauth!=NULL)) || ((username!=NULL) && (accessauth==NULL))) {
            free( collname ); // JMC cppcheck - leak 
            free( username ); // JMC cppcheck - leak 
            free( accessauth ); // JMC cppcheck - leak 
            free( notflag ); // JMC cppcheck - leak 
			return (USER__NULL_INPUT_ERR);
	} else {
		if (atoi(notflag)==0)
			querytype=2;
		else
			querytype=3;
	}
		
	
	/* init gqout, gqin & mybuf structs */
	gqout = (genQueryOut_t*) malloc (sizeof (genQueryOut_t));
	memset (gqout, 0, sizeof (genQueryOut_t));
	mybuf = (bytesBuf_t*) malloc(sizeof(bytesBuf_t));
	memset (mybuf, 0, sizeof (bytesBuf_t));
	gqin.maxRows = MAX_SQL_ROWS;


	/* First get the collection id from the collection name, then
		use the collection id field as a key to obtaining all the
		other nifty info we need */ 
	snprintf (tmpstr, MAX_NAME_LEN, " = '%s'", collname);
	addInxVal (&gqin.sqlCondInp, COL_COLL_NAME, tmpstr);
	addInxIval (&gqin.selectInp, COL_D_COLL_ID, 1);

	j = rsGenQuery (rsComm, &gqin, &gqout);
	
	if (j<0) {
		appendToByteBuf (mybuf, "Coll ID Not found\n");
	}
	else if (j != CAT_NO_ROWS_FOUND) {

		printGenQueryOut(stderr, NULL, NULL, gqout);

		/* RAJA don't we really just need the first element, since all the collection id's should be the same? */
		collCollId = getSqlResultByInx (gqout, COL_D_COLL_ID);
		collid = strdup (&collCollId->value[0]);
		sprintf (tmpstr, "Collection ID:%s\n", &collCollId->value[0]);
		appendToByteBuf (mybuf, tmpstr);

	} else {
		appendToByteBuf (mybuf, "RAJA - what unknown error\n");
        free( collname ); // JMC cppcheck - leak 
        free( username ); // JMC cppcheck - leak 
        free( accessauth ); // JMC cppcheck - leak 
		return (1);  /* what return value RAJA */
	}

	/* Now do another query to get all the interesting ACL information */
	memset (&gqin, 0, sizeof (genQueryInp_t));
	gqin.maxRows = MAX_SQL_ROWS;
	
	addInxIval (&gqin.selectInp, COL_DATA_NAME, 1);
	addInxIval (&gqin.selectInp, COL_DATA_ACCESS_NAME, 1);
	addInxIval (&gqin.selectInp, COL_DATA_ACCESS_TYPE, 1);
	addInxIval (&gqin.selectInp, COL_DATA_ACCESS_DATA_ID, 1);
	addInxIval (&gqin.selectInp, COL_DATA_ACCESS_USER_ID, 1);
	//addInxIval (&gqin.selectInp, COL_DATA_TOKEN_NAMESPACE, 1);

	/* Currently necessary since other namespaces exist in the token table */
	//snprintf (tmpstr, MAX_NAME_LEN, "='%s'", "access_type");
	//addInxVal (&gqin.sqlCondInp, COL_COLL_TOKEN_NAMESPACE, tmpStr);

	snprintf (tmpstr, MAX_NAME_LEN, " = '%s'", collid);
	addInxVal (&gqin.sqlCondInp, COL_D_COLL_ID, tmpstr);

	j = rsGenQuery (rsComm, &gqin, &gqout);

	if (j<0) {
		 appendToByteBuf (mybuf, "Second gen query bad\n");
	} else if  (j != CAT_NO_ROWS_FOUND) {

		dataName = getSqlResultByInx (gqout, COL_DATA_NAME);
		dataAccessType = getSqlResultByInx (gqout, COL_DATA_ACCESS_TYPE);
		dataAccessName = getSqlResultByInx (gqout, COL_DATA_ACCESS_NAME);
		dataAccessDataId = getSqlResultByInx (gqout, COL_DATA_ACCESS_DATA_ID);
		dataAccessUserId = getSqlResultByInx (gqout, COL_DATA_ACCESS_USER_ID);
		//dataTokenNamespace = getSqlResultByInx (gqout, COL_DATA_TOKEN_NAMESPACE);

		for (i=0; i<gqout->rowCnt; i++) {
			sprintf (tmpstr, "Data name:%s\tData Access Type:%s\tData Access Name:%s\tData Access Data Id:%s\t Data Access User ID:%s\tNamespace:%s\n",
				&dataName->value[dataName->len *i], 
				&dataAccessType->value[dataAccessType->len *i], 
				&dataAccessName->value[dataAccessName->len *i],
				&dataAccessDataId->value[dataAccessDataId->len *i],
				&dataAccessUserId->value[dataAccessUserId->len *i],
		//		&dataTokenNamespace->value[dataTokenNamespace->len *i]);
				"some namespace");
			appendToByteBuf (mybuf, tmpstr);
		}	

		printGenQueryOut(stderr, NULL, NULL, gqout);
	} else appendToByteBuf (mybuf, "something else gone bad RAJA");
	

	fillBufLenInMsParam (bufout, mybuf->len, mybuf);
	fillIntInMsParam (statout, rei->status);
 
    free( collname ); // JMC cppcheck - leak 
    free( collid ); // JMC cppcheck - leak 
	return(rei->status);

	return(0);
	
}

/**
 * \fn msiVerifyExpiry (msParam_t* collinp, msParam_t* timeinp, msParam_t* typeinp, msParam_t* bufout, msParam_t* statout, ruleExecInfo_t* rei)
 *
 * \brief This microservice checks whether files in a collection have expired or not expired.
 *
 * \deprecated Since 3.0, the integrityChecks module microservices have been reproduced
 *    using rules.  These microservices only handled 256 files per collection.
 *    The example rules handle an arbitrary number of files.
 *
 * \module integrityChecks
 *
 * \since pre-2.1
 *
 * \author  Susan Lindsey
 * \date    September 2008
 *
 * \usage See clients/icommands/test/rules3.0/ 
 *
 * \param[in] collinp - a STR_MS_T containing the collection's name
 * \param[in] timeinp - a STR_MS_T containing a date
 * \param[in] typeinp - a STR_MS_T containing one of {EXPIRED or NOTEXPIRED}
 * \param[out] bufout - a STR_MS_T containing the output string
 * \param[out] statout - the returned status
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
 * \retval rei->status
 * \pre none
 * \post none
 * \sa none
**/
int
msiVerifyExpiry (msParam_t* collinp, msParam_t* timeinp, msParam_t* typeinp, msParam_t* bufout, msParam_t* statout, ruleExecInfo_t* rei)
{

	rsComm_t *rsComm;
	genQueryInp_t gqin;
	genQueryOut_t *gqout = NULL;
	char condStr[MAX_NAME_LEN];
	char tmpstr[MAX_NAME_LEN];
	char* collname;
	sqlResult_t *dataName;
	sqlResult_t *dataExpiry;
	bytesBuf_t*	mybuf=NULL;
	int i,j,status;

	char* inputtime;
	char* querytype;
	int	checkExpiredFlag=0;
	char inputtimestr[TIME_LEN];

	RE_TEST_MACRO ("    Calling msiVerifyExpiry")

	/* Sanity check */
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiListFields: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	rsComm = rei->rsComm;

	/* init stuff */
	memset (&gqin, 0, sizeof(genQueryInp_t));
	gqin.maxRows = MAX_SQL_ROWS;
    mybuf = (bytesBuf_t *) malloc (sizeof (bytesBuf_t));
    memset (mybuf, 0, sizeof (bytesBuf_t));
	gqout = (genQueryOut_t*) malloc (sizeof (genQueryOut_t));
	memset (gqout, 0, sizeof (genQueryOut_t));


	/* construct an SQL query from the parameter list */
	collname = strdup ((char*)collinp->inOutStruct);
	inputtime = strdup ((char*)timeinp->inOutStruct);
	querytype = strdup ((char*)typeinp->inOutStruct);

	/* 
		We have a couple of rule query possibilities:
		1. If 'inputtime' is valid & querytype == EXPIRED then list files which have an expiration date equal or past the input time
		2. If 'inputtime' is valid & querytype == NOTEXPIRED then list files which have yet to expire
	*/

	/* gotta at least have a collection name input */
	if (collname==NULL) { 
		free( gqout ); // JMC cppcheck - leak
		free( mybuf ); // JMC cppcheck - leak
		free( inputtime ); // JMC cppcheck - leak
		free( querytype ); // JMC cppcheck - leak
		return (USER_PARAM_TYPE_ERR); 
	} 
	
	/* convert inputtime to unixtime */
	/* SUSAN we should make an option that inputtime = "now" */
	rstrcpy (inputtimestr, inputtime, TIME_LEN);
	status = checkDateFormat (inputtimestr);
	if (status < 0) { 
		free( gqout ); // JMC cppcheck - leak
		free( mybuf ); // JMC cppcheck - leak
		free( inputtime ); // JMC cppcheck - leak
		free( collname ); // JMC cppcheck - leak
		free( querytype ); // JMC cppcheck - leak
		return (DATE_FORMAT_ERR); 
	} 

	/* now figure out what kind of query to perform */
	if (!strcmp(querytype, "EXPIRED")) {
		checkExpiredFlag = 1;
	} else if (!strcmp(querytype, "NOTEXPIRED")) {
		checkExpiredFlag = 0;
	} else { 
		free( gqout ); // JMC cppcheck - leak
		free( mybuf ); // JMC cppcheck - leak
		free( inputtime ); // JMC cppcheck - leak
		free( collname ); // JMC cppcheck - leak
		free( querytype ); // JMC cppcheck - leak
		return (USER_PARAM_TYPE_ERR); 
	} 
	

	/* this is the info we want returned from the query */
	addInxIval (&gqin.selectInp, COL_DATA_NAME, 1);
	addInxIval (&gqin.selectInp, COL_D_EXPIRY , 1);
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", collname);
	addInxVal (&gqin.sqlCondInp, COL_COLL_NAME, condStr);

	j = rsGenQuery (rsComm, &gqin, &gqout);

	if (j<0) {

		appendToByteBuf (mybuf, "General Query was bad");

	} else if (j != CAT_NO_ROWS_FOUND) {

		int dataobjexpiry, inputtimeexpiry;

		dataName = getSqlResultByInx (gqout, COL_DATA_NAME);
		dataExpiry = getSqlResultByInx (gqout, COL_D_EXPIRY);
		inputtimeexpiry = atoi (inputtimestr);

		for (i=0; i<gqout->rowCnt; i++) {
			dataobjexpiry = atoi(&dataExpiry->value[dataExpiry->len*i]);

			/* check for expired files */
			if (checkExpiredFlag) { 
				if (dataobjexpiry < inputtimeexpiry)   {
					sprintf (tmpstr, "Data object:%s\twith Expiration date:%s has expired\n", 
						&dataName->value[dataName->len *i], &dataExpiry->value[dataExpiry->len *i]);
					appendToByteBuf (mybuf, tmpstr);
				}
			} else { 
				if (dataobjexpiry >= inputtimeexpiry) {
					sprintf (tmpstr, "Data object:%s\twith Expiration date:%s is expiring\n", 
						&dataName->value[dataName->len *i], &dataExpiry->value[dataExpiry->len *i]);
					appendToByteBuf (mybuf, tmpstr);
				}
			} 
				
		}
	} else appendToByteBuf (mybuf, "No rows found\n");

	fillBufLenInMsParam (bufout, mybuf->len, mybuf);
	fillIntInMsParam (statout, rei->status);
 
    free( collname ); // JMC cppcheck - leak 
    free( querytime ); // JMC cppcheck - leak 
    free( querytype ); // JMC cppcheck - leak 
	return(rei->status);

}


/**
 * \fn msiVerifyAVU (msParam_t* collinp, msParam_t* avunameinp, msParam_t* avuvalueinp, msParam_t* avuattrsinp, 
 *    msParam_t* bufout, msParam_t* statout, ruleExecInfo_t* rei)
 *
 * \brief This microservice performs operations on the AVU metadata on files in a given collection.
 *
 * \deprecated Since 3.0, the integrityChecks module microservices have been reproduced
 *    using rules.  These microservices only handled 256 files per collection.
 *    The example rules handle an arbitrary number of files.
 *
 * \module integrityChecks
 *
 * \since pre-2.1
 *
 * \author  Susan Lindsey
 * \date    August 2008
 *
 * \note See if all files in a collection match a given AVU.
 *
 * \usage See clients/icommands/test/rules3.0/ 
 *
 * \param[in] collinp - a STR_MS_T containing the collection's name
 * \param[in] avunameinp - a STR_MS_T containing the AVU name to check
 * \param[in] avuvalueinp - a STR_MS_T containing the AVU value to check
 * \param[in] avuattrsinp - a STR_MS_T containing the AVU attrs to check
 * \param[out] bufout - a STR_MS_T containing the output string
 * \param[out] statout - the returned status
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
 * \retval rei->status
 * \pre none
 * \post none
 * \sa none
**/
int
msiVerifyAVU (msParam_t* collinp, msParam_t* avunameinp, msParam_t* avuvalueinp, msParam_t* avuattrsinp, 
  msParam_t* bufout, msParam_t* statout, ruleExecInfo_t* rei)
{

	genQueryInp_t gqin;
	genQueryOut_t *gqout1 = NULL;
	genQueryOut_t *gqout2 = NULL;
	char condStr[MAX_NAME_LEN];
	char tmpstr[MAX_NAME_LEN];
	rsComm_t *rsComm;
	int i,j;
	sqlResult_t *dataName;
	sqlResult_t *dataID1; /* points to the data ID column in our first query */
	sqlResult_t *dataID2; /* points to the data ID column in our second query */
	sqlResult_t *dataAttrName;
	sqlResult_t *dataAttrValue;
	sqlResult_t *dataAttrUnits;
	bytesBuf_t*	mybuf=NULL;
	char* collname;
	char* inputattrname;
	char* inputattrvalue;
	char* inputattrunits;
	
	RE_TEST_MACRO ("    Calling msiVerifyAVU")

	/* Sanity check */
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiVerifyAVU: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/*
		Query 1 - just return the files in the collection
		Query 2 - return the files and their AVU triplet
	
		Note that Query 2 might be a subset of Query 1 since Query 2 will not return files that have no AVU listed.
		
		Foreach file in Query1 - see if the same file exists in Query2
			if so, see if the AVU triplet matches the input parameter
				if the AVU triplet doesn't match
					append error message to buf
			if not, append error message to buf
	*/

	rsComm = rei->rsComm;

	/* init structs */
	memset (&gqin, 0, sizeof (genQueryInp_t));
	gqin.maxRows = MAX_SQL_ROWS;
    mybuf = (bytesBuf_t*) malloc(sizeof(bytesBuf_t));
    memset (mybuf, 0, sizeof (bytesBuf_t));
	gqout1 = (genQueryOut_t*) malloc (sizeof (genQueryOut_t));
	memset (gqout1, 0, sizeof (genQueryOut_t));
	gqout2 = (genQueryOut_t*) malloc (sizeof (genQueryOut_t));
	memset (gqout2, 0, sizeof (genQueryOut_t));
 

	/* construct an SQL query from the parameter list */
	collname = strdup ((char*)collinp->inOutStruct);

	/* Return just the name and id of all the files in the collection for our first query */
	addInxIval (&gqin.selectInp, COL_DATA_NAME, 1);
	addInxIval (&gqin.selectInp, COL_D_DATA_ID, 1);
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", collname);
	addInxVal (&gqin.sqlCondInp, COL_COLL_NAME, condStr);

	j = rsGenQuery (rsComm, &gqin, &gqout1);

	if (j<0) {
		free( gqout2 ); // JMC cppcheck - leak
		free( mybuf ); // JMC cppcheck - leak
		free( collname ); // JMC cppcheck - leak
		return (-1); /*RAJA badness what's the correct return error message */
	}
	if (j != CAT_NO_ROWS_FOUND) {

		printGenQueryOut(stderr, NULL, NULL, gqout1);

		/* Now construct our second query, get the AVU information */
		memset (&gqin, 0, sizeof (genQueryInp_t));
		gqin.maxRows = MAX_SQL_ROWS;

		addInxIval (&gqin.selectInp, COL_DATA_NAME, 1);
		addInxIval (&gqin.selectInp, COL_D_DATA_ID, 1);
		addInxIval (&gqin.selectInp, COL_META_DATA_ATTR_NAME, 1);
		addInxIval (&gqin.selectInp, COL_META_DATA_ATTR_VALUE, 1);
		addInxIval (&gqin.selectInp, COL_META_DATA_ATTR_UNITS, 1);
		snprintf (condStr, MAX_NAME_LEN, " = '%s'", collname);
		addInxVal (&gqin.sqlCondInp, COL_COLL_NAME, condStr);

		j = rsGenQuery (rsComm, &gqin, &gqout2);

		if (j != CAT_NO_ROWS_FOUND) { /* Second query results */

			int q1rows = gqout1->rowCnt;
			int q2rows = gqout2->rowCnt;
			int q1thisdataid, q2thisdataid;
			char* q1thisdataname = 0;
			char* thisdataattrname = 0;
			char* thisdataattrvalue = 0;
			char* thisdataattrunits = 0 

			/* OK now we have the results of two queries - time to compare
				data objects and their AVU's */
			dataName = getSqlResultByInx (gqout1, COL_DATA_NAME);
			dataID1 = getSqlResultByInx (gqout1, COL_D_DATA_ID);  /* we'll use this field for indexing into the second query */
			dataID2 = getSqlResultByInx (gqout2, COL_D_DATA_ID);  
			dataAttrName = getSqlResultByInx (gqout2, COL_META_DATA_ATTR_NAME);  
			dataAttrValue = getSqlResultByInx (gqout2, COL_META_DATA_ATTR_VALUE);  
			dataAttrUnits = getSqlResultByInx (gqout2, COL_META_DATA_ATTR_UNITS);  

			/* Assigning the inputted AVU values */
			inputattrname = strdup ((char*)avunameinp->inOutStruct);
			inputattrvalue = strdup ((char*)avuvalueinp->inOutStruct);
			inputattrunits = strdup ((char*)avuattrsinp->inOutStruct);

			/* Ok now for each data object id returned in Query 1, see if there's a matching ID returned in Query 2 */
			for (i=0; i<q1rows; i++) {
				int avufoundflag=0;
				if( q1thisdataname ) 
					free( q1thisdataname ); // JMC cppcheck - leak
				q1thisdataname = strdup((&dataName->value[dataName->len*i]));
				q1thisdataid = atoi(&dataID1->value[dataID1->len*i]);
				for (j=0; j<q2rows; j++) {
					q2thisdataid = atoi(&dataID2->value[dataID2->len*j]);
					if (q1thisdataid == q2thisdataid) { /* means this data object has an AVU triplet set*/
						avufoundflag=1;
						/* now see if the AVU matches our input */
						thisdataattrname = strdup((&dataAttrName->value[dataAttrName->len*j]));
						thisdataattrvalue = strdup((&dataAttrValue->value[dataAttrValue->len*j]));
						thisdataattrunits = strdup((&dataAttrUnits->value[dataAttrUnits->len*j]));
						if (strcmp(thisdataattrname, inputattrname)) { /* no match */
							sprintf (tmpstr, "Data object:%s with AVU:%s,%s,%s does not match input\n",
								q1thisdataname, thisdataattrname, thisdataattrvalue, thisdataattrunits);	
							appendToByteBuf (mybuf, tmpstr);
						}
						if (strcmp(thisdataattrvalue, inputattrvalue)) { /* no match */
							sprintf (tmpstr, "Data object:%s with AVU:%s,%s,%s does not match input\n",
								q1thisdataname, thisdataattrname, thisdataattrvalue, thisdataattrunits);	
							appendToByteBuf (mybuf, tmpstr);
						}
						if (strcmp(thisdataattrunits, inputattrunits)) { /* no match */
							sprintf (tmpstr, "Data object:%s with AVU:%s,%s,%s does not match input\n",
								q1thisdataname, thisdataattrname, thisdataattrvalue, thisdataattrunits);	
							appendToByteBuf (mybuf, tmpstr);
						}
						break;
					} 
					
				}
				if (!avufoundflag) { /* this data object has no AVU associated with it */
					sprintf (tmpstr, "Data object:%s has no AVU triplet set\n", q1thisdataname);	
					appendToByteBuf (mybuf, tmpstr);
				}
				
			}
		
			free( q1thisdataname ); // JMC cppcheck - leak	
			free( thisdataattrname ); // JMC cppcheck - leak	
			free( thisdataattrvalue ); // JMC cppcheck - leak	
			free( thisdataattrunits ); // JMC cppcheck - leak	

		} else {
			/* Query 2 returned no results */
		}
	} else {
		/* Query 1 returned no results */
	}

	fillBufLenInMsParam (bufout, mybuf->len, mybuf);
	fillIntInMsParam (statout, rei->status);
 
    free( collname ); // JMC cppcheck - leak 
    free( inputattrname ); // JMC cppcheck - leak 
    free( inputattrvalue ); // JMC cppcheck - leak 
    free( inputattrunits ); // JMC cppcheck - leak 
	return(rei->status);

}



/**
 * \fn msiVerifyDataType (msParam_t* collinp, msParam_t* datatypeinp, msParam_t* bufout, msParam_t* statout, ruleExecInfo_t* rei)
 *
 * \brief This microservice checks if files in a given collection are of a given data type(s).
 *
 * \deprecated Since 3.0, the integrityChecks module microservices have been reproduced
 *    using rules.  These microservices only handled 256 files per collection.
 *    The example rules handle an arbitrary number of files.
 *
 * \module integrityChecks
 *
 * \since pre-2.1
 *
 * \author  Susan Lindsey
 * \date    August 2008
 *
 * \usage See clients/icommands/test/rules3.0/ 
 *
 * \param[in] collinp - a STR_MS_T containing the collection's name
 * \param[in] datatypeinp - a STR_MS_T containing the comma delimited datatype list
 * \param[out] bufout - a STR_MS_T containing the output string
 * \param[out] statout - the returned status
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
 * \retval rei->status
 * \pre none
 * \post none
 * \sa none
**/
int
msiVerifyDataType (msParam_t* collinp, msParam_t* datatypeinp, msParam_t* bufout, msParam_t* statout, ruleExecInfo_t* rei)
{

	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut = NULL;
	char condStr[MAX_NAME_LEN];
	rsComm_t *rsComm;
	char collname[MAX_NAME_LEN];
	char datatypeparam[MAX_NAME_LEN];
	int i,j;
	sqlResult_t *dataName;
	sqlResult_t *dataType;
	char delims[]=",";
	char* word;
	keyValPair_t	*results;	
	char* key;
	char* value;
	
	RE_TEST_MACRO ("    Calling msiVerifyDataType")

	/* Sanity check */
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiVerifyDataType: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	rsComm = rei->rsComm;

	/* construct an SQL query from the parameter list */
	strcpy (collname,  (char*) collinp->inOutStruct);
	strcpy (datatypeparam, (char*) datatypeinp->inOutStruct);

	fprintf (stderr, "datatypeparam: %s\n", datatypeparam);

	// initialize results to 0; AddKeyVal does all our malloc-ing
	results = (keyValPair_t*) malloc (sizeof(keyValPair_t));
	memset (results, 0, sizeof(keyValPair_t));

	/* Parse the comma-delimited datatype list & make a separate query for each datatype*/
	for (word=strtok(datatypeparam, delims); word; word=strtok(NULL, delims)) {

		fprintf (stderr, "word: %s\n", word);

		memset (&genQueryInp, 0, sizeof(genQueryInp_t));
		genQueryInp.maxRows = MAX_SQL_ROWS;

		/* this is the info we want returned from the query */
		addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
		addInxIval (&genQueryInp.selectInp, COL_DATA_TYPE_NAME, 1);
		snprintf (condStr, MAX_NAME_LEN, " = '%s'", word);
		addInxVal (&genQueryInp.sqlCondInp, COL_DATA_TYPE_NAME, condStr); 
	

		j = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

		if (j != CAT_NO_ROWS_FOUND) {

			fprintf (stderr, "word: %s\trows:%d\n", word, genQueryOut->rowCnt);

			/* we got results - do something cool */
			dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME);
			dataType = getSqlResultByInx (genQueryOut, COL_DATA_TYPE_NAME);

			for (i=0; i<genQueryOut->rowCnt; i++) {
				key = strdup (&dataName->value[dataName->len *i]);
				value = strdup (&dataType->value[dataType->len * i]);
				addKeyVal (results, key, value);
			}	

			printGenQueryOut(stderr, NULL, NULL, genQueryOut);

		} else continue; 

	}

	fillMsParam (bufout, NULL, KeyValPair_MS_T, results, NULL);
	fillIntInMsParam (statout, rei->status);
  
	return(rei->status);

}

/**
 * \fn msiVerifyFileSizeRange (msParam_t* collinp, msParam_t* minsizeinp, msParam_t* maxsizeinp, 
 *  msParam_t* bufout, msParam_t* statout, ruleExecInfo_t *rei)
 *
 * \brief This microservice checks to see if file sizes are NOT within a certain range.
 *
 * \deprecated Since 3.0, the integrityChecks module microservices have been reproduced
 *    using rules.  These microservices only handled 256 files per collection.
 *    The example rules handle an arbitrary number of files.
 *
 * \module integrityChecks
 *
 * \since pre-2.1
 *
 * \author  Susan Lindsey
 * \date    August 2008
 *
 * \usage See clients/icommands/test/rules3.0/ 
 *
 * \param[in] collinp - a STR_MS_T containing the collection's name
 * \param[in] minsizeinp - a STR_MS_T containing the lower limit on filesize
 * \param[in] maxsizeinp - a STR_MS_T containing the upper limit on filesize
 * \param[out] bufout - a STR_MS_T containing the output string
 * \param[out] statout - the returned status
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
 * \retval rei->status
 * \pre none
 * \post none
 * \sa none
**/
int
msiVerifyFileSizeRange (msParam_t* collinp, msParam_t* minsizeinp, msParam_t* maxsizeinp, 
  msParam_t* bufout, msParam_t* statout, ruleExecInfo_t *rei)
{

	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut = NULL;
	char condStr[MAX_NAME_LEN];
	rsComm_t *rsComm;
	char collname[MAX_NAME_LEN];
	char maxfilesize[MAX_NAME_LEN]; 
	char minfilesize[MAX_NAME_LEN];
	int i,j;
	sqlResult_t *dataName;
	sqlResult_t *dataSize;
	keyValPair_t	*results;	
	char* key;
	char* value;
	
	RE_TEST_MACRO ("    Calling msiVerifyFileSizeRange")

	/* Sanity check */
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiVerifyFileSizeRange: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	rsComm = rei->rsComm;

	/* construct an SQL query from the input parameter list */
	strcpy (collname,  (char*) collinp->inOutStruct);
	strcpy (minfilesize, (char*) minsizeinp->inOutStruct);
	strcpy (maxfilesize, (char*) maxsizeinp->inOutStruct);

	/* But first, make sure our size range is valid */
	if (atoi(minfilesize) >= atoi(maxfilesize)) {
		return (USER_PARAM_TYPE_ERR);
	}

	// initialize results to 0; AddKeyVal does all our malloc-ing
	results = (keyValPair_t*) malloc (sizeof(keyValPair_t));
	memset (results, 0, sizeof(keyValPair_t));

	memset (&genQueryInp, 0, sizeof(genQueryInp_t));
	genQueryInp.maxRows = MAX_SQL_ROWS;

	/* this is the info we want returned from the query */
	addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, 1);
	addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);

	/* build the condition:
		collection name AND (filesize < minfilesize or filesize > maxfilesize) */

	snprintf (condStr, MAX_NAME_LEN, " < '%s' || > '%s'", minfilesize, maxfilesize);
	addInxVal (&genQueryInp.sqlCondInp, COL_DATA_SIZE, condStr); 
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", collname);
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr); 

	j = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	if (j != CAT_NO_ROWS_FOUND) {

		/* we got results - do something cool */
		dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME);
		dataSize = getSqlResultByInx (genQueryOut, COL_DATA_SIZE);
		for (i=0; i<genQueryOut->rowCnt; i++) {
			// fprintf (stderr, "dataName[%d]:%s\n",i,&dataName->value[dataName->len * i]);
			key = strdup (&dataName->value[dataName->len *i]);
			// fprintf (stderr, "dataSize[%d]:%s\n",i,&dataSize->value[dataSize->len * i]);
			value = strdup (&dataSize->value[dataSize->len * i]);
			addKeyVal (results, key, value);
		}

	} else {
		fillIntInMsParam (statout, rei->status);
		return (rei->status);  
	}

	//printGenQueryOut(stderr, NULL, NULL, genQueryOut);

	fillMsParam (bufout, NULL, KeyValPair_MS_T, results, NULL);
	fillIntInMsParam (statout, rei->status);
  
	return(rei->status);

}

