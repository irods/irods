#ifndef __WINI_OBJECT_SET_NODE_H__
#define __WINI_OBJECT_SET_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"
//#include "mdasC_db2_externs.h"
#include <vector>

namespace WINI
{
class SetNodeImpl : public ISetNode
{
public:
	SetNodeImpl(INode* parent, const char* name, unsigned int set_type);
	 ~SetNodeImpl();

	const char* GetName() {return m_name;};
	const char* GetPath() {return m_name;};
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	StatusCode GetChild(const char* name, INode** result);
	unsigned int GetType() { return WINI_SET;};
	unsigned int GetSetType() { return m_set_type;};

	int CountChildren();
	int CountHeightFromRoot() { return 0;};

	INode* GetChild(const char* name);
	void SetOpen(unsigned int mask) { m_isOpen = mask;};
	bool isOpen(unsigned int mask = WINI_ALL) { return ((m_isOpen & mask) == mask); };
	unsigned int GetOpen() { return m_isOpen;};

	INode* GetUncle(int pos) { return NULL;};
	int CountUncles() { return 0;};

	//impl
	void AddChild(INode* child);
	StatusCode DeleteChild(INode* child);
	void Clear();

private:
	SetNodeImpl();
	SetNodeImpl(const SetNodeImpl& source);
	SetNodeImpl& operator =(const SetNodeImpl& source);

	char* m_name;
	INode* m_parent;
	std::vector<INode*>* m_children;
	unsigned int m_isOpen;
	unsigned int m_set_type;
};


}//end namespace
#endif