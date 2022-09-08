#pragma once

#include "v8kernel/ContactConnector.h"

namespace RBX {

	class BuoyancyConnector : public RBX::ContactConnector
	{
	private:
		Vector3 position;	// force application point in object space
		Vector3 force;
		Vector3 torque;

		float floatDistance;
		float sinkDistance;
		float submergeRatio;

	protected:
		/*override*/ void computeForce( bool throttling );
		/*override*/ virtual KernelType getConnectorKernelType() const { return Connector::BUOYANCY; }

	public:
		void updateContactPoint();  // Only for debug rendering now

		const Vector3& getPosition() { return position; } 
		const Vector3 getWorldPosition();
		void setForce( const Vector3& f ) { force = f; }
		void setTorque( const Vector3& t ) { torque = t; }
		void getWaterBand( float& up, float& down ) { up = floatDistance; down = sinkDistance; }
		void setWaterBand( const float& up, const float& down ) { floatDistance = up; sinkDistance = down; }
		float getSubMergeRatio() { return submergeRatio; }
		void setSubMergeRatio( const float& ratio ) { submergeRatio = ratio; }

		BuoyancyConnector(Body* b0, Body* b1, const Vector3& pos);
	};
}