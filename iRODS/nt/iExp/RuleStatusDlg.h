#pragma once
#include "afxwin.h"


// CRuleStatusDlg dialog

class CRuleStatusDlg : public CDialog
{
	DECLARE_DYNAMIC(CRuleStatusDlg)

public:
	CRuleStatusDlg(CString & dlgTitle, CWnd* pParent = NULL);   // standard constructor
	virtual ~CRuleStatusDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_RULE_STATUS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
private:
	CComboBox m_comboPendingRules;
	CEdit m_editRuleName;
	CEdit m_editRuleStatus;
	CEdit m_editUserName;

	CStringArray m_ruleIds;
	CStringArray m_ruleStatusDescs;
	CString m_dlgTitle;
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnCbnSelchangeComboPendingRules();
//	afx_msg void OnCbnEditchangeComboPendingRules();
	afx_msg void OnBnClickedButton2();
//	afx_msg void OnRuleDeletependingrule();
};
