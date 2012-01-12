// InquisitorDoc.h : interface of the CInquisitorDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_INQUISITORDOC_H__736A9F8C_911C_46B5_BBF2_446BF4CA7230__INCLUDED_)
#define AFX_INQUISITORDOC_H__736A9F8C_911C_46B5_BBF2_446BF4CA7230__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//#include "winiSession.h"
//#include "winiObjects.h"
//#include "WorkerThread.h"
#include "Login.h"
#include "irodsUserEnv.h"
#include <stack>
#include "irodsWinDataobj.h"
#include "irodsWinCollection.h"
#include "MetaSearchDlg.h"
//#include <shldisp.h>

class CInquisitorDoc : public COleDocument
{
protected: // create from serialization only
	CInquisitorDoc();
	DECLARE_DYNCREATE(CInquisitorDoc)

public:
	static const int RLIST_MODE_COLL = 1;
	static const int RLIST_MODE_QUERY = 2;
	CString local_temp_dir;
	int rlist_disp_mode;
	rcComm_t *conn;
	CString  passwd;
	CString  user_collection_home;
	CString  default_resc_from_login;
	int      profile_id_from_login;

	irodsWinCollection *cur_selelected_coll_in_tree;
	
	CArray<irodsWinCollection, irodsWinCollection> colls_in_list;
	CArray<irodsWinDataobj, irodsWinDataobj> dataobjs_in_list;

	CString cur_selected_node_zone;
	CStringArray resc_names;

	// for preferences
	bool upload_always_override;
	bool paste_always_override;
	bool delete_obj_permanently;

	void rlist_mode_coll_delete_one_coll(CString & collFullPath);
	void rlist_mode_coll_delete_one_dataobj(CString & dataName, CString & ParCollPath, int replNum);
	void rlist_mode_coll_change_one_collname(CString & oldCollFullPath, CString & newName);
	void rlist_mode_coll_change_one_dataname(CString & parFullPath, CString & oldName, CString newName);
	bool is_only_data_copy(CString & dataName);

	bool dragdrop_upload_local_data(COleDataObject* pDataObject, CString & parentColl);

	// internal buffer for edit::copy & edit::paste menu
	CArray<irodsWinCollection, irodsWinCollection> colls_in_copy_buffer;
	CArray<irodsWinDataobj, irodsWinDataobj> dataobjs_in_copy_buffer;

	// intrenal buffer for drag and drop from list to tree --> decided to use copy buffer
	//CArray<irodsWinCollection, irodsWinCollection> colls_in_dragdrop_buffer;
	//CArray<irodsWinDataobj, irodsWinDataobj> dataobjs_in_dragdrop_buffer;
	
	//void OnFailedConnection();
	
	//WINI::StatusCode NukeDir(const char* szpath);
	//void Copy(WINI::INode* node, int type, bool cut, char* name, char* path, int replication_index);
	//void Copy(WINI::INode* target, WINI::INode* source, bool cut);
	char* GetTempPath();
	void SetSynchronizationFlags(bool purge, bool primary_only);
	void Replicate(WINI::INode* node);
	//WINI::StatusCode ChangePassword(const char* password);
	//bool isConnected();
	//WINI::StatusCode DragOutDownload(LPSTGMEDIUM lpStgMedium);
	void SetAccess(CString & irodsPath, int obj_type);
	bool Disconnect();
	//WINI::IStringNode* GetAccessConstraint();
	//WINI::StatusCode ModifyAccess(WINI::IAccessNode* target, const char* permission, bool recursive = false);
	//BOOL OnCompletedConnection();
	//WINI::INode* CInquisitorDoc::GetDomains();
	//WINI::StatusCode SetMetadataValue(const char* attribute, const char* value);
	//WINI::StatusCode SetMetadataValue(WINI::IMetadataNode* node, const char* value);
	//WINI::INode* GetRoot();
	//	WINI::INode* GetLocalRoot();
	//WINI::INode* GetHome();
	//WINI::INode* GetHomeZone();
	BOOL MyNewConnection();
	//WINI::INode* GetMCAT();
	//void SetComment(WINI::INode* node, char* comment);
	//WINI::StatusCode OpenTree(WINI::INode* node, int levels, bool clear, unsigned int mask = WINI_ALL);
	//WINI::ISetNode* GetResources(WINI::IZoneNode* node);
	//WINI::StatusCode AddNewMetadata(const char* attribute, const char* value);
	//WINI::StatusCode SetDefaultResource(WINI::INode* resource);
	//WINI::INode* GetDefaultResource() { return m_defaultResource;};
	void OnReturn();
	//void Rename(WINI::INode* node, char* name);
	//void Delete(WINI::INode* node);
	void SelectLocalFolder(CString & locaDir);
	//void Download(WINI::INode* node, const char* local_path, bool Execute);
	//void Download(WINI::INode* node, bool Execute);
	//void Upload(WINI::INode* node, const char* local_path);
	void Execute(const char* local_path);
	//void SetCurrentNode(WINI::INode* node);
	//WINI::INode* GetCurrentNode();

	//void CreateNode(WINI::INode* parent, const char* name);

	void OnBack();
	void OnForward();



//WINI::IZoneNode* GetCurrentZone();

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInquisitorDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

public:
	virtual ~CInquisitorDoc();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CInquisitorDoc)
	//afx_msg void OnQuery();
	//afx_msg void OnUpdateBack(CCmdUI* pCmdUI);
	//afx_msg void OnUpdateForward(CCmdUI* pCmdUI);
	//afx_msg void OnUpdateFileNew(CCmdUI* pCmdUI);
	//afx_msg void OnRefresh();
	//afx_msg void OnUpdateRefresh(CCmdUI* pCmdUI);
	//afx_msg void OnGetpath();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	//WINI::SessionImpl MyImpl;
	_irodsUserEnv_ m_irodsUserEnv;

private:

	//CWinThread* m_pCThread;
	//HANDLE m_hCThread;
	
	//WINI::SessionImpl* MyImpl2;
	//CLogin m_Login;
	//WINI::INode* m_current_node;
	//WINI::INode* m_defaultResource;
	
	bool m_bIsDemo;
	//WINI::INode* m_History[10];
	int m_HistoryLast, m_HistoryFirst, m_HistoryCurrent;
	unsigned int m_synchFlag;

	//WINI::IZoneNode* m_zone;

	bool m_eventConnecting;

public:
	void update_rlist(void);
	void insert_one_coll_in_rlist(irodsWinCollection & newColl);
	void insert_one_dataobj_in_rlist(irodsWinDataobj & newDataObj);
	void refresh_disp();
	int  upload_a_localfolder_into_coll(CString & parCollFullPath, irodsWinCollection & newColl);
	int  upload_localfiles_into_coll(CString & parCollFullPath, CArray<irodsWinDataobj, irodsWinDataobj> & newDataobjs);
	bool is_irods_trash_area(CString & collFullPath);
	int check_coll_with_buffered_objs(CString & mycollPath);
	void paste_buffered_irods_objs(CString & pasteToCollFullPath);
	void move_buffered_irods_objs(CString & destinationColl);
	void rlist_dbclick_coll_action(CString & collFullPath);
	void disp_err_msgbox(CString & msgHead, int errCode, CWnd *pWin = NULL);
	void statusbar_msg(CString & msg);

	// for relation with buffered data
	enum {
		NO_RELATION_WITH_BUFFERED_DATA = 0,
		HAS_A_SAME_COLL_IN_BUFFERED_DATA,
		IS_A_DESCENDANT_OF_A_COLL_IN_BUFFERED_DATA
	};

	afx_msg void OnUpdateQuery(CCmdUI *pCmdUI);
	afx_msg void OnUserInfo();
	//afx_msg void OnPreferenceAlwaysoverridewhenuploadingfile();
	afx_msg void OnIrodsPreferences();
	afx_msg void OnTreeUp();
	void disp_file_in_local_machine(CString & parPath, CString & fname, CWnd *pWin = NULL);
	afx_msg void OnUseraccountChangepassword();
	afx_msg void OnRuleSubmitanirodsrule();
	afx_msg void OnRuleCheckstatusofpendingrule();
	afx_msg void OnRuleDeletependingrule();
	//afx_msg void OnSearchMetadatasearch();
	afx_msg void OnEditSearch();
};

#if 0
class CharlieSource : public COleDataSource, public IAsyncOperation
{
public:
	~CharlieSource();
	//BOOL OnRenderData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium);
	CInquisitorDoc* m_doc;

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//IAsyncOperation
	HRESULT STDMETHODCALLTYPE SetAsyncMode(BOOL fDoOpAsync);
	HRESULT STDMETHODCALLTYPE GetAsyncMode(BOOL *pfIsOpAsync);
	HRESULT STDMETHODCALLTYPE StartOperation(IBindCtx *pbcReserved);
	HRESULT STDMETHODCALLTYPE InOperation(BOOL *pfInAsyncOp);
	HRESULT STDMETHODCALLTYPE EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects);


private:
	BOOL RenderFileData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium);
	BOOL RenderInqData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium);
	//WINI::StatusCode DownloadForCopy(LPSTGMEDIUM lpStgMedium);
	//std::vector<WINI::INode*> m_NodeDragList;
	int mysentinel;

	BOOL m_AsyncModeOn;
	BOOL m_AsyncTransferInProgress;
	ULONG m_cRef;
};
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INQUISITORDOC_H__736A9F8C_911C_46B5_BBF2_446BF4CA7230__INCLUDED_)
