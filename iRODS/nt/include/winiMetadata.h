#ifndef __WINI_OBJECT_METADATA_NODE_H__
#define __WINI_OBJECT_METADATA_NODE_H__
#include "winiGlobals.h"
#include "winiObjects.h"
//#include "mdasC_db2_externs.h"
#include <vector>

namespace WINI
{

class MetadataNodeImpl: public IMetadataNode
{
public:
	MetadataNodeImpl();
	 ~MetadataNodeImpl();
	MetadataNodeImpl(INode* parent, const char* attribute);
	MetadataNodeImpl(IMetadataNode* left, int operation, IMetadataNode* right);
	//MetadataNodeImpl(INode* parent, mdasC_sql_result_struct* result, int index);
	MetadataNodeImpl(INode* parent, const char* attribute, int operation, const char* value);
	void SetID(int i);

	const char* GetOperationString();
	const char* GetAttribute() { return m_attribute;};
	const char* GetValue() { return m_value;};
	int GetOperation() {return m_operation;};

	const char* GetName();
	const char* GetPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	StatusCode GetChild(const char* name, INode** result);
	unsigned int GetType() { return WINI_METADATA;};
	int GetID() { return m_ID;};

	int CountChildren();
	int CountHeightFromRoot();

	INode* GetChild(const char* name);

	bool isOpen(unsigned int mask = WINI_ALL) { return ((m_isOpen & mask) == mask); };
	unsigned int GetOpen() { return m_isOpen;};

	INode* GetUncle(int pos) { return NULL;};
	int CountUncles() { return 0;};

	//impl
	void AddChild(INode* child);
	void DeleteChild(INode* child); //removes node without destroying children or alerting parent
	StatusCode DeleteChild2(INode* child);
	void SetOperation(int operation);
	void SetValue(const char* value);
	void SetAttribute(const char* value);
	void SetParent(INode* parent);

	//create a new node which has this this node and 'node' as child elements
	IMetadataNode* And(IMetadataNode* node);

private:
//	MetadataNodeImpl();
	MetadataNodeImpl(const MetadataNodeImpl& source);
	MetadataNodeImpl& operator =(const MetadataNodeImpl& source);

	char* m_name;
	char* m_path;
	std::vector<INode*>* m_children;
	INode* m_parent;
	unsigned int m_isOpen;
	char* m_attribute;
	char* m_value;
	int m_operation;
	int m_ID;
};

}//end namespace
#endif
