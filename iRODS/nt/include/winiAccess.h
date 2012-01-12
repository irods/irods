#ifndef __WINI_OBJECT_ACCESS_NODE_H__
#define __WINI_OBJECT_ACCESS_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"

namespace WINI
{

class AccessNodeImpl : public IAccessNode
{
public:
	AccessNodeImpl(INode* parent, const char* group, const char* domain, const char* user, const char* constraint);
	~AccessNodeImpl();

	const char* GetName();
	const char* GetPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	unsigned int GetType() { return WINI_ACCESS;};

	int CountChildren();
	int CountHeightFromRoot();

	INode* GetPrevious(INode* child, bool recursive);
	INode* GetChild(const char* name);
	StatusCode GetChild(const char* name, INode** result);
	INode* GetNext(INode* child, bool recursive);

	bool isOpen(unsigned int mask = WINI_ALL) { return ((m_isOpen & mask) == mask); };
	unsigned int GetOpen() { return m_isOpen;};

	const char* GetGroup() {return m_group;};
	const char* GetDomain() {return m_domain;};
	const char* GetUser() {return m_user;};
	const char* GetConstraint() {return m_constraint;};

	INode* GetUncle(int pos) { return 0;};
	int CountUncles() { return 0;};

	//impl specific functionality
	StatusCode SetPermission(const char* permission);

private:
	AccessNodeImpl();
	AccessNodeImpl(const AccessNodeImpl& source);
	AccessNodeImpl& operator =(const AccessNodeImpl& source);

	char* m_name;
	char* m_path;
	INode* m_parent;
	unsigned int m_isOpen;
	char* m_group;
	char* m_domain;
	char* m_user;
	char* m_constraint;
};

}//end namespace

#endif
