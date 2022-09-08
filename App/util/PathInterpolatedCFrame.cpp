#include "stdafx.h"

#include "v8datamodel/PartInstance.h"
#include "V8World/World.h"
#include "Util/PathInterpolatedCFrame.h"
#include "Util/Color.h"
#include "Util/Analytics.h"
#include "GfxBase/Adorn.h"
#include "AppDraw/Draw.h"
#include "AppDraw/DrawAdorn.h"

using namespace RBX;

FASTINTVARIABLE(InterpolationMaxDelayMSec, 500)
DYNAMIC_FASTFLAGVARIABLE(SimpleHermiteSplineInterpolate, false)
DYNAMIC_FASTFLAGVARIABLE(RemoveInterpolationSmoothing, false)
// General Rule -> Increasing Min buffer size adjusts for error when we have large spikes in latency.
// smaller buffer is better for memory, but lost packets may cause jitter.
DYNAMIC_FASTFLAGVARIABLE(CleanUpInterpolationTimestamps, false)
SYNCHRONIZED_FASTFLAGVARIABLE(PhysicsPacketSendWorldStepTimestamp, false)
DYNAMIC_FASTINTVARIABLE(InterpolationBufferMinSize, 2) 
DYNAMIC_FASTINTVARIABLE(InterpolationBufferMaxSize, 8) // Just more than the above.
DYNAMIC_FASTINTVARIABLE(InterpolationDelayFactorTenths, 15)
DYNAMIC_FASTFLAG(EnableMotionAnalytics)

LOGGROUP(ttMetricP1)

PathInterpolatedCFrame::PathInterpolatedCFrame() : 
	frameInfos(DFFlag::CleanUpInterpolationTimestamps ? DFInt::InterpolationBufferMaxSize : NUM_MAX_HISTORY), 
	localToRemoteTimeOffset(0.0), 
	uiStepId(0), 
	beingMoved(false), 
	targetFrame(0),
	targetDelayInSeconds(0.1),
	lastTargetDelayValue(0.0),
	targetDelayDeltaMax(0.0)

{
}

void PathInterpolatedCFrame::clearHistory()
{
	if (frameInfos.size() > 0)
	{
		prevFrame = frameInfos.back();
		frameInfos.clear();
		uiStepId = 0;
		localToRemoteTimeOffset = 0.0;
	}
}

void PathInterpolatedCFrame::setTargetDelay(float value) 
{ 
	float v = std::min(value, (float)FInt::InterpolationMaxDelayMSec / 1000.0f);

	if(DFFlag::EnableMotionAnalytics)
	{
		if(lastTargetDelayValue != 0.0)
		{
			double lastTargetDelaydelta = v-lastTargetDelayValue;
			if(lastTargetDelaydelta < 0)
				lastTargetDelaydelta = -lastTargetDelaydelta;
			if(lastTargetDelaydelta > targetDelayDeltaMax)
				targetDelayDeltaMax = lastTargetDelaydelta;
		}
		lastTargetDelayValue = v;
	}

	if (!DFFlag::CleanUpInterpolationTimestamps)
	{
		float lerp = 0.1f;
		if (v > targetDelayInSeconds)
			lerp = 0.8f;	
		targetDelayInSeconds = targetDelayInSeconds * (1.0f - lerp) + v * lerp;
	}
	else
	{
		targetDelayInSeconds = targetDelayInSeconds + .1 * (v - targetDelayInSeconds);
	}
}

void PathInterpolatedCFrame::setValue(PartInstance* part, const CoordinateFrame& value, const Velocity& vel, const RemoteTime& remoteSendTime, Time now, float localTimeOffset, int numNodesAhead)
{
	int numOutstandingNodes = DFFlag::CleanUpInterpolationTimestamps ? (numNodesAhead + DFInt::InterpolationBufferMinSize) : (numNodesAhead + NUM_BUFFER_NODES);
	if (numOutstandingNodes > (int)frameInfos.capacity())
	{
		numOutstandingNodes = frameInfos.capacity();
	}

	// Record the original send time on the the definitive simulator.
	// If it is forwarded by server then the sendtime on the original sending client should be recored.
	// we do this to preserve the original time intervals between position data points as sampled
	// on the sending client/server.

	// make sure part does not goto sleep in middle of interpolation
	beingMoved = true;
	part->notifyMoved();

	Time localSendTime = now - Time::Interval(localTimeOffset);

	if(DFFlag::CleanUpInterpolationTimestamps)
	{
		if(localToRemoteTimeOffset == 0)
		{
			localToRemoteTimeOffset = (localSendTime - remoteSendTime).seconds();
		}
		else
		{
			double ltrDelta = (localSendTime - remoteSendTime).seconds() - localToRemoteTimeOffset;
			localToRemoteTimeOffset += .1*ltrDelta;
		}
	}
	else
	{
		localToRemoteTimeOffset = (localSendTime - remoteSendTime).seconds();
	}

	double delta = 0.0;
	int numFrames = frameInfos.size();
	if (numFrames > 0)
	{
		delta = (remoteSendTime - frameInfos.back().remoteTime).seconds();
		avgInterval.sample(delta);
		if (delta < 0.0)
			return;       // skip out of order data points
		else if (delta == 0.0f)
		{
			// Just update the previous frame
			frameInfos.back().coordinateFrame = value;
			return;	
		}
	}

	if (numFrames > numOutstandingNodes)
	{
		for (int i=0; i < numFrames-numOutstandingNodes; i++)
		{
			if (DFFlag::CleanUpInterpolationTimestamps)
			{
				if (1 >= targetFrame)
					break;
				frameInfos.pop_front();
				targetFrame--;
			}
			else
			{
				if (i >= (targetFrame - 1))
					break;
				frameInfos.pop_front();
			}
		}
	}

	frameInfos.push_back(FrameInfo(value, vel, localSendTime, remoteSendTime));
	if (frameInfos.size() == 1)
	{
		// initial frame comes, seed the prevFrame
		prevFrame.remoteTime = remoteSendTime;
	}
}

RemoteTime PathInterpolatedCFrame::computeSampleTargetTime(const Time& now)
{
	if (DFFlag::CleanUpInterpolationTimestamps)
	{
		double averageFrameDelay = avgInterval.value();
		// We are recieving packets that are time-stamped at least LATENCY time in the past.
		// For interpolation to work, we also need to aim a few packets behind so that we're in between two packets, AND
		// so that we're behind enough so that we can still interpolate even if packet loss happens.

		// (now - localToRemoteTimeOffset) = current Time according to Raknet
		// averageFrameDelay			   = average 1/frquency of the incoming packets
		// targetDelayInSeconds	           = smoothed value driven by current latency of packetes.
		// We apply a DelayFactor here in order to make sure we are looking far back enough in time to be okay even with packet loss.
		return (now - Time::Interval(localToRemoteTimeOffset)) - Time::Interval(((double)DFInt::InterpolationDelayFactorTenths/10.0f) * (averageFrameDelay + targetDelayInSeconds) );
	}
	else
	{
		// time delta since last call
		double localDeltaTime = (now - prevStepTime).seconds();


		if (targetDelayInSeconds > 0)
		{
			// how far the projected target time is behind actual remote time
			double acutalDelay = ((now - Time::Interval(localToRemoteTimeOffset)) - (prevFrame.remoteTime + Time::Interval(localDeltaTime))).seconds();

			// scale delta time to meet target delay
			double deltaScalor = acutalDelay / targetDelayInSeconds;
			localDeltaTime *= deltaScalor;
		}

		prevStepTime = now;

		return prevFrame.remoteTime + Time::Interval(localDeltaTime);
	}
}

CoordinateFrame PathInterpolatedCFrame::computeValue(PartInstance* part, const Time& t)
{
	RemoteTime targetTime;

	if (frameInfos.size() == 0)
	{
		// nothing to interpolate to
		prevFrame.coordinateFrame = part->getCoordinateFrame();
		return prevFrame.coordinateFrame;
	}

	if (prevFrame.remoteTime == frameInfos.back().remoteTime)
		return prevFrame.coordinateFrame;	// already at last frame
	
	targetTime = computeSampleTargetTime(t);

	FASTLOG4F(FLog::ttMetricP1, "frameinfos %f  %f %f %f", (float)(frameInfos.size())/10.0, (targetTime - frameInfos.front().remoteTime).seconds(), (frameInfos.back().remoteTime - targetTime).seconds(), (frameInfos.back().remoteTime- frameInfos.front().remoteTime).seconds());
	//frameCircBuf size, target ahead of oldest, newest ahead of target, newest ahead of oldest

	// don't move backward in time
	if ( targetTime < prevFrame.remoteTime )
		return prevFrame.coordinateFrame;

	// find target frame
	unsigned int i;
	for ( i = 0; i < frameInfos.size(); ++i )
		if ( targetTime < frameInfos[i].remoteTime )
			break;

	if ( i == frameInfos.size() )
	{
		prevFrame.remoteTime = frameInfos.back().remoteTime;
		prevFrame.coordinateFrame = frameInfos.back().coordinateFrame;

		beingMoved = false;					// no more frames to move to (TODO: extrapolate)
		if (part->getNetworkIsSleeping())
			clearHistory();

		return prevFrame.coordinateFrame;
	} 
	if(DFFlag::EnableMotionAnalytics)
	{
		Primitive * PartPrimitive = RBX::PartInstance::getPrimitive(part);
		if(PartPrimitive)
		{
			World *PartWorld = PartPrimitive->getWorld();

			if(PartWorld)
			{	
				PartWorld->addFrameinfosStat(frameInfos.size(), i,
					10.0 * (frameInfos.back().remoteTime - targetTime).seconds(),10.0 * (frameInfos.back().remoteTime- frameInfos.front().remoteTime).seconds(),10.0 * targetDelayDeltaMax, 1.0);
			}
		}
	}


	if (DFFlag::SimpleHermiteSplineInterpolate)
		return interpolateHermiteSpline(t, targetTime, i, part);

	return interpolate(t, targetTime, i, part);
}

const CoordinateFrame& PathInterpolatedCFrame::interpolateHermiteSpline( const Time& now, const Time& targetTime, const unsigned int& upper, const PartInstance* part)
{
	/// SEE http://cubic.org/docs/hermite.htm 
	
	targetFrame = upper;
	int frameBefore = upper - 1;
	FrameInfo& goalFrame = frameInfos[upper];
	FrameInfo& startFrame = frameBefore >= 0 ? frameInfos[frameBefore] : lastStartFrame ;
	float deltaTimeFrame = (goalFrame.remoteTime - startFrame.remoteTime).seconds();
	float a = (targetTime - startFrame.remoteTime).seconds() / deltaTimeFrame;
	float h1 = 2*a*a*a - 3*a*a + 1;
	float h2 = -2*a*a*a + 3*a*a;
	float h3 = a*a*a - 2*a*a + a;
	float h4 = a*a*a - a*a;

	Vector3 startAxisAngle;
	float   startAxisRadians;
	Vector3 goalAxisAngle;
	float   goalAxisRadians;
	startFrame.coordinateFrame.rotation.toAxisAngle(startAxisAngle, startAxisRadians);
	goalFrame.coordinateFrame.rotation.toAxisAngle(goalAxisAngle, goalAxisRadians);
	Vector3 startPosition = startFrame.coordinateFrame.translation;
	Vector3 goalPosition  = goalFrame.coordinateFrame.translation;

	float angleDiff = ((startAxisRadians) * startAxisAngle - goalAxisRadians * goalAxisAngle).squaredMagnitude();
	if (angleDiff > 2*(Math::pi()*Math::pi()))
	{
		// When you interpolate between 355 degrees and 5 degrees, it goes counter-clockwise.
		// To make the system go Clock-Wise/Shortest Path, we have to increase goal angle to 365 from 5.
		goalAxisRadians = 2*Math::pi() - goalAxisRadians;
		goalAxisAngle *= -1;
	}

	Vector3 hermiteLinearPos		= h1 * startPosition + h2 * goalPosition;
	Vector3 hermiteAngularAxisAngle = h1 * startAxisRadians * startAxisAngle + h2 * goalAxisRadians * goalAxisAngle;
	Vector3 hermiteLinearVel		= h3 * startFrame.velocity.linear + h4 * goalFrame.velocity.linear;
	Vector3 hermiteAngularVel		= h3 * startFrame.velocity.rotational + h4 * goalFrame.velocity.rotational;

	Vector3 finalPosition = hermiteLinearPos + hermiteLinearVel * deltaTimeFrame;
	Matrix3 finalRotation = Matrix3::fillRotation(hermiteAngularAxisAngle + hermiteAngularVel * deltaTimeFrame);
	
	return recordAndReturnHermite(CoordinateFrame(finalRotation, finalPosition), startFrame, now, targetTime);
}

const CoordinateFrame& PathInterpolatedCFrame::interpolate( const Time& now, const Time& targetTime, const unsigned int& upper, const PartInstance* part)
{
	targetFrame = upper;
    float timeToTarget = (frameInfos[upper].remoteTime - prevFrame.remoteTime).seconds();
	float alpha = (targetTime - prevFrame.remoteTime).seconds() / timeToTarget;
	RBXASSERT(alpha <= 1.0);

    if (part)
    {
        Vector3 deltaVelocity = (frameInfos[upper].coordinateFrame.translation - prevFrame.coordinateFrame.translation) / timeToTarget;
        float partVelocityLength = part->getVelocity().linear.length();
        float deltaVelocityLength = deltaVelocity.length();
        if (deltaVelocityLength > 100.f && deltaVelocityLength > partVelocityLength * 5.f)
        {
			//StandardOut::singleton()->printf(MESSAGE_INFO, "deltaVelocity too large: %f, teleporting instead of interpolation", deltaVelocity.length());
			return recordAndReturn(frameInfos[upper].coordinateFrame.translation, now, targetTime);
        }
    }

	CoordinateFrame cf = prevFrame.coordinateFrame.lerp(frameInfos[upper].coordinateFrame, alpha);
	if (!DFFlag::RemoveInterpolationSmoothing)
	{
		if (0 < upper && upper < frameInfos.size() - 1)
		{	
			// Bi-linear interpolation
			CoordinateFrame cf2;
			unsigned int lookAhead = upper + 1;
			// upper-1 <= prev < target < upper < lookAhead : Interpolate between prev and loadAhead

			alpha = (targetTime - prevFrame.remoteTime).seconds() / (frameInfos[lookAhead].remoteTime - prevFrame.remoteTime).seconds();

			RBXASSERT(alpha <= 1.0);
			cf2 = prevFrame.coordinateFrame.lerp(frameInfos[lookAhead].coordinateFrame, alpha);

			alpha = (targetTime - frameInfos[upper - 1].remoteTime).seconds() / (frameInfos[upper].remoteTime - frameInfos[upper - 1].remoteTime).seconds();

			cf = cf.lerp(cf2, alpha);
		}
	}

	return recordAndReturn(cf, now, targetTime);
}

void PathInterpolatedCFrame::setRenderedFrame(const CoordinateFrame& value)
{
	prevFrame.coordinateFrame = value;
}

void PathInterpolatedCFrame::setRenderedFrame(const CoordinateFrame& value, const RemoteTime& remoteTime)
{
	setRenderedFrame(value);
	prevFrame.remoteTime = remoteTime;
}

Color3 PathInterpolatedCFrame::getSampleIntervalColor() const
{
	return Color::colorFromTemperature(static_cast<float>(getSampleInterval() * 0.2f));
}

float PathInterpolatedCFrame::getSampleInterval() const
{
	return avgInterval.value();
}

void PathInterpolatedCFrame::renderPath(Adorn* adorn)
{
	adorn->setObjectToWorldMatrix(CoordinateFrame());
	for (unsigned int i = 0; i < frameInfos.size(); ++i )
	{
		Color3 color = Color::lightBlue();
		if (i == targetFrame)
			color = Color::red();

		Extents box = Extents::fromCenterRadius(frameInfos[i].coordinateFrame.translation, 0.5f);
		DrawAdorn::outlineBox(adorn, box, color);
	}
}