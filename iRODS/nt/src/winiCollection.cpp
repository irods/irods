#include "winiCollection.h"
#include "winiQuery.h"

namespace WINI
{
CollectionNodeImpl::CollectionNodeImpl(INode* parent, const char* name)
{
	m_name = strdup(name);
	m_path = NULL;
	m_parent = parent;
	m_children = NULL;
	m_uncle = NULL;
	m_isOpen = WINI_NONE;
}

#if 1
//root node constructor.
//use this to create the 'root' of the collection tree.
//root_name specifies the name of the root collection, as well as the initial path.
//root_name can be "" (empty_string) or NULL (considered equivalent to "" in this case)
CollectionNodeImpl::CollectionNodeImpl(const char* root_name)
{
	if(root_name)
	{
		m_name = strdup(root_name);
		m_path = (char*)calloc(strlen(root_name) + 2, sizeof(char));
		sprintf(m_path, "/%s", root_name);
	}else
	{
		m_name = strdup("");
		m_path = strdup("/");
	}

	m_parent = NULL;
	m_children = NULL;
	m_uncle = NULL;
	m_isOpen = WINI_NONE;
}
#endif

#if 0
//experimental home-as-root constructor
CollectionNodeImpl::CollectionNodeImpl(const char* name, const char* domain)
{
	char buf[128];

	sprintf(buf, "%s.%s", name, domain);
	m_name = strdup(buf);
	sprintf(buf, "/home/%s", m_name);
	m_path = strdup(buf);
	
	m_parent = NULL;
	m_children = NULL;
	m_uncle = NULL;
	m_isOpen = WINI_NONE;
}
#endif

CollectionNodeImpl::~CollectionNodeImpl()
{
	if(m_uncle)
		delete m_uncle;

	free(m_name);

	if(m_path)
		free(m_path);

	if(m_children)
	{
		std::vector<INode*>::iterator i = m_children->begin();

		for(i; i != m_children->end(); ++i)
			delete *i;
	
		delete m_children;
	}

}

//GetPath on demand
const char* CollectionNodeImpl::GetPath()
{
	int size;
	
	if(NULL == m_path)
	{
		size = strlen(m_parent->GetPath());
		size += strlen(m_name);
		size += 2;	//include slash divider, null characters
		m_path = (char*)calloc(size, sizeof(char));

		//later replace with better implementation
		sprintf(m_path, "%s/%s", m_parent->GetPath(), m_name);
	}

	return m_path;
}

INode* CollectionNodeImpl::GetChild(int pos)
{
	if(m_children && pos < m_children->size() && pos >= 0)
			return m_children->at(pos);
	
	return NULL;
}

int CollectionNodeImpl::CountChildren()
{
	if(m_children)
		return m_children->size();

	return 0;
}

INode* CollectionNodeImpl::GetUncle(int pos)
{
	if(m_uncle && pos < m_uncle->size() && pos >= 0)
			return m_uncle->at(pos);
	
	return NULL;
}

int CollectionNodeImpl::CountUncles()
{
	if(m_uncle)
		return m_uncle->size();

	return 0;
}

int CollectionNodeImpl::CountHeightFromRoot()
{
	if(NULL == m_parent)
		return 0;

	return m_parent->CountHeightFromRoot() + 1;
}

//if string begins with '/' it will be taken as a relative path
//else it will take ptr as the name of a child
//trailing / may be present or omitted
INode* CollectionNodeImpl::GetChild(const char* name)
{
	if(NULL == name)
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

		child = GetChild(&szname[1]);

		szname[j] = '/';

		if(!status.isOk())
		{
			free(szname);
			return child;
		}

		if(j < len)
		{
			child = child->GetChild(&szname[j]);
		}

		free(szname);
		return child;

	}

	char* myptr;// = (char*)child->GetName();
	if(m_children)
	{
		for(int i = 0; i < m_children->size(); i++)
		{
			child = m_children->at(i);
			myptr = (char*)child->GetName();
			if(0 == strcmp(name, myptr))
			{
				return child;
			}
		}
	}

	return NULL;
}

StatusCode CollectionNodeImpl::GetChild(const char* name, INode** result)
{
	if((NULL == name)||(NULL == result))
		return WINI_ERROR_INVALID_PARAMETER;

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

		szname[j] = '/';

		if(!status.isOk())
		{
			free(szname);
			*result = child;
			return status;
		}

		if(j < len)
		{
			status = child->GetChild(&szname[j], &child);
		}

		free(szname);
		*result = child;
		return status;

	}

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

void CollectionNodeImpl::Clear()
{
	std::vector<INode*>::iterator begin, end;

	if(m_children)
	{
		begin = m_children->begin();
		end = m_children->end();

		for(begin; begin != end; begin++)
		{
			delete *begin;
		}

		delete m_children;

		m_children = NULL;
	}

	m_isOpen = WINI_NONE;
}

void CollectionNodeImpl::AddChild(INode* child)
{
	if(NULL == child)
		return;

	if(NULL == m_children)
	{
		m_children = new std::vector<INode*>;
		m_children->reserve(8);
	}	

	m_children->push_back(child);
}

void CollectionNodeImpl::AddUncle(INode* uncle)
{
	if(NULL == uncle)
		return;

	if(NULL == m_uncle)
	{
		m_uncle = new std::vector<INode*>;
	}	

	m_uncle->push_back(uncle);
}

void CollectionNodeImpl::ClearUncle()
{
	if(NULL == m_uncle)
	{
		delete m_uncle;
		m_uncle = NULL;
	}
}

StatusCode CollectionNodeImpl::DeleteUncle(INode* uncle)
{
	if(NULL == uncle)
		return WINI_ERROR_INVALID_PARAMETER;

	if(NULL == m_uncle)
		return WINI_ERROR_UNCLE_NOT_FOUND;

	std::vector<INode*>::iterator i = m_uncle->begin();

	for(i; i != m_uncle->end(); i++)
	{
		if(*i == uncle)
		{
			m_uncle->erase(i); //erases from vector
			return WINI_OK;
		}
	}

	return WINI_ERROR_CHILD_NOT_FOUND;
}

StatusCode CollectionNodeImpl::DeleteChild(INode* child)
{
	if(NULL == child)
		return WINI_ERROR_INVALID_PARAMETER;

	if(NULL == m_children)
		return WINI_ERROR_NOT_FILLED;

	std::vector<INode*>::iterator i = m_children->begin();

	QueryNodeImpl* query;
	
	for(i; i != m_children->end(); i++)
	{
		if(*i == child)
		{
			m_children->erase(i); //erases from vector

			int ucount = child->CountUncles();

			if(ucount)
			{
				for(int i = 0; i < ucount; i++)
				{
					//assume for right now that only queries assert themselves as uncles
					query = (QueryNodeImpl*)child->GetUncle(i);
					query->RemoveChild(child);
				}
			}

			delete child;
			return WINI_OK;
		}
	}
	return WINI_ERROR_CHILD_NOT_FOUND;
}

void CollectionNodeImpl::SetName(const char* name)
{
	if(name)
	{
		free(m_name);
		m_name = strdup(name);

		if(m_path)
		{
			free(m_path);
			m_path = NULL;
		}
	}
}

}//end namespace