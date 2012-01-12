// Login.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "Login.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bool gbIsDemo;


/////////////////////////////////////////////////////////////////////////////
// CLogin dialog

CLogin::CLogin(CWnd* pParent /*=NULL*/)
	: CDialog(CLogin::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLogin)
	//}}AFX_DATA_INIT
	m_profile_number = 0;
	m_szPort = "";
	m_szName = "";
	m_szHost = "";
	m_szZone = "";
	m_szPassword = "";
	m_szPort = "";
	m_szHome = "";
	m_szDefaultResource = "";
	m_nProfileId = -1;
}

CLogin::~CLogin()
{
#if 0
	SMART_DELETE(m_szPort);
	SMART_DELETE(m_szName);
	SMART_DELETE(m_szHost);
	SMART_DELETE(m_szZone);
	//SMART_DELETE(m_szPassword);
	SMART_DELETE(m_szPort);
	SMART_FREE(m_szHome);
	SMART_FREE(m_szDefaultResource);
#endif
}

CString CLogin::GetMyName()
{
	return m_szName;
}

bool CLogin::SetDefaultResource(const char* resource)
{
	if(NULL == resource)
		return false;

	if(0 == m_profile_number)
		return false;

	//no checking to see if it's a valid resource. assume the doc does that
	static char buf[255];
	sprintf(buf, "History\\%d", m_profile_number);
	if(AfxGetApp()->WriteProfileString(buf, "DefaultResource", resource))
		return true;

	return false;
}

BOOL CLogin::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CWinApp* pApp = AfxGetApp();

	//determine range of accounts in history.
	//range begins at 1 and ends at and includes latest.  Bing: "latest" is actually the count for previous logins.
	//
	//in this case, if 0 is the latest, then there is no history
	//and thus, don't do anything.

	UINT latest = pApp->GetProfileInt("History", "Latest", 0);

	m_ListBox.SetExtendedStyle( m_ListBox.GetExtendedStyle() | LVS_EX_FULLROWSELECT );
	m_ListBox.InsertColumn(0, "Name", LVCFMT_LEFT, 75);
	m_ListBox.InsertColumn(1, "Zone", LVCFMT_LEFT, 75);
	m_ListBox.InsertColumn(2, "Server Host", LVCFMT_LEFT, 110);
	m_ListBox.InsertColumn(3, "Port", LVCFMT_LEFT, 75);

	for(UINT i = 1; i <= latest; i++)
	{
		FillBoxes(i, true);
	}

	int count = m_ListBox.GetItemCount();

	if(count > 0)
	{
		VERIFY(m_ListBox.SetItemState(count-1, 0xFFFFFFFF, LVIS_SELECTED));
	}

	// set the last login
	int last_login = pApp->GetProfileInt("History", "LastLogin", 0);
	if(last_login == 0) /* no last login */
	{
		if(latest > 0)
		{
			last_login = latest;
		}
	}
	if(last_login > 0)
	{
		FillBoxes(last_login, false);
	}

	CButton *cbox = (CButton *)GetDlgItem(IDC_CHECK_PRELOGINS);
	if(pApp->GetProfileInt("ShowPreviousLogins", "YesNo", 1) == 0)
	{
		cbox->SetCheck(BST_UNCHECKED);
	}
	else
	{
		cbox->SetCheck(BST_CHECKED);
	}
	OnBnClickedCheckPrelogins();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLogin)
	DDX_Control(pDX, IDC_LOGIN_PASSWORD, m_PasswordEditbox);
	DDX_Control(pDX, IDC_LIST1, m_ListBox);
	DDX_Control(pDX, IDC_LOGIN_PORT, m_PortCombobox);
	DDX_Control(pDX, IDC_LOGIN_NAME, m_NameCombobox);
	DDX_Control(pDX, IDC_LOGIN_HOST, m_HostCombobox);
	DDX_Control(pDX, IDC_LOGIN_ZONE, m_ZoneCombobox);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLogin, CDialog)
	//{{AFX_MSG_MAP(CLogin)
	ON_BN_CLICKED(IDC_LOGIN_NEW, OnLoginNew)
	ON_BN_CLICKED(IDC_LOGIN_REMOVE, OnLoginRemove)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, OnClickList)
	ON_BN_CLICKED(IDC_LOGIN_DEMO, OnLoginDemo)
	ON_CBN_EDITCHANGE(IDC_LOGIN_ZONE, OnEditchangeLoginZone)
	ON_CBN_EDITCHANGE(IDC_LOGIN_HOST, OnEditchangeLoginHost)
	ON_CBN_EDITCHANGE(IDC_LOGIN_NAME, OnEditchangeLoginName)
	ON_CBN_EDITCHANGE(IDC_LOGIN_PORT, OnEditchangeLoginPort)
	//}}AFX_MSG_MAP
//	ON_WM_TIMER()
ON_BN_CLICKED(IDC_CHECK_PRELOGINS, &CLogin::OnBnClickedCheckPrelogins)
END_MESSAGE_MAP()

CString CLogin::GetHome()
{
	int len;

	const char* domain = GetZone();
	const char* name = GetMyName();

	m_szHome = "/" + GetZone() + "/home/" + GetMyName();

	return m_szHome;
}

/////////////////////////////////////////////////////////////////////////////
// CLogin message handlers

void CLogin::OnOK() 
{
	if((m_PasswordEditbox.GetWindowTextLength() == 0) || (m_ZoneCombobox.GetWindowTextLength() == 0) ||
		(m_HostCombobox.GetWindowTextLength() == 0) || (m_NameCombobox.GetWindowTextLength() == 0) ||
		(m_PortCombobox.GetWindowTextLength() == 0))
	{
		AfxMessageBox("At least one field is empty!");
		return;
	}
	
	m_ZoneCombobox.GetWindowText(m_szZone);
	m_HostCombobox.GetWindowText(m_szHost);
	m_NameCombobox.GetWindowText(m_szName);
	m_PortCombobox.GetWindowText(m_szPort);
	m_PasswordEditbox.GetWindowText(m_szPassword);

	this->BeginWaitCursor();
	
	// now we start to connect to iRODS server

	CInquisitorApp* pApp = (CInquisitorApp *)AfxGetApp();
	rErrMsg_t errMsg;
	CString msg;

	// conn
	pApp->conn = rcConnect((char *)(LPCTSTR)(m_szHost), atoi((char *)(LPCTSTR)(m_szPort)), 
					(char *)(LPCTSTR)(m_szName), ((char *)(LPCTSTR)m_szZone), 0, &errMsg);
	if(pApp->conn == NULL)
	{
		this->EndWaitCursor();
		pApp->conn = NULL;
		//this->KillTimer(CLOGIN_TIMER_ID);
		msg = CString(errMsg.msg) + CString("The server on ") + m_szHost + CString(" is probably down.");
		AfxMessageBox(msg);
		return;
	}

	int t = clientLoginWithPassword(pApp->conn, (char *)(LPCTSTR)(m_szPassword));
	if(t < 0)
	{
		char *myErrName, *mySubErrName;
		myErrName = rodsErrorName(t, &mySubErrName);
		msg.Format("irods login error: %s %s.", myErrName, mySubErrName);
		free(pApp->conn);
		pApp->conn = NULL;
		this->EndWaitCursor();
		AfxMessageBox(msg);
		return;
	}

	// go through the list to see if there is a match. If yes, get the default resource
	CString login_match_str = m_szName + " in " + m_szZone + " at " + m_szHost + " | " + m_szPort;
	for(int i=0;i<m_ListBox.GetItemCount();i++)
	{
		//CString tmpstr = m_ListBox.GetItemText(i, 0);
		//if(login_match_str.Compare(tmpstr) == 0)
		if(login_match_str.Compare(m_prevLogins[i]) == 0)
		{
			char buf[256];
			sprintf(buf, "History\\%d", i+1);
			m_szDefaultResource = pApp->GetProfileString(buf, "DefaultResource");
			m_nProfileId = i+1;

			pApp->WriteProfileInt("History", "LastLogin", i+1);
		}
	}

	this->EndWaitCursor();

	CDialog::OnOK();
}

void CLogin::OnCancel()
{
	exit(0);
}

//Creating a new login profile
void CLogin::OnLoginNew() 
{
	CString name, host, zone, port;

	m_NameCombobox.GetWindowText(name);
	m_HostCombobox.GetWindowText(host);
	m_ZoneCombobox.GetWindowText(zone);
	m_PortCombobox.GetWindowText(port);

	CString new_profile_id = name + " in " + zone + " at " + host + " | " + port;

	CString tmp;

	for(int i = 0; i < m_ListBox.GetItemCount(); i++)
	{
		//tmp = m_ListBox.GetItemText(i, 0);
		tmp = m_prevLogins[i];
		if(0 == new_profile_id.Compare(tmp))
			return;		//the profile already exists - do nothing
	}

	CWinApp* pApp = AfxGetApp();

	char buf[256];

	UINT latest = pApp->GetProfileInt("History", "Latest", 0);

	if(0 == latest)
	{
		//there is no history - begin a new one.
		//never store anything in "History\\0"!
		latest = 1;
		sprintf(buf, "History\\1");
	}
	else
	{
		//create a new slot
		sprintf(buf, "History\\%d", ++latest);
	}

	//adding the new profile to the registry and incrementing latest
	pApp->WriteProfileInt("History", "Latest", latest);
	pApp->WriteProfileString(buf, "Name", name);
	pApp->WriteProfileString(buf, "Host", host);
	pApp->WriteProfileString(buf, "Zone", zone);
	pApp->WriteProfileString(buf, "Port", port);

	//add the new profile to the listbox - remember listbox is always one behind the registry!
	//m_ListBox.InsertItem(latest-1, new_profile_id, 0);
	int n = m_ListBox.InsertItem(latest-1, name);
	latest++;
	m_ListBox.SetItemText(n, 1, zone);
	m_ListBox.SetItemText(n, 2, host);
	m_ListBox.SetItemText(n, 3, port);

	m_prevLogins.Add(new_profile_id);
}

void CLogin::OnLoginRemove() 
{
	int count = m_ListBox.GetSelectedCount();

	//someone hit remove without selecting something to remove.
	if(0 == count)
		return;

	ASSERT(1 == count);


	if(AfxMessageBox("Do you want to delete the selected login data?", MB_YESNO) != IDYES)
		return;

	POSITION POS = m_ListBox.GetFirstSelectedItemPosition();
	int i = m_ListBox.GetNextSelectedItem(POS);

	CWinApp* pApp = AfxGetApp();

	UINT latest = pApp->GetProfileInt("History", "Latest", 0);

	//latest should never be 0 if there was something to remove.
	//nevertheless, if it is 0 we should return and do nothing.
	if(0 == latest)
	{
		ASSERT(1);	
		return;
	}

	CString name, host, zone, port;

	char buf[256];
	char buf2[256];

	//i is the listbox id for the profile to be removed.
	//the registry id for the profile is always i+1.
	//latest is always a valid entry, but latest+1 is not - hence no reason to move
	for(UINT j = i + 1; j < latest; j++)
	{
		sprintf(buf, "History\\%d", j);
		sprintf(buf2, "History\\%d", j+1);

		name = pApp->GetProfileString(buf2, "Name");
		host = pApp->GetProfileString(buf2, "Host");
		zone = pApp->GetProfileString(buf2, "Zone");
		port = pApp->GetProfileString(buf2, "Port");
		pApp->WriteProfileString(buf, "Name", name);
		pApp->WriteProfileString(buf, "Host", host);
		pApp->WriteProfileString(buf, "Zone", zone);
		pApp->WriteProfileString(buf, "Port", port);
	}

	//reflect that we're now one-less a profile and remove the last one.
	sprintf(buf, "History\\%d", latest);
	//erase all the child values
	pApp->WriteProfileString(buf, "Name", NULL);
	pApp->WriteProfileString(buf, "Host", NULL);
	pApp->WriteProfileString(buf, "Zone", NULL);
	pApp->WriteProfileString(buf, "Port", NULL);

	//erase the history key itself
	HKEY hKey;
	sprintf(buf, "%d", latest);
	::RegOpenKey(HKEY_CURRENT_USER, "Software\\San Diego Supercomputer Center\\WINI\\History", &hKey);
	::RegDeleteKey(hKey, buf);

	if(--latest < 0)
		latest = 0;

	pApp->WriteProfileInt("History", "Latest", latest);

	//finally, update the GUI
	m_ListBox.DeleteItem(i);

	//fills the left-hand boxes with the profile following the removed one.
	//don't fill up the right-hand listbox. it should handle case where i
	//is an illegal value safely.
	//remember, the parameter is i+1 not to find the subsequent value.
	//the subsequent value is now i, we are feeding fillboxes i+1 because
	//it works in registry range, not listbox range!
	FillBoxes(i+1, false);
}

void CLogin::OnClickList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	if(1 != m_ListBox.GetSelectedCount())
		return;

	POSITION POS = m_ListBox.GetFirstSelectedItemPosition();
	int i = m_ListBox.GetNextSelectedItem(POS);

	FillBoxes(i+1, false);
}

//fillboxes expects i to be a registry value(1 to latest), not a listbox value (0 to latest-1)!
void CLogin::FillBoxes(int i, bool FillListBox)
{
	if(0 > i)
	{
		ASSERT(1);
		return;
	}

	CWinApp* pApp = AfxGetApp();

	UINT latest = pApp->GetProfileInt("History", "Latest", 0);

	//out of bounds - not ASSERT()able since the calling functions can be dumb to this.
	if(latest < i)
		return;


	static char buf[256];

	sprintf(buf, "History\\%d", i);

	CString name, host, zone, port, default_resource;

	//1st, copy all values from this registry entry to local values.
	name = pApp->GetProfileString(buf, "Name");
	host = pApp->GetProfileString(buf, "Host");
	zone = pApp->GetProfileString(buf, "Zone");
	port = pApp->GetProfileString(buf, "Port");

	//for debug purposes, assert that the values actually exist.
	//ASSERT(0 != name.Compare(""));
	//ASSERT(0 != host.Compare(""));
	//ASSERT(0 != zone.Compare(""));
	//ASSERT(0 != port.Compare(""));

	//to keep multiple copies of the same value appearing in each box
	//(there may be more than one domain with the username charliec, for instance).
	//only add if the value isn't in there already.
	int status = m_NameCombobox.FindStringExact(-1, name);
	if(CB_ERR == status)
		status = m_NameCombobox.AddString(name);
	m_NameCombobox.SetCurSel(status);

	status = m_HostCombobox.FindStringExact(-1, host);
	if(CB_ERR == status)
		status = m_HostCombobox.AddString(host);
	m_HostCombobox.SetCurSel(status);

	status = m_ZoneCombobox.FindStringExact(-1, zone);
	if(CB_ERR == status)
		status = m_ZoneCombobox.AddString(zone);
	m_ZoneCombobox.SetCurSel(status);

	status = m_PortCombobox.FindStringExact(-1, port);
	if(CB_ERR == status)
		status = m_PortCombobox.AddString(port);
	m_PortCombobox.SetCurSel(status);

	int n;
	if(FillListBox)
	{
		CString new_profile = name + " in " + zone + " at " + host + " | " + port;
		m_prevLogins.Add(new_profile);

		//m_ListBox.InsertItem(i-1, new_profile, 0); 
		n = m_ListBox.InsertItem(i-1, name);
		m_ListBox.SetItemText(n, 1, zone);
		m_ListBox.SetItemText(n, 2, host);
		m_ListBox.SetItemText(n, 3, port);
	}
}

void CLogin::OnLoginDemo() 
{
	AfxMessageBox("not implemented!");
}

void CLogin::OnEditchangeLoginZone() 
{
	/*
	POSITION POS = m_ListBox.GetFirstSelectedItemPosition();
	if(POS)
	{
		int i = m_ListBox.GetNextSelectedItem(POS);
		m_ListBox.SetItemState(i, 0, LVIS_SELECTED);
	}
	*/
}

void CLogin::OnEditchangeLoginHost() 
{
	// TODO: Add your control notification handler code here
	
}

void CLogin::OnEditchangeLoginName() 
{
	// TODO: Add your control notification handler code here
	
}

void CLogin::OnEditchangeLoginPort() 
{
	// TODO: Add your control notification handler code here
	
}

//void CLogin::OnTimer(UINT_PTR nIDEvent)
//{
//	// TODO: Add your message handler code here and/or call default
//	if(nIDEvent == CLOGIN_TIMER_ID)
//	{
//		this->EndWaitCursor();
//		AfxMessageBox("Login time out! Please double check login info.");
//	}
//
//	CDialog::OnTimer(nIDEvent);
//}

void CLogin::OnBnClickedCheckPrelogins()
{
	// TODO: Add your control notification handler code here
	// IDC_CHECK_PRELOGINS
	CButton *cbox = (CButton *)GetDlgItem(IDC_CHECK_PRELOGINS);
	int n = cbox->GetCheck();

	CButton *add_button = (CButton *)GetDlgItem(IDC_LOGIN_NEW);
	CButton *remove_button = (CButton *)GetDlgItem(IDC_LOGIN_REMOVE);

	CWinApp* pApp = AfxGetApp();
	if(n == 0)  /* unchecked */
	{
		m_ListBox.ShowWindow(SW_HIDE);
		add_button->ShowWindow(SW_HIDE);
		remove_button->ShowWindow(SW_HIDE);

		pApp->WriteProfileInt("ShowPreviousLogins", "YesNo", 0);
	}
	else
	{
		m_ListBox.ShowWindow(SW_SHOW);
		add_button->ShowWindow(SW_SHOW);
		remove_button->ShowWindow(SW_SHOW);

		pApp->WriteProfileInt("ShowPreviousLogins", "YesNo", 1);
	}
}
