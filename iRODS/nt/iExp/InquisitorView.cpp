// InquisitorView.cpp : implementation of the CInquisitorView class

#include "stdafx.h"
#include "Inquisitor.h"
#include "InquisitorDoc.h"
#include "InquisitorView.h"
//#include "winiObjects.h"
#include "winbase.h"
#include "GenericDialog.h"
#include "MainFrm.h"
#include "irodsWinCollection.h"
#include "irodsWinUtils.h"
#include <vector>
#include <algorithm>
#include "LeftView.h"
#include "MetadataDlg.h"
#include <AccCtrl.h>
#include <Aclapi.h>
#include "MetaSearchDlg.h"
#include "DialogWebDisp.h"
#include "irodsWinProgressCB.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define REPORT_NAME 0
#define REPORT_REPL 1
#define REPORT_SIZE 2
#define REPORT_OWNR 3
#define REPORT_TIME 4
#define REPORT_RPLS 5
#define REPORT_RGRP 7
#define REPORT_RESC 6
#define REPORT_CSUM 8

#define REPORT_WIDTH_NAME 200
#define REPORT_WIDTH_SIZE 75
#define REPORT_WIDTH_OWNR 75
#define REPORT_WIDTH_TIME 125
#define REPORT_WIDTH_REPL 75
#define REPORT_WIDTH_RPLS 75
#define REPORT_WIDTH_RGRP 100
#define REPORT_WIDTH_RESC 75
#define REPORT_WIDTH_CSUM 200

extern CMutex gCS;
extern int gCount;
extern bool g_bIsDragging;
extern CImageList* gDragImage;
//extern UINT gInqClipboardFormat;
extern UINT gIExpClipboardFormat;
extern CImageList gIconListLarge;
extern CImageList gIconListSmall;
extern CImageList gResourceIconListLarge;
extern CImageList gResourceIconListSmall;
//extern CharlieSource* gDS;
//extern std::vector<WINI::INode*> gNodeDragList;
//extern unsigned int gOn;
//extern HANDLE ghDragList;
/////////////////////////////////////////////////////////////////////////////
// CInquisitorView

IMPLEMENT_DYNCREATE(CInquisitorView, CListView)

BEGIN_MESSAGE_MAP(CInquisitorView, CListView)
	//{{AFX_MSG_MAP(CInquisitorView)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_COMMAND(ID_DELETE, OnDelete)
	ON_WM_CREATE()
	ON_COMMAND(ID_DOWNLOAD, OnDownload)
	//ON_COMMAND(ID_UPLOAD, OnUpload)
	//ON_COMMAND(ID_UPLOADFOLDER, OnUploadFolder)
	ON_COMMAND(ID_NEW_COLLECTION, OnNewCollection)
	ON_WM_CONTEXTMENU()
	//ON_COMMAND(ID_OPEN, OnOpen)
	//ON_COMMAND(ID_OPENTREE, OnOpenTree)
	ON_COMMAND(ID_REPLICATE, OnReplicate)
	ON_COMMAND(ID_ACCESS_CTRL, OnAccessControl)
	//ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndlabeledit)
	//ON_COMMAND(ID_METADATA, OnMetadata)
	//ON_UPDATE_COMMAND_UI(ID_METADATA, OnUpdateMetadata)
	//ON_UPDATE_COMMAND_UI(ID_QUERY, OnUpdateQuery)
	ON_UPDATE_COMMAND_UI(ID_ACCESS_CTRL, OnUpdateAccessCtrl)
	//ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBegindrag)   // BBBBBBBBBB
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	//ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	//ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_NEW_COLLECTION, OnUpdateNewCollection)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDelete)
	ON_COMMAND(ID_RENAME, OnRename)
	ON_UPDATE_COMMAND_UI(ID_RENAME, OnUpdateRename)
	//ON_COMMAND(ID_COMMENT, OnComment)
	//ON_UPDATE_COMMAND_UI(ID_COMMENT, OnUpdateComment)
	//ON_UPDATE_COMMAND_UI(ID_UPLOAD, OnUpdateUpload)
	//ON_UPDATE_COMMAND_UI(ID_UPLOADFOLDER, OnUpdateUploadFolder)
	ON_UPDATE_COMMAND_UI(ID_DOWNLOAD, OnUpdateDownload)
	//ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, OnBeginlabeledit)
	//ON_COMMAND(ID_NEW_META, OnNewMeta)
	//ON_UPDATE_COMMAND_UI(ID_NEW_META, OnUpdateNewMeta)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	//ON_COMMAND(ID_GETPATH, OnGetpath)
	//}}AFX_MSG_MAP
	
	ON_COMMAND(ID_SELECT_ALL, &CInquisitorView::OnSelectAll)
	ON_UPDATE_COMMAND_UI(ID_SELECT_ALL, &CInquisitorView::OnUpdateSelectAll)
	ON_COMMAND(ID_VIEW_REFRESH, &CInquisitorView::OnViewRefresh)
	ON_COMMAND(ID_UPLOAD_UPLOADAFOLDER, &CInquisitorView::OnUploadUploadafolder)
	ON_UPDATE_COMMAND_UI(ID_UPLOAD_UPLOADAFOLDER, &CInquisitorView::OnUpdateUploadUploadafolder)
	ON_COMMAND(ID_UPLOAD_UPLOADFILES, &CInquisitorView::OnUploadUploadfiles)
	ON_UPDATE_COMMAND_UI(ID_UPLOAD_UPLOADFILES, &CInquisitorView::OnUpdateUploadUploadfiles)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPYPATHTOCLIPBOARD, &CInquisitorView::OnUpdateEditCopypathtoclipboard)
	ON_UPDATE_COMMAND_UI(ID_REPLICATE, &CInquisitorView::OnUpdateReplicate)
	//ON_COMMAND(ID_EDIT_METADATA, &CInquisitorView::OnEditMetadata)
	//ON_UPDATE_COMMAND_UI(ID_EDIT_METADATA, &CInquisitorView::OnUpdateEditMetadata)
	ON_COMMAND(ID_EDIT_COPYPATHTOCLIPBOARD, &CInquisitorView::OnEditCopypathtoclipboard)
	ON_COMMAND(ID_EDIT_METADATA_SYSMETADATA, &CInquisitorView::OnEditMetadataSystemmetadata)
	ON_UPDATE_COMMAND_UI(ID_EDIT_METADATA_SYSMETADATA, &CInquisitorView::OnUpdateEditMetadataSystemmetadata)
	ON_COMMAND(ID_EDIT_METADATA_USERMETADATA, &CInquisitorView::OnEditMetadataUsermetadata)
	ON_UPDATE_COMMAND_UI(ID_EDIT_METADATA_USERMETADATA, &CInquisitorView::OnUpdateEditMetadataUsermetadata)
	//ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CInquisitorView::OnLvnBegindrag)
END_MESSAGE_MAP()
//ON_REGISTERED_THREAD_MESSAGE(msgUpdateOperation, OnUpdateOperation)
//
// PLUMBING SECTION
//

CInquisitorView::CInquisitorView()
{
	m_bInitialized = false;
}

CInquisitorView::~CInquisitorView()
{
}

BOOL CInquisitorView::PreCreateWindow(CREATESTRUCT& cs)
{
	if(!CListView::PreCreateWindow(cs))
		return FALSE;

	//cs.style |=  LVS_EDITLABELS;

	return TRUE;
}

/*
void CInquisitorView::OnDraw(CDC* pDC)
{
	CInquisitorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CListCtrl& refCtrl = GetListCtrl();
	refCtrl.InsertItem(0, "Item!");		//note: this is boilerplate code - don't touch
}
*/

void CInquisitorView::OnInitialUpdate()
{
	CInquisitorDoc* pDoc = GetDocument();
	if(!m_bInitialized)
	{
		CListCtrl& list = GetListCtrl();

		list.SetImageList(&gIconListSmall,LVSIL_SMALL);
		list.SetImageList(&gIconListLarge,LVSIL_NORMAL);

		list.InsertColumn(REPORT_NAME,"Name", LVCFMT_LEFT,				REPORT_WIDTH_NAME);
		list.InsertColumn(REPORT_REPL,"Replica", LVCFMT_LEFT,			REPORT_WIDTH_REPL);
		list.InsertColumn(REPORT_SIZE,"Size", LVCFMT_LEFT,				REPORT_WIDTH_SIZE);
		list.InsertColumn(REPORT_OWNR,"Owner", LVCFMT_LEFT,				REPORT_WIDTH_OWNR);
		list.InsertColumn(REPORT_TIME,"Modified Time", LVCFMT_LEFT,		REPORT_WIDTH_TIME);
		list.InsertColumn(REPORT_RPLS,"Repl. Status", LVCFMT_LEFT,		REPORT_WIDTH_RPLS);
		list.InsertColumn(REPORT_RGRP,"Resource Group", LVCFMT_LEFT,	REPORT_WIDTH_RGRP);
		list.InsertColumn(REPORT_RESC,"Resource", LVCFMT_LEFT,			REPORT_WIDTH_RESC);
		//list.InsertColumn(REPORT_CSUM,"Checksum", LVCFMT_LEFT,			REPORT_WIDTH_CSUM);
		this->ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
		//list.SetExtendedStyle( list.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT );
		list.SetExtendedStyle( list.GetExtendedStyle() | LVS_EX_FULLROWSELECT );

		// set initial directory.
		char curdir[1024];
		int t = GetCurrentDirectory(1024, curdir);
		if(t > 0)
		{
			curdir[t] = '\0';
			pDoc->local_temp_dir = curdir;
			pDoc->local_temp_dir += "\\tmp";

			struct __stat64 mystat;
			if(_stat64((char *)LPCTSTR(pDoc->local_temp_dir), &mystat) < 0)
			{
				// create the temp directory if it does not exist.
				SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
				PSID everyone_sid = NULL;
				AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &everyone_sid);
				EXPLICIT_ACCESS ea;
				ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
				ea.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
				ea.grfAccessMode = SET_ACCESS;
				ea.grfInheritance = NO_INHERITANCE;
				ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
				ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
				ea.Trustee.ptstrName  = (LPSTR)everyone_sid;  // (LPWSTR)everyone_sid;
				PACL acl = NULL;
				SetEntriesInAcl(1, &ea, NULL, &acl);
				PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
				InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
				SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE);
				SECURITY_ATTRIBUTES sa;
				sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				sa.lpSecurityDescriptor = sd;
				sa.bInheritHandle = FALSE;

				::CreateDirectory((char *)LPCTSTR(pDoc->local_temp_dir), &sa);

				FreeSid(everyone_sid);
				LocalFree(sd);
				LocalFree(acl);
			}
			else  /* try to set permissions */
			{
			}
		}
		
		m_bInitialized = true;
	}

	// start to fill the tree with initial collection information
	pDoc->user_collection_home = CString("/") + pDoc->conn->clientUser.rodsZone + CString("/home/") + pDoc->conn->clientUser.userName;
	pDoc->SetTitle("iRODS Explorer");

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CTreeCtrl& tree = frame->GetLeftPane()->GetTreeCtrl();

	// build the tree nodes for /ZONE/home/user
	irodsWinCollection *rootColl, *zoneColl, *homeColl, *userColl;

	CString tmpStr = "/";
	rootColl = new irodsWinCollection();
	rootColl->fullPath = "/";
	rootColl->name = "/";
	HTREEITEM htiRoot = tree.InsertItem(tmpStr, 23, 44);
	tree.SetItemData(htiRoot, (DWORD_PTR)rootColl);

	HTREEITEM htiParent, htiCurrent;
	htiParent = htiRoot;
	tmpStr = CString(pDoc->conn->clientUser.rodsZone);
	htiCurrent = tree.InsertItem(tmpStr, 23, 44, htiParent);
	zoneColl = new irodsWinCollection();
	zoneColl->fullPath = CString("/") + CString(pDoc->conn->clientUser.rodsZone);
	zoneColl->name = CString(pDoc->conn->clientUser.rodsZone);
	tree.SetItemData(htiCurrent, (DWORD_PTR)zoneColl);
	tree.Expand(htiParent, TVE_EXPAND);

	htiParent = htiCurrent;
	tmpStr = "home";
	htiCurrent = tree.InsertItem(tmpStr, 23, 44, htiParent);
	homeColl = new irodsWinCollection();
	homeColl->fullPath = CString("/") + pDoc->conn->clientUser.rodsZone + CString("/home");
	homeColl->name = "home";
	tree.SetItemData(htiCurrent, (DWORD_PTR)homeColl);
	tree.Expand(htiParent, TVE_EXPAND);

	htiParent = htiCurrent;
	tmpStr = CString(pDoc->conn->clientUser.userName);
	userColl = new irodsWinCollection();
	userColl->fullPath = pDoc->user_collection_home;
	userColl->name = pDoc->conn->clientUser.userName;
	htiCurrent = tree.InsertItem(tmpStr, 2, 44, htiParent);
	tree.SetItemData(htiCurrent, (DWORD_PTR)userColl);
	tree.Expand(htiParent, TVE_EXPAND);

	rootColl->childCollections.push_back(*zoneColl);
	zoneColl->childCollections.push_back(*homeColl);
	homeColl->childCollections.push_back(*userColl);

	rootColl->queried = true;
	zoneColl->queried = true;
	homeColl->queried = true;

	rootColl = NULL;
	zoneColl = NULL;
	homeColl = NULL;
	userColl = NULL;

	frame->ShowWindow(SW_SHOWNORMAL);

	// query the resources
	CString msg = "getting the irods resouces ...";
	frame->statusbar_msg(msg);
	frame->BeginWaitCursor();
	int t = irodsWinGetResourceNames(pDoc->conn, pDoc->resc_names);
	frame->statusbar_clear();
	if(t < 0)
	{
		CString msgHead = CString("Get iRODS resources error ");
		pDoc->disp_err_msgbox(msgHead, t);
		frame->EndWaitCursor();
		return;
	}
	CStringArray grpresc;
	t = irodsWinGetGroupResourceNames(pDoc->conn, grpresc);
	int i;
	if(t > 0)
	{
		for(i=0;i<grpresc.GetSize();i++)
		{
			pDoc->resc_names.Add(grpresc[i]);
		}
	}
	frame->EndWaitCursor();
	frame->FillResourceList(pDoc->resc_names, pDoc->default_resc_from_login);
	frame->EndProgressBar();

	tree.SelectItem(htiCurrent);

	gGuiProgressCB = NULL;
}

#ifdef _DEBUG
void CInquisitorView::AssertValid() const
{
	CListView::AssertValid();
}

void CInquisitorView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CInquisitorDoc* CInquisitorView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CInquisitorDoc)));
	return (CInquisitorDoc*)m_pDocument;
}
#endif //_DEBUG

void CInquisitorView::GetImage(const int& node_type, const char* sub_type, bool isOpen, int &image, int& selected_image)
{
	switch(node_type) 
	{ 
	case WINI_DATASET: 
		image = 1; 
		break; 
	case WINI_COLLECTION: 
		image = 2; 
		break; 
	case WINI_METADATA: 
		image = 3; 
		break; 
	case WINI_ACCESS: 
		image = 12; 
		break; 
	case WINI_RESOURCE: 
		if(0 == strcmp("unix file system", sub_type)) 
			image = 14; 
		else if(0 == strcmp("hpss file system", sub_type)) 
			image = 15; 
		else if(0 == strcmp("oracle dblobj database", sub_type)) 
			image = 13; 
		else 
			image = 7; 
		break; 
	case WINI_QUERY: 
		image = 4; 
		break; 
	case WINI_DOMAIN: 
		image = 10; 
		break; 
	case WINI_USER: 
		image = 9; 
		break; 
	case WINI_ZONE: 
		image = 16; 
		break; 
	default: 
		image = 0; 
		break; 
	} 
	 
	selected_image = image + 42; 

	if(isOpen) 
		image += 21;
}


void CInquisitorView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CListCtrl& list = GetListCtrl();


#if 0
	VERIFY(list.DeleteAllItems());	

	m_nRedirector.clear();

	if(3 == lHint)	//inQ hardcode to clear screen - screen is clear so return
	{
		((CMainFrame*)AfxGetMainWnd())->SetCountMessage("");
		return;
	}

	WINI::INode* node = GetDocument()->GetCurrentNode();
		
	if(NULL == node)	//returns if disconnected and someone plays with icon/detail modes
		return;

	int count = node->CountChildren();

	m_nRedirector.resize(count);

	WINI::INode* child;

	int type;
	int dataset_count = 0;
	int collection_count = 0;
	int metadata_count = 0;
	int query_count = 0;
	int resource_count = 0;
	int container_count = 0;
	int access_count = 0;

	DWORD dwStyle = GetWindowLong(list.m_hWnd, GWL_STYLE);

	int image, selected_image;

	//we can't use count because count can decrement
	//when deleting metadata.
	int i;
	for(i = 0; i < node->CountChildren(); i++)
	{
		child = node->GetChild(i);
		type = child->GetType();

		switch(type)
		{
		case WINI_DATASET:
			dataset_count++;
			break;
		case WINI_COLLECTION:
			collection_count++;
			break;
		case WINI_METADATA:
			metadata_count++;
			continue;	//do not add metadata to node, go to next child;
		case WINI_QUERY:
			query_count++;
			break;
		case WINI_RESOURCE:
			resource_count++;
			break;
		case WINI_ACCESS:
			access_count++;
			continue;		//this is so access permissions aren't displayed in this window
			break;
		default:
			continue;
		}

		WINI::IDatasetNode* dataset;

		const char * blah = child->GetName();

		const char* szSub_type = NULL;

		if(WINI_RESOURCE == type)
			szSub_type = ((WINI::IResourceNode*)child)->GetResourceType();

		GetImage(type, szSub_type, child->isOpen(WINI_ALL ^ WINI_ACCESS ^ WINI_METADATA), image, selected_image);

		int position = list.InsertItem(i, child->GetName(), image);

		m_nRedirector[position] = i;

		if(dwStyle & LVS_REPORT)
		{
			switch(type)
			{
			case WINI_DATASET:
				dataset = (WINI::IDatasetNode*)child;
				list.SetItemText(position, REPORT_NAME, dataset->GetName());
				list.SetItemText(position, REPORT_SIZE, dataset->GetSize());
				list.SetItemText(position, REPORT_OWNR, dataset->GetOwner());
				list.SetItemText(position, REPORT_TIME, dataset->GetModifiedTime());
				list.SetItemText(position, REPORT_REPL, dataset->GetReplicantNumber());
				list.SetItemText(position, REPORT_RPLS, dataset->GetReplicantStatus());
				list.SetItemText(position, REPORT_RGRP, dataset->GetResourceGroup());
				list.SetItemText(position, REPORT_RESC, dataset->GetResource());
				list.SetItemText(position, REPORT_CSUM, dataset->GetChecksum());
				break;
			default:
				break;
			}
		}

	}

	static char c[32];
	static char d[32];
	static char m[32];
	static char q[32];
	static char r[32];
	static char cn[32];
	static char a[32];

	if(collection_count > 0)
	{
		sprintf(c, "Collections: %d", collection_count);
		if(dataset_count || metadata_count || query_count || resource_count || container_count || access_count)
			strcat(c, " - ");
	}else
		c[0]=0;

	if(dataset_count > 0)
	{
		sprintf(d, "Datasets: %d", dataset_count);
		if(query_count || metadata_count || resource_count || container_count || access_count)
			strcat(d, " - ");
	}else
		d[0]=0;

	if(query_count > 0)
	{
		sprintf(d, "Queries: %d", query_count);
		if(metadata_count || resource_count || container_count || access_count)
			strcat(q, " - ");
	}else
		q[0]=0;

	if(metadata_count > 0)
	{
		sprintf(m, "Metadata: %d", metadata_count);
		if(resource_count || container_count || access_count)
			strcat(m, " - ");
	}else
		m[0]=0;

	if(resource_count > 0)
	{
		sprintf(r, "Resources: %d", resource_count);
		if(container_count || access_count)
			strcat(r, " - ");
	}else
		r[0]=0;

	if(container_count > 0)
	{
		sprintf(cn, "Containers: %d", container_count);
		if(access_count)
			strcat(cn, " - ");
	}else
		cn[0]=0;

	if(access_count > 0)
	{
		sprintf(a, "Users: %d", access_count);
	}else
		a[0]=0;

	static char message[128];
	message[0] = 0;

	strcat(message, c);
	strcat(message, d);
	strcat(message, q);
	strcat(message, m);
	strcat(message, r);
	strcat(message, cn);
	strcat(message, a);

	if(0 == strcmp("", message))
		strcpy(message, "no children");

	((CMainFrame*)AfxGetMainWnd())->SetCountMessage(message);
#endif
}

void CInquisitorView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() != 1)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int i = list.GetNextSelectedItem(POS);
	
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(i);
	CInquisitorDoc* pDoc = GetDocument();
	if(pDoc->rlist_disp_mode == CInquisitorDoc::RLIST_MODE_COLL)
	{
		if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_COLLECTION_TYPE)
		{
			irodsWinCollection *pColl = (irodsWinCollection *)list.GetItemData(i);
			pDoc->rlist_dbclick_coll_action(pColl->fullPath);
		}
		else  // it is a file. download the file and launch app.
		{
			irodsWinDataobj *pData = (irodsWinDataobj *)list.GetItemData(i);
			//CString fname = pData->parentCollectionFullPath + "/" + pData->name;
			pDoc->disp_file_in_local_machine(pData->parentCollectionFullPath, pData->name);
		}
	}
}

void CInquisitorView::OnUpdateOperation()
{
	CInquisitorDoc* pDoc = GetDocument();

	pDoc->UpdateAllViews(NULL);
}

int CInquisitorView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if(!m_dropTarget.Register(this))
	{
		AfxMessageBox("Could not register InquisitorView as dropTarget");
		return -1;
	}

	if (CListView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

#if 0
void CInquisitorView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
}
#endif

#if 0
void CInquisitorView::OnOpen()
{
	CListCtrl& list = GetListCtrl();
	POSITION POS = list.GetFirstSelectedItemPosition();
	int i = list.GetNextSelectedItem(POS);

	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* parent = pDoc->GetCurrentNode();

	WINI::INode* child = parent->GetChild(m_nRedirector[i]);

	gCount = 1;

	if(WINI_DATASET == child->GetType())
	{
		pDoc->Download(child, true);
	}

	pDoc->OpenTree(child, 1, true);

	pDoc->UpdateAllViews(NULL, 2);

}
#endif

#if 0
void CInquisitorView::OnOpenTree()
{
	CListCtrl& list = GetListCtrl();
	POSITION POS = list.GetFirstSelectedItemPosition();
	int i = list.GetNextSelectedItem(POS);

	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* parent = pDoc->GetCurrentNode();

	WINI::INode* child = parent->GetChild(m_nRedirector[i]);

	pDoc->SetCurrentNode(child);

	pDoc->UpdateAllViews(NULL);
}
#endif

void CInquisitorView::OnRename() 
{
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();
	if(count != 1)
		return;

	CString NewName;
	CGenericDialog myDlg("Change Name");
	if(myDlg.DoModal() == IDOK)
	{
		NewName = myDlg.m_Edit;
		// validate the string
		if(NewName.Find("/") >= 0)
		{
			AfxMessageBox("A collection or dataset name cannot contain '/' character(s)!");
			return;
		}
	}
	else
	{
		return;
	}

	POSITION POS = list.GetFirstSelectedItemPosition();
	int p = list.GetNextSelectedItem(POS);

	irodsWinCollection *pColl;
	irodsWinDataobj *pDataObj;

	int t;
	CString tmpstr;
	CString OldName;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->BeginWaitCursor();

	CInquisitorDoc* pDoc = GetDocument();
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);
	switch(tmpiobj->my_win_obj_type)
	{
	case irodsWinObj::IRODS_COLLECTION_TYPE:
		pColl = (irodsWinCollection *)list.GetItemData(p);
		irodsWinUnixPathGetParent(pColl->fullPath, tmpstr);
		irodsWinUnixPathGetName(pColl->fullPath, OldName);
		tmpstr = tmpstr + "/" + NewName;
		t = irodsWinRenameOneCollection(pDoc->conn, pColl->fullPath, tmpstr);
		if(t == 0)
		{
			if(pDoc->rlist_disp_mode == CInquisitorDoc::RLIST_MODE_COLL)
				pDoc->rlist_mode_coll_change_one_collname(pColl->fullPath, NewName);
			pColl->fullPath = tmpstr;
			pColl->name = NewName;	
			list.SetItemText(p, 0, NewName);
		}
		break;
	case irodsWinObj::IRODS_DATAOBJ_TYPE:
		pDataObj = (irodsWinDataobj *)list.GetItemData(p);
		OldName = pDataObj->name;
		t = irodsWinRenameOneDataobj(pDoc->conn, pDataObj->parentCollectionFullPath, pDataObj->name, NewName);
		if(t == 0)
		{
			if(pDoc->rlist_disp_mode == CInquisitorDoc::RLIST_MODE_COLL)
				pDoc->rlist_mode_coll_change_one_dataname(pDataObj->parentCollectionFullPath, pDataObj->name, NewName);
			//pDataObj->name = NewName;
			for(int i=0;i<list.GetItemCount();i++)
			{
				tmpiobj = (irodsWinObj *)list.GetItemData(i);
				if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_DATAOBJ_TYPE)
				{
					pDataObj = (irodsWinDataobj *)list.GetItemData(i);
					if(pDataObj->name == OldName)
					{
						pDataObj->name = NewName;
						list.SetItemText(i, 0, NewName);
					}
				}
			}
		}
		break;
	}

	if(t != 0)
	{
		frame->EndWaitCursor();
		CString msgHead = CString("Rename Operation error ");
		pDoc->disp_err_msgbox(msgHead, t);
		return;
	}
	
	frame->EndWaitCursor();
	CString msg = "Rename operation succeeded.";
	frame->statusbar_msg(msg);
}

void CInquisitorView::OnDelete() 
{
	CListCtrl& list = GetListCtrl();

	if(list.GetSelectedCount() <= 0)
		return;

	// confirmation for delete selected collections recursively and all selected datasets.
	if(AfxMessageBox("This will delete selected datasets and all selected collections recursively. Are you sure?", MB_YESNO) != IDYES)
		return;

	std::vector<int> listbox_sels;
	POSITION POS = list.GetFirstSelectedItemPosition();
	if(POS == NULL)
		return;
	while(POS)
	{
		int t = list.GetNextSelectedItem(POS); 
		listbox_sels.push_back(t);
	}
	sort(listbox_sels.begin(), listbox_sels.end());

	CInquisitorDoc* pDoc = GetDocument();
	irodsWinCollection *tColl;
	irodsWinDataobj *tData;
	CString obj_fullpath;
	int t;
	CString msgHead;
	CString msg;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();

	frame->StartProgressBar();
	frame->BeginWaitCursor();

	int repnum_to_del;
	bool recursive = true;    //always since we already confirmed with user.
    bool only_copy;
	// deleting the item from bottom up, which means in the decreasing order with selected pos.
	int n = listbox_sels.size();
	for(int i=0;i<n;i++)
	{
		int j = listbox_sels[n-i-1];       // delete items from bottom up

		irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(j);

		switch(tmpiobj->my_win_obj_type)
		{
		case irodsWinObj::IRODS_DATAOBJ_TYPE:
			tData = (irodsWinDataobj *)list.GetItemData(j);
			// check if it is the only replica in the system
			only_copy = pDoc->is_only_data_copy(tData->name);
			if((only_copy)&&(!pDoc->delete_obj_permanently))
			{
				repnum_to_del = -1;
			}
			else
			{
				repnum_to_del = tData->replNum;
			}
			obj_fullpath = tData->parentCollectionFullPath + "/" + tData->name;

			msg = CString("deleting ") + obj_fullpath + CString("...");
			irodsWinMainFrameProgressMsg((char *)LPCTSTR(msg), -1.0);

			t = irodsWinDeleteOneDataobj(pDoc->conn, tData->name, obj_fullpath, repnum_to_del, pDoc->delete_obj_permanently);
			if((t == 0)&&(pDoc->rlist_disp_mode == CInquisitorDoc::RLIST_MODE_COLL))
			{
				pDoc->rlist_mode_coll_delete_one_dataobj(tData->name, tData->parentCollectionFullPath, tData->replNum);
			}
			break;
		case irodsWinObj::IRODS_COLLECTION_TYPE:
			tColl = (irodsWinCollection *)list.GetItemData(j);
			obj_fullpath = tColl->fullPath;

			msg = CString("deleting collection ") + obj_fullpath + CString("...");
			irodsWinMainFrameProgressMsg((char *)LPCTSTR(msg), -1.0);

			t = irodsWinDeleteOneCollection(pDoc->conn, obj_fullpath, pDoc->delete_obj_permanently, true);
			if((t == 0)&&(pDoc->rlist_disp_mode == CInquisitorDoc::RLIST_MODE_COLL))
			{
				pDoc->rlist_mode_coll_delete_one_coll(obj_fullpath);
			}
			break;
		}

		if(t < 0)
		{
			frame->EndWaitCursor();
			frame->EndProgressBar();
			msgHead = CString("Delete Operation error for ") +  obj_fullpath;
			pDoc->disp_err_msgbox(msgHead, t);
			return;
		}

		// remove the item from rlist
		rlist_remove_one_itemdata(j);
		list.DeleteItem(j);
	}

	msg = CString("Delete Operation finished.");
	frame->statusbar_msg(msg);
	frame->EndWaitCursor();
	frame->EndProgressBar();
}

void CInquisitorView::OnDownload()
{
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() <= 0)
		return;

	irodsWinCollection *pColl;
	irodsWinDataobj *pData;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc* pDoc = GetDocument();

	CString selectedLocalDir = "";
	pDoc->SelectLocalFolder(selectedLocalDir);
	if(selectedLocalDir.GetLength() == 0)
		return;

	int forceful_flag = 1;
	int recursive_flag = 1;
	int t;
	CString irod_obj_fullpath;
	POSITION POS = list.GetFirstSelectedItemPosition();
	int p;

	CString msg;

	frame->StartProgressBar();
	gGuiProgressCB = &irodsWinMainFrameProgressCB;
	frame->BeginWaitCursor();

	while(NULL != POS)
	{
		p = list.GetNextSelectedItem(POS);
		irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);
		switch(tmpiobj->my_win_obj_type)
		{
		case irodsWinObj::IRODS_COLLECTION_TYPE:
			pColl = (irodsWinCollection *)list.GetItemData(p);
			irod_obj_fullpath = pColl->fullPath;
			break;
		case irodsWinObj::IRODS_DATAOBJ_TYPE:
			pData = (irodsWinDataobj *)list.GetItemData(p);
			irod_obj_fullpath = pData->parentCollectionFullPath + "/" + pData->name;
			break;
		}

		t = irodsWinDownloadOneObject(pDoc->conn, irod_obj_fullpath, selectedLocalDir, 
									recursive_flag, forceful_flag);
		if(t < 0)
		{
			CString msgHead = CString("Downlod Operation error ");
			pDoc->disp_err_msgbox(msgHead, t);
			frame->EndWaitCursor();
			frame->EndProgressBar();
			return;
		}
	}	

	msg = CString("Downalod collection(s)/file(s) finished.");
	frame->statusbar_msg(msg);
	frame->EndWaitCursor();
	frame->EndProgressBar();
	gGuiProgressCB = NULL;
}

#if 0
void CInquisitorView::OnUpload() 
{
	CInquisitorDoc* pDoc = GetDocument();
	WINI::INode* node = pDoc->GetCurrentNode();

	CListCtrl& list = GetListCtrl();

	UINT count = list.GetSelectedCount();

	POSITION POS;
	int i;
	int st;
	CString FileName;

	//windows FileDialog is jacked - if validation is on, it seems that any combination
	//of long-filenames (or short?) past a certain length automatically invalidates (or
	//at least makes the dialog return IDCANCEL. Turned it off with NOVALIDATE. but now
	//we don't have validation!
	CFileDialog FileDlg(TRUE, NULL, NULL, OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST, NULL, NULL);

	switch(count)
	{
	case 0:
		//use current directory
		break;
	case 1:
		//use that directory
		POS = list.GetFirstSelectedItemPosition();
		i = list.GetNextSelectedItem(POS);
		node = node->GetChild(m_nRedirector[i]);
		if(WINI_COLLECTION != node->GetType())
		{
			AfxMessageBox("You must select a collection.");
			return;
		}
		break;
	default:
		AfxMessageBox("Please select a single collection, or deselect all to use this collection.");
		return;
	}

	FileDlg.m_ofn.lpstrTitle = "Upload";

	st = FileDlg.DoModal();

	if(st != IDOK)
	{
		if(IDCANCEL != st)
		{
			MessageBox("Error Occured");
		}
	
		return;
	}

	POS = FileDlg.GetStartPosition();
	gCount = 0;
	while(POS != NULL)
	{
		FileName = FileDlg.GetNextPathName(POS);
		gCount++;
		//definitely we cannot resume the thread until we're done!
		//otherwise the call will have a different (and wrong) gCount each time!
		pDoc->Upload(node, FileName.GetBuffer(MAX_PATH));
	}
}
#endif

#if 0
void CInquisitorView::OnUploadFolder() 
{
	BROWSEINFO bwinfo;
	char buffer[MAX_PATH];
	LPITEMIDLIST itemids;
	char* folder_path;
	LPMALLOC shallocator;
	CInquisitorDoc* pDoc;
	WINI::INode* node;
	POSITION POS;
	int i;

	CListCtrl& list = GetListCtrl();

	gCount = 1;

	FillMemory((void*)&bwinfo,sizeof(bwinfo),0);
	bwinfo.hwndOwner=NULL;
	bwinfo.pidlRoot=NULL;
	bwinfo.pszDisplayName=(char*) buffer;
	bwinfo.lpszTitle="Place downloads in folder:";
	bwinfo.ulFlags=0;
	itemids = SHBrowseForFolder(&bwinfo);
	if(itemids == NULL)
		return;

	folder_path = new char[MAX_PATH];
	SHGetPathFromIDList(itemids,(char*) folder_path);

	shallocator;
	SHGetMalloc(&shallocator);
	shallocator->Free((void*) itemids);
	shallocator->Release();

	pDoc = GetDocument();

	node = pDoc->GetCurrentNode();

	POS = list.GetFirstSelectedItemPosition();

	pDoc->Upload(node, folder_path);
}
#endif

void CInquisitorView::OnNewCollection()
{
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();
	if(count != 1)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int i = list.GetNextSelectedItem(POS);

	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(i);
	if(tmpiobj->my_win_obj_type != irodsWinObj::IRODS_COLLECTION_TYPE)
		return;
	
	irodsWinCollection *tColl = (irodsWinCollection *)list.GetItemData(i);
	CString sel_coll_path = tColl->fullPath;
	
	// get user input
	CGenericDialog myDlg("New Collection");
	if(myDlg.DoModal() == IDCANCEL)
		return;

	CString newCollectionName = myDlg.m_Edit.GetString();
	newCollectionName.Trim();
	if(newCollectionName.GetLength() == 0)
	{
		AfxMessageBox("The input collection name is invalid!");
		return;
	}

	CInquisitorDoc* pDoc = GetDocument();
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->BeginWaitCursor();

	// add code to create new collection
	CString newCollectionWholePath = sel_coll_path + "/" + newCollectionName;
	int t = mkColl(pDoc->conn, (char *)LPCTSTR(newCollectionWholePath));
	if(t != 0)
	{
		CString msgHead = CString("mkColl() error for ") +  newCollectionWholePath;
		pDoc->disp_err_msgbox(msgHead, t);
		frame->EndWaitCursor();
		return;
	}
	frame->EndWaitCursor();
	
	AfxMessageBox("The new sub-collection is created.");
}

void CInquisitorView::OnReplicate()
{
	CListCtrl& list = GetListCtrl();

	if(list.GetSelectedCount() <= 0)
		return;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString irodsResc = frame->get_default_resc();
	if(irodsResc.GetLength() == 0)
	{
		AfxMessageBox("Please select a deafult resource for this operation!");
		return;
	}

	if(AfxMessageBox("This will replicate the selected collections/datasets recursively into default resource. Are you sure?", MB_YESNO) != IDYES)
	{
		return;
	}

	irodsWinCollection *pColl;
	irodsWinDataobj *pDataObj;
	CStringArray ObjsToBeReplicated;
	CString tmpstr;

	CArray<int, int> sel_coll_pos;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int p;
	while(NULL != POS)
	{
		p = list.GetNextSelectedItem(POS);
		irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);
		switch(tmpiobj->my_win_obj_type)
		{
		case irodsWinObj::IRODS_COLLECTION_TYPE:
			sel_coll_pos.Add(p);
			pColl = (irodsWinCollection *)list.GetItemData(p);
			ObjsToBeReplicated.Add(pColl->fullPath);
			break;
		case irodsWinObj::IRODS_DATAOBJ_TYPE:
			pDataObj = (irodsWinDataobj *)list.GetItemData(p);
			tmpstr = pDataObj->parentCollectionFullPath + "/" + pDataObj->name;
			ObjsToBeReplicated.Add(tmpstr);
			break;
		}
	}	

	CInquisitorDoc* pDoc = GetDocument();
	frame->BeginWaitCursor();
	int t = irodsWinReplicateObjects(pDoc->conn, ObjsToBeReplicated, irodsResc);
	frame->EndWaitCursor();

	if(t != 0)
	{
		CString msgHead = CString("Replaicate operation error ");
		pDoc->disp_err_msgbox(msgHead, t);
		return;
	}

	// refresh the selected parent.
	if(pDoc->rlist_disp_mode == CInquisitorDoc::RLIST_MODE_COLL)
		pDoc->refresh_disp();
}

void CInquisitorView::OnAccessControl() 
{
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() != 1)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int p = list.GetNextSelectedItem(POS);
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);

	int obj_type = tmpiobj->my_win_obj_type;
	CString irodsObjFullPath;

	irodsWinCollection *pColl;
	irodsWinDataobj *pData;
	CInquisitorDoc* pDoc = GetDocument();

	switch(obj_type)
	{
	case irodsWinObj::IRODS_COLLECTION_TYPE:
		pColl = (irodsWinCollection *)list.GetItemData(p);
		irodsObjFullPath = pColl->fullPath;
		
		break;
	case irodsWinObj::IRODS_DATAOBJ_TYPE:
		pData = (irodsWinDataobj *)list.GetItemData(p);
		irodsObjFullPath = pData->parentCollectionFullPath + "/" + pData->name;
		break;
	}
	pDoc->SetAccess(irodsObjFullPath, obj_type);

	

	

#if 0
	WINI::INode* current_node;
	POSITION POS;
	int pos;

	CInquisitorDoc* pDoc = GetDocument();

	current_node = pDoc->GetCurrentNode();

	switch(list.GetSelectedCount())
	{
	case 0:
		//use current directory
		break;
	case 1:
		//use child
		 POS = list.GetFirstSelectedItemPosition();
		 pos = list.GetNextSelectedItem(POS);
		current_node = current_node->GetChild(m_nRedirector[pos]);
		break;
	default:
		AfxMessageBox("You cannot select more than one element at a time.");
		return;
	}

	pDoc->SetAccess(current_node);
#endif
}

#if 0
void CInquisitorView::OnMetadata() 
{
	CMetadataDialog mydlg;
	mydlg.DoModal();
}
#endif

//
//	DRAG AND DROP / CLIPBOARD SECTION
//

//drop-target function: called when dnd operation first enters view's area.
//potential in future to call update and/or do some other visual information besides dragover?
#if 0
DROPEFFECT CInquisitorView::OnDragEnter( COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	return OnDragOver(pDataObject, dwKeyState, point);
}
#endif

//drop-target function: workhorse function. can be called thousands of times over (as long as dnd
//operation is occuring in client area). Job is to evaluate and return drop-effect code identifying
//what drop operation is allowed at that coordinate point, if any. This can be affected by what is
//being dragged, what it is sitting over (collection, dataset, nothing, etc.), and keystate (pressing
//ctrl may change the operation from copy to move)
//assume that oledataobject contains all data of same type. that is, if HDROP(file) is available,
//then InqType isn't. Assume if file drop then drag coming from outside and if inqtype drag coming from
//inside. (Don't support dragging between instances!)
#if 0 // Bing
DROPEFFECT CInquisitorView::OnDragOver( COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{

	if(pDataObject->IsDataAvailable(gInqClipboardFormat))
	{
		return InternalDataDragOver(pDataObject, dwKeyState, point);
	}
	else if(pDataObject->IsDataAvailable(CF_HDROP))
	{
		return DROPEFFECT_MOVE;
		//return FileDragOver(pDataObject, dwKeyState, point);
	}

	/*  BBBBBB take care it later
	if(pDataObject->IsDataAvailable(CF_HDROP))
		return FileDragOver(pDataObject, dwKeyState, point);
	*/

	return DROPEFFECT_NONE;
}
#endif

#if 0
DROPEFFECT CInquisitorView::FileDragOver(COleDataObject* pDataObject, DWORD& dwKeyState, CPoint& point)
{
	CListCtrl& list = GetListCtrl();

	int test = list.HitTest(point);

	WINI::INode* node;

	if(-1 == test)
	{
		//currently over current_node, and not any particular child of current_node
		node = GetDocument()->GetCurrentNode();
		if(WINI_COLLECTION == node->GetType())
			return DROPEFFECT_COPY;
	}else
	{
		//currently selecting a child of the current_node
		node = GetDocument()->GetCurrentNode()->GetChild(m_nRedirector[test]);
		if(WINI_COLLECTION == node->GetType())
		{
			list.SetItemState(test, 1, LVIS_SELECTED);
			return DROPEFFECT_COPY;
		}
	}

	return DROPEFFECT_NONE;
}
#endif

DROPEFFECT CInquisitorView::InternalDataDragOver(COleDataObject* pDataObject, DWORD& dwKeyState, CPoint& point)
{
	CListCtrl& list = GetListCtrl();

	int t = list.HitTest(point);

	// -1 is no hit
	if(t >= 0)
	{
		//currently selecting a child of the current_node
		irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(t);
		if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_COLLECTION_TYPE);
		{
			list.SetItemState(t, 1, LVIS_DROPHILITED); // LVIS_SELECTED);
			return DROPEFFECT_MOVE;
		}
	}

	return DROPEFFECT_MOVE;
	//return DROPEFFECT_NONE;
}

#if 0
DROPEFFECT CInquisitorView::InqDragOver(COleDataObject* pDataObject, DWORD& dwKeyState, CPoint& point)
{
	CListCtrl& list = GetListCtrl();

	int test = list.HitTest(point);

	//WINI::INode* node;

	if(-1 == test)
	{
		//currently over current_node, and not any particular child of current_node
		node = GetDocument()->GetCurrentNode();
		if(WINI_COLLECTION == node->GetType())
			return DROPEFFECT_MOVE;
	}else
	{
		//currently selecting a child of the current_node
		node = GetDocument()->GetCurrentNode()->GetChild(m_nRedirector[test]);
		if(WINI_COLLECTION == node->GetType())
		{
			list.SetItemState(test, 1, LVIS_SELECTED);
			return DROPEFFECT_MOVE;
		}
	}

	return DROPEFFECT_NONE;
}
#endif

//drop-target function: called when a drag op moves out of the view - this function can be used
//to clean up after any visual things done, such as destroying a drag image created once the object
//was dragged into the view. We might need to use this function to download selected files if the
//drag originated from within inqview, and convert information on the clipboard from inqview type to
//file drop type, which points to the dl'ed files, now stored in temp directory.
#if 0
void CInquisitorView::OnDragLeave()
{
		//g_bIsDragging = false;
}
#endif

//drop-target function: function is called when a selection is dropped onto this view.
//very important to accept only file and inq drop types. drop can originate from outside inq
//or inside (passing then, file or inq type respectively).
#if 0 // Bing
BOOL CInquisitorView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) 
{
	/*
	POINT pt;
	GetCursorPos(&pt);
	CListCtrl& list = GetListCtrl();
	int t = list.HitTest(pt);
	if(t >= 0)
	{
		// check if the selected item is a collection. otherwise. go to next : to parent
	}
	else
	{
		return 0;
	}
	*/

	// if it is a file from outside, take care of it. Otherwise let BeginDrag take of of internal drag drop.
    if(!pDataObject->IsDataAvailable(CF_HDROP))
	{
		return 1;
	}

	// drag drop from files of local machine
	g_bIsDragging = false;

	return 1;
}
#endif

#if 0
BOOL CInquisitorView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) 
{
	g_bIsDragging = false;

	CInquisitorDoc* pDoc = GetDocument();

	CListCtrl& list = GetListCtrl();
	
	int position = list.HitTest(point);

	WINI::INode* selected_node;

	if(-1 == position)
		selected_node = pDoc->GetCurrentNode();	//it is not on one of the items in the view
	else
		//it is on one of the items in the view
		selected_node = pDoc->GetCurrentNode()->GetChild(m_nRedirector[position]);

	if(!selected_node->isOpen())
		pDoc->OpenTree(selected_node, 1, true);

	int count;
	WINI::INode* dragged_node;

	if(pDataObject->IsDataAvailable(gInqClipboardFormat))
	{
		count = gNodeDragList.size();
		gCount = 0;
		ASSERT(DROPEFFECT_MOVE == dropEffect);
		int i;
		for(i = 0; i < count; i++)
		{
			dragged_node = gNodeDragList.at(i);

			if(NULL == dragged_node)
				continue;
		
			if(selected_node == dragged_node)
			{
				AfxMessageBox("Source and target are same, skipping...");
				continue;
			}

			gCount++;
			pDoc->Copy(selected_node, dragged_node, true);
		}

		gNodeDragList.clear();
		gOn = 0;;

		return TRUE;
	}

	STGMEDIUM item;
	HDROP DropHandle;
	UINT quantity;
	char buf[MAX_PATH];

	if(pDataObject->IsDataAvailable(CF_HDROP))
	{
		if(pDataObject->GetData(CF_HDROP, &item))
		{
			DropHandle = (HDROP)item.hGlobal;
			quantity = DragQueryFile(DropHandle, 0xFFFFFFFF, NULL, 0);
			gCount = quantity;
			for(UINT i = 0; i < quantity; i++)
			{
				DragQueryFile(DropHandle, i, buf, MAX_PATH);
				pDoc->Upload(selected_node, buf);
			}
		}

		return TRUE;
	}

	return FALSE;	//default
}
#endif

#if 0 // BIng
void CInquisitorView::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CListCtrl& list = GetListCtrl();

	int count = list.GetSelectedCount();

	*pResult = 0;
	if(0 == count)
		return;

	ListSelectedItemsToCopyBuffer();

	g_bIsDragging = true;

	COleDropSource DropSource;
	COleDataSource DropData;

	HGLOBAL hMem = ::GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE|
                               GMEM_DDESHARE,
                               0);
	memcpy( (char*)::GlobalLock(hMem), NULL,0 );

	::GlobalUnlock(hMem);

	DropData.CacheGlobalData( CF_HDROP, hMem );
	DROPEFFECT de = DropData.DoDragDrop(DROPEFFECT_COPY|
                                      DROPEFFECT_MOVE,NULL);

	if(g_bIsDragging == false)
		return;

	POINT pt;
	GetCursorPos(&pt);
	list.ScreenToClient(&pt);
	int t = list.HitTest(pt);
	if(t < 0)
	{
		g_bIsDragging = false;
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString msg;

	// 1. get selected copy. no operation on an item.  
	irodsWinObj *drop_obj = (irodsWinObj *)list.GetItemData(t);
	if(drop_obj->my_win_obj_type != irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		msg = "Cannot move or copy selected data to a file.";
		frame->statusbar_msg(msg);
		return;
	}

	irodsWinCollection *pColl;
	pColl = (irodsWinCollection *)list.GetItemData(t);

	// check if the operation is allowed. GGGGG
	CInquisitorDoc* pDoc = GetDocument();
	
	if(pDoc->check_coll_with_buffered_objs(pColl->fullPath) == pDoc->HAS_A_SAME_COLL_IN_BUFFERED_DATA)
	{
		msg = "The destinated collection is among selected data.";
		frame->statusbar_msg(msg);
		g_bIsDragging = false;
		return;
	}
	else if(pDoc->check_coll_with_buffered_objs(pColl->fullPath) == pDoc->IS_A_DESCENDANT_OF_A_COLL_IN_BUFFERED_DATA)
	{
		msg = "The detinated collection is a descendant of one selected collection to be moved or copied.";
		frame->statusbar_msg(msg);
		g_bIsDragging = false;
		return;
	}

	if(de == DROPEFFECT_COPY)
	{
		pDoc->paste_buffered_irods_objs(pColl->fullPath);
	}
	else if(de == DROPEFFECT_MOVE)
	{   
		pDoc->move_buffered_irods_objs(pColl->fullPath);
	}

	g_bIsDragging = false;
	*pResult = 0;
}
#endif
	
//drop-source function: called when a user drags one or more items.
//this function will always then create inq-types onto the clipboard.
//I think this is actually part of the non-ole API but hey, it works. I am sure there's
//no law mixing the two... It is better to have the OS decide what starts a drag op
//than myself hardcoding it into OnLButtonDown.
#if 0
void CInquisitorView::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC fe2= {gInqClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	CInquisitorDoc* pDoc = GetDocument();

	CharlieSource* pDataSource = new CharlieSource;
	pDataSource->m_doc = pDoc;

	CListCtrl& list = GetListCtrl();

	int count = list.GetSelectedCount();

	if(0 == count)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int pos;

	WINI::INode* current_node = pDoc->GetCurrentNode();

	if(NULL == current_node)
		return;

	WINI::INode* child;

//	int list_count = gNodeDragList.size();

	gNodeDragList.clear();
	gOn = 0;

//	list_count = gNodeDragList.size();
	int i;
	for(i = 0; i < count; i++)
	{
		pos = list.GetNextSelectedItem(POS);
		child = current_node->GetChild(m_nRedirector[pos]);
		if(NULL == child)
			continue;
		gNodeDragList.push_back(child);
	}

	STGMEDIUM stgmedium;

	stgmedium.tymed = TYMED_HGLOBAL;
	stgmedium.hGlobal = NULL;

	pDataSource->SetAsyncMode(TRUE);
	pDataSource->CacheData(gInqClipboardFormat, &stgmedium);
	pDataSource->DelayRenderData(CF_HDROP, &fe);

	g_bIsDragging = true;

	DROPEFFECT dropEffect = pDataSource->DoDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE);

	

	BOOL myBOOL;

	ASSERT(S_OK == pDataSource->InOperation(&myBOOL));

	*pResult = 0;
}
#endif


//general dnd function: with this function you can update the screen by repositioning a image, etc.
void CInquisitorView::OnMouseMove(UINT nFlags, CPoint point) 
{
	CListView::OnMouseMove(nFlags, point);
}

void CInquisitorView::OnEditCopy() 
{
	ListSelectedItemsToCopyBuffer();
}

void CInquisitorView::ListSelectedItemsToCopyBuffer()
{
	CListCtrl& list = GetListCtrl();

	int n = list.GetSelectedCount();
	if(n == 0)
		return;

	CInquisitorDoc* pDoc = GetDocument();
	pDoc->colls_in_copy_buffer.RemoveAll();
	pDoc->dataobjs_in_copy_buffer.RemoveAll();

	// add the selected items
	irodsWinDataobj *pDataObj, tDataObj;
	irodsWinCollection *pColl, tColl;
	int pos;
	bool already_in_dataobj_buffer;
	POSITION POS = list.GetFirstSelectedItemPosition();
	for(int i=0;i<n;i++)
	{
		pos = list.GetNextSelectedItem(POS);
		list.GetItemData(pos);
		irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(pos);
		switch(tmpiobj->my_win_obj_type)
		{
		case irodsWinObj::IRODS_DATAOBJ_TYPE:
			pDataObj = (irodsWinDataobj *)list.GetItemData(pos);
			already_in_dataobj_buffer = false;
			// remove the replicas
			for(int j=0;j<pDoc->dataobjs_in_copy_buffer.GetSize();j++)
			{
				if(pDoc->dataobjs_in_copy_buffer[j].name == pDataObj->name)
				{
					already_in_dataobj_buffer = true;
					break;
				}
			}
			if(! already_in_dataobj_buffer)
			{
				pDataObj->CopyTo(tDataObj);
				pDoc->dataobjs_in_copy_buffer.Add(tDataObj);
			}
			break;

		case irodsWinObj::IRODS_COLLECTION_TYPE:
			pColl = (irodsWinCollection *)list.GetItemData(pos);
			pColl->CopyTo(tColl);
			pDoc->colls_in_copy_buffer.Add(tColl);
			break;
		}
	}
}

#if 0
void CInquisitorView::OnEditCut() 
{
	gDS = new CharlieSource;
	gDS->m_doc = GetDocument();

	FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC fe2= {gInqClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	CListCtrl& list = GetListCtrl();
	UINT count = list.GetSelectedCount();

	POSITION POS = list.GetFirstSelectedItemPosition();
	int pos, type;
	int size = 0;

	WINI::INode* child;

	WINI::INode* parent = GetDocument()->GetCurrentNode();

	for(UINT i = 0; i < count; i++)
	{
		pos = list.GetNextSelectedItem(POS);
		child = parent->GetChild(m_nRedirector[pos]);
		type = child->GetType();
		if(WINI_COLLECTION == type || WINI_DATASET == type)
		{
			size += 9 + strlen(child->GetName()) + strlen(child->GetPath()) + sizeof(int);
		}
	}

	//++size;	//plus one for the terminating null (for double null at the end!)
	size += sizeof(int) + sizeof(char);

	HANDLE handle = ::GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, size * sizeof(char));

	void* ptr = ::GlobalLock(handle);
	int* repl = (int*)ptr;
	char* szptr = (char*)ptr + sizeof(int);

	POS = list.GetFirstSelectedItemPosition();
	size = 0;

	int i;
	for(i = 0; i < count; i++)
	{
		pos = list.GetNextSelectedItem(POS);
		child = parent->GetChild(m_nRedirector[pos]);
		type = child->GetType();
		if(WINI_COLLECTION == type || WINI_DATASET == type)
		{
			if(WINI_COLLECTION == type)
			{
				*repl = 0;		//no replication index for collections
				sprintf(szptr, "COLL");
			}
			else
			{
				*repl = atoi(((WINI::IDatasetNode*)child)->GetReplicantNumber());
				sprintf(szptr, "DATA");
			}

			szptr += 5;

			sprintf(szptr, "Y");
			szptr += 2;

			sprintf(szptr, "%s", child->GetName());
			szptr += strlen(child->GetName()) + 1;

			sprintf(szptr, "%s", child->GetPath());
			szptr += strlen(child->GetPath()) + 1;

			ptr = szptr;
			repl = (int*)ptr;
			szptr = (char*)ptr + sizeof(int);
		}
	}

	//no need to write in the final int and null char* values since they are are already set to 0 and NULL
	//(when we allocate the memory it's zeroed out);
	//szptr[0] = '\0';

	::GlobalUnlock(handle);

	gDS->CacheGlobalData(gInqClipboardFormat, handle);
	ghDragList = handle;
	gDS->DelayRenderData(CF_HDROP, &fe);
	gDS->SetClipboard();
}
#endif

void CInquisitorView::OnEditPaste() 
{
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() != 1)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int p = list.GetNextSelectedItem(POS);
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);
	if(tmpiobj->my_win_obj_type != irodsWinObj::IRODS_COLLECTION_TYPE)
		return;

	irodsWinCollection *pColl;
	pColl = (irodsWinCollection *)list.GetItemData(p);

	CInquisitorDoc* pDoc = GetDocument();
	pDoc->paste_buffered_irods_objs(pColl->fullPath);

#if 0
	FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC fe2= {gInqClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	COleDataObject odo;

	odo.AttachClipboard();

	CListCtrl& list = GetListCtrl();

	int count = list.GetSelectedCount();

	if(count > 1)
	{
		AfxMessageBox("You cannot select more than one collection.");
		return;
	}

	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* node = pDoc->GetCurrentNode();

	ASSERT(node != NULL);

	if(count != 0)
	{
		POSITION POS = list.GetFirstSelectedItemPosition();
		int pos = list.GetNextSelectedItem(POS);
		node = node->GetChild(m_nRedirector[pos]);
	}

	ASSERT(node != NULL);

	if(odo.IsDataAvailable(gInqClipboardFormat, &fe2))
	{
		HGLOBAL handle = odo.GetGlobalData(gInqClipboardFormat, &fe2);

		ASSERT(handle != NULL);

		void* ptr = ::GlobalLock(handle);
		int* repl = (int*)ptr;
		char* szptr = (char*)ptr + sizeof(int);
		int len;

		int type;
		bool cut;
		char* path;
		char* name;

		gCount = 0;

		do
		{
			szptr += strlen(szptr) + 1;
			ASSERT(szptr != NULL);
			szptr += strlen(szptr) + 1;
			ASSERT(szptr != NULL);
			szptr += strlen(szptr) + 1;
			ASSERT(szptr != NULL);
			szptr += strlen(szptr) + 1;
			szptr = szptr + sizeof(int);
			gCount++;
		} while(szptr[0] != '\0');

		szptr = (char*)ptr + sizeof(int);

		do
		{
			if('C' == szptr[0])
				type = WINI_COLLECTION;
			else
				type = WINI_DATASET;

			len = strlen(szptr);
			szptr += len + 1;
			ASSERT(szptr != NULL);

			if('Y' == szptr[0])
				cut = true;
			else
				cut = false;

			len = strlen(szptr);
			szptr += len + 1;
			ASSERT(szptr != NULL);

			name = szptr;

			len = strlen(szptr);
			szptr += len + 1;
			ASSERT(szptr != NULL);

			path = szptr;

			pDoc->Copy(node, type, cut, name, path, *repl);	//doc's copy func should copy these string values 'cause we'll clear the global memory

			len = strlen(szptr);
			szptr += len + 1;

			ptr = szptr;
			repl = (int*)ptr;
			szptr = (char*)ptr + sizeof(int);

		} while(szptr[0] != '\0');

		::GlobalUnlock(handle);
		::GlobalFree(handle);
	}
	else if(odo.IsDataAvailable(CF_HDROP, &fe))
	{
		HDROP handle = (HDROP)odo.GetGlobalData(CF_HDROP, &fe);

		ASSERT(handle != NULL);

		int count = ::DragQueryFile(handle, (UINT) -1, NULL, 0);

		if(count)
		{
			gCount = count;
			TCHAR szFile[MAX_PATH];
			int i;
			for(i = 0; i < count; i++)
			{
				::DragQueryFile(handle, i, szFile, sizeof(szFile)/sizeof(TCHAR));
				pDoc->Upload(node, szFile);
			}
		}

		::GlobalUnlock(handle);
		::GlobalFree(handle);
	}else
	{
		ASSERT(1);
	}
#endif
}


//
// UPDATEUI SECTION
//

//two ways - if nothing's selected check the current node
//if something's selected, check the children
#if 0
void CInquisitorView::OnUpdateMetadata(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();
	WINI::INode* parent;
	POSITION POS;
	int pos;
	WINI::INode* child;
	parent = GetDocument()->GetCurrentNode();

	if(NULL == parent)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(0 == count)
	{
		//nothing selected - check current node
		switch(parent->GetType())
		{
		case WINI_COLLECTION:
		case WINI_DATASET:
			return;
		default:
			pCmdUI->Enable(FALSE);
			return;
		}
	}else
	{
		POS = list.GetFirstSelectedItemPosition();
		int i;
		for(i = 0; i < count; i++)
		{
			pos = list.GetNextSelectedItem(POS);
			child = parent->GetChild(m_nRedirector[pos]);

			if(NULL == child)
				continue;

			switch(child->GetType())
			{
			case WINI_COLLECTION:
			case WINI_DATASET:
				break;
			default:
				pCmdUI->Enable(FALSE);
				return;
			}	
		}
	}
}
#endif

/*
void CInquisitorView::OnUpdateQuery(CCmdUI* pCmdUI) 
{
	CInquisitorDoc* pDoc = GetDocument();
	
	WINI::INode* Node = pDoc->GetCurrentNode();

	if(NULL == Node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(Node->GetType())
	{
	case WINI_COLLECTION:
	case WINI_QUERY:
		break;
	default:
		pCmdUI->Enable(FALSE);
	}
}
*/

void CInquisitorView::OnUpdateAccessCtrl(CCmdUI* pCmdUI) 
{
	CInquisitorDoc* pDoc = GetDocument();
#if 0
	WINI::INode* Node = pDoc->GetCurrentNode();

	if(NULL == Node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();

	if(0 == count)
	{
		switch(Node->GetType())
		{
		case WINI_DATASET:
		case WINI_COLLECTION:
			return;
		default:
			pCmdUI->Enable(FALSE);
			return;
		}
	}

	if(count != 1)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	POSITION POS = list.GetFirstSelectedItemPosition();
	int pos = list.GetNextSelectedItem(POS);
	pos = m_nRedirector[pos];
	Node = Node->GetChild(pos);

	if(NULL == Node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(Node->GetType())
	{
	case WINI_DATASET:
	case WINI_COLLECTION:
		return;
	default:
		pCmdUI->Enable(FALSE);
		return;
	}
#endif
}

void CInquisitorView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();

	if(list.GetSelectedCount() == 0)
	{
		pCmdUI->Enable(false);
		return;
	}

	pCmdUI->Enable(true);
}

#if 0
void CInquisitorView::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	return;

	int count = list.GetSelectedCount();

	if(0 == count)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	WINI::INode* parent = GetDocument()->GetCurrentNode();

	POSITION POS = list.GetFirstSelectedItemPosition();

	int pos;
	WINI::INode* child;
	int i;
	for(i = 0; i < count; i++)
	{
		pos = list.GetNextSelectedItem(POS);
		child = parent->GetChild(m_nRedirector[pos]);

		if(NULL == child)
			continue;

		switch(child->GetType())
		{
		case WINI_COLLECTION:
		case WINI_DATASET:
			break;
		default:
			pCmdUI->Enable(FALSE);
			return;
		}
	}
}
#endif

//note: code is written this way not to change pCmdUI->Enable
//unless absolutely have to. This eliminates on screen flicker
//in the button
void CInquisitorView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	set_menu_state_for_single_selected_folder(pCmdUI);
}

void CInquisitorView::OnUpdateNewCollection(CCmdUI* pCmdUI) 
{
	set_menu_state_for_single_selected_folder(pCmdUI);
}

void CInquisitorView::OnUpdateDelete(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() == 0)
	{
		pCmdUI->Enable(false);
		return;
	}
	pCmdUI->Enable(true);
}

void CInquisitorView::OnUpdateRename(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();

	if(list.GetSelectedCount() == 1)
		pCmdUI->Enable(true);
	else
		pCmdUI->Enable(false);
}

#if 0
void CInquisitorView::OnComment() 
{
	CListCtrl& list = GetListCtrl();
	UINT count = list.GetSelectedCount();

	WINI::INode* node;

	node = GetDocument()->GetCurrentNode();

	ASSERT(count <= 1);

	if(1 == count)
	{
		POSITION POS = list.GetFirstSelectedItemPosition();
		int pos = list.GetNextSelectedItem(POS);
		pos = m_nRedirector[pos];
		node = node->GetChild(pos);
	}

	ASSERT(NULL != node);
	ASSERT(WINI_DATASET == node->GetType());

	CGenericDialog myDlg("L");

	if(IDOK == myDlg.DoModal())
	{
		gCount = 1;
		GetDocument()->SetComment(node, myDlg.m_Edit.LockBuffer());
		myDlg.m_Edit.UnlockBuffer();
	}
}
#endif

/*
void CInquisitorView::OnUpdateComment(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();


	int count = list.GetSelectedCount();

	if(count > 1)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	WINI::INode* node;

	node = GetDocument()->GetCurrentNode();

	POSITION POS;
	int pos;

	if(1 == count)
	{
		POS = list.GetFirstSelectedItemPosition();
		pos = list.GetNextSelectedItem(POS);
		pos = m_nRedirector[pos];
		node = node->GetChild(pos);
	}
	
	if(NULL != node)
		if(WINI_DATASET == node->GetType())
			return;

	pCmdUI->Enable(FALSE);
}
*/

#if 0
void CInquisitorView::OnUpdateUpload(CCmdUI* pCmdUI) 
{
	WINI::INode* Node = GetDocument()->GetCurrentNode();

	if(NULL == Node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();

	if(0 == count)
	{
		switch(Node->GetType())
		{
		case WINI_DATASET:
		case WINI_COLLECTION:
			break;
		default:
			pCmdUI->Enable(FALSE);
			break;
		}
		return;
	}

	POSITION POS = list.GetFirstSelectedItemPosition();

	int pos;

	WINI::INode* child;

	while(POS)
	{
		pos = list.GetNextSelectedItem(POS);
		pos = m_nRedirector[pos];
		child = Node->GetChild(pos);

		if(NULL == child)	//to handle those cases where the child node gets deleted while updating
			continue;

		switch(child->GetType())
		{
		case WINI_COLLECTION:
		case WINI_DATASET:
			break;
		default:
			pCmdUI->Enable(FALSE);
			break;
		}
	}
}
#endif

#if 0
void CInquisitorView::OnUpdateUploadFolder(CCmdUI* pCmdUI) 
{
	WINI::INode* Node = GetDocument()->GetCurrentNode();

	if(NULL == Node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();

	if(0 == count)
	{
		switch(Node->GetType())
		{
		case WINI_DATASET:
		case WINI_COLLECTION:
			break;
		default:
			pCmdUI->Enable(FALSE);
			break;
		}
		return;
	}

	POSITION POS = list.GetFirstSelectedItemPosition();

	int pos;

	WINI::INode* child;

	while(POS)
	{
		pos = list.GetNextSelectedItem(POS);
		pos = m_nRedirector[pos];
		child = Node->GetChild(pos);

		if(NULL == child)	//to handle those cases where the child node gets deleted while updating
			continue;

		switch(child->GetType())
		{
		case WINI_COLLECTION:
		case WINI_DATASET:
			break;
		default:
			pCmdUI->Enable(FALSE);
			break;
		}
	}
}
#endif

void CInquisitorView::OnUpdateDownload(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() == 0)
	{
		pCmdUI->Enable(false);
		return;
	}
	pCmdUI->Enable(true);
}

/*
void CInquisitorView::OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pLVDispInfo = (LV_DISPINFO*)pNMHDR;

	CListCtrl& list = GetListCtrl();

	POSITION POS = list.GetFirstSelectedItemPosition();
	int pos = list.GetNextSelectedItem(POS);
	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetDocument();
	WINI::INode* node = pDoc->GetCurrentNode()->GetChild(m_nRedirector[pos]);
	
	if(node->GetType() == WINI_DATASET || node->GetType() == WINI_COLLECTION)
		*pResult = 0;
	else
		*pResult = 1;
}
*/

/*
void CInquisitorView::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pTVDispInfo = (LV_DISPINFO*)pNMHDR;
	LVITEM* tv = &pTVDispInfo->item;
	CListCtrl& list = GetListCtrl();

	if(0 == tv->pszText)
	{
		
		list.EditLabel(pTVDispInfo->item.iItem);
		*pResult = 0;
		return;
	}

	*pResult = 0;
	//if string is not acceptable, print a suitable dialog box, and then use this code to reset the dialog (just like explorer)
	//it looks like the string stays the edited value by default so there's no setting...
	if(0 == strcmp(tv->pszText, ""))
	{
		AfxMessageBox("That is an invalid name.", MB_OK);
		GetListCtrl().EditLabel(pTVDispInfo->item.iItem);
		*pResult = 0;	// returning this value tells it it's no good, but I guess you have to use EditLabel to reset it to original anyway
		return;
	}

	gCount = 1;

	CInquisitorDoc* pDoc = GetDocument();

	POSITION POS = list.GetFirstSelectedItemPosition();
	int pos = list.GetNextSelectedItem(POS);
	WINI::INode* node = pDoc->GetCurrentNode()->GetChild(m_nRedirector[pos]);

	pDoc->Rename(node, tv->pszText);
}
*/

/*
void CInquisitorView::OnNewMeta() 
{
	CMainFrame* ptr = ((CMainFrame*)AfxGetMainWnd());
	ptr->SetFocusMeta();
}
*/

#if 0
void CInquisitorView::OnUpdateNewMeta(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();
	return;

	int count = list.GetSelectedCount();

	if(count > 1)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	WINI::INode* node;

	node = GetDocument()->GetCurrentNode();

	POSITION POS;
	int pos;

	if(1 == count)
	{
		POS = list.GetFirstSelectedItemPosition();
		pos = list.GetNextSelectedItem(POS);
		pos = m_nRedirector[pos];
		node = node->GetChild(pos);
	}
	
	if(NULL != node)
		if(WINI_DATASET == node->GetType() || WINI_COLLECTION == node->GetType())
			return;

	pCmdUI->Enable(FALSE);
	
}
#endif

void CInquisitorView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();

	if(0 == count)
		return;

	//POSITION POS = list.GetFirstSelectedItemPosition();
	//int i = list.GetNextSelectedItem(POS);
	//int obj_type = list.GetItemData(i);

	CInquisitorDoc* pDoc = GetDocument();
	POSITION POS;
	int i;
	irodsWinObj *tmpiobj;

	CMenu menu;
	CMenu uploadSubMenu, metadataSubmenu;
	POINT pt;

	switch(count)
	{
	case 0:
		return;
	case 1:
		POS = list.GetFirstSelectedItemPosition();
		i = list.GetNextSelectedItem(POS);
		tmpiobj = (irodsWinObj *)list.GetItemData(i);

		GetCursorPos(&pt);
		menu.CreatePopupMenu();

		switch(tmpiobj->my_win_obj_type)
		{
		case irodsWinObj::IRODS_DATAOBJ_TYPE:
			metadataSubmenu.CreateMenu();
			metadataSubmenu.AppendMenu(MF_ENABLED, ID_EDIT_METADATA_SYSMETADATA, "System Metadata in iCAT ...");
			metadataSubmenu.AppendMenu(MF_ENABLED, ID_EDIT_METADATA_USERMETADATA, "User Metadata in iCAT ...");
			menu.AppendMenu(MF_POPUP|MF_STRING, (UINT)metadataSubmenu.m_hMenu, "Metadata");
			//menu.AppendMenu(MF_ENABLED, ID_EDIT_METADATA, "Metadata ...");
			menu.AppendMenu(MF_ENABLED, ID_DELETE, "Delete");
			menu.AppendMenu(MF_ENABLED, ID_RENAME, "Rename...");
			menu.AppendMenu(MF_ENABLED, ID_REPLICATE, "Replicate");
			menu.AppendMenu(MF_ENABLED, ID_DOWNLOAD, "Download...");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_COPYPATHTOCLIPBOARD, "Copy iRODS Path to Clipboard");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_COPY, "Copy");
			if((pDoc->colls_in_copy_buffer.GetSize()<=0)&&(pDoc->dataobjs_in_copy_buffer.GetSize()<=0))
				menu.AppendMenu(MF_DISABLED, ID_EDIT_PASTE, "Paste");
			else
				menu.AppendMenu(MF_ENABLED, ID_EDIT_PASTE, "Paste");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_SELECT_ALL, "Select All");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenuA(MF_ENABLED, ID_VIEW_REFRESH, "Refresh");
			break;
		case irodsWinObj::IRODS_COLLECTION_TYPE:
			uploadSubMenu.CreateMenu();
			uploadSubMenu.AppendMenu(MF_ENABLED, ID_UPLOAD_UPLOADFILES, "Upload files...");
			uploadSubMenu.AppendMenu(MF_ENABLED, ID_UPLOAD_UPLOADAFOLDER, "Upload a Folder");
			metadataSubmenu.CreateMenu();
			metadataSubmenu.AppendMenu(MF_DISABLED, ID_EDIT_METADATA_SYSMETADATA, "System Metadata in iCAT ...");
			metadataSubmenu.AppendMenu(MF_ENABLED, ID_EDIT_METADATA_USERMETADATA, "User Metadata iCAT ...");
			menu.AppendMenu(MF_POPUP|MF_STRING, (UINT)metadataSubmenu.m_hMenu, "Metadata");
			//menu.AppendMenu(MF_ENABLED, ID_EDIT_METADATA, "Metadata ...");
			menu.AppendMenu(MF_ENABLED, ID_DELETE, "Delete");
			menu.AppendMenu(MF_ENABLED, ID_RENAME, "Rename...");
			menu.AppendMenu(MF_ENABLED, ID_REPLICATE, "Replicate");
			menu.AppendMenu(MF_ENABLED, ID_DOWNLOAD, "Download...");
			menu.AppendMenu(MF_POPUP|MF_STRING, (UINT)uploadSubMenu.m_hMenu, "Upload");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_COPYPATHTOCLIPBOARD, "Copy iRODS Path to Clipboard");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_COPY, "Copy");
			if((pDoc->colls_in_copy_buffer.GetSize()<=0)&&(pDoc->dataobjs_in_copy_buffer.GetSize()<=0))
				menu.AppendMenu(MF_DISABLED, ID_EDIT_PASTE, "Paste");
			else
				menu.AppendMenu(MF_ENABLED, ID_EDIT_PASTE, "Paste");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_SELECT_ALL, "Select All");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenuA(MF_ENABLED, ID_VIEW_REFRESH, "Refresh");
			
			break;
		default:
			return;
		}
		break;

	default:
		GetCursorPos(&pt);
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_ENABLED, ID_DELETE, "Delete");
		menu.AppendMenu(MF_ENABLED, ID_REPLICATE, "Replicate");
		menu.AppendMenu(MF_ENABLED, ID_DOWNLOAD, "Download...");
		menu.AppendMenu(MF_SEPARATOR, 0, "");
		menu.AppendMenu(MF_ENABLED, ID_EDIT_COPY, "Copy");
		menu.AppendMenu(MF_SEPARATOR, 0, "");
		menu.AppendMenu(MF_ENABLED, ID_SELECT_ALL, "Select All");
		menu.AppendMenu(MF_SEPARATOR, 0, "");
		menu.AppendMenuA(MF_ENABLED, ID_VIEW_REFRESH, "Refresh");
		
		break;
	}

	menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y,this);
}

#if 0
void CInquisitorView::OnGetpath() 
{
	CListCtrl& list = GetListCtrl();

	int count = list.GetSelectedCount();

	if(count != 1)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int i = list.GetNextSelectedItem(POS);

	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* node = pDoc->GetCurrentNode();
	node = node->GetChild(m_nRedirector[i]);
		

		AfxMessageBox(node->GetPath());
}
#endif

void CInquisitorView::update_rlist(void)
{
	CListCtrl& list = this->GetListCtrl();
	//list.DeleteAllItems();
	rlist_remove_all(RLIST_CLEAN_ALL);

	DWORD dwStyle = GetWindowLong(list.m_hWnd, GWL_STYLE);

	CInquisitorDoc* pDoc = GetDocument();

	for(int i=0;i<pDoc->colls_in_list.GetSize();i++)
	{
		rlist_insert_one_coll(pDoc->colls_in_list[i]);
	}

	for(int i=0;i<pDoc->dataobjs_in_list.GetSize(); i++)
	{
		rlist_insert_one_dataobj(pDoc->dataobjs_in_list[i]);
	}
}

void CInquisitorView::rlist_remove_all(int CleanMode)
{
	CListCtrl& list = this->GetListCtrl();
	if(list.GetItemCount() <= 0)
		return;

	if((CleanMode == RLIST_CLEAN_SELECTED)&&(list.GetSelectedCount() <= 0))
		return;

	switch(CleanMode)
	{
	case RLIST_CLEAN_ALL:
		for(int i=0;i<list.GetItemCount();i++)
		{
			rlist_remove_one_itemdata(i);
		}
		list.DeleteAllItems();
		break;
	case RLIST_CLEAN_SELECTED:
		CArray<int, int> listbox_sels;
		POSITION POS = list.GetFirstSelectedItemPosition();
		if(POS == NULL)
			return;
		while(POS)
		{
			int t = list.GetNextSelectedItem(POS); 
			listbox_sels.Add(t);
		}
		// deleting the item from bottom up, which means in the decreasing order with selected pos.
		int n = listbox_sels.GetSize();
		for(int i=0;i<n;i++)
		{
			int t = listbox_sels[n-i-1];
			rlist_remove_one_itemdata(t);
			list.DeleteItem(t);
		}
		break;
	}
}

void CInquisitorView::rlist_remove_one_itemdata(int pos)
{
	CListCtrl& list = this->GetListCtrl();
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(pos);
	if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		irodsWinDataobj *tDataObj = (irodsWinDataobj *)list.GetItemData(pos);
		list.SetItemData(pos, (DWORD_PTR)NULL);
		delete tDataObj;
	}
	else if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		irodsWinCollection *tCollObj = (irodsWinCollection *)list.GetItemData(pos);
		list.SetItemData(pos, (DWORD_PTR)NULL);
		delete tCollObj;
	}
}

// purely insert item into the list
void CInquisitorView::rlist_insert_one_coll(irodsWinCollection & newColl)
{
	irodsWinCollection *tCollToAdd = new irodsWinCollection();
	newColl.CopyTo(*tCollToAdd);

	CListCtrl& list = this->GetListCtrl();
	int n = list.GetItemCount();
	int t = list.InsertItem(n, tCollToAdd->name, 2);
	list.SetItemData(t, (DWORD_PTR)tCollToAdd);
	tCollToAdd = NULL;
}

void CInquisitorView::rlist_insert_one_dataobj(irodsWinDataobj & newDataObj)
{
	CListCtrl& list = this->GetListCtrl();
	int n = list.GetItemCount();

	irodsWinDataobj *tDataobjToAdd = new irodsWinDataobj();
	newDataObj.CopyTo(*tDataobjToAdd);
	int pos = list.InsertItem(n, tDataobjToAdd->name, 1);
	list.SetItemData(pos, (DWORD_PTR)tDataobjToAdd);

	CString tmpstr;
	list.SetItemText(pos, REPORT_NAME, tDataobjToAdd->name);
	//tmpstr.Format("%lld",tDataobjToAdd->size);
	float ft = (float)tDataobjToAdd->size;
	DispFileSize(ft, tmpstr);
	list.SetItemText(pos, REPORT_SIZE, tmpstr);
	list.SetItemText(pos, REPORT_OWNR, tDataobjToAdd->owner);
	list.SetItemText(pos, REPORT_TIME, tDataobjToAdd->modifiedTime);
	tmpstr.Format("%d",tDataobjToAdd->replNum);
	list.SetItemText(pos, REPORT_REPL, tmpstr);
	tmpstr.Format("%d", tDataobjToAdd->replStatus);
	list.SetItemText(pos, REPORT_RPLS, tmpstr);
	list.SetItemText(pos, REPORT_RGRP, tDataobjToAdd->rescGroup);
	list.SetItemText(pos, REPORT_RESC, tDataobjToAdd->rescName);
	tDataobjToAdd = NULL;
}

void CInquisitorView::OnSelectAll()
{
	// TODO: Add your command handler code here
	CListCtrl& list = this->GetListCtrl();
	int n = -1;
	while((n=list.GetNextItem(n, LVNI_ALL)) > -1 )
	{
		list.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void CInquisitorView::OnUpdateSelectAll(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(true);
}

void CInquisitorView::OnEditCopypathtoclipboard()
{
	// TODO: Add your command handler code here
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();
	if(count != 1)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int i = list.GetNextSelectedItem(POS);

	CString path_to_copy;
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(i);
	if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		irodsWinDataobj *tData = (irodsWinDataobj *)list.GetItemData(i);
		path_to_copy = tData->parentCollectionFullPath + "/" + tData->name;
	}
	else if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_COLLECTION_TYPE)
	{
		irodsWinCollection *tColl = (irodsWinCollection *)list.GetItemData(i);
		path_to_copy = tColl->fullPath;
	}
	
	if(!OpenClipboard())
	{
		AfxMessageBox( "Cannot open the Clipboard" );
		return;
	}

	if( !EmptyClipboard())
	{
		AfxMessageBox( "Cannot empty the Clipboard" );
		return;
	}

	HANDLE hClipboardData = GlobalAlloc(GPTR, path_to_copy.GetLength()+1);
	LPTSTR lpszBuffer = (LPTSTR)GlobalLock(hClipboardData);
	_tcscpy(lpszBuffer, path_to_copy);
	GlobalUnlock(hClipboardData);
	SetClipboardData(CF_TEXT, hClipboardData);
	CloseClipboard();
}

void CInquisitorView::OnViewRefresh()
{
	// TODO: Add your command handler code here
	CInquisitorDoc* pDoc = GetDocument();
	pDoc->refresh_disp();
}

void CInquisitorView::OnUploadUploadafolder()
{
	// TODO: Add your command handler code here
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() != 1)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int p = list.GetNextSelectedItem(POS);
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);
	if(tmpiobj->my_win_obj_type != irodsWinObj::IRODS_COLLECTION_TYPE)
		return;

	irodsWinCollection *pColl;
	pColl = (irodsWinCollection *)list.GetItemData(p);

	CInquisitorDoc* pDoc = GetDocument();
	irodsWinCollection newColl;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->BeginWaitCursor();
	pDoc->upload_a_localfolder_into_coll(pColl->fullPath, newColl);
	frame->EndWaitCursor();
}

void CInquisitorView::set_menu_stat_for_single_selection(CCmdUI *pCmdUI)
{
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() == 1)
	{
		pCmdUI->Enable(true);
		return;
	}
	pCmdUI->Enable(false);
}

void CInquisitorView::set_menu_state_for_single_selected_folder(CCmdUI *pCmdUI)
{	
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() == 1)
	{
		POSITION POS = list.GetFirstSelectedItemPosition();
		int i = list.GetNextSelectedItem(POS);
		irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(i);
		if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_COLLECTION_TYPE)
		{
			pCmdUI->Enable(true);
			return;
		}
	}
	
	pCmdUI->Enable(false);
}

void CInquisitorView::set_menu_state_for_single_selected_file(CCmdUI *pCmdUI)
{	
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() == 1)
	{
		POSITION POS = list.GetFirstSelectedItemPosition();
		int i = list.GetNextSelectedItem(POS);
		irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(i);
		if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_DATAOBJ_TYPE)
		{
			pCmdUI->Enable(true);
			return;
		}
	}
	
	pCmdUI->Enable(false);
}

void CInquisitorView::OnUpdateUploadUploadafolder(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	set_menu_state_for_single_selected_folder(pCmdUI);
}

void CInquisitorView::OnUploadUploadfiles()
{
	// TODO: Add your command handler code here
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() != 1)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int p = list.GetNextSelectedItem(POS);
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);
	if(tmpiobj->my_win_obj_type != irodsWinObj::IRODS_COLLECTION_TYPE)
		return;

	irodsWinCollection *pColl;
	pColl = (irodsWinCollection *)list.GetItemData(p);

	CInquisitorDoc* pDoc = GetDocument();
	CArray<irodsWinDataobj, irodsWinDataobj> newDatas;

	pDoc->upload_localfiles_into_coll(pColl->fullPath, newDatas);
}

void CInquisitorView::OnUpdateUploadUploadfiles(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	set_menu_state_for_single_selected_folder(pCmdUI);
}

void CInquisitorView::OnUpdateEditCopypathtoclipboard(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	set_menu_stat_for_single_selection(pCmdUI);
}

void CInquisitorView::OnUpdateReplicate(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() == 0)
	{
		pCmdUI->Enable(false);
		return;
	}
	pCmdUI->Enable(true);
}

#if 0
void CInquisitorView::OnEditMetadata()
{
	// TODO: Add your command handler code here
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() != 1)
		return;

	irodsWinCollection *pColl;
	irodsWinDataobj *pDataObj;
	CString obj_name_wpath;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int p = list.GetNextSelectedItem(POS);
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);

	int obj_type = tmpiobj->my_win_obj_type;
	switch(obj_type)
	{
	case irodsWinObj::IRODS_COLLECTION_TYPE:
		pColl = (irodsWinCollection *)tmpiobj;
		obj_name_wpath = pColl->fullPath;
		break;
	case irodsWinObj::IRODS_DATAOBJ_TYPE:
		pDataObj = (irodsWinDataobj *)tmpiobj;
		obj_name_wpath = pDataObj->parentCollectionFullPath + "/" + pDataObj->name;
		break;
	default:
		return;
		break;
	}
	
	//CMetadataDlg *metaDlg = new CMetadataDlg(obj_name_wpath, obj_type, this);
	//metaDlg->Create(IDD_DIALOG_METADATA, this);
	//metaDlg->ShowWindow(SW_SHOW);

	// temporary
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->CreateMetadataDlg(obj_name_wpath, obj_type);
}
#endif

#if 0
void CInquisitorView::OnUpdateEditMetadata(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	//CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	//CString msg = "download and display the file...";
	//frame->statusbar_msg(msg);
	set_menu_stat_for_single_selection(pCmdUI);
	//msg = " ";
	//frame->statusbar_msg(msg);
}
#endif

void CInquisitorView::OnEditMetadataSystemmetadata()
{
	// TODO: Add your command handler code here
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() != 1)
		return;

	irodsWinDataobj *pDataObj;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int p = list.GetNextSelectedItem(POS);
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);

	
	CString objNameWithPath;
	if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_DATAOBJ_TYPE)
	{
		pDataObj = (irodsWinDataobj *)tmpiobj;
		objNameWithPath = pDataObj->parentCollectionFullPath + CString("/") + pDataObj->name;
	}
	else
	{
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc* pDoc = GetDocument();

	dataObjInp_t dataObjInp;
	memset(&dataObjInp, 0, sizeof (dataObjInp));
	rstrcpy (dataObjInp.objPath, (char *)LPCTSTR(objNameWithPath), MAX_NAME_LEN);
	rodsObjStat_t *rodsObjStatOut = NULL;
	int t = rcObjStat(pDoc->conn, &dataObjInp, &rodsObjStatOut);
	if(t < 0)
	{
		CString msgHead = CString("Query Object State error: ");
		pDoc->disp_err_msgbox(msgHead, t, this);
		return;
	}
	if((rodsObjStatOut->specColl != NULL)&&(rodsObjStatOut->specColl->collClass == MOUNTED_COLL))
	{
		AfxMessageBox("The metadata operation does not apply for mounted collection(s) or data in mounted collection(s).");
		return;
	}	

	iRODSObjSysMetadata objSysmeta;
	this->BeginWaitCursor();
	t = irodsWinGetObjSysMetadata(pDoc->conn, pDataObj->parentCollectionFullPath, pDataObj->name, objSysmeta);

	if(t < 0)
	{
		CString msgHead = CString("Query System Metadata error: ");
		pDoc->disp_err_msgbox(msgHead, t);
		this->EndWaitCursor();
		return;
	}

	// construct HTML page
	CString html_info = CString("<html><body bgcolor=#CCCC99><h1>System Metadata for Object: ");
	html_info += objSysmeta.parent_coll + CString("/") + objSysmeta.obj_name + CString("</h1><hr>");

	html_info += CString("<p><pre>");
	html_info += CString("iRODS Object Name: ") + objSysmeta.parent_coll + CString("/") + objSysmeta.obj_name + CString("\n");
	html_info += CString("iRODS Object id: ") + objSysmeta.obj_id + CString("\n");
	html_info += CString("Version: ") + objSysmeta.version + CString("\n");
	html_info += CString("Type: ") + + objSysmeta.type + CString("\n");
	html_info += CString("Size: ") + objSysmeta.size + CString("\n");
	html_info += CString("Zone: ") + objSysmeta.zone + CString("\n");
	html_info += CString("Owner: ") + objSysmeta.owner + CString("\n");
	html_info += CString("Status: ") + objSysmeta.status + CString("\n");
	html_info += CString("Checksum: ") + objSysmeta.checksum + CString("\n");
	html_info += CString("Expiry: ") + objSysmeta.expiry + CString("\n");
	html_info += CString("Comments: ") + objSysmeta.comments +CString("<hr>");

	int i;
	CString tmpstr;
	for(i=0;i<objSysmeta.repl_num.GetSize();i++)
	{
		tmpstr.Format("%d<br>", i+1);
		html_info += CString("<p>Replica #") + tmpstr;
		html_info += CString("<p><table border=2 cellpadding=2 frame=box>\n");

		html_info += CString("<tr><td>Replica Number</td> <td>") + objSysmeta.repl_num[i] + CString("</td></tr>\n");
		html_info += CString("<tr><td>Replica Status</td> <td>") + objSysmeta.repl_status[i] + CString("</td></tr>\n");
		html_info += CString("<tr><td>Storage Resouce Group</td> <td>") + objSysmeta.repl_resc_grp_name[i] + CString("</td></tr>\n");
		html_info += CString("<tr><td>Storage Resource</td> <td>") + objSysmeta.repl_resc_name[i] + CString("</td></tr>\n");
		html_info += CString("<tr><td>Storgae Host</td> <td>") + objSysmeta.repl_resc_host_name[i] + CString("</td></tr>\n");
		html_info += CString("<tr><td>File Path in Vault</td> <td>") + objSysmeta.repl_vault_path[i] + CString("</td></tr>\n");
		html_info += CString("<tr><td>Create Time</td> <td>") + objSysmeta.repl_create_time[i] + CString("</td></tr>\n");
		html_info += CString("<tr><td>Modify Time</td> <td>") + objSysmeta.repl_modify_time[i] + CString("</td></tr>\n");

		html_info += CString("</table></p><br>");
	}

	html_info += CString("</body></html>");

	CDialogWebDisp dlg(html_info, this);
	dlg.DoModal();
}

void CInquisitorView::OnUpdateEditMetadataSystemmetadata(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	set_menu_state_for_single_selected_file(pCmdUI);
}

void CInquisitorView::OnEditMetadataUsermetadata()
{
	// TODO: Add your command handler code here
	CListCtrl& list = GetListCtrl();
	if(list.GetSelectedCount() != 1)
		return;

	irodsWinCollection *pColl;
	irodsWinDataobj *pDataObj;
	CString obj_name_wpath;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int p = list.GetNextSelectedItem(POS);
	irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(p);

	int obj_type = tmpiobj->my_win_obj_type;
	switch(obj_type)
	{
	case irodsWinObj::IRODS_COLLECTION_TYPE:
		pColl = (irodsWinCollection *)tmpiobj;
		obj_name_wpath = pColl->fullPath;
		break;
	case irodsWinObj::IRODS_DATAOBJ_TYPE:
		pDataObj = (irodsWinDataobj *)tmpiobj;
		obj_name_wpath = pDataObj->parentCollectionFullPath + "/" + pDataObj->name;
		break;
	default:
		return;
		break;
	}
	
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->CreateMetadataDlg(obj_name_wpath, obj_type);
}

void CInquisitorView::OnUpdateEditMetadataUsermetadata(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	set_menu_stat_for_single_selection(pCmdUI);
}

DROPEFFECT CInquisitorView::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	// TODO: Add your specialized code here and/or call the base class
	drag_over_item_t = -1;

	if((dwKeyState&MK_CONTROL) == MK_CONTROL)
		return DROPEFFECT_COPY;
	else
		return DROPEFFECT_MOVE;
	//return CListView::OnDragEnter(pDataObject, dwKeyState, point);
}

DROPEFFECT CInquisitorView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	// TODO: Add your specialized code here and/or call the base class
	// This is the place to check if the item needs highlight
	CListCtrl& list = GetListCtrl();
	int t = list.HitTest(point);
	if(t >= 0)
	{
		if(t != drag_over_item_t)
		{
			if(drag_over_item_t >= 0)
			{
				list.SetItemState(drag_over_item_t, 0, LVIS_DROPHILITED);
			}
			irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(t);
			if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_COLLECTION_TYPE)
			{
				
				list.SetItemState(t, LVIS_DROPHILITED, LVIS_DROPHILITED);
				drag_over_item_t = t;
			}
			else
			{
				drag_over_item_t = -1;
			}
		}
	}
	else
	{
		drag_drop_dehilite();
	}

	if((dwKeyState&MK_CONTROL) == MK_CONTROL)
		return DROPEFFECT_COPY;
	else
		return DROPEFFECT_MOVE;
	//return CListView::OnDragOver(pDataObject, dwKeyState, point);
}

void CInquisitorView::drag_drop_dehilite()
{
	CListCtrl& list = GetListCtrl();
	if(drag_over_item_t >= 0)
	{
		list.SetItemState(drag_over_item_t, 0, LVIS_DROPHILITED);
		drag_over_item_t = -1;
	}
}

void CInquisitorView::OnDragLeave()
{
	// TODO: Add your specialized code here and/or call the base class
	CListCtrl& list = GetListCtrl();
	if(drag_over_item_t >= 0)
	{
		list.SetItemState(drag_over_item_t, 0, LVIS_DROPHILITED);
	}
	drag_over_item_t = -1;
	CListView::OnDragLeave();
}

BOOL CInquisitorView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	// TODO: Add your specialized code here and/or call the base class
	CListCtrl& list = GetListCtrl();
	int t = list.HitTest(point);
	CInquisitorDoc* pDoc = GetDocument();
	bool need_refresh = true;

	// default action to add files into selected collection in tree.
	CString drag_drop_parent_coll = pDoc->cur_selelected_coll_in_tree->fullPath;
	irodsWinCollection *pColl;
	if(t >= 0)
	{
		// check if the selected item is a collection. otherwise. go to next : to parent
		irodsWinObj *tmpiobj = (irodsWinObj *)list.GetItemData(t);
		if(tmpiobj->my_win_obj_type == irodsWinObj::IRODS_COLLECTION_TYPE)
		{
			need_refresh = false;
			pColl = (irodsWinCollection *)list.GetItemData(t);
			drag_drop_parent_coll = pColl->fullPath;
		}
	}

	if(pDataObject->IsDataAvailable(CF_HDROP))
	{
		// get a list of selected local files and folders frol drag drop and insert them into iRODS
		if(pDoc->dragdrop_upload_local_data(pDataObject, drag_drop_parent_coll))
		{
			if(need_refresh == false)
			{
				CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
				CLeftView *leftView = frame->GetLeftPane();
				leftView->SetSelectedNodeChildQueryBit(drag_drop_parent_coll, false);
			}
		}

		if(need_refresh)
		{
			pDoc->refresh_disp();
		}

		drag_drop_dehilite();
		return CListView::OnDrop(pDataObject, dropEffect, point);
	}
	
	// we need to copy local files into slected collection
	// if it is a collection, we 
	drag_drop_dehilite();
	return CListView::OnDrop(pDataObject, dropEffect, point);
}

#if 0
void CInquisitorView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// let base button down handled
	CListView::OnLButtonDown(nFlags, point);

	// TODO: Add your message handler code here and/or call default
	CListCtrl& list = GetListCtrl();

	int t;
	if((t=list.GetSelectedCount()) <= 0)
		return;

	ListSelectedItemsToCopyBuffer();

#if 0
	// From MIcrosoft's example, DDLIST.CPP.
	// http://support.microsoft.com/kb/135299
	// Create global memory for sharing dragdrop text
	HGLOBAL hgData=GlobalAlloc(GPTR,6);
	ASSERT(hgData!=NULL);

	strcpy((CHAR *)hgData, "irods");
	// Cache the data, and initiate DragDrop
	m_COleDataSource.CacheGlobalData(CF_TEXT, hgData);
#endif

	// from http://www.vckbase.com/english/code/clipboard/adv_copypaste.shtml.htm
	CSharedFile sf(GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT);
	CString stext = "irods";
	sf.Write(stext, stext.GetLength());
	HGLOBAL hMem = sf.Detach();
	if(!hMem) return;
	m_COleDataSource.CacheGlobalData(gIExpClipboardFormat, hMem);

	DROPEFFECT dropEffect = m_COleDataSource.DoDragDrop(DROPEFFECT_COPY|DROPEFFECT_MOVE,NULL);

	// We need to send WM_LBUTTONUP to control or else the selection rectangle
	// will "follow" the mouse (like when you hold the left mouse down and
	// scroll through a regular listbox). WM_LBUTTONUP was sent to window that
	// recieved the drag/drop, not the one that initiated it, so we simulate
	// an WM_LBUTTONUP to the initiating window.
	LPARAM lparam;
	// "Pack" lparam with x and y coordinates where lbuttondown originated
	lparam=point.y;
	lparam=lparam<<16;
	lparam &= point.x;

	SendMessage(WM_LBUTTONUP,0,lparam);

	// Clear the Data Source's cache
	m_COleDataSource.Empty();
}
#endif

void CInquisitorView::OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;

	CListCtrl& list = GetListCtrl();

	int t;
	if((t=list.GetSelectedCount()) <= 0)
		return;

	ListSelectedItemsToCopyBuffer();

#if 0
	// From MIcrosoft's example, DDLIST.CPP.
	// http://support.microsoft.com/kb/135299
	// Create global memory for sharing dragdrop text
	HGLOBAL hgData=GlobalAlloc(GPTR,6);
	ASSERT(hgData!=NULL);

	strcpy((CHAR *)hgData, "irods");
	// Cache the data, and initiate DragDrop
	m_COleDataSource.CacheGlobalData(CF_TEXT, hgData);
#endif

	// from http://www.vckbase.com/english/code/clipboard/adv_copypaste.shtml.htm
	CSharedFile sf(GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT);
	CString stext = "irods";
	sf.Write(stext, stext.GetLength());
	HGLOBAL hMem = sf.Detach();
	if(!hMem) return;
	m_COleDataSource.CacheGlobalData(gIExpClipboardFormat, hMem);

	DROPEFFECT dropEffect = m_COleDataSource.DoDragDrop(DROPEFFECT_COPY|DROPEFFECT_MOVE,NULL);

	// Clear the Data Source's cache
	m_COleDataSource.Empty();
}
