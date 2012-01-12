#ifndef __WINI_OBJECT_SESSION_H__
#define __WINI_OBJECT_SESSION_H__
#include "winiGlobals.h"
#include "winiObjects.h"
#include "winiOperators.h"
#include "vector"
//#include "clConnectExtern.h"
#include "winiCollectionOperator.h"
#include "winiDatasetOperator.h"
#include "winiResourceOperator.h"
#include "winiQueryOperator.h"
//#include "winiDomainOperator.h"
//#include "winiContainerOperator.h"
#include "winiZoneOperator.h"

#include "miscUtil.h"


namespace WINI
{
class SessionImpl : public ISession
{
public:
	SessionImpl();
	 ~SessionImpl();

//	StatusCode Root();

	StatusCode Clone(ISession* target);

	StatusCode Init(const char* host, int port, const char* user, const char* zone, const char* password);
	IOperator* GetOperator(int type);	//returns operator associated with node;
										//important for nested queries, etc.
	IStringNode* GetAccessConstraints();
	StatusCode Connect();
	StatusCode Disconnect();
	bool isConnected();
	StatusCode ChangePassword(const char* new_password);
	StatusCode OpenTree(INode* parent, int levels, bool clear, unsigned int mask = WINI_ALL );
	StatusCode CloseTree(INode* parent);

	ISetNode* GetResource(IZoneNode* zone);
	ICollectionNode* GetRoot(IZoneNode* zone);
	ICollectionNode* GetLocalRoot();
	ICollectionNode* GetHome(IZoneNode* zone);

	IZoneNode* GetHomeZone();
	IZoneNode* GetCurrentZone(INode* target);
//	void SetCurrentZone(IZoneNode* zone);
	
	ICollectionNode* GetUserRootCol();
	ICollectionNode* GetUserHomeCol();
	IZoneNode* GetUserZone();

	ICatalogNode* GetCatalog();
	const char* GetUserHost();
	int GetUserPortValue() { return m_port;};
	const char* GetName() { return m_user;};
	void* GetConn() { return m_conn;};

	void* CloneConn();
	
	virtual ICollectionNode* GetUserHome();



private:
	void Clean();
	SessionImpl(const SessionImpl& source);
	SessionImpl& operator =(const SessionImpl& source);
	rcComm_t* Clone();
	IZoneNode* m_HomeZone;
	ICatalogNode* m_Catalog;

	StatusCode Replicate_Recursive_Impl(ICollectionNode* target);
	char* m_user;
	char* m_host;
	char* m_szZone;
	char* m_domain;
	ISetNode* m_domainNode;
	ISetNode* m_ResourceNode;
	char* m_auth;
	int m_port;
	char* m_password;
	char* m_resource;
	char* m_user_home;
	char* m_zone_string;

	bool m_bisConnected;
	bool m_bMaster;


	ICollectionNode* m_root;
	ICollectionNode* m_localRoot;
	ICollectionNode* m_home;

	CollectionOperatorImpl* m_CollectionOp;
	DatasetOperatorImpl* m_datasetOp;
	ResourceOperatorImpl* m_ResourceOp;
	QueryOperatorImpl* m_queryOp;

	
	ZoneOperatorImpl* m_ZoneOp;

	IResourceNode* m_resources;
	std::vector<IOperator*> m_allocations;
	std::vector<INode*> m_allocations2;
	IStringNode* m_constraint_node;
	IDomainNode* m_domains;
	IZoneNode* m_zones;

	IZoneNode* m_currentZone;

	rcComm_t *m_conn;

	void ClearCatalogScratch();

	//mdasC_sql_result_struct m_result;
//	char m_qval[MAX_DCS_NUM][MAX_TOKEN];
//	int m_selval[MAX_DCS_NUM];
};


}//end namespace
#endif