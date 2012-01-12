#include "winiResource.h"

namespace WINI
{

ResourceNodeImpl::ResourceNodeImpl()
{
	m_name = strdup("(resource list)");
	m_path = strdup("");
	m_parent = NULL;
	m_children = NULL;
	m_isOpen = false;
	m_type = NULL;
}

ResourceNodeImpl::ResourceNodeImpl(INode* parent, const char* name, const char* type)
{
	m_name = strdup(name);
	m_path = NULL;
	m_parent = parent;
	m_children = NULL;
	m_isOpen = false;
	m_type = strdup(type);
}

ResourceNodeImpl::~ResourceNodeImpl()
{
	SMART_FREE(m_name);
	SMART_FREE(m_path);
	SMART_FREE(m_type);

	int count;
	INode* child;
	if(m_children)
	{
		count = CountChildren();
		for(int i = 0; i < count; i++)
		{
			child = GetChild(i);
			delete child;
		}

		delete m_children;
	}
}

const char* ResourceNodeImpl::GetPath()
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
INode* ResourceNodeImpl::GetChild(int pos)
{
	if(m_children && pos < m_children->size() && pos >= 0)
			return m_children->at(pos);
	
	return NULL;
}


StatusCode ResourceNodeImpl::GetChild(const char* name, INode** result)
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

//inline
int ResourceNodeImpl::CountChildren()
{
	if(m_children)
	{
		//does this return int?
		return m_children->size();
	}

	return 0;
}

int ResourceNodeImpl::CountHeightFromRoot()
{
	if(NULL == m_parent)
	{
		return 0;
	}

	return m_parent->CountHeightFromRoot() + 1;
}

/*
this function takes a char string. if the string begins
with a / it will perceive it as a relative path otherwise it will
take take the entire path as a name. trailing / may be
present or omitted.
*/
INode* ResourceNodeImpl::GetChild(const char* name)
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

		child = GetChild(&szname[1]);

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

void ResourceNodeImpl::Clear()
{
	if(NULL == m_children)
		return;

	std::vector<INode*>::iterator i = m_children->begin();

	for(i; i != m_children->end(); i++)
	{
		delete *i;	//calls destructor on all children
	}

	m_children->clear();

	m_isOpen = false;
}

void ResourceNodeImpl::AddNode(INode* target)
{
	if(NULL == m_children)
	{
		m_children = new std::vector<INode*>;
	}	
	m_children->push_back(target);
}

StatusCode ResourceNodeImpl::DeleteChild(INode* child)
{
	if(NULL == m_children)
		return WINI_ERROR_INVALID_PARAMETER;

	std::vector<INode*>::iterator i = m_children->begin();

	for(i; i != m_children->end(); i++)
	{
		if(*i == child)
		{
			m_children->erase(i); //erases from vector
			delete child;	//frees allocation, calls destructor, frees child elements if any.
			return WINI_OK;
		}
	}

	return WINI_ERROR;
}

}//end namespace