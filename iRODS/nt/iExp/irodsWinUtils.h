#ifndef __irods_win_utils_h__
#define __irods_win_utils_h__

#include "irodsWinCollection.h""
#include "irodsWinDataobj.h"

class iRODSUserInfo
{
public:
	CString name;
	CString id;
	CString type;
	CString zone;
	CString DN;
	CString info;
	CString comment;
	CString create_time;
	CString modify_time;

	CStringArray groups;
};

class iRODSResourceInfo
{
public:
	bool is_group_resc;

	// for physical resource
	CString name;
	CString id;
	CString zone;
	CString type;
	CString resc_class;
	CString location;
	CString vault_path;
	CString free_space;
	CString status;
	CString info;
	CString comment;
	CString create_time;
	CString modify_time;

	// for group resc
	CStringArray sub_rescs;
};

class iRODSObjSysMetadata
{
public:
	CString parent_coll;
	CString obj_name;
	CString obj_id;
	CString version;
	CString type;
	CString size;
	CString zone;
	CString owner;
	CString status;
	CString checksum;
	CString expiry;
	CString comments;

	// for replicat
	CStringArray repl_num;
	CStringArray repl_create_time;
	CStringArray repl_modify_time;
	CStringArray repl_status;
	CStringArray repl_resc_grp_name;
	CStringArray repl_resc_name;
	CStringArray repl_resc_host_name;
	CStringArray repl_resc_storage_type;
	CStringArray repl_vault_path;
};

int irodsWinGetRescInfo(rcComm_t *conn, const CString & resc_name, iRODSResourceInfo & resc_info);
int irodsWinGetChildObjs(rcComm_t *conn, const CString& ParentColl, 
				 CArray<irodsWinDataobj, irodsWinDataobj> & ChildDatasets);
int irodsWinGetChildCollections(rcComm_t *conn, const CString& ParentColl, 
						CArray<irodsWinCollection, irodsWinCollection> & ChildCollections);
int irodsWinDeleteOneCollection(rcComm_t *conn, CString & collWholePath, bool forceful, bool recursive);
int irodsWinRenameOneCollection(rcComm_t *conn, CString & oriCollWholePath, CString & newCollWholePath);
void irodsWinUnixPathGetName(CString & inPath, CString & pname);
void irodsWinPathGetName(CString & inPath, CString & pname);
void irodsWinUnixPathGetParent(CString & inPath, CString & parentPath);
int irodsWinUploadOneFile(rcComm_t *conn, CString & irodsRescName, 
						  CString & irodsCollPath, CString & localFilePathName,
						  bool forceful,
						  irodsWinDataobj & newDataObj);
int irodsWinUploadOneFolder(rcComm_t *conn, CString & irodsRescName, 
						  CString & irodsCollPath, CString & localFolderPathName,
						  bool forceful,
						  irodsWinCollection & newChildColl);
int irodsWinGetResourceNames(rcComm_t *conn, CStringArray & irodsRescNames);
int irodsWinGetGroupResourceNames(rcComm_t *conn, CStringArray & irodsGrpRescNames);
int irodsWinGetUserInfo(rcComm_t *conn, CString & userName, iRODSUserInfo & userInfo);
int irodsWinReplicateObjects(rcComm_t *conn, CStringArray & irodsCollOrObjPaths, CString & irodsRescName);
void irodsWinSetRodsEnvViaConn(rcComm_t *conn, rodsEnv *pRodsEnv);
int irodsWinDeleteOneDataobj(rcComm_t *conn, CString & dataName, CString & dataFullPath, int replNum, bool forceful);
int irodsWinRenameOneDataobj(rcComm_t *conn, CString & parColl, CString & oldName, CString newName);
int irodsWinDownloadOneObject(rcComm_t *conn, CString irodsObjFullPath, CString & localDir, 
							  int recursive, int forceful);
bool irodsWinIsDescendant(CString & p1, CString & p2);
int irodsWinPasteObjects(rcComm_t *conn, CString & irodsRescName,
			CString & irodsCollPathPasteTo,
			CStringArray & irodsObjsFullPath, bool forceful);
int irodsWinMoveObjects(rcComm_t *conn, CString & destColl, CStringArray & irodsObjsFullPath);
int irodsWinSetAccessControl(rcComm_t *conn, CString & irodsZone, CString irodsUserName, 
							 CString & irodsPermission, 
							 CString & irodsObjFullPath, bool recursive);
int irodsWinGetDatasetAccessControl(rcComm_t *conn, CString & dataObjFullPath, 
				CStringArray & irodsUsers, CStringArray & irodsConstraints);
int irodsWinGetUsersByZone(rcComm_t *conn, CString & irodsZone, CStringArray & irodsUsers);

int irodsWinGetCollectionAccessControl(rcComm_t *conn, CString & collFullPath,
					CStringArray & irodsUsers, CStringArray & irodsZones, CStringArray & irodsConstraints);

int irodsWinGetObjUserDefinedMetadata(rcComm_t *conn, 
		CString & objNameWithPath, int objType,
		CStringArray & metaAtts, CStringArray & metaVals, CStringArray & metaUnits);

int irodsWinDeleteMetadata(rcComm_t *conn, CString & objFulleName, int objType,
			   CString & attName, CString & attValues, CString & attUnit);

int irodsWinAddMetadata(rcComm_t *conn, CString & objFulleName, int objType,
			   CString & attName, CString & attValues, CString & attUnit);

int irodsWinChangePasswd(rcComm_t *conn, CString & old_passwd, CString & new_passwd);
int irodsWinExecRule(rcComm_t *conn, CString & irodsRule, 
					 CString & inParam, CString & outParam, CString &ruleOutput);
int irodsWinQueryPendingRules(rcComm_t *conn, CString & userName, CString & ruleName, 
							  CStringArray & ruleIds, CStringArray & ruleStatusDesc);
int irodsWinDeleteRule(rcComm_t *conn, CString & ruleId);
int irodsWinCleanTrashArea(rcComm_t *conn);

int irodsWinQueryMetadata(rcComm_t *conn, const CString& TopQueryColl, int RecursiveSrch,
		CArray<CString, CString> & AttNameOps, CArray<CString, CString> & AttNameQueryStrs,
		CArray<CString, CString> & AttValOps, CArray<CString, CString> & AttValQueryStrs,
		CArray<CString, CString> & ParColls, CArray<CString, CString> & DataNames);

int irodsWinQueryFilename(rcComm_t *conn, const CString& TopQueryColl, int RecursiveSrch,
				CString & FilenameQueryOp, CString & FilenameQueryStr,
				CArray<CString, CString> & ParColls, CArray<CString, CString> & DataNames);

int irodsWinQueryFilenameAdvanced(rcComm_t *conn, CString quest_where_str,
		   CString queryZone, int noDistinctFlag,
			CArray<CString, CString> & ParColls, CArray<CString, CString> & DataNames);

HBRUSH CreateCtrlBrush( CWnd* a_pDlg, CWnd* a_pCtrl, CBrush& a_brush );
void DispFileSize(float fileSize, CString & dispStr);

genQueryInp_t *irodsWinCreateGenericQueryInp(CArray<int, int> & queryFields, 
											 CArray<int, int> & condFields, CStringArray & condValues);

void irodsWinGenericQueryInPFree(int num_of_query_fields, int num_of_cond_fields,
								genQueryInp_t *pGenQueryInp);

int irodsWinGetObjSysMetadata(rcComm_t *conn, CString parent_coll, CString obj_name, iRODSObjSysMetadata & obj_sysmeta);

#endif