#pragma once

#include <boost/function.hpp>

#include "util/HttpAsync.h"

/// Keeps authentication cookies in sync between Protected Model IE and unprotected IE
class AuthenticationMarshallar
{
	std::string domain;
public:
	AuthenticationMarshallar(const char* domain);
	~AuthenticationMarshallar(void);
	std::string Authenticate(const char* url, const char* ticket);
	RBX::HttpFuture AuthenticateAsync(const char* url, const char* ticket);
};
