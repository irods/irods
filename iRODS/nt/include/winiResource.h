#ifndef __WINI_OBJECT_RESOURCE_NODE_H__
#define __WINI_OBJECT_RESOURCE_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"
#include <vector>

namespace WINI
{
class ResourceNodeImpl : public IResourceNode
{
public:
	ResourceNodeImpl();	//root resource node only stores children
	ResourceNodeImpl(INode* parent, const char* name, const char* type);	//child resource nodes from root (and more?)
	 ~ResourceNodeImpl();

	const char* GetName() {return m_name;};
	const char* GetPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	StatusCode GetChild(const char* name, INode** result);
	unsigned int GetType() { return WINI_RESOURCE;};
	const char* GetResourceType() { return m_type;};

	int CountChildren();
	int CountHeightFromRoot();

	INode* GetChild(const char* name);

	bool isOpen(unsigned int mask = WINI_ALL) { return ((m_isOpen & mask) == mask); };
	unsigned int GetOpen() { return m_isOpen;};

	INode* GetUncle(int pos) { return NULL;};
	int CountUncles() { return 0;};

	//impl specific functionality
	void Clear();
	void AddNode(INode* target);
	void SetOpen(unsigned int mask) { m_isOpen = mask;};
	StatusCode DeleteChild(INode* child);

private:
//	ResourceNodeImpl();
	ResourceNodeImpl(const ResourceNodeImpl& source);
	ResourceNodeImpl& operator =(const ResourceNodeImpl& source);

	char* m_name;
	char* m_path;
	INode* m_parent;
	std::vector<INode*>* m_children;
	unsigned int m_isOpen;
	char* m_type;
};

}//end namespace
#endif __WINI_OBJECT_RESOURCE_NODE_H__
