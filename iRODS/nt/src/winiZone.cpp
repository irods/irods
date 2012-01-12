#include "winiZone.h"

namespace WINI
{
ZoneNodeImpl::ZoneNodeImpl(INode* parent, const char* Zone)
{
	if(Zone)
		m_name = strdup(Zone);
	else
		m_name = strdup("(null)");

	m_parent = parent;
	m_path = NULL;
	m_children = NULL;
	m_isOpen = WINI_NONE;
}

ZoneNodeImpl::~ZoneNodeImpl()
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

const char* ZoneNodeImpl::GetPath()
{
	int size;
	
	if(NULL == m_path)
	{
		size = strlen(m_name);
		size += 2;	//include slash divider, null characters
		m_path = (char*)calloc(size, sizeof(char));

		//later replace with better implementation
		sprintf(m_path, "/%s", m_name);
	}

	return m_path;
}

INode* ZoneNodeImpl::GetChild(int pos)
{
	if(m_children && pos < m_children->size() && pos >= 0)
			return m_children->at(pos);
	
	return NULL;
}

int ZoneNodeImpl::CountChildren()
{
	if(m_children)
		return m_children->size();

	return 0;
}

StatusCode ZoneNodeImpl::DeleteChild(INode* child)
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
INode* ZoneNodeImpl::GetChild(const char* name)
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

StatusCode ZoneNodeImpl::GetChild(const char* name, INode** result)
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

void ZoneNodeImpl::Clear()
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

void ZoneNodeImpl::AddChild(INode* child)
{
	if(NULL == m_children)
		m_children = new std::vector<INode*>;
	
	m_children->push_back(child);
}

}//end namespace