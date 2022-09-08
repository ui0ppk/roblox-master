/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/Contact.h"

namespace RBX {

	class PolyConnector;

	//typedef RBX::FixedArray<PolyConnector*, 12> ConnectorArray;		// TODO - should only ever need 8
	typedef RBX::FixedArray<PolyConnector*, CONTACT_ARRAY_SIZE> ConnectorArray;		// TODO - should only ever need 8

	class PolyContact : public Contact 
	{
	private:
		ConnectorArray polyConnectors;

		void removeAllConnectorsFromKernel();
		void putAllConnectorsInKernel();
		void updateClosestFeatures();
		float worstFeatureOverlap();
		void deleteConnectors(ConnectorArray& deleteConnectors);
		void matchClosestFeatures(ConnectorArray& newConnectors);
		PolyConnector* matchClosestFeature(PolyConnector* newConnector);
		void updateContactPoints();

		// Contact
		/*override*/ void deleteAllConnectors();
		/*override*/ int numConnectors() const					{return polyConnectors.size();}
		/*override*/ ContactConnector* getConnector(int i);
		/*override*/ bool computeIsColliding(float overlapIgnored); 
		/*override*/ bool stepContact();

		/*implement*/ virtual void findClosestFeatures(ConnectorArray& newConnectors) = 0;

	public:
		PolyContact(Primitive* p0, Primitive* p1)
			: Contact(p0, p1)
		{}

		~PolyContact();
	};



} // namespace