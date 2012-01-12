// PreferenceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "PreferenceDlg.h"
#include "irodsWinUtils.h"


// CPreferenceDlg dialog

IMPLEMENT_DYNAMIC(CPreferenceDlg, CDialog)

CPreferenceDlg::CPreferenceDlg(CInquisitorDoc *doc, CWnd* pParent /*=NULL*/)
	: CDialog(CPreferenceDlg::IDD, pParent)
{
	pDoc = doc;
}

CPreferenceDlg::~CPreferenceDlg()
{
}

void CPreferenceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PREF_COPY_CHECK_OVERRIDE, m_prefPasteOverride);
	DDX_Control(pDX, ID_PREF_UPLOAD_OVERRIDE, m_prefUploadOverride);
}


BEGIN_MESSAGE_MAP(CPreferenceDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CPreferenceDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_PREF_CLEAN_TRASH, &CPreferenceDlg::OnBnClickedButtonPrefCleanTrash)
END_MESSAGE_MAP()


// CPreferenceDlg message handlers

void CPreferenceDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	if(BST_CHECKED == m_prefPasteOverride.GetCheck())
		pDoc->paste_always_override = true;
	else
		pDoc->paste_always_override = false;
	if(BST_CHECKED == m_prefUploadOverride.GetCheck())
		pDoc->upload_always_override = true;
	else
		pDoc->upload_always_override = false;

	CButton *deleteRadio = (CButton *)this->GetDlgItem(IDC_PREF_DELETE_PERM);
	if(deleteRadio->GetCheck() == BST_CHECKED)
		pDoc->delete_obj_permanently = true;
	else
		pDoc->delete_obj_permanently = false;

	OnOK();
}

BOOL CPreferenceDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	if(pDoc->paste_always_override)
		m_prefPasteOverride.SetCheck(BST_CHECKED);
	else
		m_prefPasteOverride.SetCheck(BST_UNCHECKED);

	if(pDoc->upload_always_override)
		m_prefUploadOverride.SetCheck(BST_CHECKED);
	else
		m_prefUploadOverride.SetCheck(BST_UNCHECKED);

	CButton *deleteRadio;
	if(pDoc->delete_obj_permanently)
	{
		deleteRadio = (CButton *)this->GetDlgItem(IDC_PREF_DELETE_PERM);
		deleteRadio->SetCheck(BST_CHECKED);
	}
	else
	{
		deleteRadio = (CButton *)this->GetDlgItem(IDC_PERF_DELETE_TOTRASH);
		deleteRadio->SetCheck(BST_CHECKED);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPreferenceDlg::OnBnClickedButtonPrefCleanTrash()
{
	// TODO: Add your control notification handler code here
	if(AfxMessageBox("Are you sure you want to permanently delete data in trash area?", MB_YESNO) != IDYES)
	{
		return;
	}

	this->BeginWaitCursor();
	int t = irodsWinCleanTrashArea(pDoc->conn);
	if( t< 0)
	{
		CString msgHead = CString("Cleaning Trash error:");
		pDoc->disp_err_msgbox(msgHead, t);
		this->EndWaitCursor();
		return;
	}

	AfxMessageBox("Files/collections in trash area are cleaned!");
}
