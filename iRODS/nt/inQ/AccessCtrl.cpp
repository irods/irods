// AccessCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "inquisitor.h"
#include "AccessCtrl.h"
#include "AccessCtrlUserAdd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAccessCtrl dialog


CAccessCtrl::CAccessCtrl(CInquisitorDoc* doc, WINI::INode* node, CWnd* pParent /*=NULL*/)
	: CDialog(CAccessCtrl::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAccessCtrl)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_doc = doc;
	m_node = node;
	m_pParent = pParent;

	m_constraints = m_doc->GetAccessConstraint();
}

void CAccessCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAccessCtrl)
	DDX_Control(pDX, IDOK, m_OK);
	DDX_Control(pDX, IDC_RECURSIVEAC, m_Recursive);
	DDX_Control(pDX, IDC_ACCESS_CTRL_LIST, m_ListBox);
	DDX_Control(pDX, IDC_ACCESS_CTRL_CONSTRAINT, m_AccessConstraint);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAccessCtrl, CDialog)
	//{{AFX_MSG_MAP(CAccessCtrl)
	ON_BN_CLICKED(IDC_ACCESS_CTRL_ADD, OnAccessCtrlAdd)
	ON_BN_CLICKED(IDC_ACCESS_CTRL_REMOVE, OnAccessCtrlRemove)
	ON_CBN_SELCHANGE(IDC_ACCESS_CTRL_CONSTRAINT, OnSelchangeAccessCtrlConstraint)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAccessCtrl message handlers

void CAccessCtrl::OnAccessCtrlAdd() 
{
	WINI::ICatalogNode* mcat = m_doc->MyImpl.GetCatalog();
	CAccessCtrlUserAdd user(mcat, m_doc->GetAccessConstraint(), m_pParent);

	WINI::StatusCode status;


	if(IDOK == user.DoModal())
	{
		m_OK.EnableWindow(FALSE);
		status = m_doc->SetAccess(m_node, user.GetUser(), user.GetPermission(), (bool)m_Recursive.GetCheck());
		m_OK.EnableWindow(TRUE);

		if(!status.isOk())
		{
			AfxMessageBox(status.GetError());
		}else
		{
			Update();
		}
	}
}

void CAccessCtrl::OnAccessCtrlRemove() 
{
	int count = m_ListBox.GetSelCount();
	WINI::IAccessNode* node;
	WINI::StatusCode status;
	int* selections = new int[count];

	if(LB_ERR == m_ListBox.GetSelItems(count, selections))
	{
		delete [] selections;
		return;
	}

	for(int i = 0; i < count; i++)
	{
		node = (WINI::IAccessNode*)m_ListBox.GetItemData(selections[i]);
		m_OK.EnableWindow(FALSE);
		status = m_doc->ModifyAccess(node, "null", (bool)m_Recursive.GetCheck());
		m_OK.EnableWindow(TRUE);

		if(!status.isOk())
			AfxMessageBox(status.GetError());
	}

	delete [] selections;

	Update();
}

void CAccessCtrl::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CAccessCtrl::OnSelchangeAccessCtrlConstraint() 
{
	//get new constraint value.
	char constraint_buf[255];

	if(0 == m_AccessConstraint.GetWindowText(constraint_buf, 255))
	{
		return;
	}

	//get access nodes to be modified
	int count = m_ListBox.GetSelCount();

	if(0 == count)
		return;

	//POSITION POS;
	//int pos;
	int* selections = new int[count];
	WINI::StatusCode status;
	WINI::IAccessNode* node;

	if(LB_ERR == m_ListBox.GetSelItems(count, selections))
	{
		delete [] selections;
		return;
	}

	for(int i = 0; i < count; i++)
	{
		node = (WINI::IAccessNode*)m_ListBox.GetItemData(selections[i]);
		m_OK.EnableWindow(FALSE);
		status = m_doc->ModifyAccess(node, constraint_buf, (bool)m_Recursive.GetCheck());
		m_OK.EnableWindow(TRUE);

		if(!status.isOk())
			AfxMessageBox(status.GetError());
	}

	delete [] selections;

	Update();
}

void CAccessCtrl::Update()
{
	int i;
	
	do
	{
		i = m_ListBox.DeleteString(0);
	} while (LB_ERR != i);

	int count = m_node->CountChildren();


	m_nRedirector.clear();
	m_nRedirector.resize(count);

	WINI::INode* node;
	WINI::IAccessNode* access_node;

	int type, position;

	const char* hhhh;
	int count2 = 0;

	for(i = 0; i < count; i++)
	{
		node = m_node->GetChild(i);
		type = node->GetType();
		
		if(WINI_ACCESS == type)
		{
			access_node = (WINI::IAccessNode*)node;

			hhhh = access_node->GetName();
			position = m_ListBox.InsertString(count2, hhhh);
			m_ListBox.SetItemData(count2, (DWORD)node);
			m_nRedirector[count2] = i;
		}
	}
}

BOOL CAccessCtrl::OnInitDialog() 
{
	CDialog::OnInitDialog();

	const char* constraint;

	if(m_node->GetType() == WINI_DATASET)
		m_Recursive.ShowWindow(SW_HIDE);

	// TODO: Add extra initialization here
	for(int i = 0; i < m_constraints->CountStrings(); i++)
	{
		constraint = m_constraints->GetString(i);
		if(NULL != constraint)
			m_AccessConstraint.AddString(constraint);
	}

	if(!m_node->isOpen(WINI_ACCESS))
		m_doc->OpenTree(m_node, 1, true);

	Update();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

