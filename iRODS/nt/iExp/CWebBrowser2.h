// CWebBrowser2.h  : Declaration of ActiveX Control wrapper class(es) created by Microsoft Visual C++

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CWebBrowser2

class CWebBrowser2 : public CWnd
{
protected:
	DECLARE_DYNCREATE(CWebBrowser2)
public:
	CLSID const& GetClsid()
	{
		static CLSID const clsid
			= { 0x8856F961, 0x340A, 0x11D0, { 0xA9, 0x6B, 0x0, 0xC0, 0x4F, 0xD7, 0x5, 0xA2 } };
		return clsid;
	}
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
						const RECT& rect, CWnd* pParentWnd, UINT nID, 
						CCreateContext* pContext = NULL)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID); 
	}

    BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, 
				UINT nID, CFile* pPersist = NULL, BOOL bStorage = FALSE,
				BSTR bstrLicKey = NULL)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID,
		pPersist, bStorage, bstrLicKey); 
	}

// Attributes
public:

// Operations
public:

	void GoBack()
	{
		InvokeHelper(0x64, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void GoForward()
	{
		InvokeHelper(0x65, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void GoHome()
	{
		InvokeHelper(0x66, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void GoSearch()
	{
		InvokeHelper(0x67, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Navigate(LPCTSTR URL, VARIANT * Flags, VARIANT * TargetFrameName, VARIANT * PostData, VARIANT * Headers)
	{
		static BYTE parms[] = VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT ;
		InvokeHelper(0x68, DISPATCH_METHOD, VT_EMPTY, NULL, parms, URL, Flags, TargetFrameName, PostData, Headers);
	}
	void Refresh()
	{
		InvokeHelper(DISPID_REFRESH, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void Refresh2(VARIANT * Level)
	{
		static BYTE parms[] = VTS_PVARIANT ;
		InvokeHelper(0x69, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Level);
	}
	void Stop()
	{
		InvokeHelper(0x6a, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	LPDISPATCH get_Application()
	{
		LPDISPATCH result;
		InvokeHelper(0xc8, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
		return result;
	}
	LPDISPATCH get_Parent()
	{
		LPDISPATCH result;
		InvokeHelper(0xc9, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
		return result;
	}
	LPDISPATCH get_Container()
	{
		LPDISPATCH result;
		InvokeHelper(0xca, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
		return result;
	}
	LPDISPATCH get_Document()
	{
		LPDISPATCH result;
		InvokeHelper(0xcb, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
		return result;
	}
	BOOL get_TopLevelContainer()
	{
		BOOL result;
		InvokeHelper(0xcc, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	CString get_Type()
	{
		CString result;
		InvokeHelper(0xcd, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	long get_Left()
	{
		long result;
		InvokeHelper(0xce, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	void put_Left(long newValue)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xce, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	long get_Top()
	{
		long result;
		InvokeHelper(0xcf, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	void put_Top(long newValue)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xcf, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	long get_Width()
	{
		long result;
		InvokeHelper(0xd0, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	void put_Width(long newValue)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xd0, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	long get_Height()
	{
		long result;
		InvokeHelper(0xd1, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	void put_Height(long newValue)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0xd1, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	CString get_LocationName()
	{
		CString result;
		InvokeHelper(0xd2, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString get_LocationURL()
	{
		CString result;
		InvokeHelper(0xd3, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	BOOL get_Busy()
	{
		BOOL result;
		InvokeHelper(0xd4, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void Quit()
	{
		InvokeHelper(0x12c, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void ClientToWindow(long * pcx, long * pcy)
	{
		static BYTE parms[] = VTS_PI4 VTS_PI4 ;
		InvokeHelper(0x12d, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pcx, pcy);
	}
	void PutProperty(LPCTSTR Property, VARIANT vtValue)
	{
		static BYTE parms[] = VTS_BSTR VTS_VARIANT ;
		InvokeHelper(0x12e, DISPATCH_METHOD, VT_EMPTY, NULL, parms, Property, &vtValue);
	}
	VARIANT GetProperty(LPCTSTR Property)
	{
		VARIANT result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x12f, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, Property);
		return result;
	}
	CString get_Name()
	{
		CString result;
		InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	long get_HWND()
	{
		long result;
		InvokeHelper(DISPID_HWND, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	CString get_FullName()
	{
		CString result;
		InvokeHelper(0x190, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString get_Path()
	{
		CString result;
		InvokeHelper(0x191, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	BOOL get_Visible()
	{
		BOOL result;
		InvokeHelper(0x192, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_Visible(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x192, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	BOOL get_StatusBar()
	{
		BOOL result;
		InvokeHelper(0x193, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_StatusBar(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x193, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	CString get_StatusText()
	{
		CString result;
		InvokeHelper(0x194, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void put_StatusText(LPCTSTR newValue)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x194, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	long get_ToolBar()
	{
		long result;
		InvokeHelper(0x195, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	void put_ToolBar(long newValue)
	{
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x195, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	BOOL get_MenuBar()
	{
		BOOL result;
		InvokeHelper(0x196, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_MenuBar(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x196, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	BOOL get_FullScreen()
	{
		BOOL result;
		InvokeHelper(0x197, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_FullScreen(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x197, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	void Navigate2(VARIANT * URL, VARIANT * Flags, VARIANT * TargetFrameName, VARIANT * PostData, VARIANT * Headers)
	{
		static BYTE parms[] = VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT ;
		InvokeHelper(0x1f4, DISPATCH_METHOD, VT_EMPTY, NULL, parms, URL, Flags, TargetFrameName, PostData, Headers);
	}
	long QueryStatusWB(long cmdID)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x1f5, DISPATCH_METHOD, VT_I4, (void*)&result, parms, cmdID);
		return result;
	}
	void ExecWB(long cmdID, long cmdexecopt, VARIANT * pvaIn, VARIANT * pvaOut)
	{
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_PVARIANT VTS_PVARIANT ;
		InvokeHelper(0x1f6, DISPATCH_METHOD, VT_EMPTY, NULL, parms, cmdID, cmdexecopt, pvaIn, pvaOut);
	}
	void ShowBrowserBar(VARIANT * pvaClsid, VARIANT * pvarShow, VARIANT * pvarSize)
	{
		static BYTE parms[] = VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT ;
		InvokeHelper(0x1f7, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pvaClsid, pvarShow, pvarSize);
	}
	long get_ReadyState()
	{
		long result;
		InvokeHelper(DISPID_READYSTATE, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	BOOL get_Offline()
	{
		BOOL result;
		InvokeHelper(0x226, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_Offline(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x226, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	BOOL get_Silent()
	{
		BOOL result;
		InvokeHelper(0x227, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_Silent(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x227, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	BOOL get_RegisterAsBrowser()
	{
		BOOL result;
		InvokeHelper(0x228, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_RegisterAsBrowser(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x228, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	BOOL get_RegisterAsDropTarget()
	{
		BOOL result;
		InvokeHelper(0x229, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_RegisterAsDropTarget(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x229, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	BOOL get_TheaterMode()
	{
		BOOL result;
		InvokeHelper(0x22a, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_TheaterMode(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x22a, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	BOOL get_AddressBar()
	{
		BOOL result;
		InvokeHelper(0x22b, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_AddressBar(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x22b, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	BOOL get_Resizable()
	{
		BOOL result;
		InvokeHelper(0x22c, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void put_Resizable(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x22c, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}


};
