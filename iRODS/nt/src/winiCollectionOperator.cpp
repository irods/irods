#include "winiCollectionOperator.h"
#include "winiCollection.h"
#include "winiDataset.h"
#include "winiMetadata.h"
#include "winiAccess.h"
#include <dirent.h>
#include "dataObjRename.h"
#include "mvUtil.h"
#include "getUtil.h"

namespace WINI
{

CollectionOperatorImpl::CollectionOperatorImpl(ISession* session)
{
	m_bytes_received = 0;
	m_bytes_expected = 0;
	m_bIsDownloading = false;
	m_session = session;
	m_conn = (rcComm_t*)session->GetConn();
	INode* m_binding = NULL;
}

CollectionOperatorImpl::~CollectionOperatorImpl()
{
}

StatusCode CollectionOperatorImpl::Bind(INode* node) { m_binding = node; return WINI_OK;};

StatusCode CollectionOperatorImpl::Delete(ICollectionNode* target)
{
	if(target == m_session->GetUserHomeCol() || target == m_session->GetUserRootCol())
		return WINI_ERROR;
	
	WINI::StatusCode status;

	collInp_t targetCollInp_t;

	sprintf(targetCollInp_t.collName, "%s", target->GetPath());

	memset((void*)&targetCollInp_t.condInput, 0, sizeof(keyValPair_t));

	status = rcRmColl(m_conn, &targetCollInp_t, 0);

	if(!status.isOk())
		return status;

	CollectionNodeImpl* parent = (CollectionNodeImpl*)target->GetParent();

	if(NULL == parent)
		return WINI_ERROR;

	parent->DeleteChild(target);

	return WINI_OK;
}

//this might not actually work since I do not know the state of renaming collections on the server side
//(the server needs to change the pathnames for all subcollections of the collection)
StatusCode CollectionOperatorImpl::Rename(ICollectionNode* target, const char* name)
{
	if(NULL == target || NULL == name)
		return WINI_ERROR_INVALID_PARAMETER;

	ICollectionNode* parent = (ICollectionNode*)target->GetParent();

	if(NULL == parent)
		return WINI_ERROR_INVALID_PARAMETER;

	StatusCode status;

	const char* szParentPath = parent->GetPath();

	int length = strlen(szParentPath);

	length += strlen(name);

	char* buf = new char[length + 2];	//+1 for / and +1 for NULL

	sprintf(buf, "%s/%s", szParentPath, name);

	rodsPathInp_t myRodsPath;
	rodsPath_t src;
	rodsPath_t dest;
	rodsPath_t targ;
	memset((void*)&myRodsPath, 0, sizeof (rodsPathInp_t));
	memset((void*)&src, 0, sizeof (rodsPath_t));
	memset((void*)&dest, 0, sizeof (rodsPath_t));
	memset((void*)&targ, 0, sizeof (rodsPath_t));
	myRodsPath.srcPath = &src;
	myRodsPath.destPath = &dest;
	myRodsPath.targPath = &targ;

	myRodsPath.numSrc = 1;

#if 0
	src.objType = UNKNOWN_OBJ_T;
	src.objState = UNKNOWN__ST;
	src.size = 0;
	sprintf(src.inPath, "%s", collection name);
	sprintf(src.outPath, "%s", collection path);
	src.dataId[0] = '/0';
	src.chksum[0] = '/0';

	dest.objType = UNKNOWN_OBJ_T;
	dest.objState = UNKNOWN__ST;
	dest.size = 0;
	sprintf(dest.inPath, "%s", new collection name);
	sprintf(dest.outPath, "%s", new collection path);
	dest.dataId[0] = '/0';
	dest.chksum[0] = '/0';
#endif

	sprintf(src.inPath, "%s", (char*)target->GetName());
	sprintf(src.outPath, "%s", (char*)target->GetPath());
	sprintf(dest.inPath, "%s", name);
	sprintf(dest.outPath, "%s", buf);

	rodsArguments_t myRodsArgs;
	memset((void*)&myRodsArgs, 0, sizeof (rodsArguments_t));

	rodsEnv myEnv;
	memset((void*)&myEnv, 0, sizeof (rodsEnv));

	status = mvUtil(m_conn, &myEnv, &myRodsArgs, &myRodsPath);

	delete buf;

	if(status.isOk())
		((CollectionNodeImpl*)target)->SetName(name);

	return status;
}

StatusCode CollectionOperatorImpl::Create(ICollectionNode* target, const char* name, ICollectionNode** result)
{
	if(NULL == target || NULL == name)
		return WINI_ERROR_INVALID_PARAMETER;

	StatusCode status;

	status = WINI_ERROR;

	char buf[2048];
	sprintf(buf, "%s/%s", target->GetPath(), name);

	status = mkColl(m_conn, buf);

	ICollectionNode* child;

	if(status.isOk())
	{
		child = new CollectionNodeImpl(target, name);	
		((CollectionNodeImpl*)target)->AddChild(child);
	}

	if(result)
		*result = child;

	return status;
}

StatusCode CollectionOperatorImpl::ModifyAccess(IAccessNode* target, const char* permission, bool recursive)
{
	if(NULL == target || NULL == permission)
		return WINI_ERROR_INVALID_PARAMETER;
#if 0
	IStringNode* constraints = m_session->GetAccessConstraints();

	int count = constraints->CountStrings();

	for(int i = 0; i < count; i++)
	{
		if(0 == strcmp(constraints->GetString(i), permission))
			break;
	}

	if(count == i)
		return WINI_ERROR_INVALID_PARAMETER;

	char* collection = (char*)target->GetParent()->GetPath();
	char* user = (char*)target->GetUser();
	char* domain = (char*)target->GetDomain();
	char* buf;

	StatusCode status;

	int option = 0;

	if(recursive)
		option = R_FLAG;

	int len = 0;

	len += strlen(user);
	len += strlen(domain);
	len += 2;

	buf = (char*)calloc(len, sizeof(char));

	sprintf(buf, "%s@%s", user, domain);
	status = chmodInColl(m_conn, 0, collection, option, (char*)permission, buf);

	free(buf);
	
	if(status.isOk())
	{
		implModifyAccess(permission, target, recursive);
	}

	return status;
#endif
	return WINI_ERROR;

}

void CollectionOperatorImpl::implModifyAccess(const char* permission, IAccessNode* target, bool recursive)
{
	INode* parent = target->GetParent();
	if(0 == strcmp("null", permission))
	{
		if(recursive)
		{
			char* szDomain = strdup(target->GetDomain());
			char* szUser = strdup(target->GetUser());
			implModifyAccessSubFunc1(parent, szDomain, szUser);
			free(szDomain);
			free(szUser);
		}else
		{
			switch(parent->GetType())
			{
			case WINI_COLLECTION:
				((CollectionNodeImpl*)parent)->DeleteChild(target);
				break;
			case WINI_DATASET:
				((DatasetNodeImpl*)parent)->DeleteChild(target);
				break;
			}
		}
	}else
	{
		if(recursive)
		{
			char* szDomain = strdup(target->GetDomain());
			char* szUser = strdup(target->GetUser());
			implModifyAccessSubFunc2(parent, szDomain, szUser, permission);
			free(szDomain);
			free(szUser);
		}else
			((AccessNodeImpl*)target)->SetPermission(permission);
	}
}

void CollectionOperatorImpl::implModifyAccessSubFunc1(INode* node, const char* szDomain, const char* szUser)
{
	if(node->isOpen())
	{
		INode* child;
		int i = 0;
		child = node->GetChild(0);
		int type = node->GetType();

		IAccessNode* access;

		do
		{
			if(child->GetType() == WINI_ACCESS)
			{
				access = (IAccessNode*)child;

				if(0 == strcmp(access->GetUser(), szUser))
				{
					if(0 == strcmp(access->GetDomain(), szDomain))
					{
						switch(type)
						{
						case WINI_DATASET:
							((DatasetNodeImpl*)node)->DeleteChild(child);
							break;
						case WINI_COLLECTION:
							((CollectionNodeImpl*)node)->DeleteChild(child);
							break;
						}
						//ith child deleted, do not increment i
					}
				}else
					i++;
			}else
				i++;

			child = node->GetChild(i);
		} while(NULL != child);

		//if this is a collection we must examine its children also
		if(type == WINI_COLLECTION)
		{
			int count = node->CountChildren();

			for(int i = 0; i < count; i++)
			{
				INode* child = node->GetChild(i);

				type = child->GetType();

				if(WINI_COLLECTION == type || WINI_DATASET == type)
				{
					implModifyAccessSubFunc1(child, szDomain, szUser);
				}
			}
		}
	}
}

void CollectionOperatorImpl::implModifyAccessSubFunc2(INode* node, const char* szDomain, const char* szUser, const char* new_permission)
{
	if(node->isOpen())
	{
		int type = node->GetType();
		int count = node->CountChildren();

		INode* child;
		IAccessNode* access;
		int i;
		for(i = 0; i < count; i++)
		{
			child = node->GetChild(i);

			if(child->GetType() == WINI_ACCESS)
			{
				access = (IAccessNode*)child;
				if(0 == strcmp(szDomain, access->GetDomain()))
					if(0 == strcmp(szUser, access->GetUser()))
					{
						((AccessNodeImpl*)child)->SetPermission(new_permission);
						break;
					}
			}
		}

		if(i == count)
		{
			//this node did not previously have the permission.
			//however, since a recursive modify will add the permission to the dataset,
			//we must create a new access node for this node
			switch(type)
			{
			case WINI_DATASET:
				((DatasetNodeImpl*)node)->AddChild(new AccessNodeImpl(node, NULL, szDomain, szUser, new_permission));
				break;
			case WINI_COLLECTION:
				((CollectionNodeImpl*)node)->AddChild(new AccessNodeImpl(node, NULL, szDomain, szUser, new_permission));
				break;
			}

		}

		//if this is a collection we must examine its children also
		if(type == WINI_COLLECTION)
		{
			count = node->CountChildren();

			for(int i = 0; i < count; i++)
			{
				child = node->GetChild(i);

				type = child->GetType();

				if(WINI_COLLECTION == type || WINI_DATASET == type)
				{
					implModifyAccessSubFunc2(child, szDomain, szUser, new_permission);
				}
			}
		}
	}
}

//we assume that "null" means remove the access
StatusCode CollectionOperatorImpl::SetAccess(ICollectionNode* target, IUserNode* owner, const char* permission, bool recursive)
{
	if(NULL == target || NULL == owner || NULL == permission)
		return WINI_ERROR_INVALID_PARAMETER;
#if 0
	int option = 0;

	int retraction_type;
	char* sz;

	if(0 == strcmp("null", permission))
	{
		retraction_type = D_DELETE_COLL_ACCS;
		sz = "";
	}
	else
	{
		retraction_type = D_INSERT_COLL_ACCS;
		sz = (char*)permission;
	}

	StatusCode status;

	if(!recursive)
		status = srbModifyCollect(m_conn, 0, (char*)target->GetPath(), (char*)owner->GetName(), (char*)owner->GetParent()->GetName(), (char*)permission, retraction_type);
	else
	{
		status = srbModifyCollect(m_conn, 0, (char*)target->GetPath(), (char*)owner->GetName(), (char*)owner->GetParent()->GetName(), (char*)permission, retraction_type);
		if(status.isOk())
		{
			status = chmodInColl(m_conn, 0, (char*)target->GetPath(), R_FLAG, (char*)permission, (char*)owner->GetPath());
		}

	}

	if(status.isOk())
	{
		SetAccessNodes(target, owner, permission, recursive, retraction_type);
	}

	return status;
#endif
}




StatusCode CollectionOperatorImpl::ModifyCollectionRecursively(ICollectionNode* target, IUserNode* owner, const char* permission)
{
	return WINI_OK;
}


void CollectionOperatorImpl::SetAccessNodes(INode* target, IUserNode* owner, const char* permission, bool recursive, int retraction_type)
{
	int count = target->CountChildren();
	INode* child;
	int i;
	if(target->isOpen())
	{
		//first account for target
		for(i = 0; i < count; i++)
		{
			child = target->GetChild(i);
			if(WINI_ACCESS == child->GetType())
			{
				if(0 == strcmp(owner->GetName(), ((IAccessNode*)child)->GetUser()))
				{
					((AccessNodeImpl*)child)->SetPermission(permission);
					break;
				}
			}
		}

		if(i == count)
		{
			//a new user was added
			if(target->GetType() == WINI_COLLECTION)
				((CollectionNodeImpl*)target)->AddChild(new AccessNodeImpl(target, NULL, owner->GetParent()->GetName(), owner->GetName(), permission));
			else
				((DatasetNodeImpl*)target)->AddChild(new AccessNodeImpl(target, NULL, owner->GetParent()->GetName(), owner->GetName(), permission));

		}

		//now account for children, if need be
		int type;
		if(recursive)
		{
			for(int j = 0; j < target->CountChildren(); j++)
			{
				child = target->GetChild(j);
				type = child->GetType();
				if(type == WINI_COLLECTION || type == WINI_DATASET)
					SetAccessNodes(child, owner, permission, recursive, retraction_type);
			}
		}
	}
}


StatusCode CollectionOperatorImpl::AddMeta(ICollectionNode* target, const char* attribute, IMetadataNode** result)
{
#if 0
	StatusCode status;

	status = WINI_ERROR;

	//StatusCode id = srbModifyCollect(m_conn, 0, (char*)target->GetPath(), "0", (char*)attribute, "", C_INSERT_USER_DEFINED_COLL_STRING_META_DATA);

	if(!id.isOk())
		return id;

	CollectionNodeImpl* collection = (CollectionNodeImpl*)target;

	MetadataNodeImpl* baby = new MetadataNodeImpl(target, attribute, WINI_MD_EQUAL, NULL);
	baby->SetID(id.GetCode());

	collection->AddChild(baby);

	if(NULL != result)
	{
		*result = baby;
	}
#endif
	return WINI_OK;
}	

StatusCode CollectionOperatorImpl::ModifyMeta(IMetadataNode* target, const char* value)
{
#if 0
	ICollectionNode* parent = (ICollectionNode*)target->GetParent();

	static char buf[256];

	sprintf(buf, "1@%d", ((MetadataNodeImpl*)target)->GetID());

	StatusCode status = srbModifyCollect(m_conn, 0, (char*)parent->GetPath(), buf, (char*)value, "", C_CHANGE_USER_DEFINED_COLL_STRING_META_DATA);

	if(status.isOk())
	{
		((MetadataNodeImpl*)target)->SetValue(value);
	}

	return status;
#endif
	return WINI_ERROR;

};

StatusCode CollectionOperatorImpl::DeleteMeta(IMetadataNode* target)
{
#if 0
	static char buf[10];
	sprintf(buf, "%d", ((MetadataNodeImpl*)target)->GetID());

	CollectionNodeImpl* parent = (CollectionNodeImpl*)target->GetParent();
	StatusCode status = srbModifyCollect(m_conn, 0, (char*)parent->GetPath(),buf, "", "", C_DELETE_USER_DEFINED_COLL_STRING_META_DATA);

	if(status.isOk())
	{
		parent->DeleteChild(target);
	}

	return status;
#endif
	return WINI_ERROR;
};

StatusCode CollectionOperatorImpl::GetChildren(CollectionNodeImpl* target, unsigned int mask)
{
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	target->Clear();

	StatusCode status;
	
	//unsigned int success = WINI_ALL ^ WINI_ACCESS ^ WINI_METADATA ^ WINI_COLLECTION ^ WINI_DATASET;
	unsigned int success = WINI_ALL ^ WINI_COLLECTION ^ WINI_DATASET;

#if 0
	if(WINI_ACCESS & mask)
	{
		status = GetAccess(target);
		if(status.isOk())
		{
			success |= WINI_ACCESS;
		}
		else
		{
			target->SetOpen(success);
			return status;
		}
	}
#endif
	
#if 0
	if(WINI_METADATA & mask)
	{
		status = GetMetadata(target);
		if(status.isOk())
		{
			success |= WINI_METADATA;
		}
		else
		{
			target->SetOpen(success);
			return status;
		}
	}
#endif

	if(WINI_COLLECTION & mask)
	{
		status = GetChildCollections(target);
		if(status.isOk())
		{
			success |= WINI_COLLECTION;
		}
		else
		{
			target->SetOpen(success);
			return status;
		}
	}

	if(WINI_DATASET & mask)
	{
		status = GetChildDatasets(target);
		if(status.isOk())
		{
			success |= WINI_DATASET;
		}
		else
		{
			target->SetOpen(success);
			return status;
		}
	}

	target->SetOpen(success);
	return status;
}

StatusCode CollectionOperatorImpl::GetAccess(CollectionNodeImpl* target)
{
#if 0
	ClearMCATScratch();

	sprintf(m_qval[ACCESS_GROUP_NAME], " = '%s'", target->GetPath());

	m_selval[USER_NAME] = 1;
	m_selval[DOMAIN_DESC] = 1;
	m_selval[GROUP_ACCESS_CONSTRAINT] = 1;

	IZoneNode* myzone = m_session->GetCurrentZone(target);

	StatusCode status;

	if(myzone)
	 status = srbGetDataDirInfoWithZone(m_conn, 0, (char*)myzone->GetName(), m_qval, m_selval, &m_result, MAX_ROWS);
	else
	status = srbGetDataDirInfo(m_conn, 0, m_qval, m_selval, &m_result, MAX_ROWS);

	if(!status.isOk())
	{
		if(-3005 == status)
		{
			status = WINI_OK;
		}
		return status;
	}

	if(0 == m_result.row_count)
		return WINI_OK;

	filterDeleted(&m_result);

	IAccessNode* access_node;
	

	char *user, *domain, *constraint;
	
	user = getFromResultStruct(&m_result, dcs_tname[USER_NAME], dcs_aname[USER_NAME]);
	domain = getFromResultStruct(&m_result, dcs_tname[DOMAIN_DESC], dcs_aname[DOMAIN_DESC]);
	constraint = getFromResultStruct(&m_result, dcs_tname[GROUP_ACCESS_CONSTRAINT], dcs_aname[GROUP_ACCESS_CONSTRAINT]);
	access_node = new AccessNodeImpl(target, NULL, domain, user, constraint);
	target->AddChild(access_node);

	for(int i = 1; i < m_result.row_count; i++)
	{
		user += MAX_DATA_SIZE;
		domain += MAX_DATA_SIZE;
		constraint += MAX_DATA_SIZE;
		access_node = new AccessNodeImpl(target, NULL, domain, user, constraint);
		target->AddChild(access_node);
	}

	//while(m_result.continuation_index >= 0)
	//{
		//implement later
	//}

	return WINI_OK;
#endif
	return WINI_ERROR;
}

StatusCode CollectionOperatorImpl::GetMetadata(CollectionNodeImpl* target)
{
#if 0
	ClearMCATScratch();

	sprintf(m_qval[DATA_GRP_NAME], " = '%s'", target->GetPath());
	sprintf(m_qval[METADATA_NUM_COLL], " >= 0");

	m_selval[METADATA_NUM_COLL] = 1;
	m_selval[UDSMD_COLL0] = 1;
	m_selval[UDSMD_COLL1] = 1;


	StatusCode status;
	IZoneNode* myzone = m_session->GetCurrentZone(target);
	if(myzone)
		status = srbGetDataDirInfoWithZone(m_conn, 0, (char*)myzone->GetName(), m_qval, m_selval, &m_result, MAX_ROWS);
	else
		status = srbGetDataDirInfo(m_conn, 0, m_qval, m_selval, &m_result, MAX_ROWS);

	if(!status.isOk())
	{
		if(-3005 == status)
		{
			status = WINI_OK;
		}
		return status;
	}

	filterDeleted(&m_result);

	IMetadataNode* meta;

	for(int i = 0; i < m_result.row_count; i++)
	{
		meta = new MetadataNodeImpl(target, &m_result, i);
		
		target->AddChild(meta);
	}

	//while(m_result.continuation_index >= 0)
	//{
		//implement later
	//}

	return WINI_OK;
#endif
	return WINI_ERROR;
}

StatusCode CollectionOperatorImpl::GetChildCollections(CollectionNodeImpl* target)
{
	//InitQuery();

    genQueryOut_t *genQueryOut = NULL;
    genQueryInp_t genQueryInp;
	rodsArguments_t myRodsArgs;

	//char szPathBuf[1024];

	memset ((void*)&genQueryInp, 0, sizeof (genQueryInp_t));

	void* fooX = this->m_session->GetConn();
	rcComm_t* foo = (rcComm_t*)fooX;
	queryHandle_t queryHandle;
    int s = queryCollInColl (&queryHandle, (char*)target->GetPath(), 0, &genQueryInp, &genQueryOut);

	//TODO Add support for no collections
	if(-808000 == s)
		return WINI_OK;

    int i, status, j;

    sqlResult_t *colName, *dataId;
    char *tmpDataName, *tmpDataId;

    if ((colName = getSqlResultByInx (genQueryOut, COL_COLL_NAME)) == NULL)
        return UNMATCHED_KEY_OR_INDEX;

    for (i = 0;i < genQueryOut->rowCnt; i++)
	{
        tmpDataName = &colName->value[colName->len * i];

		for(j = strlen(tmpDataName)-1; j != 0; j--)
			if('/' == tmpDataName[j])
				break;

		CollectionNodeImpl* child = new CollectionNodeImpl(target,&tmpDataName[++j]);
		target->AddChild(child);
    }

    return WINI_OK;
}

// by default GetPath returns the path of the node with no trailing slash
// this is because the trailing slash must be omitted for collections and
// present for datasets. 
StatusCode CollectionOperatorImpl::GetChildDatasets(CollectionNodeImpl* target)
{
    char collQCond[MAX_NAME_LEN];
    int status;
	int i,j;
	sqlResult_t *dataId, *dataName, *dataRepl, *dataSize, *dataResc,
	*dataReplStatus, *dataModifyTime, *dataOwner, *dataPath,
	*dataChecksum, *dataColl;
    char *tmpDataName, *tmpDataId;
	genQueryOut_t *genQueryOut = NULL;

	genQueryInp_t genQueryInp;

    memset (&genQueryInp, 0, sizeof (genQueryInp_t));
    genQueryInp.maxRows = MAX_SQL_ROWS;

    snprintf (collQCond, MAX_NAME_LEN, " = '%s'", target->GetPath());
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, collQCond);

	addInxIval (&genQueryInp.selectInp, COL_D_DATA_ID, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_REPL_NUM, 1);
	addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, 1);
	addInxIval (&genQueryInp.selectInp, COL_D_RESC_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_D_REPL_STATUS, 1);
	addInxIval (&genQueryInp.selectInp, COL_D_MODIFY_TIME, 1);
	addInxIval (&genQueryInp.selectInp, COL_D_OWNER_NAME, 1);
	addInxIval (&genQueryInp.selectInp, COL_D_DATA_PATH, 1);
	addInxIval (&genQueryInp.selectInp, COL_D_DATA_CHECKSUM, 1);
	addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);

    status =  rcGenQuery (m_conn, &genQueryInp, &genQueryOut);

	if(-808000 == status)
		return WINI_OK;

    if ((dataId = getSqlResultByInx (genQueryOut, COL_D_DATA_ID)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataRepl = getSqlResultByInx (genQueryOut, COL_DATA_REPL_NUM)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataSize = getSqlResultByInx (genQueryOut, COL_DATA_SIZE)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataResc = getSqlResultByInx (genQueryOut, COL_D_RESC_NAME)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataReplStatus = getSqlResultByInx (genQueryOut, COL_D_REPL_STATUS)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataModifyTime = getSqlResultByInx (genQueryOut, COL_D_MODIFY_TIME)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataOwner = getSqlResultByInx (genQueryOut, COL_D_OWNER_NAME)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataPath = getSqlResultByInx (genQueryOut, COL_D_DATA_PATH)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataChecksum = getSqlResultByInx (genQueryOut, COL_D_DATA_CHECKSUM)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);
    if ((dataColl = getSqlResultByInx (genQueryOut, COL_COLL_NAME)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);

    for (i = 0;i < genQueryOut->rowCnt; i++)
	{
		char* tdId = &dataId->value[dataId->len * i];
		char* tdName = &dataName->value[dataName->len * i];
		char* tdRepl = &dataRepl->value[dataRepl->len * i];
		char* tdSize = &dataSize->value[dataSize->len * i];
		char* tdResc = &dataResc->value[dataResc->len * i];
		char* tdReplStatus = &dataReplStatus->value[dataReplStatus->len * i];
		char* tdModifyTime = &dataModifyTime->value[dataModifyTime->len * i];
		char* tdOwner = &dataOwner->value[dataOwner->len * i];
		char* tdPath = &dataPath->value[dataPath->len * i];
		char* tdChecksum = &dataChecksum->value[dataChecksum->len * i];
		char* tdColl = &dataColl->value[dataColl->len * i];
		DatasetNodeImpl* child = new DatasetNodeImpl(target, tdName, tdSize, tdOwner, tdModifyTime, tdRepl, "", tdResc, tdReplStatus, tdChecksum);
		target->AddChild(child);	
    }
	//getmorerows?
}

StatusCode CollectionOperatorImpl::Download(INode* target, const char* local_path)
{
	if(NULL == local_path)
		return WINI_ERROR_INVALID_PARAMETER;

	if(WINI_COLLECTION != target->GetType())
		return WINI_ERROR_INVALID_PARAMETER;

	if(m_bIsDownloading)
		return WINI_ERROR;

	m_bIsDownloading = true;

	WINI::ICollectionNode* target_collection = (WINI::ICollectionNode*)target;

	int repl = 0;

	if(WINI_DATASET == target->GetType())
		repl = atoi(((IDatasetNode*)target)->GetReplicantNumber());

	rcComm_t* copyconn = (rcComm_t*)m_session->CloneConn();

	char sanitized_path[MAX_PATH];

	int len = strlen(local_path);

	if(local_path[len-1] == '\\')
		len--;

	strncpy(sanitized_path, local_path, len);
	sanitized_path[len] = '\0';

	m_bytes_received = 0;
	m_bytes_expected = 0;

	StatusCode status;

	rodsEnv myRodsEnv;
	rodsArguments_t myRodsArgs;
	rodsPathInp_t rodsPathInp;
	rodsPath_t rp1, rp2, rp3;

	memset((void*)&myRodsEnv, 0, sizeof(rodsEnv));
	memset((void*)&myRodsArgs, 0, sizeof(rodsArguments_t));
	memset((void*)&rodsPathInp, 0, sizeof(rodsPathInp_t));
	memset((void*)&rp1, 0, sizeof(rodsPath_t));
	memset((void*)&rp2, 0, sizeof(rodsPath_t));
	memset((void*)&rp3, 0, sizeof(rodsPath_t));
	rodsPathInp.srcPath = &rp1;
	rodsPathInp.destPath = &rp2;
	rodsPathInp.targPath = &rp3;
	rodsPathInp.numSrc = 1;

	rodsPathInp.srcPath->objType = COLL_OBJ_T;
	rodsPathInp.srcPath->objState = EXIST_ST;
	sprintf(rodsPathInp.srcPath->inPath, "%s", target->GetName());	//$4 = "hh", '\0' <repeats 1021 times>
	sprintf(rodsPathInp.srcPath->outPath, "%s", target->GetPath());	//$5 = "/tempZone/home/rods/hh", '\0' <repeats 1001 times>
	rodsPathInp.destPath->objType = LOCAL_DIR_T;
	rodsPathInp.destPath->objState = EXIST_ST;
	sprintf(rodsPathInp.destPath->inPath, "test"); //$11 = "." '\0' <repeats 1022 times>
	sprintf(rodsPathInp.destPath->outPath, "/test"); //$12 = ".", '\0' <repeats 1022 times>
	//rodsPathInp.targPath.objType $15 = UNKNOWN_OBJ_T
	//rodsPathInp.targPath.objState $16 = UNKNOWN__ST

	myRodsArgs.recursive=True;

	status = getUtil(m_conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);

	m_bIsDownloading = false;

	m_bytes_received = 0;
	m_bytes_expected = 0;

    return status;
}



int CollectionOperatorImpl::GetProgress(char** name)
{
	if(0 == m_bytes_expected)
	{
		if(name)
			*name = NULL;

		return 0;
	}

	if(!m_bIsDownloading)
	{
		if(name)
			*name = NULL;
		return -1;
	}

	//this is intensive, but more accurate - need to make a #define for platform specific long ints
	//this way we can mult an int by 100 and store it in a long int instead of a double (make sure long
	//int or _i64 or whatever is larger than int (32).
	//(int*100)/int is probably fastest way and still accurate (no 106%! =))
	double retval = m_bytes_received;
	retval = (retval / m_bytes_expected) * 100;

	if(NULL != name)
		if(retval)
			*name = m_downloading;
		else
			*name = NULL;

	return retval;
}


//As with datasetoperator and files, directories are considered as collections without metadata and
//specialaccess right information.
StatusCode CollectionOperatorImpl::Upload(INode* target, const char* local_path, unsigned int overwrite, INode** result)
{
#if 0
	if(WINI_COLLECTION != target->GetType())
		return WINI_ERROR_INVALID_PARAMETER;

	if(NULL == local_path)
		return WINI_ERROR_INVALID_PARAMETER;

	char* szptr = strdup(local_path);

	for(int i = 0; i < strlen(szptr); i++)
		if('\\' == szptr[i])
			szptr[i] = '/';
#if 0
	//chkFileName already converts an SRB path to an NT path before it checks.
	//hence it's okay to switch to a regular SRB path (replace \ with /) that dirtocolcopyBC needs (even for local files)
    if(-1 == chkFileName (&myPathName))
		return WINI_ERROR_DOES_NOT_EXIST;
#endif

	m_bIsDownloading = true;

	StatusCode status;

	int nFlag = 0;

	if(overwrite)
		nFlag |= F_FLAG;

	srbConn* copyconn = (srbConn*)m_session->CloneConn();

	if(WINI_CONTAINER == m_binding->GetType())
		status = 1;
	else
		status = UploadImpl(copyconn, szptr, (char*)target->GetPath(), (char*)m_binding->GetName(), false);

	clFinish(copyconn);

	m_downloading[0] = '\0';

	m_bIsDownloading = false;

	free(szptr);

	WINI::ICollectionNode* child;

	int len = strlen(local_path);

	const char* name = NULL;

	for(i = len - 1; i != 0; i--)
	{
		if('\\' == local_path[i])
		{
			//found the beginning of our name
			name = &local_path[i+1];
			break;
		}
	}

	if(NULL == name)
	{
		name = local_path;
	}

	if(status.isOk())
	{
		INode* old_child = target->GetChild(name);
		if(NULL != old_child)
			((CollectionNodeImpl*)target)->DeleteChild(old_child);


		child = new CollectionNodeImpl(target, name);
		((CollectionNodeImpl*)target)->AddChild(child);
		if(result)
			*result = child;
	}

	m_bytes_received = 0;
	m_bytes_expected = 0;

    return status;
#endif
	return WINI_ERROR;
}


StatusCode CollectionOperatorImpl::Copy(ICollectionNode* target, IDatasetNode* data, IDatasetNode** result)
{
#if 0
    StatusCode status;

	char *path, *name, *size, *type;

	path = strdup(data->GetParent()->GetPath());
	name = strdup(data->GetName());
	size = strdup(data->GetSize());
	type = strdup(data->GetDataType());

    m_result.result_count = 4;
    m_result.row_count = 1;
    m_result.sqlresult[0].tab_name = dcs_tname[DATA_GRP_NAME];
    m_result.sqlresult[0].att_name = dcs_aname[DATA_GRP_NAME];
    m_result.sqlresult[0].values = (mdasC_handle)path;
    m_result.sqlresult[1].tab_name = dcs_tname[DATA_NAME];
    m_result.sqlresult[1].att_name = dcs_aname[DATA_NAME];
    m_result.sqlresult[1].values = (mdasC_handle)name;
    m_result.sqlresult[2].tab_name = dcs_tname[SIZE];
    m_result.sqlresult[2].att_name = dcs_aname[SIZE];
    m_result.sqlresult[2].values = (mdasC_handle)size;
    m_result.sqlresult[3].tab_name = dcs_tname[DATA_TYP_NAME];
    m_result.sqlresult[3].att_name = dcs_aname[DATA_TYP_NAME];
    m_result.sqlresult[3].values = (mdasC_handle)type;

	if(WINI_CONTAINER == m_binding->GetType())
		status = dataToCollCopy(m_conn, 0, c_FLAG, &m_result, atoi(data->GetReplicationIndex()), (char*)target->GetPath(), "", "", (char*)((IContainerNode*)m_binding)->GetContainerPath());
	else
		//resource
		status = dataToCollCopy (m_conn, 0, 0, &m_result, atoi(data->GetReplicationIndex()), (char*)target->GetPath(), (char*)m_binding->GetName(), "", "");

	m_result.result_count = 0;		//since we're faking it, we don't want any subsequent clean to
    m_result.row_count = 0;			//try and delete the dcs_aname, etc which are on the stack, as well
									//as our GetPath();
	free(path);
	free(name);
	free(size);
	free(type);
	
	IDatasetNode* blah;

	if(status.isOk())
	{
		if(target->isOpen())
			status = FillDataset((CollectionNodeImpl*)target, &blah, data->GetName());
	}

	if(result)
		*result = blah;

    return status;
#endif
	return WINI_ERROR;
}



StatusCode CollectionOperatorImpl::Copy(ICollectionNode* target, ICollectionNode* data, ICollectionNode** result)
{
#if 0
	if(target == data)
		return WINI_ERROR_SOURCE_EQUALS_TARGET;

    StatusCode status;

    m_result.result_count = 1;
    m_result.row_count = 1;
    m_result.sqlresult[0].tab_name = dcs_tname[DATA_GRP_NAME];
    m_result.sqlresult[0].att_name = dcs_aname[DATA_GRP_NAME];
    m_result.sqlresult[0].values = (mdasC_handle)  data->GetPath();

	if(WINI_CONTAINER == m_binding->GetType())
	    status = collToCollCopy(m_conn, 0, F_FLAG | c_FLAG, COLLECTION_T, &m_result, (char*)target->GetPath(), "", (char*)((IContainerNode*)m_binding)->GetContainerPath());
	else
	    status = collToCollCopy(m_conn, 0, F_FLAG, COLLECTION_T, &m_result, (char*)target->GetPath(), (char*)m_binding->GetName(), "");

	m_result.result_count = 0;		//since we're faking it, we don't want any subsequent clean to
    m_result.row_count = 0;			//try and delete the dcs_aname, etc which are on the stack, as well
									//as our GetPath();

	ICollectionNode* blah = NULL;

	if(status.isOk())
	{
		if(target->isOpen())
		{
			blah = new CollectionNodeImpl(target, data->GetName());
			((CollectionNodeImpl*)target)->AddChild(blah);
		}
	}

	if(result)
		*result = blah;


    return status;
#endif
	return WINI_ERROR;
}

StatusCode CollectionOperatorImpl::Move(ICollectionNode* target, IDatasetNode* data, IDatasetNode** result)
{
#if 0
	StatusCode status = srbModifyDataset(m_conn, 0, (char*)data->GetName(), (char*)data->GetParent()->GetPath(), "","", (char*)target->GetPath(), "", D_CHANGE_GROUP);

	IDatasetNode* blah = NULL;

	if(status.isOk())
	{
		if(target->isOpen())
		{
			status = FillDataset((CollectionNodeImpl*)target, &blah, data->GetName());
			((CollectionNodeImpl*)data->GetParent())->DeleteChild(data);
		}

	}

	if(result)
		*result = blah;
	
	
	return status;
#endif
	return WINI_ERROR;
}

StatusCode CollectionOperatorImpl::Move(ICollectionNode* target, ICollectionNode* data, ICollectionNode** result)
{
#if 0
	int len = strlen(target->GetPath()) + strlen(data->GetName()) + 2;
	char* szNewPath = new char[len];

	sprintf(szNewPath, "%s/%s", target->GetPath(), data->GetName());

	StatusCode status = srbModifyCollect(m_conn, 0, (char*)data->GetPath(), szNewPath, "", "", D_CHANGE_COLL_NAME);

	ICollectionNode* child;

	if(status.isOk())
	{
		child = new CollectionNodeImpl(target, data->GetName());
		((CollectionNodeImpl*)target)->AddChild(child);
		((CollectionNodeImpl*)data->GetParent())->DeleteChild(data);
	}

	return status;
#endif
	return WINI_ERROR;
}

StatusCode CollectionOperatorImpl::SetComment(ICollectionNode* target, const char* comment)
{
	//COLL_COMMENTS
	return WINI_ERROR_INVALID_PARAMETER;
}

StatusCode CollectionOperatorImpl::Replicate_Recursive_Impl(ICollectionNode* target)
{
	WINI::INode* ptr;
	WINI::StatusCode status;
	WINI::IDatasetOperator* op;

	ptr = (WINI::INode*)target;

	if(!ptr->isOpen())
			m_session->OpenTree(ptr, 1, false);

	unsigned int count = ptr->CountChildren();

	for(int i = 0; i < count; i++)
	{
		ptr = target->GetChild(i);
		if(ptr->GetType() == WINI_COLLECTION)
		{
			if(!ptr->isOpen())
				m_session->OpenTree(ptr, 1, false);

			status = Replicate_Recursive_Impl((WINI::ICollectionNode*)ptr);
		}else
		if(ptr->GetType() == WINI_DATASET)
		{
			op = (WINI::IDatasetOperator*)m_session->GetOperator(WINI_DATASET);
			op->Bind(m_binding);
			op->Replicate((WINI::IDatasetNode*)ptr);
		}
	}

	return status;
}

StatusCode CollectionOperatorImpl::Replicate(ICollectionNode* target)
{
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	return Replicate_Recursive_Impl(target);
}


StatusCode CollectionOperatorImpl::FillDataset(CollectionNodeImpl* parent, IDatasetNode** child, const char* name)
{
#if 0
	if(NULL == parent || NULL == child)
		return WINI_ERROR_INVALID_PARAMETER;

	*child = NULL;
	
	ClearMCATScratch();

    m_selval[DATA_NAME] = 1;
    m_selval[DATA_GRP_NAME] = 1;
    m_selval[SIZE] = 1;
    m_selval[DATA_TYP_NAME] = 1;
	m_selval[DATA_OWNER] = 1;
	m_selval[REPL_TIMESTAMP] = 1;
	m_selval[DATA_REPL_ENUM] = 1;
	m_selval[PHY_RSRC_NAME] = 1;
	//m_selval[RSRC_NAME] = 1;
	m_selval[CONTAINER_NAME] = 1;
	m_selval[DATA_COMMENTS] = 1;

    sprintf(m_qval[DATA_GRP_NAME]," = '%s'", parent->GetPath());
    sprintf(m_qval[DATA_NAME]," = '%s'", name);

	StatusCode status = srbGetDataDirInfoWithZone(m_conn, 0, (char*)m_session->GetCurrentZone(parent)->GetName(), m_qval, m_selval, &m_result, MAX_ROWS);

	if(!status.isOk())
	{
		if(-3005 == status)
			return WINI_OK;
		return status;
	}

	filterDeleted(&m_result);

	if(m_result.row_count != 1)
		return WINI_ERROR;

	DatasetNodeImpl* baby;

	baby = new DatasetNodeImpl(parent, &m_result, 0);

	parent->AddChild(baby);

	*child = baby;

	return WINI_OK;
#endif
	return WINI_ERROR;
}

#if 0
int CollectionOperatorImpl::UploadImpl(srbConn* bloadconn, char* local, char* collection, char* resource, bool isContainer)
{

    int c, ii;
    srbPathName nameArray[MAX_TOKEN], targNameArray[MAX_TOKEN];
    char contName[MAX_TOKEN], contCollection[MAX_TOKEN], inpCont[MAX_TOKEN];
  
    srbConn *conn;
    int status;
    int flagval = 0;
    char *userName, *domainName;
    char containerName[MAX_TOKEN];
    char *mcatName = NULL;
    char myMcatName[MAX_TOKEN];
    int copyNum = -1;

    strcpy(inCondition , "");

	if(isContainer)
	{
		strcpy (inpCont, resource);
		strcpy (ResourceName, "");
	}else
	{

		strcpy (ResourceName, resource);
	}

	targNameArray[0].inpArgv = collection;
	strcpy (datatype, "");
	strcpy (newpathname, "");

    nameArray[0].inpArgv = strdup(local);
    SrbNtPathForwardSlash(nameArray[0].inpArgv);

    /* Convert the target name to SQL like */
    if (convNameArrayToSQL (targNameArray, 1) < 0)
      {clFinish(conn); exit(4);}

    if (isContainer)
	{         /* user container */
    	if (conn->clientUser == NULL) {
            userName = conn->proxyUser->userName;
            domainName = conn->proxyUser->domainName;
    	} else {
            userName = conn->clientUser->userName;
            domainName = conn->clientUser->domainName;
    	}


   
                char *thisMcat = NULL;

                thisMcat = getmcatNamefromHint (targNameArray[0].outArgv);
                if (thisMcat == NULL) {
                    status =srbGetMcatZone (conn, userName, domainName,
                     myMcatName);
                    if (status >= 0)
                        mcatName = myMcatName;
                    else
                        mcatName = NULL;
                } else if (strcmp (thisMcat, "home") == 0 ||
                 strcmp (thisMcat, "container") == 0) {
                    mcatName = NULL;
                } else {
                    mcatName = thisMcat;
                }
           


        parseContainerName (inpCont, contName, contCollection, userName, domainName, mcatName);
        sprintf (containerName, "%s/%s", contCollection, contName);
		strcpy (ContainerName, containerName); /*XXXX used in bload.take out */ 
    }else
	{
		containerName[0] = '\0';
    }

    /* set the target resource */

    if (strlen(ResourceName) == 0) {
        get_user_preference(ResourceName, "DRSRC");
    }


	char *myHost;
	srbConn *myConn;

        myHost = getHostByResource (bloadconn, ResourceName, mcatName);
        if (myHost != NULL)
		{
            if (strncmp (srbHost, myHost, strlen (srbHost)) != 0)
			{
                /* different host */
                myConn = srbConnect (myHost, NULL, srbAuth,NULL, NULL, NULL, NULL);
                if (clStatus(myConn) == CLI_CONNECTION_OK)
				{
                    clFinish (conn);
                    conn = myConn;
                    strcpy (srbHost, myHost);
                }
            }
        }
 
		m_bytes_received = 0;
		m_bytes_expected = 0;

      ii = bloadMainBC (bloadconn, MDAS_CATALOG, 1, nameArray, targNameArray,
       flagval, &m_bytes_received, &m_bytes_expected);


 
    if (ii >= 0 && RStatus >= 0)
      return 0;
   
	return 1;

	return WINI_ERROR;
}






int CollectionOperatorImpl::DownloadImpl(srbConn* bdown_conn, const char* local, const char* source, int copyNum, srb_long_t* bytes_received, srb_long_t* bytes_expected)
{

    int  newargc;
   
    int c;
    int flagval = 0;
    srbPathName targNameArray[MAX_TOKEN], srcNameArray[MAX_TOKEN];
    int nArgv;

	nArgv = 1;
  
    FILE *fd;
    int status;
    char ticketId[MDAS_TICKET_SIZE+10];
   
    int i, ii;
   
    int option_index = 0;

    iGlbRetriesMax=(-1);
    strcpy(ticketId , "");
    strcpy(inCondition , "");
    lcval = 0;

    flagval |= b_FLAG;
    
	targNameArray[0].inpArgv = (char*)local;
  
    srcNameArray[0].inpArgv = (char*)source;
	srcNameArray[0].type = COLLECTION_T;
	srcNameArray[0].outArgv = (char*)source;

    if (convNameArrayToSQL (srcNameArray, 0) < 0)
		return -1;

    ii= getMainBC (bdown_conn, 0, nArgv, srcNameArray, targNameArray, flagval, copyNum, ticketId, bytes_received, bytes_expected);
 
	if(ii >= 0 && RStatus >= 0)
		return 0;
	else
		return -1;

	return WINI_ERROR;
}



#endif


}//end namespace

