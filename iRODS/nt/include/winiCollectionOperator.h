#ifndef __WINI_OBJECT_COLLECTION_OPERATOR_H__
#define __WINI_OBJECT_COLLECTION_OPERATOR_H__
#include "winiGlobals.h"
#include "winiOperators.h"
#include "winiCollection.h"
#include "winiObjects.h"
#include "rodsGenQuery.h"
#include "parseCommandLine.h"	//for rodsArguements_t
#include "rcConnect.h"
#include "miscUtil.h"

namespace WINI
{
class CollectionOperatorImpl : public ICollectionOperator
{
public:
	CollectionOperatorImpl(ISession* session);
	~CollectionOperatorImpl();
	StatusCode Bind(INode* node);
	StatusCode Download(INode* target, const char* local_path);
	StatusCode Upload(INode* target, const char* local_path, unsigned int overwrite, INode** result);
	StatusCode Copy(ICollectionNode* target, IDatasetNode* data, IDatasetNode** result = NULL);
	StatusCode Copy(ICollectionNode* target, ICollectionNode* data, ICollectionNode** result = NULL);
	StatusCode Move(ICollectionNode* target, IDatasetNode* data, IDatasetNode** result = NULL);
	StatusCode Move(ICollectionNode* target, ICollectionNode* data, ICollectionNode** result = NULL);
	StatusCode Delete(ICollectionNode* target);
	StatusCode Replicate(ICollectionNode* target);
	StatusCode Replicate_Recursive_Impl(ICollectionNode* target);
	StatusCode Rename(ICollectionNode* target, const char* name);
	StatusCode SetComment(ICollectionNode* target, const char* comment);
	StatusCode Create(ICollectionNode* target, const char* name, ICollectionNode** result = NULL);
	StatusCode AddMeta(ICollectionNode* target, const char* attribute, IMetadataNode** result = NULL);
	StatusCode ModifyMeta(IMetadataNode* target, const char* value);
	StatusCode DeleteMeta(IMetadataNode* target);
	StatusCode ModifyAccess(IAccessNode* target, const char* permission, bool recursive);
	StatusCode SetAccess(ICollectionNode* target, IUserNode* owner, const char* permission, bool recursive);
	int GetType() { return WINI_OP_COLLECTION; };
	INode* GetBinding() { return m_binding;};
	int GetProgress(char** name);

	//impl
	StatusCode ModifyCollectionRecursively(ICollectionNode* target, IUserNode* owner, const char* permission);
	StatusCode GetChildren(CollectionNodeImpl* target, unsigned int mask = WINI_ALL);

private:
	//int UploadImpl(srbConn* bloadconn, char* local, char* collection, char* resource, bool isContainer);
	//int DownloadImpl(srbConn* bdown_conn, const char* local, const char* source, int copyNum, srb_long_t* bytes_received, srb_long_t* bytes_expected);
	//int getMain (srbConn *conn, int catType, int nArgv, srbPathName nameArray[], srbPathName targNameArray[], int flagval, int copyNum, char *ticketId, srb_long_t* bytes_received);
	void implModifyAccessSubFunc2(INode* node, const char* szDomain, const char* szUser, const char* new_permission);
	void implModifyAccessSubFunc1(INode* node, const char* szDomain, const char* szUser);
	void implModifyAccess(const char* permission, IAccessNode* target, bool recursive);
	CollectionOperatorImpl();
	CollectionOperatorImpl(const CollectionOperatorImpl& source);
	CollectionOperatorImpl& operator =(const CollectionOperatorImpl& source);
	StatusCode GetAccess(CollectionNodeImpl* target);
	StatusCode GetChildCollections(CollectionNodeImpl* target);
	StatusCode GetChildDatasets(CollectionNodeImpl* target);
 	StatusCode GetMetadata(CollectionNodeImpl* target);
	void SetAccessNodes(INode* target, IUserNode* owner, const char* permission, bool recursive, int retraction_type);
	StatusCode FillDataset(CollectionNodeImpl* parent, IDatasetNode** child, const char* name);

	int m_bytes_received;
	int m_bytes_expected;
	rcComm_t *m_conn;
	ISession* m_session;
	INode* m_binding;
	char m_downloading[1024];
	bool m_bIsDownloading;
};
}//end namespace
#endif

