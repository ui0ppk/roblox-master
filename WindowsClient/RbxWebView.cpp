#include "stdafx.h"
#include "RbxWebView.h"
#include <Exdispid.h> // platform SDK header
#include "util/FileSystem.h"
#include "util/Statistics.h"
#include "util/Http.h"
#include "format_string.h"
#include <atltypes.h>
#include "v8datamodel/GuiService.h"
#include "v8datamodel/DataModel.h"

FASTSTRING(ClientExternalBrowserUserAgent)

RbxWebView::RbxWebView(const std::string& url, shared_ptr<RBX::Game> newGame)
	: CAxDialogImpl()
	, url(url)
	, m_cRef(1)
	, game(newGame)
	, dialogActive(false)
{
}

HRESULT RbxWebView::QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = 0; // this line pleases Raymond Chen
	if (IID_IUnknown == riid)
	{
		*ppvObject = (LPUNKNOWN)(IDispatch*)this;
		AddRef();
		return S_OK;
	}
	else if (IID_IDispatch == riid)
	{
		*ppvObject = (IDispatch*)this;
		AddRef();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

ULONG RbxWebView::AddRef(void)
{
	return InterlockedIncrement(&m_cRef);
}

ULONG RbxWebView::Release(void)
{
	return InterlockedDecrement(&m_cRef);
}

HRESULT RbxWebView::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
	return E_NOTIMPL;
}

#define ROBLOX_BROWSERFLAGS \
	DOCHOSTUIFLAG_DISABLE_HELP_MENU |\
	DOCHOSTUIFLAG_ENABLE_FORMS_AUTOCOMPLETE |\
	DOCHOSTUIFLAG_THEME |\
	DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE |\
	DOCHOSTUIFLAG_LOCAL_MACHINE_ACCESS_CHECK |\
	DOCHOSTUIFLAG_DISABLE_UNTRUSTEDPROTOCOL |\
	0

HRESULT RbxWebView::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
	pInfo->dwFlags |= 
		DOCHOSTUIFLAG_NO3DBORDER | 
		ROBLOX_BROWSERFLAGS |
		0;

	return S_OK;
}

HRESULT RbxWebView::ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::HideUI(void)
{
	return S_OK;
}

HRESULT RbxWebView::UpdateUI(void)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::EnableModeless(BOOL fEnable)
{

	return E_NOTIMPL;
}

HRESULT RbxWebView::OnDocWindowActivate(BOOL fActivate)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::OnFrameWindowActivate(BOOL fActivate)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::GetExternal(IDispatch **ppDispatch)
{
	*ppDispatch = this;
	return S_OK;
}

HRESULT RbxWebView::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
	return E_NOTIMPL;
}

HRESULT RbxWebView::GetTypeInfoCount(UINT* pctinfo)
{
	if (pctinfo == NULL)
		return E_POINTER;

	*pctinfo = 1;
	return S_OK;
}

HRESULT RbxWebView::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
	return S_OK;
}

HRESULT RbxWebView::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
	return S_OK;
}

HRESULT RbxWebView::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
{	
	return S_OK;
}

SHDocVw::IWebBrowserAppPtr RbxWebView::getWebBrowser()
{
	SHDocVw::IWebBrowserAppPtr pWebBrowser = NULL;
	GetDlgControl(IDC_RBXEXPLORER, __uuidof(SHDocVw::IWebBrowserAppPtr), (void**)&pWebBrowser);

	return pWebBrowser;
}

LRESULT RbxWebView::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CenterWindow();

	webBrowserEvents.SetRbxWebView(this);

	// Load the Roblox Icon
	m_hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINDOW_ICON));
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	SHDocVw::IWebBrowserAppPtr pWebBrowser = NULL;
	HRESULT hr = GetDlgControl(IDC_RBXEXPLORER, __uuidof(SHDocVw::IWebBrowserAppPtr), (void**)&pWebBrowser);

	if (SUCCEEDED(hr)) 
	{
		// setup hooks into browser events
		CComQIPtr<IConnectionPointContainer, &IID_IConnectionPointContainer> cpc(pWebBrowser);
		CComPtr<IConnectionPoint> cp1;
		hr = cpc->FindConnectionPoint(__uuidof(SHDocVw::DWebBrowserEventsPtr), &cp1);
		DWORD dwCookie;
		hr = cp1->Advise((LPUNKNOWN)&webBrowserEvents, &dwCookie);

		CComPtr<IConnectionPoint> cp2;
		hr = cpc->FindConnectionPoint(__uuidof(SHDocVw::DWebBrowserEvents2Ptr), &cp2);
		hr = cp2->Advise((LPUNKNOWN)&webBrowserEvents, &dwCookie);

		// now set the header before the request
		std::string userAgent = "Mozilla/4.0 (compatible; MSIE 7.0) ";
		userAgent.append(FString::ClientExternalBrowserUserAgent);

		std::vector<char> userAgentChars(userAgent.c_str(), userAgent.c_str() + userAgent.size() + 1u);
		::UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, &userAgentChars[0], userAgent.length(), 0);

		// navigate to default page
		pWebBrowser->Navigate(_bstr_t(url.c_str()),NULL,NULL,NULL,NULL);
	}
	else 
	{
		MessageBox("Failed to open web browser", "Error", MB_OK);
		return FALSE;
	}

	dialogActive = true;
	return TRUE;    // let the system set the focus
}

LRESULT RbxWebView::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(dialogActive)
		webBrowserEvents.WindowClosing();

	return 0;
}

void RbxWebView::closeDialog()
{
	if(dialogActive)
		webBrowserEvents.WindowClosing();
}


LRESULT RbxWebView::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SHDocVw::IWebBrowserAppPtr pWebBrowser = NULL;
	HRESULT hr = GetDlgControl(IDC_RBXEXPLORER, __uuidof(SHDocVw::IWebBrowserAppPtr), (void**)&pWebBrowser);

	if (SUCCEEDED(hr))
	{
		CRect rect;
		GetWindowRect(&rect);

		pWebBrowser->put_Width(rect.Width() - ::GetSystemMetrics(SM_CXVSCROLL));
		pWebBrowser->put_Height(rect.Height() - ::GetSystemMetrics(SM_CXHSCROLL) - ::GetSystemMetrics(SM_CYSIZE));
	}

	return 0;
}

HRESULT __stdcall WebBrowserEvents::QueryInterface(REFIID riid, LPVOID* ppv)
{
	*ppv = NULL;

	if (IID_IUnknown == riid || __uuidof(SHDocVw::DWebBrowserEventsPtr) == riid)
	{
		*ppv = (LPUNKNOWN)(SHDocVw::DWebBrowserEventsPtr*)this;
		AddRef();
		return NOERROR;
	}
	else if (IID_IOleClientSite == riid)
	{
		*ppv = (IOleClientSite*)this;
		AddRef();
		return NOERROR;
	}
	else if (IID_IDispatch == riid)
	{
		*ppv = (IDispatch*)this;
		AddRef();
		return NOERROR;
	}
	else
	{
		return E_NOTIMPL;
	}
}

ULONG __stdcall WebBrowserEvents::AddRef() { return 1;}
ULONG __stdcall WebBrowserEvents::Release() { return 0;}

// IDispatch methods
HRESULT __stdcall WebBrowserEvents::GetTypeInfoCount(UINT* pctinfo)
{ 
	return E_NOTIMPL; 
}

HRESULT __stdcall WebBrowserEvents::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{ 
	return E_NOTIMPL; 
}

HRESULT __stdcall WebBrowserEvents::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{ 
	return E_NOTIMPL; 
}

HRESULT __stdcall WebBrowserEvents::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
{ 
	if (dispIdMember == DISPID_WINDOWCLOSING) 
		WindowClosing(pDispParams);

	return S_OK;
}

static void doSignalGuiServiceUrlWindowClose(RBX::DataModel* dataModel)
{
	if (RBX::GuiService* guiService = dataModel->find<RBX::GuiService>())
		guiService->urlWindowClosed();
}

static void signalGuiServiceUrlWindowClosed(RBX::DataModel* dataModel)
{
	if(dataModel)
		dataModel->submitTask(boost::bind(&doSignalGuiServiceUrlWindowClose,dataModel), RBX::DataModelJob::Write);
}

HRESULT WebBrowserEvents::WindowClosing(DISPPARAMS __RPC_FAR *pDispParams)
{
	if (pDispParams)
		*((VARIANT_BOOL*)pDispParams->rgvarg[0].byref) = VARIANT_TRUE;

	rbxWebView->EndDialog(IDCANCEL);
	rbxWebView->setDialogActive(false);

	if(shared_ptr<RBX::Game> game = rbxWebView->getGame().lock())
	{
		signalGuiServiceUrlWindowClosed(game->getDataModel().get());
	}

	rbxWebView = NULL;

	return S_OK;
}

HRESULT WebBrowserEvents::BeforeNavigate2(_bstr_t URL, long Flags, _bstr_t TargetFrameName, VARIANT * PostData, _bstr_t Headers, VARIANT_BOOL * Cancel)
{
	std::string ulrStr = convert_w2s(std::wstring((wchar_t*)URL));
	return RBX::Http::trustCheckBrowser(ulrStr.c_str()) ? S_OK : E_FAIL;
}