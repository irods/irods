// SubmitIRodsRuleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "SubmitIRodsRuleDlg.h"
#include "irodsWinUtils.h"
#include "MainFrm.h"
#include "InquisitorDoc.h"
#include "DispTextDlg.h"
#include <stdio.h>


// CSubmitIRodsRuleDlg dialog

IMPLEMENT_DYNAMIC(CSubmitIRodsRuleDlg, CDialog)

CSubmitIRodsRuleDlg::CSubmitIRodsRuleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSubmitIRodsRuleDlg::IDD, pParent)
{

}

CSubmitIRodsRuleDlg::~CSubmitIRodsRuleDlg()
{
}

void CSubmitIRodsRuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_RULE, m_editRule);
	DDX_Control(pDX, IDC_EDIT_INPUT_PARAM, m_editInputParam);
	DDX_Control(pDX, IDC_EDIT_OUTPUT_PARAM, m_editOutputParam);
}


BEGIN_MESSAGE_MAP(CSubmitIRodsRuleDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CSubmitIRodsRuleDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CSubmitIRodsRuleDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CSubmitIRodsRuleDlg message handlers

void CSubmitIRodsRuleDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CString irods_rule, input_param, output_param, result_str;
	m_editRule.GetWindowText(irods_rule);
	irods_rule.Trim();
	if(irods_rule.GetLength() <= 0)
	{
		AfxMessageBox("Please enter a rule.");
		return;
	}
	m_editInputParam.GetWindowText(input_param);
	input_param.Trim();
	m_editOutputParam.GetWindowText(output_param);
	output_param.Trim();
	if(output_param.GetLength() <= 0)
	{
		output_param = "null";
	}

	// check rule syntax
	int cnt = 0;
	int t = 0;
	while((t = irods_rule.Find('|', t)) >= 0)
	{
		t = t + 1;
		cnt++;
	}
	if(cnt != 3)
	{
		AfxMessageBox("The rule has syntax error!");
		return;
	}

	// a simple check for the syntax of input param
	/*
	if((input_param.GetLength()>0) && (input_param.Find("=") < 0))
	{
		AfxMessageBox("There is a syntax error in the 'Input Parameters'");
		return;
	}
	*/

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	frame->BeginWaitCursor();
	t = irodsWinExecRule(pDoc->conn, irods_rule, input_param, output_param, result_str);
	frame->EndWaitCursor();
	if(t < 0)
	{
		CString msgHead = CString("Execute Rule error: ");
		pDoc->disp_err_msgbox(msgHead, t);
		return;
	}

	if(result_str.GetLength() > 0)
	{
		CString dlgTitle = "Results of Excuting the Rule";
		result_str.Replace("\n", "\r\n");
		CDispTextDlg tdlg(dlgTitle, result_str, this);
		tdlg.DoModal();
	}
	else
	{
		AfxMessageBox("The rule was submitted to iRODS system.");
	}

	//OnOK();
}

void CSubmitIRodsRuleDlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	CFileDialog file_dlg(TRUE, NULL, NULL,  OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST, NULL, NULL);

	file_dlg.m_ofn.lpstrTitle = "Select a Rule File";
	if(file_dlg.DoModal() != IDOK)
		return;

	CString sel_filename = file_dlg.GetPathName();

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	CString irods_rule, irods_in_param, irods_out_param;
	irods_rule = "";
	irods_in_param = "";
	irods_out_param = "";

	CString msg;
	char buff[2048];
	FILE *f = iRODSNt_fopen((char *)LPCTSTR(sel_filename), "r");
	if(f == NULL)
	{
		msg = CString("Failed to open file '") + sel_filename + CString("' for read!");
		AfxMessageBox(msg);
		return;
	}

	if(fgets(buff, 2048, f) == NULL)
	{
		msg = CString("Reading '") + sel_filename + CString("' failed!");
		fclose(f);
		AfxMessageBox(msg);
		return;
	}
	irods_rule = buff;

	buff[0] ='\0';
	if(fgets(buff, 2048, f) != NULL)
	{
		irods_in_param = buff;
		buff[0] = '\0';
		if(fgets(buff, 2048, f) != NULL)
			irods_out_param = buff;
	}
	fclose(f);

	m_editRule.SetWindowText(irods_rule);
	m_editInputParam.SetWindowText(irods_in_param);
	m_editOutputParam.SetWindowText(irods_out_param);
}
