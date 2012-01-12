#ifndef __WINI_OBJECT_ZONE_OPERATOR_H__
#define __WINI_OBJECT_ZONE_OPERATOR_H__
#include "winiGlobals.h"
#include "winiObjects.h"
#include "winiOperators.h"
#include "winiZone.h"

namespace WINI
{

class ZoneOperatorImpl : public IZoneOperator
{
public:
	StatusCode Download(INode* target, const char* local_path) { return -1;};
	StatusCode Upload(INode* target, const char* local_path, unsigned int overwrite, INode** result) { return -1;};
	int GetProgress(char** name) { return -1;};
	StatusCode Bind(INode* node) { return WINI_ERROR_TYPE_NOT_SUPPORTED;};
	int GetType() { return WINI_OP_ZONE; };

	StatusCode GetChildren(ZoneNodeImpl* target, int clean, unsigned int mask = WINI_ALL);
	ZoneOperatorImpl(ISession* session);
	 ~ZoneOperatorImpl();


private:
	ZoneOperatorImpl();
	ZoneOperatorImpl(const ZoneOperatorImpl& source);
	ZoneOperatorImpl& operator =(const ZoneOperatorImpl& source);

	StatusCode GetResources(ZoneNodeImpl* target);
	StatusCode GetUsers(ZoneNodeImpl* target);
	StatusCode GetCollections(ZoneNodeImpl* target);
	void InitQuery();

	ISession* m_session;
	IZoneNode* m_zone;
};
}//end namespace
#endif __WINI_OBJECT_ZONE_OPERATOR_H__

