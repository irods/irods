// ChangePasswdDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "MainFrm.h"
#include "ChangePasswdDlg.h"
#include "InquisitorDoc.h"
#include "irodsWinCollection.h"
#include "irodsWinUtils.h"


// CChangePasswdDlg dialog

IMPLEMENT_DYNAMIC(CChangePasswdDlg, CDialog)

CChangePasswdDlg::CChangePasswdDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChangePasswdDlg::IDD, pParent)
{

}

CChangePasswdDlg::~CChangePasswdDlg()
{
}

void CChangePasswdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_CPW, m_editCPW);
	DDX_Control(pDX, IDC_EDIT_NPW, m_editNewPW);
	DDX_Control(pDX, IDC_EDIT_RE_NPW, m_editReNewPw);
}


BEGIN_MESSAGE_MAP(CChangePasswdDlg, CDialog)
	ON_BN_CLICKED(IDCANCEL, &CChangePasswdDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &CChangePasswdDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CChangePasswdDlg message handlers

void CChangePasswdDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void CChangePasswdDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CString cur_passwd, new_passwd, re_new_passwd;
	m_editCPW.GetWindowText(cur_passwd);
	if(cur_passwd.GetLength() <= 0)
	{
		AfxMessageBox("Please enter the current password!");
		return;
	}
	m_editNewPW.GetWindowText(new_passwd);
	if(new_passwd.GetLength() <= 0)
	{
		AfxMessageBox("Please enter the new password!");
		return;
	}
	m_editReNewPw.GetWindowText(re_new_passwd);
	if(re_new_passwd.GetLength() <= 0)
	{
		AfxMessageBox("Please re-enter the new password!");
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	if(cur_passwd != pDoc->passwd)
	{
		AfxMessageBox("The input of the current password is incorrect!");
		return;
	}

	if(new_passwd != re_new_passwd)
	{
		AfxMessageBox("The new password does not match the re-typed one!");
		return;
	}

	frame->BeginWaitCursor();
	CString msg;
	int t = irodsWinChangePasswd(pDoc->conn, cur_passwd, new_passwd);
	if(t < 0)
	{
		CString msgHead = CString("Change Password error: ");
		pDoc->disp_err_msgbox(msgHead, t);
	}
	else 
	{
		pDoc->passwd = new_passwd;
		msg = "Change Password succeeded.";
		frame->statusbar_msg(msg);
		AfxMessageBox(msg);
	}
	frame->EndWaitCursor();
	OnOK();
}
