// AccessCtrlUserAdd.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "AccessCtrlUserAdd.h"
#include "MainFrm.h"
#include "InquisitorDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAccessCtrlUserAdd dialog


CAccessCtrlUserAdd::CAccessCtrlUserAdd(CWnd* pParent /*=NULL*/)
	: CDialog(CAccessCtrlUserAdd::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAccessCtrlUserAdd)
	//}}AFX_DATA_INIT
	m_mcat = NULL;
	m_pParent = pParent;
}

CAccessCtrlUserAdd::CAccessCtrlUserAdd(WINI::ICatalogNode* mcat, WINI::IStringNode* constraints, CWnd* pParent /*=NULL*/)
	: CDialog(CAccessCtrlUserAdd::IDD, pParent)
{
	m_mcat = mcat;
	m_constraints = constraints;
	m_szConstraint = NULL;
}


void CAccessCtrlUserAdd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAccessCtrlUserAdd)
	DDX_Control(pDX, IDC_LISTZONE, m_Zone);
	DDX_Control(pDX, IDC_CONSTRAINT, m_CC);
	DDX_Control(pDX, IDC_USER_ADD_USER, m_User);
	DDX_Control(pDX, IDC_USER_ADD_DOMAIN, m_Domain);
	//}}AFX_DATA_MAP
}

void CAccessCtrlUserAdd::Update()
{
	WINI::INode* child;

	for(int i = 0; i < m_mcat->CountChildren(); i++)
	{
		child = m_mcat->GetChild(i);
		if(child->GetType() == WINI_ZONE)
		m_Zone.AddString(child->GetName());
	}
}

BOOL CAccessCtrlUserAdd::OnInitDialog()
{
	CDialog::OnInitDialog();

	const char* foo;

	for(int i = 0; i < m_constraints->CountStrings(); i++)
	{
		foo = m_constraints->GetString(i);
		m_CC.AddString(foo);
	}

	m_CC.SetCurSel(0);

	Update();

	return TRUE;
}




BEGIN_MESSAGE_MAP(CAccessCtrlUserAdd, CDialog)
	//{{AFX_MSG_MAP(CAccessCtrlUserAdd)
	ON_LBN_SELCHANGE(IDC_USER_ADD_DOMAIN, OnSelchangeUserAddDomain)
	ON_LBN_SELCHANGE(IDC_USER_ADD_USER, OnSelchangeUserAddUser)
	ON_LBN_SELCHANGE(IDC_LISTZONE, OnSelchangeListzone)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAccessCtrlUserAdd message handlers

void CAccessCtrlUserAdd::OnSelchangeListzone() 
{
	// TODO: Add your control notification handler code here
	int blah;
	
	do
	{
		blah = m_Domain.DeleteString(0);
	}while(LB_ERR != blah);

	do
	{
		blah = m_User.DeleteString(0);
	}while(LB_ERR != blah);

	blah = m_Zone.GetCurSel();

	if(LB_ERR == blah)
		return;

	WINI::IZoneNode* zone;
	WINI::ISetNode* domain_set;
	WINI::IDomainNode* domain;

	zone = (WINI::IZoneNode*)m_mcat->GetChild(blah);

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc* pDoc = (CInquisitorDoc*)frame->GetActiveDocument();
	pDoc->OpenTree(zone, 1, false, WINI_DOMAIN);

	domain_set = (WINI::ISetNode*)zone->GetChild("domains");

	if(domain_set)
	{
	for(int i = 0; i < domain_set->CountChildren(); i++)
	{
		domain = (WINI::IDomainNode*)domain_set->GetChild(i);
		m_Domain.AddString(domain->GetName());
	}
	}
}

void CAccessCtrlUserAdd::OnSelchangeUserAddDomain() 
{
	int blah;

	blah = m_Zone.GetCurSel();

	if(LB_ERR == blah)
		return;

	WINI::IZoneNode* zone;



	zone = (WINI::IZoneNode*)m_mcat->GetChild(blah);
	
	do
	{
		blah = m_User.DeleteString(0);
	}while(LB_ERR != blah);

	blah = m_Domain.GetCurSel();

	if(LB_ERR == blah)
		return;

	WINI::ISetNode* domain_set = (WINI::ISetNode*)zone->GetChild("domains");
	WINI::INode *child, *node = domain_set->GetChild(blah);

	if(!node->isOpen())
	{
		CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
		CInquisitorDoc* pDoc = (CInquisitorDoc*)frame->GetActiveDocument();
		pDoc->OpenTree(node, 1, false, WINI_USER);
	}

	blah = node->CountChildren();

	int pos;

	for(int i = 0; i < blah; i++)
	{
		child = node->GetChild(i);
		pos = m_User.AddString(child->GetName());
	}
}

void CAccessCtrlUserAdd::OnSelchangeUserAddUser() 
{	
}

void CAccessCtrlUserAdd::OnOK() 
{
	int zone_sel = m_Zone.GetCurSel();
 	int domain_sel = m_Domain.GetCurSel();
	int user_sel = m_User.GetCurSel();

	if(LB_ERR == zone_sel)
	{
		AfxMessageBox("Please select a zone");
		return;
	}

	if(LB_ERR == domain_sel)
	{
		AfxMessageBox("Please select a domain");
		return;
	}

	if(LB_ERR == user_sel)
	{
		AfxMessageBox("Please select a user");
		return;
	}

	int constraint_sel = m_CC.GetCurSel();

	if(LB_ERR == user_sel)
	{
		AfxMessageBox("Please select an access constraint");
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc* pDoc = (CInquisitorDoc*)frame->GetActiveDocument();
	WINI::ISetNode* domain_set = (WINI::ISetNode*)pDoc->MyImpl.GetCatalog()->GetChild(zone_sel)->GetChild("domains");

	m_newUser = (WINI::IUserNode*)domain_set->GetChild(domain_sel)->GetChild(user_sel);

	int len = m_CC.GetWindowTextLength();
	char* buf = new char[++len];
	m_CC.GetWindowText(buf, len);
	m_szConstraint = buf;	//right now this is not getting freed

	CDialog::OnOK();
}


