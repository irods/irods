// RuleStatusDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "RuleStatusDlg.h"
#include "irodsWinUtils.h"
#include "MainFrm.h"
#include "InquisitorDoc.h"


// CRuleStatusDlg dialog

IMPLEMENT_DYNAMIC(CRuleStatusDlg, CDialog)

CRuleStatusDlg::CRuleStatusDlg(CString & dlgTitle, CWnd* pParent /*=NULL*/)
	: CDialog(CRuleStatusDlg::IDD, pParent)
{
	m_dlgTitle = dlgTitle;
}

CRuleStatusDlg::~CRuleStatusDlg()
{
}

void CRuleStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PENDING_RULES, m_comboPendingRules);
	DDX_Control(pDX, IDC_EDIT_RULE_NAME, m_editRuleName);
	DDX_Control(pDX, IDC_EDIT_RULE_STATUS, m_editRuleStatus);
	DDX_Control(pDX, IDC_EDIT_USER_NAME, m_editUserName);
}


BEGIN_MESSAGE_MAP(CRuleStatusDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CRuleStatusDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CRuleStatusDlg::OnBnClickedButton1)
	ON_CBN_SELCHANGE(IDC_COMBO_PENDING_RULES, &CRuleStatusDlg::OnCbnSelchangeComboPendingRules)
//	ON_CBN_EDITCHANGE(IDC_COMBO_PENDING_RULES, &CRuleStatusDlg::OnCbnEditchangeComboPendingRules)
ON_BN_CLICKED(IDC_BUTTON2, &CRuleStatusDlg::OnBnClickedButton2)
//ON_COMMAND(ID_RULE_DELETEPENDINGRULE, &CRuleStatusDlg::OnRuleDeletependingrule)
END_MESSAGE_MAP()


// CRuleStatusDlg message handlers

void CRuleStatusDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//OnOK();
}

BOOL CRuleStatusDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();
	m_editUserName.SetWindowText(pDoc->conn->clientUser.userName);

	CRect rcDropped;
	CRect rcCombo;
	int nItemHeight = m_comboPendingRules.GetItemHeight(-1);
	m_comboPendingRules.GetClientRect(&rcCombo);
	m_comboPendingRules.GetDroppedControlRect(&rcDropped);
	m_comboPendingRules.GetParent()->ScreenToClient(&rcDropped);
	rcDropped.bottom = rcDropped.top + rcCombo.Height() + nItemHeight*3;
	m_comboPendingRules.MoveWindow(&rcDropped);

	this->SetWindowText(m_dlgTitle);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRuleStatusDlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	CString user_name, rule_name;
	m_editRuleName.GetWindowText(rule_name);
	m_editUserName.GetWindowText(user_name);

	rule_name.Trim();
	user_name.Trim();

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	if(user_name.GetLength() <= 0)
		user_name = pDoc->conn->clientUser.userName;

	frame->BeginWaitCursor();
	int t = irodsWinQueryPendingRules(pDoc->conn, user_name, rule_name, m_ruleIds, m_ruleStatusDescs); 
	frame->EndWaitCursor();

	if(t < 0)
	{
		CString msgHead = CString("Query Rule Status error: ");
		pDoc->disp_err_msgbox(msgHead, t);
		return;
	}

	if(m_ruleIds.GetSize() <= 0)
	{
		AfxMessageBox("No pending rule is found.");
		return;
	}

	m_comboPendingRules.Clear();
	m_editRuleStatus.SetWindowText("");

	for(int i=0;i<m_ruleIds.GetSize();i++)
	{
		m_comboPendingRules.InsertString(i, m_ruleIds[i]);
	}
	m_comboPendingRules.SelectString(0, m_ruleIds[0]);
	m_editRuleStatus.SetWindowText(m_ruleStatusDescs[0]);
}

void CRuleStatusDlg::OnCbnSelchangeComboPendingRules()
{
	// TODO: Add your control notification handler code here
	CString sel_rule_id;
	m_comboPendingRules.GetWindowText(sel_rule_id);

	for(int i=0;i<m_ruleIds.GetSize();i++)
	{
		if(sel_rule_id == m_ruleIds[i])
		{
			m_editRuleStatus.SetWindowText(m_ruleStatusDescs[i]);
			return;
		}
	}
}
void CRuleStatusDlg::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	CString sel_rule_id;
	m_comboPendingRules.GetWindowText(sel_rule_id);

	CString msg = CString("Are you sure you want to delete rule '") + sel_rule_id + CString("'?");
	if(AfxMessageBox(msg, MB_YESNO) != IDYES)
	{
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	frame->BeginWaitCursor();
	int t = irodsWinDeleteRule(pDoc->conn, sel_rule_id);
	frame->EndWaitCursor();

	if(t < 0)
	{
		CString msgHead = CString("Delete Rule error: ");
		pDoc->disp_err_msgbox(msgHead, t);
		return;
	}

	int i, j;
	j = -1;
	for(i=0;i<m_ruleIds.GetSize();i++)
	{
		if(sel_rule_id == m_ruleIds[i])
		{
			m_ruleIds.RemoveAt(i);
			m_ruleStatusDescs.RemoveAt(i);
			j = i;
			break;
		}
	}

	if(j < 0)
		return;
	m_comboPendingRules.DeleteString(j);
	if(m_ruleIds.GetSize() > 0)
	{
		m_comboPendingRules.SelectString(0, m_ruleIds[0]);
		m_editRuleStatus.SetWindowText(m_ruleStatusDescs[0]);
	}
	else
	{
		m_comboPendingRules.SetWindowText("");
		m_editRuleStatus.SetWindowText("");
	}
}