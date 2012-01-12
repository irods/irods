#ifndef __WINI_OBJECT_CATALOG_OPERATOR_H__
#define __WINI_OBJECT_CATALOG_OPERATOR_H__

#include "winiGlobals.h"
#include "winiObjects.h"
#include "winiOperators.h"
#include "winiCatalog.h"
//#include "rodsGenQuery.h"

namespace WINI
{

class CatalogOperatorImpl : public ICatalogOperator
{
public:
	StatusCode Bind(INode* node) { return WINI_ERROR_TYPE_NOT_SUPPORTED;};
	int GetType() { return WINI_OP_Catalog; };

	StatusCode GetChildren(CatalogNodeImpl* target, unsigned int mask = WINI_ALL);
	CatalogOperatorImpl(ISession* session);
	 ~CatalogOperatorImpl();

private:
	CatalogOperatorImpl();
	CatalogOperatorImpl(const CatalogOperatorImpl& source);
	CatalogOperatorImpl& operator =(const CatalogOperatorImpl& source);

	StatusCode GetZones(CatalogNodeImpl* target);

	void ClearCatalogScratch();
	//mdasC_sql_result_struct m_result;
	//char m_qval[MAX_DCS_NUM][MAX_TOKEN];
	//nt m_selval[MAX_DCS_NUM];
	//srbConn* m_conn;
	ISession* m_session;
	ICatalogNode* m_Catalog;
};
}//end namespace
#endif __WINI_OBJECT_CATALOG_OPERATOR_H__









