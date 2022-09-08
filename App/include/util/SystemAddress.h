#pragma once

// RakNet doesn't namespace this - ouch
namespace RBX { 

class SystemAddress
{
public:
	///The peer address from inet_addr.
	unsigned int binaryAddress;
	///The port number
	unsigned short port;

	SystemAddress()
		: binaryAddress(0xFFFFFFFF)
		, port(0xFFFF)
	{}

	SystemAddress(unsigned int binaryAddress, unsigned short port)
		: binaryAddress(binaryAddress)
		, port(port)
	{}

	bool empty() const
	{
		return binaryAddress == 0xFFFFFFFF && port == 0xFFFF;
	}

	void clear()
	{
		binaryAddress = 0xFFFFFFFF;
		port = 0xFFFF;
	}

	unsigned int getAddress() const { return binaryAddress; }
	unsigned short getPort() const { return port; }

	bool operator == ( const SystemAddress& right ) const;
	bool operator != ( const SystemAddress& right ) const;
	bool operator > ( const SystemAddress& right ) const;
	bool operator < ( const SystemAddress& right ) const;
};



}	// namespace

