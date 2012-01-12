#pragma once
#include "afxwin.h"


// CSubmitIRodsRuleDlg dialog

class CSubmitIRodsRuleDlg : public CDialog
{
	DECLARE_DYNAMIC(CSubmitIRodsRuleDlg)

public:
	CSubmitIRodsRuleDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSubmitIRodsRuleDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_SUBMIT_RULE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
private:
	CEdit m_editRule;
	CEdit m_editInputParam;
	CEdit m_editOutputParam;
public:
	afx_msg void OnBnClickedButton1();
};
