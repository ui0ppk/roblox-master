#pragma once
#include "resource.h"

#include <atlcom.h>
#include <Exdispid.h> // platform SDK header
#include "boost/shared_ptr.hpp"
#include "boost/function.hpp"
#include "v8datamodel/Game.h"

class RbxWebView; // forward declaration

class WebBrowserEvents : public DWebBrowserEvents
{
	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDispatch methods
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo);
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId);
	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr);

	// Methods:
	HRESULT BeforeNavigate(_bstr_t URL, long Flags, _bstr_t TargetFrameName, VARIANT * PostData, _bstr_t Headers, VARIANT_BOOL * Cancel);
	HRESULT BeforeNavigate2(_bstr_t URL, long Flags, _bstr_t TargetFrameName, VARIANT * PostData, _bstr_t Headers, VARIANT_BOOL * Cancel);

	// members
	RbxWebView *rbxWebView; // any time a IWebBrowser instance is needed
public:
	void SetRbxWebView(RbxWebView *newView) { rbxWebView = newView; }
	HRESULT WindowClosing(DISPPARAMS __RPC_FAR *pDispParams = NULL);
};

class RbxWebView : 
	public CAxDialogImpl<RbxWebView>,
	public IDocHostUIHandler,
	public IDispatch
{
	ULONG m_cRef;
	HICON m_hIcon;
	std::string url;
	weak_ptr<RBX::Game> game;
	bool dialogActive;

	WebBrowserEvents webBrowserEvents;

public:
	enum { IDD = IDD_RBXWEBVIEW };

	BEGIN_MSG_MAP(RbxWebView)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
	END_MSG_MAP()

	RbxWebView(const std::string& url, shared_ptr<RBX::Game> game);

	weak_ptr<RBX::Game> getGame() {return game;}
	SHDocVw::IWebBrowserAppPtr RbxWebView::getWebBrowser();

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	void closeDialog();

	void PostNcDestroy();

	void setDialogActive(bool active) { dialogActive = active; }

	// IUnknown
	STDMETHOD(QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject));
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

	// IDispatch methods
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo);
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId);
	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr);

	// IDocHostUIHandler
	STDMETHOD(ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved));
	STDMETHOD(GetHostInfo(DOCHOSTUIINFO *pInfo));
	STDMETHOD(ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc));
	STDMETHOD(HideUI(void));
	STDMETHOD(UpdateUI(void));
	STDMETHOD(EnableModeless(BOOL fEnable));
	STDMETHOD(OnDocWindowActivate(BOOL fActivate));
	STDMETHOD(OnFrameWindowActivate(BOOL fActivate));
	STDMETHOD(ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow));
	STDMETHOD(TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID));
	STDMETHOD(GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw));
	STDMETHOD(GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget));
	STDMETHOD(GetExternal(IDispatch **ppDispatch));
	STDMETHOD(TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut));
	STDMETHOD(FilterDataObject(IDataObject *pDO, IDataObject **ppDORet));
};