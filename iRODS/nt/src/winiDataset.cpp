#include "winiDataset.h"
#include "winiQuery.h"

namespace WINI
{

DatasetNodeImpl::DatasetNodeImpl(INode* parent, char* name, char* size, char* owner,
								 char* modified_time, char* replicant_number, char* resource_group,
								 char* resource, char* replicant_status, char* checksum)
{
	m_parent = parent;
	m_path = NULL;
	m_children = NULL;
	m_uncle = NULL;
	m_isOpen = WINI_NONE;

	m_name = strdup(name);
	m_size = strdup(size);
	m_owner = strdup(owner);
	m_modified_time = strdup(modified_time);
	m_replicant_number = strdup(replicant_number);
	m_resource_group = strdup(resource_group);
	m_resource = strdup(resource);
	m_replicant_status = strdup(replicant_status);
	m_checksum = strdup(checksum);
}

DatasetNodeImpl::~DatasetNodeImpl()
{
	if(m_path != NULL)
		free(m_path);
	if(m_name != NULL)
		free(m_name);
	if(m_size != NULL)
		free(m_size);
	if(m_owner != NULL)
		free(m_owner);
	if(m_modified_time != NULL)
		free(m_modified_time);
	if(m_replicant_number != NULL)
		free(m_replicant_number);
	if(m_resource_group != NULL)
		free(m_resource_group);
	if(m_resource != NULL)
		free(m_resource);
	if(m_replicant_status != NULL)
		free(m_replicant_status);
	if(m_checksum != NULL)
		free(m_checksum);

	INode* child;

	if(m_uncle)
		delete m_uncle;

	if(m_children)
	{
		int count = m_children->size();
		for(int i = count - 1; i >= 0; i--)
		{
			child = m_children->at(i);
			delete child;
		}
		delete m_children;
	}
}

void DatasetNodeImpl::Wash(char* size, char* owner, char* modified_time, char* replicant_number, char* resource_group, char* resource, char* replicant_status, char* checksum)
{
	//1. Delete
	if(m_size != NULL)
		free(m_size);
	if(m_owner != NULL)
		free(m_owner);
	if(m_modified_time != NULL)
		free(m_modified_time);
	if(m_replicant_number != NULL)
		free(m_replicant_number);
	if(m_resource_group != NULL)
		free(m_resource_group);
	if(m_resource != NULL)
		free(m_resource);
	if(m_replicant_status != NULL)
		free(m_replicant_status);
	if(m_checksum != NULL)
		free(m_checksum);

	//children, including metadata and access rights, stay the same

	//2. ReCreate
	//Note some variables like the parent and any uncles are and should stay untouched
	m_size = strdup(size);
	m_owner = strdup(owner);
	m_modified_time = strdup(modified_time);
	m_replicant_number = strdup(replicant_number);
	m_resource_group = strdup(resource_group);
	m_resource = strdup(resource);
	m_replicant_status = strdup(replicant_status);
	m_checksum = strdup(checksum);
}

const char* DatasetNodeImpl::GetPath()
{
	int size;
	
	if(NULL == m_path)
	{
		size = strlen(m_parent->GetPath());

		size += strlen(m_name);

		m_path = new char[size + 2];	//one for '/' and one for null

		sprintf(m_path, "%s/%s", m_parent->GetPath(), m_name);
	}

	return m_path;
}

//inline
INode* DatasetNodeImpl::GetChild(int pos)
{
	if(m_children && pos < m_children->size() && pos >= 0)
			return m_children->at(pos);
	
	return NULL;
}

INode* DatasetNodeImpl::GetUncle(int pos)
{
	if(m_uncle && pos < m_uncle->size() && pos >= 0)
			return m_uncle->at(pos);
	
	return NULL;
}

//it's inefficient, but since we don't add new nodes in any set order and the data
//structure's a vector, we have to search through all possibilites.
IDatasetNode* DatasetNodeImpl::GetReplicant(int replicant_number)
{
	if(replicant_number < 0)
		return NULL;

	if(replicant_number == atoi(m_replicant_number))
		return this;

	int count = m_parent->CountChildren();

	INode* node;

	for(int i = 0; i < count; i++)
	{
		node = m_parent->GetChild(i);

		if(WINI_DATASET == node->GetType())
		{
			if(0 == strcmp(node->GetName(), m_name))
			{
				if(replicant_number == atoi(((IDatasetNode*)node)->GetReplicantNumber()))
					return (IDatasetNode*)node;
			}
		}
	}

	return NULL;
}

//inline
int DatasetNodeImpl::CountChildren()
{
	if(m_children)
		return m_children->size();

	return 0;
}

int DatasetNodeImpl::CountUncles()
{
	if(m_uncle)
		return m_uncle->size();

	return 0;
}

int DatasetNodeImpl::CountHeightFromRoot()
{
	if(NULL == m_parent)
		return 0;

	return m_parent->CountHeightFromRoot() + 1;
}


StatusCode DatasetNodeImpl::DeleteChild(INode* child)
{
	if(NULL == m_children)
		return WINI_ERROR_INVALID_PARAMETER;

	std::vector<INode*>::iterator i = m_children->begin();

	for(i; i != m_children->end(); i++)
	{
		if(*i == child)
		{
			m_children->erase(i);

			int ucount = child->CountUncles();
			if(ucount)
			{
				QueryNodeImpl* query;
				for(int i = 0; i < ucount; i++)
				{
					query = (QueryNodeImpl*)child->GetUncle(i);
					query->RemoveChild(child);
				}
			}

			delete child;

			return WINI_OK;
		}
	}
	return WINI_ERROR;
}

/*
this function takes a char string. if the string begins
with a / it will perceive it as a relative path otherwise it will
take take the entire path as a name. trailing / may be
present or omitted.
*/
INode* DatasetNodeImpl::GetChild(const char* name)
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

StatusCode DatasetNodeImpl::GetChild(const char* name, INode** result)
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

void DatasetNodeImpl::Clear()
{
	if(NULL == m_children)
		return;

	std::vector<INode*>::iterator begin, end;

	if(m_children)
	{
		begin = m_children->begin();
		end = m_children->end();

		for(begin; begin != end; begin++)
			delete *begin;

		delete m_children;
		m_children = NULL;
	}

	m_isOpen = WINI_NONE;
}

void DatasetNodeImpl::AddChild(INode* child)
{
	if(NULL == m_children)
		m_children = new std::vector<INode*>;
	
	m_children->push_back(child);
}

void DatasetNodeImpl::AddUncle(INode* uncle)
{
	if(NULL == m_uncle)
		m_uncle = new std::vector<INode*>;
	
	m_uncle->push_back(uncle);
}

StatusCode DatasetNodeImpl::SetName(const char* name)
{
	if(NULL == name)
		return WINI_ERROR_INVALID_PARAMETER;

	if(m_path)
		free(m_path);

	if(m_name)
		free(m_name);

	m_name = strdup(name);

	return WINI_OK;
}

}//end namespace