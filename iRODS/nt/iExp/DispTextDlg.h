#pragma once
#include "afxwin.h"


// CDispTextDlg dialog

class CDispTextDlg : public CDialog
{
	DECLARE_DYNAMIC(CDispTextDlg)

public:
	CDispTextDlg(CString & DlgTitle, CString & DispText, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDispTextDlg();

private:
	CString m_dlgTitle;
	CString m_dispText;

	// for resizing the dialog
	bool m_dlgInitialized;
	int  m_controlOffset;
	int  ok_w, ok_h;   // size of OK button        
// Dialog Data
	enum { IDD = IDD_DIALOG_DISPTEXT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
private:
	CEdit m_editDispText;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
private:
	CButton m_buttonOk;
};
