/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/Contact.h"
#include "V8World/Mesh.h"
#include "Voxel/Util.h"
#include "Util/Vector3int32.h"

namespace RBX {

	class PolyConnector;

	const Vector3int16 kFaceDirectionToLocationOffset[7] =
	{
		Vector3int16( 1, 0, 0),
		Vector3int16( 0, 0, 1),
		Vector3int16(-1, 0, 0),
		Vector3int16( 0, 0,-1),
		Vector3int16( 0, 1, 0),
		Vector3int16( 0,-1, 0),
		Vector3int16( 0, 0, 0),
	};

	static inline Voxel::FaceDirection oppositeSideOffset(Voxel::FaceDirection f) 
	{
		static Voxel::FaceDirection OPPOSITES[6] = { Voxel::MinusX, Voxel::MinusZ, Voxel::PlusX, Voxel::PlusZ, Voxel::MinusY, Voxel::PlusY };
		return OPPOSITES[f];
	}

	class CellContact: public Contact 
	{
	public:
		CellContact(Primitive* p0, Primitive* p1, const Vector3int32& gridFeature)
			: Contact(p0, p1)
			, gridFeature(gridFeature)
		{}

        const Vector3int32& getGridFeature() const { return gridFeature; }

		virtual ContactType getContactType() const { return Contact_Cell; }

    protected:
        Vector3int32 gridFeature;
	};

	class CellMeshContact: public CellContact
	{
	public:
		typedef RBX::FixedArray<PolyConnector*, CONTACT_ARRAY_SIZE> ConnectorArray;		// TODO - should only ever need 8

    protected:
        POLY::Mesh* cellMesh;

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
		CellMeshContact(Primitive* p0, Primitive* p1, const Vector3int32& gridFeature)
			: CellContact(p0, p1, gridFeature)
            , cellMesh(NULL)
		{}

		~CellMeshContact();

        POLY::Mesh* getCellMesh(void) {return cellMesh;}

		bool cellFaceIsInterior(const Vector3int16& mainCellLoc, RBX::Voxel::FaceDirection faceDir);
	};
} // namespace
