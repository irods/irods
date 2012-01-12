#include "winiGlobals.h"
#include "winiObjects.h"
#include "winiAccess.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace WINI
{

AccessNodeImpl::AccessNodeImpl(INode* parent, const char* group, const char* domain, const char* user, const char* constraint)
{
	m_parent = parent;
	m_group = strdup(group);
	m_domain = strdup(domain);
	m_user = strdup(user);
	m_constraint = strdup(constraint);
	m_isOpen = WINI_ALL;
	m_name = NULL;
	m_path = NULL;
}

AccessNodeImpl::~AccessNodeImpl()
{
	if(m_group)
		free(m_group);
	if(m_domain)
		free(m_domain);
	if(m_user)
		free(m_user); 
	if(m_constraint)
		free(m_constraint);
	if(m_name)
		free(m_name);  
	if(m_path)
		free(m_path);
}

const char* AccessNodeImpl::GetName()
{
	if(m_name)
		free(m_name);

	int len = 0;

	len += strlen(m_user);
	len += strlen(m_domain);
	len += strlen(m_constraint);
	len += 9;
	m_name = (char*)calloc(len, sizeof(char));
	sprintf(m_name, "%s @ %s:  %s", m_user, m_domain, m_constraint);

	return m_name;
}

//inline
const char* AccessNodeImpl::GetPath()
{
	return NULL;
}

//inline
INode* AccessNodeImpl::GetChild(int pos)
{
	return NULL;
}
//inline
int AccessNodeImpl::CountChildren()
{
	return 0;
}

int AccessNodeImpl::CountHeightFromRoot()
{
	if(NULL == m_parent)
	{
		return 0;
	}

	return m_parent->CountHeightFromRoot() + 1;
}

INode* AccessNodeImpl::GetPrevious(INode* child, bool recursive)
{
	return NULL;
}

INode* AccessNodeImpl::GetChild(const char* name)
{
	return NULL;
}

StatusCode AccessNodeImpl::GetChild(const char* name, INode** result)
{
	return NULL;
}

INode* AccessNodeImpl::GetNext(INode* child, bool recursive)
{
	return NULL;
}

StatusCode AccessNodeImpl::SetPermission(const char* permission)
{
	if(permission == NULL)
		return WINI_ERROR_INVALID_PARAMETER;

	if(m_name)
	{
		free(m_name);
		m_name = NULL;
	}

	if(m_path)
	{
		free(m_path);
		m_path = NULL;
	}

	if(m_constraint)
	{
		free(m_constraint);
		m_constraint = strdup(permission);
	}

	return WINI_OK;
}

}//end namespace