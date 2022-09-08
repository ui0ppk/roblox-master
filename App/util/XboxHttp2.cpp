#include "stdafx.h"

#if defined(RBX_PLATFORM_DURANGO)

#include "Util/Http.h"

#define NOMINMAX
// includes required for xbox
#include <ixmlhttprequest2.h>

#include <boost/intrusive_ptr.hpp>

#include "util/HttpPlatformImpl.h"  // TODO this is here for cache. Maybe move cache to separate file?

#include "rbx/Profiler.h"

DYNAMIC_FASTINT(ExternalHttpResponseTimeoutMillis)
DYNAMIC_FASTINT(ExternalHttpResponseSizeLimitKB)

enum
{
    kForceHttpAssets = 0,      // force all https assets to http
    kReportHttpTimings = 0,    // dprintf every url fetch time
    kReportHttpRedirects = 1,  // report redirects to http
};

void dprintf( const char* fmt, ... );

namespace boost
{
    void intrusive_ptr_add_ref(const IUnknown* p) { const_cast<IUnknown*>(p)->AddRef(); }
    void intrusive_ptr_release(const IUnknown* p) { const_cast<IUnknown*>(p)->Release(); }
}

using boost::intrusive_ptr;

static std::wstring s2ws( const std::string& s )
{
    return std::wstring(s.begin(), s.end());
}
std::string ws2s(const wchar_t* data);

class Callback : public IXMLHTTPRequest2Callback
{
    volatile long       refcnt;
    const std::string   url;
    std::string         response;
    int                 status;
    HRESULT             error;
    HANDLE              evt;
    std::wstring        redir;
    bool                cacheValid;

    enum { kWaitTimeoutMs = 120 * 1000 };
    
    virtual ~Callback()
    {
        CloseHandle(evt);
    }

public:

    // timings
    double timeStart;
    double timeRedirect; // only the last redirect matters really.
    double timeHeaders;
    double timeData;
    double timeDone;


    Callback( const std::string& url_ ): url(url_) 
    {
        refcnt = 1;
        status = -1;
        error = S_OK;
        evt = CreateEventA(0,TRUE,0,0);
        cacheValid = false;

        timeStart    = RBX::Time::nowFastSec();
        timeRedirect = 0;
        timeHeaders  = -1;
        timeData     = -1;
        timeDone     = -1;
    }

    bool wait()
    {
        if ( WAIT_OBJECT_0 == WaitForSingleObject(evt, kWaitTimeoutMs) )
            return true; // ok

        return false; // timeout or worse
    }

    // can call only once!
    // wait() before calling!
    void getResponse( std::string& into ) 
    {
        return into.swap(response);
    }

    int getStatus()
    {
        return status;
    }

    bool getCacheValid()
    {
        return cacheValid;
    }

    int getHresult()
    {
        return error;
    }

    const std::wstring& getRedir() { return redir; }

    //////////////////////////////////////////////////////////////////////////
    // callback stuff


    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject) override
    {
        AddRef();
        *ppvObject = 0;
        if( riid == IID_IUnknown ) { *ppvObject = static_cast<IUnknown*>(this); return S_OK; }
        if( riid == __uuidof(IXMLHTTPRequest2Callback) ) { *ppvObject = this; return S_OK; }
        Release();
        return E_NOINTERFACE;
    }

    virtual ULONG __stdcall AddRef() override
    {
        InterlockedIncrement(&refcnt);
        return 0;
    }

    virtual ULONG __stdcall Release() override
    {
        if( !InterlockedDecrement(&refcnt) ) delete this;
        return 0;
    }

    virtual HRESULT __stdcall OnRedirect( IXMLHTTPRequest2 *pXHR, const WCHAR *pwszRedirectUrl) override
    {
        redir = pwszRedirectUrl;
        timeRedirect = RBX::Time::nowFastSec() - timeStart;
        if(kForceHttpAssets) return S_OK; // don't spam

        if( kReportHttpRedirects && wcsncmp(pwszRedirectUrl, L"http://", 7) == 0 )
        {
            dprintf("HTTP:  %s\nREDIR: %S\n", url.c_str(), pwszRedirectUrl );
        }
        
        std::string redirect = ws2s(pwszRedirectUrl);
        RBX::HttpPlatformImpl::Cache::CacheResult cacheResult = RBX::HttpPlatformImpl::Cache::CacheResult::open(url.c_str(), redirect.c_str());

        if (cacheResult.isValid())
        {
            timeData = RBX::Time::nowFastSec() - timeStart;
            response.clear();
            response = cacheResult.getResponseBody().toString();
            timeDone = timeData = RBX::Time::nowFastSec() - timeStart;
            cacheValid = true;

            pXHR->Abort();
            SetEvent(evt);
        }

        return S_OK;
    }

    virtual HRESULT __stdcall OnHeadersAvailable( IXMLHTTPRequest2 *pXHR, DWORD dwStatus, const WCHAR *pwszStatus) override
    {
        status = dwStatus;
        timeHeaders = RBX::Time::nowFastSec() - timeStart;
        return S_OK;
    }

    virtual HRESULT __stdcall OnDataAvailable( IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream) override
    {
        return S_OK;
    }

    virtual HRESULT __stdcall OnResponseReceived( IXMLHTTPRequest2 *pXHR, ISequentialStream *pResponseStream) override
    {
        enum { kChunkSize = 32767 };
        unsigned long long int bytes = 0; // yeah yeah
        HRESULT hr;
        ULONG rd;

        timeData = RBX::Time::nowFastSec() - timeStart;

        response.clear();

        do{
            response.insert( response.end(), kChunkSize, '\0' );
            hr = pResponseStream->Read( bytes + (char*)response.data(), kChunkSize, &rd );
            if( FAILED(hr) )
                return E_FAIL;
            bytes += rd;
        }
        while(rd == kChunkSize );
        response.resize( bytes );

        timeDone = timeData = RBX::Time::nowFastSec() - timeStart;

        SetEvent(evt);
        return S_OK;
    }

    virtual HRESULT __stdcall OnError( IXMLHTTPRequest2 *pXHR, HRESULT hrError) override
    {
        error = hrError;
        SetEvent(evt);
        return S_OK;
    }

};


class Stream : public ISequentialStream
{
    volatile long refcnt;
    std::istream& stream;

    virtual ~Stream(){}

public:
    Stream( std::istream& s ): stream(s), refcnt(1) {}

    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject) override
    {
        AddRef();
        *ppvObject = 0;
        if( riid == IID_IUnknown ) { *ppvObject = static_cast<IUnknown*>(this); return S_OK; }
        if( riid == IID_ISequentialStream ) { *ppvObject = this; return S_OK; }
        Release();
        return E_NOINTERFACE;
    }

    virtual ULONG __stdcall AddRef() override
    {
        InterlockedIncrement(&refcnt); return 0;
    }

    virtual ULONG __stdcall Release() override
    {
        if( !InterlockedDecrement(&refcnt) ) delete this;
        return 0;
    }

    virtual HRESULT __stdcall Read( void *pv, ULONG cb, ULONG *pcbRead ) override
    {
        stream.read((char*)pv, cb);
        *pcbRead = stream.gcount();
        return *pcbRead == cb ? S_OK : S_FALSE;
    }

    virtual HRESULT __stdcall Write( const void *pv, ULONG cb, ULONG *pcbWritten ) override
    {
        RBXASSERT(false);
        return E_NOTIMPL;
    }
};

static void addHeader( IXMLHTTPRequest2* request, const std::wstring& header, const std::wstring value, const std::string& theUrl )
{
    HRESULT hr = request->SetRequestHeader( header.c_str(), value.c_str() );
    if( FAILED(hr) )
        throw RBX::runtime_error( "http failed to add header '%S' : '%S' (0x%x) [%s]", header.c_str(), value.c_str(), hr, theUrl.c_str());
}

namespace RBX
{
    void Http::httpGetPostXbox(bool isPost, std::istream& data, const std::string& contentType, bool compressData, const HttpAux::AdditionalHeaders& additionalHeaders, bool externalRequest, HttpCache::Policy cachePolicy, std::string& response)
    {
        std::string theUrl = alternateUrl.empty() ? url : alternateUrl;
        HRESULT hr;
        intrusive_ptr< Callback > callback;
        intrusive_ptr< IXMLHTTPRequest2 > request;

        RBXPROFILER_SCOPE("Http", __FUNCTION__);
        RBXPROFILER_LABEL("Http", theUrl.c_str());

        RBX::Timer<RBX::Time::Precise> timer;
        
        static const std::wstring userAgentHeader   = L"User-Agent";
        static const std::wstring userAgentValue    = s2ws(Http::rbxUserAgent) + L" ROBLOX Xbox App 1.0.0";
        static const std::wstring accessKeyHeader   = L"XboxAccessKey";
        static const std::wstring accessKeyValue    = L"b9309a51-7b54-458c-ad73-7b2c287497e2";
        static const std::wstring contentTypeHeader = L"Content-Type";
        static const std::wstring csrfTokenHeader   = L"X-CSRF-TOKEN";


        if( kForceHttpAssets && theUrl.find("https://") != std::string::npos && theUrl.find("/asset") != std::string::npos )
            theUrl.replace(0,5, "http");

        if (theUrl.size()==0)
            throw RBX::runtime_error("http empty url");

        if (!Http::trustCheck(theUrl.c_str(), externalRequest))
            throw RBX::runtime_error("http trust check failed for %s", theUrl.c_str());

        if(0) dprintf("http %s %s\n", (isPost?"POST":"GET"), theUrl.c_str());

        hr = ::CoCreateInstance( __uuidof(FreeThreadedXMLHTTP60), 0, CLSCTX_SERVER, __uuidof(IXMLHTTPRequest2), (void**)&request );
        if(FAILED(hr) || !request)
            throw RBX::runtime_error("http is not available at the moment (0x%x) [%s]", hr, theUrl.c_str());

        callback = boost::intrusive_ptr< Callback >( new Callback(theUrl), false );

        hr = request->Open( (isPost ? L"POST" : L"GET"), s2ws(theUrl).c_str(), callback.get(), 0, 0, 0, 0 );
        if (FAILED(hr))
            throw RBX::runtime_error("http could not open connection (0x%x) [%s]", hr, theUrl.c_str());

        addHeader( request.get(), userAgentHeader, userAgentValue, theUrl );

        if( !externalRequest )
            addHeader( request.get(), accessKeyHeader, accessKeyValue, theUrl );

        for( auto& x : additionalHeaders )
        {
            addHeader( request.get(), s2ws(x.first), s2ws(x.second), theUrl );
        }

        if( isPost )
        {
            addHeader( request.get(), contentTypeHeader, s2ws(contentType), theUrl );
            if (!externalRequest)
            {
                addHeader( request.get(), csrfTokenHeader, s2ws(getLastCsrfToken()), theUrl );
            }
        }

#ifdef RBX_XBOX_SITETEST1
        // fun with snicker doodles (sitetest1 auth)
        __int64 sysTime;
        GetSystemTimeAsFileTime( (FILETIME*)&sysTime);
        sysTime += (__int64)10000000 * 3600 * 24 * 365 * 100;

        XHR_COOKIE site1Cookie = {};
        site1Cookie.pwszUrl = L".sitetest1.robloxlabs.com/";
        site1Cookie.pwszName = L"SnickerdoodleConstraint";
        site1Cookie.pwszValue = L"";
        site1Cookie.ftExpires = (FILETIME&)sysTime;
        site1Cookie.pwszP3PPolicy = L"";
        site1Cookie.dwFlags = XHR_COOKIE_IS_SESSION;

        DWORD state = 0;
        hr = request->SetCookie(&site1Cookie, &state);
#endif

        request->SetProperty(XHR_PROP_ONDATA_THRESHOLD, XHR_PROP_ONDATA_NEVER);

        // send the request
        if( !isPost )
        {
            // GET
            hr = request->Send( 0, 0 );
            if( FAILED(hr) )
                throw RBX::runtime_error("http failed to send (0x%x) [%s]", hr, theUrl.c_str() );
        }
        else
        {
            // POST
            data.clear(); // this does not clear the stream!
            const unsigned long long int size = data.seekg(0, std::ios_base::end).tellg();
            data.seekg(0);

            hr = request->Send( new Stream(data), size );
            if( FAILED(hr) )
                RBX::runtime_error("http send failed (0x%x) [%s]", hr, theUrl.c_str() );
        }

        // get the results
        int statusCode = -1;

        if( !callback->wait() )
        {
            // timeout
            request->Abort();
            throw RBX::runtime_error( "http TIMEOUT [%s]", theUrl.c_str() );
        }
        
        hr = callback->getHresult();
        if (FAILED(hr))
            throw RBX::runtime_error( "http error hr=0x%x [%s]", hr, theUrl.c_str() );

        statusCode = callback->getStatus();
        callback->getResponse(response);

        if( statusCode == 200 || statusCode == 201 || callback->getCacheValid() )
        {
            if (!isPost && cachePolicy == HttpCache::Policy::PolicyFinalRedirect && !callback->getCacheValid())
            {
                wchar_t* requestHeaders; 
                request->GetAllResponseHeaders(&requestHeaders);  
                std::string headersStr = ws2s(requestHeaders);
                CoTaskMemFree(requestHeaders);

                HttpPlatformImpl::Cache::Data headers(headersStr.data(), headersStr.size());
                HttpPlatformImpl::Cache::Data body(response.data(), response.size());

                std::string redirUrl = ws2s(callback->getRedir().data());

                try
                {
                    HttpPlatformImpl::Cache::CacheResult::update(url.c_str(), redirUrl.c_str(), statusCode, headers, body);
                }
                catch (const std::runtime_error& e)
                {
                    dprintf("Cache update failed: %s", e.what());
                }
            }

            if( kReportHttpTimings )
            {
                const wchar_t* redir = callback->getRedir().c_str();
                dprintf( "HTTP %s %d %.0f ms (%.0f ms / %.0f ms / %.0f ms) : %s%s%S\n",  (isPost?"POST":"GET"), statusCode, 1000*callback->timeDone,
                     1000*callback->timeRedirect, 1000*callback->timeHeaders, 1000*callback->timeData, 
                     theUrl.c_str(), (*redir?" ---->  ":""), redir);
            }
            return; // success!
        }

        // error handling and stuff
        //special case: CSRF token. Important security stuff. Talk to our web team for more info. ("Hey, what's CSRF?")
        if( statusCode == 403 && isPost && !externalRequest)
        {
            WCHAR* header = 0;
            HRESULT hr = request->GetResponseHeader(L"X-CSRF-TOKEN",&header);
            if( FAILED(hr) )
                throw http_status_error(statusCode, theUrl);

            setLastCsrfToken(std::string(header, header + wcslen(header)));
            CoTaskMemFree(header);
            response.clear();
            return httpGetPostXbox( isPost, data, contentType, compressData, additionalHeaders, externalRequest, cachePolicy,  response);
        }
        throw http_status_error( statusCode, theUrl );
    }
}


// stubs for studio-related things
namespace RBX{ namespace HttpPlatformImpl {

void init(Http::CookieSharingPolicy cookieSharingPolicy) { RBXASSERT(0); }
void setCookiesForDomain(const std::string& domain, const std::string& cookies) { RBXASSERT(0); }
void getCookiesForDomain(const std::string& domain, std::string& cookies) { RBXASSERT(0); }
boost::filesystem::path getRobloxCookieJarPath() { RBXASSERT(0); return ""; }
void setProxy(const std::string& host, long port) { RBXASSERT(0); }
void perform(HttpOptions& options, std::string& response) { RBXASSERT(0); }

}}

#endif
