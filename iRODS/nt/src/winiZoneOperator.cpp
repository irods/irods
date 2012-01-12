#include "winiZoneOperator.h"
#include "winiZone.h"
#include "winiUser.h"
#include "winiSession.h"
#include "winiCollection.h"
//#include "winiCollectionOperator.h"
//#include "winiDatasetOperator.h"
//#include "winiQueryOperator.h"
//#include "winiQueryOperator2.h"
//#include "winiRewiniurceOperator.h"
//#include "winiContainerOperator.h"
//#include "winiDomainOperator.h"
#include "winiZoneOperator.h"
#include "winiString.h"
#include "winiSet.h"
#include "rcMisc.h"
#include "genQuery.h"

namespace WINI
{

ZoneOperatorImpl::ZoneOperatorImpl(ISession* session)
{
	m_session = session;
}

ZoneOperatorImpl::~ZoneOperatorImpl()
{
}

StatusCode ZoneOperatorImpl::GetChildren(ZoneNodeImpl* target, int clean, unsigned int mask)
{
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	if(clean)
	{
		target->Clear();
	}

	StatusCode status;
	
	unsigned int success = WINI_ALL ^ WINI_COLLECTION ^ WINI_RESOURCE;

	if((WINI_COLLECTION & mask) && !(WINI_COLLECTION & target->GetOpen()))
	{
		status = GetCollections(target);
		if(status.isOk())
		{
			success |= WINI_COLLECTION;
		}
		
		if(!(success & WINI_COLLECTION))
		{
			target->SetOpen(success);
			return status;
		}

	}

	if((WINI_RESOURCE & mask) && !(WINI_RESOURCE & target->GetOpen()))
	{
		status = GetResources(target);
		if(status.isOk())
		{
			success |= WINI_RESOURCE;
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

void ZoneOperatorImpl::InitQuery()
{
}

StatusCode ZoneOperatorImpl::GetCollections(ZoneNodeImpl* target)
{
	InitQuery();

	SetNodeImpl* collection_set = new SetNodeImpl(target, "collections", WINI_COLLECTION);
	target->AddChild(collection_set);

    genQueryOut_t *genQueryOut = NULL;
    genQueryInp_t genQueryInp;
	rodsArguments_t myRodsArgs;

	char szPathBuf[1024];

	memset ((void*)&genQueryInp, 0, sizeof (genQueryInp_t));

	void* fooX = this->m_session->GetConn();
	rcComm_t* foo = (rcComm_t*)fooX;
	queryHandle_t queryHandle;
    int s = queryCollInColl (&queryHandle, (char*)target->GetPath(), 0, &genQueryInp, &genQueryOut);

    int i, status, j;

    sqlResult_t *colName, *dataId;
    char *tmpDataName, *tmpDataId;

    if ((colName = getSqlResultByInx (genQueryOut, COL_COLL_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_COLL_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    for (i = 0;i < genQueryOut->rowCnt; i++)
	{
        tmpDataName = &colName->value[colName->len * i];

		for(j = strlen(tmpDataName)-1; j != 0; j--)
			if('/' == tmpDataName[j])
				break;

		CollectionNodeImpl* blah = new CollectionNodeImpl(target,&tmpDataName[++j]);
		collection_set->AddChild(blah);
    }

    return WINI_OK;
}

#if 0
StatusCode ZoneOperatorImpl::GetDomains(ZoneNodeImpl* target)
{
	ClearMCATScratch();

	//sprintf(m_qval[ZONE_NAME]," = '%s'", target->GetName());
	m_selval[DOMAIN_DESC] = 1;

	StatusCode status = srbGetDataDirInfoWithZone((srbConn*)m_session->GetConn(), 0, (char*)target->GetName(), m_qval, m_selval, &m_result, MAX_ROWS);
						
	if(!status.isOk())
		return status;

	filterDeleted (&m_result);

	SetNodeImpl* domain_set = new SetNodeImpl(target, "domains", SOB_DOMAIN);
	target->AddChild(domain_set);

	DomainNodeImpl* child;

	char* szDomain = getFromResultStruct(&m_result, dcs_tname[DOMAIN_DESC], dcs_aname[DOMAIN_DESC]);

	if(szDomain == NULL)
		if(m_result.row_count != 0)
			return SRB_ERROR;
		else
			return SRB_OK;

	child = new DomainNodeImpl(target, szDomain);
	domain_set->AddChild(child);

	for (i=1; i < m_result.row_count; i++) 
	{
		szDomain += MAX_DATA_SIZE;
		child = new DomainNodeImpl(target, szDomain);
		domain_set->AddChild(child);
	}

	return SRB_OK;
}
#endif
StatusCode ZoneOperatorImpl::GetResources(ZoneNodeImpl* target)
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int i1a[20];
   int i1b[20]={0,0,0,0,0,0,0,0,0,0,0,0};
   int i2a[20];
   char *condVal[10];
   char v1[200];
   int i, status;

	//resources are a little different in that there is a set of resources, and no root
	//resource, unlike a collection. In the beginning we wanted to have each resource
	//be its own child of the zone, but in many instances that was not ideal - as we
	//had to iterate through the entire zone to find all the resources to display.
	SetNodeImpl* resource_set = new SetNodeImpl(target, "resources", WINI_RESOURCE);
	target->AddChild(resource_set);

	//TODO: modify query to limit search by zone.

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));

   i=0;

	void* fooX = this->m_session->GetConn();
	rcComm_t* foo = (rcComm_t*)fooX;

   i1a[i++]=COL_R_RESC_NAME;
   i1a[i++]=COL_R_RESC_ID;
   i1a[i++]=COL_R_ZONE_NAME;
   i1a[i++]=COL_R_TYPE_NAME;
   i1a[i++]=COL_R_CLASS_NAME;
   i1a[i++]=COL_R_LOC;
   i1a[i++]=COL_R_VAULT_PATH;
   i1a[i++]=COL_R_FREE_SPACE;
   i1a[i++]=COL_R_RESC_INFO;
   i1a[i++]=COL_R_RESC_COMMENT;
   i1a[i++]=COL_R_CREATE_TIME;
   i1a[i++]=COL_R_MODIFY_TIME;

   
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = i;

   genQueryInp.sqlCondInp.inx = i2a;
   genQueryInp.sqlCondInp.value = condVal;

   genQueryInp.sqlCondInp.len=0;

   genQueryInp.maxRows=50;
   genQueryInp.continueInx=0;
   status = rcGenQuery(foo, &genQueryInp, &genQueryOut);

   if (status == CAT_NO_ROWS_FOUND)
   {
		target->SetOpen(WINI_ALL);
		return WINI_OK;
   }else if(status != 0)
   {
	   return status;
   }

    int j;

    sqlResult_t *colName, *dataId;
    char *tmpDataName, *tmpDataId;

    if ((colName = getSqlResultByInx (genQueryOut, COL_R_RESC_NAME)) == NULL)
        return (UNMATCHED_KEY_OR_INDEX);

	IResourceNode* child;
	//TODO: change resource type to be real type.

	char* type = "foo";

    for (i = 0;i < genQueryOut->rowCnt; i++)
	{
        tmpDataName = &colName->value[colName->len * i];

		ResourceNodeImpl* blah = new ResourceNodeImpl(target,tmpDataName, type);
		resource_set->AddChild(blah);
    }

	//target->SetOpen(WINI_ALL);
	return WINI_OK;
}




};








