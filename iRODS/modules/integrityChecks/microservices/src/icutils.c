#include "integrityChecksMS.h"
#include "icutils.h"


/* Utility function for listing file object and an input parameter field */
int msiListFields (msParam_t *collinp, msParam_t *fieldinp, 
	msParam_t *bufout, msParam_t* statout, ruleExecInfo_t *rei) {

	genQueryInp_t gqin;
	genQueryOut_t *gqout = NULL;
	char condStr[MAX_NAME_LEN];
	char tmpstr[MAX_NAME_LEN];
	rsComm_t *rsComm;
	char* collname;
	char* fieldname;
	sqlResult_t *dataName;
	sqlResult_t *dataField;
	bytesBuf_t*	mybuf=NULL;
	int i,j;
	int	fieldid;

	RE_TEST_MACRO ("    Calling msiListFields")

	/* Sanity check */
	if (rei == NULL || rei->rsComm == NULL) {
		rodsLog (LOG_ERROR, "msiListFields: input rei or rsComm is NULL");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	rsComm = rei->rsComm;

	/* init gqout, gqin & mybuf structs */
	mybuf = (bytesBuf_t *) malloc(sizeof(bytesBuf_t));
	memset (mybuf, 0, sizeof (bytesBuf_t));
	gqout = (genQueryOut_t *) malloc (sizeof (genQueryOut_t));
	memset (gqout, 0, sizeof (genQueryOut_t));
	memset (&gqin, 0, sizeof (genQueryInp_t));
	gqin.maxRows = MAX_SQL_ROWS;

	/* construct an SQL query from the parameter list */
	collname = strdup ((char*)collinp->inOutStruct);
	fieldname = strdup ((char*)fieldinp->inOutStruct);
	if ((fieldid = getAttrIdFromAttrName(fieldname))==NO_COLUMN_NAME_FOUND) {
		sprintf (tmpstr, "Field: %s not found in database", fieldname);
		appendToByteBuf (mybuf, tmpstr);
		free( gqout ); // JMC cppcheck - leak
		free( collname ); // JMC cppcheck - leak
		return (-1);
	}

	/* this is the info we want returned from the query */
	addInxIval (&gqin.selectInp, COL_DATA_NAME, 1);
	addInxIval (&gqin.selectInp, fieldid, 1);
	snprintf (condStr, MAX_NAME_LEN, " = '%s'", collname);
	addInxVal (&gqin.sqlCondInp, COL_COLL_NAME, condStr);

	j = rsGenQuery (rsComm, &gqin, &gqout);

	if (j<0) {
		appendToByteBuf (mybuf, "Gen Query Error returned badness");
	} else if (j != CAT_NO_ROWS_FOUND) {

		printGenQueryOut(stderr, NULL, NULL, gqout);

		dataName = getSqlResultByInx (gqout, COL_DATA_NAME);
		dataField = getSqlResultByInx (gqout, fieldid);

		//rodsLog (LOG_ERROR, "got here 3 rowCnt=%d",gqout->rowCnt);

		for (i=0; i<gqout->rowCnt; i++) {
			sprintf (tmpstr, "Data object:%s\t%s:%s,", &dataName->value[dataName->len *i], fieldname, &dataField->value[dataField->len *i]);
			appendToByteBuf (mybuf, tmpstr);
		}

	} else appendToByteBuf (mybuf, "No matching rows found");

	bufout->type = strdup(GenQueryOut_MS_T);
	fillBufLenInMsParam (bufout, mybuf->len, mybuf);
	fillIntInMsParam (statout, rei->status);
 
    free( collname ); // JMC cppcheck - leak 
	return(rei->status);
	
}


/* Test writePosInt microservice */
int msiTestWritePosInt (msParam_t* mPout1, ruleExecInfo_t *rei) {

	int butter=99;

	RE_TEST_MACRO ("    Calling msiTestWritePosInt")

	fillIntInMsParam (mPout1, butter);

	return(rei->status);
	
}

/* Testing passing of info from C function to rule */
int msiTestForEachExec (msParam_t* mPout1, ruleExecInfo_t *rei) {

	int numbers[10];
	int i;
	bytesBuf_t* mybuf;
	char tmpstr[MAX_NAME_LEN];

	RE_TEST_MACRO ("    Calling msiTestForEachExec")

	/* make some fake data */
	for (i=0;i<10;i++) numbers[i]=i;	

	mybuf = (bytesBuf_t *) malloc(sizeof(bytesBuf_t));
	memset (mybuf,0, sizeof(bytesBuf_t));

	/* mybuf will contain a comma-separated list */
	for (i=0;i<10;i++) {
		sprintf (tmpstr, "number[%d]=%d,", i, i);
		appendToByteBuf (mybuf, tmpstr);
	}

	mPout1->type = strdup (STR_MS_T);
	//fillBufLenInMsParam (mPout1, mybuf->len, mybuf);
	fillStrInMsParam (mPout1, (char*)mybuf->buf);

	return(rei->status);
	
}

/* Silly hello world microservice */
int msiHiThere (msParam_t* mPout1, ruleExecInfo_t *rei) {

	char str[]="hi there\n";

	RE_TEST_MACRO ("    Calling msiHiThere")

	fillStrInMsParam (mPout1, str);

	return(rei->status);
	
}

/* verify consistency of the owners across a query */
int verifyCollOwners (genQueryOut_t* gqout, char* ownerlist, bytesBuf_t* mybuf) {

	sqlResult_t* collName;
	sqlResult_t* collOwner;
	sqlResult_t* collID;
	int ownercount=0;
	int i,j;
	char* word;
	char delims[]=",";
    char** olist=NULL;
	char tmpstr[MAX_NAME_LEN];

	collName = getSqlResultByInx (gqout, COL_COLL_NAME);
	collOwner = getSqlResultByInx (gqout, COL_COLL_OWNER_NAME);
	collID = getSqlResultByInx (gqout, COL_COLL_ID);

	//Assume ownerlist is not NULL
	fprintf(stderr, "ownerlist: %s\n", ownerlist);

	/* Construct a list of owners from our input parameter*/
	for (word=strtok(ownerlist, delims); word; word=strtok(NULL, delims)) {
		olist = (char**) realloc (olist, sizeof (char*) * (ownercount));
		olist[ownercount] = strdup (word);
		ownercount++;
	}

	/* Now compare each subCollection's owner with our list */
	for (i=0; i<gqout->rowCnt; i++) {
		int foundflag=0;
		for (j=0; j<ownercount; j++) {
			char* thisowner = strdup(&collOwner->value[collOwner->len*i]);
			fprintf(stderr, "comparing %s and %s\n", thisowner, olist[j]);
			if (!(strcmp(thisowner, olist[j]))) {
				/* We only care about the ones that DON'T match */
				foundflag=1;
                free( thisowner ); // JMC cppcheck - leak 
				break;  
			}
		}

		if (!foundflag) {
			snprintf (tmpstr, MAX_NAME_LEN, "Sub Collection: %s with owner: %s does not match input list\n",
				&collName->value[collName->len *i], &collOwner->value[collOwner->len * i]);
			appendToByteBuf (mybuf, tmpstr);
		}
	}

	return (0);
}


int verifyCollAVU (genQueryOut_t* gqout, char* myavuname, char* myavuvalue, char* myavuattr, bytesBuf_t* mybuf) {

//	sqlResult_t* collName;
//	sqlResult_t* avuname;
//	sqlResult_t* avuavlue;
//	sqlResult_t* avuattr;
	int status = 0; // JMC cppcheck - uninit var

	return (status);
}

int verifyCollACL (genQueryOut_t* gqout, char* myaclname, char* myacltype, bytesBuf_t* mybuf) {
	int status = 0; // JMC cppcheck - uninit var

	return (status);
}
