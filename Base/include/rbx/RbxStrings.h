/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#ifndef Rbx_strcasestr
namespace RBX
{

/* GCC often has strcasestr(); if not, you can use the following */

/* borrowed these definitions from Apache */
#define ap_tolower(c) (tolower(((unsigned char)(c))))
#define ap_toupper(c) (toupper(((unsigned char)(c))))

	static const char *Rbx_strcasestr( const char *h, const char *n )
	{
		if( !h || !*h || !n || !*n ) { return 0; }
		char *a= (char*)h, *e=(char*)n;
		while( *a && *e ) {
			if( ap_toupper(*a) != ap_toupper(*e) ) {
				++h; a=(char*)h; e=(char*)n;
			} else {
				++a; ++e;
			}
		}
		return (const char *)(*e) ? 0 : h;

	}
	static inline const char *Rbx_strcasestr( const char *h, char *n )
	{
		return Rbx_strcasestr( h, static_cast<const char*>(n));
	}

	static inline const char *Rbx_strcasestr( char *h, const char *n )
	{
		return Rbx_strcasestr( static_cast<const char*>(h), n);
	}
}
#endif // defined Rbx_strcasestr

