#ifndef __WINI_OBJECT_RESOURCE_OPERATOR_H__
#define __WINI_OBJECT_RESOURCE_OPERATOR_H__

#include "winiGlobals.h"
#include "winiOperators.h"
#include "winiResource.h"
#include "rodsGenQuery.h"
#include "parseCommandLine.h"	//for rodsArguements_t
#include "rcConnect.h"
#include "miscUtil.h"

namespace WINI
{
class ResourceOperatorImpl : public IResourceOperator
{
public:

	StatusCode Download(INode* target, const char* local_path) { return -1;};
	StatusCode Upload(INode* target, const char* local_path, unsigned int overwrite, INode** result) { return -1;};

	int GetProgress(char** name) { return -1;};
	ResourceOperatorImpl(ISession* session);
	 ~ResourceOperatorImpl();
	INode* GetBinding() { return NULL;};
	StatusCode Bind(INode* node) { return WINI_ERROR_TYPE_NOT_SUPPORTED;};
	int GetType() { return WINI_OP_RESOURCE; };

	//impl
	StatusCode GetChildren(ResourceNodeImpl* target, unsigned int mask = WINI_ALL);
	StatusCode GetContainer(ResourceNodeImpl* target);
	StatusCode GetAllContainers();

private:
	ResourceOperatorImpl();
	ResourceOperatorImpl(const ResourceOperatorImpl& source);
	ResourceOperatorImpl& operator =(const ResourceOperatorImpl& source);

	ISession* m_session;
	rcComm_t* m_conn;

	IResourceNode* m_resources;
};
}//end namespace
#endif

