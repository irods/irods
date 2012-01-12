#include "stdafx.h"
#include "dirent.h"
//#include "rodsGenQueryNames.h"
#include "dataObjRename.h"
#include "mvUtil.h"
#include "getUtil.h"
#include "irodsWinUtils.h"
#include "dataObjInpOut.h"
#include "putUtil.h"
#include "replUtil.h"
#include "rmUtil.h"
#include "mvUtil.h"
#include "cpUtil.h"
#include "modAccessControl.h"
#include "miscUtil.h"
#include "rodsClient.h"
#include "userAdmin.h"
#include "execMyRule.h"
#include "rmtrashUtil.h"

int irodsWinQueryFilenameAdvanced(rcComm_t *conn, CString quest_where_str,
		   CString queryZone, int noDistinctFlag,
			CArray<CString, CString> & ParColls, CArray<CString, CString> & DataNames)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut = NULL;
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	CString quest_str = CString("SELECT COLL_NAME, DATA_NAME WHERE ") + quest_where_str; // notice the selected fields are fixed.

	int t = fillGenQueryInpFromStrCond((char *)LPCTSTR(quest_str), &genQueryInp);
	if(t < 0)
	{	
		return t;
	}

	if(noDistinctFlag)
	{
		genQueryInp.options = NO_DISTINCT;
	}
	if(queryZone.GetLength() > 0)
	{
		addKeyVal (&genQueryInp.condInput, ZONE_KW, (char *)LPCTSTR(queryZone));
	}
	
	CString obj_name, coll_name;
	sqlResult_t *collName, *dataName;

	genQueryInp.maxRows= MAX_SQL_ROWS;
	genQueryInp.continueInx=0;
	do 
	{
		if(genQueryOut != NULL)
		{
			genQueryInp.continueInx=genQueryOut->continueInx;
		}
		t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
		if(t < 0)
		{
			if( t == CAT_NO_ROWS_FOUND)
			{
				return 0;
			}
			return t;
		}

		// save query data
		if ((collName = getSqlResultByInx (genQueryOut, COL_COLL_NAME))== NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME))== NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		
		for (int i = 0;i < genQueryOut->rowCnt; i++)
		{
			coll_name = &collName->value[collName->len * i];
			obj_name = &dataName->value[dataName->len * i];
			ParColls.Add(coll_name);
			DataNames.Add(obj_name);
		}
	}
	while((t == 0)&&(genQueryInp.continueInx>0));

	return 0;
}

int irodsWinQueryFilename(rcComm_t *conn, const CString& TopQueryColl, int RecursiveSrch,
				CString & FilenameQueryOp, CString & FilenameQueryStr,
				CArray<CString, CString> & ParColls, CArray<CString, CString> & DataNames)
{
	int t;
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut=NULL;
	sqlResult_t *collName, *dataName;

	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	char collQCond[MAX_NAME_LEN];
	if(TopQueryColl.GetLength() > 0)
	{
		if(RecursiveSrch)
		{
			snprintf (collQCond, MAX_NAME_LEN, "like '%s%%'", (char *)LPCTSTR(TopQueryColl));
		}
		else
		{
			snprintf (collQCond, MAX_NAME_LEN, "='%s'", (char *)LPCTSTR(TopQueryColl));
		}
		addInxVal (&(genQueryInp.sqlCondInp), COL_COLL_NAME, collQCond);
	}

	char fnameQCond[MAX_NAME_LEN];
	if(FilenameQueryOp.CompareNoCase("contains") == 0)
	{
		snprintf(fnameQCond, MAX_NAME_LEN, "like '%%%s%%'", (char *)LPCTSTR(FilenameQueryStr));
	}
	else
	{
		snprintf(fnameQCond, MAX_NAME_LEN, "%s '%s'", (char *)LPCTSTR(FilenameQueryOp), (char *)LPCTSTR(FilenameQueryStr));
	}
	addInxVal (&(genQueryInp.sqlCondInp), COL_DATA_NAME, fnameQCond);

	addInxIval (&(genQueryInp.selectInp), COL_COLL_NAME, 1);
	addInxIval (&(genQueryInp.selectInp), COL_DATA_NAME, 1);

	genQueryInp.maxRows = 400;
	genQueryInp.continueInx = 0;
	t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
	if(t< 0)
	{
		if( t == CAT_NO_ROWS_FOUND)
		{
			return 0;
		}

		return t;
	}

	int total_cnt = 0;
	ParColls.RemoveAll();
	DataNames.RemoveAll();
	CString obj_name, coll_name;
	bool loop_stop = false;
	do
	{
		
		if ((collName = getSqlResultByInx (genQueryOut, COL_COLL_NAME))== NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME))== NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		
		for (int i = 0;i < genQueryOut->rowCnt; i++)
		{
			// &collName->value[collName->len * i];
			coll_name = &collName->value[collName->len * i];
			obj_name = &dataName->value[dataName->len * i];
			ParColls.Add(coll_name);
			DataNames.Add(obj_name);
			total_cnt++;
		}

		if(genQueryOut->continueInx == 0)
		{
			loop_stop = true;
		}
		else
		{
			genQueryInp.continueInx=genQueryOut->continueInx;
			t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
			loop_stop = false;
		}
	}
	while ((t==0) && (!loop_stop));

	freeGenQueryOut(&genQueryOut);

	return total_cnt;
}


int irodsWinQueryMetadata(rcComm_t *conn, const CString& TopQueryColl, int RecursiveSrch,
		CArray<CString, CString> & AttNameOps, CArray<CString, CString> & AttNameQueryStrs,
		CArray<CString, CString> & AttValOps, CArray<CString, CString> & AttValQueryStrs,
		CArray<CString, CString> & ParColls, CArray<CString, CString> & DataNames)
{
	int t;
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut=NULL;
	sqlResult_t *collName, *dataName;

	// we support up to 4 queries for att name and values. This should be taken care by the callers.
	int n = AttNameOps.GetSize();
	if((n <= 0)||(n > 4))
		return -1;
	if(n != AttNameQueryStrs.GetSize())
		return -1;
	if(n != AttValOps.GetSize())
		return -1;
	if(n != AttValQueryStrs.GetSize())
		return -1;

	// now let's start

	// condition field(s)
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	char collQCond[MAX_NAME_LEN];
	if(TopQueryColl.GetLength() > 0)
	{
		if(RecursiveSrch)
		{
			snprintf (collQCond, MAX_NAME_LEN, "like '%s%%'", (char *)LPCTSTR(TopQueryColl));
		}
		else
		{
			snprintf (collQCond, MAX_NAME_LEN, "='%s'", (char *)LPCTSTR(TopQueryColl));
		}
		addInxVal (&(genQueryInp.sqlCondInp), COL_COLL_NAME, collQCond);  // COL_COLL_PARENT_NAME
	}
	char attNameQConds[4][MAX_NAME_LEN], attValQConds[4][MAX_NAME_LEN];
	int i;
	for(i=0;i<n;i++)
	{
		if(AttNameOps[i].CompareNoCase("contains") == 0)
		{
			snprintf(attNameQConds[i], MAX_NAME_LEN, "like '%%%s%%'", (char *)LPCTSTR(AttNameQueryStrs[i]));
		}
		else
		{
			snprintf(attNameQConds[i], MAX_NAME_LEN, "%s '%s'", (char *)LPCTSTR(AttNameOps[i]), (char *)LPCTSTR(AttNameQueryStrs[i]));
		}
		addInxVal (&(genQueryInp.sqlCondInp), COL_META_DATA_ATTR_NAME, attNameQConds[i]);

		if(AttValOps[i].CompareNoCase("contains") == 0)
		{
			snprintf(attValQConds[i], MAX_NAME_LEN, "like '%%%s%%'", (char *)LPCTSTR(AttValQueryStrs[i]));
		}
		else
		{
			snprintf(attValQConds[i], MAX_NAME_LEN, "%s '%s'", (char *)LPCTSTR(AttValOps[i]), (char *)LPCTSTR(AttValQueryStrs[i]));
		}
		addInxVal (&(genQueryInp.sqlCondInp), COL_META_DATA_ATTR_VALUE, attValQConds[i]);
	}

	// selection fields
	addInxIval (&(genQueryInp.selectInp), COL_COLL_NAME, 1);
	addInxIval (&(genQueryInp.selectInp), COL_DATA_NAME, 1);

	genQueryInp.maxRows = 400;
	genQueryInp.continueInx = 0;
	t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
	if(t< 0)
	{
		if( t == CAT_NO_ROWS_FOUND)
		{
			return 0;
		}

		return t;
	}

	int total_cnt = 0;
	CString obj_name, coll_name;
	bool loop_stop = false;
	do
	{
		
		if ((collName = getSqlResultByInx (genQueryOut, COL_COLL_NAME))== NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME))== NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		
		for (int i = 0;i < genQueryOut->rowCnt; i++)
		{
			// &collName->value[collName->len * i];
			coll_name = &collName->value[collName->len * i];
			obj_name = &dataName->value[dataName->len * i];
			ParColls.Add(coll_name);
			DataNames.Add(obj_name);
			total_cnt++;
		}

		if(genQueryOut->continueInx == 0)
		{
			loop_stop = true;
		}
		else
		{
			genQueryInp.continueInx=genQueryOut->continueInx;
			t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
			loop_stop = false;
		}
	}
	while ((t==0) && (!loop_stop));

	freeGenQueryOut(&genQueryOut);

	return total_cnt;
}

static int irodsWinGetChildObjsInMountedColl(rcComm_t *conn, const CString& ParentColl, specColl_t *pSpecColl,
				 CArray<irodsWinDataobj, irodsWinDataobj> & ChildDatasets)
{
	dataObjInp_t dataObjInp;
	memset(&dataObjInp, 0, sizeof (dataObjInp));
	addKeyVal (&dataObjInp.condInput, SEL_OBJ_TYPE_KW, "dataObj");
	rstrcpy(dataObjInp.objPath, (char *)LPCTSTR(ParentColl), MAX_NAME_LEN);
	dataObjInp.openFlags = 0;
	dataObjInp.specColl = pSpecColl;

	int t;
	genQueryOut_t *genQueryOut = NULL;
	t = rcQuerySpecColl(conn, &dataObjInp, &genQueryOut);
	if(t< 0)
	{
		if( t == CAT_NO_ROWS_FOUND)
		{
			ChildDatasets.RemoveAll();
			return 0;
		}

		return t;
	}

	ChildDatasets.RemoveAll();

	char *tmpstr;
	char thelocaltime[20];
	bool loop_stop = false;
	sqlResult_t *dataName, *replNum, *dataSize, *rescName, *replStatus, *dataModify, *dataOwnerName;
	irodsWinDataobj tmpIrodsDatobj;
	do
	{
		if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		if ((dataSize = getSqlResultByInx (genQueryOut, COL_DATA_SIZE)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		if ((dataModify = getSqlResultByInx (genQueryOut, COL_D_MODIFY_TIME)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		replStatus = getSqlResultByInx (genQueryOut, COL_D_REPL_STATUS);
		rescName = getSqlResultByInx (genQueryOut, COL_D_RESC_NAME);
		dataOwnerName = getSqlResultByInx (genQueryOut, COL_D_OWNER_NAME);
		replNum = getSqlResultByInx (genQueryOut, COL_DATA_REPL_NUM);

		for (int i = 0;i < genQueryOut->rowCnt; i++)
		{
			tmpIrodsDatobj.name = &dataName->value[dataName->len * i];
			tmpIrodsDatobj.parentCollectionFullPath = ParentColl;
			if(replNum != NULL)
			{
				tmpstr = &replNum->value[replNum->len * i];
				tmpIrodsDatobj.replNum = atoi(tmpstr);
			}
			tmpstr = &dataSize->value[dataSize->len * i];
			tmpIrodsDatobj.size = (__int64)_atoi64(tmpstr);
			//if(rescName != NULL)
			//tmpIrodsDatobj.rescName = &rescName->value[rescName->len * i];
			 tmpIrodsDatobj.rescName = pSpecColl->resource;
			if(dataOwnerName != NULL)
				tmpIrodsDatobj.owner = &dataOwnerName->value[dataOwnerName->len * i]; 
			if(replStatus != NULL)
			{
				tmpstr = &replStatus->value[replStatus->len * i];
				tmpIrodsDatobj.replStatus = atoi (tmpstr);
			}
			//tmpIrodsDatobj.modifiedTime = &dataModify->value[dataModify->len * i];
			tmpstr = &dataModify->value[dataModify->len * i];
			getLocalTimeFromRodsTime(tmpstr, thelocaltime);
			tmpIrodsDatobj.modifiedTime = thelocaltime;
			ChildDatasets.Add(tmpIrodsDatobj);
		}
		
		// next query
		if(genQueryOut->continueInx > 0)
		{
			dataObjInp.openFlags = genQueryOut->continueInx;
			free(genQueryOut);  genQueryOut = NULL;
			t = rcQuerySpecColl(conn, &dataObjInp, &genQueryOut);
			loop_stop = false;
		}
		else
		{
			loop_stop = true;
		}
	}
	while ((t==0) && (!loop_stop));

	freeGenQueryOut(&genQueryOut);

	return 0;
}

// The 'ParentColl'is the full path for parent collection.
int irodsWinGetChildObjs(rcComm_t *conn, const CString& ParentColl,
				 CArray<irodsWinDataobj, irodsWinDataobj> & ChildDatasets)
{
	queryHandle_t queryHandle;
	int queryFlags = VERY_LONG_METADATA_FG;    // LONG_METADATA_FG;
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut = NULL;
	int t;

	sqlResult_t *dataName, *replNum, *dataSize, *rescName,
				*replStatus, *dataModify, *dataOwnerName, *dataId;


	memset (&genQueryInp, 0, sizeof(genQueryInp_t));
	
	// get coll type
	dataObjInp_t dataObjInp;
	memset(&dataObjInp, 0, sizeof (dataObjInp));
	rstrcpy (dataObjInp.objPath, (char *)LPCTSTR(ParentColl), MAX_NAME_LEN);
	rodsObjStat_t *rodsObjStatOut = NULL;
	t = rcObjStat (conn, &dataObjInp, &rodsObjStatOut);
	if(t < 0)
		return t;

	if((rodsObjStatOut->specColl != NULL)&&(rodsObjStatOut->specColl->collClass == MOUNTED_COLL))
	{
		t = irodsWinGetChildObjsInMountedColl(conn, ParentColl, rodsObjStatOut->specColl, ChildDatasets);
		return t;
	}

	char collQCond[MAX_NAME_LEN];
	snprintf (collQCond, MAX_NAME_LEN, "='%s'", (char *)LPCTSTR(ParentColl));
    addInxVal (&(genQueryInp.sqlCondInp), COL_COLL_NAME, collQCond);
	setQueryInpForData (queryFlags, &genQueryInp);
	genQueryInp.maxRows = 400;

	t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
	if(t < 0)
	{
		if(t == CAT_NO_ROWS_FOUND)
		{
			ChildDatasets.RemoveAll();
			return 0;
		}
		return t;
	}

	ChildDatasets.RemoveAll();

	char *tmpstr;
	char thelocaltime[20];
	irodsWinDataobj tmpIrodsDatobj;
	bool loop_stop = false;
	do
	{
		if ((dataId = getSqlResultByInx (genQueryOut, COL_D_DATA_ID)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}

		if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}

		if ((replNum = getSqlResultByInx (genQueryOut, COL_DATA_REPL_NUM)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}

		if ((dataSize = getSqlResultByInx (genQueryOut, COL_DATA_SIZE)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}

		if ((rescName = getSqlResultByInx (genQueryOut, COL_D_RESC_NAME)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}

		if ((replStatus = getSqlResultByInx (genQueryOut, COL_D_REPL_STATUS)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}
		if ((dataModify = getSqlResultByInx (genQueryOut, COL_D_MODIFY_TIME)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}

		if ((dataOwnerName = getSqlResultByInx (genQueryOut, COL_D_OWNER_NAME)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}

		for (int i = 0;i < genQueryOut->rowCnt; i++)
		{
			/*
			if ((dataId = getSqlResultByInx (genQueryOut, COL_D_DATA_ID)) == NULL) 
			{
				return (UNMATCHED_KEY_OR_INDEX);
			}

			if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME)) == NULL) 
			{
				return (UNMATCHED_KEY_OR_INDEX);
			}

			if ((replNum = getSqlResultByInx (genQueryOut, COL_DATA_REPL_NUM)) == NULL) 
			{
				return (UNMATCHED_KEY_OR_INDEX);
			}

			if ((dataSize = getSqlResultByInx (genQueryOut, COL_DATA_SIZE)) == NULL) 
			{
				return (UNMATCHED_KEY_OR_INDEX);
			}

			if ((rescName = getSqlResultByInx (genQueryOut, COL_D_RESC_NAME)) == NULL) 
			{
				return (UNMATCHED_KEY_OR_INDEX);
			}

			if ((replStatus = getSqlResultByInx (genQueryOut, COL_D_REPL_STATUS)) == NULL) 
			{
				return (UNMATCHED_KEY_OR_INDEX);
			}
			if ((dataModify = getSqlResultByInx (genQueryOut, COL_D_MODIFY_TIME)) == NULL) 
			{
				return (UNMATCHED_KEY_OR_INDEX);
			}

			if ((dataOwnerName = getSqlResultByInx (genQueryOut, COL_D_OWNER_NAME)) == NULL) 
			{
				return (UNMATCHED_KEY_OR_INDEX);
			}
			*/

			tmpIrodsDatobj.name = &dataName->value[dataName->len * i];
			tmpIrodsDatobj.parentCollectionFullPath = ParentColl;
			tmpstr = &replNum->value[replNum->len * i];
			tmpIrodsDatobj.replNum = atoi(tmpstr);
			tmpstr = &dataSize->value[dataSize->len * i];
			tmpIrodsDatobj.size = (__int64)_atoi64(tmpstr);
			tmpIrodsDatobj.rescName = &rescName->value[rescName->len * i];
			tmpIrodsDatobj.owner = &dataOwnerName->value[dataOwnerName->len * i]; 
			tmpstr = &replStatus->value[replStatus->len * i];
			tmpIrodsDatobj.replStatus = atoi (tmpstr);
			//tmpIrodsDatobj.modifiedTime = &dataModify->value[dataModify->len * i];
			tmpstr = &dataModify->value[dataModify->len * i];
			getLocalTimeFromRodsTime(tmpstr, thelocaltime);
			tmpIrodsDatobj.modifiedTime = thelocaltime;

			ChildDatasets.Add(tmpIrodsDatobj);
		}

		if(genQueryOut->continueInx <= 0)
		{
			loop_stop = true;
		}
		else
		{
			genQueryInp.continueInx=genQueryOut->continueInx;
			t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
			loop_stop = false;
		}
	}
	while ((t==0) && (!loop_stop));

	freeGenQueryOut(&genQueryOut);

	return 0;
}

static int irodsWinGetChildCollsInMountedColl(rcComm_t *conn, const CString& ParentColl, 
				specColl_t *pSpecColl, CArray<irodsWinCollection, irodsWinCollection> & ChildCollections)
{
	dataObjInp_t dataObjInp;
	memset(&dataObjInp, 0, sizeof (dataObjInp));
	addKeyVal (&dataObjInp.condInput, SEL_OBJ_TYPE_KW, "collection");
	rstrcpy(dataObjInp.objPath, (char *)LPCTSTR(ParentColl), MAX_NAME_LEN);
	dataObjInp.openFlags = 0;
	dataObjInp.specColl = pSpecColl;

	int t;
	genQueryOut_t *genQueryOut = NULL;
	t = rcQuerySpecColl(conn, &dataObjInp, &genQueryOut);
	if(t< 0)
	{
		if( t == CAT_NO_ROWS_FOUND)
		{
			ChildCollections.RemoveAll();
			return 0;
		}

		return t;
	}

	ChildCollections.RemoveAll();

	CString tname;
	irodsWinCollection tmpColl;
	sqlResult_t *collName, *collType, *collInfo1, *collInfo2, *ownerName;
	bool loop_stop = false;
	do
	{
		if ((collName = getSqlResultByInx (genQueryOut, COL_COLL_NAME))== NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		} 
		collType = getSqlResultByInx (genQueryOut, COL_COLL_TYPE);
		collInfo1 = getSqlResultByInx (genQueryOut, COL_COLL_INFO1);
		collInfo2 = getSqlResultByInx (genQueryOut, COL_COLL_INFO2);
		ownerName = getSqlResultByInx (genQueryOut, COL_COLL_OWNER_NAME);

		for (int i = 0; i < genQueryOut->rowCnt; i++)
		{
			tname = &collName->value[collName->len * i];
			if(tname[tname.GetLength()-1] == '/')
			{
				tname.Delete(tname.GetLength()-1);
			}
			tname.Trim();
			if(tname.GetLength() == 0)
				continue;
			if(tname == ParentColl)
				continue;
			// it is the full path name 
			tmpColl.fullPath = tname;
			int hh = tname.GetLength() - tname.ReverseFind('/') - 1;
			tmpColl.name = tname.Right(hh);
			if(collType != NULL)
				tmpColl.type = &collType->value[collType->len * i];
			if(collInfo1 != NULL)
				tmpColl.info1 = &collInfo1->value[collInfo1->len * i];
			if(collInfo2 != NULL)
				tmpColl.info2 = &collInfo2->value[collInfo2->len * i];
			if(ownerName != NULL)
				tmpColl.owner = &ownerName->value[ownerName->len * i];
			ChildCollections.Add(tmpColl);
		}

		if(genQueryOut->continueInx <= 0)
		{
			loop_stop = true;
		}
		else
		{
			dataObjInp.openFlags = genQueryOut->continueInx;
			free(genQueryOut);  genQueryOut = NULL;
			t = rcQuerySpecColl(conn, &dataObjInp, &genQueryOut);
			loop_stop = false;
		}
	}
	while ((t==0) && (!loop_stop));

	freeGenQueryOut(&genQueryOut);

	return 0;
}

// The 'ParentColl' is the full path for parent collection.
int irodsWinGetChildCollections(rcComm_t *conn, const CString& ParentColl, 
						CArray<irodsWinCollection, irodsWinCollection> & ChildCollections)
{
	int t;   // storing status code.
	//queryHandle_t queryHandle;
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut=NULL;
	sqlResult_t *collName, *collType, *collInfo1, *collInfo2, *ownerName;

	// get coll type
	dataObjInp_t dataObjInp;
	memset(&dataObjInp, 0, sizeof (dataObjInp));
	rstrcpy (dataObjInp.objPath, (char *)LPCTSTR(ParentColl), MAX_NAME_LEN);
	rodsObjStat_t *rodsObjStatOut = NULL;
	t = rcObjStat (conn, &dataObjInp, &rodsObjStatOut);
	if(t < 0)
		return t;

	if((rodsObjStatOut->specColl != NULL)&&(rodsObjStatOut->specColl->collClass == MOUNTED_COLL))
	{
		t = irodsWinGetChildCollsInMountedColl(conn, ParentColl, rodsObjStatOut->specColl, ChildCollections);
		return t;
	}

	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	char collQCond[MAX_NAME_LEN];
	snprintf (collQCond, MAX_NAME_LEN, "='%s'", (char *)LPCTSTR(ParentColl));
    addInxVal (&(genQueryInp.sqlCondInp), COL_COLL_PARENT_NAME, collQCond);
	addInxIval (&(genQueryInp.selectInp), COL_COLL_NAME, 1);
    addInxIval (&(genQueryInp.selectInp), COL_COLL_OWNER_NAME, 1);
    addInxIval (&(genQueryInp.selectInp), COL_COLL_CREATE_TIME, 1);
    addInxIval (&(genQueryInp.selectInp), COL_COLL_MODIFY_TIME, 1);
    addInxIval (&(genQueryInp.selectInp), COL_COLL_TYPE, 1);
    addInxIval (&(genQueryInp.selectInp), COL_COLL_INFO1, 1);
    addInxIval (&(genQueryInp.selectInp), COL_COLL_INFO2, 1);
    //genQueryInp.maxRows = MAX_SQL_ROWS;
	//genQueryInp.options = RETURN_TOTAL_ROW_COUNT;
	genQueryInp.maxRows = 400;

	irodsWinCollection tmpColl;
	//CString ParentCollNameWithSlash = ParentColl + CString("/");
	CString tname;

	t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
	if(t< 0)
	{
		if( t == CAT_NO_ROWS_FOUND)
		{
			ChildCollections.RemoveAll();
			return 0;
		}

		return t;
	}

	/* miscUtil.c: getNextCollMetaInfo( )  */
	ChildCollections.RemoveAll();
	bool loop_stop = false;
	do
	{
		if ((collName = getSqlResultByInx (genQueryOut, COL_COLL_NAME))== NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		} 
		else if ((collType = getSqlResultByInx (genQueryOut, COL_COLL_TYPE)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		} 
		else if ((collInfo1 = getSqlResultByInx (genQueryOut, COL_COLL_INFO1)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		} 
		else if ((collInfo2 = getSqlResultByInx (genQueryOut, COL_COLL_INFO2)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		} 
		else if ((ownerName = getSqlResultByInx (genQueryOut, COL_COLL_OWNER_NAME)) == NULL) 
		{
			return (UNMATCHED_KEY_OR_INDEX);
		}

		for (int i = 0; i < genQueryOut->rowCnt; i++)
		{
			tname = &collName->value[collName->len * i];
			if(tname[tname.GetLength()-1] == '/')
			{
				tname.Delete(tname.GetLength()-1);
			}
			tname.Trim();
			if(tname.GetLength() == 0)
				continue;
			if(tname == ParentColl)
				continue;
			// it is the full path name 
			tmpColl.fullPath = tname;
			int hh = tname.GetLength() - tname.ReverseFind('/') - 1;
			tmpColl.name = tname.Right(hh);
			tmpColl.type = &collType->value[collType->len * i];
			tmpColl.info1 = &collInfo1->value[collInfo1->len * i];
			tmpColl.info2 = &collInfo2->value[collInfo2->len * i];
			tmpColl.owner = &ownerName->value[ownerName->len * i];
			ChildCollections.Add(tmpColl);
		}

		if(genQueryOut->continueInx <= 0)
		{
			loop_stop = true;
		}
		else
		{
			genQueryInp.continueInx=genQueryOut->continueInx;
			free(genQueryOut);  genQueryOut = NULL;
			t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
			loop_stop = false;
		}
	}
	while ((t==0) && (!loop_stop));

	freeGenQueryOut(&genQueryOut);

	return 0;
}

/*
int irodsWinGetChildCollections(rcComm_t *conn, const CString& ParentColl, 
						CArray<irodsWinCollection, irodsWinCollection> & ChildCollections)
{
	collHandle_t collHandle;
	int queryFlags = DATA_QUERY_FIRST_FG | LONG_METADATA_FG | NO_TRIM_REPL_FG;
	int t = rclOpenCollection (conn, srcColl, queryFlags, &collHandle);
	if(t < 0) 
		return t;

	while ((t = rclReadCollection (conn, &collHandle, &collEnt)) >= 0)
	{
	}
}
*/

bool irodsWinIsTrashArea(CString & irodsObjFullPath)
{
	int i = irodsObjFullPath.Find('/', 1);
	if(i < 0)
		return false;

	int j = irodsObjFullPath.Find('/', i+1);
	if(j < 0)
		return false;

	CString coll_name = irodsObjFullPath.Left(j);
	coll_name = coll_name.Right(j-i-1);

	if(coll_name.CompareNoCase("trash") == 0)
		return true;

	return false;
}

int irodsWinDeleteOneCollection(rcComm_t *conn, CString & collWholePath, bool forceful, bool recursive)
{
	collInp_t collInp;
	int mforce = forceful;

	if(irodsWinIsTrashArea(collWholePath))
		mforce = true;

	memset (&collInp, 0, sizeof (collInp_t));
	strcpy(collInp.collName, (char *)LPCTSTR(collWholePath));
	if(mforce)
		addKeyVal (&(collInp.condInput), FORCE_FLAG_KW, "");
	if(recursive)
		addKeyVal (&(collInp.condInput), RECURSIVE_OPR__KW, "");

	int t = rcRmColl(conn, &collInp, false);

	return t;
}

int irodsWinDeleteOneDataobj(rcComm_t *conn, CString & dataName, CString & dataFullPath, int replNum, bool forceful)
{
	rodsEnv myRodsEnv;
	memset(&myRodsEnv, 0, sizeof(myRodsEnv));
	irodsWinSetRodsEnvViaConn(conn, &myRodsEnv);

	bool mforce = forceful;
	if(irodsWinIsTrashArea(dataFullPath))
		mforce = true;

	rodsArguments_t myRodsArgs;
	memset(&myRodsArgs, 0, sizeof(rodsArguments_t));
	if(mforce)
		myRodsArgs.force = 1;
	else
		myRodsArgs.force = 0;

	char tmpstr[200];
	if(replNum >= 0)
	{
		myRodsArgs.replNum = 1;
		sprintf(tmpstr, "%d", replNum);
		myRodsArgs.replNumValue = strdup(tmpstr);
	}
	else
	{
		myRodsArgs.replNum = 0;
	}

	rodsPathInp_t rodsPathInp;
    memset(&rodsPathInp, 0, sizeof(rodsPathInp_t));

	rodsPathInp.numSrc = 1;
    rodsPathInp.srcPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.srcPath[0]), 0, sizeof(rodsPath_t));
    rodsPathInp.srcPath[0].objType = UNKNOWN_OBJ_T;   // DATA_OBJ_T
    rodsPathInp.srcPath[0].objState = UNKNOWN_ST;
    strcpy(rodsPathInp.srcPath[0].inPath, (char *)LPCTSTR(dataName));
    strcpy(rodsPathInp.srcPath[0].outPath, (char *)LPCTSTR(dataFullPath));

    int t = rmUtil(conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);


	free(rodsPathInp.srcPath);

	return t;
}

int irodsWinRenameOneCollection(rcComm_t *conn, CString & oriCollWholePath, CString & newCollWholePath)
{
	int t;
	dataObjCopyInp_t RenameInp;
	
	memset(&RenameInp, 0, sizeof(dataObjCopyInp_t));
	RenameInp.destDataObjInp.oprType = RENAME_COLL;
	rstrcpy(RenameInp.destDataObjInp.objPath, (char *)LPCTSTR(newCollWholePath), MAX_NAME_LEN);

	RenameInp.srcDataObjInp.oprType = RENAME_COLL;
	rstrcpy(RenameInp.srcDataObjInp.objPath, (char *)LPCTSTR(oriCollWholePath), MAX_NAME_LEN);

	t = rcDataObjRename(conn, &RenameInp);

	return t;
}

int irodsWinRenameOneDataobj(rcComm_t *conn, CString & parColl, CString & oldName, CString newName)
{
	rodsEnv myRodsEnv;
	memset(&myRodsEnv, 0, sizeof(myRodsEnv));
	irodsWinSetRodsEnvViaConn(conn, &myRodsEnv);

	rodsArguments_t myRodsArgs;
	memset(&myRodsArgs, 0, sizeof(rodsArguments_t));

	rodsPathInp_t rodsPathInp;
    memset(&rodsPathInp, 0, sizeof(rodsPathInp_t));

	CString oldFullPath = parColl + "/" + oldName;
	rodsPathInp.numSrc = 1;
    rodsPathInp.srcPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.srcPath[0]), 0, sizeof(rodsPath_t));
    rodsPathInp.srcPath[0].objType = UNKNOWN_OBJ_T;
    rodsPathInp.srcPath[0].objState = UNKNOWN_ST;
    strcpy(rodsPathInp.srcPath[0].inPath, (char *)LPCTSTR(oldFullPath));
    strcpy(rodsPathInp.srcPath[0].outPath, (char *)LPCTSTR(oldFullPath));

	CString newFullPath = parColl + "/" + newName;
    rodsPathInp.destPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.destPath[0]), 0, sizeof(rodsPath_t));
	rodsPathInp.destPath[0].objType = UNKNOWN_OBJ_T;
	rodsPathInp.destPath[0].objState = UNKNOWN_ST;
    strcpy(rodsPathInp.destPath[0].inPath, (char *)LPCTSTR(newFullPath));
    strcpy(rodsPathInp.destPath[0].outPath, (char *)LPCTSTR(newFullPath));
    
    rodsPathInp.targPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.targPath[0]), 0, sizeof(rodsPath_t));

    int t = mvUtil(conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);

	free(rodsPathInp.srcPath);
	free(rodsPathInp.destPath);
	free(rodsPathInp.targPath);

	return t;
}

void irodsWinSetRodsEnvViaConn(rcComm_t *conn, rodsEnv *pRodsEnv)
{
	// user need to take care of "default resource" and "cwd".
	if(pRodsEnv == NULL)
		return;

	strcpy(pRodsEnv->rodsUserName, conn->clientUser.userName);
	strcpy(pRodsEnv->rodsHost, conn->host);
	pRodsEnv->rodsPort = conn->irodsProt;
	char tmpstr[1024];
	sprintf(tmpstr, "/%s/home/%s", conn->clientUser.rodsZone, conn->clientUser.userName);
	strcpy(pRodsEnv->rodsHome, tmpstr);
	strcpy(pRodsEnv->rodsCwd, tmpstr);
	strcpy(pRodsEnv->rodsDefResource, "..");
	strcpy(pRodsEnv->rodsZone, conn->clientUser.rodsZone);
}

int irodsWinDownloadOneObject(rcComm_t *conn, CString irodsObjFullPath, CString & localDir, 
							  int recursive, int forceful)
{
	rodsEnv myRodsEnv;
	memset(&myRodsEnv, 0, sizeof(myRodsEnv));
	irodsWinSetRodsEnvViaConn(conn, &myRodsEnv);

	rodsArguments_t rodsArgs;
	memset(&rodsArgs, 0, sizeof(rodsArgs));
	rodsArgs.recursive = recursive;
	rodsArgs.force =  forceful;

	rodsPathInp_t rodsPathInp;
    memset(&rodsPathInp, 0, sizeof(rodsPathInp_t));

	rodsPathInp.numSrc = 1;
    rodsPathInp.srcPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.srcPath[0]), 0, sizeof(rodsPath_t));
    rodsPathInp.srcPath[0].objType = UNKNOWN_OBJ_T;
    rodsPathInp.srcPath[0].objState = UNKNOWN_ST;
    strcpy(rodsPathInp.srcPath[0].inPath, (char *)LPCTSTR(irodsObjFullPath));
    strcpy(rodsPathInp.srcPath[0].outPath, (char *)LPCTSTR(irodsObjFullPath));

    rodsPathInp.destPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.destPath[0]), 0, sizeof(rodsPath_t));
	rodsPathInp.destPath[0].objType = LOCAL_DIR_T;
	rodsPathInp.destPath[0].objState = EXIST_ST;
    strcpy(rodsPathInp.destPath[0].inPath, (char *)LPCTSTR(localDir));
    strcpy(rodsPathInp.destPath[0].outPath, (char *)LPCTSTR(localDir));
    
    rodsPathInp.targPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.targPath[0]), 0, sizeof(rodsPath_t));

    int t = getUtil(conn, &myRodsEnv, &rodsArgs, &rodsPathInp);

	free(rodsPathInp.srcPath);
	free(rodsPathInp.destPath);
	free(rodsPathInp.targPath);

	return t;
}

// for UNIX path format only
void irodsWinUnixPathGetName(CString & inPath, CString & pname)
{
	int t = inPath.GetLength() - inPath.ReverseFind('/') -1;
	pname = inPath.Right(t);
}

void irodsWinUnixPathGetParent(CString & inPath, CString & parentPath)
{
	int t = inPath.ReverseFind('/');
	parentPath = inPath.Left(t);
}

// works for both UNIX and Windows path format
void irodsWinPathGetName(CString & inPath, CString & pname) 
{
	CString tmpstr = inPath;
	tmpstr.Replace("\\", "/");
	irodsWinUnixPathGetName(tmpstr, pname);
}


int irodsWinUploadOneFile(rcComm_t *conn, CString & irodsRescName, 
						  CString & irodsCollPath, CString & localFilePathName,
						  bool forceful,
						  irodsWinDataobj & newDataObj)
{
	WCHAR unicode_str[2048];
	int n=MultiByteToWideChar(CP_UTF8, 0, (char *)LPCTSTR(localFilePathName), localFilePathName.GetLength(), unicode_str, 2048);

	struct __stat64 mystat;
	if(_stat64((char *)LPCTSTR(localFilePathName), &mystat) != 0)
	{
		return UNABLE_TO_STAT_FILE;
	}
	
	rodsEnv myRodsEnv;
	memset(&myRodsEnv, 0, sizeof(myRodsEnv));
	irodsWinSetRodsEnvViaConn(conn, &myRodsEnv);
	strcpy(myRodsEnv.rodsDefResource, (char *)LPCTSTR(irodsRescName));

	rodsArguments_t myRodsArgs;
	memset(&myRodsArgs, 0, sizeof(rodsArguments_t));
	if(forceful)
		myRodsArgs.force = 1;

	rodsPathInp_t rodsPathInp;
    memset(&rodsPathInp, 0, sizeof(rodsPathInp_t));

	rodsPathInp.numSrc = 1;
    rodsPathInp.srcPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.srcPath[0]), 0, sizeof(rodsPath_t));
    rodsPathInp.srcPath[0].objType = LOCAL_FILE_T;
    rodsPathInp.srcPath[0].objState = EXIST_ST;
    rodsPathInp.srcPath[0].size = mystat.st_size;
    strcpy(rodsPathInp.srcPath[0].inPath, (char *)LPCTSTR(localFilePathName));
    strcpy(rodsPathInp.srcPath[0].outPath, (char *)LPCTSTR(localFilePathName));

    rodsPathInp.destPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.destPath[0]), 0, sizeof(rodsPath_t));
    strcpy(rodsPathInp.destPath[0].inPath, (char *)LPCTSTR(irodsCollPath));
    strcpy(rodsPathInp.destPath[0].outPath, (char *)LPCTSTR(irodsCollPath));
    
    rodsPathInp.targPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.targPath[0]), 0, sizeof(rodsPath_t));

    int t = putUtil(&conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);

	free(rodsPathInp.srcPath);
	free(rodsPathInp.destPath);
	free(rodsPathInp.targPath);

	CString pname;
	irodsWinPathGetName(localFilePathName, pname);
	newDataObj.name = pname;
	newDataObj.owner = myRodsEnv.rodsUserName;
	newDataObj.replNum = 0;
	newDataObj.size = mystat.st_size;
	newDataObj.rescName = irodsRescName;
	newDataObj.parentCollectionFullPath = irodsCollPath;

    return t;
}

//move one at a time. It seems that moving multiple objects is not working.
int irodsWinMoveObjects(rcComm_t *conn, CString & destColl, CStringArray & irodsObjsFullPath)
{
	rodsEnv myRodsEnv;
	memset(&myRodsEnv, 0, sizeof(myRodsEnv));
	irodsWinSetRodsEnvViaConn(conn, &myRodsEnv);

	rodsArguments_t myRodsArgs;
	memset(&myRodsArgs, 0, sizeof(rodsArguments_t));
	myRodsArgs.recursive = 1;

	int n = irodsObjsFullPath.GetSize();
	rodsPathInp_t rodsPathInp;
    memset(&rodsPathInp, 0, sizeof(rodsPathInp_t));
	rodsPathInp.numSrc = n;
	rodsPathInp.srcPath = (rodsPath_t *)calloc(n, sizeof(rodsPath_t));
	for(int i=0;i<n;i++)
	{
		memset(&(rodsPathInp.srcPath[i]), 0, sizeof(rodsPath_t));
		rodsPathInp.srcPath[i].objType = UNKNOWN_OBJ_T;
		rodsPathInp.srcPath[i].objState = UNKNOWN_ST;
		strcpy(rodsPathInp.srcPath[i].inPath, (char *)LPCTSTR(irodsObjsFullPath[i]));
		strcpy(rodsPathInp.srcPath[i].outPath, (char *)LPCTSTR(irodsObjsFullPath[i]));
	}

	rodsPathInp.destPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.destPath[0]), 0, sizeof(rodsPath_t));
    strcpy(rodsPathInp.destPath[0].inPath, (char *)LPCTSTR(destColl));
    strcpy(rodsPathInp.destPath[0].outPath, (char *)LPCTSTR(destColl));
    
    rodsPathInp.targPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.targPath[0]), 0, sizeof(rodsPath_t));

	int t = mvUtil(conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);

	free(myRodsArgs.resourceString);
	free(rodsPathInp.srcPath);
	free(rodsPathInp.destPath);
	free(rodsPathInp.targPath);

	return t;
}

// it is always recursive
int irodsWinPasteObjects(rcComm_t *conn, CString & irodsRescName,
			CString & irodsCollPathPasteTo,
			CStringArray & irodsObjsFullPath, bool forceful)
{
	rodsEnv myRodsEnv;
	memset(&myRodsEnv, 0, sizeof(myRodsEnv));
	irodsWinSetRodsEnvViaConn(conn, &myRodsEnv);
	strcpy(myRodsEnv.rodsDefResource, (char *)LPCTSTR(irodsRescName));

	rodsArguments_t myRodsArgs;
	memset(&myRodsArgs, 0, sizeof(rodsArguments_t));
	myRodsArgs.recursive = 1;
	myRodsArgs.resource = 1;
	myRodsArgs.resourceString = (char *)strdup((char *)LPCTSTR(irodsRescName));
	if(forceful)
		myRodsArgs.force = 1;

	int n = irodsObjsFullPath.GetSize();
	rodsPathInp_t rodsPathInp;
    memset(&rodsPathInp, 0, sizeof(rodsPathInp_t));
	rodsPathInp.numSrc = n;
	rodsPathInp.srcPath = (rodsPath_t *)calloc(n, sizeof(rodsPath_t));
	for(int i=0;i<n;i++)
	{
		memset(&(rodsPathInp.srcPath[i]), 0, sizeof(rodsPath_t));
		rodsPathInp.srcPath[i].objType = UNKNOWN_OBJ_T;
		rodsPathInp.srcPath[i].objState = UNKNOWN_ST;
		strcpy(rodsPathInp.srcPath[0].inPath, (char *)LPCTSTR(irodsObjsFullPath[i]));
		strcpy(rodsPathInp.srcPath[0].outPath, (char *)LPCTSTR(irodsObjsFullPath[i]));
	}

	rodsPathInp.destPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.destPath[0]), 0, sizeof(rodsPath_t));
    strcpy(rodsPathInp.destPath[0].inPath, (char *)LPCTSTR(irodsCollPathPasteTo));
    strcpy(rodsPathInp.destPath[0].outPath, (char *)LPCTSTR(irodsCollPathPasteTo));
    
    rodsPathInp.targPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.targPath[0]), 0, sizeof(rodsPath_t));

	int t = cpUtil(conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);

	free(myRodsArgs.resourceString);
	free(rodsPathInp.srcPath);
	free(rodsPathInp.destPath);
	free(rodsPathInp.targPath);

	return t;
}

int irodsWinUploadOneFolder(rcComm_t *conn, CString & irodsRescName, 
						  CString & irodsCollPath, CString & localFolderPathName,
						  bool forceful,
						  irodsWinCollection & newChildColl)
{
	rodsEnv myRodsEnv;
	memset(&myRodsEnv, 0, sizeof(myRodsEnv));
	irodsWinSetRodsEnvViaConn(conn, &myRodsEnv);
	strcpy(myRodsEnv.rodsDefResource, (char *)LPCTSTR(irodsRescName));

	rodsArguments_t myRodsArgs;
	memset(&myRodsArgs, 0, sizeof(rodsArguments_t));
	myRodsArgs.recursive = 1;
	if(forceful)
		myRodsArgs.force = 1;

	rodsPathInp_t rodsPathInp;
    memset(&rodsPathInp, 0, sizeof(rodsPathInp_t));

	rodsPathInp.numSrc = 1;
    rodsPathInp.srcPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.srcPath[0]), 0, sizeof(rodsPath_t));
    rodsPathInp.srcPath[0].objType = LOCAL_DIR_T;
    rodsPathInp.srcPath[0].objState = EXIST_ST;
    rodsPathInp.srcPath[0].size = 0;
    strcpy(rodsPathInp.srcPath[0].inPath, (char *)LPCTSTR(localFolderPathName));
    strcpy(rodsPathInp.srcPath[0].outPath, (char *)LPCTSTR(localFolderPathName));

    rodsPathInp.destPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.destPath[0]), 0, sizeof(rodsPath_t));
    strcpy(rodsPathInp.destPath[0].inPath, ".");
    strcpy(rodsPathInp.destPath[0].outPath, (char *)LPCTSTR(irodsCollPath));
    
    rodsPathInp.targPath = (rodsPath_t *)calloc(1, sizeof(rodsPath_t));
    memset(&(rodsPathInp.targPath[0]), 0, sizeof(rodsPath_t));

    int t = putUtil(&conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);

	free(rodsPathInp.srcPath);
	free(rodsPathInp.destPath);
	free(rodsPathInp.targPath);

	CString pname;
	irodsWinPathGetName(localFolderPathName, pname);
	newChildColl.name = pname;
	newChildColl.fullPath = irodsCollPath + "/" + pname;
	newChildColl.owner = myRodsEnv.rodsUserName;
	newChildColl.queried = false;
	
	return t;
}

int irodsWinGetResourceNames(rcComm_t *conn, CStringArray & irodsRescNames)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut = NULL;
	int i1a[20];
	int i1b[20]={0,0,0,0,0,0,0,0,0,0,0,0};
	int i2a[20];
	char *condVal[10];

	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	int i=0;
	i1a[i++]=COL_R_RESC_NAME;

	genQueryInp.selectInp.inx = i1a;
	genQueryInp.selectInp.value = i1b;
	genQueryInp.selectInp.len = i;

	genQueryInp.sqlCondInp.inx = i2a;
	genQueryInp.sqlCondInp.value = condVal;
	genQueryInp.sqlCondInp.len=0;

	int total_resc_cnt = 0;
	genQueryInp.maxRows=100;
	genQueryInp.continueInx=0;
	int t = 0;
	bool continue_query = true;
	while(continue_query)
	{
		t = rcGenQuery(conn, &genQueryInp, &genQueryOut);
		if(t != CAT_NO_ROWS_FOUND)
		{
			for(int j=0;j<genQueryOut->rowCnt;j++)
			{
				CString tmpname;
				tmpname = genQueryOut->sqlResult[0].value + j*genQueryOut->sqlResult[0].len;
				irodsRescNames.Add(tmpname);
				total_resc_cnt++;
			}
		}
		else
		{
			return total_resc_cnt;
		}
		if((t < 0) || (genQueryInp.continueInx<=0))
			continue_query = false;
	}

	freeGenQueryOut(&genQueryOut);

	return total_resc_cnt;
}

int irodsWinGetGroupResourceNames(rcComm_t *conn, CStringArray & irodsGrpRescNames)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut = NULL;

	memset(&genQueryInp, 0, sizeof(genQueryInp));

	char grpRescCond[MAX_NAME_LEN];
	strcpy(grpRescCond, "like '%'");
	addInxVal (&(genQueryInp.sqlCondInp), COL_RESC_GROUP_NAME, grpRescCond);

	addInxIval (&(genQueryInp.selectInp), COL_RESC_GROUP_NAME, 1);

	genQueryInp.maxRows = 100;
	genQueryInp.continueInx = 0;
	int t;
	int total_resc_cnt = 0;
	bool continue_query = true;
	while(continue_query)
	{
		t = rcGenQuery(conn, &genQueryInp, &genQueryOut);
		if(t != CAT_NO_ROWS_FOUND)
		{
			for(int j=0;j<genQueryOut->rowCnt;j++)
			{
				CString tmpname;
				tmpname = genQueryOut->sqlResult[0].value + j*genQueryOut->sqlResult[0].len;
				irodsGrpRescNames.Add(tmpname);
				total_resc_cnt++;
			}
		}
		else
		{
			return total_resc_cnt;
		}
		if((t < 0) || (genQueryInp.continueInx<=0))
			continue_query = false;
	}

	freeGenQueryOut(&genQueryOut);

	return total_resc_cnt;
}

int irodsWinGetRescInfo(rcComm_t *conn, const CString & resc_name, iRODSResourceInfo & resc_info)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut;

	resc_info.is_group_resc = false;
	resc_info.name = resc_name;

	// assume it is a group resc
	memset(&genQueryInp, 0, sizeof(genQueryInp));

	char resc_cond[MAX_NAME_LEN];
	sprintf(resc_cond, "= '%s'", resc_name);
	addInxVal (&(genQueryInp.sqlCondInp), COL_RESC_GROUP_NAME, resc_cond);

	addInxIval (&(genQueryInp.selectInp), COL_R_RESC_NAME, 1);

	genQueryInp.maxRows = 100;
	genQueryInp.continueInx = 0;
	int t = rcGenQuery(conn, &genQueryInp, &genQueryOut);
	if((t < 0)&&(t != CAT_NO_ROWS_FOUND))
		return t;
	
	int i, j;
	CString tmpstr;
	if(t!=CAT_NO_ROWS_FOUND)
	{
		resc_info.is_group_resc = true;
		for(j=0;j<genQueryOut->rowCnt;j++)
		{
			tmpstr = genQueryOut->sqlResult[0].value + j*genQueryOut->sqlResult[0].len;
			resc_info.sub_rescs.Add(tmpstr);
		}
		return 0;
	}

	// otheriwse, treat it as a physical resource
	memset(&genQueryInp, 0, sizeof(genQueryInp));

	sprintf(resc_cond, "= '%s'", resc_name);
	addInxVal (&(genQueryInp.sqlCondInp), COL_R_RESC_NAME, resc_cond);

	int query_sels[] = {COL_R_ZONE_NAME, COL_R_TYPE_NAME, COL_R_CLASS_NAME,
				COL_R_LOC, COL_R_VAULT_PATH, COL_R_FREE_SPACE, COL_R_RESC_STATUS,
				COL_R_RESC_INFO, COL_R_RESC_COMMENT, COL_R_CREATE_TIME, COL_R_MODIFY_TIME,
				COL_R_RESC_ID}; 
	int num_query_sels = 12;
	for(i=0;i<num_query_sels;i++)
		addInxIval (&(genQueryInp.selectInp), query_sels[i], 1);
	
	genQueryInp.maxRows = 5;
	genQueryInp.continueInx = 0;
	t = rcGenQuery(conn, &genQueryInp, &genQueryOut);
	if((t < 0)&&(t != CAT_NO_ROWS_FOUND))
		return t;

	if(t == CAT_NO_ROWS_FOUND)
	{
		return 0;
	}

	char localTime[20];
	resc_info.zone = genQueryOut->sqlResult[0].value;
	resc_info.type = genQueryOut->sqlResult[1].value;
	resc_info.resc_class = genQueryOut->sqlResult[2].value;
	resc_info.location = genQueryOut->sqlResult[3].value;
	resc_info.vault_path = genQueryOut->sqlResult[4].value;
	resc_info.free_space = genQueryOut->sqlResult[5].value;
	resc_info.status = genQueryOut->sqlResult[6].value;
	resc_info.info = genQueryOut->sqlResult[7].value;
	resc_info.comment = genQueryOut->sqlResult[8].value;
	getLocalTimeFromRodsTime(genQueryOut->sqlResult[9].value, localTime);
	resc_info.create_time = localTime;
	getLocalTimeFromRodsTime(genQueryOut->sqlResult[10].value, localTime);
	resc_info.modify_time = localTime;
	resc_info.id = genQueryOut->sqlResult[11].value;

	freeGenQueryOut(&genQueryOut);

	return 0;
}

int irodsWinGetObjSysMetadata(rcComm_t *conn, CString parent_coll, CString obj_name, iRODSObjSysMetadata & obj_sysmeta)
{
	obj_sysmeta.parent_coll = parent_coll;
	obj_sysmeta.obj_name = obj_name;

	CArray<int, int> query_fields;
	CArray<int, int> cond_fields;
	CStringArray cond_values;

	int num_cond_fields;

	cond_fields.Add(COL_COLL_NAME);		cond_values.Add(parent_coll);
	cond_fields.Add(COL_DATA_NAME);		cond_values.Add(obj_name);
	num_cond_fields = 2;

	int num_query_fields = 0;
	query_fields.Add(COL_DATA_REPL_NUM); num_query_fields++;    // 0 
	query_fields.Add(COL_DATA_VERSION); num_query_fields++;     // 1
	query_fields.Add(COL_DATA_SIZE); num_query_fields++;        // 2
	query_fields.Add(COL_D_RESC_GROUP_NAME); num_query_fields++;  // 3
	query_fields.Add(COL_D_RESC_NAME); num_query_fields++;   // 4
	query_fields.Add(COL_D_DATA_PATH); num_query_fields++;   // 5
	query_fields.Add(COL_D_OWNER_NAME); num_query_fields++;  // 6
	query_fields.Add(COL_D_OWNER_ZONE); num_query_fields++;  // 7
	query_fields.Add(COL_D_REPL_STATUS); num_query_fields++;  // 8
	query_fields.Add(COL_D_DATA_STATUS); num_query_fields++;  // 9
	query_fields.Add(COL_D_DATA_CHECKSUM); num_query_fields++;  //10
	query_fields.Add(COL_D_EXPIRY); num_query_fields++;  // 11
	query_fields.Add(COL_D_COMMENTS); num_query_fields++;  // 12
	query_fields.Add(COL_D_CREATE_TIME); num_query_fields++;  // 13
	query_fields.Add(COL_D_MODIFY_TIME); num_query_fields++;  // 14
	query_fields.Add(COL_D_DATA_ID); num_query_fields++; // 15
	query_fields.Add(COL_R_LOC); num_query_fields++; // 16

	genQueryInp_t *pGenQueryInp = irodsWinCreateGenericQueryInp(query_fields, cond_fields, cond_values);

	if(pGenQueryInp == NULL)
		return -1;

	genQueryOut_t *pGenQueryOut = NULL;
	int t;
	CString attname, attval, attunit;
	pGenQueryInp->maxRows = 50;
	pGenQueryInp->continueInx = 0;
	bool continue_fetch = true;
	
	// we assume that number of replicas would not exceed 50.
	t = rcGenQuery(conn, pGenQueryInp, &pGenQueryOut);
	if(t == CAT_NO_ROWS_FOUND)
	{
		return 0;
	}

	if(t < 0)
		return t;

	CString tmpstr;
	char *pTmpStr;
	char localTime[20];
	int j;
	for (int i=0;i<pGenQueryOut->rowCnt;i++)
	{
		j = 0;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.repl_num.Add(tmpstr);

		j = 1;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.version = tmpstr;

		j = 2;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.size = tmpstr;

		j = 3;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.repl_resc_grp_name.Add(tmpstr);

		j = 4;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.repl_resc_name.Add(tmpstr);

		j = 5;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.repl_vault_path.Add(tmpstr);

		j = 6;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.owner = tmpstr;

		j = 7;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.zone = tmpstr;

		j = 8;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.repl_status.Add(tmpstr);

		j = 9;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.repl_status.Add(tmpstr);

		j = 10;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.checksum = tmpstr;

		j = 11;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.expiry = tmpstr;

		j = 12;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.comments = tmpstr;

		j = 13;
		pTmpStr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		getLocalTimeFromRodsTime(pTmpStr, localTime); pTmpStr = NULL;
		tmpstr = localTime;
		obj_sysmeta.repl_create_time.Add(tmpstr);

		j = 14;
		pTmpStr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		getLocalTimeFromRodsTime(pTmpStr, localTime); pTmpStr = NULL;
		tmpstr = localTime;
		obj_sysmeta.repl_modify_time.Add(tmpstr);

		j = 15;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.obj_id = tmpstr;

		j = 16;
		tmpstr = pGenQueryOut->sqlResult[j].value + i*pGenQueryOut->sqlResult[j].len;
		obj_sysmeta.repl_resc_host_name.Add(tmpstr);
	}

	freeGenQueryOut(&pGenQueryOut);
	irodsWinGenericQueryInPFree(num_query_fields, num_cond_fields, pGenQueryInp);
	return 0;
}

void irodsWinGenericQueryInPFree(int num_of_query_fields, int num_of_cond_fields,
								genQueryInp_t *pGenQueryInp)
{
	if(pGenQueryInp == NULL)
		return;

	free(pGenQueryInp->selectInp.inx);
	pGenQueryInp->selectInp.inx = NULL;
	free(pGenQueryInp->selectInp.value);
	pGenQueryInp->selectInp.value = NULL;

	free(pGenQueryInp->sqlCondInp.inx);
	pGenQueryInp->sqlCondInp.inx = NULL;
	for(int i=0;i<num_of_cond_fields;i++)
	{
		free(pGenQueryInp->sqlCondInp.value[i]);
		pGenQueryInp->sqlCondInp.value[i] = NULL;
	}
	free(pGenQueryInp->sqlCondInp.value);
	pGenQueryInp->sqlCondInp.value = NULL;
	free(pGenQueryInp);
}

// used for small queyr. It is not suitable for a query with large amout of retrns, espcially haivng pages of data
genQueryInp_t *irodsWinCreateGenericQueryInp(CArray<int, int> & queryFields, 
											 CArray<int, int> & condFields, CStringArray & condValues)
{
	genQueryInp_t *pGenQueryInp;
	
	pGenQueryInp = (genQueryInp_t *)calloc(1, sizeof(genQueryInp_t));
	if(pGenQueryInp == NULL)
		return NULL;

	memset(pGenQueryInp, 0, sizeof(genQueryInp_t));

	int n = queryFields.GetSize();
	int *i1a = (int *)calloc(n, sizeof(int));
	int i = 0;
	for(i=0;i<n;i++)
	{
		i1a[i] = queryFields[i];
	}
	int *i1b = (int *)calloc(n, sizeof(int));
	for(i=0;i<n;i++)
		i1b[i] = 0;
	pGenQueryInp->selectInp.inx = i1a;
	pGenQueryInp->selectInp.value = i1b;
	pGenQueryInp->selectInp.len = n;

	i1a = NULL;
	i1b = NULL;

	n = condFields.GetSize();
	int *i2a = (int *)calloc(n, sizeof(int));
	char **condVales = (char **)calloc(n, sizeof(char **));
	char tmpstr[2048];
	for(i=0;i<n;i++)
	{
		i2a[i] = condFields[i];
		sprintf(tmpstr, "='%s'", (char *)LPCTSTR(condValues[i]));
		condVales[i] = strdup(tmpstr);
	}

	pGenQueryInp->sqlCondInp.inx = i2a;
	pGenQueryInp->sqlCondInp.value = condVales;
	pGenQueryInp->sqlCondInp.len = n;

	i2a = NULL;
	condVales = NULL;
	
	return pGenQueryInp;
}

int irodsWinGetUserInfo(rcComm_t *conn, CString & userName, iRODSUserInfo & userInfo)
{
	genQueryInp_t genQueryInp;
	genQueryOut_t *genQueryOut=NULL;

	memset (&genQueryInp, 0, sizeof (genQueryInp_t));

	char condstr[MAX_NAME_LEN];
	snprintf(condstr, MAX_NAME_LEN, "= '%s'",  (char *)LPCTSTR(userName));
	addInxVal (&(genQueryInp.sqlCondInp), COL_USER_NAME, condstr);

	addInxIval (&(genQueryInp.selectInp), COL_USER_NAME, 1);    // 0
	addInxIval (&(genQueryInp.selectInp), COL_USER_ID, 1);      // 1
	addInxIval (&(genQueryInp.selectInp), COL_USER_TYPE, 1);    // 2
	addInxIval (&(genQueryInp.selectInp), COL_USER_ZONE, 1);    // 3
	//addInxIval (&(genQueryInp.selectInp), COL_USER_DN, 1);      // 4
	addInxIval (&(genQueryInp.selectInp), COL_USER_INFO, 1);    // 5
	addInxIval (&(genQueryInp.selectInp), COL_USER_COMMENT, 1);  // 6
	addInxIval (&(genQueryInp.selectInp), COL_USER_CREATE_TIME, 1);  // 7
	addInxIval (&(genQueryInp.selectInp), COL_USER_MODIFY_TIME, 1);  // 8

	genQueryInp.maxRows = 50;
	genQueryInp.continueInx = 0;
	int t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
	if(t == CAT_NO_ROWS_FOUND)
	{
		return 0;
	}
	else if(t < 0)
	{
		return t;
	}

	char localTime[20];

	sqlResult_t *uname, *uid, *utype, *uzone, *uinfo, *ucomment, *uctime, *umtime;

	if ((uname = getSqlResultByInx (genQueryOut, COL_USER_NAME))== NULL) 
	{
			return (UNMATCHED_KEY_OR_INDEX);
	}
	if ((uid = getSqlResultByInx (genQueryOut, COL_USER_ID))== NULL) 
	{
			return (UNMATCHED_KEY_OR_INDEX);
	}
	if ((utype = getSqlResultByInx (genQueryOut, COL_USER_TYPE))== NULL) 
	{
			return (UNMATCHED_KEY_OR_INDEX);
	}
	if ((uzone = getSqlResultByInx (genQueryOut, COL_USER_ZONE))== NULL) 
	{
			return (UNMATCHED_KEY_OR_INDEX);
	}
	if ((uinfo = getSqlResultByInx (genQueryOut, COL_USER_INFO))== NULL) 
	{
			return (UNMATCHED_KEY_OR_INDEX);
	}
	if ((ucomment = getSqlResultByInx (genQueryOut, COL_USER_COMMENT))== NULL) 
	{
			return (UNMATCHED_KEY_OR_INDEX);
	}
	if ((uctime = getSqlResultByInx (genQueryOut, COL_USER_CREATE_TIME))== NULL) 
	{
			return (UNMATCHED_KEY_OR_INDEX);
	}
	if ((umtime = getSqlResultByInx (genQueryOut, COL_USER_MODIFY_TIME))== NULL) 
	{
			return (UNMATCHED_KEY_OR_INDEX);
	}
	userInfo.name = userName;
	userInfo.id = &uid->value[0];
	userInfo.type = &utype->value[0];
	userInfo.zone = &uzone->value[0];
	//userInfo.DN = &udn->value[0];
	userInfo.info = &uinfo->value[0];
	userInfo.comment = &ucomment->value[0];
	getLocalTimeFromRodsTime(&uctime->value[0], localTime);
	userInfo.create_time = CString(localTime);
	getLocalTimeFromRodsTime(&umtime->value[0], localTime);
	userInfo.modify_time = CString(localTime);

	// get group info
	memset (&genQueryInp, 0, sizeof (genQueryInp_t));
	snprintf(condstr, MAX_NAME_LEN, "= '%s'",  (char *)LPCTSTR(userName));
	addInxVal (&(genQueryInp.sqlCondInp), COL_USER_NAME, condstr);

	addInxIval (&(genQueryInp.selectInp), COL_USER_GROUP_NAME, 1);

	genQueryInp.maxRows = 200;
	genQueryInp.continueInx = 0;
	t = rcGenQuery (conn, &genQueryInp, &genQueryOut);
	if(t == CAT_NO_ROWS_FOUND)
	{
		return 0;
	}
	else if(t < 0)
	{
		return t;
	}
	CString tmpstr;
	sqlResult_t *ugroups;
	if ((ugroups = getSqlResultByInx (genQueryOut, COL_USER_GROUP_NAME))== NULL) 
	{
			return (UNMATCHED_KEY_OR_INDEX);
	}
	for(int i=0;i<genQueryOut->rowCnt; i++)
	{
		tmpstr = &ugroups->value[ugroups->len * i];
		userInfo.groups.Add(tmpstr);
	}

	freeGenQueryOut(&genQueryOut);
	return 0;
}

// Objects can be collection (recursive) or data set.
int irodsWinReplicateObjects(rcComm_t *conn, CStringArray & irodsCollOrObjPaths, CString & irodsRescName)
{
	rodsEnv myRodsEnv;
	memset(&myRodsEnv, 0, sizeof(myRodsEnv));
	irodsWinSetRodsEnvViaConn(conn, &myRodsEnv);
	strcpy(myRodsEnv.rodsDefResource, (char *)LPCTSTR(irodsRescName));

	rodsArguments_t myRodsArgs;
	memset(&myRodsArgs, 0, sizeof(rodsArguments_t));
	myRodsArgs.recursive = 1;
	myRodsArgs.resource = 1;
	myRodsArgs.resourceString = (char *)strdup((char *)LPCTSTR(irodsRescName));

	int n = irodsCollOrObjPaths.GetSize();
	rodsPathInp_t rodsPathInp;
    memset(&rodsPathInp, 0, sizeof(rodsPathInp_t));
	rodsPathInp.numSrc = n;
	rodsPathInp.srcPath = (rodsPath_t *)calloc(n, sizeof(rodsPath_t));
	for(int i=0;i<n;i++)
	{
		memset(&(rodsPathInp.srcPath[i]), 0, sizeof(rodsPath_t));
		rodsPathInp.srcPath[i].objType = UNKNOWN_OBJ_T;
		rodsPathInp.srcPath[i].objState = UNKNOWN_ST;
		strcpy(rodsPathInp.srcPath[0].inPath, (char *)LPCTSTR(irodsCollOrObjPaths[i]));
		strcpy(rodsPathInp.srcPath[0].outPath, (char *)LPCTSTR(irodsCollOrObjPaths[i]));
	}

	int t = replUtil(conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);

	free(myRodsArgs.resourceString);
	free(rodsPathInp.srcPath);

	return t;
}

// check if the param #1 is the Child of param #2
bool irodsWinIsDescendant(CString & p1, CString & p2)
{
	if(p1.GetLength() <= p2.GetLength())
		return false;

	int t = p2.GetLength();
	
	CString tmpstr = p1.Left(t);
	if(tmpstr == p2)
		return true;

	return false;
}

int irodsWinSetAccessControl(rcComm_t *conn, CString & irodsZone, CString irodsUserName, 
							 CString & irodsPermission, 
							 CString & irodsObjFullPath, bool recursive)
{
	modAccessControlInp_t modAccessControl;

	modAccessControl.recursiveFlag = (int)recursive;
	modAccessControl.accessLevel = (char *)strdup((char *)LPCTSTR(irodsPermission));
	modAccessControl.userName = (char *)strdup((char *)LPCTSTR(irodsUserName));
	modAccessControl.zone = (char *)strdup((char *)LPCTSTR(irodsZone));
	modAccessControl.path = (char *)strdup((char *)LPCTSTR(irodsObjFullPath));

	int t = rcModAccessControl(conn, &modAccessControl);

	free(modAccessControl.accessLevel);
	free(modAccessControl.userName);
	free(modAccessControl.zone);
	free(modAccessControl.path);

	return t;
}

int irodsWinGetObjIdByName(rcComm_t *conn, CString & objFulPath, CString & objId)
{
	rodsPath_t myPath;

	memset(&myPath, 0, sizeof(myPath));
	strcpy(myPath.outPath, (char *)LPCTSTR(objFulPath));
	int t = getRodsObjType(conn, &myPath);
	if(t < 0)
		return t;

	objId = myPath.dataId;
	return 0;
}

int irodsWinGetCollectionAccessControl(rcComm_t *conn, CString & collFullPath,
					CStringArray & irodsUsers, CStringArray & irodsZones, CStringArray & irodsConstraints)
{
	genQueryOut_t *genQueryOut = NULL;
	int t = queryCollAcl (conn, (char *)LPCTSTR(collFullPath), &genQueryOut);

	CString user, uperm, uzone, tmpstr;
	char *user_id;
	sqlResult_t *userName, *userZone, *dataAccess;

	/* if((userName = getSqlResultByInx (genQueryOut, COL_COLL_ACCESS_USER_ID)) == NULL) */
	if((userName = getSqlResultByInx (genQueryOut, COL_COLL_USER_NAME)) == NULL)
		return UNMATCHED_KEY_OR_INDEX;

	if ((userZone = getSqlResultByInx (genQueryOut, COL_COLL_USER_ZONE)) == NULL) 
        return (UNMATCHED_KEY_OR_INDEX);

	if((dataAccess = getSqlResultByInx (genQueryOut, COL_COLL_ACCESS_NAME)) == NULL)
		return UNMATCHED_KEY_OR_INDEX;

	 for (int i = 0; i < genQueryOut->rowCnt; i++) 
	 {
        /* 
		user_id = &userName->value[userName->len * i];
		user = useridToName(conn, user_id);
		*/

		user = &userName->value[userName->len * i];
        uzone = &userZone->value[userZone->len * i];
        uperm = &dataAccess->value[dataAccess->len * i];

        tmpstr = &dataAccess->value[dataAccess->len * i];
		if(tmpstr == "read object")
			uperm = "read";
		else if(tmpstr == "modify object")
			uperm = "write";
		else 
			uperm = tmpstr;

        irodsUsers.Add(user);
		irodsZones.Add(uzone);
		irodsConstraints.Add(uperm);
	 }

	 freeGenQueryOut(&genQueryOut);

	return 0;
}

int irodsWinGetDatasetAccessControl(rcComm_t *conn, CString & dataObjFullPath, 
				CStringArray & irodsUsers, CStringArray & irodsConstraints)
{
	CString objId;
	int t = irodsWinGetObjIdByName(conn, dataObjFullPath, objId);
	if(t < 0)
		return t;

	genQueryOut_t *genQueryOut = NULL;
	sqlResult_t *userName, *dataAccess;
	CString userNameStr, dataAccessStr, tmpstr;

	t = queryDataObjAcl (conn, (char *)LPCTSTR(objId), &genQueryOut);
	if(t < 0)
		return t;

	if ((userName = getSqlResultByInx (genQueryOut, COL_USER_NAME)) == NULL) 
		return (UNMATCHED_KEY_OR_INDEX);

   if ((dataAccess = getSqlResultByInx (genQueryOut, COL_DATA_ACCESS_NAME))== NULL) 
       return (UNMATCHED_KEY_OR_INDEX);

   for (int i = 0; i < genQueryOut->rowCnt; i++) 
   {
		userNameStr = &userName->value[userName->len * i];
		tmpstr = &dataAccess->value[dataAccess->len * i];
		if(tmpstr == "read object")
			dataAccessStr = "read";
		else if(tmpstr == "modify object")
			dataAccessStr = "write";
		else 
			dataAccessStr = tmpstr;
       
		irodsUsers.Add(userNameStr);
		irodsConstraints.Add(dataAccessStr);
   }

   freeGenQueryOut (&genQueryOut);

   return 0;
}

int irodsWinGetUsersByZone(rcComm_t *conn, CString & irodsZone, CStringArray & irodsUsers)
{
	CString parentCollFullPath = "/" + irodsZone + "/home";
	CArray<irodsWinCollection, irodsWinCollection> childCollections; 
	int t = irodsWinGetChildCollections(conn, parentCollFullPath, childCollections);
	if(t < 0)
		return t;

	for(int i=0;i<childCollections.GetSize();i++)
	{
		irodsUsers.Add(childCollections[i].name);
	}

	return 0;
}

int irodsWinGetObjUserDefinedMetadata(rcComm_t *conn, 
		CString & objNameWithPath, int objType,
		CStringArray & metaAtts, CStringArray & metaVals, CStringArray & metaUnits)
{
	CArray<int, int> query_fields;
	CArray<int, int> cond_fields;
	CStringArray cond_values;

	int num_query_diields;
	int num_cond_fields;

	if(objType == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		query_fields.Add(COL_META_COLL_ATTR_NAME);        // 0
		query_fields.Add(COL_META_COLL_ATTR_VALUE);       // 1
		query_fields.Add(COL_META_COLL_ATTR_UNITS);       // 2
		num_query_diields = 3;
	}
	else if(objType == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		query_fields.Add(COL_META_DATA_ATTR_NAME);        // 0
		query_fields.Add(COL_META_DATA_ATTR_VALUE);       // 1
		query_fields.Add(COL_META_DATA_ATTR_UNITS);       // 2
		num_query_diields = 3;
	}
	

	if(objType == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		cond_fields.Add(COL_COLL_NAME);		cond_values.Add(objNameWithPath);
		num_cond_fields = 1;
	}
	else if(objType == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		CString obj_name, parent_path;
		irodsWinUnixPathGetName(objNameWithPath, obj_name);
		irodsWinUnixPathGetParent(objNameWithPath, parent_path);
		
		cond_fields.Add(COL_COLL_NAME);		cond_values.Add(parent_path);
		cond_fields.Add(COL_DATA_NAME);		cond_values.Add(obj_name);
		num_cond_fields = 2;
	}
	else
	{
		return -1;
	}

	genQueryInp_t *pGenQueryInp = irodsWinCreateGenericQueryInp(query_fields, cond_fields, cond_values);
	if(pGenQueryInp == NULL)
		return -1; 

	genQueryOut_t *pGenQueryOut = NULL;

	int t;
	CString attname, attval, attunit;
	pGenQueryInp->maxRows = 50;
	pGenQueryInp->continueInx = 0;
	bool continue_fetch = true;
	while(continue_fetch)
	{
		t = rcGenQuery(conn, pGenQueryInp, &pGenQueryOut);
		if(t == CAT_NO_ROWS_FOUND)
		{
			return 0;
		}

		if(t < 0)
			return t;

		// get data
		for (int i=0;i<pGenQueryOut->rowCnt;i++)
		{
			attname = pGenQueryOut->sqlResult[0].value + i*pGenQueryOut->sqlResult[0].len;   // att name
			attval = pGenQueryOut->sqlResult[1].value + i*pGenQueryOut->sqlResult[1].len;   // att value
			attunit = pGenQueryOut->sqlResult[2].value + i*pGenQueryOut->sqlResult[2].len;

			metaAtts.Add(attname);
			metaVals.Add(attval);
			metaUnits.Add(attunit);

		}

		if((t!=0)||(pGenQueryOut->continueInx <= 0))
		{
			continue_fetch = false;
		}
	}

	freeGenQueryOut(&pGenQueryOut);
	irodsWinGenericQueryInPFree(num_query_diields, num_cond_fields, pGenQueryInp);

	return 0;
}

int irodsWinDeleteMetadata(rcComm_t *conn, CString & objFulleName, int objType,
			   CString & attName, CString & attValues, CString & attUnit)
{
	modAVUMetadataInp_t modAVUMetadataInp;
	char arg0[3];
	char arg1[3];
	char arg2[2048];   // full obj name
	char arg3[1024];   // att name
 	char arg4[2048];   // att value
	char arg5[1024];   // att unit

	strcpy(arg0, "rm");
	if(objType == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		strcpy(arg1, "-c");
	}
	else if(objType == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		strcpy(arg1, "-d");
	}
	else
	{
		return -1;
	}
	strcpy(arg2, (char *)LPCTSTR(objFulleName));
	strcpy(arg3, (char *)LPCTSTR(attName));
	strcpy(arg4, (char *)LPCTSTR(attValues));
	if(attUnit.GetLength() > 0)
	{
		strcpy(arg5, (char *)LPCTSTR(attUnit));
	}
	else
	{
		arg5[0] = '\0';
	}

	modAVUMetadataInp.arg0 = arg0;
	modAVUMetadataInp.arg1 = arg1;
	modAVUMetadataInp.arg2 = arg2;
	modAVUMetadataInp.arg3 = arg3;
	modAVUMetadataInp.arg4 = arg4;
	modAVUMetadataInp.arg5 = arg5;
	modAVUMetadataInp.arg6 = "";
	modAVUMetadataInp.arg7 = "";
	modAVUMetadataInp.arg8 ="";
	modAVUMetadataInp.arg9 ="";

	return rcModAVUMetadata(conn, &modAVUMetadataInp);
}

int irodsWinAddMetadata(rcComm_t *conn, CString & objFulleName, int objType,
			   CString & attName, CString & attValues, CString & attUnit)
{
	modAVUMetadataInp_t modAVUMetadataInp;
	char arg0[4];
	char arg1[4];
	char arg2[2048];   // full obj name
	char arg3[1024];   // att name
 	char arg4[2048];   // att value
	char arg5[1024];   // att unit

	strcpy(arg0, "add");
	if(objType == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		strcpy(arg1, "-c");
	}
	else if(objType == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		strcpy(arg1, "-d");
	}
	else
	{
		return -1;
	}
	strcpy(arg2, (char *)LPCTSTR(objFulleName));
	strcpy(arg3, (char *)LPCTSTR(attName));
	strcpy(arg4, (char *)LPCTSTR(attValues));
	if(attUnit.GetLength() > 0)
	{
		strcpy(arg5, (char *)LPCTSTR(attUnit));
	}
	else
	{
		arg5[0] = '\0';
	}

	modAVUMetadataInp.arg0 = arg0;
	modAVUMetadataInp.arg1 = arg1;
	modAVUMetadataInp.arg2 = arg2;
	modAVUMetadataInp.arg3 = arg3;
	modAVUMetadataInp.arg4 = arg4;
	modAVUMetadataInp.arg5 = arg5;
	modAVUMetadataInp.arg6 = "";
	modAVUMetadataInp.arg7 = "";
	modAVUMetadataInp.arg8 ="";
	modAVUMetadataInp.arg9 ="";

	return rcModAVUMetadata(conn, &modAVUMetadataInp);
}

int irodsWinChangePasswd(rcComm_t *conn, CString & old_passwd, CString & new_passwd)
{
	userAdminInp_t userAdminInp;
	char rand[]="1gCBizHWbwIYyWLoysGzTe6SyzqFKMniZX05faZHWAwQKXf6Fs";
	char buf0[MAX_PASSWORD_LEN+10];
	char buf1[MAX_PASSWORD_LEN+10];
	char buf2[MAX_PASSWORD_LEN+10];

	strncpy(buf0, (char *)LPCTSTR(new_passwd), MAX_PASSWORD_LEN);
	int len = new_passwd.GetLength();
	int lcopy = MAX_PASSWORD_LEN-10-len;
	if (lcopy > 15)
	{
		strncat(buf0, rand, lcopy);
	}
	strcpy(buf1, (char *)LPCTSTR(old_passwd));

	obfEncodeByKey(buf0, buf1, buf2);

	userAdminInp.arg0 = "userpw";
	userAdminInp.arg1 = conn->clientUser.userName;
	userAdminInp.arg2 = "password";
	userAdminInp.arg3 = buf2;
	userAdminInp.arg4 = "";
	userAdminInp.arg5 = "";
	userAdminInp.arg6 = "";
	userAdminInp.arg7 = "";
	userAdminInp.arg8 = "";
	userAdminInp.arg9 = "";

	int t = rcUserAdmin(conn, &userAdminInp);

	return t;
}

int irodsWinExecRule(rcComm_t *conn, CString & irodsRule, 
					 CString & inParam, CString & outParam, CString &ruleOutput)
{
	execMyRuleInp_t execMyRuleInp;
	msParamArray_t msParamArray;
	msParamArray_t *outParamArray = NULL;
	strArray_t strArray;
	int i, j, t;
	char *value;
	msParam_t *mP;
	execCmdOut_t *execCmdOut;

	memset(&execMyRuleInp, 0, sizeof (execMyRuleInp));
	memset (&msParamArray, 0, sizeof (msParamArray));
	execMyRuleInp.inpParamArray = &msParamArray;
	execMyRuleInp.condInput.len=0;

	/* rule */
	rstrcpy(execMyRuleInp.myRule, (char *)LPCTSTR(irodsRule), META_STR_LEN);
	/* output format */
	if(outParam.Compare("null") != 0)
		rstrcpy(execMyRuleInp.outParamDesc, (char *)LPCTSTR(outParam), LONG_NAME_LEN);

	/* input format */
	if((inParam.GetLength() <= 0) || (inParam.CompareNoCase("null") == 0))
	{
		execMyRuleInp.inpParamArray = NULL;
	}
	else
	{
		memset (&strArray, 0, sizeof (strArray));
		t = parseMultiStr((char *)LPCTSTR(inParam), &strArray);
		if(t < 0)
			return t;
		value = strArray.value;
		for (i = 0; i < strArray.len; i++)
		{
			char *valPtr = &value[i * strArray.size];
			char *tmpPtr;
			if ((tmpPtr = strstr (valPtr, "=")) != NULL)
			{
				*tmpPtr = '\0';
				tmpPtr++;
				addMsParam(execMyRuleInp.inpParamArray, valPtr, STR_MS_T, strdup (tmpPtr), NULL);
				if (*tmpPtr == '\\')
				{
					tmpPtr++;
					addMsParam (execMyRuleInp.inpParamArray, valPtr, STR_MS_T, strdup (tmpPtr), NULL);
				}
				else
				{
					addMsParam (execMyRuleInp.inpParamArray, valPtr,STR_MS_T, strdup(tmpPtr), NULL);
				}
			}
		}
	}

	t = rcExecMyRule (conn, &execMyRuleInp, &outParamArray);
	if(t < 0)
		return t;

	if ((mP = getMsParamByLabel (outParamArray, "ruleExecOut")) != NULL)
	{
		execCmdOut = (execCmdOut_t *) mP->inOutStruct;
		if (execCmdOut->stdoutBuf.buf != NULL)
			ruleOutput = (char *)execCmdOut->stdoutBuf.buf;
	}

	return 0;
}

#define RULE_STATUS_BIG_STR 200
static void PrintRuleStatusQueryResults(rcComm_t *Conn, genQueryOut_t *genQueryOut,
					char *descriptions[], 
					CStringArray & ruleIds, CStringArray & ruleStatusDescs)
{
	int printCount;
	int i, j;
	char localTime[20];
	printCount=0;
	CString one_id, one_status_desc;
	CString tmpstr;

	one_id = "";
	one_status_desc = "";

	for (i=0;i<genQueryOut->rowCnt;i++) 
	{
		one_id = "";
		one_status_desc = "";
		for (j=0;j<genQueryOut->attriCnt;j++) 
		{
               char *tResult;
               tResult = genQueryOut->sqlResult[j].value;
               tResult += i*genQueryOut->sqlResult[j].len;
                if (strcmp(descriptions[j], "time")==0) 
				{
					getLocalTimeFromRodsTime(tResult, localTime);
					tmpstr.Format("%s: %s : %s\r\n", descriptions[j], tResult,localTime);
                }
                else 
				{
					tmpstr.Format("%s: %s\r\n", descriptions[j], tResult);
                }

				if(j == 0)
				{
					one_id = tResult;
				}

				one_status_desc += tmpstr;
               printCount++;
		}
		ruleIds.Add(one_id);
		ruleStatusDescs.Add(one_status_desc);
	}
}

int irodsWinQueryPendingRules(rcComm_t *conn, CString & userName, CString & ruleName, 
							  CStringArray & ruleIds, CStringArray & ruleStatusDesc)
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int i1a[20];
   int i1b[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   int i2a[20];
   char *condVal[10];
   char v1[RULE_STATUS_BIG_STR];
   char v2[RULE_STATUS_BIG_STR];
   int i, status;
   int printCount;
   char *columnNames[]={"id", "name", "rei_file_path", "user_name",
                        "address", "time", "frequency", "priority",
                        "estimated_exe_time", "notification_addr",
                        "last_exe_time", "exec_status"};

   printCount=0;
   memset (&genQueryInp, 0, sizeof (genQueryInp_t));

   i=0;
   i1a[i++]=COL_RULE_EXEC_ID;
   i1a[i++]=COL_RULE_EXEC_NAME;
   i1a[i++]=COL_RULE_EXEC_REI_FILE_PATH;
   i1a[i++]=COL_RULE_EXEC_USER_NAME;
   i1a[i++]=COL_RULE_EXEC_ADDRESS;
   i1a[i++]=COL_RULE_EXEC_TIME;
   i1a[i++]=COL_RULE_EXEC_FREQUENCY;
   i1a[i++]=COL_RULE_EXEC_PRIORITY;
   i1a[i++]=COL_RULE_EXEC_ESTIMATED_EXE_TIME;
   i1a[i++]=COL_RULE_EXEC_NOTIFICATION_ADDR;
   i1a[i++]=COL_RULE_EXEC_LAST_EXE_TIME;
   i1a[i++]=COL_RULE_EXEC_STATUS;

   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = i;

   i2a[0]=COL_RULE_EXEC_USER_NAME;
	sprintf(v1,"='%s'",(char *)LPCTSTR(userName));
	condVal[0]=v1;
	genQueryInp.sqlCondInp.inx = i2a;
	genQueryInp.sqlCondInp.value = condVal;
	genQueryInp.sqlCondInp.len=1;

	if (ruleName.GetLength() > 0)
	{
		int i;
		i =  genQueryInp.sqlCondInp.len;
		i2a[i]=COL_RULE_EXEC_NAME;
		sprintf(v2,"='%s'", (char *)LPCTSTR(ruleName));
		condVal[i]=v2;
		genQueryInp.sqlCondInp.len++;
    }

	genQueryInp.condInput.len=0;

   genQueryInp.maxRows=150;
   genQueryInp.continueInx=0;
   status = rcGenQuery(conn, &genQueryInp, &genQueryOut);
   
   if (status == CAT_NO_ROWS_FOUND) 
      return 0;

   if(status < 0)
	   return status;

   ruleIds.RemoveAll();
   ruleStatusDesc.RemoveAll();
   do
   {
	   // extract query data
	   if(status !=CAT_NO_ROWS_FOUND)
			PrintRuleStatusQueryResults(conn, genQueryOut, columnNames, ruleIds, ruleStatusDesc);

	   // next round
	   status = rcGenQuery(conn, &genQueryInp, &genQueryOut);
   }
   while((status==0) && (genQueryOut->continueInx > 0));

   freeGenQueryOut(&genQueryOut);
   return 0;
}

int irodsWinDeleteRule(rcComm_t *conn, CString & ruleId)
{
	ruleExecDelInp_t ruleExecDelInp;

	strncpy(ruleExecDelInp.ruleExecId, (char *)LPCTSTR(ruleId), NAME_LEN);
	int status = rcRuleExecDel(conn, &ruleExecDelInp);

	return(status);
}

int irodsWinCleanTrashArea(rcComm_t *conn)
{
	rodsEnv myRodsEnv;
	memset(&myRodsEnv, 0, sizeof(myRodsEnv));
	irodsWinSetRodsEnvViaConn(conn, &myRodsEnv);

	rodsArguments_t myRodsArgs;
	memset(&myRodsArgs, 0, sizeof(rodsArguments_t));

	rodsPathInp_t rodsPathInp;
    memset(&rodsPathInp, 0, sizeof(rodsPathInp_t));

	return rmtrashUtil(conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);
}

HBRUSH CreateCtrlBrush( CWnd* a_pDlg, CWnd* a_pCtrl, CBrush& a_brush )
{
   CRect ctrlRect;
   CRect dlgRect;

   CDC* pTempDC = a_pDlg->GetDC();

   a_pCtrl->GetWindowRect( &ctrlRect );
   a_pDlg->GetWindowRect( &dlgRect );

   // Map control rect to parent background
   int x = ctrlRect.left - dlgRect.left;
   int y = ctrlRect.top - dlgRect.top;
   int w = ctrlRect.Width();
   int h = ctrlRect.Height();

   // Bitmap to hold background for control
   CBitmap bkgndBitmap;
   bkgndBitmap.CreateCompatibleBitmap( pTempDC, w, h );

   // And a DC used to fill it in.
   CDC dcBitmap;
   dcBitmap.CreateCompatibleDC( pTempDC );
   dcBitmap.SelectObject( bkgndBitmap );

   // Copy background into bitmap
   dcBitmap.BitBlt(0, 0, w, h, pTempDC, x, y, SRCCOPY);

   if ( a_brush.m_hObject )
         a_brush.DeleteObject();

   a_brush.CreatePatternBrush( &bkgndBitmap );

   a_pDlg->ReleaseDC( pTempDC );

   return (HBRUSH) a_brush;
}

void DispFileSize(float fileSize, CString & dispStr)
{
	float ft = fileSize/1048576.0;
	int nt = (int)(ft/1024.0);
	if(nt > 0)
	{
		dispStr.Format("%.2f GB", ft/1024.0);
	}
	else
	{
		nt = (int)ft;
		if(nt > 0)
		{
			dispStr.Format("%.2f MB", ft);
		}
		else
		{
			ft = fileSize/1024.0;
			nt = (int)ft;
			if(nt > 0)
			{
				dispStr.Format("%.2f KB", ft);
			}
			else
			{
				dispStr.Format("%d Bytes", (int)fileSize);
			}
		}
	}
}