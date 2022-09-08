#pragma once

#include "Util/G3DCore.h"
#include "rbx/rbxTime.h"
#include <boost/circular_buffer.hpp>
#include "Util/Average.h"
#include "Util/Velocity.h"
#include "rbx/RunningAverage.h"

namespace RBX {

	#define NUM_MAX_HISTORY 8
	#define NUM_BUFFER_NODES 2

	class PartInstance;

	// This class keeps a list of frames and the time they were set. 

	class PathInterpolatedCFrame
	{
	private:

		struct FrameInfo
		{
			CoordinateFrame coordinateFrame;
			Velocity velocity;
			RemoteTime remoteTime;	// in sender's time scale
			FrameInfo() {}
			FrameInfo(const CoordinateFrame &cf, const Velocity& vel, const Time& local, const RemoteTime& remote) : coordinateFrame(cf), velocity(vel), remoteTime(remote) {}
		};

		FrameInfo prevFrame;
		FrameInfo lastStartFrame;

		bool beingMoved;
		int uiStepId;
		double localToRemoteTimeOffset;	// subtract local time by this value to get remote time
		RunningAverage<> avgInterval;
		Time prevStepTime;
		boost::circular_buffer_space_optimized<FrameInfo> frameInfos;
		
		float targetDelayInSeconds;
		int targetFrame;

		// for analytics
		double lastTargetDelayValue;
		double targetDelayDeltaMax;


		inline const CoordinateFrame& recordAndReturn(const CoordinateFrame& value, const Time& local, const RemoteTime& remote)
		{
			beingMoved = true;
			prevFrame.coordinateFrame = value;
			prevFrame.remoteTime = remote;
			return prevFrame.coordinateFrame;
		}

		inline const CoordinateFrame& recordAndReturnHermite(const CoordinateFrame& value, const FrameInfo& startFrame, const Time& local, const RemoteTime& remote)
		{
			beingMoved = true;
			lastStartFrame = startFrame;
			prevFrame.coordinateFrame = value;
			prevFrame.remoteTime = remote;
			return prevFrame.coordinateFrame;
		}

		const CoordinateFrame& interpolate( const Time& now, const Time& targetTime, const unsigned int& upper, const PartInstance* part = NULL);
		const CoordinateFrame& interpolateHermiteSpline( const Time& now, const Time& targetTime, const unsigned int& upper, const PartInstance* part = NULL);
		RemoteTime computeSampleTargetTime( const Time& now);

	public:
		PathInterpolatedCFrame();
		~PathInterpolatedCFrame() {}

		void clearHistory();

		// timeStamp is time this value was set. If coming from network, the value should be time it was send from the server
		void setValue(PartInstance* part, const CoordinateFrame& value, const Velocity& vel, const RemoteTime& timeStamp, Time now, float localTimeOffest, int numNodesAhead);

		void setTargetDelay(float value);

		void setUiStepId(int id) {uiStepId = id;}
		int getUiStepId() const {return uiStepId;}
		
		double getLocalToRemoteTimeOffset() {return localToRemoteTimeOffset;}
		
		void setRenderedFrame(const CoordinateFrame& value);
		void setRenderedFrame(const CoordinateFrame& value, const RemoteTime& remoteTime);
		
		CoordinateFrame computeValue(PartInstance* part, const Time& t);
		CoordinateFrame getLastComputedValue() const { return prevFrame.coordinateFrame; }
		
		bool isBeingMovedByInterpolator() const { return beingMoved; }
		
		Color3 getSampleIntervalColor() const;
		float getSampleInterval() const;

		void renderPath(Adorn* adorn);
	};

} // namespace
