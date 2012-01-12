// GenericDialog.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "GenericDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenericDialog dialog


CGenericDialog::CGenericDialog(char* szTitle, CWnd* pParent /*=NULL*/)
	: CDialog(CGenericDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGenericDialog)
	m_Edit = _T("");
	//}}AFX_DATA_INIT
}


void CGenericDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGenericDialog)
	DDX_Text(pDX, IDC_EDIT1, m_Edit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGenericDialog, CDialog)
	//{{AFX_MSG_MAP(CGenericDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGenericDialog message handlers

void CGenericDialog::OnOK() 
{
	CDialog::OnOK();
}

void CGenericDialog::PostNcDestroy() 
{
	//m_Edit.Replace('\'', '\'\'');
	CDialog::PostNcDestroy();
}
