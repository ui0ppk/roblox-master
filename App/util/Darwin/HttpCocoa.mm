//
//  HttpImpl.m - Mac Specific HTTP implementation
//  Note: This is very much a work in progress..
//

#include <sstream>

#define BOOST_IOSTREAMS_NO_LIB
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace io = boost::iostreams;

// later to make gcc happy
#import "HttpCocoa.h"

#include "RbxFormat.h"
#include "Http.h"
#include "FastLog.h"

static NSURL* createSanitizedURL(std::string url)
{
    // Windows HTTP implementation strips trailing spaces from the URL; NSUrl treats spaces as illegal, so let's strip them as well
    while (url.size() > 0 && isspace(url[url.size() - 1]))
        url.resize(url.size() - 1);
    
    // strip # anchor
    size_t firstAnchor = url.find('#');
    if (firstAnchor != std::string::npos)
    {
        url.resize(firstAnchor);
    }
    return [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
}
/**
 *
 */

@interface MacHttpController : NSObject
{
	BOOL shouldKeepRunning;
	BOOL compressedContent;
	NSRunLoop *privateRunLoop;
	NSURLConnection *theConnection;
	NSMutableData *receivedData;
	NSMutableData *postData;
	NSURL *url;
	NSInteger errorCode;
	RBX::HttpAux::AdditionalHeaders headers;
    @public std::string responseCsrfToken;
}

// 
@property (assign) NSURL *url;

//
- (id)initWithUrl:(const std::string &)aURL additionalHeaders:(const RBX::HttpAux::AdditionalHeaders&) headers;
- (void)setPostDataFromStream:(std::istream&) stream;
- (void)setPostCompressedDataFromString:(std::string&) str;
- (void)dealloc;
- (NSInteger)doGetPost: (const std::string &)authDomainUrl contentType:(const std::string&)ct useDefaultTimeout:(bool)useDefaultTimeout requestTimeoutMillis:(int)requestTimeoutMillis;
- (void)configureRequest:(NSMutableURLRequest*)request;
- (void)setAuthDomain:(const std::string &)domain withr:(NSMutableURLRequest*) r;

- (NSMutableData *)receivedData;

/**
 * delegate callbacks
 */

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSHTTPURLResponse *)response;
- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data;
- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error;
- (void)connectionDidFinishLoading:(NSURLConnection *)connection;
- (NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse;

// Mac 10.6 and 10.7 couldn't recognize the certificate for GT2, resulting in SSL errors.
// In order to get things going, we need to ignore those errors. This is being done only for the test sites.
- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace;
- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge;

@end


/**
 *
 */

@implementation MacHttpController

static NSString* kHttpRunLoopMode = @"RobloxHttpController";

@synthesize url;

- (NSMutableData *)receivedData
{
	return receivedData;
}

- (id)initWithUrl: (const std::string &) aURL additionalHeaders: (const RBX::HttpAux::AdditionalHeaders&) h
{
    self = [super init];
	
    if (self) {
		shouldKeepRunning = YES;
		privateRunLoop = [NSRunLoop currentRunLoop];
		self.url = createSanitizedURL(aURL);
		receivedData = [[NSMutableData data] retain];
		postData = nil;
		compressedContent = NO;
		
		errorCode = 0;
		
		headers = h;
		
	}
	
    return self;
}

- (void)setPostDataFromStream:(std::istream&) stream
{
	postData = [[NSMutableData data] retain];
	// TODO: This is slow. Use something more intelligent.
	// better yet, bind the istream to an NS stream using setHTTPBodyStream
	while (true)
	{
		char c;
		stream.get(c);
		if (stream.eof())
			break;
		[postData appendBytes:&c length:1];
	}
}

- (void)setPostCompressedDataFromString:(std::string&) str
{
	compressedContent = YES;
	postData = [[NSMutableData data] retain];
	// TODO: This is slow. Use something more intelligent.
	// better yet, bind the istream to an NS stream using setHTTPBodyStream
	for (int i=0; i<str.size(); ++i)
	{
		char c = str[i];
		[postData appendBytes:&c length:1];
	}
}

- (void)dealloc 
{
	[receivedData release];
	[super dealloc];
}


- (void)setAuthDomain:(const std::string &)d withr:(NSMutableURLRequest*) r
{
	NSString *domainStr = [NSString stringWithCString:d.c_str() encoding:[NSString defaultCStringEncoding]];
	[r setValue:domainStr forHTTPHeaderField:@"RBXAuthenticationNegotiation"];
}

- (void)configureRequest:(NSMutableURLRequest*) request
{
	[request setValue:@"gzip" forHTTPHeaderField:@"Accept-Encoding"]; 
	
	if (compressedContent)
		[request setValue:@"gzip" forHTTPHeaderField:@"Content-Encoding"];
	
	for (RBX::HttpAux::AdditionalHeaders::const_iterator iter = headers.begin(); iter != headers.end(); ++iter)
	{
		NSString* nsKey = [[NSString alloc] initWithUTF8String:iter->first.c_str() ];
		NSString* nsValue = [[NSString alloc] initWithUTF8String:iter->second.c_str() ];
		[request setValue:nsValue forHTTPHeaderField:nsKey]; 
	}
	
	if (postData)
	{
		[request setHTTPMethod:@"POST"];
		// TODO: Use setHTTPBodyStream
		[request setHTTPBody:postData];
	}
}

- (void)startConnectionWithRequest:(NSMutableURLRequest*) request
{
	theConnection = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:NO];

    [theConnection scheduleInRunLoop:privateRunLoop forMode:kHttpRunLoopMode];
    [theConnection start];
}

- (NSInteger)doGetPost: (const std::string &)authDomainUrl contentType:(const std::string&)ct useDefaultTimeout:(bool)useDefaultTimeout requestTimeoutMillis:(int)requestTimeoutMillis
{
	// TODO: Consider using http://allseeing-i.com/ASIHTTPRequest/
	
	NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL: url];

    if (!useDefaultTimeout)
        [request setTimeoutInterval:((float)requestTimeoutMillis / 1000.0f)];
    
    // todo: remove this, should be set elsewhere
#ifdef RBX_PLATFORM_IOS
    extern NSString* getUserAgentString();
    [request setValue:getUserAgentString() forHTTPHeaderField:@"User-Agent"];
#else
    [request setValue:@"Roblox/Darwin" forHTTPHeaderField:@"User-Agent"];
#endif

	[self configureRequest:request];
    if (postData)
    {
        NSString* nsValue = [[NSString alloc] initWithUTF8String:ct.c_str()];
        [request setValue:nsValue forHTTPHeaderField:@"Content-Type"];
    }
	if (!authDomainUrl.empty())
	{
		[self setAuthDomain:authDomainUrl withr:request];
	}
	
    [self startConnectionWithRequest:request];

	while (shouldKeepRunning && [privateRunLoop runMode:kHttpRunLoopMode beforeDate:[NSDate distantFuture]])
	{
	}

    [theConnection cancel];
    [theConnection release];
	
	return errorCode;
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
	errorCode = [error code];
	shouldKeepRunning=NO;
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
	[receivedData appendData: data];
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSHTTPURLResponse *)response
{
    int status = [response statusCode];

    if (errorCode == 0 && status != 0 && status != 200)
    {
        errorCode = status;
        shouldKeepRunning = NO;
        
        NSDictionary* dict = [response allHeaderFields];
        NSString* newToken = [dict objectForKey:@"X-CSRF-TOKEN"];
        if (newToken != nil)
        {
            responseCsrfToken = [newToken UTF8String];
        }
    }

	[receivedData setLength:0];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
	shouldKeepRunning=NO;
}

- (NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse
{
	if ([redirectResponse URL]) {
#if 1		
		// For 10.6.3/10.6.4 compatability, we cancel the old connection and create a new one with the
		//  redirect request
		RBXASSERT( connection == theConnection );
		[connection cancel];
		[connection release];
		
		NSMutableURLRequest *mutableRequest = [[request mutableCopy] autorelease];
		
		[self configureRequest:mutableRequest];
        [self startConnectionWithRequest:mutableRequest];
	
		return nil;
#else		
		// This is the desired code for handling redirect requests...
		//  However! due to an NSURLConnection instability in 10.6.3 and 10.6.4, it will cause 
		//  crashes on those versions
        
		NSMutableURLRequest *r = [[request mutableCopy] autorelease];
        [r setURL: [request URL]];
        return r;
#endif
    } else {
        return request;
    }
}

- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace
{
    if (kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber10_8)
    {
        if ([protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust])
            return [protectionSpace.host rangeOfString:@".robloxlabs.com"].location != NSNotFound;
    }
    return NO;
}

- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    if (kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber10_8)
    {
        if ([challenge.protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust] &&
            ([challenge.protectionSpace.host rangeOfString:@".robloxlabs.com"].location != NSNotFound))
        {
            // trust the credentials...
            [challenge.sender useCredential:[NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust] forAuthenticationChallenge:challenge];
            return;
        }
    }
    
    // continue with default handling
    [challenge.sender continueWithoutCredentialForAuthenticationChallenge:challenge];
}

@end

int rbx_isRobloxSite(const char* url)
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

	NSURL* nsUrl = createSanitizedURL(url);

    if (!nsUrl)
    {
        [pool drain];
        return 0;
    }

	NSString *host = [nsUrl host];
    
    if(!host)
    {
        [pool drain];
        return 0;
    }
    
	NSRange textRange;
	textRange =[host rangeOfString:@".roblox.com"];
    bool isRobloxUrl = textRange.location != NSNotFound;
    
    if (!isRobloxUrl)
    {
        textRange =[host rangeOfString:@".robloxlabs.com"];
        isRobloxUrl = textRange.location != NSNotFound;
    }
    
    [pool drain];
    
	return isRobloxUrl;
}

int rbx_isMoneySite(const char* url)
{
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	
	NSURL* nsUrl = createSanitizedURL(url);

    if (!nsUrl)
    {
        [pool drain];
        return 0;
    }

	NSString *host = [nsUrl host];
	
	NSRange textRange1 = [host rangeOfString:@".paypal.com"];
	NSRange textRange2 = [host rangeOfString:@".rixty.com"];
	
	[pool drain];
	
	return (textRange1.location != NSNotFound) || (textRange2.location != NSNotFound);
}

namespace RBX {
	namespace Cocoa {
		struct String_sink  : public io::sink {
			std::string& s;
			String_sink(std::string& s):s(s){}
			std::streamsize write(const char* s, std::streamsize n) 
			{
				this->s.append(s, n);
				return n;
			}
		};
		
		void httpGetPostCocoa(const std::string& url, const std::string &authDomainUrl, bool isPost, std::istream& data, const std::string& contentType, bool compressData, HttpAux::AdditionalHeaders& additionalHeaders, bool externalRequest, std::string& response, bool useDefaultTimeout, int requestTimeoutMillis)
		{
			NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
            
            const bool useCsrfHeaders = isPost && !externalRequest;
            std::string currentCsrfToken;
            if (useCsrfHeaders)
            {
                data.seekg(std::istream::beg);
                currentCsrfToken = Http::getLastCsrfToken();
                if (!currentCsrfToken.empty())
                {
                    additionalHeaders["X-CSRF-TOKEN"] = currentCsrfToken;
                }
            }

			MacHttpController *macHttp= [[MacHttpController alloc] initWithUrl: url additionalHeaders: additionalHeaders];
			
			if (isPost)
            {
				if ( compressData )
                {
					
					std::string uploadData;
					try
					{
						const io::gzip_params gzipParams = io::gzip::default_compression;
						io::filtering_ostream out;
						if (compressData)
						{
							out.push(io::gzip_compressor(gzipParams));
						}
						out.push(String_sink(uploadData));
						io::copy(data, out);
					}
					catch (io::gzip_error& e)
					{
						int e1 = e.error();
						int e2 = e.zlib_error_code();
						throw RBX::runtime_error("Upload GZip error %d, ZLib error %d, \"%s\"", e1, e2, url.c_str());
					}
					
					[macHttp setPostCompressedDataFromString:uploadData];
				}
				else
				{
					[macHttp setPostDataFromStream:data];
				}
			}
            
			// make request
			int code = [macHttp doGetPost:authDomainUrl contentType:contentType useDefaultTimeout:useDefaultTimeout requestTimeoutMillis:requestTimeoutMillis];
            
            std::string responseToken;
            if (code == 403 && useCsrfHeaders)
            {
                responseToken = macHttp->responseCsrfToken;
            }
				
            // Don't mess with encoding. We're supposed to return raw data
            NSMutableData* d = [macHttp receivedData];
            const char* bytes = (const char*)[d bytes];
            int l = [d length];
                
            response.assign(bytes, bytes + l);
			
			[macHttp release];

			[pool drain];
            
            if (useCsrfHeaders && !responseToken.empty() && responseToken != currentCsrfToken)
            {
                Http::setLastCsrfToken(responseToken);
                response.clear();
                httpGetPostCocoa(url, authDomainUrl, isPost, data, contentType, compressData, additionalHeaders, externalRequest, response, useDefaultTimeout, requestTimeoutMillis);
                return;
            }
			
			if (code)
            {
                throw RBX::http_status_error(code);
            }
		}
	}
}
