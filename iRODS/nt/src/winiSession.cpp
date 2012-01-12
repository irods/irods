#include "winiDatasetOperator.h"
#include "winiQueryOperator.h"
#include "winiResourceOperator.h"
#include "winiSession.h"
#include "winiCatalog.h"
#include "winiCatalogOperator.h"
#include "winiCollectionOperator.h"
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "lsUtil.h"

//#include "winiZoneOperator.h"
//#include "winiICATOperator.h"
//#include "winiString.h"

namespace WINI
{
SessionImpl::SessionImpl()
{
	m_user = NULL;
	m_host = NULL;
	m_auth = NULL;
	m_port = NULL;
	m_password = NULL;
	m_conn = NULL;
	m_root = NULL;
	m_home = NULL;
	m_resource = NULL;
	m_ResourceNode = NULL;

	m_datasetOp = NULL;
	m_CollectionOp = NULL;
	m_ResourceOp = NULL;
	m_queryOp = NULL;

	m_ZoneOp = NULL;
	m_Catalog = NULL;
	m_bMaster = true;
	m_constraint_node = NULL;
	m_HomeZone = NULL;
	m_bisConnected = false;
	m_user_home = NULL;
	m_zones = NULL;
}

SessionImpl::~SessionImpl()
{
}

void SessionImpl::Clean()
{
/*
	Disconnect();

//	SMART_DELETE(m_CollectionOp);
//	SMART_DELETE(m_ResourceOp);
//	SMART_DELETE(m_datasetOp);
//	SMART_DELETE(m_queryOp);
//	SMART_DELETE(m_containerOp);
//	SMART_DELETE(m_ZoneOp);

//	if(m_domainOp)
//	{
//		delete m_domainOp;
//		m_domainOp = NULL;
	}

	IOperator* blah;

	for(int i = 0; i < m_allocations.size(); i++)
	{
		blah = m_allocations.at(i);
		delete blah;
	}

	INode* blah2;

	for(i = 0; i < m_allocations2.size(); i++)
	{
		blah2 = m_allocations2.at(i);
		delete blah2;
	}

	if(m_bMaster)
	{
		SMART_FREE(m_user);
		SMART_FREE(m_host);
		SMART_FREE(m_domain);
		SMART_FREE(m_auth);
		SMART_FREE(m_port);
		SMART_FREE(m_password);
		SMART_FREE(m_user_home);
		if(m_root)
		{
			delete m_root;
			m_root = NULL;
		}
		if(m_constraint_node)
		{
			delete m_constraint_node;
			m_constraint_node = NULL;
		}
	}
*/
}

StatusCode SessionImpl::Init(const char* host, int port, const char* user, const char* zone, const char* password)
{
	if(NULL == user || NULL == port || NULL == host || NULL == password)
		return WINI_ERROR_INVALID_PARAMETER;

	if(m_bisConnected)					
		Clean();

	m_user = strdup(user);
	m_host = strdup(host);

	m_auth = NULL;
	m_port = port;
	m_password = strdup(password);
	m_user_home = NULL;
	m_szZone = strdup(zone);

	StatusCode status = Connect();

	if(!status.isOk())
		return status;

	//create MCAT root node
	//get Zones (children of MCAT)

	m_Catalog = new CatalogNodeImpl("iCAT Catalog");
	CatalogOperatorImpl* CatalogOp = new CatalogOperatorImpl(this);
	status = CatalogOp->GetChildren((CatalogNodeImpl*)m_Catalog);
	if(!status.isOk())
		return status;

	//create a new zone operator
	m_ZoneOp = new ZoneOperatorImpl(this);

	//locate user's zone
	status = m_Catalog->GetChild("tempZone", (WINI::INode**)&m_HomeZone);
	if(!status.isOk())
		return WINI_ERROR;

	if(!m_HomeZone)
		return WINI_ERROR;

	//we'll need the root collection, the user's home collection,
	//and the resource list.
	//everything else can remain closed.
	m_ZoneOp->GetChildren((ZoneNodeImpl*)m_HomeZone, WINI_ALL ^ WINI_DOMAIN);

	m_ResourceOp = new ResourceOperatorImpl(this);
	m_CollectionOp = new CollectionOperatorImpl(this);

	m_ResourceNode = (ISetNode*)m_HomeZone->GetChild("resources");

	WINI::ISetNode* Collections = (ISetNode*)m_HomeZone->GetChild("collections");

	m_root = (ICollectionNode*)Collections->GetChild("home");
	m_CollectionOp->GetChildren((CollectionNodeImpl*)m_root);

	m_home = (ICollectionNode*)m_root->GetChild("rods");
	m_CollectionOp->GetChildren((CollectionNodeImpl*)m_home);

	m_datasetOp = new DatasetOperatorImpl(this);

	return WINI_OK;
}


IOperator* SessionImpl::GetOperator(int type)
{
	switch(type)
	{
	case WINI_COLLECTION:
		return m_CollectionOp;
	case WINI_DATASET:
		if(NULL == m_datasetOp)
			m_datasetOp = new DatasetOperatorImpl(this);
		return m_datasetOp;
	case WINI_QUERY:
		if(NULL == m_queryOp)
			m_queryOp = new QueryOperatorImpl(this);
		return m_queryOp;
	case WINI_RESOURCE:
		return m_ResourceOp;
	}
	return NULL;
}

StatusCode SessionImpl::Clone(ISession* target)
{
	((SessionImpl*)target)->m_conn = Clone();

	if(NULL == ((SessionImpl*)target)->m_conn)
	{
		//clFinish(((SessionImpl*)target)->m_conn);

		//clearing this allows us to re-srbConnect a session...
		//free(DefServerAuthInfo);
		//DefServerAuthInfo = NULL;

		return WINI_ERROR;
	}

	((SessionImpl*)target)->m_bisConnected = true;
	((SessionImpl*)target)->m_user		= m_user;	
	((SessionImpl*)target)->m_host		= m_host;	
	
	((SessionImpl*)target)->m_auth		= m_auth;
	((SessionImpl*)target)->m_port		= m_port;
	((SessionImpl*)target)->m_password	= m_password;
	((SessionImpl*)target)->m_constraint_node = m_constraint_node;
	((SessionImpl*)target)->m_root = m_root;
	((SessionImpl*)target)->m_home = m_home;
	((SessionImpl*)target)->m_user_home = m_user_home;

	//have your own operators;
	((SessionImpl*)target)->m_CollectionOp = new CollectionOperatorImpl(this);
	((SessionImpl*)target)->m_datasetOp = new DatasetOperatorImpl(this);
	((SessionImpl*)target)->m_ResourceOp = new ResourceOperatorImpl(this);
	((SessionImpl*)target)->m_queryOp = new QueryOperatorImpl(this);
	((SessionImpl*)target)->m_bMaster = false;

	return WINI_OK;
}
	
void* SessionImpl::CloneConn()
{
	return (void*)Clone();
}


rcComm_t* SessionImpl::Clone()
{
	//srbConn* blah = srbConnect(m_host, m_port, m_password, m_user, m_domain, m_auth, "");

	StatusCode status;
	//status = blah->status;

	const char* text = status.GetError();

	return NULL;
}


ICollectionNode* SessionImpl::GetUserHomeCol()
{
	if(!m_home)
	{
		if(m_root)
			m_home = (ICollectionNode*)m_root->GetChild(m_user_home);
	}
	return m_home;
}

ICollectionNode* SessionImpl::GetUserRootCol()
{
	return m_root;
}

IZoneNode* SessionImpl::GetUserZone()
{
	return m_HomeZone;
}

ICatalogNode* SessionImpl::GetCatalog()
{
	return m_Catalog;
}

//Checked
StatusCode SessionImpl::Connect()
{
	WINI::StatusCode status;
	int s;

	if(NULL == m_host)
		return WINI_ERROR_INVALID_PARAMETER;

    rErrMsg_t errMsg;
   
	m_conn = rcConnect((char*)m_host, m_port, (char*)m_user, (char*)m_szZone, 0, &errMsg);

	if(NULL == m_conn)
	{
		//any cleanup?
		return WINI_ERROR;
	}

	if(0 != m_conn->status)
	{
		//cleanup?
		m_conn = NULL;
		return m_conn->status;
	}

    s = clientLoginWithPassword(m_conn, (char*)m_password);

	if(s < 0)
		return s;

	m_bisConnected = true;

	return s;
}

bool SessionImpl::isConnected()
{
	return m_bisConnected;
}

StatusCode SessionImpl::Disconnect()
{

	if(m_conn)
	{
		//clFinish(m_conn);
		//clearing this allows us to re-srbConnect a session...
		//free(DefServerAuthInfo);
		//DefServerAuthInfo = NULL;
		m_conn = NULL;
		m_bisConnected = false;
	}

	if(m_constraint_node)
	{
		delete m_constraint_node;
		m_constraint_node = NULL;
	}

	return WINI_OK;
}

const char* SessionImpl::GetUserHost(){return m_host;};
ICollectionNode* SessionImpl::GetUserHome(){return m_home;}

StatusCode SessionImpl::ChangePassword(const char* new_password)
{

	if(NULL == new_password)
		return WINI_ERROR_INVALID_PARAMETER;

	int localStatus = 0;

	//localStatus = srbModifyUser(m_conn, MDAS_CATALOG, (char*)new_password, "", U_CHANGE_PASSWORD);

	if(localStatus < 0)
		return localStatus;

	return 0;
}

StatusCode SessionImpl::OpenTree(INode* parent, int levels, bool clear, unsigned int mask)	//here or in .h set levels default to 0?
{
	if(NULL == parent)
		return WINI_ERROR_INVALID_PARAMETER;

 	if(0 == levels)		//this allows us to stop at zero. thus if they supply negative number, it will grab whole tree
		return WINI_OK;

	StatusCode status;

	//in future it might be better to have a basicOperatorImpl which all OperatorImpls inherit from

	switch(parent->GetType())
	{
	case WINI_COLLECTION:
		status = m_CollectionOp->GetChildren((CollectionNodeImpl*)parent, mask);	//assuming switch correctly maps opimpls to node interfaces (should since this is session function) each operator knows how to handle its own node type
		break;
	case WINI_DATASET:
		status = m_datasetOp->GetChildren((DatasetNodeImpl*)parent, mask);
		break;
	case WINI_METADATA:
		status = WINI_OK;
		//do nothing
		break;
	case WINI_QUERY:
		if(NULL == m_queryOp)
			m_queryOp = new QueryOperatorImpl(this);
		status = m_queryOp->GetChildren((QueryNodeImpl*)parent, mask);
		break;
	case WINI_RESOURCE:
		status = m_ResourceOp->GetChildren((ResourceNodeImpl*)parent, mask);
		break;
	case WINI_ZONE:
		status = m_ZoneOp->GetChildren((ZoneNodeImpl*)parent, clear, mask);
		break;
	default:
		return WINI_ERROR_INVALID_PARAMETER;
	}

	if(!status.isOk())
		return status;

	int count = parent->CountChildren();

	for(int i = 0; i < count, levels > 1; i++)
	{
		status = OpenTree(parent->GetChild(i), levels - 1, true, mask);
		if(!status.isOk())
			break;
	}

	return status;
}

StatusCode SessionImpl::CloseTree(INode* parent)
{
	if((ICollectionNode*)parent == m_root)
		m_home = NULL;
	switch(parent->GetType())
	{
	case WINI_COLLECTION:
		((CollectionNodeImpl*)parent)->Clear();
		return WINI_OK;
	case WINI_QUERY:
		((QueryNodeImpl*)parent)->Clear(false);
		return WINI_OK;
	default:
		return WINI_ERROR;
	}

	return NULL;
}



void SessionImpl::ClearCatalogScratch()
{
	/*
    clearSqlResult (&m_result);

    for(int i = 0; i < MAX_DCS_NUM; i++)
	{
        sprintf(m_qval[i],"");
        m_selval[i] = 0;
    }
	*/
}

IStringNode* SessionImpl::GetAccessConstraints()
{

	if(m_constraint_node)
		return m_constraint_node;

	//ClearCatalogScratch();

	//m_selval[ACCESS_CONSTRAINT] = 1;

    //StatusCode status = srbGetDataDirInfo(m_conn, 0, m_qval, m_selval, &m_result, MAX_ROWS);

	//filterDeleted(&m_result);

	//char* constraint = getFromResultStruct(&m_result,dcs_tname[ACCESS_CONSTRAINT], dcs_aname[ACCESS_CONSTRAINT]);

	//m_constraint_node = new StringNodeImpl(NULL);
	//((StringNodeImpl*)m_constraint_node)->AddString(constraint);

	//for(int i = 1; i < m_result.row_count; i++)
	//{
	//	constraint += MAX_DATA_SIZE;
	//	((StringNodeImpl*)m_constraint_node)->AddString(constraint);
	//}

	//return m_constraint_node;

	return NULL;
}

ISetNode* SessionImpl::GetResource(IZoneNode* zone)
{
	if(zone)
		if(zone->CountChildren() == 0)
			OpenTree(zone, 1, 1);
	else
		return (ISetNode*)(zone->GetChild("resources"));
	
	return (ISetNode*)(m_HomeZone->GetChild("resources"));
}

ICollectionNode* SessionImpl::GetRoot(IZoneNode* zone)
{
	return m_root;
}

ICollectionNode* SessionImpl::GetLocalRoot()
{
	return m_localRoot;
}

ICollectionNode* SessionImpl::GetHome(IZoneNode* zone)
{
	return m_home;
}

IZoneNode* SessionImpl::GetHomeZone()
{
	return m_HomeZone;
}


IZoneNode* SessionImpl::GetCurrentZone(INode* target)
{
	char buf[1024];
	int i;

	INode* blah;

	switch(target->GetType())
	{
	case WINI_COLLECTION:
		sprintf(buf, "%s",target->GetPath());
		for(i = 1; i < 1024; i++)
		{
			if(buf[i] == '/')
			{
				buf[i] = '\0';
				break;
			}
		}
		return (IZoneNode*)m_Catalog->GetChild(buf);
		break;
	case WINI_DATASET:
		return GetCurrentZone(target->GetParent());
	case WINI_RESOURCE:
		blah = target->GetParent();
		blah = target->GetParent();
		return (IZoneNode*)blah;
	default:
		return NULL;
	}
		return 0;
}
}//end namespace




