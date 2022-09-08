#pragma once
#include "resource.h"

#include <atlcom.h>
#include <Exdispid.h> // platform SDK header
#include "v8datamodel/DataModel.h"
#include "boost/shared_ptr.hpp"
#include "boost/function.hpp"

class WebBrowserAxDialog; // forward declaration

class DWebBrowserEventsImpl : public DWebBrowserEvents
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
	HRESULT NavigateComplete(_bstr_t URL);

	// members
	WebBrowserAxDialog *m_cpParent; // any time a IWebBrowser instance is needed
public:
	void SetParent(WebBrowserAxDialog *pParent) { m_cpParent = pParent; }
};

class WebBrowserAxDialog : 
	public CAxDialogImpl<WebBrowserAxDialog>,
	public IDocHostUIHandler,
	public IDispatch
{
	ULONG m_cRef;
	HICON m_hIcon;
	std::string url;
	boost::shared_ptr<RBX::DataModel> dataModel;
	boost::function<void(bool)> enableUpload;
	
	void DoPostImage(std::string filename, std::string seostr);
	void UploadVideo(std::string token, SHORT doPost, SHORT postSetting, std::string title);
	void DoUploadVideo(std::string token, std::string title, std::string seostr, int placeId);

	//video upload data
	std::string videoSEOInfo;
	bool siteSEO;
	std::string videoTitle;
	std::string youtubeToken;
	//captured video file name
	std::string fileName;

	DWebBrowserEventsImpl m_events;
public:
    enum { IDD = IDD_UPLOADVIDEODIALOG };

    BEGIN_MSG_MAP(WebBrowserAxDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
    END_MSG_MAP()

	WebBrowserAxDialog(const std::string& url, boost::shared_ptr<RBX::DataModel> dataModel, boost::function<void(bool)> enableUpload);
	WebBrowserAxDialog(const std::string& url, boost::shared_ptr<RBX::DataModel> dataModel);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	void SetFileName(std::string file) { fileName = file;}

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