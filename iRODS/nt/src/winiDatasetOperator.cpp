#include "winiDatasetOperator.h"
#include "winiDataset.h"
#include "winiMetadata.h"
#include "winiUser.h"
#include "winiAccess.h"
#include "winiCollection.h"
#include "rodsGenQuery.h"
#include "rodsPath.h"
#include "parseCommandLine.h"
#include "getUtil.h"
#include "dataObjRename.h"
#include "mvUtil.h"
#include "getUtil.h"



namespace WINI
{

DatasetOperatorImpl::DatasetOperatorImpl(ISession* session)
{
//	m_result.result_count = 0;
	//m_result.row_count = 0;
	m_session = session;
	m_conn = (rcComm_t*)session->GetConn();
	//m_bytes_received = 0;
	//m_bytes_expected = 0;
	m_bIsDownloading = false;
}

DatasetOperatorImpl::~DatasetOperatorImpl()
{
	m_session = NULL;	//just do something
}

StatusCode DatasetOperatorImpl::ModifyAccess(IAccessNode* target, const char* permission)
{
	return WINI_ERROR;
#if 0
	if(NULL == target || NULL == permission)
		return WINI_ERROR_INVALID_PARAMETER;

	IStringNode* constraints = m_session->GetAccessConstraints();

	int count = constraints->CountStrings();
	int i;
	for(i = 0; i < count; i++)
	{
		if(0 == strcmp(constraints->GetString(i), permission))
			break;
	}

	if(count == i)
		return WINI_ERROR_INVALID_PARAMETER;

	char* sz;

	char* dataset = (char*)target->GetParent()->GetName();
	char* collection = (char*)target->GetParent()->GetParent()->GetPath();
	char* user = (char*)target->GetUser();
	char* domain = (char*)target->GetDomain();
	char* buf;
	StatusCode status;

	int retraction_type;
	int len = 0;

	if(0 == strcmp("null", permission))
	{
		retraction_type = D_DELETE_ACCS;
		sz = "";
	}
	else
	{
		retraction_type = D_INSERT_ACCS;
		sz = (char*)permission;
	}
	//(char*)((IDatasetNode*)target->GetParent())->GetOwner()
	len += strlen(user);
	len += strlen(domain);
	len += 2;
	buf = (char*)calloc(len, sizeof(char));
	sprintf(buf, "%s@%s", user, domain);
	status = srbModifyDataset(m_conn, 0, dataset, collection, "", "", buf, sz, retraction_type);
	free(buf);
	
	WINI::INode* parent;

	if(status.isOk())
	{
		if(D_DELETE_ACCS == retraction_type)
		{
			parent = target->GetParent();

			switch(parent->GetType())
			{
			case WINI_COLLECTION:
				status = ((CollectionNodeImpl*)parent)->DeleteChild(target);
				break;
			case WINI_DATASET:
				status = ((DatasetNodeImpl*)parent)->DeleteChild(target);
				break;
			default:
				((AccessNodeImpl*)target)->SetPermission(permission);
			}
		}else
		{
			((AccessNodeImpl*)target)->SetPermission(permission);
		}
	}

	return status;
#endif

}

StatusCode DatasetOperatorImpl::SetAccess(IDatasetNode* target, IUserNode* owner, const char* permission)
{
	return WINI_ERROR;
#if 0
	if(NULL == target || NULL == owner || NULL == permission)
		return WINI_ERROR_INVALID_PARAMETER;

	//do operation
	int retraction_type;
	StatusCode status;
	char* sz;

	if(0 == strcmp("null", permission))
	{
		retraction_type = D_DELETE_ACCS;
		sz = "";
	}
	else
	{
		retraction_type = D_INSERT_ACCS;
		sz = (char*)permission;
	}

	status = srbModifyDataset(m_conn, 0, (char*)target->GetName(), (char*)target->GetParent()->GetPath(), "", "", (char*)owner->GetPath(), sz, retraction_type);

	INode* child;
	int count;
	
	if(status.isOk())
	{
		count = target->CountChildren();

		if(count)
		{
			for(i = 0; i < count; i++)
			{
				child = target->GetChild(i);
				if(WINI_ACCESS == child->GetType())
				{
					if(0 == strcmp(owner->GetName(), child->GetName()))
					{
						((AccessNodeImpl*)child)->SetPermission(permission);
						break;
					}
				}
			}

			if(i == count)
			{
				//a new user was added
				((DatasetNodeImpl*)target)->AddChild(new AccessNodeImpl(target, NULL, owner->GetParent()->GetName(), owner->GetName(), permission));
			}
		}else
		{
			if(D_INSERT_ACCS == retraction_type)
			{
				//insert new access - this handles case where user deletes all access permissions and is inserting a first child
				((DatasetNodeImpl*)target)->AddChild(new AccessNodeImpl(target, NULL, owner->GetParent()->GetName(), owner->GetName(), permission));
			}
		}
	}
	return status;
#endif
}

//handle the name parameter later
int DatasetOperatorImpl::GetProgress(char** name)
{
	return -1;
#if 0
	if(0 == m_bytes_expected)
		return 0;

	if(!m_bIsDownloading)
		return -1;

	double retval = m_bytes_received;
	retval = (retval / m_bytes_expected) * 100;

	return retval;
#endif
}


StatusCode DatasetOperatorImpl::Download(INode* target, const char* local_path)
{
	if(NULL == local_path)
		return WINI_ERROR_INVALID_PARAMETER;

	if(WINI_DATASET != target->GetType())
		return WINI_ERROR_INVALID_PARAMETER;

	m_bIsDownloading = true;

	IDatasetNode* target_dataset = (IDatasetNode*)target;

	m_bytes_expected = atoi(((IDatasetNode*)target)->GetSize());

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


	rodsPathInp.srcPath->objType = DATA_OBJ_T;
	rodsPathInp.srcPath->objState = EXIST_ST;
	sprintf(rodsPathInp.srcPath->inPath, "%s", target->GetName());	//$4 = "hh", '\0' <repeats 1021 times>
	sprintf(rodsPathInp.srcPath->outPath, "%s", target->GetPath());	//$5 = "/tempZone/home/rods/hh", '\0' <repeats 1001 times>
	rodsPathInp.destPath->objType = LOCAL_DIR_T;
	rodsPathInp.destPath->objState = EXIST_ST;
	sprintf(rodsPathInp.destPath->inPath, "test"); //$11 = "." '\0' <repeats 1022 times>
	sprintf(rodsPathInp.destPath->outPath, "/test"); //$12 = ".", '\0' <repeats 1022 times>
	//rodsPathInp.targPath.objType $15 = UNKNOWN_OBJ_T
	//rodsPathInp.targPath.objState $16 = UNKNOWN__ST

	status = getUtil(m_conn, &myRodsEnv, &myRodsArgs, &rodsPathInp);

	m_bIsDownloading = false;

	m_bytes_received = 0;
	m_bytes_expected = 0;

    return status;
}

StatusCode DatasetOperatorImpl::Bind(INode* node) { m_binding = node; return WINI_OK;};

StatusCode DatasetOperatorImpl::Upload(INode* target, const char* local_path, unsigned int overwrite, INode** result)
{
		//CHARLIECHARLIE
	return WINI_ERROR;
#if 0
	if(WINI_COLLECTION != target->GetType())
		return WINI_ERROR_INVALID_PARAMETER;

    srbPathName srbPathName;
    int flagval = 0;
	int target_type = -1;

    srbPathName.inpArgv = (char*)local_path;

    /* fill in size, etc */
	if(-1 == chkFileName(&srbPathName))
		return WINI_ERROR_DOES_NOT_EXIST;

	if(overwrite & WINI_OVERWRITE)
	{
		flagval = F_FLAG;
		target_type = DATANAME_T;
		if((overwrite & WINI_OVERWRITE_ALL) == WINI_OVERWRITE_ALL)
			flagval |= a_FLAG;
	}else
	{
		flagval = 0;
		target_type = -1;
	}

	char* resource;
	char* container;

	switch(m_binding->GetType())
	{
	case WINI_RESOURCE:
		resource = (char*)m_binding->GetName();
		container = NULL;
		break;
	case WINI_CONTAINER:
		resource = NULL;
		container = (char*)m_binding->GetName();
		flagval |= c_FLAG;
		break;
	default:
		return WINI_ERROR_INVALID_BINDING;
	}

	int len = strlen(local_path);

	const char* name = NULL;

	for(int i = len - 1; i != 0; i--)
	{
		if('\\' == local_path[i])
		{
			//found the beginning of our name
			name = &local_path[i+1];
			break;
		}
	}

	if(NULL == name)
		name = local_path;

	m_bytes_expected = srbPathName.size;

	m_bIsDownloading = true;


    StatusCode status = fileToDataCopyBC(m_conn, 0, flagval, target_type, &srbPathName, (char*)target->GetPath(), (char*)name, "", "", resource, container, 0, &m_bytes_received);

	m_bIsDownloading = false;

    /* clean up */
    free (srbPathName.outArgv);

	INode* blarg;

	

	if(status.isOk())
	{
		CollectionNodeImpl* collection = (CollectionNodeImpl*)target;

		if(overwrite)
		{
			//new datasets were not created, but the overwritten dataset(s)
			//contain some invalid data.
			//status = Wash(collection, name);

			//for 118p, only one dataset exists so then it's easier to wash
			status = Wash(collection, name);

		}else
		{
			//a new dataset must have been created
			status = Fill(collection, &blarg, name);
		}
	}

	m_bytes_received = 0;
	m_bytes_expected = 0;

    return status;
#endif

}

//faster wash reserved for 118p's single overwrite only
StatusCode DatasetOperatorImpl::Wash(CollectionNodeImpl* collection, const char* dataset_name)
{
	return WINI_ERROR;
#if 0
	if(NULL == collection || NULL == dataset_name)
		return WINI_ERROR_INVALID_PARAMETER;

	//1st get query for all datasets matching that name
	ClearMCATScratch();

    m_selval[SIZE] = 1;
    m_selval[DATA_TYP_NAME] = 1;
	m_selval[DATA_OWNER] = 1;
	m_selval[REPL_TIMESTAMP] = 1;
	m_selval[DATA_REPL_ENUM] = 1;
	m_selval[PHY_RSRC_NAME] = 1;
	m_selval[CONTAINER_NAME] = 1;
	m_selval[DATA_COMMENTS] = 1;

	m_selval[DATA_GRP_NAME] = 1;
	m_selval[DATA_NAME] = 1;

    sprintf(m_qval[DATA_GRP_NAME]," = '%s'", collection->GetPath());
    sprintf(m_qval[DATA_NAME]," = '%s'", dataset_name);

	
    StatusCode status = srbGetDataDirInfoWithZone(m_conn, 0, (char*)m_session->GetCurrentZone(collection)->GetName(), m_qval, m_selval, &m_result, MAX_ROWS);

	if(!status.isOk())
	{
		if(-3005 == status)
			return WINI_OK;
		return status;
	}

	filterDeleted(&m_result);

	if(m_result.continuation_index >= 0)
		return WINI_ERROR;

	if(m_result.row_count != 1)
		return WINI_ERROR;

	INode* node;
	for(int i = 0; i < collection->CountChildren(); i++)
	{
		node = collection->GetChild(i);
			if(WINI_DATASET == node->GetType())
				if(0 == strcmp(dataset_name, node->GetName()))
					break;
	}

	if(i == collection->CountChildren())
		return WINI_ERROR;

	char *size, *owner, *resource, *time, *container, *type, *comment;
	char* szptr = getFromResultStruct(&m_result, dcs_tname[DATA_REPL_ENUM], dcs_aname[DATA_REPL_ENUM]);

	if(0 != strcmp(((IDatasetNode*)node)->GetReplicationIndex(), szptr))
		return WINI_ERROR;

	size = getFromResultStruct(&m_result, dcs_tname[SIZE], dcs_aname[SIZE]);
	owner = getFromResultStruct(&m_result, dcs_tname[DATA_OWNER], dcs_aname[DATA_OWNER]);
	time = getFromResultStruct(&m_result, dcs_tname[REPL_TIMESTAMP], dcs_aname[REPL_TIMESTAMP]);
	resource = getFromResultStruct(&m_result, dcs_tname[PHY_RSRC_NAME], dcs_aname[PHY_RSRC_NAME]);
	container = getFromResultStruct(&m_result, dcs_tname[CONTAINER_NAME], dcs_aname[CONTAINER_NAME]);
	comment = getFromResultStruct(&m_result, dcs_tname[DATA_COMMENTS], dcs_aname[DATA_COMMENTS]);
	type = getFromResultStruct(&m_result, dcs_tname[DATA_TYP_NAME], dcs_aname[DATA_TYP_NAME]);
	((DatasetNodeImpl*)node)->Wash(size, owner, time, szptr, "", resource, container, type, comment);

	return WINI_OK;
#endif
}


#if 0
{
	if(NULL == collection || NULL == dataset_name)
		return WINI_ERROR_INVALID_PARAMETER;

	//1st get query for all datasets matching that name
	ClearMCATScratch();

    m_selval[SIZE] = 1;
    m_selval[DATA_TYP_NAME] = 1;
	m_selval[DATA_OWNER] = 1;
	m_selval[REPL_TIMESTAMP] = 1;
	m_selval[DATA_REPL_ENUM] = 1;
	m_selval[PHY_RSRC_NAME] = 1;
	m_selval[CONTAINER_NAME] = 1;
	m_selval[DATA_COMMENTS] = 1;

	m_selval[DATA_GRP_NAME] = 1;
	m_selval[DATA_NAME] = 1;

    sprintf(m_qval[DATA_GRP_NAME]," = '%s'", collection->GetPath());
    sprintf(m_qval[DATA_NAME]," = '%s'", dataset_name);

     StatusCode status = srbGetDataDirInfoWithZone(m_conn, 0, (char*)m_session->GetCurrentZone(target)->GetName(), m_qval, m_selval, &m_result, MAX_ROWS);

	if(!status.isOk())
	{
		if(-3005 == status)
			return WINI_OK;
		return status;
	}

	filterDeleted(&m_result);

	if(m_result.continuation_index >= 0)
		return WINI_ERROR;

	//2nd make a stack of all children in the collection with that name
	std::vector<INode*> v;
	v.reserve(m_result.row_count);

	INode* node;
	for(int i = 0; i < collection->CountChildren(); i++)
	{
		node = collection->GetChild(i);
			if(WINI_DATASET == node->GetType())
				if(0 == strcmp(dataset_name, node->GetName()))
					v.push_back(node);
	}

	if(m_result.row_count != v.size())
		return WINI_ERROR;

	//3rd for each result
	//	a) match the result to a dataset in the stack by replication index
	//	b) DatasetNodeImpl::wash the dataset with the result data

	std::vector<INode*>::iterator iv;

	char* kkkk;
	char* jjjj;
	char *size, *owner, *resource, *time, *container, *type, *comment;
	char* szptr = getFromResultStruct(&m_result, dcs_tname[DATA_REPL_ENUM], dcs_aname[DATA_REPL_ENUM]);
	szptr -= MAX_DATA_SIZE;

	for(i = 0; i < m_result.row_count; i++)
	{
		szptr += MAX_DATA_SIZE;

		iv = v.begin();
		
		do
		{
			if(0 == strcmp(((IDatasetNode*)*iv)->GetReplicationIndex(), szptr))
			{
				jjjj = getFromResultStruct(&m_result, dcs_tname[DATA_NAME], dcs_aname[DATA_NAME]);
				kkkk = getFromResultStruct(&m_result, dcs_tname[DATA_GRP_NAME], dcs_aname[DATA_GRP_NAME]);
				size = getFromResultStruct(&m_result, dcs_tname[SIZE], dcs_aname[SIZE]);
				owner = getFromResultStruct(&m_result, dcs_tname[DATA_OWNER], dcs_aname[DATA_OWNER]);
				time = getFromResultStruct(&m_result, dcs_tname[REPL_TIMESTAMP], dcs_aname[REPL_TIMESTAMP]);
				resource = getFromResultStruct(&m_result, dcs_tname[PHY_RSRC_NAME], dcs_aname[PHY_RSRC_NAME]);
				container = getFromResultStruct(&m_result, dcs_tname[CONTAINER_NAME], dcs_aname[CONTAINER_NAME]);
				comment = getFromResultStruct(&m_result, dcs_tname[DATA_COMMENTS], dcs_aname[DATA_COMMENTS]);
				type = getFromResultStruct(&m_result, dcs_tname[DATA_TYP_NAME], dcs_aname[DATA_TYP_NAME]);
				size += i * MAX_DATA_SIZE;
				owner += i * MAX_DATA_SIZE;
				resource += i * MAX_DATA_SIZE;
				time += i * MAX_DATA_SIZE;
				container += i * MAX_DATA_SIZE;
				comment += i * MAX_DATA_SIZE;
				type += i * MAX_DATA_SIZE;

				((DatasetNodeImpl*)*iv)->Wash(size, owner, time, szptr, "", resource, container, type, comment);
				v.erase(iv);
				break;	//you need to break to optimize and because iv will become invalid
			}
		} while(iv != v.end());
	}

	return WINI_OK;
}
#endif

StatusCode DatasetOperatorImpl::Fill(CollectionNodeImpl* parent, INode** child, const char* name)
{
	return WINI_ERROR;
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

	//while(m_result.continuation_index >= 0)
	//{
		//add code for this later	
	//}

	*child = baby;

	return WINI_OK;
#endif
}

StatusCode DatasetOperatorImpl::Delete(IDatasetNode* target)
{
	dataObjInp_t dataObjInp;

	memset((void*)&dataObjInp, 0, sizeof(dataObjInp_t));
	rstrcpy (dataObjInp.objPath, (char*)target->GetPath(), MAX_NAME_LEN);
	WINI::StatusCode status = rcDataObjUnlink(m_conn, &dataObjInp);
	//TODO handle replication number

	WINI::CollectionNodeImpl* parent = (CollectionNodeImpl*)target->GetParent();

	if(status.isOk())
		parent->DeleteChild(target);

	return status;

}

StatusCode DatasetOperatorImpl::Replicate(IDatasetNode* target)
{
	return WINI_ERROR;
#if 0
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	int replIn;

	char* szContainer = (char*)target->GetContainer();
	if(0 == strcmp("",szContainer))
	{
		replIn = atoi(target->GetReplicationIndex());
	}else
	{
		replIn = -1;
	}

	char buf[MAX_TOKEN];

    sprintf(buf, "%s&COPY=%i", target->GetName(), replIn);

    StatusCode status = srbObjReplicate(m_conn, 0, (char*)target->GetName(), (char*) target->GetParent()->GetPath(), (char*)m_binding->GetName(), "");

	if(status.isOk())
	{

		status = GetReplicant((ICollectionNode*)target->GetParent(), target->GetName());
	}

	return status;
#endif
}

StatusCode DatasetOperatorImpl::GetReplicant(ICollectionNode* target, const char* name)
{
	return WINI_ERROR;
#if 0
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

    sprintf(m_qval[DATA_GRP_NAME]," = '%s'", target->GetPath());
	sprintf(m_qval[DATA_NAME], " = '%s'", name);
	//sprintf(m_qval[DATA_REPL_ENUM], " = '%d'", repl);

    StatusCode status = srbGetDataDirInfoWithZone(m_conn, 0, (char*)m_session->GetCurrentZone(target)->GetName(), m_qval, m_selval, &m_result, MAX_ROWS);

	if(!status.isOk())
	{
		if(-3005 == status)
			return WINI_OK;
		return status;
	}

	filterDeleted(&m_result);

	DatasetNodeImpl* baby;

	INode* node;


	char* repl;

	bool found;
	for(int j = 0; j < m_result.row_count; j++)
	{
		found = false;

		repl = getFromResultStruct(&m_result,dcs_tname[DATA_REPL_ENUM], dcs_aname[DATA_REPL_ENUM]);
		repl += j * MAX_DATA_SIZE;

		for(int i = 0; i < target->CountChildren(); i++)
		{
			node = target->GetChild(i);
			if(WINI_DATASET == node->GetType())
			{
				if(0 == strcmp(name, node->GetName()))
				{
					if(0 == strcmp(repl, ((IDatasetNode*)node)->GetReplicationIndex()))
					{
						found = true;
						break;
					}
				}
			}
		}

		if(!found)
		{
			baby = new DatasetNodeImpl(target, &m_result, j);
			((CollectionNodeImpl*)target)->AddChild(baby);
		}
	}

	return WINI_OK;
#endif
}

StatusCode DatasetOperatorImpl::GetChildren(DatasetNodeImpl* target, unsigned int mask)
{
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	target->Clear();

	unsigned int success = WINI_ALL ^ (WINI_ACCESS | WINI_METADATA);

#if 1
	target->SetOpen(WINI_ALL);
	return WINI_OK;
#endif

	StatusCode status;

	if(WINI_ACCESS & mask)
	{
		status = GetAccess(target);
		if(status.isOk())
			success |= WINI_ACCESS;
		else
		{
			target->SetOpen(success);
			return status;
		}
	}

	if(WINI_METADATA & mask)
	{
		status = GetMetadata(target);
		if(status.isOk())
			success |= WINI_METADATA;
		else
		{
			target->SetOpen(success);
			return status;
		}
	}

	target->SetOpen(success);
	return status;
}

StatusCode DatasetOperatorImpl::GetAccess(DatasetNodeImpl* target)
{
	return WINI_ERROR;
#if 0
	ClearMCATScratch();

	WINI::INode* blah = target->GetParent();

	sprintf(m_qval[DATA_GRP_NAME], " = '%s'", blah->GetPath());
	sprintf(m_qval[DATA_NAME], " = '%s'", target->GetName());

	m_selval[USER_NAME] = 1;
	m_selval[DOMAIN_DESC] = 1;
	m_selval[ACCESS_CONSTRAINT] = 1;

    StatusCode status = srbGetDataDirInfoWithZone(m_conn, 0, (char*)m_session->GetCurrentZone(target)->GetName(), m_qval, m_selval, &m_result, MAX_ROWS);

	if(!status.isOk())
	{
		if(-3005 == status)
			status = WINI_OK;

		return status;
	}

	if(0 == m_result.row_count)
		return WINI_OK;

	filterDeleted(&m_result);

	IAccessNode* access_node;
	

	char *user, *domain, *constraint;
	
	user = getFromResultStruct(&m_result, dcs_tname[USER_NAME], dcs_aname[USER_NAME]);
	domain = getFromResultStruct(&m_result, dcs_tname[DOMAIN_DESC], dcs_aname[DOMAIN_DESC]);
	constraint = getFromResultStruct(&m_result, dcs_tname[ACCESS_CONSTRAINT], dcs_aname[ACCESS_CONSTRAINT]);
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
}

StatusCode DatasetOperatorImpl::GetMetadata(DatasetNodeImpl* target)
{
	return WINI_ERROR;
#if 0
	ClearMCATScratch();

	WINI::INode* blah = target->GetParent();

	sprintf(m_qval[DATA_GRP_NAME], " = '%s'", blah->GetPath());
	sprintf(m_qval[DATA_NAME], " = '%s'", target->GetName());
	sprintf(m_qval[METADATA_NUM], " >= 0");

	m_selval[UDSMD0] = 1;
	m_selval[UDSMD1] = 1;
	m_selval[METADATA_NUM] = 1;

    StatusCode status = srbGetDataDirInfoWithZone(m_conn, 0, (char*)m_session->GetCurrentZone(target)->GetName(), m_qval, m_selval, &m_result, MAX_ROWS);

	if(!status.isOk())
	{
		if(-3005 == status)
			status = WINI_OK;

		return status;
	}

	filterDeleted(&m_result);

	MetadataNodeImpl* meta;

	char *attribute, *value, *ID;
	
	for(int i = 0; i < m_result.row_count; i++)
	{
		attribute = getFromResultStruct(&m_result, dcs_tname[UDSMD0], dcs_aname[UDSMD0]);
		attribute += i * MAX_DATA_SIZE;
		value = getFromResultStruct(&m_result, dcs_tname[UDSMD1], dcs_aname[UDSMD1]);
		value += i * MAX_DATA_SIZE;
		ID = getFromResultStruct(&m_result, dcs_tname[METADATA_NUM], dcs_aname[METADATA_NUM]);
		ID += i * MAX_DATA_SIZE;

		meta = new MetadataNodeImpl(target, attribute, WINI_MD_EQUAL, value);
		meta->SetID(atoi(ID));

		target->AddChild(meta);
	}

	//while(m_result.continuation_index >= 0)
	//{
		//implement later
	//}

	return WINI_OK;
#endif
}

StatusCode DatasetOperatorImpl::AddMeta(IDatasetNode* target, const char* attribute, IMetadataNode** result = 0)
{
	return WINI_ERROR;
#if 0
	if(NULL == target || NULL == attribute)
		return WINI_ERROR_INVALID_PARAMETER;

	int id = srbModifyDataset(m_conn, 0, (char*)target->GetName(),(char*)target->GetParent()->GetPath(),"","","0",(char*)attribute, D_INSERT_USER_DEFINED_STRING_META_DATA);

	DatasetNodeImpl* dataset = (DatasetNodeImpl*)target;

	MetadataNodeImpl* baby = new MetadataNodeImpl(target, attribute, WINI_MD_EQUAL, NULL);
	baby->SetID(id);

	dataset->AddChild(baby);

	if(NULL != result)
		*result = baby;

	return WINI_OK;
#endif
}

StatusCode DatasetOperatorImpl::ModifyMeta(IMetadataNode* target, const char* value)
{
	return WINI_ERROR;
#if 0
	if(NULL == target || NULL == value)
		return WINI_ERROR_INVALID_PARAMETER;

	IDatasetNode* dataset = (IDatasetNode*)target->GetParent();
	ICollectionNode* collection = (ICollectionNode*)dataset->GetParent();

	static char buf[256];

	sprintf(buf, "1@%d", ((MetadataNodeImpl*)target)->GetID());
	//StatusCode status = srbModifyDataset(m_conn, 0,(char*)dataset->GetName(),(char*)collection->GetPath(),buf,"", (char*)value, "", D_CHANGE_USER_DEFINED_STRING_META_DATA);
	StatusCode status = srbModifyDataset(m_conn, 0, (char*)dataset->GetName(), (char*)collection->GetPath(), "", "", buf, (char*)value, D_CHANGE_USER_DEFINED_STRING_META_DATA);

	if(status.isOk())
		((MetadataNodeImpl*)target)->SetValue(value);

	return status;
#endif
}

StatusCode DatasetOperatorImpl::DeleteMeta(IMetadataNode* target)
{
	return WINI_ERROR;
#if 0
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	DatasetNodeImpl* parent = (DatasetNodeImpl*)target->GetParent();

	static char buf[10];

	sprintf(buf, "%d", ((MetadataNodeImpl*)target)->GetID());

	StatusCode status = srbModifyDataset(m_conn, 0, (char*)parent->GetName(), (char*)parent->GetParent()->GetPath(),"","",buf,"",D_DELETE_USER_DEFINED_STRING_META_DATA);

	if(status.isOk())
		parent->DeleteChild(target);

	return status;
#endif
}
#if 0
void makesafeWINIstring(char* original, char* copy)
{
	len = strlen(original);
	
	while
}
#endif

StatusCode DatasetOperatorImpl::Rename(IDatasetNode* target, const char* name)
{
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
		((DatasetNodeImpl*)target)->SetName(name);

	return status;
};

StatusCode DatasetOperatorImpl::SetComment(IDatasetNode* target, const char* comment)
{
	return WINI_ERROR;
#if 0
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	int retractionType;

	if(0 == strcmp("", target->GetComment()))
		retractionType = D_INSERT_COMMENTS;
	else
		if(NULL == comment || '\0' == comment[0])
			retractionType = D_DELETE_COMMENTS;			//can we supply modifydataset with NULL? or does it have to be ""?
		else
			retractionType = D_UPDATE_COMMENTS;

	StatusCode status = srbModifyDataset(m_conn, 0, (char*)target->GetName(), (char*)target->GetParent()->GetPath(), "", "", (char*)comment,"", retractionType);

	if(status.isOk())
	{
		((DatasetNodeImpl*)target)->SetComment(comment);
	}

	return status;
#endif
};
}//end namespace
	
