// AccessCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "inquisitor.h"
#include "AccessCtrl.h"
#include "MainFrm.h"
#include "AccessCtrlUserAdd.h"
#include "irodsWinUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAccessCtrl dialog


CAccessCtrl::CAccessCtrl(CInquisitorDoc* doc, CString & irodsPath, int obj_type, CWnd* pParent /*=NULL*/)
	: CDialog(CAccessCtrl::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAccessCtrl)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_doc = doc;
	m_objType = obj_type;
	m_irodsPath = irodsPath;
	m_pParent = pParent;

	//m_constraints = m_doc->GetAccessConstraint();
}

void CAccessCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAccessCtrl)
	DDX_Control(pDX, IDOK, m_OK);
	//DDX_Control(pDX, IDC_RECURSIVEAC, m_Recursive);
	DDX_Control(pDX, IDC_ACCESS_CTRL_LIST, m_ListBox);
	//DDX_Control(pDX, IDC_ACCESS_CTRL_CONSTRAINT, m_AccessConstraint);
	//DDX_Control(pDX, IDC_ACCESSCONTROL_RECURSIVE_CHECK, m_AccessConstraint);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_STATIC_CD_CAPTION, m_staticCollDataCaption);
	DDX_Control(pDX, IDC_STATIC_CD_DATA, m_staticCollDataPath);
	DDX_Control(pDX, IDC_ACCESSCONTROL_RECURSIVE_CHECK, m_recursiveCheckBtn);
	DDX_Control(pDX, IDC_ACL_USERS_COMBO, m_comboUsers);
	DDX_Control(pDX, IDC_ACL_PERMISSION_COMBO, m_comboPermission);
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

//void CAccessCtrl::add_permission_for_dataset()
void CAccessCtrl::OnAccessCtrlAdd() 
{
	CString sel_user;
	CString sel_permission;

	if((m_comboUsers.GetWindowTextLength() <= 0) || (m_comboPermission.GetWindowTextLength() <= 0))
	{
		AfxMessageBox("Please select both user and permission for this operation!");
		return;
	}

	m_comboUsers.GetWindowText(sel_user);
	m_comboPermission.GetWindowTextA(sel_permission);

	int pos = 0;

	int case_type = 0;   // 0 -> not in list, 1 -> in lst but diff perm, 2 -> in list same perm

	// check if the user is in the list
	for(int i=0;i<irodsUsers.GetSize();i++)
	{
		if(irodsUsers[i] == sel_user)
		{
			if(irodsConstraints[i] == sel_permission)
			{
				case_type = 2;
			}
			else
			{
				case_type = 1;
				pos = i;
			}
		}
	}

	if(case_type == 2)
	{
		AfxMessageBox("The permission is already in list!");
		return;
	}

	int recursive = 0;
	if(m_objType == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		recursive = m_recursiveCheckBtn.GetCheck();
	}

	this->BeginWaitCursor();
	int t = irodsWinSetAccessControl(m_doc->conn, my_zone, sel_user, sel_permission, m_irodsPath, recursive);
	if(t < 0)
	{
		CString msgHead = CString("Get Dataset Access Control() error");
		m_doc->disp_err_msgbox(msgHead, t);
		this->EndWaitCursor();
		return;
	}
	this->EndWaitCursor();

	CString tmpstr;
	if(case_type == 0)
	{
		// insert it
		int j = m_ListBox.GetCount();
		irodsUsers.Add(sel_user);
		irodsConstraints.Add(sel_permission);
		tmpstr = sel_user + ": " + sel_permission;
		m_ListBox.InsertString(j, tmpstr);
	}
	else if(case_type == 1)
	{
		//tmpstr = sel_user + ": " + sel_permission;
		//m_ListBox.SetItemText(
		//m_ListBox.SetDlgItemText(pos, tmpstr);

		irodsConstraints[pos] = sel_permission;

		// we redisplay permission
		m_ListBox.ResetContent();
		for(int i=0;i<irodsUsers.GetSize();i++)
		{
			tmpstr = irodsUsers[i] + ": " + irodsConstraints[i];
			m_ListBox.InsertString(i, tmpstr);
		}
	}
}

#if 0
void CAccessCtrl::OnAccessCtrlAdd() 
{
	if(m_objType == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		add_permission_for_dataset();
	}
	else if(m_objType == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
	}



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
#endif

void CAccessCtrl::OnAccessCtrlRemove() 
{
	int cnt = m_ListBox.GetSelCount();
	if(cnt <= 0)
		return;

	// confirmation
	if(AfxMessageBox("Are you sure you want to delete the access permission for selected user(s)?", MB_YESNO) != IDYES)
	{
		return;
	}

	CArray<int, int> selections;
	selections.SetSize(cnt);
	m_ListBox.GetSelItems(cnt, selections.GetData());

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString irodsZone = "";
	CString newPermission = "null";
	int i,j;
	int t;

	int recursive;
	if(m_objType == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		recursive = false;
	}
	else if(m_objType == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		recursive = m_recursiveCheckBtn.GetCheck();
	}
	else
	{
		return; // should never happen.
	}

	this->BeginWaitCursor();
	for(i=0;i<cnt;i++)
	{
		j = selections[cnt - i -1];
		t = irodsWinSetAccessControl(m_doc->conn, irodsZone, irodsUsers[j], newPermission, m_irodsPath, recursive);
		if(t < 0)
		{
			this->EndWaitCursor();
			CString msgHead = CString("Remove Access Permission() error");
			m_doc->disp_err_msgbox(msgHead, t);
			return;
		}
		m_ListBox.DeleteString(j);
		irodsUsers.RemoveAt(j);
		irodsConstraints.RemoveAt(j);
	}
	this->EndWaitCursor();

#if 0
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
#endif
}

void CAccessCtrl::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CAccessCtrl::OnSelchangeAccessCtrlConstraint() 
{
# if 0
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
#endif
}

void CAccessCtrl::Update()
{
#if 0
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
#endif
}

void  CAccessCtrl::SetDropLength(CComboBox* pDropList, int nItems)
{
	CRect rcCombo;
    CRect rcDropped;
    int nItemHeight = pDropList->GetItemHeight(-1);
      
    pDropList->GetClientRect(&rcCombo);
    pDropList->GetDroppedControlRect(&rcDropped);
    pDropList->GetParent()->ScreenToClient(&rcDropped);
    rcDropped.bottom = rcDropped.top + rcCombo.Height() + nItemHeight*nItems;
    pDropList->MoveWindow(&rcDropped);

}

BOOL CAccessCtrl::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int t;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString msg = "getting the access control info...";
	
	m_staticCollDataPath.SetWindowTextA(m_irodsPath);

	// set the captions
	if(m_objType == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		m_staticCollDataCaption.SetWindowTextA("Collection:");
		t = irodsWinGetCollectionAccessControl(m_doc->conn, m_irodsPath, irodsUsers, irodsZones, irodsConstraints);
	}
	else if(m_objType == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		m_staticCollDataCaption.SetWindowTextA("Dataset:");
		m_recursiveCheckBtn.ShowWindow(SW_HIDE);

		t = irodsWinGetDatasetAccessControl(m_doc->conn, m_irodsPath, irodsUsers, irodsConstraints);
	}
	

	if(t < 0)
	{
		CString msgHead = CString("Get Dataset Access Control() error");
		m_doc->disp_err_msgbox(msgHead, t);
		OnCancel();
	}

	CString tmpstr;
	for(int i=0;i<irodsUsers.GetSize();i++)
	{
		tmpstr = irodsUsers[i] + ": " + irodsConstraints[i];
		m_ListBox.InsertString(i, tmpstr);
	}

	m_comboPermission.AddString("read");
	m_comboPermission.AddString("write");
	m_comboPermission.AddString("own");
	SetDropLength(&m_comboPermission, 3);

	my_zone = m_doc->conn->clientUser.rodsZone;
	t = irodsWinGetUsersByZone(m_doc->conn, my_zone, users_in_my_zone);
	if(t < 0)
	{
		CString msgHead = CString("Get Users in Zone() error");
		m_doc->disp_err_msgbox(msgHead, t);
		OnCancel();
	}
	for(int i=0;i<users_in_my_zone.GetSize();i++)
	{
		m_comboUsers.AddString(users_in_my_zone[i]);
	}
	SetDropLength(&m_comboUsers, 8);

#if 0
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
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
