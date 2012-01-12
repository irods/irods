#ifndef __WINI_OPERATOR_INTERFACE_H__
#define __WINI_OPERATOR_INTERFACE_H__

#include "winiStatusCode.h"
#include "winiObjects.h"

namespace WINI
{

class IOperator
{
public:
	virtual ~IOperator() {};
	virtual StatusCode Bind(INode* node) = 0;
	virtual int GetType() = 0;
};

class ITransferOperator : public IOperator
{
public:
	virtual ~ITransferOperator() {};
	virtual StatusCode Download(INode* target, const char* local_path) = 0;
	virtual StatusCode Upload(INode* target, const char* local_path, unsigned int overwrite, INode** result = 0) = 0;
	virtual int GetProgress(char** name) = 0;
};

class ISession
{
public:
	virtual ~ISession() {};
	virtual IStringNode* GetAccessConstraints() = 0;
	virtual StatusCode Clone(ISession* target) = 0;
	virtual IOperator* GetOperator(int type) = 0;
	virtual StatusCode Connect()  = 0;
	virtual StatusCode Disconnect()  = 0;
	virtual bool isConnected() = 0;
	virtual StatusCode ChangePassword(const char* new_password)  = 0;
	virtual StatusCode OpenTree(INode* parent, int levels, bool clear, unsigned int mask = WINI_ALL)  = 0;
	virtual StatusCode CloseTree(INode* parent) = 0;
	virtual ICollectionNode* GetUserRootCol() = 0;
	virtual ICollectionNode* GetUserHomeCol() = 0;
	virtual ICatalogNode* GetCatalog() = 0;
	virtual IZoneNode* GetUserZone() = 0;
	virtual ICollectionNode* GetUserHome() = 0;
	virtual ISetNode* GetResource(IZoneNode* zone) = 0;
	virtual ICollectionNode* GetRoot(IZoneNode* zone) = 0;
	virtual ICollectionNode* GetLocalRoot() = 0;
	virtual ICollectionNode* GetHome(IZoneNode* zone) = 0;
	virtual IZoneNode* GetCurrentZone(INode* target) = 0;
	virtual const char* GetName() = 0;
	virtual const char* GetUserHost() = 0;
	virtual int GetUserPortValue() = 0;
	virtual void* GetConn() = 0;
	virtual void* CloneConn() = 0;
};


class IDatasetOperator : public ITransferOperator
{
public:
	virtual ~IDatasetOperator() {};
	virtual StatusCode Delete(IDatasetNode* target) = 0;
	virtual StatusCode Replicate(IDatasetNode* target) = 0;
	virtual StatusCode Rename(IDatasetNode* target, const char* name) = 0;
	virtual StatusCode SetComment(IDatasetNode* target, const char* comment) = 0;
	virtual StatusCode AddMeta(IDatasetNode* target, const char* attribute, IMetadataNode** result = 0) = 0;
	virtual StatusCode ModifyMeta(IMetadataNode* target, const char* value) = 0;
	virtual StatusCode DeleteMeta(IMetadataNode* target) = 0;
	virtual StatusCode ModifyAccess(IAccessNode* target, const char* permission) = 0;
	virtual StatusCode SetAccess(IDatasetNode* target, IUserNode* owner, const char* permission) = 0;

};

class ICollectionOperator : public ITransferOperator
{
public:
	virtual ~ICollectionOperator() {};
	virtual StatusCode Copy(ICollectionNode* target, IDatasetNode* data, IDatasetNode** result = 0) = 0;
	virtual StatusCode Copy(ICollectionNode* target, ICollectionNode* data, ICollectionNode** result = 0) = 0;
	virtual StatusCode Move(ICollectionNode* target, IDatasetNode* data, IDatasetNode** result = 0) = 0;
	virtual StatusCode Move(ICollectionNode* target, ICollectionNode* data, ICollectionNode** result = 0) = 0;
	virtual StatusCode Delete(ICollectionNode* target) = 0;
	virtual StatusCode Replicate(ICollectionNode* target) = 0;
	virtual StatusCode Rename(ICollectionNode* target, const char* name) = 0;
	virtual StatusCode SetComment(ICollectionNode* target, const char* comment) = 0;
	virtual StatusCode Create(ICollectionNode* target, const char* name, ICollectionNode** result = 0) = 0;	//optional param sets result to the Node of the new collection

	virtual StatusCode AddMeta(ICollectionNode* target, const char* attribute, IMetadataNode** result = 0) = 0;
	virtual StatusCode ModifyMeta(IMetadataNode* target, const char* value) = 0;
	virtual StatusCode DeleteMeta(IMetadataNode* target) = 0;

	virtual StatusCode ModifyAccess(IAccessNode* target, const char* permission, bool recursive) = 0;
	virtual StatusCode SetAccess(ICollectionNode* target, IUserNode* owner, const char* permission, bool recursive) = 0;

};

class IResourceOperator : public IOperator
{
public:
	virtual ~IResourceOperator() {};
};

class IDomainOperator : public IOperator
{
public:
	virtual ~IDomainOperator() {};
};

class IZoneOperator : public IOperator
{
public:
	virtual ~IZoneOperator() {};
	virtual IZoneNode* GetZones() {return NULL;};
};

class ICatalogOperator : public IOperator
{
public:
	virtual ~ICatalogOperator() {};
	virtual ICatalogNode* GetZones() {return NULL;};
};

class IQueryOperator : public IOperator
{
public:
	virtual ~IQueryOperator() {};
	//query operations
	virtual StatusCode Create(IQueryNode** query) = 0;
	virtual StatusCode Delete(IQueryNode* query) = 0;
	virtual StatusCode Nest(IQueryNode* parent, IQueryNode* child) = 0;
	virtual StatusCode Attach(IQueryNode* query, IMetadataNode* expression) = 0;
	virtual StatusCode Bind(IQueryNode* query, INode* node) = 0;

	//expression operations
	virtual StatusCode Add(IMetadataNode** expression) = 0;
	virtual StatusCode Remove(IMetadataNode* expression) = 0;
	virtual StatusCode SetValue(IMetadataNode* expression, const char* value) = 0;
	virtual StatusCode SetAttribute(IMetadataNode* expression, const char* attribute) = 0;
	virtual StatusCode SetOperation(IMetadataNode* expression, int operation) = 0;
	virtual StatusCode And(IMetadataNode* left, IMetadataNode* right, IMetadataNode** result) = 0;
	virtual StatusCode Or(IMetadataNode* left, IMetadataNode* right, IMetadataNode** result) = 0;
};

}	//end namespace
#endif __WINI_OPERATOR_INTERFACE_H__