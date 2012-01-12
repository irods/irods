#ifndef __WINI_OBJECT_INTERFACE_H__
#define __WINI_OBJECT_INTERFACE_H__

#include "winiStatusCode.h"

namespace WINI
{

class INode
{
public:
	virtual ~INode() {};
	virtual const char* GetName() = 0;
	virtual const char* GetPath() = 0;
	virtual INode* GetParent() = 0;
	virtual INode* GetChild(int pos) = 0;
	virtual unsigned int GetType() = 0;

	virtual int CountChildren() = 0;
	virtual int CountHeightFromRoot() = 0;

	//if initial character is '/' then traverse path
	virtual INode* GetChild(const char* name) = 0;
	virtual StatusCode GetChild(const char* name, INode** result) = 0;

	virtual bool isOpen(unsigned int mask = WINI_ALL) = 0;
	virtual unsigned int GetOpen() = 0;

	virtual INode* GetUncle(int pos) = 0;
	virtual int CountUncles() = 0;
};

class IMetadataNode: public INode
{
public:
	virtual ~IMetadataNode() {};
	virtual const char* GetAttribute() = 0;
	virtual const char* GetValue() = 0;
	virtual int GetOperation() = 0;
	virtual const char* GetOperationString() = 0;
};

class IStringNode: public INode
{
public:
	virtual ~IStringNode() {};
	virtual const char* GetString(int pos) = 0;
	virtual int CountStrings() = 0;
};

class IDatasetNode : public INode
{
public:
	virtual ~IDatasetNode() {};
	virtual const char* GetSize() = 0;
	virtual const char* GetOwner() = 0;
	virtual const char* GetModifiedTime() = 0;
	virtual const char* GetReplicantNumber() = 0;
	virtual const char* GetResourceGroup() = 0;
	virtual const char* GetResource() = 0;
	virtual const char* GetReplicantStatus() = 0;
	virtual const char* GetChecksum() = 0;
	virtual IDatasetNode* GetReplicant(int replication_index) = 0;
};

class IAccessNode : public INode
{
public:
	virtual ~IAccessNode() {};
	virtual const char* GetGroup() = 0;
	virtual const char* GetDomain() = 0;
	virtual const char* GetUser() = 0;
	virtual const char* GetConstraint() = 0;
};

class ICollectionNode : public INode
{
public:
	virtual ~ICollectionNode() {};
};

class IResourceNode : public INode
{
public:
	virtual ~IResourceNode() {};
	virtual const char* GetResourceType() = 0;
};

class IDomainNode : public INode
{
public:
	virtual ~IDomainNode() {};
};

class ISetNode : public INode
{
public:
	virtual ~ISetNode() {};
	virtual unsigned int GetSetType() = 0;
};

class IZoneNode : public INode
{
public:
	virtual ~IZoneNode() {};
};

class ICatalogNode : public INode
{
public:
	virtual ~ICatalogNode() {};
};

class IUserNode : public IDomainNode
{
public:
	virtual ~IUserNode() {};
};

class IQueryNode : public INode
{
public:
	virtual ~IQueryNode() {};
};

}	//end namespace

#endif __WINI_OBJECT_INTERFACE_H__
