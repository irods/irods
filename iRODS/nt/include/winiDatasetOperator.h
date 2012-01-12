#ifndef __WINI_OBJECT_DATASET_OPERATOR_H__
#define __WINI_OBJECT_DATASET_OPERATOR_H__

#include "winiGlobals.h"
#include "winiObjects.h"
#include "winiDataset.h"
#include "winiCollection.h"

#include "winiCollectionOperator.h"
#include "winiMetadata.h"
#include "winiAccess.h"
#include <dirent.h>
#include "winiOperators.h"
#include "parseCommandLine.h"



namespace WINI
{
class DatasetOperatorImpl : public IDatasetOperator
{
public:
	DatasetOperatorImpl(ISession* session);
	 ~DatasetOperatorImpl();
	StatusCode Download(INode* target, const char* local_path);
	StatusCode Upload(INode* target, const char* local_path, unsigned int overwrite, INode** result = NULL);
	StatusCode GetReplicant(ICollectionNode* target, const char* name);
	int GetProgress(char** name);
	StatusCode Delete(IDatasetNode* target);
	StatusCode Replicate(IDatasetNode* target);
	StatusCode Export(IDatasetNode* source, const char* local_path);
	StatusCode Import(IDatasetNode* target, const char* local_path, IDatasetNode** result);
	StatusCode Rename(IDatasetNode* target, const char* name);
	StatusCode SetComment(IDatasetNode* target, const char* comment);
	StatusCode SetAccess(IDatasetNode* target, IUserNode* owner, const char* permission);
	StatusCode AddMeta(IDatasetNode* target, const char* attribute, IMetadataNode** result);
	StatusCode ModifyMeta(IMetadataNode* target, const char* value);
	StatusCode DeleteMeta(IMetadataNode* target);
	StatusCode ModifyAccess(IAccessNode* target, const char* permission);
	int GetType() { return WINI_OP_DATASET; };
	StatusCode Bind(INode* node);

	INode* GetBinding() {return m_binding;};

	//impl
	StatusCode GetChildren(DatasetNodeImpl* target, unsigned int mask = WINI_ALL);
	StatusCode GetMetadata(DatasetNodeImpl* target);
	StatusCode GetAccess(DatasetNodeImpl* target);
	void ClearMCATScratch();

private:
	DatasetOperatorImpl();
	StatusCode Wash(CollectionNodeImpl* collection, const char* dataset_name);
	DatasetOperatorImpl(const DatasetOperatorImpl& source);
	DatasetOperatorImpl& operator =(const DatasetOperatorImpl& source);

	StatusCode Fill(CollectionNodeImpl* parent, INode** child, const char* name);
	ISession* m_session;
	rcComm_t* m_conn;
	char* m_resource;



	int m_bytes_received;
	int m_bytes_expected;
	INode* m_binding;
	bool m_bIsDownloading;
};
}//end namespace
#endif



