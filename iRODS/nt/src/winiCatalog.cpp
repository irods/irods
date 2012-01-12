#include "winiCatalog.h"

namespace WINI
{

CatalogNodeImpl::CatalogNodeImpl(const char* catalog)
{
	if(catalog)
		m_name = strdup(catalog);
	else
		m_name = strdup("(null)");

	m_parent = NULL;
	m_children = NULL;
	m_isOpen = WINI_NONE;
}

CatalogNodeImpl::~CatalogNodeImpl()
{
	std::vector<INode*>::iterator begin, end;

	if(m_children)
	{
		begin = m_children->begin();
		end = m_children->end();

		for(begin; begin != end; begin++)
			delete *begin;

		delete m_children;
	}

	free(m_name);
}

INode* CatalogNodeImpl::GetChild(int pos)
{
	if(m_children && pos < m_children->size() && pos >= 0)
			return m_children->at(pos);
	
	return NULL;
}

int CatalogNodeImpl::CountChildren()
{
	if(m_children)
		return m_children->size();

	return 0;
}

StatusCode CatalogNodeImpl::DeleteChild(INode* child)
{
	if(NULL == child)
		return WINI_ERROR_INVALID_PARAMETER;

	if(NULL == m_children)
		return WINI_ERROR_NOT_FILLED;

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

	return WINI_ERROR_CHILD_NOT_FOUND;
}

//if string begins with '/' it will be taken as a relative path
//else it will take ptr as the name of a child
//trailing / may be present or omitted
INode* CatalogNodeImpl::GetChild(const char* name)
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
	}
	//else

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

StatusCode CatalogNodeImpl::GetChild(const char* name, INode** result)
{
	if((NULL == name)||(NULL == result))
		return WINI_ERROR_INVALID_PARAMETER;

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

	}
	//else
	
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

	*result = NULL;
	return WINI_ERROR_CHILD_NOT_FOUND;

}

void CatalogNodeImpl::Clear()
{
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

void CatalogNodeImpl::AddChild(INode* child)
{
	if(NULL == m_children)
		m_children = new std::vector<INode*>;
	
	m_children->push_back(child);
}

}//end namespace