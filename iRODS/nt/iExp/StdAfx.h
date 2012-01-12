// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__F0985499_8B86_40E1_889A_5740B97DDFAA__INCLUDED_)
#define AFX_STDAFX_H__F0985499_8B86_40E1_889A_5740B97DDFAA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcview.h>
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxole.h>	//charliecharlie
#include <afxadv.h>
#include "winiObjects.h"


const UINT msgOpenCompleted = ::RegisterWindowMessage(_T("opensrbcompleted"));
const UINT msgOpen = ::RegisterWindowMessage(_T("opensrb"));
const UINT msgMyMsg = ::RegisterWindowMessage(_T("blah"));
const UINT msgUpdateOperation = ::RegisterWindowMessage(_T("operation"));
const UINT msgStatusMessage1 = ::RegisterWindowMessage(_T("status message1"));
const UINT msgStatusMessage2 = ::RegisterWindowMessage(_T("status message2"));
const UINT msgDownload = ::RegisterWindowMessage(_T("download"));
const UINT msgUpload = ::RegisterWindowMessage(_T("upload"));

const UINT msgDelete = ::RegisterWindowMessage(_T("delete"));
const UINT msgNewNode = ::RegisterWindowMessage(_T("new_node"));
const UINT msgReplicate = ::RegisterWindowMessage(_T("replicate"));
const UINT msgRefresh = ::RegisterWindowMessage(_T("refresh"));
const UINT msgExecute = ::RegisterWindowMessage(_T("execute"));
const UINT msgGoAnimation = ::RegisterWindowMessage(_T("goA"));	//startA
const UINT msgStopAnimation = ::RegisterWindowMessage(_T("stopA"));
const UINT msgStatusLine = ::RegisterWindowMessage(_T("status line"));
const UINT msgProgress = ::RegisterWindowMessage(_T("progress"));
const UINT msgDLProgress = ::RegisterWindowMessage(_T("dlprogress"));
const UINT msgULProgress = ::RegisterWindowMessage(_T("ulprogress"));
const UINT msgConnectedUpdate = ::RegisterWindowMessage(_T("connectedupdate"));
const UINT msgSynchronize = ::RegisterWindowMessage(_T("synchronize"));
const UINT msgRefillResourceList = ::RegisterWindowMessage(_T("refillresourcelist"));
const UINT msgDSprogress = ::RegisterWindowMessage(_T("dsprogress"));
const UINT msgDSmsg = ::RegisterWindowMessage(_T("dsmsg"));
const UINT msgCopy = ::RegisterWindowMessage(_T("copy"));
const UINT msgRename = ::RegisterWindowMessage(_T("rename"));
const UINT msgSetComment = ::RegisterWindowMessage(_T("set_comment"));
const UINT msgProgressRotate = ::RegisterWindowMessage(_T("progress_rotate"));

const UINT METADATA_DLG_CLOSE_MSG = ::RegisterWindowMessage(_T("MetadataDlgClose"));

#include <afxmt.h>

#include "winiObjects.h"
#include "winiOperators.h"
#include <afxdisp.h>

// define user message IDs
#define METADAT_DLG_CLOSED_ID (WM_USER+ 102)



//UPDATE OPTIONS
#define QUERY 1;
#define REFRESH 2;


struct dl
{
	WINI::INode* node;
	const char* local_path;
	WINI::INode* binding;
};

struct open_node
{
	WINI::INode* node;
	int levels;
	bool clear;
	unsigned int mask;
};

struct new_node
{
	WINI::INode* parent;
	char* name;
};

struct copy_op
{
	WINI::INode* target;
	WINI::INode* node;		//to copy
	WINI::INode* resource;
	bool delete_original;	//delete original post copy operation ("cut")?
};

struct new_replicant
{
	WINI::INode* node;
	WINI::INode* binding;
};

struct ProgThreadParams
{
	WINI::ITransferOperator* op;
	char* name;
	CEvent* finished;
	HWND handle;
	UINT msg;
};

struct DragStatusThreadParams
{
	CEvent* cancel;
	CEvent* done;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__F0985499_8B86_40E1_889A_5740B97DDFAA__INCLUDED_)
