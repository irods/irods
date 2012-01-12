#ifndef __WINI_OBJECT_ICAT_NODE_H__
#define __WINI_OBJECT_ICAT_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"
//#include "mdasC_db2_externs.h"
#include <vector>

namespace WINI
{
class CatalogNodeImpl : public ICatalogNode
{
public:
	CatalogNodeImpl(const char* name);
	 ~CatalogNodeImpl();

	const char* GetName() {return m_name;};
	const char* GetPath() {return m_name;};
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	StatusCode GetChild(const char* name, INode** result);
	unsigned int GetType() { return WINI_CATALOG;};

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
	CatalogNodeImpl();
	CatalogNodeImpl(const CatalogNodeImpl& source);
	CatalogNodeImpl& operator =(const CatalogNodeImpl& source);

	char* m_name;
	INode* m_parent;
	std::vector<INode*>* m_children;
	unsigned int m_isOpen;
};


}//end namespace
#endif