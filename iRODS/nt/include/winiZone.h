#ifndef __WINI_OBJECT_Zone_NODE_H__
#define __WINI_OBJECT_Zone_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"
//#include "mdasC_db2_externs.h"
#include <vector>

namespace WINI
{
class ZoneNodeImpl : public IZoneNode
{
public:
	ZoneNodeImpl(INode* parent, const char* name);
	 ~ZoneNodeImpl();

	const char* GetName() {return m_name;};
	const char* GetPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	StatusCode GetChild(const char* name, INode** result);
	unsigned int GetType() { return WINI_ZONE;};

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
	unsigned int m_isOpen;
private:
	ZoneNodeImpl();
	ZoneNodeImpl(const ZoneNodeImpl& source);
	ZoneNodeImpl& operator =(const ZoneNodeImpl& source);

	char* m_name;
	char* m_path;
	INode* m_parent;
	std::vector<INode*>* m_children;
	
};


}//end namespace
#endif