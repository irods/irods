// DispTextDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "DispTextDlg.h"


// CDispTextDlg dialog

IMPLEMENT_DYNAMIC(CDispTextDlg, CDialog)

CDispTextDlg::CDispTextDlg(CString & DlgTitle, CString & DispText, CWnd* pParent /*=NULL*/)
	: CDialog(CDispTextDlg::IDD, pParent)
{
	this->m_dlgTitle = DlgTitle;
	this->m_dispText = DispText;
	m_dlgInitialized = false;
}

CDispTextDlg::~CDispTextDlg()
{
}

void CDispTextDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_DLG_DISPTEXT, m_editDispText);
	DDX_Control(pDX, IDOK, m_buttonOk);
}


BEGIN_MESSAGE_MAP(CDispTextDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CDispTextDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDispTextDlg::OnBnClickedCancel)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CDispTextDlg message handlers

void CDispTextDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}

void CDispTextDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

BOOL CDispTextDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	this->m_editDispText.SetWindowText(this->m_dispText);
	this->SetWindowText(this->m_dlgTitle);

	RECT rect;
	m_editDispText.GetWindowRect(&rect);
	ScreenToClient(&rect);
	m_controlOffset = rect.top;
	
	m_buttonOk.GetWindowRect(&rect);
	ScreenToClient(&rect);
	ok_w = rect.right - rect.left;
	ok_h = rect.bottom - rect.top;

	m_dlgInitialized = true;
	

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDispTextDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if(!m_dlgInitialized)
		return;

	// start to resize controls
	RECT rect;
	this->GetClientRect(&rect);
	int dw = rect.right - rect.left;
	int dh = rect.bottom - rect.top;

	int x, y;
	x = (int)((dw-ok_w)/2);
	y = dh - ok_h - (int)(m_controlOffset/2);
	m_buttonOk.MoveWindow(x,y,ok_w,ok_h);

	int h = y - 2*m_controlOffset;
	int w = dw - 2*m_controlOffset;
	x = m_controlOffset;
	y = m_controlOffset;
	m_editDispText.MoveWindow(x,y,w,h);
}
