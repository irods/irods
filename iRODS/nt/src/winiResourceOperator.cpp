#include "winiResourceOperator.h"

namespace WINI
{
ResourceOperatorImpl::ResourceOperatorImpl(ISession* session)
{
	m_session = session;
	m_conn = (rcComm_t*)session->GetConn();
	m_resources = NULL;
}

ResourceOperatorImpl::~ResourceOperatorImpl()
{
	delete m_resources;
}

StatusCode ResourceOperatorImpl::GetChildren(ResourceNodeImpl* target, unsigned int mask)
{
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	target->Clear();

	StatusCode status;
	
	unsigned int success = WINI_ALL;

	target->SetOpen(success);
	return status;
}
} //end namespace


