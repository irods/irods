#include "winiCatalogOperator.h"
#include "winiCatalog.h"
#include "winiZone.h"
#include "winiSession.h"
#include "winiCollection.h"
#include "parseCommandLine.h"

namespace WINI
{

CatalogOperatorImpl::CatalogOperatorImpl(ISession* session)
{
	m_session = session;
	//zero byte counts
}

CatalogOperatorImpl::~CatalogOperatorImpl()
{
}

StatusCode CatalogOperatorImpl::GetChildren(CatalogNodeImpl* target, unsigned int mask)
{
	if(NULL == target)
		return WINI_ERROR_INVALID_PARAMETER;

	target->Clear();

	StatusCode status;
	
	unsigned int success = WINI_ALL ^ WINI_ZONE;

	if(WINI_ZONE & mask)
	{
		status = GetZones(target);
		if(status.isOk())
		{
			success |= WINI_ZONE;
		}
		else
		{
			target->SetOpen(success);
			return status;
		}
	}

	target->SetOpen(success);
	return status;
}

StatusCode CatalogOperatorImpl::GetZones(CatalogNodeImpl* target)
{
	//for now there is only tempZone
	char* zone = "tempZone";

	ZoneNodeImpl* child = new ZoneNodeImpl(target, zone);

	target->AddChild(child);

	return WINI_OK;
}

} //end namespace
