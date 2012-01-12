// MetadataDialog.cpp : implementation file
//

#include "stdafx.h"
#include "inquisitor.h"
#include "MetadataDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMetadataDialog dialog

//CMetadataDialog::CMetadataDialog(CInquisitorDoc* doc, SRB::INode* node, CWnd* pParent /*=NULL*/)
//	: CDialog(CMetadataDialog::IDD, pParent)

CMetadataDialog::CMetadataDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CMetadataDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMetadataDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMetadataDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMetadataDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMetadataDialog, CDialog)
	//{{AFX_MSG_MAP(CMetadataDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMetadataDialog message handlers
