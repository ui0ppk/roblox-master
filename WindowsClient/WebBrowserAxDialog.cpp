#include "stdafx.h"
#include "WebBrowserAxDialog.h"
#include <Exdispid.h> // platform SDK header
#include "util/FileSystem.h"
#include "util/Statistics.h"
#include "util/Http.h"
#include "v8datamodel/DataModel.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/GameBasicSettings.h"
#include "v8datamodel/PlayerGui.h"
#include "format_string.h"

static const std::string titlePrefix("<media:title type=\"plain\">");
static const std::string titlePostfix("</media:title>");

WebBrowserAxDialog::WebBrowserAxDialog(const std::string& url, boost::shared_ptr<RBX::DataModel> dataModel, boost::function<void(bool)> enableUpload) 
	: CAxDialogImpl()
	, url(url)
	, dataModel(dataModel)
	, m_cRef(1)
	, siteSEO(false)
	, enableUpload(enableUpload)
{
}

WebBrowserAxDialog::WebBrowserAxDialog(const std::string& url, boost::shared_ptr<RBX::DataModel> dataModel)
	: CAxDialogImpl()
	, url(url)
	, dataModel(dataModel)
	, m_cRef(1)
	, siteSEO(false)
{
}

HRESULT WebBrowserAxDialog::QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
{
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
		return E_NOTIMPL;
	}
}

ULONG WebBrowserAxDialog::AddRef(void)
{
	return InterlockedIncrement(&m_cRef);
}

ULONG WebBrowserAxDialog::Release(void)
{
	return InterlockedDecrement(&m_cRef);
}

HRESULT WebBrowserAxDialog::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
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

HRESULT WebBrowserAxDialog::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
	pInfo->dwFlags |= 
		DOCHOSTUIFLAG_NO3DBORDER | 
		ROBLOX_BROWSERFLAGS |
		0;

	return S_OK;
}

HRESULT WebBrowserAxDialog::ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::HideUI(void)
{
	return S_OK;
}

HRESULT WebBrowserAxDialog::UpdateUI(void)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::EnableModeless(BOOL fEnable)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::OnDocWindowActivate(BOOL fActivate)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::OnFrameWindowActivate(BOOL fActivate)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::GetExternal(IDispatch **ppDispatch)
{
	*ppDispatch = this;
	return S_OK;
}

HRESULT WebBrowserAxDialog::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
	return E_NOTIMPL;
}

HRESULT WebBrowserAxDialog::GetTypeInfoCount(UINT* pctinfo)
{
	if (pctinfo == NULL)
		return E_POINTER;

	*pctinfo = 1;
	return S_OK;
}

HRESULT WebBrowserAxDialog::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
	return S_OK;
}

HRESULT WebBrowserAxDialog::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
	if (wcscmp(*rgszNames, L"CheckAppHost") == 0)
	{
		rgDispId[0] = 0;
	}
	else if (wcscmp(*rgszNames, L"AppHostOpenVideoFolder") == 0) 
	{
		rgDispId[0] = 1;
	}
	else if (wcscmp(*rgszNames, L"AppHostUploadVideo") == 0) 
	{
		rgDispId[0] = 2;
	}
	else if (wcscmp(*rgszNames, L"AppHostOpenPicFolder") == 0)
	{
		rgDispId[0] = 3;
	}
	else if (wcscmp(*rgszNames, L"AppHostPostImage") == 0)
	{
		rgDispId[0] = 4;
	}

	return S_OK;
}

HRESULT WebBrowserAxDialog::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
{
	if (wFlags & DISPATCH_METHOD) 
	{
		if (dispIdMember == 1)
		{
			ShellExecuteW(NULL, L"open", RBX::FileSystem::getUserDirectory(true, RBX::DirVideo).native().c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		else if (dispIdMember == 2)
		{
			BSTR title_ = pDispParams->rgvarg[0].bstrVal;
			SHORT postSetting = pDispParams->rgvarg[1].iVal;
			SHORT doPost = pDispParams->rgvarg[2].iVal;
			BSTR token_ = pDispParams->rgvarg[3].bstrVal;

			std::string tile = convert_w2s(title_);
			std::string token = convert_w2s(token_);

			UploadVideo(token, doPost, postSetting, tile);
		}
		else if (dispIdMember == 3)
		{
			ShellExecuteW(NULL, L"open", RBX::FileSystem::getUserDirectory(true, RBX::DirPicture).native().c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		else if (dispIdMember == 4)
		{
			// This should only happen when the user clicks the "Do not show this window again" button
			RBX::GameSettings::singleton().setPostImageSetting(RBX::GameSettings::NEVER);
		}
	}	
	return S_OK;
}

static void PostImageFinished(std::string *response, std::exception *ex, weak_ptr<RBX::DataModel> weakDataModel)
{
	if(shared_ptr<RBX::DataModel> dataModel = weakDataModel.lock())
	{
		if ((ex == NULL) && (response->compare("ok") == 0))
		{
			dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Image uploaded to Facebook", 2), RBX::DataModelJob::Write);
		}
		else
		{
			dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Failed to upload image", 2), RBX::DataModelJob::Write);
			RBX::GameSettings::singleton().setPostImageSetting(RBX::GameSettings::ASK);
		}

		dataModel->submitTask(boost::bind(&RBX::DataModel::ScreenshotUploadTask, weak_ptr<RBX::DataModel>(dataModel), true), RBX::DataModelJob::Write);
	}
}

void WebBrowserAxDialog::DoPostImage(std::string filename, std::string seostr)
{
	try
	{
		std::string url = RBX::format("%s/UploadMedia/DoPostImage.ashx?from=client", ::GetBaseURL().c_str());
		RBX::Http http(url);
		// in case the seo info contains nothing but whitespaces, add a line break to prevent facebook from returning errors
		http.additionalHeaders[seostr] = seostr + "%0D%0A";
		shared_ptr<std::ifstream> in(new std::ifstream);
		in->open(filename.c_str(), std::ios::binary);
		if (in->fail())
		{
			dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Failed to upload image", 2), RBX::DataModelJob::Write);
		}
		else
		{
			dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Uploading image ...", 0), RBX::DataModelJob::Write);

			dataModel->submitTask(boost::bind(&RBX::DataModel::ScreenshotUploadTask, weak_ptr<RBX::DataModel>(dataModel), false), RBX::DataModelJob::Write);
			http.post(in, RBX::Http::kContentTypeDefaultUnspecified, false,
				boost::bind(&PostImageFinished, _1, _2, weak_ptr<RBX::DataModel>(dataModel)));
		}
	}
	catch (...)
	{
		dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Failed to upload image", 2), RBX::DataModelJob::Write);

		dataModel->submitTask(boost::bind(&RBX::DataModel::ScreenshotUploadTask, weak_ptr<RBX::DataModel>(dataModel), true), RBX::DataModelJob::Write);
	}
}

DWORD ThreadDoUploadVideo(shared_ptr<RBX::DataModel> dataModel, bool siteSEO, std::string videoTitle, std::string videoSEOInfo, std::string fileName, std::string youtubeToken, boost::function<void(bool)> enableUpload) 
{
	shared_ptr<RBX::CoreGuiService> coreGuiService;
	if(dataModel)
		coreGuiService = shared_from(dataModel->find<RBX::CoreGuiService>());

	if(coreGuiService)
	{
		dataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Uploading video ...", 0), RBX::DataModelJob::Write);
	}

	RBX::Http http(RBX::format("http://uploads.gdata.youtube.com/feeds/api/users/default/uploads"));
	try{
		std::string request1;
		if (!siteSEO)
		{
			std::string requestTmp(
				"--f93dcbA3\r\n"
				"Content-Type: application/atom+xml; charset=UTF-8\r\n"
				"\r\n"
				"<?xml version=\"1.0\"?>\r\n"
				"<entry xmlns=\"http://www.w3.org/2005/Atom\"\r\n"
				"xmlns:media=\"http://search.yahoo.com/mrss/\"\r\n"
				"xmlns:yt=\"http://gdata.youtube.com/schemas/2007\">\r\n"
				"<media:group>\r\n"
				"<media:title type=\"plain\">" + videoTitle + "</media:title>\r\n"
				"<media:description type=\"plain\">\r\n"
				"" + videoSEOInfo + "\r\n"
				"For more games visit http://www.roblox.com\r\n"
				"</media:description>\r\n"
				"<media:category\r\n"
				"scheme=\"http://gdata.youtube.com/schemas/2007/categories.cat\">Games\r\n"
				"</media:category>\r\n"
				"<media:keywords>ROBLOX, video, free game, online virtual world</media:keywords>\r\n"
				"</media:group>\r\n"
				"</entry>\r\n"
				"--f93dcbA3\r\n"
				"Content-Type: video/avi\r\n"
				"Content-Transfer-Encoding: binary\r\n"
				"\r\n"
				);
			request1 = requestTmp;
		}
		else
		{
			int s = videoSEOInfo.find(titlePrefix) + titlePrefix.length();
			int e = videoSEOInfo.find(titlePostfix);
			videoSEOInfo = videoSEOInfo.replace(s, e - s, videoTitle);

			request1 = RBX::format("--f93dcbA3\r\n"
				"Content-Type: application/atom+xml; charset=UTF-8\r\n"
				"\r\n"
				"%s\r\n"
				"--f93dcbA3\r\n"
				"Content-Type: video/avi\r\n"
				"Content-Transfer-Encoding: binary\r\n"
				"\r\n", videoSEOInfo.c_str());
		}

		std::stringstream buffer;		
		buffer << request1;

		RBXASSERT(!fileName.empty());
		std::ifstream file(fileName.c_str(), std::ios::in | std::ios::binary);
		if ( file.is_open() )
		{
			buffer << file.rdbuf();
			file.close();
		}
		else
		{
			if(coreGuiService)
				dataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Failed to upload video", 2), RBX::DataModelJob::Write);

			enableUpload(true);
			return 1;
		}

		std::string request2("\r\n--f93dcbA3--\r\n");

		buffer << request2;

		buffer << '\0';

		http.additionalHeaders["Authorization"] = "AuthSub token=\"" + youtubeToken + "\"";
		http.additionalHeaders["GData-Version"] = "2";
		http.additionalHeaders["X-GData-Key"] = "key=AI39si5sZKe6qAobFgnT9UFGXq9bBO7mUCsK3_cWy_LJmgKDtl-GOMHNNV_Bh7Jk7KqDX7vI8D30jFHwnu8RJcDmcJN47yPW7A";
		http.additionalHeaders["Slug"] = "roblox.avi";
		http.additionalHeaders["Connection"] = "close";
		http.additionalHeaders["Content-Length"] = RBX::format("%d", buffer.str().length());

		std::string response;
		http.post(buffer, "multipart/related; boundary=\"f93dcbA3\"", false, response);

		// Check the response to see if the upload succeeded.
		// TODO: better way of checking if the upload succeeded from youtube?
		int pos = response.find("videoid");
		if(coreGuiService){
			if (pos == std::string::npos){
				dataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Failed to upload video", 2), RBX::DataModelJob::Write);
			}
			else{
				dataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Video uploaded to YouTube", 2), RBX::DataModelJob::Write);;
			}
		}
	}
	catch (...) {
		if(coreGuiService)
			dataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Failed to upload video", 2), RBX::DataModelJob::Write);
	}

	enableUpload(true);
	return 1;
}

void WebBrowserAxDialog::DoUploadVideo(std::string token, std::string title, std::string seostr, int placeId)
{
	std::string titelString = title;

	if (seostr.empty())
	{
		siteSEO = false;
		if (placeId > 0) 
		{
			videoSEOInfo = format_string("To play this game, please visit: http://www.roblox.com/item.aspx?id=%d&amp;rbx_source=youtube&amp;rbx_medium=uservideo", placeId);
		} else {
			videoSEOInfo = seostr;
		}
	}
	else
	{
		videoSEOInfo = seostr;
		siteSEO = true;
	}

	videoTitle = titelString;
	if (videoTitle.length() == 0)
		videoTitle = "ROBLOX ROCKS!";

	youtubeToken = token;
	
	enableUpload(false);
	boost::thread(boost::bind(ThreadDoUploadVideo, dataModel, siteSEO, videoTitle, videoSEOInfo, fileName, youtubeToken, enableUpload));
}

void WebBrowserAxDialog::UploadVideo(std::string  token, SHORT doPost, SHORT postSetting, std::string  title)
{
	RBX::GameBasicSettings::singleton().setUploadVideoSetting((RBX::GameSettings::UploadSetting)postSetting);

	if (doPost == 1) {
		int placeId = dataModel->getPlaceID();
		if (dataModel->isVideoSEOInfoSet())
		{
			DoUploadVideo(token, title, dataModel->getVideoSEOInfo(), placeId);
		}
		else
		{
			DoUploadVideo(token, title, "", placeId);
		}
	}
}

LRESULT WebBrowserAxDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CenterWindow();

	m_hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINDOW_ICON));
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_events.SetParent(this);

	SHDocVw::IWebBrowserAppPtr pWebBrowser = NULL;
	HRESULT hr = GetDlgControl(IDC_EXPLORER1, __uuidof(SHDocVw::IWebBrowserAppPtr), (void**)&pWebBrowser);

	if (SUCCEEDED(hr)) {
		CComQIPtr<IConnectionPointContainer, &IID_IConnectionPointContainer> cpc(pWebBrowser);
		CComPtr<IConnectionPoint> cp1;
		hr = cpc->FindConnectionPoint(__uuidof(SHDocVw::DWebBrowserEventsPtr), &cp1);
		DWORD dwCookie;
		hr = cp1->Advise((LPUNKNOWN)&m_events, &dwCookie);

		CComPtr<IConnectionPoint> cp2;
		hr = cpc->FindConnectionPoint(__uuidof(SHDocVw::DWebBrowserEvents2Ptr), &cp2);
		hr = cp2->Advise((LPUNKNOWN)&m_events, &dwCookie);
		
		hr = pWebBrowser->Navigate( _bstr_t(url.c_str()) );
	} else {
		MessageBox("Failed to open web browser", "Error", MB_OK);
		return FALSE;
	}

	return TRUE;    // let the system set the focus
}

LRESULT WebBrowserAxDialog::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	EndDialog(IDCANCEL);
	return 0;
}

HRESULT __stdcall DWebBrowserEventsImpl::QueryInterface(REFIID riid, LPVOID* ppv)
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

ULONG __stdcall DWebBrowserEventsImpl::AddRef() { return 1;}
ULONG __stdcall DWebBrowserEventsImpl::Release() { return 0;}

// IDispatch methods
HRESULT __stdcall DWebBrowserEventsImpl::GetTypeInfoCount(UINT* pctinfo)
{ 
	return E_NOTIMPL; 
}

HRESULT __stdcall DWebBrowserEventsImpl::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{ 
	return E_NOTIMPL; 
}

HRESULT __stdcall DWebBrowserEventsImpl::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{ 
	return E_NOTIMPL; 
}
        
HRESULT __stdcall DWebBrowserEventsImpl::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
{ 
	// proces OnBeforeNavigate
	if (dispIdMember == DISPID_BEFORENAVIGATE)
	{
		BeforeNavigate(_bstr_t(pDispParams->rgvarg[5].bstrVal), 0, _bstr_t(pDispParams->rgvarg[3].bstrVal), NULL, _bstr_t(""), NULL);
	}
	if (dispIdMember == DISPID_BEFORENAVIGATE2) 
	{
		BeforeNavigate2(_bstr_t(pDispParams->rgvarg[5].bstrVal), 0, _bstr_t(pDispParams->rgvarg[3].bstrVal), NULL, _bstr_t(""), NULL);
	}
	else if (dispIdMember == DISPID_NAVIGATECOMPLETE)
	{
		NavigateComplete(_bstr_t(pDispParams->rgvarg[0].bstrVal));
	}
	else if (dispIdMember == DISPID_WINDOWCLOSING) 
	{
		*((VARIANT_BOOL*)pDispParams->rgvarg[0].byref) = VARIANT_TRUE;
		m_cpParent->EndDialog(IDCANCEL);
	}

	return NOERROR;
}


// Methods:
HRESULT DWebBrowserEventsImpl::BeforeNavigate(_bstr_t URL, long Flags, _bstr_t TargetFrameName, VARIANT * PostData, _bstr_t Headers, VARIANT_BOOL * Cancel)
{
	return S_OK;
}

HRESULT DWebBrowserEventsImpl::BeforeNavigate2(_bstr_t URL, long Flags, _bstr_t TargetFrameName, VARIANT * PostData, _bstr_t Headers, VARIANT_BOOL * Cancel)
{
	std::string ulrStr = convert_w2s(std::wstring((wchar_t*)URL));
	return RBX::Http::trustCheckBrowser(ulrStr.c_str()) ? S_OK : E_FAIL;
}

HRESULT DWebBrowserEventsImpl::NavigateComplete(_bstr_t URL) 
{ 
	SHDocVw::IWebBrowserAppPtr pWebBrowser = NULL;
	HRESULT hr = m_cpParent->GetDlgControl(IDC_EXPLORER1, __uuidof(SHDocVw::IWebBrowserAppPtr), (void**)&pWebBrowser);

	CComPtr<IDispatch> spDoc;
	hr = pWebBrowser->get_Document(&spDoc);

	CComPtr<ICustomDoc> customDoc = CComQIPtr<ICustomDoc>(spDoc);

	customDoc->SetUIHandler(m_cpParent);

	return S_OK; 
}
