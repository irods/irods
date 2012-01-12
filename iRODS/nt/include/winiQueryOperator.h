#ifndef __WINI_OBJECT_QUERY_OPERATOR2_H__
#define __WINI_OBJECT_QUERY_OPERATOR2_H__

#include "winiGlobals.h"
#include "winiOperators.h"

#include "winiCollection.h"
#include "winiMetadata.h"
#include "winiQuery.h"

namespace WINI
{
class QueryOperatorImpl : public IQueryOperator
{
public:
	QueryOperatorImpl(ISession* session);
	 ~QueryOperatorImpl();

	int GetProgress(char** name) { return 0;};
	//interface
	StatusCode Download(INode* target, const char* local_path) { return -1;};
	StatusCode Upload(INode* target, const char* local_path, unsigned int overwrite, INode** result) { return -1;};
	//query operations
	StatusCode Create(IQueryNode** query);
	StatusCode Delete(IQueryNode* query);
	StatusCode Nest(IQueryNode* parent, IQueryNode* child);
	StatusCode Attach(IQueryNode* query, IMetadataNode* expression);
	StatusCode Bind(IQueryNode* query, INode* node);	//perform the query on a collection, or a resource, etc.
	StatusCode Bind(INode* node) { return WINI_ERROR_TYPE_NOT_SUPPORTED;};
	//expression operations
	int GetType() { return WINI_OP_QUERY; };
	StatusCode Add(IMetadataNode** expression);
	StatusCode Remove(IMetadataNode* expression);
	StatusCode SetValue(IMetadataNode* expression, const char* value);
	StatusCode SetAttribute(IMetadataNode* expression, const char* attribute);
	StatusCode SetOperation(IMetadataNode* expression, int operation);
	StatusCode And(IMetadataNode* left, IMetadataNode* right, IMetadataNode** result);
	StatusCode Or(IMetadataNode* left, IMetadataNode* right, IMetadataNode** result);
	//impl
	StatusCode GetChildren(QueryNodeImpl* target, unsigned int mask = WINI_ALL);
private:
	QueryOperatorImpl();
	QueryOperatorImpl(const QueryOperatorImpl& source);
	QueryOperatorImpl& operator =(const QueryOperatorImpl& source);

	void ClearMCATScratch();
	StatusCode LoadCollectionQuery(QueryNodeImpl* target);
	StatusCode LoadDatasetQuery(QueryNodeImpl* target);
	StatusCode LoadCollectionQuery2(IMetadataNode* ptr, int& count);
	StatusCode Fetch(QueryNodeImpl* target);
	StatusCode FetchCollections(QueryNodeImpl* target);
	StatusCode FetchDatasets(QueryNodeImpl* target);
	StatusCode GetFirstChildCollection(QueryNodeImpl* target);
	StatusCode GetChildCollections(QueryNodeImpl* target);
	StatusCode GetChildDatasets(QueryNodeImpl* target);
	StatusCode LoadDatasetQuery2(IMetadataNode* ptr, int& count);
	void yet_another_collection_load_function(int count, char* operation);
	void yet_another_dataset_load_function(int count, char* operation);


	//srbConn* m_conn;
	ISession* m_session;
	INode* m_binding;
	std::vector<INode*>* m_nodes;
};
}//end namespace
#endif

