#include "winiQuery.h"
#include "winiMetadata.h"
#include "winiCollection.h"
#include "winiDataset.h"


extern "C"
{
//char * getFromResultStruct(mdasC_sql_result_struct *result, char *tab_name, char *att_name);

}

namespace WINI
{

//parent can be null, or another querynode if nested query
QueryNodeImpl::QueryNodeImpl(INode* parent, IMetadataNode* query)
{
	m_parent = parent;
	m_children = new std::vector<INode*>;
	m_name = NULL;
	m_children->push_back(query);
	m_isOpen = WINI_QUERY;	//this one we have to think about
}

//used by QueryOp2
QueryNodeImpl::QueryNodeImpl()
{
	m_name = NULL;
	m_path = NULL;
	m_children = NULL;
	m_parent = NULL;
	m_isOpen = WINI_NONE;
}

QueryNodeImpl::~QueryNodeImpl()
{
	Clear(true);
	delete m_name;
}

const char* QueryNodeImpl::GetPath()
{
	int size;
	
	if(NULL == m_path)
	{
		size = strlen(m_parent->GetPath());

		size += strlen(m_name);

		m_path = new char[++size];

		sprintf(m_path, "%s/%s", m_parent->GetPath(), m_name);
	}

	return m_path;
}

//inline
int QueryNodeImpl::CountChildren()
{
	if(m_children)
	{
		//does this return int?
		return m_children->size();
	}

	return 0;
}

int QueryNodeImpl::CountHeightFromRoot()
{
	if(NULL == m_parent)
	{
		return 0;
	}

	return m_parent->CountHeightFromRoot() + 1;
}

//inline
INode* QueryNodeImpl::GetChild(int pos)
{
	if(m_children && pos < m_children->size() && pos >= 0)
			return m_children->at(pos);
	
	return NULL;
}


void QueryNodeImpl::AddChild(INode* child)
{
	if(NULL == m_children)
	{
		m_children = new std::vector<INode*>;
	}
	
	switch(child->GetType())
	{
	case WINI_DATASET:
		((DatasetNodeImpl*)child)->AddUncle(this);
		break;
	case WINI_COLLECTION:
		((CollectionNodeImpl*)child)->AddUncle(this);
		break;
	}

	m_children->push_back(child);
}

//this function takes a char string. if the string begins
//with a / it will perceive it as a relative path otherwise it will
//take take the entire path as a name. trailing / may be
//present or omitted.
INode* QueryNodeImpl::GetChild(const char* name)
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


void QueryNodeImpl::Clear(bool deleteMeta)
{
	m_isOpen = false;

	if(NULL == m_children)
		return;

	std::vector<INode*>::iterator i = m_children->begin();

	int type;

	INode* blah;
	for(i; i != m_children->end(); i++)
	{
		blah = *i;

		type = blah->GetType();
		if(WINI_QUERY == type)
			delete *i;	//calls destructor on all immediately nested queries
	}

	if(deleteMeta)
	{
		delete m_children;
		m_children = NULL;
	}
}

void QueryNodeImpl::Close()
{
	m_isOpen = false;

	if(NULL == m_children)
		return;

	std::vector<INode*>::iterator i = m_children->begin();

	INode* blah;
	for(i; i != m_children->end(); i++)
	{
		blah = *i;

		if(WINI_QUERY == blah->GetType())
			delete *i;	//calls destructor on all immediately nested queries
	}
	
	i = m_children->begin();

	m_children->erase(++i, m_children->end());	//keep query but lose everything else
}


void QueryNodeImpl::AddChildFront(INode* child)
{
	if(NULL == m_children)
	{
		m_children = new std::vector<INode*>;
	}	

	m_children->insert(m_children->begin(), child);
}

StatusCode QueryNodeImpl::GetChild(const char* name, INode** result)
{
	if(NULL == name)
		return NULL;

	if(NULL == result)
		return NULL;

	INode* child;
	char *szname;
	int j;

	StatusCode status;

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

//A query's name is equal to that of the query itself.
//hence, the name of the query is equal to the name of it's metadata node (child 0)
const char* QueryNodeImpl::GetName()
{
	if(m_name)
		free(m_name);

	const char* blah = GetChild(0)->GetName();

	m_name = strdup(blah);

	return m_name;
}

StatusCode QueryNodeImpl::RemoveChild(INode* target)
{
	if((NULL == m_children)||(NULL == target))
		return WINI_ERROR_INVALID_PARAMETER;

	//this is 
	if(WINI_METADATA == target->GetType())
		m_isOpen = WINI_NONE;


	std::vector<INode*>::iterator pos = m_children->begin();

	for(pos ; pos != m_children->end(); pos++)
	{
		if(*pos == target)	
		{
			m_children->erase(pos);
			break;
		}
	}
	return WINI_OK;
}

}//end namespace