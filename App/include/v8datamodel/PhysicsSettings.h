#pragma once

#include "V8DataModel/GlobalSettings.h"
#include "V8DataModel/DataModelJob.h"
#include "V8World/World.h"

namespace RBX {



	// A generic mechanism for displaying stats (like 3D FPS, network traffic, etc.)
	extern const char *const sPhysicsSettings;
	class PhysicsSettings 
		 : public GlobalAdvancedSettingsItem<PhysicsSettings, sPhysicsSettings>
	{
		double throttleAdjustTime;
        bool physicsAnalyzerState;

	public:
		PhysicsSettings();

		bool getThrottleAt30Fps() const;
		void setThrottleAt30Fps(bool value);

		EThrottle::EThrottleType getEThrottle() const;
		void setEThrottle(EThrottle::EThrottleType value);

		bool getShowEPhysicsOwners() const;
		void setShowEPhysicsOwners(bool value);

		bool getShowEPhysicsRegions() const;
		void setShowEPhysicsRegions(bool value);

		bool getShowMechanisms() const;
		void setShowMechanisms(bool value);

		bool getShowAssemblies() const;
		void setShowAssemblies(bool value);

		bool getShowAnchoredParts() const;
		void setShowAnchoredParts(bool value);

		bool getShowUnalignedParts() const;
		void setShowUnalignedParts(bool value);

		bool getShowContactPoints() const;
		void setShowContactPoints(bool value);

        bool getShowJointCoordinates() const;
        void setShowJointCoordinates(bool value);

		bool getHighlightSleepParts() const;
		void setHighlightSleepParts(bool value);

		bool getHighlightAwakeParts() const;
		void setHighlightAwakeParts(bool value);

		bool getShowBodyTypes() const;
		void setShowBodyTypes(bool value);

		bool getShowPartCoordinateFrames() const;
		void setShowPartCoordinateFrames(bool value);

		bool getShowModelCoordinateFrames() const;
		void setShowModelCoordinateFrames(bool value);

		bool getShowWorldCoordinateFrame() const;
		void setShowWorldCoordinateFrame(bool value);

		bool getAllowSleep() const;
		void setAllowSleep(bool value);

		bool getParallelPhysics() const;
		void setParallelPhysics(bool value);

		bool getShowSpanningTree() const;
		void setShowSpanningTree(bool value);

		bool getShowReceiveAge() const;
		void setShowReceiveAge(bool value);

		float getWaterViscosity() const;
		void setWaterViscosity(float value);

		double getThrottleAdjustTime() const {return throttleAdjustTime;}
		void setThrottleAdjustTime(double value);

		bool getRenderDecompositionData() const;
		void setRenderDecompositionData(bool value);

        void setPhysicsAnalyzerState( bool enabled );
        bool getPhysicsAnalyzerState() const { return physicsAnalyzerState; }
	};


} // namespace
