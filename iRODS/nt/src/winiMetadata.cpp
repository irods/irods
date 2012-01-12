#include "winiMetadata.h"
#include "winiQuery.h"

extern "C"
{
}

namespace WINI
{
MetadataNodeImpl::MetadataNodeImpl()
{
	m_name = NULL;
	m_attribute = NULL;
	m_operation = WINI_MD_EQUAL;
	m_value = NULL;
	m_path = strdup("//");
	m_parent = NULL;
	m_children = NULL;
	m_ID = NULL;
	m_isOpen = WINI_NONE;
}

MetadataNodeImpl::~MetadataNodeImpl()
{
}

//general new attribute constructor. creates a root node.
//it is somewhat bad to default to equal - since this constructor can also be used
//for sorts, etc. but is better than creating an 'undefined' type.
MetadataNodeImpl::MetadataNodeImpl(INode* parent, const char* attribute)
{
	m_name = NULL;
	m_attribute = strdup(attribute);
	m_operation = WINI_MD_EQUAL;
	m_value = NULL;
	m_path = strdup("//");
	m_parent = parent;
	m_children = NULL;
	m_isOpen = WINI_ALL;
}

//'join' constructor
MetadataNodeImpl::MetadataNodeImpl(IMetadataNode* left, int operation, IMetadataNode* right)
{
	m_name = NULL;

	m_attribute = NULL;
	m_value = NULL;
	m_path = NULL;
	m_parent = left->GetParent();

	m_children = new std::vector<INode*>;

	((MetadataNodeImpl*)left)->SetParent(this);
	((MetadataNodeImpl*)right)->SetParent(this);

	m_children->push_back(left);
	m_children->push_back(right);

	m_operation = operation;

	m_isOpen = WINI_ALL;

}

MetadataNodeImpl::MetadataNodeImpl(INode* parent, const char* attribute, int operation, const char* value)
{
	m_name = NULL;
	m_attribute = NULL;
	m_value = NULL;
	m_children = 0;
	m_path = NULL;
	m_parent = parent;

	m_attribute = strdup(attribute);
	m_value = strdup(value);
	m_operation = WINI_MD_EQUAL;

	m_isOpen = WINI_NONE;
}

void MetadataNodeImpl::SetID(int i)
{
	m_ID = i;
}

#if 0
//a implementation specific version of (szAttribute = szValue) for collections;
MetadataNodeImpl::MetadataNodeImpl(INode* parent, mdasC_sql_result_struct* result, int index)
{
	m_name = NULL;
	m_attribute = NULL;
	m_value = NULL;
	m_children = 0;
	m_path = NULL;
	m_parent = parent;

	char *szID = getFromResultStruct(result, dcs_tname[METADATA_NUM_COLL], dcs_aname[METADATA_NUM_COLL]);

	if(szID)
	{
		szID += index * MAX_DATA_SIZE;
		m_ID = atoi(szID);
	}else
	{
		m_ID = -1;
	}

	m_attribute = getFromResultStruct(result, dcs_tname[UDSMD_COLL0], dcs_aname[UDSMD_COLL0]);

	if(m_attribute)
	{
		m_attribute += index * MAX_DATA_SIZE;
		m_attribute = strdup(m_attribute);
	}else
	{
		m_attribute = NULL;
	}

	m_value = getFromResultStruct(result, dcs_tname[UDSMD_COLL1], dcs_aname[UDSMD_COLL1]);
	if(m_value)
	{
		m_value += index * MAX_DATA_SIZE;
		m_value = strdup(m_value);
	}else
	{
		m_value = NULL;
	}

	m_operation = WINI_MD_EQUAL;
	//int len = strlen(m_attribute);
	//len += strlen(m_value) + 4;

	//m_name = (char*)calloc(sizeof(char), len);

	//sprintf(m_name, "%s = %s", m_attribute, m_value);

	m_isOpen = WINI_NONE;
}
#endif


const char* MetadataNodeImpl::GetName()
{
	if(m_name)
	{
		return m_name;
	}

	//create name from values if it wasn't defined already.
	int len = 0;

	//filled data will differ depending on whether or not the node is a joining node
	//such as AND or OR or a statement node (EQUAL, LESS THAN, etc).
	char *left, *right;


	switch(m_operation)
	{
	case WINI_MD_AND:
		left = (char*)(GetChild(0))->GetName();
		right = (char*)(GetChild(1))->GetName();

		len += strlen(left);
		len += strlen(right);

		//4 for parenthesis surrounding each statement
		//2 for spaces
		//3 for and
		//1 for null
		m_name = (char*)calloc(len + 10, sizeof(char));
		sprintf(m_name, "(%s) AND (%s)", left, right);
		break;
	//case WINI_MD_SORT:
		//left = (char*)(GetChild(0))->GetName();
		//len += strlen(left);

		//m_name = (char*)calloc(len + 10, sizeof(char));
		//sprintf(m_name, "%s:%s", m_attribute, left);
	//	sprintf(m_name, "%s
	//	break;
	default:
		//equal and sort
		len += strlen(m_attribute);
		len += strlen(op_map[m_operation]);

		if(m_value)
		{
			len += strlen(m_value);
			m_name = (char*)calloc(len + 3, sizeof(char));	//len + 3 for terminating NULL, plus 2 spaces
			sprintf(m_name, "%s %s %s", m_attribute, op_map[m_operation], m_value);
		}else
		{
			len += strlen("(null)");
			m_name = (char*)calloc(len + 3, sizeof(char)); //ditto
			sprintf(m_name, "%s %s (null)", m_attribute, op_map[m_operation], m_value);
		}
		break;
	}

	return m_name;
}

IMetadataNode* MetadataNodeImpl::And(IMetadataNode* node)
{
	if(NULL == node)
	{
		return NULL;
	}

	MetadataNodeImpl* blah = new MetadataNodeImpl(this, WINI_MD_AND, node);

	return blah;
}

const char* MetadataNodeImpl::GetPath()
{
	if(m_path)
	{
		return m_path;
	}

	int len = strlen(m_parent->GetPath());
	len += strlen(this->GetName());

	m_path = (char*)calloc(len + 2, sizeof(char));	//1 for slash, 1 for terminating null

	sprintf(m_path, "%s/%s", m_parent->GetPath(), this->GetName());

	return m_path;
}

//inline
int MetadataNodeImpl::CountChildren()
{
	if(m_children)
	{
		//does this return int?
		return m_children->size();
	}

	return 0;
}

int MetadataNodeImpl::CountHeightFromRoot()
{
	if(NULL == m_parent)
	{
		return 0;
	}

	return m_parent->CountHeightFromRoot() + 1;
}

//inline
INode* MetadataNodeImpl::GetChild(int pos)
{
	if(m_children && pos < m_children->size() && pos >= 0)
			return m_children->at(pos);
	
	return NULL;
}

StatusCode MetadataNodeImpl::GetChild(const char* name, INode** result)
{
	if(NULL == name)
		return NULL;

	if(NULL == result)
		return NULL;

	INode* child;
	char *szname;

	StatusCode status;

	int j;

	if('/' == name[0])
	{
		szname = strdup(name);

		int len = strlen(name);

		for( j = 1; j < len; j++)
		{
			if('/' == szname[j])
			{
				szname[j] = 0;
				break;
			}
		}

		status = GetChild(&szname[1], &child);

		if(!status.isOk())
		{
			free(szname);
			*result = child;
			return status;
		}

		if(j < len)
		{
			status = child->GetChild(&szname[j+1], &child);
		}

		free(szname);
		*result = child;
		return status;

	}else
	{
		if(m_children)
		{
			for(int i = 0; i < m_children->size(); i++)
			{
				child = m_children->at(i);
				if(0 == strcmp(name, child->GetName()))
				{
					*result = child;
					return WINI_OK;
				}
			}
		}else
		{
			*result = this;
			return WINI_ERROR_NOT_FILLED;
		}
	}

	//should never reach here
	*result = NULL;
	return WINI_ERROR;
}
void MetadataNodeImpl::AddChild(INode* child)
{
	if(child)
	{
		if(NULL == m_children)
		{
			m_children = new std::vector<INode*>;
		}	
		m_children->push_back(child);
	}
}

//intelligent remove
//handles case where this node is an and/or node
//this code sucks
StatusCode MetadataNodeImpl::DeleteChild2(INode* child)
{
	if((NULL == m_children)||(NULL == child))
		return WINI_ERROR_INVALID_PARAMETER;

	if((WINI_MD_AND != m_operation)||(WINI_MD_OR != m_operation))
		return WINI_ERROR;

	StatusCode status = WINI_OK;

	INode* other;

	MetadataNodeImpl* mdni;
	QueryNodeImpl* qni;

	switch(m_parent->GetType())
	{
	case WINI_METADATA:
		mdni = (MetadataNodeImpl*)m_parent;
		qni = NULL;
		break;
	case WINI_QUERY:
		qni = (QueryNodeImpl*)m_parent;
		mdni = NULL;
		break;
	default:
		return WINI_ERROR;
	}

	if(child == GetChild(0))
	{
		//left child
		DeleteChild(child);
		other = GetChild(1);
		if(mdni)
			mdni->AddChild(other);
		else
			qni->AddChild(other);
		delete this;
	}else if(child == GetChild(1))
	{
		//right child
		DeleteChild(child);
		other = GetChild(0);
		if(mdni)
			mdni->AddChild(other);
		else
			qni->AddChild(other);
		delete this;
	}else
	{
		status = WINI_ERROR_NOT_CHILD;
	}

	return status;
}

void MetadataNodeImpl::DeleteChild(INode* child)
{
	if((NULL == m_children)||(NULL == child))
		return;

	std::vector<INode*>::iterator pos = m_children->begin();

	for(pos ; pos != m_children->end(); pos++)
	{
		if(*pos == child)	
		{
			m_children->erase(pos);	//should we delete?
			break;
		}
	}	
}


//this function takes a char string. if the string begins
//with a / it will perceive it as a relative path otherwise it will
//take take the entire path as a name. trailing / may be
//present or omitted.
INode* MetadataNodeImpl::GetChild(const char* name)
{
	if((NULL == name)||(NULL == m_children))
		return NULL;

	INode* child;
	char *szname;
	int j;

	if('/' == name[0])
	{
		szname = strdup(name);

		int len = strlen(name);

		for(j = 1; j < len; j++)
		{
			if('/' == szname[j])
			{
				szname[j] = 0;
				break;
			}
		}

		child = GetChild(szname);

		if(NULL == child)
		{
			free(szname);
			return NULL;
		}

		if(j < len)
		{
			child = child->GetChild(&szname[j+1]);
		}

		free(szname);

		return child;
	}else
	{
		for(int i = 0; i < m_children->size(); i++)
		{
			child = m_children->at(i);
			if(0 == strcmp(name, child->GetName()))
			{
				return child;
			}
		}
		
		return NULL;
	}

	//should never reach here
	return NULL;
}

void MetadataNodeImpl::SetOperation(int operation)
{
	//add some error checking - make enum?
	m_operation = operation;

	if(m_name)
	{
		free(m_name);
		m_name = NULL;
	}
}

void MetadataNodeImpl::SetValue(const char* value)
{
	if(m_value)
	{
		free(m_value);
	}

	m_value = strdup(value);

	if(m_name)
	{
		free(m_name);
		m_name = NULL;
	}
}

void MetadataNodeImpl::SetAttribute(const char* attribute)
{
	if(m_attribute)
	{
		free(m_attribute);
	}

	m_attribute = strdup(attribute);

	if(m_name)
	{
		free(m_name);
		m_name = NULL;
	}
}

//metadata nodes are different from other nodes in that they are intended to be moved around
//in the case of defining a query.
void MetadataNodeImpl::SetParent(INode* parent)
{
	m_parent = parent;
}


const char* MetadataNodeImpl::GetOperationString()
{
	return op_map[m_operation];
}



}//end namespace