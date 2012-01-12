// InquisitorView.cpp : implementation of the CInquisitorView class

#include "stdafx.h"
#include "Inquisitor.h"
#include "InquisitorDoc.h"
#include "InquisitorView.h"
#include "winiObjects.h"
#include "winbase.h"
#include "GenericDialog.h"
#include "MainFrm.h"
#include "MetadataDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define REPORT_NAME 0
#define REPORT_SIZE 1
#define REPORT_OWNR 2
#define REPORT_TIME 3
#define REPORT_REPL 4
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
extern UINT gInqClipboardFormat;
extern CImageList gIconListLarge;
extern CImageList gIconListSmall;
extern CImageList gResourceIconListLarge;
extern CImageList gResourceIconListSmall;
extern CharlieSource* gDS;
extern std::vector<WINI::INode*> gNodeDragList;
extern unsigned int gOn;
extern HANDLE ghDragList;
/////////////////////////////////////////////////////////////////////////////
// CInquisitorView

IMPLEMENT_DYNCREATE(CInquisitorView, CListView)

BEGIN_MESSAGE_MAP(CInquisitorView, CListView)
	//{{AFX_MSG_MAP(CInquisitorView)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_COMMAND(ID_DELETE, OnDelete)
	ON_WM_CREATE()
	ON_COMMAND(ID_DOWNLOAD, OnDownload)
	ON_COMMAND(ID_UPLOAD, OnUpload)
	ON_COMMAND(ID_UPLOADFOLDER, OnUploadFolder)
	ON_COMMAND(ID_NEW_COLLECTION, OnNewCollection)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_OPEN, OnOpen)
	ON_COMMAND(ID_OPENTREE, OnOpenTree)
	ON_COMMAND(ID_REPLICATE, OnReplicate)
	ON_COMMAND(ID_ACCESS_CTRL, OnAccessControl)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndlabeledit)
	ON_COMMAND(ID_NEW_CONTAINER, OnNewContainer)
	ON_UPDATE_COMMAND_UI(ID_REPLICATE, OnUpdateReplicate)
	ON_COMMAND(ID_METADATA, OnMetadata)
	ON_UPDATE_COMMAND_UI(ID_METADATA, OnUpdateMetadata)
	ON_UPDATE_COMMAND_UI(ID_QUERY, OnUpdateQuery)
	ON_UPDATE_COMMAND_UI(ID_ACCESS_CTRL, OnUpdateAccessCtrl)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBegindrag)
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_NEW_COLLECTION, OnUpdateNewCollection)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDelete)
	ON_COMMAND(ID_RENAME, OnRename)
	ON_UPDATE_COMMAND_UI(ID_RENAME, OnUpdateRename)
	ON_COMMAND(ID_COMMENT, OnComment)
	ON_UPDATE_COMMAND_UI(ID_COMMENT, OnUpdateComment)
	ON_UPDATE_COMMAND_UI(ID_UPLOAD, OnUpdateUpload)
	ON_UPDATE_COMMAND_UI(ID_UPLOADFOLDER, OnUpdateUploadFolder)
	ON_UPDATE_COMMAND_UI(ID_DOWNLOAD, OnUpdateDownload)
	ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, OnBeginlabeledit)
	ON_COMMAND(ID_NEW_META, OnNewMeta)
	ON_UPDATE_COMMAND_UI(ID_NEW_META, OnUpdateNewMeta)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_COMMAND(ID_GETPATH, OnGetpath)
	//}}AFX_MSG_MAP
	
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

	cs.style |=  LVS_EDITLABELS;

	return TRUE;
}

void CInquisitorView::OnDraw(CDC* pDC)
{
	CInquisitorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CListCtrl& refCtrl = GetListCtrl();
	refCtrl.InsertItem(0, "Item!");		//note: this is boilerplate code - don't touch
}

void CInquisitorView::OnInitialUpdate()
{
	if(!m_bInitialized)
	{
		CListCtrl& list = GetListCtrl();

		list.SetImageList(&gIconListSmall,LVSIL_SMALL);
		list.SetImageList(&gIconListLarge,LVSIL_NORMAL);

		list.InsertColumn(REPORT_NAME,"Name", LVCFMT_LEFT,				REPORT_WIDTH_NAME);
		list.InsertColumn(REPORT_SIZE,"Size", LVCFMT_LEFT,				REPORT_WIDTH_SIZE);
		list.InsertColumn(REPORT_OWNR,"Owner", LVCFMT_LEFT,				REPORT_WIDTH_OWNR);
		list.InsertColumn(REPORT_TIME,"Modified Time", LVCFMT_LEFT,		REPORT_WIDTH_TIME);
		list.InsertColumn(REPORT_REPL,"Replicant", LVCFMT_LEFT,			REPORT_WIDTH_REPL);
		list.InsertColumn(REPORT_RPLS,"Repl. Status", LVCFMT_LEFT,		REPORT_WIDTH_RPLS);
		list.InsertColumn(REPORT_RGRP,"Resource Group", LVCFMT_LEFT,	REPORT_WIDTH_RGRP);
		list.InsertColumn(REPORT_RESC,"Resource", LVCFMT_LEFT,			REPORT_WIDTH_RESC);
		list.InsertColumn(REPORT_CSUM,"Checksum", LVCFMT_LEFT,			REPORT_WIDTH_CSUM);

		this->ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
		m_bInitialized = true;
	}
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
}

void CInquisitorView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	CListCtrl& list = GetListCtrl();

	int count = list.GetSelectedCount();

	if(count != 1)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int i = list.GetNextSelectedItem(POS);

	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* node = pDoc->GetCurrentNode();
	node = node->GetChild(m_nRedirector[i]);

	if(WINI_DATASET != node->GetType())
	{
		OnOpenTree();
		return;
	}

	OnOpen();
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

void CInquisitorView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
}

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

void CInquisitorView::OnRename() 
{



	CListCtrl& list = GetListCtrl();

	int count = list.GetSelectedCount();

	if(count > 1)
		return;

	gCount = 1;

	POSITION POS;
	int pos;

	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't Suspend Thread");

		return;
	}

	WINI::INode* node = pDoc->GetCurrentNode();

	POS = list.GetFirstSelectedItemPosition();

	static char buf[1024];

	if(count)
	{
		pos = list.GetNextSelectedItem(POS);
		node = node->GetChild(m_nRedirector[pos]);

		ASSERT(NULL != node);
		ASSERT(WINI_COLLECTION == node->GetType() || WINI_DATASET == node->GetType());
	}

	CGenericDialog myDlg("ll");

	char* new_name = NULL;

	if(IDOK == myDlg.DoModal())
	{
		new_name = myDlg.m_Edit.LockBuffer();
		pDoc->Rename(node, new_name);
		myDlg.m_Edit.UnlockBuffer();
	}

	if(!pDoc->ResumeWorkThread())
		MessageBox("Couldn't Resume Thread");

}

void CInquisitorView::OnDelete() 
{
	CListCtrl& list = GetListCtrl();

	int count = list.GetSelectedCount();

	gCount = count;

	POSITION POS;
	int pos;

	CInquisitorDoc* pDoc = GetDocument();

	if(IDNO == AfxMessageBox("Do you wish to delete the selected item(s)?", MB_YESNO))
		return;

	WINI::INode* parent = pDoc->GetCurrentNode();
	WINI::INode* child;

	POS = list.GetFirstSelectedItemPosition();

	static char buf[1024];

	while(POS)
	{
		pos = list.GetNextSelectedItem(POS);
		child = parent->GetChild(m_nRedirector[pos]);

		switch(child->GetType())
		{
		case WINI_COLLECTION:
		case WINI_DATASET:
			break;
		default:
			sprintf(buf, "%s is not an item which can be deleted.", child->GetName());
			AfxMessageBox(buf, MB_OK);
			continue;
		}
		pDoc->Delete(child);
	}
}

void CInquisitorView::OnDownload() 
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

	UINT count = list.GetSelectedCount();

	if(0 == count)
	{
		MessageBox("Please make a selection");
		return;
	}

	gCount = count;

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

	while(POS)
	{
		i = list.GetNextSelectedItem(POS);
		pDoc->Download(node->GetChild(m_nRedirector[i]), folder_path, false);
	}
}

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

void CInquisitorView::OnNewCollection()
{
	CGenericDialog myDlg("New Collection");

	WINI::INode *child, *current_node;

	int a, pos;
	char* szName = NULL;

	POSITION POS;
	CListCtrl& list = GetListCtrl();
	UINT count = list.GetSelectedCount();
	CInquisitorDoc* pDoc = GetDocument();
	current_node = pDoc->GetCurrentNode();

	if(0 == count)
	{
		//add new child to current collection
		if(WINI_COLLECTION != current_node->GetType())
		{
			AfxMessageBox("Collections can only be created within another collection.");
			return;
		}

		a = myDlg.DoModal();

		if(a == IDCANCEL)
			return;

		ASSERT(a == IDOK);

		szName = myDlg.m_Edit.LockBuffer();

		gCount = 1;

		pDoc->CreateNode(current_node, szName);

	}else
	{
		a = myDlg.DoModal();

		if(a == IDCANCEL)
			return;

		ASSERT(a == IDOK);

		szName = myDlg.m_Edit.LockBuffer();


		//add new child to all selected items (fail on non collections)
		POS = list.GetFirstSelectedItemPosition();

		gCount = count;

		while(POS)
		{
			pos = list.GetNextSelectedItem(POS);
			child = current_node->GetChild(m_nRedirector[pos]);
			if(WINI_COLLECTION != child->GetType())
			{
				AfxMessageBox("Collections can only be created within another collection.");
			}else
			{
				pDoc->CreateNode(child, szName);
			}
		}
	}

	myDlg.m_Edit.UnlockBuffer();
}

void CInquisitorView::OnReplicate()
{
	CListCtrl& list = GetListCtrl();

	UINT count = list.GetSelectedCount();

	POSITION POS = list.GetFirstSelectedItemPosition();
	int pos;

	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* node = pDoc->GetCurrentNode();
	WINI::INode* child;

	gCount = count;

	while(NULL != POS)
	{
		pos = list.GetNextSelectedItem(POS);
		child = node->GetChild(m_nRedirector[pos]);
		pDoc->Replicate(child);
	}	
}

void CInquisitorView::OnAccessControl() 
{
	CListCtrl& list = GetListCtrl();

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
}

void CInquisitorView::OnNewContainer() 
{
	CGenericDialog myDlg("New Container");

	WINI::INode *child, *current_node;

	int a, pos;
	char* szName = NULL;

	POSITION POS;
	CListCtrl& list = GetListCtrl();
	UINT count = list.GetSelectedCount();
	CInquisitorDoc* pDoc = GetDocument();
	current_node = pDoc->GetCurrentNode();

	if(0 == count)
	{
		//add new child to current container
		if(WINI_RESOURCE != current_node->GetType())
		{
			AfxMessageBox("Containers can only be created within a resource.");
			return;
		}

		a = myDlg.DoModal();

		if(a == IDCANCEL)
			return;

		ASSERT(a == IDOK);

		szName = myDlg.m_Edit.LockBuffer();

		gCount = 1;

		pDoc->CreateNode(current_node, szName);

	}else
	{
		a = myDlg.DoModal();

		if(a == IDCANCEL)
			return;

		ASSERT(a == IDOK);

		szName = myDlg.m_Edit.LockBuffer();

		//add new child to all selected items (fail on non containers)
		POS = list.GetFirstSelectedItemPosition();

		gCount = count;

		while(POS)
		{
			pos = list.GetNextSelectedItem(POS);
			child = current_node->GetChild(m_nRedirector[pos]);
			pDoc->CreateNode(child, szName);
		}
	}

	myDlg.m_Edit.UnlockBuffer();
}

void CInquisitorView::OnMetadata() 
{
	CMetadataDialog mydlg;
	mydlg.DoModal();
}

//
//	DRAG AND DROP / CLIPBOARD SECTION
//

//drop-target function: called when dnd operation first enters view's area.
//potential in future to call update and/or do some other visual information besides dragover?
DROPEFFECT CInquisitorView::OnDragEnter( COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	return OnDragOver(pDataObject, dwKeyState, point);
}

//drop-target function: workhorse function. can be called thousands of times over (as long as dnd
//operation is occuring in client area). Job is to evaluate and return drop-effect code identifying
//what drop operation is allowed at that coordinate point, if any. This can be affected by what is
//being dragged, what it is sitting over (collection, dataset, nothing, etc.), and keystate (pressing
//ctrl may change the operation from copy to move)
//assume that oledataobject contains all data of same type. that is, if HDROP(file) is available,
//then InqType isn't. Assume if file drop then drag coming from outside and if inqtype drag coming from
//inside. (Don't support dragging between instances!)
DROPEFFECT CInquisitorView::OnDragOver( COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	if(pDataObject->IsDataAvailable(gInqClipboardFormat))
		return InqDragOver(pDataObject, dwKeyState, point);

	if(pDataObject->IsDataAvailable(CF_HDROP))
		return FileDragOver(pDataObject, dwKeyState, point);

	return DROPEFFECT_NONE;
}

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

DROPEFFECT CInquisitorView::InqDragOver(COleDataObject* pDataObject, DWORD& dwKeyState, CPoint& point)
{
	CListCtrl& list = GetListCtrl();

	int test = list.HitTest(point);

	WINI::INode* node;

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

//drop-target function: called when a drag op moves out of the view - this function can be used
//to clean up after any visual things done, such as destroying a drag image created once the object
//was dragged into the view. We might need to use this function to download selected files if the
//drag originated from within inqview, and convert information on the clipboard from inqview type to
//file drop type, which points to the dl'ed files, now stored in temp directory.
void CInquisitorView::OnDragLeave()
{
		//g_bIsDragging = false;
}

//drop-target function: function is called when a selection is dropped onto this view.
//very important to accept only file and inq drop types. drop can originate from outside inq
//or inside (passing then, file or inq type respectively).

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
	
//drop-source function: called when a user drags one or more items.
//this function will always then create inq-types onto the clipboard.
//I think this is actually part of the non-ole API but hey, it works. I am sure there's
//no law mixing the two... It is better to have the OS decide what starts a drag op
//than myself hardcoding it into OnLButtonDown.
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


//general dnd function: with this function you can update the screen by repositioning a image, etc.
void CInquisitorView::OnMouseMove(UINT nFlags, CPoint point) 
{
	CListView::OnMouseMove(nFlags, point);
}

void CInquisitorView::OnEditCopy() 
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

			sprintf(szptr, "N");
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

void CInquisitorView::OnEditPaste() 
{
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

}

//
// UPDATEUI SECTION
//

void CInquisitorView::OnUpdateReplicate(CCmdUI* pCmdUI) 
{

	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();
	WINI::INode* parent = GetDocument()->GetCurrentNode();

	if(NULL == parent)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(0 == count)
	{
		switch(parent->GetType())
		{
		case WINI_DATASET:
		case WINI_COLLECTION:
		return;
		default:
			pCmdUI->Enable(FALSE);
			return;
		}
	}

	POSITION POS = list.GetFirstSelectedItemPosition();

	int i,pos;
	WINI::INode* child;

	for(i = 0; i < count; i++)
	{
		pos = list.GetNextSelectedItem(POS);
		child = parent->GetChild(m_nRedirector[pos]);

		//some operations modify the number of children (such as delete) and therefore
		//potentially invalidate the m_nRedirector. It is possible during an update to
		//have a selected item in the listview which no longer exists. We handle this by
		//ignoring null children.

		if(NULL == child)
			continue;

		if(WINI_DATASET != child->GetType() && WINI_COLLECTION != child->GetType())
		{
			pCmdUI->Enable(FALSE);
			break;
		}
	}
}

//two ways - if nothing's selected check the current node
//if something's selected, check the children
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

void CInquisitorView::OnUpdateAccessCtrl(CCmdUI* pCmdUI) 
{
	CInquisitorDoc* pDoc = GetDocument();
	
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
}

void CInquisitorView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();

	int count = list.GetSelectedCount();

	if(0 == count)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	WINI::INode* parent = GetDocument()->GetCurrentNode();

	POSITION POS = list.GetFirstSelectedItemPosition();

	int i,pos;
	WINI::INode* child;

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

void CInquisitorView::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();

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

//note: code is written this way not to change pCmdUI->Enable
//unless absolutely have to. This eliminates on screen flicker
//in the button
void CInquisitorView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	COleDataObject odo;

	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	int node_type = node->GetType();

	odo.AttachClipboard();

	if(odo.IsDataAvailable(CF_HDROP))
	{
		if(WINI_COLLECTION != node_type)
		{
			pCmdUI->Enable(FALSE);
		}
			
	}else if(odo.IsDataAvailable(gInqClipboardFormat))
	{
		switch(node_type)
		{
		case WINI_COLLECTION:
		case WINI_DATASET:
		case WINI_METADATA:
			break;
		default:
			pCmdUI->Enable(FALSE);
		}
	}else
	{
		pCmdUI->Enable(FALSE);
	}

	odo.Detach();
}

void CInquisitorView::OnUpdateNewCollection(CCmdUI* pCmdUI) 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();
	
	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	
	if(WINI_COLLECTION != node->GetType())
		pCmdUI->Enable(FALSE);
}

void CInquisitorView::OnUpdateDelete(CCmdUI* pCmdUI) 
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
		pCmdUI->Enable(FALSE);
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

void CInquisitorView::OnUpdateRename(CCmdUI* pCmdUI) 
{
	CInquisitorDoc* pDoc = GetDocument();
	
	WINI::INode* Node = pDoc->GetCurrentNode();

	if(NULL == Node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();
	int pos;
	POSITION POS;
	switch(count)
	{
	case 0:
		switch(Node->GetType())
		{
		case WINI_DATASET:
		case WINI_COLLECTION:
			break;
		default:
			pCmdUI->Enable(FALSE);
			break;
		}
		break;
	case 1:
		POS = list.GetFirstSelectedItemPosition();
		pos = list.GetNextSelectedItem(POS);
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
			break;
		default:
			pCmdUI->Enable(FALSE);
			break;
		}
		break;
	default:
		pCmdUI->Enable(FALSE);
		break;
	}
}

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

void CInquisitorView::OnUpdateDownload(CCmdUI* pCmdUI) 
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
	/*if string is not acceptable, print a suitable dialog box, and then use this code to reset the dialog (just like explorer)*/
	/*it looks like the string stays the edited value by default so there's no setting...*/
	if(0 == strcmp(tv->pszText, ""))
	{
		AfxMessageBox("That is an invalid name.", MB_OK);
		GetListCtrl().EditLabel(pTVDispInfo->item.iItem);
		*pResult = 0;	/*returning this value tells it it's no good, but I guess you have to use EditLabel to reset it to original anyway*/
		return;
	}

	gCount = 1;

	CInquisitorDoc* pDoc = GetDocument();

	POSITION POS = list.GetFirstSelectedItemPosition();
	int pos = list.GetNextSelectedItem(POS);
	WINI::INode* node = pDoc->GetCurrentNode()->GetChild(m_nRedirector[pos]);

	pDoc->Rename(node, tv->pszText);
}

void CInquisitorView::OnNewMeta() 
{
	CMainFrame* ptr = ((CMainFrame*)AfxGetMainWnd());
	ptr->SetFocusMeta();
}

void CInquisitorView::OnUpdateNewMeta(CCmdUI* pCmdUI) 
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
		if(WINI_DATASET == node->GetType() || WINI_COLLECTION == node->GetType())
			return;

	pCmdUI->Enable(FALSE);
	
}

void CInquisitorView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();

	if(0 == count)
		return;

	POSITION POS = list.GetFirstSelectedItemPosition();
	int i = list.GetNextSelectedItem(POS);
	CInquisitorDoc* pDoc = GetDocument();
	WINI::INode* node = pDoc->GetCurrentNode();
	node = node->GetChild(m_nRedirector[i]);

	CMenu menu;
	POINT pt;

	switch(count)
	{
	case 0:
		return;
	case 1:
		GetCursorPos(&pt);
		menu.CreatePopupMenu();

		switch(node->GetType())
		{
		case WINI_DATASET:
			menu.AppendMenu(MF_ENABLED, ID_OPENTREE, "Expose");
			menu.AppendMenu(MF_ENABLED, ID_OPEN, "Open");
			menu.AppendMenu(MF_ENABLED, ID_GETPATH, "Copy path to clipboard");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_ACCESS_CTRL, "Access Control...");
			menu.AppendMenu(MF_ENABLED, ID_DELETE, "Delete");
			menu.AppendMenu(MF_ENABLED, ID_RENAME, "Rename");
			menu.AppendMenu(MF_ENABLED, ID_REPLICATE, "Replicate");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_DOWNLOAD, "Download...");
			menu.AppendMenu(MF_ENABLED, ID_UPLOAD, "Upload Files...");
			menu.AppendMenu(MF_ENABLED, ID_UPLOADFOLDER, "Upload Folder...");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_COPY, "Copy");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_CUT, "Cut");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_PASTE, "Paste");
			break;
		case WINI_COLLECTION:
			menu.AppendMenu(MF_ENABLED, ID_OPENTREE, "Expose");
			menu.AppendMenu(MF_ENABLED, ID_GETPATH, "Copy path to clipboard");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_ACCESS_CTRL, "Access Control...");
			menu.AppendMenu(MF_ENABLED, ID_NEW_COLLECTION, "Create Subcollection");
			menu.AppendMenu(MF_ENABLED, ID_DELETE, "Delete");
			menu.AppendMenu(MF_ENABLED, ID_RENAME, "Rename");
			menu.AppendMenu(MF_ENABLED, ID_QUERY, "Query...");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_DOWNLOAD, "Download...");
			menu.AppendMenu(MF_ENABLED, ID_UPLOAD, "Upload Files...");
			menu.AppendMenu(MF_ENABLED, ID_UPLOADFOLDER, "Upload Folder...");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_COPY, "Copy");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_CUT, "Cut");
			menu.AppendMenu(MF_ENABLED, ID_EDIT_PASTE, "Paste");
			break;
		case WINI_RESOURCE:
			menu.AppendMenu(MF_ENABLED, ID_OPENTREE, "Expose");
			menu.AppendMenu(MF_SEPARATOR, 0, "");
			menu.AppendMenu(MF_ENABLED, ID_NEW_CONTAINER, "Create Container");
			break;
		default:
			return;
		}
		break;
	default:
		if(WINI_COLLECTION != node->GetType())
			return;

		GetCursorPos(&pt);
		menu.CreatePopupMenu();

		menu.AppendMenu(MF_ENABLED, ID_DELETE, "Delete");
		menu.AppendMenu(MF_SEPARATOR, 0, "");
		menu.AppendMenu(MF_ENABLED, ID_DOWNLOAD, "Download...");
		menu.AppendMenu(MF_ENABLED, ID_UPLOAD, "Upload Files...");
		menu.AppendMenu(MF_ENABLED, ID_UPLOAD, "Upload Folder...");
		menu.AppendMenu(MF_SEPARATOR, 0, "");
		menu.AppendMenu(MF_ENABLED, ID_EDIT_COPY, "Copy");
		menu.AppendMenu(MF_ENABLED, ID_EDIT_CUT, "Cut");
		menu.AppendMenu(MF_ENABLED, ID_EDIT_PASTE, "Paste");
		break;
	}

	menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y,this);
}

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
