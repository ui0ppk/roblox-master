#include "stdafx.h"

#include "Util/SystemAddress.h"

namespace RBX { 

bool SystemAddress::operator==( const RBX::SystemAddress& right ) const
{
	return binaryAddress == right.binaryAddress && port == right.port;
}

bool SystemAddress::operator!=( const RBX::SystemAddress& right ) const
{
	return binaryAddress != right.binaryAddress || port != right.port;
}

bool SystemAddress::operator>( const RBX::SystemAddress& right ) const
{
	return ( ( binaryAddress > right.binaryAddress ) || ( ( binaryAddress == right.binaryAddress ) && ( port > right.port ) ) );
}

bool SystemAddress::operator<( const RBX::SystemAddress& right ) const
{
	return ( ( binaryAddress < right.binaryAddress ) || ( ( binaryAddress == right.binaryAddress ) && ( port < right.port ) ) );
}

}	// namespace

