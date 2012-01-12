#ifndef __WINI_OBJECT_STRING_NODE_H__
#define __WINI_OBJECT_STRING_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"
#include <vector>

namespace WINI
{
class StringNodeImpl : public IStringNode
{
public:
	StringNodeImpl(INode* parent);
	 ~StringNodeImpl();

	const char* GetName() {return m_name;};
	const char* GetPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	StatusCode GetChild(const char* name, INode** result);
	unsigned int GetType() { return WINI_DATASET;};

	int CountChildren();
	int CountHeightFromRoot();

	INode* GetChild(const char* name);

	bool isOpen(unsigned int mask = WINI_ALL) { return ((m_isOpen & mask) == mask); };
	unsigned int GetOpen() { return m_isOpen;};

	const char* GetString(int pos);
	int CountStrings();

	INode* GetUncle(int pos) { return NULL;};
	int CountUncles() { return 0;};

	//impl
	StatusCode AddString(const char* string);
	StatusCode DeleteString(const char* string);

private:
	StringNodeImpl();
	StringNodeImpl(const StringNodeImpl& source);
	StringNodeImpl& operator =(const StringNodeImpl& source);

	char* m_name;
	char* m_path;
	INode* m_parent;
	std::vector<INode*>* m_children;
	std::vector<char*>* m_strings;
	unsigned int m_isOpen;
};


}//end namespace
#endif