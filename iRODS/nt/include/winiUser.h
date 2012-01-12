#ifndef __WINI_OBJECT_USER_NODE_H__
#define __WINI_OBJECT_USER_NODE_H__

#include "winiGlobals.h"
#include "winiObjects.h"
#include <vector>

namespace WINI
{
class UserNodeImpl : public IUserNode
{
public:
	UserNodeImpl(INode* parent, const char* name, const char* type);
	 ~UserNodeImpl();

	const char* GetPermission() {return m_permission;};
	void SetPermission(const char* permission) { m_permission = strdup(permission);};
	const char* GetName() {return m_name;};
	const char* GetPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	StatusCode GetChild(const char* name, INode** result);
	const char* GetUserType() {return m_type;};
	unsigned int GetType() { return WINI_USER;};

	int CountChildren();
	int CountHeightFromRoot();

	INode* GetChild(const char* name);

	bool isOpen(unsigned int mask = WINI_ALL) { return ((m_isOpen & mask) == mask); };
	unsigned int GetOpen() { return m_isOpen;};

	INode* GetUncle(int pos) { return NULL;};
	int CountUncles() { return 0;};

	//impl
	void AddNode(INode* target);
	StatusCode DeleteChild(INode* child);
	void Clear();

private:
	UserNodeImpl();
	UserNodeImpl(const UserNodeImpl& source);
	UserNodeImpl& operator =(const UserNodeImpl& source);

	char* m_name;
	char* m_path;
	INode* m_parent;
	std::vector<INode*>* m_children;
	unsigned int m_isOpen;
	char* m_permission;
	char* m_type;
};
}//end namespace
#endif