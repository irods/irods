#pragma once

#include "InquisitorDoc.h"
#include "afxwin.h"
// CPreferenceDlg dialog

class CPreferenceDlg : public CDialog
{
	DECLARE_DYNAMIC(CPreferenceDlg)
	CInquisitorDoc *pDoc;

public:
	CPreferenceDlg(CInquisitorDoc *doc, CWnd* pParent = NULL);   // standard constructor
	virtual ~CPreferenceDlg();

// Dialog Data
	enum { IDD = IDD_PREFERENCE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
private:
	CButton m_prefPasteOverride;
	CButton m_prefUploadOverride;
public:
	afx_msg void OnBnClickedButtonPrefCleanTrash();
};
