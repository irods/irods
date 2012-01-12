#include "stdafx.h"
#include "Inquisitor.h"
#include "InquisitorDoc.h"
#include "LeftView.h"
#include "MainFrm.h"
#include "winiObjects.h"
#include "GenericDialog.h"
#include "irodsWinCollection.h"
#include "irodsWinUtils.h"
//#include "rodsLog.h"
#include "MetadataDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CMutex gCS;
extern int gCount;
extern bool g_bIsDragging;
extern bool g_bDeleteOriginalPostCopy;
//extern UINT gInqClipboardFormat;
extern UINT gIExpClipboardFormat;
int image_reverse_map[];
extern CImageList gIconListSmall;

extern std::vector<WINI::INode*> gNodeDragList;
extern HANDLE ghDragList;
//extern CharlieSource* gDS;
extern bool gCaptureOn;
extern unsigned int gOn;
extern bool gbExiting;

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
	//{{AFX_MSG_MAP(CLeftView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_COMMAND(ID_DELETE, OnDelete)
	ON_WM_CREATE()
	ON_COMMAND(ID_DOWNLOAD, OnDownload)
	ON_COMMAND(ID_ACCESS_CTRL, OnAccessCtrl)
	//ON_COMMAND(ID_UPLOAD, OnUpload)
	//ON_COMMAND(ID_UPLOADFOLDER, OnUploadFolder)
	ON_COMMAND(ID_NEW_COLLECTION, OnNewCollection)
	ON_COMMAND(ID_NEW_CONTAINER, OnNewContainer)
	ON_COMMAND(ID_RENAME, OnRename)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	/*
	ON_UPDATE_COMMAND_UI(ID_VIEW_LARGEICON, OnUpdateViewLargeicon)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LIST, OnUpdateViewList)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SMALLICON, OnUpdateViewSmallicon)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DETAILS, OnUpdateViewDetails)
	ON_UPDATE_COMMAND_UI(ID_UPLOAD, OnUpdateUpload)
	ON_UPDATE_COMMAND_UI(ID_UPLOADFOLDER, OnUpdateUploadFolder)
	ON_UPDATE_COMMAND_UI(ID_DOWNLOAD, OnUpdateDownload)
	ON_UPDATE_COMMAND_UI(ID_NEW_COLLECTION, OnUpdateNewCollection)
	ON_UPDATE_COMMAND_UI(ID_ACCESS_CTRL, OnUpdateAccessCtrl)
	ON_UPDATE_COMMAND_UI(ID_METADATA, OnUpdateMetadata)
	ON_UPDATE_COMMAND_UI(ID_SYNCHRONIZE, OnUpdateSynchronize)
	ON_UPDATE_COMMAND_UI(ID_NEW_CONTAINER, OnUpdateNewContainer)
	
	
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)O
	
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDelete)
	ON_UPDATE_COMMAND_UI(ID_RENAME, OnUpdateRename)
	ON_UPDATE_COMMAND_UI(ID_NEW_META, OnUpdateNewMeta)
	*/

	
	ON_COMMAND(ID_REPLICATE, OnReplicate)
	ON_COMMAND(ID_METADATA, OnMetadata)
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	//ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBegindrag)
	
	//ON_COMMAND(ID_COMMENT, OnComment)
	//ON_UPDATE_COMMAND_UI(ID_COMMENT, OnUpdateComment)
	//ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginlabeledit)
	//ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndlabeledit)
	//ON_COMMAND(ID_NEW_META, OnNewMeta)
	
	//}}AFX_MSG_MAP


	ON_COMMAND(ID_UPLOAD_UPLOADAFOLDER, &CLeftView::OnUploadUploadafolder)
	ON_COMMAND(ID_UPLOAD_UPLOADFILES, &CLeftView::OnUploadUploadfiles)
	ON_UPDATE_COMMAND_UI(ID_SELECT_ALL, &CLeftView::OnUpdateSelectAll)
	ON_COMMAND(ID_EDIT_COPYPATHTOCLIPBOARD, &CLeftView::OnEditCopypathtoclipboard)
	ON_COMMAND(ID_VIEW_REFRESH, &CLeftView::OnViewRefresh)
	ON_NOTIFY_REFLECT(NM_RCLICK, &CLeftView::OnNMRclick)
	//ON_COMMAND(ID_EDIT_METADATA, &CLeftView::OnEditMetadata)
	ON_UPDATE_COMMAND_UI(ID_EDIT_METADATA_SYSMETADATA, &CLeftView::OnUpdateEditMetadataSystemmetadata)
	ON_COMMAND(ID_EDIT_METADATA_USERMETADATA, &CLeftView::OnEditMetadataUsermetadata)
END_MESSAGE_MAP()
//ON_REGISTERED_MESSAGE(msgMyMsg, OnDelete)

CLeftView::CLeftView()
{
#if 0
	m_bDontChange = false;
	m_bPossibleDrag = false;
	m_ReverseMap.InitHashTable(181);
	m_RZoneToCZoneMap.InitHashTable(17);
	m_CZoneToRZoneMap.InitHashTable(17);
#endif
	m_drag_over_item_node = NULL;
}

CLeftView::~CLeftView()
{
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	if(!CTreeView::PreCreateWindow(cs))
		return FALSE;

	//cs.style |= TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_EDITLABELS;
	cs.style |= TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;

	return TRUE;
}

void CLeftView::OnDraw(CDC* pDC)
{
	CInquisitorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

void CLeftView::OnInitialUpdate()
{
	//This implementation keeps default from calling OnUpdate function.
	int foo;
	foo = 1;
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->FooFighter();

	CTreeCtrl& tree = GetTreeCtrl();
	tree.SetImageList(&gIconListSmall,TVSIL_NORMAL);
}

#if 0
LRESULT CLeftView::ConnectedUpdate(WPARAM wParam, LPARAM lParam)
{
	WINI::StatusCode status((int)wParam);
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc* pDoc = GetDocument();

	static char buf[1024];

	if(!status.isOk())
	{
		//take care of this if(SRB_ERROR_NO_HOME_COLLECTION != status)
		if(0 != status)
		{
			sprintf(buf, "Connection failed: %s", status.GetError());
			frame->SetStatusMessage(buf);
			//frame->OnStopAnimation(NULL, NULL);
			//pDoc->OnFailedConnection();
			return FALSE;
		}
	}

	//pDoc->OnCompletedConnection();

	CTreeCtrl& tree = GetTreeCtrl();

	//clear tree (SDI document reuses document and views)
	m_ReverseMap.RemoveAll();
	tree.DeleteAllItems();
	tree.SetImageList(&gIconListSmall,TVSIL_NORMAL);

	HTREEITEM h1, h2;

	WINI::INode* catalog = pDoc->GetMCAT();
	int count = catalog->CountChildren();

	for(int i = 0; i < count; i++)
	{
		WINI::INode* node = catalog->GetChild(i);
		ASSERT(node->GetType() == WINI_ZONE);
		h1 = tree.InsertItem(node->GetName(), 38, 59);
		tree.SetItemData(h1, (DWORD_PTR)node);
		m_ReverseMap.SetAt(node, h1);
		
	}


	WINI::INode* myhome = pDoc->GetHome();

	if(myhome)
	{
		m_ReverseMap.Lookup(myhome, h2);
		tree.Select(h2, TVGN_CARET);
	};

	pDoc->UpdateAllViews(this);

	frame->PostMessage(msgStopAnimation, NULL, NULL);
	frame->PostMessage(msgStatusLine, NULL, 0);
	frame->PostMessage(msgRefillResourceList, NULL, NULL);

	return TRUE;
}
#endif

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CInquisitorDoc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CInquisitorDoc)));
	return (CInquisitorDoc*)m_pDocument;
}
#endif //_DEBUG

#if 0
//save it fr the purpose of examing image index
void CLeftView::GetImage(WINI::INode* node, int &image, int& selected_image)
{
	const char* szptr;
	WINI::IResourceNode* resource;
	int value = node->GetType();
	

	if(WINI_SET == value)
	{
		value = ((WINI::ISetNode*)node)->GetSetType();
		if(WINI_RESOURCE == value)
			szptr = "";
	}else if(value == WINI_RESOURCE)
	{
		resource = (WINI::IResourceNode*)node;
		szptr = resource->GetResourceType();
	}

	switch(value) 
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
		if(0 == strcmp("unix file system", szptr))
			image = 14; 
		else if(0 == strcmp("hpss file system", szptr))
			image = 15; 
		else if(0 == strcmp("oracle dblobj database", szptr))
			image = 13; 
		else 
			image = 7; 
		break; 
	case WINI_QUERY: 
		image = 5; 
		break; 
	case WINI_DOMAIN: 
		image = 10; 
		break; 
	case WINI_USER: 
		image = 9; 
		break; 
	case WINI_SET:
		ASSERT(value != WINI_SET);
		break; 
	case WINI_ZONE: 
		image = 16; 
		break; 
	default: 
		image = 0; 
		break; 
	} 
	 
	selected_image = image + 42; 

	if(node->isOpen()) 
		image += 21;
}
#endif

void CLeftView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// BING: IT IS DONE.
	//OnSelChanged is called whenever the selection is changed - whether internally or by the user.
	//m_bDontChange is toggled when we internally
	//change the selection.
	if(gbExiting)
		return;

	int t;

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
	{
		selected_node = NULL;
		return;
	}
	selected_node = hSelNode;

	CInquisitorDoc* pDoc = GetDocument();

	irodsWinCollection *coll = (irodsWinCollection *)tree.GetItemData(hSelNode);
	pDoc->cur_selelected_coll_in_tree = coll;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString msg = "getting the content of collection " + coll->fullPath;
	frame->statusbar_msg(msg);
	frame->BeginWaitCursor();

	if(coll->queried)
	{
		// copy the children into document and call update list.
		pDoc->colls_in_list.RemoveAll();
		pDoc->dataobjs_in_list.RemoveAll();

		for(int i=0;i<coll->childCollections.size();i++)
		{
			pDoc->colls_in_list.Add(coll->childCollections[i]);
		}
		for(int i=0;i<coll->childDataObjs.size();i++)
		{
			pDoc->dataobjs_in_list.Add(coll->childDataObjs[i]);
		}
		pDoc->update_rlist();
		frame->statusbar_clear();
		msg.Format("sub-collections: %d, files: %d", pDoc->colls_in_list.GetSize(), pDoc->dataobjs_in_list.GetSize());
		frame->statusbar_msg2(msg);
		frame->EndWaitCursor();
		return;
	}

	refresh_disp_with_a_node_in_tree(hSelNode);
}

void CLeftView::OnBack()
{
#if 0
	CInquisitorDoc* pDoc = GetDocument();
	pDoc->OnBack();

	pDoc->UpdateAllViews(NULL);
#endif
}

void CLeftView::OnForward()
{
#if 0
	CInquisitorDoc* pDoc = GetDocument();
	pDoc->OnForward();
	pDoc->UpdateAllViews(NULL);
#endif

}

void CLeftView::OnAccessCtrl()
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	CInquisitorDoc* pDoc = GetDocument();
	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	pDoc->SetAccess(iSelColl->fullPath, irodsWinObj::IRODS_COLLECTION_TYPE);
}

/*
void CLeftView::UpdateDialogBars(WINI::INode* child_node)
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();

	frame->ClearStack(false);

	WINI::INode* ptr = child_node;

	int i = 0;
	char* szptr;

	int type = ptr->GetType();

	while(type != WINI_SET && type != WINI_ZONE)
	{
		szptr = (char*)ptr->GetName();
		frame->Push(szptr);
		i++;
		ptr = ptr->GetParent();
		if(ptr)
			type = ptr->GetType();
		else
			type = WINI_SET;	//hack makes WINI_SET test for NULL as well.
	}

	frame->SetStack(i-1);
}
*/

void CLeftView::OnRename() 
{
	// BING: it is done!
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	CString NewCollName;
	CGenericDialog myDlg("Change Collection Name");
	if(myDlg.DoModal() == IDOK)
	{
		NewCollName = myDlg.m_Edit;
		// validate the string
		if(NewCollName.Find("/") >= 0)
		{
			AfxMessageBox("A collection name cannot contain '/' character(s)!");
			return;
		}
	}
	else
	{
		return;
	}

	HTREEITEM hParentOfSelNode = tree.GetParentItem(hSelNode);
	if(hParentOfSelNode == NULL)
	{
		AfxMessageBox("An unknow error occured!");
		return;
	}

	irodsWinCollection *iParentColl = (irodsWinCollection *)tree.GetItemData(hParentOfSelNode);
	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	CString preCollNameWithPath = iSelColl->fullPath;
	CString newCollNameWithPath = iParentColl->fullPath + "/" + NewCollName;

	CInquisitorDoc* pDoc = GetDocument();
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->BeginWaitCursor();

	int t = irodsWinRenameOneCollection(pDoc->conn, preCollNameWithPath, newCollNameWithPath);
	if(t < 0)
	{
		CString msgHead = CString("Rename Coll error for ") + iSelColl->fullPath;
		pDoc->disp_err_msgbox(msgHead, t);
		frame->EndWaitCursor();
		return;
	}

	// change info in the sel node
	iSelColl->fullPath = newCollNameWithPath;
	iSelColl->name = NewCollName;

	// if renaming collection succeeded, change info of the selected one in the parent.
	for(int i=0;i<iParentColl->childCollections.size();i++)
	{
		if(iParentColl->childCollections[i].fullPath == preCollNameWithPath)
		{
			iParentColl->childCollections[i].fullPath = newCollNameWithPath;
			iParentColl->childCollections[i].name = NewCollName;
			break;
		}
	}

	// deleting all children of selected node. 
	delete_child_nodes(hSelNode);
	// refresh the labe in the selected node
	tree.SetItemText(hSelNode, NewCollName);
	refresh_disp_with_a_node_in_tree(hSelNode);

	frame->EndWaitCursor();
}

void CLeftView::OnDelete() 
{
	// BING: IT IS DONE.
	// deleting the selected node (irods collection) in tree.
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	if(AfxMessageBox("This will delete the selected collection and its contents recursively. Are you sure?", MB_YESNO) != IDYES)
	{
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc* pDoc = GetDocument();
	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	CString SelCollWholePath = iSelColl->fullPath;

	frame->BeginWaitCursor();
	int t = irodsWinDeleteOneCollection(pDoc->conn, iSelColl->fullPath, pDoc->delete_obj_permanently, true);
	if(t < 0)
	{
		CString msgHead = CString("mkColl() error for ") + iSelColl->fullPath;
		pDoc->disp_err_msgbox(msgHead, t);
		frame->EndWaitCursor();
		return;
	}
	
	// remove the selected node from tree
	HTREEITEM hParentOfSelNode = tree.GetParentItem(hSelNode);
	if(hParentOfSelNode == NULL)
	{
		AfxMessageBox("An unknow error occured!");
		frame->EndWaitCursor();
		return;
	}
	
	// need to remove the hSelNode from child collection of hPreNode
	irodsWinCollection *tColl = (irodsWinCollection *)tree.GetItemData(hParentOfSelNode);
	for(int i=0;i<tColl->childCollections.size();i++)
	{
		if(SelCollWholePath == tColl->childCollections[i].fullPath)
		{
			tColl->childCollections.erase(tColl->childCollections.begin()+i);
			break;
		}
	}

	this->delete_one_node(hSelNode);
	frame->EndWaitCursor();
}

int CLeftView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if(!m_dropTarget.Register(this))
		MessageBox("couldn't register LView");

	if (CTreeView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CLeftView::OnDownload() 
{
	//BING: it is done!
	CInquisitorDoc* pDoc = GetDocument();
	
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;
	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	CString selectedLocalDir = "";
	pDoc->SelectLocalFolder(selectedLocalDir);
	if(selectedLocalDir.GetLength() == 0)
		return;

	// check if the dir alread exist.
	CString pname = "";
	irodsWinUnixPathGetName(iSelColl->fullPath, pname);
	
	CString NewDirName = selectedLocalDir + "/" + pname;
	// if the fird exists, need to get confirmation from user.
	struct __stat64 mystat;
	if(_stat64((char *)LPCTSTR(NewDirName), &mystat) == 0)
	{
		if(AfxMessageBox("The local folder already exists. Override?", MB_YESNO|MB_ICONQUESTION) != IDYES)
			return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();

	frame->StartProgressBar();
	frame->BeginWaitCursor();

	int recusive = 1;
	int forceful = 1;

	int t = irodsWinDownloadOneObject(pDoc->conn, iSelColl->fullPath, selectedLocalDir,
					recusive, forceful);

	if(t != 0)
	{
		CString msgHead = CString("Download error for ") + iSelColl->fullPath;
		pDoc->disp_err_msgbox(msgHead, t);
	}

	CString msg = "Download Collection finished.";
	frame->statusbar_msg(msg);
	frame->EndWaitCursor();
	frame->EndProgressBar();
}

#if 0
LRESULT CLeftView::OnRefresh(WPARAM wParam, LPARAM lParam)
{
	//this should actually refresh the INode tree instead of just
	//updating all the views

	CInquisitorDoc* pDoc = GetDocument();

	pDoc->UpdateAllViews(NULL, lParam);

	return 0;
}
#endif
#if 0
WINI::StatusCode CLeftView::AddMetadataAttribute(const char* attribute)
{
	CInquisitorDoc* pDoc = GetDocument();

	return pDoc->AddMetadataAttribute(attribute);
}


WINI::StatusCode CLeftView::SetMetadataValue(const char* attribute, const char* value)
{
	CInquisitorDoc* pDoc = GetDocument();

	return pDoc->SetMetadataValue(attribute, value);
}
#endif

#if 0
void CLeftView::OnUpload() 
{
	CInquisitorDoc* pDoc = GetDocument();
	CFileDialog FileDlg(TRUE, NULL, NULL,  OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST, NULL, NULL);

	FileDlg.m_ofn.lpstrTitle = "Upload";
	int t = FileDlg.DoModal();


	WINI::INode* node = pDoc->GetCurrentNode();

	POSITION POS;
	int st;
	CString FileName;


	//CFileDialog FileDlg(TRUE, NULL, NULL,OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NODEREFERENCELINKS , NULL, NULL);

	CFileDialog FileDlg(TRUE, NULL, NULL,  OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST, NULL, NULL);

	//FileDlg.m_ofn.Flags &= ~(OFN_EXPLORER | OFN_SHOWHELP);

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
	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't suspend work thread");

		return;
	}

	WINI::INode* node = pDoc->GetCurrentNode();

	POSITION POS;
	int st;
	CString FileName;


	CFileDialog FileDlg(TRUE, NULL, NULL,  OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST, NULL, NULL);


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

	if(!pDoc->ResumeWorkThread())
	{
		MessageBox("Couldn't Resume Thread");
	}
#endif

/*
void CLeftView::OnUploadFolder() 
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

	gCount = 1;

	FillMemory((void*)&bwinfo,sizeof(bwinfo),0);
	bwinfo.hwndOwner=NULL;
	bwinfo.pidlRoot=NULL;
	bwinfo.pszDisplayName=(char*) buffer;
	bwinfo.lpszTitle="Place downloads in folder:";
	bwinfo.ulFlags=0;
	itemids = SHBrowseForFolder(&bwinfo);
	if(itemids == NULL)
	{
		return;
	}

	folder_path = new char[MAX_PATH];
	SHGetPathFromIDList(itemids,(char*) folder_path);

	shallocator;
	SHGetMalloc(&shallocator);
	shallocator->Free((void*) itemids);
	shallocator->Release();

	pDoc = GetDocument();

	node = pDoc->GetCurrentNode();

	pDoc->Upload(node, folder_path);

}
*/

void CLeftView::OnNewCollection()
{
	// BING: IT IS DONE.
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;
	
	CInquisitorDoc* pDoc = GetDocument();

	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	pDoc->cur_selelected_coll_in_tree = iSelColl;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	
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

	frame->BeginWaitCursor();

	// add code to create new collection
	CString newCollectionWholePath = iSelColl->fullPath + "/" + newCollectionName;
	int t = mkColl(pDoc->conn, (char *)LPCTSTR(newCollectionWholePath));
	if(t < 0)
	{
		CString msgHead = CString("mkColl() error for ") +  newCollectionWholePath;
		pDoc->disp_err_msgbox(msgHead, t);
		frame->EndWaitCursor();
		return;
	}
	// add a child coll node in tree and in the list as well.
	irodsWinCollection *pTmpColl = new irodsWinCollection();
	pTmpColl->name = newCollectionName;
	pTmpColl->fullPath = newCollectionWholePath;
	pTmpColl->queried = false;
	// BBB need to add other info for the coll
	HTREEITEM tCurrent = tree.InsertItem(pTmpColl->name, 23, 44, hSelNode);
	tree.SetItemData(tCurrent, (DWORD_PTR)pTmpColl);
	pTmpColl = NULL;

	irodsWinCollection tmpColl;
	tmpColl.name = newCollectionName;
	tmpColl.fullPath = newCollectionWholePath;
	tmpColl.queried = false;
	iSelColl->childCollections.push_back(tmpColl);
	pDoc->insert_one_coll_in_rlist(tmpColl);

	frame->EndWaitCursor();
}

void CLeftView::OnNewContainer() 
{
	CInquisitorDoc* pDoc = GetDocument();
#if 0
	WINI::INode* current_node = pDoc->GetCurrentNode();

	if(WINI_RESOURCE != current_node->GetType())
	{
		AfxMessageBox("Containers can only be created within a resource.");

		return;
	}

	CGenericDialog myDlg("New Container");

	int a = myDlg.DoModal();

	if(a == IDCANCEL)
	{
	
		return;
	}

	if(a != IDOK)
		AfxMessageBox("Dialog Error!");


	char* szName = myDlg.m_Edit.LockBuffer();

	//add new child to current collection
	gCount = 1;

	pDoc->CreateNode(current_node, szName);

	myDlg.m_Edit.UnlockBuffer();
#endif
}

void CLeftView::OnReplicate() 
{
	CInquisitorDoc* pDoc = GetDocument();

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString irodsResc = frame->get_default_resc();
	if(irodsResc.GetLength() == 0)
	{
		AfxMessageBox("Please select a deafult resource for this operation!");
		return;
	}

	if(AfxMessageBox("This will replicate the selected collection and its contents recursively into default resource. Are you sure?", MB_YESNO) != IDYES)
	{
		return;
	}

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	irodsWinCollection *SelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	CStringArray CollsToBeReplicated;
	CollsToBeReplicated.Add(SelColl->fullPath);

	frame->BeginWaitCursor();
	int t = irodsWinReplicateObjects(pDoc->conn, CollsToBeReplicated, irodsResc);
	frame->EndWaitCursor();

	if(t < 0)
	{
		CString msgHead = CString("replicate collection() error for ") +  SelColl->fullPath;
		pDoc->disp_err_msgbox(msgHead, t);
		return;
	}

	// refresh the content of the replicated node.
	refresh_disp_with_a_node_in_tree(hSelNode);

	CString msg = "The selected collection has been succesfully replicated!";
	frame->statusbar_msg(msg);
}

void CLeftView::OnMetadata() 
{
}

//
// DRAG AND DROP / CLIPBOARD SECTION
//
//extern gDragDropList;
void CLeftView::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
#if 0
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC fe2= {gInqClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	CharlieSource* pDataSource = new CharlieSource;
	pDataSource->m_doc = GetDocument();

	//pDataSource->CacheData(gInqClipboardFormat, &fe2);
	STGMEDIUM stgmedium;

	stgmedium.tymed = TYMED_HGLOBAL;
	stgmedium.hGlobal = NULL;

	pDataSource->CacheData(gInqClipboardFormat, &stgmedium);
	pDataSource->DelayRenderData(CF_HDROP, &fe);

	CTreeCtrl& tree = GetTreeCtrl();

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	HTREEITEM blah = tree.HitTest(pt);
	ASSERT(blah != NULL);
	WINI::INode* node = (WINI::INode*)tree.GetItemData(blah);
	ASSERT(node != NULL);

	gNodeDragList.clear();
	gOn = 0;
	gNodeDragList.push_back(node);

	g_bIsDragging = true;
	DROPEFFECT dropEffect = pDataSource->DoDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE);
#endif

	*pResult = 0;
}

DROPEFFECT CLeftView::OnDragEnter( COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	CTreeCtrl& tree = GetTreeCtrl();
	selected_node = tree.GetSelectedItem();
	m_drag_over_item_node = NULL;

	if((dwKeyState&MK_CONTROL) == MK_CONTROL)
		return DROPEFFECT_COPY;
	else
		return DROPEFFECT_MOVE;
	//return OnDragOver(pDataObject, dwKeyState, point);
}

DROPEFFECT CLeftView::OnDragOver( COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hit_node = tree.HitTest(point);
	if(hit_node != NULL)
	{
		if(hit_node != m_drag_over_item_node)
		{
			if(m_drag_over_item_node != NULL)
			{
				// clear the select
				tree.SetItemState(m_drag_over_item_node, 0, TVIS_DROPHILITED);
			}

			// select the hit_node
			tree.SetItemState(hit_node, TVIS_DROPHILITED, TVIS_DROPHILITED);
				// TVIS_DROPHILITED | TVIS_SELECTED, TVIS_DROPHILITED | TVIS_SELECTED);

			m_drag_over_item_node = hit_node;
		}
	}
	
	if((dwKeyState&MK_CONTROL) == MK_CONTROL)
		return DROPEFFECT_COPY;
	else
		return DROPEFFECT_MOVE;

#if 0
	HTREEITEM blah = tree.HitTest(point);

	if(NULL == blah)
		return DROPEFFECT_NONE;

	WINI::INode* node = (WINI::INode*)tree.GetItemData(blah);

	if(NULL == node)
	{
		ASSERT(1);
		return DROPEFFECT_NONE;
	}

	if(WINI_COLLECTION != node->GetType())
		return DROPEFFECT_NONE;

	m_bDontChange = true;

	if(pDataObject->IsDataAvailable(gInqClipboardFormat))
	{
	//	ASSERT(g_bIsDragging);	//if bool is true, then inQ is the program that's the source of the drag
		tree.SelectDropTarget(blah);
		return DROPEFFECT_MOVE;
	}

	if(pDataObject->IsDataAvailable(CF_HDROP))
	{
		tree.SelectDropTarget(blah);	//assume all cfhdrops originate from outside of inq
		return DROPEFFECT_COPY;
	}

	return DROPEFFECT_NONE;	//dragging something into inq that's not a file or inqtype
#endif
}

void CLeftView::dragdrop_de_higlite()
{
	CTreeCtrl& tree = GetTreeCtrl();
	if(m_drag_over_item_node != NULL)
	{
		if(m_drag_over_item_node != selected_node)
		{
			tree.SetItemState(m_drag_over_item_node, 0, TVIS_DROPHILITED);
		}
		m_drag_over_item_node = NULL;
	}
}

//later we should consider dropEffect, not simply imply copy or move
BOOL CLeftView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	CString msg;
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hit_node = tree.HitTest(point);
	if(hit_node != NULL)
	{
		irodsWinCollection *coll = (irodsWinCollection *)tree.GetItemData(hit_node);
		CString drag_drop_parent_coll = coll->fullPath;
		CInquisitorDoc* pDoc = GetDocument();

		if(pDataObject->IsDataAvailable(CF_HDROP))  // drag drop from external file system
		{
			if(pDoc->dragdrop_upload_local_data(pDataObject, drag_drop_parent_coll))
			{
				if(hit_node == selected_node) // need to refresh the right list
				{
					refresh_disp_with_a_node_in_tree(hit_node);
				}
				else
				{
					refresh_children_for_a_tree_node(hit_node);
				}
			}
			dragdrop_de_higlite();
			return false;
		}
		else if(pDataObject->IsDataAvailable(gIExpClipboardFormat))   // drag drop from the right data item list
		{
			// check if the droped node is the select node , ie. the parent of selected items.
			if(hit_node == selected_node)
			{
				CString msg = "The selected item(s) is/are already in the droped node."; 
				frame->statusbar_msg(msg);
				dragdrop_de_higlite();
				return false;
			}
			if((pDoc->colls_in_copy_buffer.GetSize() > 0) || (pDoc->dataobjs_in_copy_buffer.GetSize() > 0))
			{
				switch(dropEffect)
				{
				case DROPEFFECT_COPY:
					msg = CString("Are you sure you want to copy the selected data into '") +  drag_drop_parent_coll + CString("'?");
					if(AfxMessageBox(msg, MB_YESNO) != IDYES)
					{
						frame->statusbar_clear();
						dragdrop_de_higlite();
						return false;
					}
					pDoc->paste_buffered_irods_objs(drag_drop_parent_coll);
					refresh_children_for_a_tree_node(hit_node);
					//refresh_disp_with_a_node_in_tree(selected_node);
					break;
				case DROPEFFECT_MOVE:
					msg = CString("Are you sure you want to move the selected data into '") +  drag_drop_parent_coll + CString("'?");
					if(AfxMessageBox(msg, MB_YESNO) != IDYES)
					{
						frame->statusbar_clear();
						dragdrop_de_higlite();
						return false;
					}
					pDoc->move_buffered_irods_objs(drag_drop_parent_coll);
					refresh_children_for_a_tree_node(hit_node);
					refresh_disp_with_a_node_in_tree(selected_node);
					break;
				}
			}
		}
	}

	dragdrop_de_higlite();

#if 0
	g_bIsDragging = false;

	m_bDontChange = false;

	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM selected_item = tree.HitTest(point);

	WINI::INode* selected_node;
	
	selected_node = (WINI::INode*)tree.GetItemData(selected_item);

	CInquisitorDoc* pDoc = GetDocument();

	STGMEDIUM item;
	HDROP DropHandle;
	UINT quantity;
	char buf[MAX_PATH];

	BOOL nRet = FALSE;

	int count;
	WINI::INode* dragged_node;

	if(pDataObject->IsDataAvailable(gInqClipboardFormat))
	{
		count = gNodeDragList.size();
		gCount = count;
		ASSERT(DROPEFFECT_MOVE == dropEffect);
	
		for(int i = 0; i < count; i++)
		{
			dragged_node = gNodeDragList.at(i);

			if(NULL == dragged_node)
				continue;

			pDoc->Copy(selected_node, dragged_node, true);
		}

		gNodeDragList.clear();
		gOn = 0;
	}
	else if(pDataObject->IsDataAvailable(CF_HDROP))
	{
		ASSERT(DROPEFFECT_COPY);
		tree.SelectItem(selected_item);

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

			nRet = TRUE;
		}

	}else
	{
		ASSERT(1);
	}
#endif
	return false;
}

void CLeftView::OnDragLeave()
{
	CTreeCtrl& tree = GetTreeCtrl();
	if(m_drag_over_item_node != NULL)
	{
		if(m_drag_over_item_node != selected_node)
		{
			tree.SetItemState(m_drag_over_item_node, 0, TVIS_DROPHILITED);
		}
	}
	m_drag_over_item_node = NULL;
#if 0
	POINT pt;
	GetCursorPos(&pt);
	CRect rect;
	CWnd* wnd = AfxGetMainWnd();
	wnd->GetClientRect(&rect);

//	if(!rect.PtInRect(pt))			what if it comes back inside after it leaves?
//		g_bIsDragging = false;		let DataSource::OnRender handle it

	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM hitem = tree.GetDropHilightItem();

	if(hitem != NULL)
		tree.SelectDropTarget(NULL);

	m_bDontChange = false;		//if it wasn't already
#endif
}

#if 0
void CLeftView::OnMouseMove(UINT nFlags, CPoint point) 
{

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM blah = tree.HitTest(pt);
	if(blah == m_collection_root)
	{
		::SendMessage((AfxGetMainWnd())->m_hWnd, WM_LBUTTONDBLCLK, NULL, 0);
	}

	CTreeView::OnMouseMove(nFlags, point);
}
#endif

void CLeftView::OnEditCopy()
{
	// Bing: It is done.
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;
	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);

	CInquisitorDoc* pDoc = GetDocument();

	// clear copy buffer
	pDoc->colls_in_copy_buffer.RemoveAll();
	pDoc->dataobjs_in_copy_buffer.RemoveAll();

	// add the new one in
	irodsWinCollection tColl;
	iSelColl->CopyTo(tColl);
	pDoc->colls_in_copy_buffer.Add(tColl);

}

#if 0
void CLeftView::OnEditCut() 
{
	gDS = new CharlieSource;
	gDS->m_doc = GetDocument();

	FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC fe2= {gInqClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM hItem = tree.GetSelectedItem();

	WINI::INode* node = (WINI::INode*)tree.GetItemData(hItem);

	int type = node->GetType();

	ASSERT(WINI_COLLECTION == type || WINI_DATASET == type);

	//5 nulls, 1 four letter type, and 1 one letter Y or N
	//add n bytes for size of an int. (replication index) + n bytes for another int
	//(terminator is no longer a null after the final null terminated string)
	//(terminator is now a null string after a final sizeof(int) sized block (yes it's waste).
	int blah = 10 + strlen(node->GetName()) + strlen(node->GetPath()) + 2 * sizeof(int);

	HANDLE handle = ::GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, blah * sizeof(char));

	void* ptr = ::GlobalLock(handle);
	int* repl = (int*)ptr;
	char* szptr = (char*)ptr + sizeof(int);

	if(WINI_COLLECTION == type)
	{
		*repl = 0;	//no replication value for a collection
		sprintf(szptr, "COLL");
	}
	else
	{
		*repl = atoi(((WINI::IDatasetNode*)node)->GetReplicantNumber());
		sprintf(szptr, "DATA");
	}

	szptr += 5;

	sprintf(szptr, "Y");
	szptr += 2;

	sprintf(szptr, "%s", node->GetName());
	szptr += strlen(node->GetName()) + 1;

	sprintf(szptr, "%s", node->GetPath());
	szptr += strlen(node->GetPath()) + 1;

	//final 4 bytes for integer and last 1 byte for null string is already there( memory initialized with zeroes).

	::GlobalUnlock(handle);

	gDS->CacheGlobalData(gInqClipboardFormat, handle);
	ghDragList = handle;
	gDS->DelayRenderData(CF_HDROP, &fe);
	gDS->SetClipboard();
}
#endif

void CLeftView::OnEditPaste() 
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;
	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);

	CInquisitorDoc* pDoc = GetDocument();
	pDoc->paste_buffered_irods_objs(iSelColl->fullPath);
}

//
// UPDATE SECTION
//

/*
void CLeftView::OnUpdateMetadata(CCmdUI* pCmdUI) 
{
	#if 0
	WINI::INode* current_node = GetDocument()->GetCurrentNode();

	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(current_node->GetType())
	{
	case WINI_DATASET:
	case WINI_COLLECTION:
		break;
	default:
		pCmdUI->Enable(FALSE);
	}		
	#endif
}
*/

void CLeftView::OnUpdateSynchronize(CCmdUI* pCmdUI) 
{
	#if 0
	WINI::INode* current_node = GetDocument()->GetCurrentNode();

	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_CONTAINER != current_node->GetType())
		pCmdUI->Enable(FALSE);	
	#endif
}

void CLeftView::OnUpdateNewContainer(CCmdUI* pCmdUI) 
{
	#if 0
	WINI::INode* Node = GetDocument()->GetCurrentNode();

	if(NULL == Node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(Node->GetType() != WINI_RESOURCE)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(NULL == Node->GetParent())
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	#endif
}

void CLeftView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
	{
		pCmdUI->Enable(false);
		return;
	}
	
	pCmdUI->Enable(true);
}

//note: code is written this way not to change pCmdUI->Enable
//unless absolutely have to. This eliminates on screen flicker
//in the button
void CLeftView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	CInquisitorDoc* pDoc = GetDocument();
	if((pDoc->colls_in_copy_buffer.GetSize()==0)&&(pDoc->dataobjs_in_copy_buffer.GetSize()==0))
	{
		pCmdUI->Enable(false);
		return;
	}
	pCmdUI->Enable(true);
}

void CLeftView::OnUpdateViewLargeicon(CCmdUI* pCmdUI) 
{
		pCmdUI->Enable(FALSE);
}

void CLeftView::OnUpdateViewList(CCmdUI* pCmdUI) 
{
		pCmdUI->Enable(FALSE);
}

void CLeftView::OnUpdateViewSmallicon(CCmdUI* pCmdUI) 
{
		pCmdUI->Enable(FALSE);
}

void CLeftView::OnUpdateViewDetails(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);	
}

#if 0
void CLeftView::OnUpdateUpload(CCmdUI* pCmdUI) 
{

	WINI::INode* current_node = GetDocument()->GetCurrentNode();
	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_COLLECTION != current_node->GetType())
	{
		pCmdUI->Enable(FALSE);
	}

}
#endif
#if 0
void CLeftView::OnUpdateUploadFolder(CCmdUI* pCmdUI) 
{

	WINI::INode* current_node = GetDocument()->GetCurrentNode();
	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_COLLECTION != current_node->GetType())
	{
		pCmdUI->Enable(FALSE);
	}
}
#endif

#if 0
void CLeftView::OnUpdateDownload(CCmdUI* pCmdUI) 
{

	WINI::INode* current_node = GetDocument()->GetCurrentNode();
	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(current_node->GetType())
	{
	case WINI_COLLECTION:
	case WINI_DATASET:
		break;
	default:
		pCmdUI->Enable(FALSE);
	}

}
#endif

void CLeftView::OnUpdateNewCollection(CCmdUI* pCmdUI) 
{
	
}

#if 0
void CLeftView::OnUpdateDelete(CCmdUI* pCmdUI) 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(node == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(node->GetType())
	{
	case WINI_COLLECTION:
	case WINI_DATASET:
	case WINI_METADATA:
		break;
	default:
		pCmdUI->Enable(FALSE);
		break;
	}
}
#endif

void CLeftView::OnUpdateRename(CCmdUI* pCmdUI) 
{
	int i;
	i = 9;
#if 0
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(node == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_DATASET != node->GetType())
		pCmdUI->Enable(FALSE);
#endif
}

#if 0
void CLeftView::OnComment() 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();

	ASSERT(NULL != node);

	CGenericDialog myDlg("L");

	if(IDOK == myDlg.DoModal())
	{
		gCount = 1;
		GetDocument()->SetComment(node, myDlg.m_Edit.LockBuffer());
		myDlg.m_Edit.UnlockBuffer();
	}
}
#endif

#if 0
void CLeftView::OnUpdateComment(CCmdUI* pCmdUI) 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_DATASET != node->GetType())
		pCmdUI->Enable(FALSE);
}
#endif

#if 0
void CLeftView::OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{

	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItem = tree.GetSelectedItem();

	WINI::INode* node = (WINI::INode*)tree.GetItemData(hItem);

	if(NULL == node)
		*pResult = 0;
		

	if(node->GetType() == WINI_DATASET || node->GetType() == WINI_COLLECTION)
		*pResult = 0;
	else
		*pResult = 1;
}
#endif

#if 0
void CLeftView::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	
	TVITEM* tv = &pTVDispInfo->item;

	if(NULL == tv->pszText)
	{
		//for TVN_ENDLABELEDIT, a NULL in pszText means the edit was cancelled.
		//GetTreeCtrl().EditLabel(pTVDispInfo->item.hItem);
		*pResult = 1;
		return;
	}

	/*if string is not acceptable, print a suitable dialog box, and then use this code to reset the dialog (just like explorer)*/
	/*it looks like the string stays the edited value by default so there's no setting...*/
	if(0 == strcmp(tv->pszText, ""))
	{
		AfxMessageBox("That is an invalid name.", MB_OK);
		GetTreeCtrl().EditLabel(pTVDispInfo->item.hItem);
		*pResult = 0;	/*returning this value tells it it's no good, but I guess you have to use EditLabel to reset it to original anyway*/
		return;
	}

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItem = tree.GetSelectedItem();
	WINI::INode* node = (WINI::INode*)tree.GetItemData(pTVDispInfo->item.hItem);
	
	*pResult = 1;


	gCount = 1;

	CInquisitorDoc* pDoc = GetDocument();

	pDoc->Rename(node, tv->pszText);
}
#endif

/*
void CLeftView::OnNewMeta() 
{
	CMainFrame* ptr = ((CMainFrame*)AfxGetMainWnd());
	ptr->SetFocusMeta();
}
*/

#if 0
void CLeftView::OnUpdateNewMeta(CCmdUI* pCmdUI) 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	int type = node->GetType();

	if(WINI_DATASET != type && WINI_COLLECTION != type)
		pCmdUI->Enable(FALSE);
}
#endif

// to be called right before the application exits.
void CLeftView::clear_tree()
{
	CTreeCtrl& tree = GetTreeCtrl();
	
	HTREEITEM root = tree.GetRootItem();
	delete_one_node(root);
}

void CLeftView::delete_one_node(HTREEITEM hDelNode)
{
	CTreeCtrl& tree = GetTreeCtrl();
	if(tree.ItemHasChildren(hDelNode))
	{
		HTREEITEM hChildItem = tree.GetChildItem(hDelNode);
		while(hChildItem != NULL)
		{
			this->delete_one_node(hChildItem);
			//hChildItem = tree.GetNextSiblingItem(hChildItem);
			hChildItem = tree.GetChildItem(hDelNode);
		}
	}

	irodsWinCollection *iDelColl = (irodsWinCollection *)tree.GetItemData(hDelNode);
	tree.SetItemData(hDelNode, (DWORD_PTR)NULL);
	delete iDelColl;
	tree.DeleteItem(hDelNode);
}

void CLeftView::delete_child_nodes(HTREEITEM hNode)
{
	CTreeCtrl& tree = GetTreeCtrl();
	if(tree.ItemHasChildren(hNode))
	{
		HTREEITEM hChildItem = tree.GetChildItem(hNode);
		while(hChildItem != NULL)
		{
			this->delete_one_node(hChildItem);
			hChildItem = tree.GetChildItem(hNode);
		}
	}
	irodsWinCollection *iColl = (irodsWinCollection *)tree.GetItemData(hNode);
	iColl->queried = false;
}

void CLeftView::OnUploadUploadafolder()
{
	// TODO: Add your command handler code here
	CInquisitorDoc* pDoc = GetDocument();
	
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;
	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->BeginWaitCursor();

	irodsWinCollection newColl;
	if(pDoc->upload_a_localfolder_into_coll(iSelColl->fullPath, newColl) != 0)
	{
		frame->EndWaitCursor();
		return;
	}

	// add the new coll into the tree
	irodsWinCollection *tColl = new irodsWinCollection();
	newColl.CopyTo(*tColl);
	HTREEITEM tCurrent = tree.InsertItem(tColl->name, 23, 44, hSelNode);
	tree.SetItemData(tCurrent, (DWORD_PTR)tColl);
	tColl = NULL;

	// add the new coll into parent data structure.
	iSelColl->childCollections.push_back(newColl);

	// if right mode, add it into rlist
	if(pDoc->rlist_disp_mode == CInquisitorDoc::RLIST_MODE_COLL)
	{
		pDoc->insert_one_coll_in_rlist(newColl);
	}

	frame->EndWaitCursor();
}

void CLeftView::OnUploadUploadfiles()
{
	// TODO: Add your command handler code here
	CInquisitorDoc* pDoc = GetDocument();
	
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;
	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);

	CArray<irodsWinDataobj, irodsWinDataobj> newDatas;
	if(pDoc->upload_localfiles_into_coll(iSelColl->fullPath, newDatas) != 0)
		return;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->BeginWaitCursor();
	for(int i=0;i<newDatas.GetSize();i++)
	{
		// check if the data is already in the place.
		bool in_list = false;
		for(int j=0;j<iSelColl->childDataObjs.size();j++)
		{
			if((iSelColl->childDataObjs[j].name==newDatas[i].name)&&(iSelColl->childDataObjs[j].replNum==newDatas[i].replNum))
			{
				in_list = true;
				break;
			}
		}
		if(in_list == false)
		{
			iSelColl->childDataObjs.push_back(newDatas[i]);
			if(pDoc->rlist_disp_mode == CInquisitorDoc::RLIST_MODE_COLL)
			{
				pDoc->insert_one_dataobj_in_rlist(newDatas[i]);
			}
		}
	}

	frame->EndWaitCursor();
}

void CLeftView::OnUpdateSelectAll(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(FALSE);
}

void CLeftView::OnEditCopypathtoclipboard()
{
	// TODO: Add your command handler code here
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	irodsWinCollection *iSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);

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

	HANDLE hClipboardData = GlobalAlloc(GPTR, iSelColl->fullPath.GetLength()+1);
	LPTSTR lpszBuffer = (LPTSTR)GlobalLock(hClipboardData);
	_tcscpy(lpszBuffer, iSelColl->fullPath);
	GlobalUnlock(hClipboardData);
	SetClipboardData(CF_TEXT, hClipboardData);
	CloseClipboard();
}

void CLeftView::delete_node_from_selected_parent(CString & nodeFullPath)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;
	
	int i;
	irodsWinCollection *SelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	for(i=0;i<SelColl->childCollections.size();i++)
	{
		if(nodeFullPath == SelColl->childCollections[i].fullPath)
		{
			SelColl->childCollections.erase(SelColl->childCollections.begin()+i);
			break;
		}
	}

	// find the coll in child nodes in tree
	if(tree.ItemHasChildren(hSelNode)) // get child list
	{
		HTREEITEM tChild = tree.GetChildItem(hSelNode);
		while(tChild != NULL)
		{
			irodsWinCollection *tColl = (irodsWinCollection *)tree.GetItemData(tChild);
			if(nodeFullPath == tColl->fullPath)
			{
				this->delete_one_node(tChild);
				break;
			}
			tChild = tree.GetNextSiblingItem(tChild);
		}
	}
}

void CLeftView::delete_dataobj_from_selected_parent(CString & dataName, CString & parCollPath, int replNum)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	irodsWinCollection *SelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	for(int i=0;i<SelColl->childDataObjs.size();i++)
	{
		if((SelColl->childDataObjs[i].name==dataName)&&(SelColl->childDataObjs[i].parentCollectionFullPath==parCollPath)
			&&(SelColl->childDataObjs[i].replNum==replNum))
		{
			SelColl->childDataObjs.erase(SelColl->childDataObjs.begin()+i);
			return;
		}
	}
}

void CLeftView::change_one_collname_from_selected_parent(CString & collOldFullPath, CString & newName)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	irodsWinCollection *SelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	for(int i=0;i<SelColl->childCollections.size();i++)
	{
		if(collOldFullPath == SelColl->childCollections[i].fullPath)
		{
			//SelColl->childCollections.erase(SelColl->childCollections.begin()+i);
			CString parPath, newFullPath;
			irodsWinUnixPathGetParent(collOldFullPath, parPath);
			newFullPath = collOldFullPath + "/" + newName;
			SelColl->childCollections[i].fullPath = newFullPath;
			SelColl->childCollections[i].name = newName;
			break;
		}
	}

	// find the coll in child nodes in tree
	if(tree.ItemHasChildren(hSelNode)) // get child list
	{
		HTREEITEM tChild = tree.GetChildItem(hSelNode);
		while(tChild != NULL)
		{
			irodsWinCollection *tColl = (irodsWinCollection *)tree.GetItemData(tChild);
			if(collOldFullPath == tColl->fullPath)
			{
				CString parPath, newFullPath;
				irodsWinUnixPathGetParent(collOldFullPath, parPath);
				newFullPath = collOldFullPath + "/" + newName;
				tColl->fullPath = newFullPath;
				tColl->name = newName;
				tree.SetItemText(tChild, newName);
				break;
			}
			tChild = tree.GetNextSiblingItem(tChild);
		}
	}
}

void CLeftView::change_one_dataname_from_selected_parent(CString & oldName, CString newName)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	irodsWinCollection *SelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);
	for(int i=0;i<SelColl->childDataObjs.size();i++)
	{
		if(SelColl->childDataObjs[i].name == oldName)
		{
			SelColl->childDataObjs[i].name = newName;
		}
	}
}

void CLeftView::refresh_children_for_a_tree_node(HTREEITEM theNode)
{
	CTreeCtrl& tree = GetTreeCtrl();

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc* pDoc = GetDocument();
	irodsWinCollection *theColl = (irodsWinCollection *)tree.GetItemData(theNode);
	
	frame->BeginWaitCursor();

	// query irods for child collection and child files
	CString msg = "getting child collections...";
	frame->statusbar_msg(msg);
	int t = irodsWinGetChildCollections(pDoc->conn, theColl->fullPath, pDoc->colls_in_list);
	if(t < 0)
	{
		CString msgHead = CString("Get Child Collections() error for ") + CString(theColl->fullPath);
		pDoc->disp_err_msgbox(msgHead, t);
		frame->statusbar_clear();
		frame->EndWaitCursor();
		return;
	}

	// query for irods files
	msg = "getting child datasets...";
	frame->statusbar_msg(msg);
	t = irodsWinGetChildObjs(pDoc->conn, theColl->fullPath, pDoc->dataobjs_in_list);
	if(t < 0)
	{
		CString msgHead = CString("Get Child Datasets() error for ") + theColl->fullPath;
		pDoc->disp_err_msgbox(msgHead, t);
		frame->statusbar_clear();
		frame->EndWaitCursor();
		return;
	}
	theColl->queried = true;

	HTREEITEM tChild = tree.GetChildItem(theNode);
	while(tChild != NULL)
	{
		delete_one_node(tChild);
		tChild = tree.GetChildItem(theNode);
	}

	add_child_collections(theNode, pDoc->colls_in_list);

	// add the child colls into selected node.
	theColl->childCollections.clear();
	for(int i=0;i<pDoc->colls_in_list.GetSize();i++)
	{
		theColl->childCollections.push_back(pDoc->colls_in_list[i]);
	}
	
	// add the child dataobjs into the selected node.
	theColl->childDataObjs.clear();
	for(int i=0;i<pDoc->dataobjs_in_list.GetSize();i++)
	{
		theColl->childDataObjs.push_back(pDoc->dataobjs_in_list[i]);
	}
}

void CLeftView::refresh_disp_with_a_node_in_tree(HTREEITEM theNode)
{
	CTreeCtrl& tree = GetTreeCtrl();

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc* pDoc = GetDocument();
	irodsWinCollection *theColl = (irodsWinCollection *)tree.GetItemData(theNode);
	
	frame->BeginWaitCursor();

	// query irods for child collection and child files
	CString msg = "getting child collections...";
	frame->statusbar_msg(msg);
	int t = irodsWinGetChildCollections(pDoc->conn, theColl->fullPath, pDoc->colls_in_list);
	if(t < 0)
	{
		CString msgHead = CString("Get Child Collections() error for ") + CString(theColl->fullPath);
		pDoc->disp_err_msgbox(msgHead, t);
		frame->statusbar_clear();
		frame->EndWaitCursor();
		return;
	}

	// query for irods files
	msg = "getting child datasets...";
	frame->statusbar_msg(msg);
	t = irodsWinGetChildObjs(pDoc->conn, theColl->fullPath, pDoc->dataobjs_in_list);
	if(t < 0)
	{
		CString msgHead = CString("Get Child Datasets() error for ") + theColl->fullPath;
		pDoc->disp_err_msgbox(msgHead, t);
		frame->statusbar_clear();
		frame->EndWaitCursor();
		return;
	}
	theColl->queried = true;

	// delete all Child nodes in tree of 'theNode'.
	msg = "deleting child nodes in tree...";
	frame->statusbar_msg(msg);
	HTREEITEM tChild = tree.GetChildItem(theNode);
	while(tChild != NULL)
	{
		delete_one_node(tChild);
		tChild = tree.GetChildItem(theNode);
	}

	// added sub-collections as new child nodes
	msg = "inserting new child collections...";
	frame->statusbar_msg(msg);
	add_child_collections(theNode, pDoc->colls_in_list);

	// add the child colls into selected node.
	theColl->childCollections.clear();
	for(int i=0;i<pDoc->colls_in_list.GetSize();i++)
	{
		theColl->childCollections.push_back(pDoc->colls_in_list[i]);
	}
	
	// add the child dataobjs into the selected node.
	theColl->childDataObjs.clear();
	for(int i=0;i<pDoc->dataobjs_in_list.GetSize();i++)
	{
		theColl->childDataObjs.push_back(pDoc->dataobjs_in_list[i]);
	}

	// fill the list
	msg = "updating list...";
	frame->statusbar_msg(msg);
	pDoc->update_rlist();
	pDoc->rlist_disp_mode = CInquisitorDoc::RLIST_MODE_COLL;

	frame->statusbar_clear();
	msg.Format("sub-collections: %d, files: %d", pDoc->colls_in_list.GetSize(), pDoc->dataobjs_in_list.GetSize());
	frame->statusbar_msg2(msg);
	frame->EndWaitCursor();
}

void CLeftView::OnViewRefresh()
{
	// TODO: Add your command handler code here
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;
	
	refresh_disp_with_a_node_in_tree(hSelNode);
}

void CLeftView::add_child_collections(HTREEITEM parNode, CArray<irodsWinCollection, irodsWinCollection> & colls)
{
	CTreeCtrl& tree = GetTreeCtrl();
	irodsWinCollection *tmpColl;
	for(int i=0;i<colls.GetSize();i++)
	{
		tmpColl = new irodsWinCollection();
		colls[i].CopyTo(*tmpColl);
		tmpColl->queried = false;

		HTREEITEM tCurrent = tree.InsertItem(tmpColl->name, 23, 44, parNode);
		tree.SetItemData(tCurrent, (DWORD_PTR)tmpColl);
		tmpColl = NULL;
	}
}

void CLeftView::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	POINT pt;
	GetCursorPos(&pt);

	CMenu tree_popmenu;;

	CMenu newSubmenu;
	newSubmenu.CreatePopupMenu();
	newSubmenu.AppendMenu(MF_ENABLED, ID_NEW_COLLECTION, "Collection...");

	CMenu uploadSubmenu;
	uploadSubmenu.CreatePopupMenu();
	uploadSubmenu.AppendMenu(MF_ENABLED, ID_UPLOAD_UPLOADFILES, "Upload Files...");
	uploadSubmenu.AppendMenu(MF_ENABLED, ID_UPLOAD_UPLOADAFOLDER, "Upload a Folder...");

	tree_popmenu.CreatePopupMenu();
	tree_popmenu.AppendMenu(MF_POPUP|MF_STRING, (UINT)newSubmenu.m_hMenu, "New");
	tree_popmenu.AppendMenu(MF_ENABLED, ID_DELETE, "Delete");
	tree_popmenu.AppendMenu(MF_ENABLED, ID_RENAME, "Rename...");
	tree_popmenu.AppendMenu(MF_ENABLED, ID_REPLICATE, "Replicate");
	tree_popmenu.AppendMenu(MF_ENABLED, ID_DOWNLOAD, "Download...");
	tree_popmenu.AppendMenu(MF_POPUP|MF_STRING, (UINT)uploadSubmenu.m_hMenu, "Upload");
	tree_popmenu.AppendMenu(MF_SEPARATOR, 0, "");
	tree_popmenu.AppendMenu(MF_ENABLED, ID_EDIT_COPYPATHTOCLIPBOARD, "Copy iRODS Path to Clipboard");
	tree_popmenu.AppendMenu(MF_SEPARATOR, 0, "");
	tree_popmenu.AppendMenu(MF_ENABLED, ID_EDIT_COPY, "Copy");
	tree_popmenu.AppendMenu(MF_ENABLED, ID_EDIT_PASTE, "Paste");
	tree_popmenu.AppendMenu(MF_SEPARATOR, 0, "");
	tree_popmenu.AppendMenuA(MF_ENABLED, ID_VIEW_REFRESH, "Refresh");

	tree_popmenu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y,this);
}

void CLeftView::SetSelectedNodeChildQueryBit(CString & childFullPath, bool newval)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	HTREEITEM tChild = tree.GetChildItem(hSelNode);
	while(tChild != NULL)
	{
		irodsWinCollection *tColl = (irodsWinCollection *)tree.GetItemData(tChild);
		if(tColl->fullPath == childFullPath)
		{
			tColl->queried = newval;
			return;
		}
		tChild = tree.GetNextSiblingItem(tChild);
	}
}

void CLeftView::rlist_dbclick_coll_action(CString & collFullPath)
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	tree.Expand(hSelNode, TVIS_EXPANDED);

	HTREEITEM tChild = tree.GetChildItem(hSelNode);
	while(tChild != NULL)
	{
		irodsWinCollection *tColl = (irodsWinCollection *)tree.GetItemData(tChild);
		if(tColl->fullPath == collFullPath)
		{
			//tree.Expand(tChild, TVIS_EXPANDED);
			tree.SelectItem(tChild);
			return;
		}
		tChild = tree.GetNextSiblingItem(tChild);
	}
}

void CLeftView::tree_goto_parent_node()
{
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	HTREEITEM hParentOfSelNode = tree.GetParentItem(hSelNode);

	if((hParentOfSelNode!=NULL)&&(hParentOfSelNode != hSelNode))
		tree.SelectItem(hParentOfSelNode);
}

#if 0
void CLeftView::OnEditMetadata()
{
	// TODO: Add your command handler code here
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	irodsWinCollection *pSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);

	//CMetadataDlg *metaDlg = new CMetadataDlg(pSelColl->fullPath, irodsWinObj::IRODS_COLLECTION_TYPE, this);
	//metaDlg->Create(IDD_DIALOG_METADATA, this);
	//metaDlg->ShowWindow(SW_SHOW);

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->CreateMetadataDlg(pSelColl->fullPath, irodsWinObj::IRODS_COLLECTION_TYPE);
}
#endif
void CLeftView::OnUpdateEditMetadataSystemmetadata(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(false);
}

void CLeftView::OnEditMetadataUsermetadata()
{
	// TODO: Add your command handler code here
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelNode = tree.GetSelectedItem();
	if(hSelNode == NULL)
		return;

	irodsWinCollection *pSelColl = (irodsWinCollection *)tree.GetItemData(hSelNode);

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->CreateMetadataDlg(pSelColl->fullPath, irodsWinObj::IRODS_COLLECTION_TYPE);
}
