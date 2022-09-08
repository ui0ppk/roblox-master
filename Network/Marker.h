#pragma once

#include "V8Tree/Instance.h"

namespace RBX { 
	namespace Network {

extern const char* const sMarker;

class Marker
	: public RBX::DescribedNonCreatable<Marker, Instance, sMarker>
{
	bool returned;	// == the marker has made the round-trip
public:
	Marker();
	const long id;
	rbx::signal<void()> receivedSignal;
	void fireReturned();
};


	}
}

