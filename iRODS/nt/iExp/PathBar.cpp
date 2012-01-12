// PathBar.cpp : implementation file
//

#include "stdafx.h"
#include "inquisitor.h"
#include "PathBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PathBar dialog


PathBar::PathBar(CWnd* pParent /*=NULL*/)
	: CDialog(PathBar::IDD, pParent)
{
	//{{AFX_DATA_INIT(PathBar)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void PathBar::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PathBar)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PathBar, CDialog)
	//{{AFX_MSG_MAP(PathBar)
	ON_BN_CLICKED(IDC_PATHGO, OnPathgo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PathBar message handlers

void PathBar::OnPathgo() 
{
	AfxMessageBox("Hello!");	
}
