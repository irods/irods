#ifndef __WINI_OBJECT_DATASET_NODE_H__
#define __WINI_OBJECT_DATASET_NODE_H__

#include "winiGlobals.h"
#include "winiObjects.h"
#include "winiOperators.h"
#include <vector>

namespace WINI
{
class DatasetNodeImpl : public IDatasetNode
{
public:
	DatasetNodeImpl(INode* parent, char* name, char* size, char* owner, char* modified_time, char* replicant_number, char* resource_group, char* resource, char* replicant_status, char* checksum);
	~DatasetNodeImpl();

	const char* GetName() {return m_name;};
	const char* GetPath();
	INode* GetParent() {return m_parent;};
	INode* GetChild(int pos);
	StatusCode GetChild(const char* name, INode** result);
	unsigned int GetType() { return WINI_DATASET;};

	int CountChildren();
	int CountHeightFromRoot();

	INode* GetChild(const char* name);
	IDatasetNode* GetReplicant(int replicant_number);

	bool isOpen(unsigned int mask = WINI_ALL) { return ((m_isOpen & mask) == mask); };
	unsigned int GetOpen() { return m_isOpen;};
	const char* GetSize() { return m_size; };
	const char* GetOwner() { return m_owner; };
	const char* GetModifiedTime() { return m_modified_time; };
	const char* GetReplicantNumber() { return m_replicant_number; };
	const char* GetResourceGroup() { return m_resource_group; };
	const char* GetResource() { return m_resource; };
	const char* GetReplicantStatus() { return m_replicant_status; };
	const char* GetChecksum() { return m_checksum; };

	INode* GetUncle(int pos);
	int CountUncles();

	//impl
	void AddUncle(INode* uncle);

	//impl
	StatusCode DeleteChild(INode* child);
	void Clear();
	void SetOpen(unsigned int mask) { m_isOpen = mask;};
	void AddChild(INode* child);
	StatusCode SetName(const char* name);
	void Wash(char* size, char* owner, char* modify_time, char* replicant_number, char* resource_group, char* resource, char* replicant_status, char* checksum);


private:
	DatasetNodeImpl();
	DatasetNodeImpl(const DatasetNodeImpl& source);
	DatasetNodeImpl& operator =(const DatasetNodeImpl& source);

	INode* m_parent;
	char* m_path;
	std::vector<INode*>* m_children;
	std::vector<INode*>* m_uncle;
	unsigned int m_isOpen;

	char* m_name;
	char* m_size;
	char* m_owner;
	char* m_modified_time;
	char* m_replicant_number;
	char* m_resource_group;
	char* m_resource;
	char* m_replicant_status;
	char* m_checksum;
};
}//end namespace
#endif