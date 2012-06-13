/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include <stdarg.h>
#include "eraUtil.h"






/*
 * appendStrToBBuf() - Appends a string to a bytesBuf_t buffer.
 * Returns number of bytes written or a negative value upon failure.
 *
 */
int
appendStrToBBuf(bytesBuf_t *dest, char *str)
{
	int written;
	
	if (str==NULL) {
		return (-1);
	}

	/* call appendFormattedStrToBBuf() */
	written = appendFormattedStrToBBuf(dest, strlen(str)+1, "%s", str);

	return (written);
}



/*
 * appendFormattedStrToBBuf() - Appends a formatted string to a bytesBuf_t buffer.
 * No more than size characters will be appended.
 * Allocates memory (or more memory) if the buffer is NULL or not large enough.
 * The buffer is treated as a string buffer.
 * Returns number of bytes written or a negative value upon failure.
 *
 */
int
appendFormattedStrToBBuf(bytesBuf_t *dest, size_t size, const char *format, ...)
{
	va_list ap;
	int written;
	size_t index=0;
	char *tmpPtr;

	/* Initial memory check */
	if (dest->buf==NULL) {
		dest->len=size+1;
		dest->buf=(char *)malloc(dest->len);
		memset(dest->buf, '\0', dest->len);
	}

	/* How much has already been written? */
	index = strlen((char *)dest->buf);
	
	/* Increase buffer size if needed */
	if (index+size >= (size_t)dest->len) {
		dest->len=2*(index+size);
		tmpPtr=(char *)malloc(dest->len);
		memset(tmpPtr, '\0', dest->len);
		strncpy(tmpPtr, (char*)dest->buf, dest->len);
		free(dest->buf);
		dest->buf=tmpPtr;
	}

	/* Append new string to previously written characters */
	va_start(ap, format);
	written=vsnprintf(((char *)dest->buf)+index, size, format, ap);
	va_end(ap);

	return (written);
}



/*
 * Copies metadata AVUs from one iRODS object to another one.
 * Both the source and destination object can be either a file
 * or a collection, independently of each other.
 *
 */
int
copyAVUMetadata(char *destPath, char *srcPath, rsComm_t *rsComm)
{
	char destObjType[NAME_LEN];
	char srcObjType[NAME_LEN];
	modAVUMetadataInp_t modAVUMetadataInp;
	int status;


	/* Get type of source object */
	status = getObjType(rsComm, srcPath, srcObjType);
	
	if (status == INVALID_OBJECT_TYPE) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, status,
			    "copyAVUMetadata: Invalid object type for source object: %s.", srcPath);
		return(status);
	}
	
	
	/* Get type of destination object */
	status = getObjType(rsComm, destPath, destObjType);
	
	if (status == INVALID_OBJECT_TYPE) {
		rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, status,
			    "copyAVUMetadata: Invalid object type for destination object: %s.", destPath);
		return(status);
	}

	
	/* Fill in modAVUMetadataInp */
	modAVUMetadataInp.arg0 = "cp";
	modAVUMetadataInp.arg1 = srcObjType;
	modAVUMetadataInp.arg2 = destObjType;
	modAVUMetadataInp.arg3 = srcPath;
	modAVUMetadataInp.arg4 = destPath;
	modAVUMetadataInp.arg5 = "";
	modAVUMetadataInp.arg6 = "";
	modAVUMetadataInp.arg7 = "";
	modAVUMetadataInp.arg8 = "";
	modAVUMetadataInp.arg9 = "";

	
	/* Call rsModAVUMetadata() */
	status = rsModAVUMetadata(rsComm, &modAVUMetadataInp);
	
	return(status);
	
}



/*
 * Recursively copies a collection into another collection.
 * Data object and collection metadata AVUs are also copied.
 *
 */
int
recursiveCollCopy(collInp_t *destCollInp, collInp_t *srcCollInp, rsComm_t *rsComm)
{
	char *destColl, *srcColl;	/* full path of source and destination collections as entered */

	genQueryInp_t genQueryInp;	/* for rsGenquery() */
	genQueryOut_t *genQueryOut;	/* for rsGenquery() */
	char collQCond[2*MAX_NAME_LEN];	/* for rsGenquery() */

	collInp_t collCreateInp;	/* for rsCollCreate() */
	char destSubColl[MAX_NAME_LEN];	/* for rsCollCreate() */

	dataObjCopyInp_t dataObjCopyInp;		/* for rsDataObjCopy() */
	dataObjInp_t srcDataObjInp, destDataObjInp;	/* for rsDataObjCopy() */
	transferStat_t *transStat = NULL;			/* for rsDataObjCopy() */

	char *subCollName, *fileName;	/* for rsCollCreate() and rsDataObjCopy() */

	int i;				/* row index */
	int status=0;


	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "recursiveCollCopy: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	

	/* check for proper inputs */
	if ((destCollInp == NULL) || (destCollInp->collName == NULL)) {
		rodsLog (LOG_ERROR, "recursiveCollCopy: destination collection input is NULL");
		return (USER__NULL_INPUT_ERR);
	}

	if ((srcCollInp == NULL) || (srcCollInp->collName == NULL)) {
		rodsLog (LOG_ERROR, "recursiveCollCopy: source collection input is NULL");
		return (USER__NULL_INPUT_ERR);
	}


	/* get collections pathnames */
	destColl = destCollInp->collName;
	srcColl = srcCollInp->collName;


	/* Get all collections (recursively) under our source collection */
	/* Prepare query */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	/* condition */
	genAllInCollQCond (srcColl, collQCond);	
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, collQCond);

	/* wanted field */
	addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
	
	/* unlimited number of hits (one per collection) */
	genQueryInp.maxRows = MAX_SQL_ROWS;

	
	/* ICAT query for subcollections */
	status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	
	/* copy all subcollections */
	for (i=0; i < genQueryOut->rowCnt; i++) {

		/* only one attribute here, which is the collection name */
		subCollName = genQueryOut->sqlResult[0].value;
		subCollName += i*genQueryOut->sqlResult[0].len;
		
		/* get full path of new collection to be created */
		snprintf(destSubColl, MAX_NAME_LEN, "%s%s", destColl, subCollName+strlen(srcColl));

		/* Create new collection */
		memset (&collCreateInp, 0, sizeof (collCreateInp));
		rstrcpy (collCreateInp.collName, destSubColl, MAX_NAME_LEN);
		rsCollCreate (rsComm, &collCreateInp);

		/* copy metadata AVUs between the 2 collections */
		copyAVUMetadata(destSubColl, subCollName, rsComm);
	}

	/* not done? */	
	while (status==0 && genQueryOut->continueInx > 0) {
		genQueryInp.continueInx=genQueryOut->continueInx;
		status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

		for (i=0; i < genQueryOut->rowCnt; i++) {
	
			/* only one attribute here, which is the collection name */
			subCollName = genQueryOut->sqlResult[0].value;
			subCollName += i*genQueryOut->sqlResult[0].len;
			
			/* get full path of new collection to be created */
			snprintf(destSubColl, MAX_NAME_LEN, "%s%s", destColl, subCollName+strlen(srcColl));
	
			/* Create new collection */
			memset (&collCreateInp, 0, sizeof (collCreateInp));
			rstrcpy (collCreateInp.collName, destSubColl, MAX_NAME_LEN);
			rsCollCreate (rsComm, &collCreateInp);

			/* copy metadata AVUs between the 2 collections */
			copyAVUMetadata(destSubColl, subCollName, rsComm);
		}


	}


	/* Same thing now for the files (data objects) */

	/* Get all files under our source collection, recursively */
	/* Prepare query */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	/* condition */
	genAllInCollQCond (srcColl, collQCond);	
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, collQCond);

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);

	/* unlimited number of hits (one per file) */
	genQueryInp.maxRows = MAX_SQL_ROWS;

	
	/* ICAT query for data objects */
	status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	if (status == CAT_NO_ROWS_FOUND) {
		return (0);
	}


	/* copy all data objects */
	for (i=0; i < genQueryOut->rowCnt; i++) {

		/* prepare parameters for rsDataObjCopy() */
		memset (&srcDataObjInp, 0, sizeof (dataObjInp_t));
		memset (&destDataObjInp, 0, sizeof (dataObjInp_t));

		/* first attribute here is the parent collection name */
		subCollName = genQueryOut->sqlResult[0].value;
		subCollName += i*genQueryOut->sqlResult[0].len;

		/* second attribute is the filename */
		fileName = genQueryOut->sqlResult[1].value;
		fileName += i*genQueryOut->sqlResult[1].len;

		/* get full path of source file */
		snprintf(srcDataObjInp.objPath, MAX_NAME_LEN, "%s/%s", subCollName, fileName);
		
		/* get full path of new file to be created */
		snprintf(destDataObjInp.objPath, MAX_NAME_LEN, "%s%s/%s", destColl, subCollName+strlen(srcColl), fileName);


		/* copy files with rsDataObjCopy() */
		dataObjCopyInp.srcDataObjInp = srcDataObjInp;
		dataObjCopyInp.destDataObjInp = destDataObjInp;
		rsDataObjCopy (rsComm, &dataObjCopyInp, &transStat);

		/* no need for transStat here, free memory allocated by rsDataObjCopy() */
		if (transStat != NULL) {
			free (transStat);
		}

		/* copy metadata AVUs between the 2 files */
		copyAVUMetadata(destDataObjInp.objPath, srcDataObjInp.objPath, rsComm);

	}

	/* not done? */	
	while (status==0 && genQueryOut->continueInx > 0) {
		genQueryInp.continueInx=genQueryOut->continueInx;
		status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

		for (i=0; i < genQueryOut->rowCnt; i++) {
	
			/* prepare parameters for rsDataObjCopy() */
			memset (&srcDataObjInp, 0, sizeof (dataObjInp_t));
			memset (&destDataObjInp, 0, sizeof (dataObjInp_t));
	
			/* first attribute here is the parent collection name */
			subCollName = genQueryOut->sqlResult[0].value;
			subCollName += i*genQueryOut->sqlResult[0].len;
	
			/* second attribute is the filename */
			fileName = genQueryOut->sqlResult[1].value;
			fileName += i*genQueryOut->sqlResult[1].len;
	
			/* get full path of source file */
			snprintf(srcDataObjInp.objPath, MAX_NAME_LEN, "%s/%s", subCollName, fileName);
			
			/* get full path of new file to be created */
			snprintf(destDataObjInp.objPath, MAX_NAME_LEN, "%s%s/%s", destColl, subCollName+strlen(srcColl), fileName);
	
	
			/* copy files with rsDataObjCopy() */
			dataObjCopyInp.srcDataObjInp = srcDataObjInp;
			dataObjCopyInp.destDataObjInp = destDataObjInp;
			rsDataObjCopy (rsComm, &dataObjCopyInp, &transStat);
	
			/* no need for transStat here, free memory allocated by rsDataObjCopy() */
			if (transStat != NULL) {
				free (transStat);
			}
	
			/* copy metadata AVUs between the 2 files */
			copyAVUMetadata(destDataObjInp.objPath, srcDataObjInp.objPath, rsComm);
		}

	}


	return(status);	
}



/*
 * Gets pipe separated metadata AVUs for a data object.
 * 
 */
int
getDataObjPSmeta(char *objPath, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int i1a[10];
   int i1b[10];
   int i2a[10];
   char *condVal[10];
   char v1[MAX_NAME_LEN];
   char v2[MAX_NAME_LEN];
   char fullName[MAX_NAME_LEN];
   char myDirName[MAX_NAME_LEN];
   char myFileName[MAX_NAME_LEN];
   int printCount=0;
   int status;


   if (rsComm == NULL) {
	rodsLog (LOG_ERROR,
	  "getDataObjPSmeta: input rsComm is NULL");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
   }

   
   memset (&genQueryInp, 0, sizeof (genQueryInp_t));

   i1a[0]=COL_META_DATA_ATTR_NAME;
   i1b[0]=0; /* currently unused */
   i1a[1]=COL_META_DATA_ATTR_VALUE;
   i1b[1]=0; /* currently unused */
   i1a[2]=COL_META_DATA_ATTR_UNITS;
   i1b[2]=0; /* currently unused */
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = 3;

   /* Extract cwd name and object name */
   strncpy(fullName, objPath, MAX_NAME_LEN);
   status = splitPathByKey(fullName, myDirName, myFileName, '/');

   i2a[0]=COL_COLL_NAME;
   sprintf(v1,"='%s'",myDirName);
   condVal[0]=v1;

   i2a[1]=COL_DATA_NAME;
   sprintf(v2,"='%s'",myFileName);
   condVal[1]=v2;


   genQueryInp.sqlCondInp.inx = i2a;
   genQueryInp.sqlCondInp.value = condVal;
   genQueryInp.sqlCondInp.len=2;

   genQueryInp.maxRows=10;
   genQueryInp.continueInx=0;
   genQueryInp.condInput.len=0;


   /* Actual query happens here */
   status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);


   if (status == CAT_NO_ROWS_FOUND) {
      i1a[0]=COL_D_DATA_PATH;
      genQueryInp.selectInp.len = 1;
      status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
      if (status==0) {
    	  appendStrToBBuf(mybuf, "");
    	  /* printf("None\n"); */
    	  return(0);
      }
      if (status == CAT_NO_ROWS_FOUND) {

	rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, status,
          "getDataObjPSmeta: DataObject %s not found. status = %d", fullName, status);
	return (status);
      }
      printCount+=extractPSQueryResults(status, genQueryOut, mybuf, fullName);
   }
   else {
      printCount+=extractPSQueryResults(status, genQueryOut, mybuf, fullName);
   }

   while (status==0 && genQueryOut->continueInx > 0) {
      genQueryInp.continueInx=genQueryOut->continueInx;
      status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
      printCount+= extractPSQueryResults(status, genQueryOut, mybuf, fullName);
   }

  return (status);
}



/*
 * Gets pipe separated metadata AVUs for a collection.
 * 
 */
int
getCollectionPSmeta(char *objPath, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int i1a[10];
   int i1b[10];
   int i2a[10];
   char *condVal[10];
   char v1[MAX_NAME_LEN];
   int printCount=0;
   int status;



   if (rsComm == NULL) {
	rodsLog (LOG_ERROR,
	  "getCollectionPSmeta: input rsComm is NULL");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
   }


   memset (&genQueryInp, 0, sizeof (genQueryInp_t));

   i1a[0]=COL_META_COLL_ATTR_NAME;
   i1b[0]=0; /* currently unused */
   i1a[1]=COL_META_COLL_ATTR_VALUE;
   i1b[1]=0;
   i1a[2]=COL_META_COLL_ATTR_UNITS;
   i1b[2]=0;
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = 3;

   i2a[0]=COL_COLL_NAME;
   sprintf(v1,"='%s'", objPath);
   condVal[0]=v1;

   genQueryInp.sqlCondInp.inx = i2a;
   genQueryInp.sqlCondInp.value = condVal;
   genQueryInp.sqlCondInp.len=1;

   genQueryInp.maxRows=10;
   genQueryInp.continueInx=0;
   genQueryInp.condInput.len=0;


   /* Actual query happens here */
   status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);

   if (status == CAT_NO_ROWS_FOUND) {
      i1a[0]=COL_COLL_COMMENTS;
      genQueryInp.selectInp.len = 1;
      status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
      if (status==0) {
    	  appendStrToBBuf(mybuf, "");
    	  /* printf("None\n"); */
    	  return(0);
      }
      if (status == CAT_NO_ROWS_FOUND) {

	rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, status,
          "getCollectionPSmeta: Collection %s not found. status = %d", objPath, status);
	return (status);
      }
   }

   printCount+=extractPSQueryResults(status, genQueryOut, mybuf, objPath);

   while (status==0 && genQueryOut->continueInx > 0) {
      genQueryInp.continueInx=genQueryOut->continueInx;
      status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
      printCount+= extractPSQueryResults(status, genQueryOut, mybuf, objPath);
   }

  return (status);

}



/*
 * Gets pipe separated ACL tokens for a data object.
 * 
 */
int
getDataObjACL(dataObjInp_t *myDataObjInp, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	rodsObjStat_t *rodsObjStatOut;
	char condStr[MAX_NAME_LEN];

	int printCount=0;
	int status;
	
	
	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "getDataObjACL: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
	
	
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	
	/* could spare that extra query with a bit of parsing... */
	status = rsObjStat(rsComm, myDataObjInp, &rodsObjStatOut);
	
	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_USER_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_ACCESS_NAME, 1);
	
	/* conditions */
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", rodsObjStatOut->dataId);
	addInxVal (&genQueryInp.sqlCondInp, COL_DATA_ACCESS_DATA_ID, condStr);

	/* Currently necessary since other namespaces exist in the token table */
	snprintf (condStr, MAX_NAME_LEN, "='%s'", "access_type");
	addInxVal (&genQueryInp.sqlCondInp, COL_DATA_TOKEN_NAMESPACE, condStr);
	
	genQueryInp.maxRows = MAX_SQL_ROWS;

	status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
	
	
	if (status == CAT_NO_ROWS_FOUND) {
		/* check if file exists */
		addInxIval (&genQueryInp.selectInp, COL_D_DATA_PATH, 1);
		genQueryInp.selectInp.len = 1;
		status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
		
		if (status == CAT_NO_ROWS_FOUND) {
			rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, status,
				"getDataObjACL: DataID %s not found. status = %d", rodsObjStatOut->dataId, status);
			return (status);
		}
		
		printCount+=extractACLQueryResults(genQueryOut, mybuf, 0);
	}
	
	else {
		printCount+=extractACLQueryResults(genQueryOut, mybuf, 0);
	}
	
	while (status==0 && genQueryOut->continueInx > 0) {
		genQueryInp.continueInx=genQueryOut->continueInx;
		status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
		printCount+=extractACLQueryResults(genQueryOut, mybuf, 0);
	}
	
	return (status);
}



/*
 * Gets pipe separated ACL tokens for a collection.
 * 
 */
int
getCollectionACL(collInp_t *myCollInp, char *label, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	char condStr[MAX_NAME_LEN];
	rodsLong_t collId;
	int status;
	
	
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "getCollectionACL: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* Check if collection exists */
	status = isColl (rsComm, myCollInp->collName, &collId);
	if (status == CAT_NO_ROWS_FOUND)
	{
		rodsLog (LOG_ERROR, "getCollectionACL: collection %s not found.", myCollInp->collName);
		return (status);
	}


	/*** Get ACLs on collection ***/

	/* Init genQueryInp */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	genQueryInp.maxRows = MAX_SQL_ROWS;

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_COLL_USER_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_COLL_USER_ZONE, 1);
	addInxIval (&genQueryInp.selectInp, COL_COLL_ACCESS_NAME, 1);

	/* conditions */
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", myCollInp->collName);
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr);

	/* Not sure if this is still required... */
	snprintf (condStr, MAX_NAME_LEN, "='%s'", "access_type");
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_TOKEN_NAMESPACE, condStr);

	
	/* ICAT call to get ACL tokens */
	status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
	
	
	/* Extract results and get more rows */
	writeCollAclToBBuf(genQueryOut, mybuf);
	
	while (status==0 && genQueryOut->continueInx > 0) {
		genQueryInp.continueInx=genQueryOut->continueInx;
		freeGenQueryOut (&genQueryOut);
		status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
		writeCollAclToBBuf(genQueryOut, mybuf);
	}
	
	/* Cleanup */
	freeGenQueryOut (&genQueryOut);

	/* check for query errors */
	if (status < 0)
	{
		return (status);
	}

	/* If not in recursive mode, we're done here. */
	if (!label || strcmp(label, "recursive"))
	{
		return (0);
	}


	/*** Get ACLs on subcollections ***/

	/* Init genQueryInp */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	genQueryInp.maxRows = MAX_SQL_ROWS;

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_COLL_USER_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_COLL_USER_ZONE, 1);
	addInxIval (&genQueryInp.selectInp, COL_COLL_ACCESS_NAME, 1);

	/* conditions */
	snprintf (condStr, MAX_NAME_LEN, " like '%s/%%'", myCollInp->collName);
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr);

	/* Not sure if this is still required... */
	snprintf (condStr, MAX_NAME_LEN, "='%s'", "access_type");
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_TOKEN_NAMESPACE, condStr);


	/* ICAT call to get ACL tokens */
	status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);


	/* Extract results and get more rows */
	writeCollAclToBBuf(genQueryOut, mybuf);

	while (status==0 && genQueryOut->continueInx > 0) {
		genQueryInp.continueInx=genQueryOut->continueInx;
		freeGenQueryOut (&genQueryOut);
		status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
		writeCollAclToBBuf(genQueryOut, mybuf);
	}
	
	/* Cleanup */
	freeGenQueryOut (&genQueryOut);

	/* check for query errors */
	/* CAT_NO_ROWS_FOUND is ok here, means there were no sub-collections */
	if (status < 0 && status != CAT_NO_ROWS_FOUND)
	{
		return (status);
	}


	/*** Now get ACLs on data objects ***/

	/* Init genQueryInp */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	genQueryInp.maxRows = MAX_SQL_ROWS;

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_USER_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_USER_ZONE, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_ACCESS_NAME, 1);

	/* conditions */
	snprintf (condStr, MAX_NAME_LEN, " like '%s/%%'", myCollInp->collName);
	addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, condStr);

	/* Not sure if this is still required... */
	snprintf (condStr, MAX_NAME_LEN, "='%s'", "access_type");
	addInxVal (&genQueryInp.sqlCondInp, COL_DATA_TOKEN_NAMESPACE, condStr);


	/* ICAT call to get ACL tokens */
	status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);


	/* Extract results and get more rows */
	writeDataAclToBBuf(genQueryOut, mybuf);

	while (status==0 && genQueryOut->continueInx > 0) {
		genQueryInp.continueInx=genQueryOut->continueInx;
		freeGenQueryOut (&genQueryOut);
		status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
		writeDataAclToBBuf(genQueryOut, mybuf);
	}

	/* Cleanup */
	freeGenQueryOut (&genQueryOut);
	
	/* check for query errors */
	/* CAT_NO_ROWS_FOUND is ok here, means there were no files */
	if (status < 0 && status != CAT_NO_ROWS_FOUND)
	{
		return (status);
	}

	return (0);
}



/*
 * loadMetadataFromDataObj()
 *
 */
int
loadMetadataFromDataObj(dataObjInp_t *dataObjInp, rsComm_t *rsComm)
{
	openedDataObjInp_t dataObjReadInp;
	openedDataObjInp_t dataObjCloseInp;
	openedDataObjInp_t dataObjLseekInp;
	fileLseekOut_t *dataObjLseekOut;
	bytesBuf_t *readBuf;
	int status;
	int objID;
	char *lineStart, *lineEnd;
	int bytesRead;

	
	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "loadMetadataFromDataObj: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* check for proper input */
	if (dataObjInp == NULL) {
		rodsLog (LOG_ERROR, "loadMetadataFromDataObj: input data object is NULL");
		return (USER__NULL_INPUT_ERR);
	}


	/* open metadata file */
	if ((objID=rsDataObjOpen(rsComm, dataObjInp)) < 0) {
		return (objID);
	}

	/* read buffer init */
	readBuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	readBuf->len = 5*1024*1024;	/* no more than 5MB at a time */
	readBuf->buf = (char *)malloc(readBuf->len);
	memset (readBuf->buf, '\0', readBuf->len);
	
	
	/* read and parse metadata file */
	memset (&dataObjReadInp, 0, sizeof (dataObjReadInp));
	dataObjReadInp.l1descInx = objID;
	dataObjReadInp.len = readBuf->len;/* 200; */

	/* init offset */
	dataObjLseekInp.l1descInx = objID;
	dataObjLseekInp.offset = 0;
	

	
	while ((bytesRead = rsDataObjRead (rsComm, &dataObjReadInp, readBuf)) > 0) {
		
		/* in case buffer is not null terminated */
		appendStrToBBuf(readBuf, "");

		
		lineStart = (char*)readBuf->buf;

		while ( (lineEnd=strstr(lineStart, "\n")) ) {
			lineEnd[0]='\0';
			
			status = parseMetadataModLine(lineStart, rsComm);
			
			lineStart=lineEnd+1;
		}
		
		/* must read again any final line that got cut before '\n' */
		dataObjLseekInp.offset = dataObjLseekInp.offset + bytesRead - strlen(lineStart);
		dataObjLseekInp.whence = SEEK_SET;
		
		
		/* make sure not to get stuck, for example if the file doesn't end with a new line */
		if (strlen(lineStart) >= (size_t)bytesRead) {
			parseMetadataModLine(lineStart, rsComm);
			break;
		}
		
		status = rsDataObjLseek (rsComm, &dataObjLseekInp, &dataObjLseekOut);

		
		memset(readBuf->buf, 0, readBuf->len);

	
	}


	
	/* close metadata file */
	memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
	dataObjCloseInp.l1descInx = objID;
	
	status = rsDataObjClose (rsComm, &dataObjCloseInp);	


	return (status);
}



char *unescape(char *myStr)
{
	char *tmpPtr;
	
	while ( (tmpPtr = strstr(myStr, "\\|")) ) {
		snprintf(tmpPtr, strlen(tmpPtr)+1, "%s", tmpPtr+1);
	}
	
	return (myStr);
}



int
parseMetadataModLine(char *inpLine, rsComm_t *rsComm)
{
	modAVUMetadataInp_t modAVUMetadataInp;
	int authFlag;
	int status;
	char *objPath, *attribute, *value, *units=NULL;
	char *tmpPtr;
	char *collPtr;
	

	/* sanity checks */
	if (!rsComm) {
		rodsLog (LOG_ERROR, "parseMetadataModLine: rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);	
	}
	
	if (!inpLine) {
		rodsLog (LOG_ERROR, "parseMetadataModLine: inpLine is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	
	
	/* is this a comment? */
	if (inpLine[0] == '#') {
		return (0);
	}

	/* get user privilege level */
	/* maybe used to skip test in rsModAVUMetadata() */
	authFlag = rsComm->clientUser.authInfo.authFlag;


	/* init modAVU input */
	memset (&modAVUMetadataInp, 0, sizeof(modAVUMetadataInp_t));
	modAVUMetadataInp.arg0 = "add";

	
	/* get object path */
	objPath = inpLine;
	if ( (tmpPtr = strstr(objPath, " |")) == NULL) {
		rodsLog (LOG_ERROR, "parseMetadataModLine: Parse error: object path is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	tmpPtr[0] = '\0';
	
	/* is it a file or a collection? */
	collPtr = strstr(objPath, "C-");
	if (collPtr && (collPtr == objPath)) {
		modAVUMetadataInp.arg1 = "-c";
		
		/* actual path starts after preceding C- */
		objPath += 2;
	}
	else {
		modAVUMetadataInp.arg1 = "-d";
	}

	
	/* set path field of query input object */
	modAVUMetadataInp.arg2 = unescape(objPath);


	/* get attribute name */
	attribute = tmpPtr+2;	
	if ( (tmpPtr = strstr(attribute, " |")) == NULL) {
		rodsLog (LOG_ERROR, "parseMetadataModLine: Parse error: attribute is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	tmpPtr[0] = '\0';
	
	
	/* set attribute field of query input object */
	modAVUMetadataInp.arg3 = unescape(attribute);
	

	/* get attribute value */
	value = tmpPtr+2;	
	if ( (tmpPtr = strstr(value, " |")) ) {
		tmpPtr[0] = '\0';
		units = tmpPtr+2;
	}

		
	/* set value field of query input object */
	modAVUMetadataInp.arg4 = unescape(value);

	
	/* get attribute units */
	if (units)  {
		/* set path field of query input object */
		modAVUMetadataInp.arg5 = unescape(units);
	}
	else {
		modAVUMetadataInp.arg5 = "";
	}

	
	/* finally... the query */
	status = rsModAVUMetadata (rsComm, &modAVUMetadataInp);

	
	return (status);
}



/*
 * genQueryOutToXML() - Extracts AVU results from genQueryOut_t and writes them
 * to a dynamic buffer in an XML syntax.
 * Array length of argument tags must be equal or greater than genQueryOut->attriCnt + 1.
 * tags[0] is used to separate query hits (row count), then each tag[n+1] will wrap
 * the nth attribute.
 */
int
genQueryOutToXML(genQueryOut_t *genQueryOut, bytesBuf_t *mybuf, char **tags)
{
	int printCount;
	int i, j;
	size_t size;
	
	printCount=0;

	
	for (i=0;i<genQueryOut->rowCnt;i++) {
		
		if ( (tags[0] != NULL) && strlen(tags[0]) ) {
			appendFormattedStrToBBuf(mybuf, strlen(tags[0])+4, "<%s>\n", tags[0]);
		}
		
		for (j=0;j<genQueryOut->attriCnt;j++) {
			char *tResult;
			tResult = genQueryOut->sqlResult[j].value;
			tResult += i*genQueryOut->sqlResult[j].len;
			
			if ( (tags[j+1] != NULL) && strlen(tags[j+1]) ) {
				size = genQueryOut->sqlResult[j].len + 2*strlen(tags[j+1]) + 10;
				appendFormattedStrToBBuf(mybuf, size, "<%s>%s</%s>\n", tags[j+1], tResult, tags[j+1]);
			}
			else {
				size = genQueryOut->sqlResult[j].len + 1;
				appendFormattedStrToBBuf(mybuf, size, "%s\n",tResult);
			}
			printCount++;
		}
		
		if ( (tags[0] != NULL) && strlen(tags[0]) ) {
			appendFormattedStrToBBuf(mybuf, strlen(tags[0])+5, "</%s>\n", tags[0]);
		}
	
	}
	
	
	
	return (printCount);
}



/*
 * extractPSQueryResults() - Extracts AVU results from a genQueryOut_t 
 * and writes them to a dynamic buffer in a pipe-separated, one AVU per line format.
 * To be used in msiGetDataObjPSmeta().
 */
int
extractPSQueryResults(int status, genQueryOut_t *genQueryOut, bytesBuf_t *mybuf, char *fullName)
{
   int printCount;
   int i, j;
   size_t size;


   printCount=0;

   if (status!=0) {
	rodsLog (LOG_ERROR, "extractPSQueryResults error: %d", status);
	return (status);
   }
   else {
      if (status !=CAT_NO_ROWS_FOUND) {
	 for (i=0;i<genQueryOut->rowCnt;i++) {
	 
	    appendFormattedStrToBBuf(mybuf, strlen(fullName)+1, fullName);

	    for (j=0;j<genQueryOut->attriCnt;j++) {
		char *tResult;
		tResult = genQueryOut->sqlResult[j].value;
		tResult += i*genQueryOut->sqlResult[j].len;
		
		/* skip final | if no units were defined */
		if (j<2 || strlen(tResult)) {
			size = genQueryOut->sqlResult[j].len + 2;
			appendFormattedStrToBBuf(mybuf, size, "|%s",tResult);
		}
		
		printCount++;
	    }

	    appendStrToBBuf(mybuf, "\n");

	 }
      }
   }

   return (printCount);
}



/*
 * prints the results of a general query.
 */
int
extractGenQueryResults(genQueryOut_t *genQueryOut, bytesBuf_t *mybuf, char *header, char **descriptions)
{
	int i, j;
	char localTime[20];

	for (i=0;i<genQueryOut->rowCnt;i++) {

		if ((header != NULL) && strlen(header)) {
			appendFormattedStrToBBuf(mybuf, strlen(header)+2, "%s|", header);
		}

		for (j=0;j<genQueryOut->attriCnt;j++) {
			char *tResult;
			tResult = genQueryOut->sqlResult[j].value;
			tResult += i*genQueryOut->sqlResult[j].len;

			if (j) {
				appendStrToBBuf(mybuf, "|");
			}

			/* write dates in human readable format */
			if ((descriptions != NULL) && (descriptions[j] != NULL) && (strstr(descriptions[j],"time")!=0)) {
				getLocalTimeFromRodsTime(tResult, localTime);
				appendStrToBBuf(mybuf, localTime);
			}
			else {
				appendStrToBBuf(mybuf, tResult);
			}

		}

	appendStrToBBuf(mybuf, "\n");
	}

	return (i);
}



/*
 * prints the results of an ACL query.
 */
int
extractACLQueryResults(genQueryOut_t *genQueryOut, bytesBuf_t *mybuf, int coll_flag)
{
	int i, j;

	for (i=0;i<genQueryOut->rowCnt;i++) {

		for (j=0;j<genQueryOut->attriCnt;j++) {
			char *tResult;
			tResult = genQueryOut->sqlResult[j].value;
			tResult += i*genQueryOut->sqlResult[j].len;

			if (j) {
				if (j==1 && !coll_flag) {
					/* attach collection and fileName into one path */
					appendStrToBBuf(mybuf, "/");
				}
				else {
					appendStrToBBuf(mybuf, "|");
				}
			}

			appendStrToBBuf(mybuf, tResult);
		}

		appendStrToBBuf(mybuf, "\n");
	}

	return (i);
}


/*
 * Modified version of printDataAcl() to write the results
 * of a data object ACL query to a bytesBuf_t*.
 */
int
writeDataAclToBBuf(genQueryOut_t *genQueryOut, bytesBuf_t *mybuf)
{
	int i;
    sqlResult_t *collName, *dataName, *userName, *userZone, *dataAccess;
    char *collNameStr, *dataNameStr, *userNameStr, *userZoneStr, *dataAccessStr;
    char ACLStr[MAX_NAME_LEN];

    /* Extract column results from genQueryOut */
    if ((collName = getSqlResultByInx (genQueryOut, COL_COLL_NAME)) == NULL)
    {
        rodsLog (LOG_ERROR,
          "writeDataAclToBBuf: getSqlResultByInx for COL_COLL_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME)) == NULL)
    {
        rodsLog (LOG_ERROR,
          "writeDataAclToBBuf: getSqlResultByInx for COL_DATA_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((userName = getSqlResultByInx (genQueryOut, COL_USER_NAME)) == NULL)
    {
        rodsLog (LOG_ERROR,
          "writeDataAclToBBuf: getSqlResultByInx for COL_USER_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((userZone = getSqlResultByInx (genQueryOut, COL_USER_ZONE)) == NULL)
    {
        rodsLog (LOG_ERROR,
          "writeDataAclToBBuf: getSqlResultByInx for COL_USER_ZONE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataAccess = getSqlResultByInx (genQueryOut, COL_DATA_ACCESS_NAME)) == NULL)
    {
        rodsLog (LOG_ERROR,
          "writeDataAclToBBuf: getSqlResultByInx for COL_DATA_ACCESS_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }


    /* Extract results for each row */
	for (i=0;i<genQueryOut->rowCnt;i++)
	{

		memset(ACLStr,'\0', MAX_NAME_LEN);

		collNameStr = &collName->value[collName->len * i];
		dataNameStr = &dataName->value[dataName->len * i];
		userNameStr = &userName->value[userName->len * i];
		userZoneStr = &userZone->value[userZone->len * i];
		dataAccessStr = &dataAccess->value[dataAccess->len * i];

		snprintf(ACLStr, MAX_NAME_LEN, "%s/%s|%s#%s|%s\n", collNameStr, dataNameStr, userNameStr, userZoneStr, dataAccessStr);
		appendStrToBBuf(mybuf, ACLStr);

	}

	return (i);
}



/*
 * Modified version of printCollAcl() to write the results
 * of a collection ACL query to a bytesBuf_t*.
 * Careful when using return value as status when empty results are not an error.
 */
int
writeCollAclToBBuf(genQueryOut_t *genQueryOut, bytesBuf_t *mybuf)
{
	int i;
    sqlResult_t *collName, *userName, *userZone, *collAccess;
    char *collNameStr, *userNameStr, *userZoneStr, *collAccessStr;
    char ACLStr[MAX_NAME_LEN];

    /* Extract column results from genQueryOut */
    if ((collName = getSqlResultByInx (genQueryOut, COL_COLL_NAME)) == NULL)
    {
        rodsLog (LOG_ERROR,
          "writeCollAclToBBuf: getSqlResultByInx for COL_COLL_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((userName = getSqlResultByInx (genQueryOut, COL_COLL_USER_NAME)) == NULL)
    {
        rodsLog (LOG_ERROR,
          "writeCollAclToBBuf: getSqlResultByInx for COL_COLL_USER_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((userZone = getSqlResultByInx (genQueryOut, COL_COLL_USER_ZONE)) == NULL)
    {
        rodsLog (LOG_ERROR,
          "writeCollAclToBBuf: getSqlResultByInx for COL_COLL_USER_ZONE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((collAccess = getSqlResultByInx (genQueryOut, COL_COLL_ACCESS_NAME)) == NULL)
    {
        rodsLog (LOG_ERROR,
          "writeCollAclToBBuf: getSqlResultByInx for COL_COLL_ACCESS_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }


    /* Extract results for each row */
	for (i=0;i<genQueryOut->rowCnt;i++)
	{

		memset(ACLStr,'\0', MAX_NAME_LEN);

		collNameStr = &collName->value[collName->len * i];
		userNameStr = &userName->value[userName->len * i];
		userZoneStr = &userZone->value[userZone->len * i];
		collAccessStr = &collAccess->value[collAccess->len * i];

		snprintf(ACLStr, MAX_NAME_LEN, "%s|%s#%s|%s\n", collNameStr, userNameStr, userZoneStr, collAccessStr);
		appendStrToBBuf(mybuf, ACLStr);

	}

	return (i);
}


/*
 *
 *
 */
int 
getUserInfo(char *userName, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	char condStr[MAX_NAME_LEN];
	int status;
	char *attrs[] = {"name", "id", "type", "zone", "dn", "info", "comment",
                        "create time", "modify time"};

	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "getUserInfo: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* check for proper input */
	if (userName == NULL) {
		rodsLog (LOG_ERROR, "getUserInfo: username input is NULL");
		return (USER__NULL_INPUT_ERR);
	}


	/* Query for user info */
	/* Prepare query */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	/* wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_USER_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_USER_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_USER_TYPE, 1);
	addInxIval (&genQueryInp.selectInp, COL_USER_ZONE, 1);
//	addInxIval (&genQueryInp.selectInp, COL_USER_DN, 1);   /* No such column in r_user_main anymore */
	addInxIval (&genQueryInp.selectInp, COL_USER_INFO, 1);
	addInxIval (&genQueryInp.selectInp, COL_USER_COMMENT, 1);
	addInxIval (&genQueryInp.selectInp, COL_USER_CREATE_TIME, 1);
	addInxIval (&genQueryInp.selectInp, COL_USER_MODIFY_TIME, 1);

	/* make the condition */
	/* '%' in username is treated as a wildcard */
	if (strstr(userName, "%")) {
		snprintf (condStr, MAX_NAME_LEN, " like '%s'", userName);
	}
	else {
		snprintf (condStr, MAX_NAME_LEN, " = '%s'", userName);
	}

	addInxVal (&genQueryInp.sqlCondInp, COL_USER_NAME, condStr);

	/* might get more than one row if wildcards are used */
	genQueryInp.maxRows = MAX_SQL_ROWS;

	
	/* ICAT query for user info */
	status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	/* print results out to buffer */
	if (status == 0) {
		extractGenQueryResults(genQueryOut, mybuf, NULL, attrs);
	
		/* not done? */
		while (status==0 && genQueryOut->continueInx > 0) {
			genQueryInp.continueInx=genQueryOut->continueInx;
			status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);

			extractGenQueryResults(genQueryOut, mybuf, NULL, attrs);
		}
	}

	return (0);
}



/*
 * gets ACLs by username 
 *
 */
int 
getUserACL(char *userName, bytesBuf_t *mybuf, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	char condStr[MAX_NAME_LEN];
	int status;

		/* check for valid connection */
		if (rsComm == NULL) {
			rodsLog (LOG_ERROR, "getUserACL: input rsComm is NULL");
			return (SYS_INTERNAL_NULL_INPUT_ERR);
		}

		/* check for proper input */
		if (userName == NULL) {
			rodsLog (LOG_ERROR, "getUserACL: username input is NULL");
			return (USER__NULL_INPUT_ERR);
		}


		/* 1st query to get permissions on data objects */
		/* Prepare query */
		memset (&genQueryInp, 0, sizeof (genQueryInp_t));
		
		/* wanted fields */
		addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
		addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
		addInxIval (&genQueryInp.selectInp, COL_USER_NAME, 1);
		addInxIval (&genQueryInp.selectInp, COL_DATA_ACCESS_NAME, 1);
	
		/* make the condition */
		/* '%' in username is treated as a wildcard */
		if (strstr(userName, "%")) {
			snprintf (condStr, MAX_NAME_LEN, " like '%s'", userName);
		}
		else {
			snprintf (condStr, MAX_NAME_LEN, " = '%s'", userName);
		}

		addInxVal (&genQueryInp.sqlCondInp, COL_USER_NAME, condStr);
		
		

		/* Currently necessary since other namespaces exist in the token table */
		snprintf (condStr, MAX_NAME_LEN, "='%s'", "access_type");
		addInxVal (&genQueryInp.sqlCondInp, COL_DATA_TOKEN_NAMESPACE, condStr);
	
		genQueryInp.maxRows = MAX_SQL_ROWS;

	
		/* ICAT query for user info */
		status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

		/* print results out to buffer */
		if (status == 0) {
			extractACLQueryResults(genQueryOut, mybuf, 0);
	
			/* not done? */
			while (status==0 && genQueryOut->continueInx > 0) {
				genQueryInp.continueInx=genQueryOut->continueInx;
				status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);

				extractACLQueryResults(genQueryOut, mybuf, 0);
			}
		}
		
		
		/* 2d query to get permissions on collections */
		/* Prepare query */
		memset (&genQueryInp, 0, sizeof (genQueryInp_t));
		
		/* wanted fields */
		addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
		addInxIval (&genQueryInp.selectInp, COL_USER_NAME, 1);
		addInxIval (&genQueryInp.selectInp, COL_DATA_ACCESS_NAME, 1);
	
		/* make the condition */
		/* '%' in username is treated as a wildcard */
		if (strstr(userName, "%")) {
			snprintf (condStr, MAX_NAME_LEN, " like '%s'", userName);
		}
		else {
			snprintf (condStr, MAX_NAME_LEN, " = '%s'", userName);
		}

		addInxVal (&genQueryInp.sqlCondInp, COL_USER_NAME, condStr);
		
		

		/* Currently necessary since other namespaces exist in the token table */
		snprintf (condStr, MAX_NAME_LEN, "='%s'", "access_type");
		addInxVal (&genQueryInp.sqlCondInp, COL_DATA_TOKEN_NAMESPACE, condStr);
	
		genQueryInp.maxRows = MAX_SQL_ROWS;

	
		/* ICAT query for user info */
		status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

		/* print results out to buffer */
		if (status == 0) {
			extractACLQueryResults(genQueryOut, mybuf, 1);
	
			/* not done? */
			while (status==0 && genQueryOut->continueInx > 0) {
				genQueryInp.continueInx=genQueryOut->continueInx;
				status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);

				extractACLQueryResults(genQueryOut, mybuf, 1);
			}
		}

		return (0);
}



/*
 * genAdminOpFromDataObj()
 *
 */
int
genAdminOpFromDataObj(dataObjInp_t *dataObjInp, generalAdminInp_t *generalAdminInp, rsComm_t *rsComm)
{
	openedDataObjInp_t dataObjReadInp;
	openedDataObjInp_t dataObjCloseInp;
	openedDataObjInp_t dataObjLseekInp;
	fileLseekOut_t *dataObjLseekOut;
	bytesBuf_t *readBuf;
	int status;
	int objID;
	char *lineStart, *lineEnd;
	int bytesRead;

	
	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "genAdminOpFromDataObj: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* check for proper input */
	if (dataObjInp == NULL) {
		rodsLog (LOG_ERROR, "genAdminOpFromDataObj: input data object is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	
	
	/* no need to go further if client user is not local admin */
	if (rsComm->clientUser.authInfo.authFlag != 5) {
		return (CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
	}
	

	/* open user info file */
	if ((objID=rsDataObjOpen(rsComm, dataObjInp)) < 0) {
		return (objID);
	}

	/* read buffer init */
	readBuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	readBuf->len = 5*1024*1024;	/* no more than 5MB at a time */
	readBuf->buf = (char *)malloc(readBuf->len);
	memset (readBuf->buf, '\0', readBuf->len);
	
	
	/* read and parse user info file */
	memset (&dataObjReadInp, 0, sizeof (dataObjReadInp));
	dataObjReadInp.l1descInx = objID;
	dataObjReadInp.len = readBuf->len;/* 200; */

	/* init offset */
	dataObjLseekInp.l1descInx = objID;
	dataObjLseekInp.offset = 0;
	

	
	while ((bytesRead = rsDataObjRead (rsComm, &dataObjReadInp, readBuf)) > 0) {
		
		/* in case buffer is not null terminated */
		appendStrToBBuf(readBuf, "");

		
		lineStart = (char*)readBuf->buf;

		while ( (lineEnd=strstr(lineStart, "\n")) ) {
			lineEnd[0]='\0';
			
			status = parseGenAdminLine(lineStart, generalAdminInp, rsComm);
			
			lineStart=lineEnd+1;
		}
		
		/* must read again any final line that got cut before '\n' */
		dataObjLseekInp.offset = dataObjLseekInp.offset + bytesRead - strlen(lineStart);
		dataObjLseekInp.whence = SEEK_SET;
		
		
		/* make sure not to get stuck, for example if the file doesn't end with a new line */
		if (strlen(lineStart) >= (size_t)bytesRead) {
			parseGenAdminLine(lineStart, generalAdminInp, rsComm);
			break;
		}
		
		status = rsDataObjLseek (rsComm, &dataObjLseekInp, &dataObjLseekOut);

		
		memset(readBuf->buf, 0, readBuf->len);

	
	}


	
	/* close user info file */
	memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
	dataObjCloseInp.l1descInx = objID;
	
	status = rsDataObjClose (rsComm, &dataObjCloseInp);	


	return (status);
}



int
parseGenAdminLine(char *inpLine, generalAdminInp_t *generalAdminInp, rsComm_t *rsComm)
{
	int status;
	char *userName, *userID, *userType, *userZone;
	char *field, *newValue;
	char *tmpPtr;

	

	/* sanity checks */
	if (!rsComm) {
		rodsLog (LOG_ERROR, "parseGenAdminLine: rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);	
	}
	
	if (!inpLine) {
		rodsLog (LOG_ERROR, "parseGenAdminLine: inpLine is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	
	if (!generalAdminInp->arg0 || !generalAdminInp->arg1) {
		rodsLog (LOG_ERROR, "parseGenAdminLine: Action type not specified");
		return (USER__NULL_INPUT_ERR);
	}
	
	
	/* is this a comment? */
	if (inpLine[0] == '#') {
		return (0);
	}


	/* What kind of operation are we doing? */
	/* create user */
	if (!strcmp(generalAdminInp->arg0, "add") && !strcmp(generalAdminInp->arg1, "user")) {
		/* get user name */
		userName = inpLine;
		if ( (tmpPtr = strstr(userName, "|")) == NULL) {
			rodsLog (LOG_ERROR, "parseGenAdminLine: Parse error: user name is NULL");
			return (USER__NULL_INPUT_ERR);
		}
		tmpPtr[0] = '\0';
		
		/* set username field of admin input object */
		generalAdminInp->arg2 = userName;
	
	
		/* get userID - not used for now */
		userID = tmpPtr+1;	
		if ( (tmpPtr = strstr(userID, "|")) == NULL) {
			rodsLog (LOG_ERROR, "parseGenAdminLine: Parse error: missing argument(s)");
			return (USER__NULL_INPUT_ERR);
		}
		tmpPtr[0] = '\0';
		
	
		/* get user type */
		userType = tmpPtr+1;	
		if ( (tmpPtr = strstr(userType, "|")) == NULL) {
			rodsLog (LOG_ERROR, "parseGenAdminLine: Parse error: missing argument(s)");
			return (USER__NULL_INPUT_ERR);
		}
		tmpPtr[0] = '\0';
	
		/* set user type field of admin input object */
		generalAdminInp->arg3 = userType;
	
		
		/* get user zone */
		userZone = tmpPtr+1;	
		if ( (tmpPtr = strstr(userZone, "|")) == NULL) {
			rodsLog (LOG_ERROR, "parseGenAdminLine: Parse error: missing argument(s)");
			return (USER__NULL_INPUT_ERR);
		}
		tmpPtr[0] = '\0';
	
		/* set user zone field of admin input object */
		generalAdminInp->arg4 = userZone;
		
	
		/* auth string - to avoid error */
		generalAdminInp->arg5 = "";
		
		
		/* user create query */
		status = rsGeneralAdmin(rsComm, generalAdminInp);
		
		return (status);
	}
	
	/* modify user */
	if (!strcmp(generalAdminInp->arg0, "modify") && !strcmp(generalAdminInp->arg1, "user")) {
		/* get user name */
		userName = inpLine;
		if ( (tmpPtr = strstr(userName, "|")) == NULL) {
			rodsLog (LOG_ERROR, "parseGenAdminLine: Parse error: user name is NULL");
			return (USER__NULL_INPUT_ERR);
		}
		tmpPtr[0] = '\0';
		
		/* set username field of admin input object */
		generalAdminInp->arg2 = userName;
		
	
		/* get field to modify */
		field = tmpPtr+1;	
		if ( (tmpPtr = strstr(field, "|")) == NULL) {
			rodsLog (LOG_ERROR, "parseGenAdminLine: Parse error: missing argument(s)");
			return (USER__NULL_INPUT_ERR);
		}
		tmpPtr[0] = '\0';
	
		/* set field type of admin input object */
		generalAdminInp->arg3 = field;
	
		
		/* get new value */
		newValue = tmpPtr+1;	
		if (!newValue || !strlen(newValue)) {
			generalAdminInp->arg4 = "";
		}
		else {
			/* set new value field of admin input object */
			generalAdminInp->arg4 = newValue;
		}
		
		/* specific steps when dealing with passwords */
		if (!strcmp(field,"password")) {
			int i, len, lcopy;
			char buf0[MAX_PASSWORD_LEN+10];
			char buf1[MAX_PASSWORD_LEN+10];
			char buf2[MAX_PASSWORD_LEN+10];
			char rand[]="1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs"; 

			snprintf(buf0, MAX_PASSWORD_LEN+1, "%s", newValue);
			len = strlen(newValue);
			lcopy = MAX_PASSWORD_LEN-10-len;
			if (lcopy > 15) {  /* server will look for 15 characters of random string */
				strncat(buf0, rand, lcopy);
			}
			i = obfGetPw(buf1);
			if (i !=0) {

				rodsLog (LOG_ERROR, "parseGenAdminLine: Error getting password");
				return (USER__NULL_INPUT_ERR);
			}
			obfEncodeByKey(buf0, buf1, buf2);
			
			generalAdminInp->arg4 = buf2;
		}
		
		
		/* user create query */
		status = rsGeneralAdmin(rsComm, generalAdminInp);
		
		return (status);
	}
	
	/* delete user */
	if (!strcmp(generalAdminInp->arg0, "rm") && !strcmp(generalAdminInp->arg1, "user")) {
		/* get user name */
		userName = inpLine;
		if ( (tmpPtr = strstr(userName, "|")) != NULL) {
			tmpPtr[0] = '\0';
		}
		
		/* set username field of admin input object */
		generalAdminInp->arg2 = userName;
		
	
		/* get zone name from the client user (who should be the admin for this zone) */
		generalAdminInp->arg3 = rsComm->clientUser.rodsZone;

		
		/* user create query */
		status = rsGeneralAdmin(rsComm, generalAdminInp);
		
		return (status);
	}
	

	return (0);
}



/*
 * loadACLFromDataObj()
 *
 */
int
loadACLFromDataObj(dataObjInp_t *dataObjInp, rsComm_t *rsComm)
{
	openedDataObjInp_t dataObjReadInp;
	openedDataObjInp_t dataObjCloseInp;
	openedDataObjInp_t dataObjLseekInp;
	fileLseekOut_t *dataObjLseekOut;
	bytesBuf_t *readBuf;
	int status;
	int objID;
	char *lineStart, *lineEnd;
	int bytesRead;

	
	/* check for valid connection */
	if (rsComm == NULL) {
		rodsLog (LOG_ERROR, "loadACLFromDataObj: input rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* check for proper input */
	if (dataObjInp == NULL) {
		rodsLog (LOG_ERROR, "loadACLFromDataObj: input data object is NULL");
		return (USER__NULL_INPUT_ERR);
	}


	/* open ACL file */
	if ((objID=rsDataObjOpen(rsComm, dataObjInp)) < 0) {
		return (objID);
	}

	/* read buffer init */
	readBuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	readBuf->len = 5*1024*1024;	/* no more than 5MB at a time */
	readBuf->buf = (char *)malloc(readBuf->len);
	memset (readBuf->buf, '\0', readBuf->len);
	
	
	/* read and parse ACL file */
	memset (&dataObjReadInp, 0, sizeof (dataObjReadInp));
	dataObjReadInp.l1descInx = objID;
	dataObjReadInp.len = readBuf->len;/* 200; */

	/* init offset */
	dataObjLseekInp.l1descInx = objID;
	dataObjLseekInp.offset = 0;
	

	
	while ((bytesRead = rsDataObjRead (rsComm, &dataObjReadInp, readBuf)) > 0) {
		
		/* in case buffer is not null terminated */
		appendStrToBBuf(readBuf, "");

		
		lineStart = (char*)readBuf->buf;

		while ( (lineEnd=strstr(lineStart, "\n")) ) {
			lineEnd[0]='\0';
			
			status = parseACLModLine(lineStart, rsComm);
			
			lineStart=lineEnd+1;
		}
		
		/* must read again any final line that got cut before '\n' */
		dataObjLseekInp.offset = dataObjLseekInp.offset + bytesRead - strlen(lineStart);
		dataObjLseekInp.whence = SEEK_SET;
		
		
		/* make sure not to get stuck, for example if the file doesn't end with a new line */
		if (strlen(lineStart) >= (size_t)bytesRead) {
			parseACLModLine(lineStart, rsComm);
			break;
		}
		
		status = rsDataObjLseek (rsComm, &dataObjLseekInp, &dataObjLseekOut);

		
		memset(readBuf->buf, 0, readBuf->len);

	
	}


	
	/* close ACL file */
	memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
	dataObjCloseInp.l1descInx = objID;
	
	status = rsDataObjClose (rsComm, &dataObjCloseInp);	


	return (status);
}



int
parseACLModLine(char *inpLine, rsComm_t *rsComm)
{
	modAccessControlInp_t modAccessControl;
	int status;
	char *accessLevel, *user, *path;
	char userName[NAME_LEN];
	char zoneName[NAME_LEN];
	char *tmpPtr;
	

	/* sanity checks */
	if (!rsComm) {
		rodsLog (LOG_ERROR, "parseACLModLine: rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);	
	}
	
	if (!inpLine) {
		rodsLog (LOG_ERROR, "parseACLModLine: inpLine is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	
	
	/* is this a comment? */
	if (inpLine[0] == '#') {
		return (0);
	}


	/* init modACL input */
	memset (&modAccessControl, 0, sizeof(modAccessControlInp_t));
	modAccessControl.recursiveFlag = 0;

	
	/* get object path */
	path = inpLine;
	if ( (tmpPtr = strstr(path, "|")) == NULL) {
		rodsLog (LOG_ERROR, "parseACLModLine: Parse error: object path is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	tmpPtr[0] = '\0';
	
	/* set path field of input struct */
	modAccessControl.path = path;


	/* get user name and zone */
	user = tmpPtr+1;	
	if ( (tmpPtr = strstr(user, "|")) == NULL) {
		rodsLog (LOG_ERROR, "parseACLModLine: Parse error: user is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	tmpPtr[0] = '\0';

	status = parseUserName(user, userName, zoneName);

	/* set userName and zone fields of input struct */
	modAccessControl.userName = userName;
	modAccessControl.zone = zoneName;
	

	/* get access level */
	accessLevel = tmpPtr+1;
	if (!accessLevel || !strlen(accessLevel)) {
		rodsLog (LOG_ERROR, "parseACLModLine: Parse error: accessLevel is NULL");
		return (USER__NULL_INPUT_ERR);
	}
	
	
	/* set accessLevel field of input struct */
	modAccessControl.accessLevel = accessLevel;

	
	/* query for ACL mods */
	status = rsModAccessControl(rsComm, &modAccessControl);
	
	return (status);
}



/*
 * getSqlRowsByInx()
 *
 */
int
getSqlRowsByInx(genQueryOut_t *genQueryOut, intArray_t *indexes, bytesBuf_t *mybuf)
{
	char *resultStringToken;
	sqlResult_t *sqlResult;
	int i, j;


	for (i=0;i<genQueryOut->rowCnt;i++) {

		for (j=0;j<indexes->len;j++) {
			sqlResult = getSqlResultByInx (genQueryOut, indexes->value[j]);

			resultStringToken = sqlResult->value + i*sqlResult->len;

			appendStrToBBuf(mybuf, resultStringToken);
			appendStrToBBuf(mybuf, "|");
		}


		appendStrToBBuf(mybuf, "\n");
	}

	return (i);
}


/*
 * getObjectByFilePath()
 *
 * objPath should be a string of size MAX_NAME_LEN
 *
 */
int
getObjectByFilePath(char *filePath, char *rescName, char *objPath, rsComm_t *rsComm)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;
	char condStr[MAX_NAME_LEN];
    sqlResult_t *dataName, *collName;
    int status;


	/* Query init */
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	genQueryInp.maxRows = 1;

	/* Wanted fields */
	addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);

	/* Conditions */
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", filePath);
	addInxVal (&genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr);

	snprintf (condStr, MAX_NAME_LEN, " = '%s'", rescName);
	addInxVal (&genQueryInp.sqlCondInp, COL_D_RESC_NAME, condStr);

	/* Query */
	status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

	if (status >= 0)
	{
		/* Extract results */
		dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME);
		collName = getSqlResultByInx (genQueryOut, COL_COLL_NAME);

		/* Print out what we want */
		snprintf(objPath, MAX_NAME_LEN, "%s/%s", collName->value, dataName->value);
	}

	return (status);
}



