#pragma once
#include "webdisp_explorer.h"
#include "afxwin.h"


// CDialogWebDisp dialog

class CDialogWebDisp : public CDialog
{
	DECLARE_DYNAMIC(CDialogWebDisp)

public:
	CDialogWebDisp(CString & HtmlStr, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogWebDisp();

// Dialog Data
	enum { IDD = IDD_DIALOG_WEBDISPLAY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	CWebdisp_explorer m_webdispExplorer;

	int m_topOffset, m_leftOffset, m_rightOffset, m_bottomOffset;
	CButton m_okButton;

	bool m_pageLoaded;

	CString m_htmlStr;
	bool m_dlgInitialized;

	void LoadHtmlString();
public:
	DECLARE_EVENTSINK_MAP()
	void DocumentCompleteWebdispExplorer(LPDISPATCH pDisp, VARIANT* URL);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};
