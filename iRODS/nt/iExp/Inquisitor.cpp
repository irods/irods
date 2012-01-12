// Inquisitor.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Inquisitor.h"

#include "MainFrm.h"
#include "InquisitorDoc.h"
#include "LeftView.h"
#include "Hyperlink.h"

#include <process.h>
#include <io.h>
#include <stdio.h>
#include <direct.h>
#include "CWebBrowser2.h"
#include "afxwin.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

	//
/////////////////////////////////////////////////////////////////////////////
// CInquisitorApp

BEGIN_MESSAGE_MAP(CInquisitorApp, CWinApp)
	//{{AFX_MSG_MAP(CInquisitorApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	//ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInquisitorApp construction

CInquisitorApp::CInquisitorApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	this->conn = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CInquisitorApp object

int image_reverse_map[] = {0, WINI_DATASET, WINI_COLLECTION, WINI_METADATA, WINI_QUERY, 0, WINI_RESOURCE
, WINI_USER, WINI_DOMAIN, 0, WINI_ACCESS};


CInquisitorApp theApp;
//CMutex gMutex;
//CCriticalSection gCS;
CMutex gCS;
bool gbExiting = false;
int gCount;
bool g_bIsDragging = false;
//bool g_bDeleteOriginalPostCopy = false;
std::vector<WINI::INode*> gNodeDragList;
unsigned int gOn;
HANDLE ghDragList = 0;
//CImageList* gDragImage = NULL;
//FORMATETC gformat;
//CharlieSource* gDS;
//UINT gInqClipboardFormat = 0;
UINT gIExpClipboardFormat = 0;
bool gbIsDemo = false;
CImageList gIconListLarge;
CImageList gIconListSmall;
CEvent killWorker;
bool gCaptureOn = false;



char* gDemoUser = "demouser";
char* gDemoZone = "npaci";
char* gDemoHost = "orion.sdsc.edu";
char* gDemoPort = "7544";
char* gDemoPassword = "SC99D";



/////////////////////////////////////////////////////////////////////////////
// CInquisitorApp initialization
WINI::StatusCode NukeDir(const char* szpath)
{
	char szdir[MAX_PATH];
	char szfilepath[MAX_PATH];
	struct _finddata_t fileinfo;
	long myhandle;
	
	if(NULL == _getcwd(szdir,MAX_PATH))
		return WINI_ERROR;

	if(0 != _chdir(szpath))
		return WINI_ERROR;

	myhandle = _findfirst("*.*", &fileinfo);	//returns "."

	if(-1 == myhandle)
		return WINI_ERROR;

	VERIFY(0 == _findnext(myhandle, &fileinfo));	//returns ".."
	int len;
	while(0 == _findnext(myhandle, &fileinfo))
	{
		strcpy(szfilepath, szpath);
		len = strlen(szfilepath);
		if(szfilepath[len-1] != '\\')
		{
			szfilepath[len] = '\\';
			szfilepath[len+1] = '\0';
		}
			
		strcat(szfilepath, fileinfo.name);

		if(fileinfo.attrib & _A_SUBDIR)
		{
			if(-1 == NukeDir(szfilepath))
			{
				_findclose(myhandle);
				return WINI_ERROR;
			}

		}else
		{
			if(-1 == remove(szfilepath))
			{
				_findclose(myhandle);
				return WINI_ERROR;
			}
		}
	}

	_findclose(myhandle);

	if(0 != _chdir(szdir))
		return WINI_ERROR;

	if(0 != _rmdir(szpath))
		return WINI_ERROR;

	return WINI_OK;
}

BOOL CInquisitorApp::InitInstance()
{
	//charliecharlie
	AfxOleInit();	//this is to enable drag and drop support
	//endcharliecharlie

	InitCommonControls();

	//gInqClipboardFormat = RegisterClipboardFormat("INQ Clipboard Format");
	gIExpClipboardFormat = RegisterClipboardFormat("iExp Clipboard Format");
	gIconListLarge.Create(IDB_VIEW_ICONS_LARGE,32,1,RGB(255,255,255));
	gIconListSmall.Create(IDB_VIEW_ICONS,16,1,RGB(255,255,255));

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("San Diego Supercomputer Center"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CInquisitorDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CLeftView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

#if 0
	//find temp file directory
	if(0 == ::GetTempPath(MAX_PATH, m_szTempPath))
	{
		AfxMessageBox("Cannot locate your temporary files directory. Please set your TEMP variable.");
		return FALSE;
	}

	char buf[MAX_PATH];

	//CHARLIEC
	sprintf(buf, "srb%d\\", _getpid());

	//sprintf(buf, "srb%d", _getpid());
	strcat(m_szTempPath, buf); 

	if(FALSE == ::CreateDirectory(m_szTempPath, NULL))
	{
		DWORD error = ::GetLastError();

		if(ERROR_ALREADY_EXISTS == error)
		{
			//clean up the directory and reuse
			/*
			if(!NukeDir(m_szTempPath).isOk())
			{
				AfxMessageBox("Error cleaning up from prior instance. Please delete srbXXX directories in your temp directory.");
				return FALSE;
			}
			*/

			if(FALSE == ::CreateDirectory(m_szTempPath, NULL))
			{
				error = ::GetLastError();
				sprintf(buf, "Error creating temp directory - Windows Code: %d", error);
				AfxMessageBox(buf);
				return FALSE;
			}
		}else
		{
			//report error
			sprintf(buf, "Error creating temp directory - Windows Code: %d", error);
			AfxMessageBox(buf);
			return FALSE;
		}
	}
#endif

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CHyperLink	m_email;
	CHyperLink	m_hyperlink;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
private:
	CStatic m_staticVersionBuild;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT

	m_hyperlink.SetURL(_T("http://www.irods.org"));
	m_email.SetURL(_T("mailto:srb@sdsc.edu"));
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_STATIC0001, m_email);
	DDX_Control(pDX, IDC_STATIC1111, m_hyperlink);
	//}}AFX_DATA_MAP
	
	DDX_Control(pDX, IDC_STATIC_VERSION, m_staticVersionBuild);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
	//ON_STN_CLICKED(IDC_STATIC0001, &CAboutDlg::OnStnClickedStatic0001)
END_MESSAGE_MAP()

// App command to run the dialog
void CInquisitorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CInquisitorApp message handlers

//

int CInquisitorApp::ExitInstance() 
{
	NukeDir(m_szTempPath);
	return CWinApp::ExitInstance();
}


void CInquisitorApp::DoWaitCursor(int nCode) 
{
	CWinApp::DoWaitCursor(nCode);
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	//CString myVersion = CString("Version ") + CString(RODS_REL_VERSION) + CString(RODS_API_VERSION);
	CString myVersion = CString("iRODS Explorer\nVersion ") + 
		CString(RODS_REL_VERSION) + CString(RODS_API_VERSION) +
		CString("\n(Build ") + CString(__DATE__) + CString(")");
	m_staticVersionBuild.SetWindowText(myVersion);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
