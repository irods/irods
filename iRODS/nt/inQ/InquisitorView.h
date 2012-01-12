// InquisitorView.h : interface of the CInquisitorView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_INQUISITORVIEW_H__AE8139AB_64D7_4F98_A058_8E2432C592C0__INCLUDED_)
#define AFX_INQUISITORVIEW_H__AE8139AB_64D7_4F98_A058_8E2432C592C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <vector>

class CInquisitorView : public CListView
{
protected: // create from serialization only
	CInquisitorView();
	DECLARE_DYNCREATE(CInquisitorView)

public:
	void OnDragLeave();
	CInquisitorDoc* GetDocument();
	void Refresh() { OnUpdate(NULL, NULL, NULL);};
	void OnUpdateOperation();
	virtual ~CInquisitorView();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	DROPEFFECT FileDragOver(COleDataObject* pDataObject, DWORD& dwKeyState, CPoint& point);
	DROPEFFECT InqDragOver(COleDataObject* pDataObject, DWORD& dwKeyState, CPoint& point);

	bool				m_bInitialized;
	std::vector<int>	m_nRedirector;
	BOOL				m_bDragging;
	int					m_iItemDrag;
	int					m_iItemDrop;
	COleDropTarget		m_dropTarget;
	void GetImage(const int& node_type, const char* sub_type, bool isOpen, int &image, int& selected_image);

public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInquisitorView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
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
	afx_msg void OnUpload();
	afx_msg void OnUploadFolder();
	afx_msg void OnNewCollection();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnOpen();
	afx_msg void OnOpenTree();
	afx_msg void OnReplicate();
	afx_msg void OnAccessControl();
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNewContainer();
	afx_msg void OnUpdateReplicate(CCmdUI* pCmdUI);
	afx_msg void OnMetadata();
	afx_msg void OnUpdateMetadata(CCmdUI* pCmdUI);
	afx_msg void OnUpdateQuery(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAccessCtrl(CCmdUI* pCmdUI);
	afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNewCollection(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDelete(CCmdUI* pCmdUI);
	afx_msg void OnRename();
	afx_msg void OnUpdateRename(CCmdUI* pCmdUI);
	afx_msg void OnComment();
	afx_msg void OnUpdateComment(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUpload(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUploadFolder(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDownload(CCmdUI* pCmdUI);
	afx_msg void OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNewMeta();
	afx_msg void OnUpdateNewMeta(CCmdUI* pCmdUI);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetpath();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in InquisitorView.cpp
inline CInquisitorDoc* CInquisitorView::GetDocument()
   { return (CInquisitorDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INQUISITORVIEW_H__AE8139AB_64D7_4F98_A058_8E2432C592C0__INCLUDED_)
