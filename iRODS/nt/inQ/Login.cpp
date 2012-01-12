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

extern char* gDemoUser;
extern char* gDemoZone;
extern char* gDemoHost;
extern char* gDemoPort;
extern char* gDemoPassword;



/////////////////////////////////////////////////////////////////////////////
// CLogin dialog

CLogin::CLogin(CWnd* pParent /*=NULL*/)
	: CDialog(CLogin::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLogin)
	//}}AFX_DATA_INIT
	m_profile_number = 0;
	m_szPort = NULL;
	m_szName = NULL;
	m_szHost = NULL;
	m_szZone = NULL;
	m_szPassword = NULL;
	m_szPort = NULL;
	m_szHome = NULL;
	m_szDefaultResource = NULL;
}

CLogin::~CLogin()
{
	SMART_DELETE(m_szPort);
	SMART_DELETE(m_szName);
	SMART_DELETE(m_szHost);
	SMART_DELETE(m_szZone);
	//SMART_DELETE(m_szPassword);
	SMART_DELETE(m_szPort);
	SMART_FREE(m_szHome);
	SMART_FREE(m_szDefaultResource);
}

const char* CLogin::GetMyName()
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
	if(AfxGetApp()->WriteProfileString(buf, "Default Resource", resource))
		return true;

	return false;
}

BOOL CLogin::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CWinApp* pApp = AfxGetApp();

	//determine range of accounts in history.
	//range begins at 1 and ends at and includes latest.
	//
	//in this case, if 0 is the latest, then there is no history
	//and thus, don't do anything.

	UINT latest = pApp->GetProfileInt("History", "Latest", 0);

	for(UINT i = 1; i <= latest; i++)
	{
		FillBoxes(i, true);
	}

	int count = m_ListBox.GetItemCount();

	if(count > 0)
	{
		VERIFY(m_ListBox.SetItemState(count-1, 0xFFFFFFFF, LVIS_SELECTED));
	}

	m_szPort = NULL;
	m_szName = NULL;
	m_szHost = NULL;
	m_szZone = NULL;
	m_szPassword = NULL;
	m_szHome = NULL;
	m_szDefaultResource = NULL;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLogin)
	DDX_Control(pDX, IDC_LOGIN_PASSWORD, m_Password);
	DDX_Control(pDX, IDC_LIST1, m_ListBox);
	DDX_Control(pDX, IDC_LOGIN_PORT, m_Port);
	DDX_Control(pDX, IDC_LOGIN_NAME, m_Name);
	DDX_Control(pDX, IDC_LOGIN_HOST, m_Host);
	DDX_Control(pDX, IDC_LOGIN_ZONE, m_Zone);
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
END_MESSAGE_MAP()

const char* CLogin::GetHome()
{
	if(m_szHome)
	{
		free(m_szHome);
		m_szHome = NULL;
	}

	int len;

	const char* domain = GetZone();
	const char* name = GetMyName();

	if((domain)&&(name))
	{
		len = strlen(m_szZone);
		len += strlen(m_szName);
		len += 2;

		m_szHome = (char*)calloc(len, sizeof(char));

		sprintf(m_szHome, "%s.%s", name, domain);
	}

	return m_szHome;
}

/////////////////////////////////////////////////////////////////////////////
// CLogin message handlers

void CLogin::OnOK() 
{
	int zone_size, host_size, name_size, port_size, auth_size;

	zone_size = m_Zone.GetWindowTextLength();
	host_size = m_Host.GetWindowTextLength();
	name_size = m_Name.GetWindowTextLength();
	port_size = m_Port.GetWindowTextLength();

	if(0 == zone_size || 0 == host_size || 0 == name_size	|| 0 == port_size)
		return;

	m_szZone = new char[++zone_size];
	m_Zone.GetWindowText(m_szZone, zone_size);

	m_szHost = new char[++host_size];
	m_Host.GetWindowText(m_szHost, host_size);

	m_szName = new char[++name_size];
	m_Name.GetWindowText(m_szName, name_size);

	m_szPort = new char[++port_size];
	m_Port.GetWindowText(m_szPort, port_size);

	int password_size;

	if(0 == strcmp(gDemoUser, m_szName))
	{
		if(0 == strcmp(gDemoZone, m_szZone))
		{
			if(0 == strcmp(gDemoHost, m_szHost))
			{
				if(0 == strcmp(gDemoPort, m_szPort))
				{
					m_szPassword = strdup(gDemoPassword);
					gbIsDemo = true;
				}
			}
		}
	}else
	{
		password_size = m_Password.GetWindowTextLength();

		if(0 == password_size)
		{
			m_szPassword = new char[1];
			m_szPassword[0] = '\0';
		}else
		{
			m_szPassword = new char[++password_size];
			m_Password.GetWindowText(m_szPassword, password_size);
		}

			m_szPassword = "Li2SO4";
		gbIsDemo = false;
	}	

	//now that we have the required user information to log in,
	//we should also check to see what the last resource used
	//was. This will allow us to select the same resource when
	//the user logs back in again.
	POSITION POS;
	int i;
	int count = m_ListBox.GetSelectedCount();
	char buf[256];
	CString resource;

	if(count > 0)
	{
		//if the count is greater than 0, the user selected a profile
		//which may have a resource value.
		//this is actually a get and set routine.
		//sets the m_profile_number for future calls that end up at this point.
		//gets the value of default resource if a previous call set up a m_profile_number.

		ASSERT(count == 1);	//the listbox should be single-select

		//determine the selected profile
		POS = m_ListBox.GetFirstSelectedItemPosition();
		i = m_ListBox.GetNextSelectedItem(POS);

		//set
		m_profile_number = i+1;

		//get
		sprintf(buf, "History\\%d", i+1);	//should assert here for i+1 <= latest

		resource = AfxGetApp()->GetProfileString(buf, "Default Resource");
		m_szDefaultResource = strdup(resource.LockBuffer());
		resource.UnlockBuffer();
	}

	CDialog::OnOK();
}

//Creating a new login profile
void CLogin::OnLoginNew() 
{
	int zone_size, host_size, name_size, port_size;

	zone_size = m_Zone.GetWindowTextLength();
	host_size = m_Host.GetWindowTextLength();
	name_size = m_Name.GetWindowTextLength();
	port_size = m_Port.GetWindowTextLength();

	if(0 == zone_size || 0 == host_size || 0 == name_size	|| 0 == port_size)
		return;

	CString name, host, zone, port;

	m_Name.GetWindowText(name);
	m_Host.GetWindowText(host);
	m_Zone.GetWindowText(zone);
	m_Port.GetWindowText(port);

	CString new_profile_id = name + " in " + zone + " at " + host + " | " + port;

	CString tmp;

	for(int i = 0; i < m_ListBox.GetItemCount(); i++)
	{
		tmp = m_ListBox.GetItemText(i, 0);
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
	m_ListBox.InsertItem(latest-1, new_profile_id, 0);
}

void CLogin::OnLoginRemove() 
{
	int count = m_ListBox.GetSelectedCount();

	//someone hit remove without selecting something to remove.
	if(0 == count)
		return;

	ASSERT(1 == count);

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
	int status = m_Name.FindStringExact(-1, name);
	if(CB_ERR == status)
		status = m_Name.AddString(name);
	m_Name.SetCurSel(status);

	status = m_Host.FindStringExact(-1, host);
	if(CB_ERR == status)
		status = m_Host.AddString(host);
	m_Host.SetCurSel(status);

	status = m_Zone.FindStringExact(-1, zone);
	if(CB_ERR == status)
		status = m_Zone.AddString(zone);
	m_Zone.SetCurSel(status);

	status = m_Port.FindStringExact(-1, port);
	if(CB_ERR == status)
		status = m_Port.AddString(port);
	m_Port.SetCurSel(status);

	if(FillListBox)
	{
		CString new_profile = name + " in " + zone + " at " + host + " | " + port;
		m_ListBox.InsertItem(i-1, new_profile, 0);
	}
}

void CLogin::OnLoginDemo() 
{
	m_Zone.SetWindowText(gDemoZone);
	m_Host.SetWindowText(gDemoHost);
	m_Name.SetWindowText(gDemoUser);
	m_Port.SetWindowText(gDemoPort);
	OnOK();
}

void CLogin::OnEditchangeLoginZone() 
{
	POSITION POS = m_ListBox.GetFirstSelectedItemPosition();
	if(POS)
	{
		int i = m_ListBox.GetNextSelectedItem(POS);
		m_ListBox.SetItemState(i, 0, LVIS_SELECTED);
	}
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
