#pragma once
#include "afxwin.h"


// CChangePasswdDlg dialog

class CChangePasswdDlg : public CDialog
{
	DECLARE_DYNAMIC(CChangePasswdDlg)

public:
	CChangePasswdDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CChangePasswdDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_CHANGE_PASSWD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
private:
	CEdit m_editCPW;
	CEdit m_editNewPW;
	CEdit m_editReNewPw;
};
