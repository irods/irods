#include "winiQueryOperator.h"
#include "winiQuery.h"
#include "winiCollectionOperator.h"
#include "winiDatasetOperator.h"

namespace WINI
{

QueryOperatorImpl::QueryOperatorImpl(ISession* session)
{
	m_session = session;
//	m_conn = (srbConn*)session->GetConn();
	//m_result.result_count = 0;
	//m_result.row_count = 0;
	m_nodes = NULL;
}

//query node is not responsible for anything it makes that is attached to a query. it is
//responsible for anything it makes that is not attached. Query nodes, including any nested
//query nodes are the property of the session object.
QueryOperatorImpl::~QueryOperatorImpl()
{
//	if(m_result.result_count != 0)
	//	clearSqlResult(&m_result);

	//delete m_nodes and everything in it
}

StatusCode QueryOperatorImpl::Create(IQueryNode** query)
{
	if(NULL == *query)
		return WINI_ERROR_INVALID_PARAMETER;

	*query = new QueryNodeImpl();

	//add to m_nodes

	return WINI_OK;
}

StatusCode QueryOperatorImpl::Delete(IQueryNode* query)
{
	if(NULL == query)
		return WINI_ERROR_INVALID_PARAMETER;

	INode* parent = query->GetParent();

	if(parent)
	{
		//parent can be binding of a root query or parent of a nested query
		//in case of root query, the collection, resource, etc, does not know that
		//a query references it as its parent.
		//remove query from parent query
		if(WINI_QUERY == parent->GetType())
			((QueryNodeImpl*)parent)->RemoveChild(query);
	};

	//can also be referenced in session if it is a root query in the list
	//should call on session to remove from its list as well.

	//cam also be in m_nodes check there as well

	delete query;	//this should delete all child query nodes but not other nodes

	return WINI_OK;
}

StatusCode QueryOperatorImpl::Nest(IQueryNode* parent, IQueryNode* child)
{
	if((NULL == parent)||(NULL == child))
		return WINI_ERROR_INVALID_PARAMETER;

	((QueryNodeImpl*)parent)->AddChild(child);

	((QueryNodeImpl*)child)->SetParent(parent);

	((QueryNodeImpl*)child)->Close();

	return WINI_OK;
}

StatusCode QueryOperatorImpl::Attach(IQueryNode* query, IMetadataNode* expression)
{
	if((NULL == query)||(NULL == expression))
		return WINI_ERROR_INVALID_PARAMETER;

	((QueryNodeImpl*)query)->AddChildFront(expression);

	((MetadataNodeImpl*)expression)->SetParent(query);

	return WINI_OK;
}

StatusCode QueryOperatorImpl::Bind(IQueryNode* query, INode* node)
{
	if(NULL == node)
		return WINI_ERROR_INVALID_PARAMETER;

	switch(node->GetType())
	{
	case WINI_COLLECTION:
	case WINI_QUERY:
		break;
	default:
		return WINI_ERROR_INVALID_PARAMETER;
	}

	((QueryNodeImpl*)query)->SetParent(node);

	return WINI_OK;
}

StatusCode QueryOperatorImpl::Add(IMetadataNode** expression)
{
	if(NULL == *expression)
		return WINI_ERROR_INVALID_PARAMETER;

	*expression = new MetadataNodeImpl();

	//add to m_node

	return WINI_OK;
}

StatusCode QueryOperatorImpl::Remove(IMetadataNode* expression)
{
	if(NULL == expression)
		return WINI_ERROR_INVALID_PARAMETER;

	INode* parent = expression->GetParent();

	StatusCode status = WINI_OK;

	if(parent)
	{
		switch(parent->GetType())
		{
		case WINI_QUERY:			//is root expression. delete from query, query is now invalid
			status = ((QueryNodeImpl*)parent)->RemoveChild(expression);
			break;
		case WINI_METADATA:		//delete from metadata, metadata must be fixed
			status = ((MetadataNodeImpl*)parent)->DeleteChild2(expression);
			break;
		default:				//do not remove nodes of collections, datasets, etc.
			return WINI_ERROR;
		}
		//remove query from parent
	};

	if(status.isOk())
		delete expression;

	return status;
}

StatusCode QueryOperatorImpl::SetValue(IMetadataNode* expression, const char* value)
{
	if((NULL == expression)||(NULL == value))
		return WINI_ERROR_INVALID_PARAMETER;

	((MetadataNodeImpl*)expression)->SetValue(value);

	return WINI_OK;
}

StatusCode QueryOperatorImpl::SetAttribute(IMetadataNode* expression, const char* attribute)
{
	if((NULL == expression)||(NULL == attribute))
		return WINI_ERROR_INVALID_PARAMETER;

	((MetadataNodeImpl*)expression)->SetAttribute(attribute);

	return WINI_OK;
}

StatusCode QueryOperatorImpl::SetOperation(IMetadataNode* expression, int operation)
{
	if((NULL == expression)||(NULL == operation))
		return WINI_ERROR_INVALID_PARAMETER;

	((MetadataNodeImpl*)expression)->SetOperation(operation);

	return WINI_OK;

}

StatusCode QueryOperatorImpl::And(IMetadataNode* left, IMetadataNode* right, IMetadataNode** result)
{
	if((NULL == left)||(NULL == right)||(NULL == *result))
		return WINI_ERROR_INVALID_PARAMETER;

	INode* left_parent = left->GetParent();
	INode* right_parent = right->GetParent();

	int lpt = -1;
	int rpt = -1;

	if(left_parent)
		lpt = left_parent->GetType();

	if(right_parent)
		rpt = right_parent->GetType();

	//both nodes can't already be attached to queries; which one would take precidence?
	if((lpt == WINI_QUERY)&&(rpt == WINI_QUERY))
		return WINI_ERROR_INVALID_PARAMETER;

	//this should have a status code but we'll leave it for now
	*result = ((MetadataNodeImpl*)left)->And(right);

	if(WINI_QUERY == lpt)
	{
		//replace query's 1st node with this one
		((QueryNodeImpl*)left_parent)->RemoveChild(left);
		((QueryNodeImpl*)left_parent)->AddChildFront(*result);
	}

	if(WINI_QUERY == rpt)
	{
		((QueryNodeImpl*)right_parent)->RemoveChild(right);
		((QueryNodeImpl*)right_parent)->AddChildFront(*result);
	}

	return WINI_OK;
}

StatusCode QueryOperatorImpl::Or(IMetadataNode* left, IMetadataNode* right, IMetadataNode** result)
{
#if 0
	if((NULL == left)||(NULL == right)||(NULL == *result))
		return WINI_ERROR_INVALID_PARAMETER;

	INode* left_parent = left->GetParent();
	INode* right_parent = right->GetParent();

	int lpt = left_parent->GetType();
	int rpt = right_parent->GetType();

	//both nodes can't already be attached to queries; which one would take precidence?
	if((lpt == WINI_QUERY)&&(rpt == WINI_QUERY))
		return WINI_ERROR_INVALID_PARAMETER;

	//this should have a status code but we'll leave it for now
	*result = ((MetadataNodeImpl*)left)->Or(right);

	if(WINI_QUERY == lpt)
	{
		//replace query's 1st node with this one
		((QueryNodeImpl*)left_parent)->RemoveChild(left);
		((QueryNodeImpl*)left_parent)->AddChildFront(*result);
	}

	if(WINI_QUERY == rpt)
	{
		((QueryNodeImpl*)right_parent)->RemoveChild(right);
		((QueryNodeImpl*)right_parent)->AddChildFront(*result);
	}

	return WINI_OK;
#endif
	return WINI_ERROR;
}

//Gets all children matching query. First collections, and then datasets
StatusCode QueryOperatorImpl::GetChildren(QueryNodeImpl* target, unsigned int mask)
{
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	StatusCode status;

	target->Close();	//this clear should nuke all children except the metadata node

	target->SetOpen(WINI_ALL ^ WINI_QUERY);

	if(WINI_QUERY & mask)
	{
		status = LoadCollectionQuery(target);

		if(status.isOk())
		{
			status = Fetch(target);

			if(status.isOk())
			{
				status = LoadDatasetQuery(target);

				if(status.isOk())
				{
					status = FetchDatasets(target);
					
					if(status.isOk())
						target->SetOpen(WINI_ALL);
				}
			}
		}
	}

	return status;
}

void QueryOperatorImpl::ClearMCATScratch()
{
#if 0
    clearSqlResult(&m_result);

    for(int i = 0; i < MAX_DCS_NUM; i++)
	{
        sprintf(m_qval[i],"");
        m_selval[i] = 0;
    }
#endif
}

StatusCode QueryOperatorImpl::LoadDatasetQuery(QueryNodeImpl* target)
{
#if 0
	ClearMCATScratch();
	m_selval[DATA_GRP_NAME] = 1;
	m_selval[DATA_NAME] = 1;

	INode* ptr = target;
	IMetadataNode* ptr_meta;

	int count = 0;

	StatusCode status;

	while(WINI_QUERY == ptr->GetType())
	{
		ptr_meta = (IMetadataNode*)ptr->GetChild(0);

		status = LoadDatasetQuery2(ptr_meta, count);

		if(!status.isOk())
			break;

		ptr = ptr->GetParent();

		if(NULL == ptr)
			break;
	}

	return status;
#endif
	return WINI_ERROR;
}

StatusCode QueryOperatorImpl::LoadCollectionQuery(QueryNodeImpl* target)
{
#if 0
	ClearMCATScratch();
	m_selval[DATA_GRP_NAME] = 1;

	INode* ptr = target;
	IMetadataNode* ptr_meta;

	int count = 0;

	StatusCode status;

	while(WINI_QUERY == ptr->GetType())
	{
		ptr_meta = (IMetadataNode*)ptr->GetChild(0);

		status = LoadCollectionQuery2(ptr_meta, count);

		if(!status.isOk())
			break;

		ptr = ptr->GetParent();

		if(NULL == ptr)
			break;
	}

	return status;
#endif
	return WINI_ERROR;
}


StatusCode QueryOperatorImpl::LoadDatasetQuery2(IMetadataNode* ptr, int& count)
{
	return WINI_ERROR;
#if 0
	if(NULL == ptr)
		return WINI_ERROR_INVALID_PARAMETER;

	MetadataNodeImpl* child;

	StatusCode status;

	switch(ptr->GetOperation())
	{
	case WINI_MD_AND:
		child = (MetadataNodeImpl*)ptr->GetChild(0);	//left hand child
		status = LoadDatasetQuery2(child, count);
		if(!status.isOk())
			return status;
		child = (MetadataNodeImpl*)ptr->GetChild(1);	//right hand child
		status = LoadDatasetQuery2(child, count);
		if(!status.isOk())
			return status;
		break;
	case WINI_MD_EQUAL:
	case WINI_MD_NOT_EQUAL:
	case WINI_MD_GREATER_THAN:
	case WINI_MD_LESS_THAN:
	case WINI_MD_GREATER_OR_EQUAL:
	case WINI_MD_LESS_OR_EQUAL:
	case WINI_MD_BETWEEN:
	case WINI_MD_NOT_BETWEEN:
	case WINI_MD_LIKE:
	case WINI_MD_NOT_LIKE:
	case WINI_MD_SOUNDS_LIKE:
	case WINI_MD_SOUNDS_NOT_LIKE:
		switch(++count)
		{
		case 1:
			sprintf(m_qval[UDSMD0]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD1]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			break;
		case 2:
			sprintf(m_qval[UDSMD0_1]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD1_1]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			break;
		case 3:
			sprintf(m_qval[UDSMD0_2]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD1_2]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			break;
		case 4:
			sprintf(m_qval[UDSMD0_3]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD1_3]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			break;
		case 5:
			sprintf(m_qval[UDSMD0_4]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD1_4]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			//could also put a ptr->GetValue(); here to reset a static int
			break;
		default:
			break;
		}
		return WINI_OK;
	default:
		return WINI_ERROR_INVALID_PARAMETER;
	}

	return status;
#endif
}

StatusCode QueryOperatorImpl::LoadCollectionQuery2(IMetadataNode* ptr, int& count)
{
	return WINI_ERROR;
#if 0
	if(NULL == ptr)
		return WINI_ERROR_INVALID_PARAMETER;

	MetadataNodeImpl* child;

	StatusCode status;

	switch(ptr->GetOperation())
	{
	case WINI_MD_AND:
		child = (MetadataNodeImpl*)ptr->GetChild(0);	//left hand child
		status = LoadCollectionQuery2(child, count);
		if(!status.isOk())
			return status;
		child = (MetadataNodeImpl*)ptr->GetChild(1);	//right hand child
		status = LoadCollectionQuery2(child, count);
		if(!status.isOk())
			return status;
		break;
	case WINI_MD_EQUAL:
	case WINI_MD_NOT_EQUAL:
	case WINI_MD_GREATER_THAN:
	case WINI_MD_LESS_THAN:
	case WINI_MD_GREATER_OR_EQUAL:
	case WINI_MD_LESS_OR_EQUAL:
	case WINI_MD_BETWEEN:
	case WINI_MD_NOT_BETWEEN:
	case WINI_MD_LIKE:
	case WINI_MD_NOT_LIKE:
	case WINI_MD_SOUNDS_LIKE:
	case WINI_MD_SOUNDS_NOT_LIKE:
		switch(++count)
		{
		case 1:
			sprintf(m_qval[UDSMD_COLL0]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD_COLL1]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			break;
		case 2:
			sprintf(m_qval[UDSMD_COLL0_1]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD_COLL1_1]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			break;
		case 3:
			sprintf(m_qval[UDSMD_COLL0_2]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD_COLL1_2]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			break;
		case 4:
			sprintf(m_qval[UDSMD_COLL0_3]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD_COLL1_3]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			break;
		case 5:
			sprintf(m_qval[UDSMD_COLL0_4]," = '%s'", ptr->GetAttribute());
			sprintf(m_qval[UDSMD_COLL1_4]," %s '%s'", ptr->GetOperationString(), ptr->GetValue());
			//could also put a ptr->GetValue(); here to reset a static int
			break;
		default:
			break;
		}
		return WINI_OK;
	default:
		return WINI_ERROR_INVALID_PARAMETER;
	}

	return status;
#endif
}

StatusCode QueryOperatorImpl::FetchCollections(QueryNodeImpl* target)
{
	return WINI_ERROR;
#if 0
	sprintf(m_qval[DATA_GRP_NAME]," = '%s'", m_binding->GetPath());

	StatusCode status = GetFirstChildCollection(target);

	if(status.isOk())
	{
		clearSqlResult(&m_result);
		sprintf(m_qval[DATA_GRP_NAME]," like '%s/%%'", m_binding->GetPath());
		status = GetChildCollections(target);
	}

	return status;
#endif
}

StatusCode QueryOperatorImpl::Fetch(QueryNodeImpl* target)
{
	return WINI_ERROR;
#if 0
	//the equal thing will only ever return either 1 or 0 so it should have special instance
	//also because when you subtract the collection name from itself it's null

	m_binding = NULL; //reset in case the same operator is used to perform different queries

	if(NULL == target)
		return WINI_ERROR;

	INode* blah = target->GetParent();

	while(NULL != blah)
	{
		if(WINI_QUERY == blah->GetType())
		{
			blah = blah->GetParent();
		}else
		{
			m_binding = blah;
			blah = NULL;
		}
	}

	if(NULL == m_binding)
		return WINI_ERROR;

	if(WINI_COLLECTION != m_binding->GetType())
		return WINI_ERROR;

	StatusCode status = FetchCollections(target);

	//if(status.isOk())
	//	FetchDatasets(QueryNodeImpl* target);

	return status;
#endif
}


StatusCode QueryOperatorImpl::FetchDatasets(QueryNodeImpl* target)
{
	return WINI_ERROR;
#if 0
	sprintf(m_qval[DATA_GRP_NAME]," = '%s'", m_binding->GetPath());

	StatusCode status = GetChildDatasets(target);

	if(status.isOk())
	{
		clearSqlResult(&m_result);
		sprintf(m_qval[DATA_GRP_NAME]," like '%s/%%'", m_binding->GetPath());
		status = GetChildDatasets(target);
	}

	return status;
#endif
}


StatusCode QueryOperatorImpl::GetChildDatasets(QueryNodeImpl* target)
{
	return WINI_ERROR;
#if 0
	StatusCode status = srbGetDataDirInfo(m_conn, 0, m_qval, m_selval, &m_result, MAX_ROWS);

	if(!status.isOk())
		return status;

	filterDeleted(&m_result);

	char* szCollection = getFromResultStruct(&m_result,dcs_tname[DATA_GRP_NAME], dcs_aname[DATA_GRP_NAME]);
	char* szDataset = getFromResultStruct(&m_result,dcs_tname[DATA_NAME], dcs_aname[DATA_NAME]);

	char *szCPtr, *szDPtr;

	const char* path = m_binding->GetPath();
	int path_len = strlen(path);


	INode* parent = m_binding;
	INode *child, *dataset;

	for(int i = 0; i < m_result.row_count; i++)
	{
		szCPtr = szCollection;
		szDPtr = szDataset;

		szCPtr += i * MAX_DATA_SIZE;
		szDPtr += i * MAX_DATA_SIZE;

		szCPtr += path_len;
		//dataset?

		if(NULL == *szCPtr)
		{
			status = parent->GetChild(szDPtr, &dataset);
			if(!status.isOk())
				return WINI_ERROR;
			target->AddChild(dataset);
			continue;
		}

start:	status = ((CollectionNodeImpl*)parent)->GetChild(szCPtr, &child);

		CollectionOperatorImpl* collection_op = (CollectionOperatorImpl*)m_session->GetOperator(WINI_COLLECTION);

		switch(status.GetCode())
		{
		case WINI_OK:
			//dataset is in a child collection of the binding
			//child collection has been found
			//now locate the dataset
startd:		status = child->GetChild(szDPtr, &dataset);
			switch(status.GetCode())
			{
			case WINI_OK:
				target->AddChild(dataset);
				break;
			case WINI_ERROR_NOT_FILLED:
				collection_op->GetChildren((CollectionNodeImpl*)child);
				goto startd;
				break;
			default:
				return WINI_ERROR;
				break;
			}
			break;
		case WINI_ERROR_NOT_FILLED:
			//child is not opened yet
			//open the child and begin again
			collection_op->GetChildren((CollectionNodeImpl*)parent);
			goto start;
			break;
		default:
			return WINI_ERROR;
			break;
		}

	}

	//while(m_result.continuation_index >= 0)
	//{
		//add code for this later	
	//}

	return WINI_OK;
#endif
}



StatusCode QueryOperatorImpl::GetFirstChildCollection(QueryNodeImpl* target)
{
#if 0
	StatusCode status = srbGetDataDirInfo(m_conn, 0, m_qval, m_selval, &m_result, MAX_ROWS);

	if(0 != status)
	{
		if(-3005 == status)
			status = 0;

		return status;
	}

	switch(m_result.row_count)
	{
	case 0:
		break;
	case 1:
		target->AddChild(m_binding);
		break;
	default:
		return WINI_ERROR;
	}
	
	return WINI_OK;
#endif
	return WINI_ERROR;
}

StatusCode QueryOperatorImpl::GetChildCollections(QueryNodeImpl* target)
{
	return WINI_ERROR;
#if 0
	StatusCode status = srbGetDataDirInfo(m_conn, 0, m_qval, m_selval, &m_result, MAX_ROWS);

	if(0 != status)
	{
		if(-3005 == status)
			status = 0;

		return status;
	}

	filterDeleted(&m_result);

	INode* baby;

	CollectionOperatorImpl* cOp;

	char* original = getFromResultStruct(&m_result,dcs_tname[DATA_GRP_NAME], dcs_aname[DATA_GRP_NAME]);
	char* pathstring;

	for(int i = 0; i < m_result.row_count; i++)
	{
		pathstring = original + (i*MAX_DATA_SIZE);
		pathstring = pathstring + strlen(m_binding->GetPath());
		
		//highly inefficient but for right now it works...
blah:
		if(pathstring)
		{
			status = ((CollectionNodeImpl*)m_binding)->GetChild(pathstring, &baby);
				
			switch(status.GetCode())
			{
			case WINI_OK:
				target->AddChild(baby);
				break;
			case WINI_ERROR_NOT_FILLED:
				cOp = new CollectionOperatorImpl(m_session);
				cOp->GetChildren((CollectionNodeImpl*)baby);
				goto blah;
					break;
			default:
				return WINI_GENERAL_ERROR;
			}
		}else
		{
			target->AddChild(m_binding);
		}
	}

	//while(m_result.continuation_index >= 0)
	//{
		//add code for this later	
	//}

	return WINI_OK;
#endif
}

} //end namespace
















