#ifndef __WINI_OBJECT_COLLECTION_NODE_H__
#define __WINI_OBJECT_COLLECTION_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"
//#include "mdasC_db2_externs.h"
#include <vector>

namespace WINI
{
class CollectionNodeImpl : public ICollectionNode
{
public:
	CollectionNodeImpl(const char* root_name);
	CollectionNodeImpl(INode* parent, const char* name);
	 ~CollectionNodeImpl();

	const char* GetName() {return m_name;};
	const char* GetPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	unsigned int GetType() { return WINI_COLLECTION;};

	int CountChildren();
	int CountHeightFromRoot();

	INode* GetChild(const char* name);
	StatusCode GetChild(const char* name, INode** result);
	bool isOpen(unsigned int mask = WINI_ALL) { return ((m_isOpen & mask) == mask); };
	unsigned int GetOpen() { return m_isOpen;};

	INode* GetUncle(int pos);
	int CountUncles();

	//impl specific functionality
	void AddUncle(INode* uncle);

	//currently implemented but unused since only time we need to delete an uncle referene
	//is when we're deleting the node anyway
	StatusCode DeleteUncle(INode* uncle);
	void ClearUncle();

	void Clear();
	void AddChild(INode* child);
	void SetOpen(unsigned int mask) { m_isOpen = mask;};
	StatusCode DeleteChild(INode* child);
	void SetName(const char* name);

private:
	CollectionNodeImpl();
	CollectionNodeImpl(const CollectionNodeImpl& source);
	CollectionNodeImpl& operator =(const CollectionNodeImpl& source);

	char* m_name;
	char* m_path;
	INode* m_parent;
	std::vector<INode*>* m_children;
	std::vector<INode*>* m_uncle;
	unsigned int m_isOpen;
};

}//end namespace
#endif
