#ifndef __WINI_OBJECT_QUERY_NODE_H__
#define __WINI_OBJECT_QUERY_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"
//#include "mdasC_db2_externs.h"
#include <vector>

namespace WINI
{
class QueryNodeImpl: public IQueryNode
{
public:
	QueryNodeImpl();
	QueryNodeImpl(INode* parent, IMetadataNode* query);
	 ~QueryNodeImpl();

	const char* GetName();
	const char* GetPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	unsigned int GetType() { return WINI_QUERY;};

	int CountChildren();
	int CountHeightFromRoot();

	INode* GetChild(const char* name);
	StatusCode GetChild(const char* name, INode** result);

	bool isOpen(unsigned int mask = WINI_ALL) { return ((m_isOpen & mask) == mask); };
	unsigned int GetOpen() { return m_isOpen;};

	INode* GetUncle(int pos) { return NULL;};
	int CountUncles() { return 0;};


	//impl
	void AddChild(INode* Child);
	StatusCode RemoveChild(INode* target);
	void Clear(bool deleteMeta);
	void Close();
	void SetOpen(unsigned int mask) { m_isOpen = mask;};
	void AddChildFront(INode* child);
	void SetParent(INode* parent) { m_parent = parent; };

private:

	QueryNodeImpl(const QueryNodeImpl& source);
	QueryNodeImpl& operator =(const QueryNodeImpl& source);

	char* m_name;
	char* m_path;
	std::vector<INode*> *m_children;
	INode* m_parent;
	unsigned int m_isOpen;

};

}//end namespace
#endif
