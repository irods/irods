// InquisitorView.h : interface of the CInquisitorView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_INQUISITORVIEW_H__AE8139AB_64D7_4F98_A058_8E2432C592C0__INCLUDED_)
#define AFX_INQUISITORVIEW_H__AE8139AB_64D7_4F98_A058_8E2432C592C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <vector>
#include "irodsWinCollection.h"

// cleanning mode
#define RLIST_CLEAN_ALL 1
#define RLIST_CLEAN_SELECTED 2

class CInquisitorView : public CListView
{
protected: // create from serialization only
	CInquisitorView();
	DECLARE_DYNCREATE(CInquisitorView)

public:
	//void OnDragLeave();
	CInquisitorDoc* GetDocument();
	void Refresh() { OnUpdate(NULL, NULL, NULL);};
	void OnUpdateOperation();
	virtual ~CInquisitorView();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	//DROPEFFECT FileDragOver(COleDataObject* pDataObject, DWORD& dwKeyState, CPoint& point);
	//DROPEFFECT InqDragOver(COleDataObject* pDataObject, DWORD& dwKeyState, CPoint& point);
	DROPEFFECT InternalDataDragOver(COleDataObject* pDataObject, DWORD& dwKeyState, CPoint& point);

	int drag_over_item_t;

	bool				m_bInitialized;
	std::vector<int>	m_nRedirector;
	BOOL				m_bDragging;
	int					m_iItemDrag;
	int					m_iItemDrop;

	COleDropTarget		m_dropTarget;
	COleDataSource		m_COleDataSource;

	void GetImage(const int& node_type, const char* sub_type, bool isOpen, int &image, int& selected_image);
	void ListSelectedItemsToCopyBuffer();
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInquisitorView)
	public:
	//virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	//virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	//virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CInquisitorView)
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDelete();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDownload();
	//afx_msg void OnUpload();
	//afx_msg void OnUploadFolder();
	afx_msg void OnNewCollection();
	//afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//afx_msg void OnOpen();
	//afx_msg void OnOpenTree();
	afx_msg void OnReplicate();
	afx_msg void OnAccessControl();
	//afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnMetadata();
	//afx_msg void OnUpdateMetadata(CCmdUI* pCmdUI);
	//afx_msg void OnUpdateQuery(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAccessCtrl(CCmdUI* pCmdUI);
	//afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	//afx_msg void OnEditCut();
	//afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNewCollection(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDelete(CCmdUI* pCmdUI);
	afx_msg void OnRename();
	afx_msg void OnUpdateRename(CCmdUI* pCmdUI);
	//afx_msg void OnComment();
	//afx_msg void OnUpdateComment(CCmdUI* pCmdUI);
	//afx_msg void OnUpdateUpload(CCmdUI* pCmdUI);
	//afx_msg void OnUpdateUploadFolder(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDownload(CCmdUI* pCmdUI);
	//afx_msg void OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnNewMeta();
	//afx_msg void OnUpdateNewMeta(CCmdUI* pCmdUI);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnGetpath();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	void update_rlist(void);
	void rlist_remove_all(int CleanMode);
	void rlist_insert_one_coll(irodsWinCollection & newColl);
	void rlist_insert_one_dataobj(irodsWinDataobj & newDataObj);
	void set_menu_state_for_single_selected_folder(CCmdUI *pCmdUI);
	void set_menu_stat_for_single_selection(CCmdUI *pCmdUI);
	void set_menu_state_for_single_selected_file(CCmdUI *pCmdUI);
private:
	void rlist_remove_one_itemdata(int pos);
	void drag_drop_dehilite();
public:
	afx_msg void OnSelectAll();
	afx_msg void OnUpdateSelectAll(CCmdUI *pCmdUI);
//	afx_msg void OnEditCopypathtoclipboard();
	afx_msg void OnViewRefresh();
	afx_msg void OnUploadUploadafolder();
	afx_msg void OnUpdateUploadUploadafolder(CCmdUI *pCmdUI);
	afx_msg void OnUploadUploadfiles();
	afx_msg void OnUpdateUploadUploadfiles(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditCopypathtoclipboard(CCmdUI *pCmdUI);
	afx_msg void OnUpdateReplicate(CCmdUI *pCmdUI);
	afx_msg void OnEditMetadata();
	afx_msg void OnUpdateEditMetadata(CCmdUI *pCmdUI);
	afx_msg void OnEditCopypathtoclipboard();
	afx_msg void OnEditMetadataSystemmetadata();
	afx_msg void OnUpdateEditMetadataSystemmetadata(CCmdUI *pCmdUI);
	afx_msg void OnEditMetadataUsermetadata();
	afx_msg void OnUpdateEditMetadataUsermetadata(CCmdUI *pCmdUI);

private:
	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual void OnDragLeave();
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
public:
	//afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
};

#ifndef _DEBUG  // debug version in InquisitorView.cpp
inline CInquisitorDoc* CInquisitorView::GetDocument()
   { return (CInquisitorDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INQUISITORVIEW_H__AE8139AB_64D7_4F98_A058_8E2432C592C0__INCLUDED_)
