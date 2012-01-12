#ifndef __WINI_OBJECT_CONTAINER_NODE_H__
#define __WINI_OBJECT_CONTAINER_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"
#include <vector>

namespace WINI
{
class ContainerNodeImpl : public IContainerNode
{
public:
	ContainerNodeImpl(INode* parent, const char* container_path);
	 ~ContainerNodeImpl();
	const char* GetName();
	const char* GetPath();
	const char* GetContainerPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos) { return NULL;};
	StatusCode GetChild(const char* name, INode** result);
	unsigned int GetType() { return WINI_CONTAINER;};

	int CountChildren() { return 0;};
	int CountHeightFromRoot();

	INode* GetChild(const char* name) { return NULL;};

	bool isOpen(unsigned int mask = WINI_ALL) { return true; };
	unsigned int GetOpen() { return WINI_ALL;};

	INode* GetUncle(int pos) { return NULL;};
	int CountUncles() { return 0;};


private:
	char* m_name;
	char* m_path;
	char* m_container_path;
	INode* m_parent;
};


}//end namespace
#endif