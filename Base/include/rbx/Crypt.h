#pragma once

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include <windows.h>
#include <wincrypt.h>
#endif

#include <string>

namespace RBX
{
	class Crypt
	{
#if defined (_WIN32) && !defined(RBX_PLATFORM_DURANGO)
	HCRYPTPROV context;
	HCRYPTKEY key;
#endif
	public:
		Crypt();
		~Crypt();
		void verifySignatureBase64(std::string message, std::string signatureBase64);
	};
}


