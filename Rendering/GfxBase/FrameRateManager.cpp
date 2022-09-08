
#include "GfxBase/FrameRateManager.h"
#include "GfxBase/RenderCaps.h"

#include "rbx/debug.h"
#include "rbx/Log.h"
#include "FastLog.h"
#include "RbxFormat.h"
#include "rbx/TaskScheduler.h"

#include "Util/RobloxGoogleAnalytics.h"
#include "Util/Math.h"
#include "rbx/SystemUtil.h"

#include <functional>

LOGGROUP(FRM)

FASTFLAGVARIABLE(DebugSSAOForce, false)
FASTINTVARIABLE(FRMRecomputeDistanceFrameDelay, 100)
FASTINTVARIABLE(RenderGBufferMinQLvl, 20) // 14 for later

namespace RBX {

/////////////////////////////////////////////////////////////////////////////
// Tweakable section
/////////////////////////////////////////////////////////////////////////////

static const int AveragingFrames = 40;
static const int VarianceFrames = 20;

static const int LockStepDelayDown = 100;		// Number of frames to wait after going a quality level down
static const int LockStepDelayUp = 150;		// Number of frames to wait after going a quality level up
static const double RenderFraction = 0.625;
static const double VarianceLimit = 5;

static const double MultiCoreRenderBottleneckFraction = 0.8; 
static const double MultiCorePrepareFraction = 0.3;

static const int SwitchCounterMax = 10;

static const int SettleDelay = 20; // Number of milliseconds that we consider level stable

// Fast backoff filter:
// If during LockStepDelayDown your average frame length (averaged by FastBackoffFPSAve) is more than MaxFrameLen...
//	... consecutively for WatchingFrames frames
//  you're going to be backed off to previous level
//  ... with StepLevel increased by FastBackoffStepLevelIncrement

#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
static const double FastBackoffMaxFrameLen = 40;	// 25 FPS
#else
static const double FastBackoffMaxFrameLen = 60;	// 16.6 FPS
#endif

static const int FastBackoffFPSAve = 10;			// frames
static const int FastBackoffWatchingFrames = 5;		// frames

static const int SqDistanceBump = 50;

struct THROTTLE_LOCKSTEP
{
	double framerate;
	float distance;
	int blockCount;
	float shadingDistance;
    int textureAnisotropy;
	SSAOLevel ssao;
	float lightGridRadius;
	bool lightAllowNonFixed;
	unsigned lightChunkBudget;
	int throttlingFactor; // Matches physics throttling table in World.cpp: - 0/8, 1/8, 1/4, 1/3, 1/2, 2/3, 3/4, 7/8, 15/16
	double StepHill;
	double MaxStepHill;
};

inline float sqrf(float value)
{
	return value*value;
}

static THROTTLE_LOCKSTEP kLockstepTable60FPS [] = {	

    // Quality levels:
    { std::numeric_limits<double>::max(),	100000.0f,	  1000000,  300,	1, ssaoNone,	 512,	true,  4,	0,	10, 10},   // Level 0: Studio (scpAlways is a hack to enable shadowing in Ogre)

    { std::numeric_limits<double>::max(),   200.0f,		 	500,	0,		1, ssaoNone,	 256,	false, 1,	8,	5, 10 },  // Level 1
    {				    80 /* 12 FPS */,	250.0f,	     	600,	0,		1, ssaoNone,	 256,	false, 1,	7,	5,	10 },  // Level 2
    {				    66 /* 15 FPS */,	300.0f,	     	600,	0,		1, ssaoNone,	 256,	false, 1,	6,	5,	10 },  // Level 3
    {				    50 /* 20 FPS */,	450.0f,	     	700,	0,		1, ssaoNone,	 293,	false, 1,	5,	5,	10 },  // Level 4
    {					42 /* 25 FPS */,	470.0f,	     	800,	40,		1, ssaoNone,	 330,	false, 1,	4,	5,	10,},  // Level 5
    {					40 /* 25 FPS */,	550.0f,	     	900,	40,		1, ssaoNone,	 367,	false, 2,	3,	5,	10,},  // Level 6
    {					35 /* 28 FPS */,	570.0f,	     	1000,	40,		1, ssaoNone,	 404,	false, 2,	2,	5,	10,},  // Level 7
    {					35 /* 28 FPS */,	600.0f,	     	1000,	50,		1, ssaoNone,	 441,	false, 2,	1,	5,	10,},  // Level 8
    {					35 /* 28 FPS */,	600.0f,	     	1000,	60,		1, ssaoNone,	 478,	false, 2,	0,	5,	10,},  // Level 9
    {					35 /* 28 FPS */,	700.0f,	     	1500,	70,		2, ssaoNone,    512,	true,  2,	0,	5,	10,},  // Level 10
    {					35 /* 28 FPS */,	1131.0f,		2000,	80,		2, ssaoNone,	 512,	true,  2,	0,	5,	10,},  // Level 11
    {					33 /* 30 FPS */,	1600.0f,		3000,	90,		2, ssaoNone,    512,	true,  2,	0,	4,	8,},  // Level 12
    {					30 /* 33 FPS */,	2263.0f,		4000,	120,	2, ssaoNone,  	 512,	true,  2,	0,	4,  8,},  // Level 13
    {					27 /* 37 FPS */,	2263.0f,		5000,	150,	4, ssaoNone,  	 512,	true,  2,	0,	4,	8,},  // Level 14
    {					25 /* 40 FPS */,	3200.0f,		7000,	180,	4, ssaoNone,	 512,	true,  2,	0,	4,	8,},  // Level 15
    {					23 /* 43 FPS */,	4525.0f,		10000,	210,	4, ssaoNone,	 512,	true,  4,	0,	4,	8,},  // Level 16
    {					20 /* 50 FPS */,	6400.0f,		20000,	240,	4, ssaoNone,	 512,	true,  4,	0,	4,	8,},  // Level 17
    {					19 /* 60 FPS */,	9051.0f,		30000,	270,	8, ssaoNone,	 512,	true,  4,	0,	3,	7,},  // Level 18
    {					19 /* 60 FPS */,	100000.0f,		100000,	300,	8, ssaoNone,  	 512,	true,  4,	0,	3,	7,},  // Level 19
    // Introducing SSAO - big step hill, bigger MaxStepHill, blank first	  				  
    {					19 /* 60 FPS */,	100000.0f,	    100000,	300,	8, ssaoFullBlank,  512,	true,  4, 0,	6,	10,},  // Level 20
    // And then turn it on
    {					19 /* 60 FPS */,	100000.0f,	    100000,	300,	8, ssaoFull,   	512,	true,  4, 0,	2,	2,}    // Level 21
};


static THROTTLE_LOCKSTEP kLockstepTable30FPS [] = {	

	// Quality levels:
	{ std::numeric_limits<double>::max(),	100000.0f,	  1000000,	300,	1, ssaoNone,	 512,	true,  4,	0,	10, 10},   // Level 0: Studio (scpAlways is a hack to enable shadowing in Ogre)
														    				   												
	{ std::numeric_limits<double>::max(),   200.0f,		 	500,	0,		1, ssaoNone,	 256,	false, 1,	8,	5,  10 },  // Level 1
	{				    50 /* 20 FPS */,	250.0f,	     	600,	0,		1, ssaoNone,	 256,	false, 1,	7,	5,	10 },  // Level 2
	{				    35 /* 28 FPS */,	300.0f,	     	600,	0,		1, ssaoNone,	 256,	false, 1,	6,	5,	10 },  // Level 3
	{				    35 /* 28 FPS */,	450.0f,	     	700,	0,		1, ssaoNone,	 293,	false, 1,	5,	5,	10 },  // Level 4
	{					35 /* 28 FPS */,	470.0f,	     	800,	40,		1, ssaoNone,	 330,	false, 1,	4,	5,	10,},  // Level 5
	{					35 /* 28 FPS */,	550.0f,	     	900,	40,		1, ssaoNone,	 367,	false, 2,	3,	5,	10,},  // Level 6
	{					35 /* 28 FPS */,	570.0f,	     	1000,	40,		1, ssaoNone,	 404,	false, 2,	2,	5,	10,},  // Level 7
	{					35 /* 28 FPS */,	600.0f,	     	1000,	50,		1, ssaoNone,	 441,	false, 2,	1,	5,	10,},  // Level 8
	{					35 /* 28 FPS */,	600.0f,	     	1000,	60,		1, ssaoNone,	 478,	false, 2,	0,	5,	10,},  // Level 9
	{					35 /* 28 FPS */,	700.0f,	     	1500,	70,		2, ssaoNone,    512,	true,  2,	0,	5,	10,},  // Level 10
	{					35 /* 28 FPS */,	1131.0f,		2000,	80,		2, ssaoNone,	 512,	true,  2,	0,	5,	10,},  // Level 11
	{					35 /* 28 FPS */,	1600.0f,		3000,	90,		2, ssaoNone,  	 512,	true,  2,	0,	5,	10,},  // Level 12
	{					35 /* 28 FPS */,	2263.0f,		4000,	120,	2, ssaoNone,  	 512,	true,  2,	0,	5,  10,},  // Level 13
	{					35 /* 28 FPS */,	2263.0f,		5000,	150,	4, ssaoNone,  	 512,	true,  2,	0,	5,	10,},  // Level 14
	{					35 /* 28 FPS */,	3200.0f,		7000,	180,	4, ssaoNone,	 512,	true,  2,	0,	5,	10,},  // Level 15
	{					35 /* 28 FPS */,	4525.0f,		10000,	210,	4, ssaoNone,	 512,	true,  4,	0,	5,	10,},  // Level 16
	{					35 /* 28 FPS */,	6400.0f,		20000,	240,	4, ssaoNone,	 512,	true,  4,	0,	5,	10,},  // Level 17
	{					35 /* 28 FPS */,	9051.0f,		30000,	270,	8, ssaoNone,	 512,	true,  4,	0,	5,	10,},  // Level 18
	{					35 /* 28 FPS */,	100000.0f,		100000,	300,	8, ssaoNone,  	 512,	true,  4,	0,	5,	10,},  // Level 19
	// Introducing SSAO - big step hill, bigger MaxStepHill, blank first	   				  
	{					35 /* 28 FPS */,	100000.0f,		100000,	300,	8, ssaoFullBlank,  512,	true,  4, 0,	14,	25,},  // Level 20
	// And then turn it on
	{					35 /* 28 FPS */,	100000.0f,		100000,	300,	8, ssaoFull,   	512,	true,  4, 0,	2,	4,}    // Level 21
};

/////////////////////////////////////////////////////////////////////////////
// Less tweakable, but still
/////////////////////////////////////////////////////////////////////////////

FrameRateManager::FrameRateManager(void) :
	mSettings(0),
	mRenderCaps(0),
	mBlockCullingEnabled(true),
	mStableFramesCounter(0),
	mThrottlingOn(false),
	mCurrentQualityLevel(0),
	frameTimeAverage(AveragingFrames),
	renderTimeAverage(AveragingFrames),
	prepareTimeAverage(AveragingFrames),
	frameTimeVarianceAverage(VarianceFrames),
	fastBackoffAverage(FastBackoffFPSAve),
	mQualityDelayDown(LockStepDelayDown),
	mQualityDelayUp(LockStepDelayDown),
	mWasQualityUp(false),
	mSwitchCounter(1),
	mIsStable(false),
	mBlockCounter(0),
	mLastBlockCounter(0),
	mAdjustmentOn(true),
	mBadBackoffFrameCounter(0),
	mRecomputeDistanceDelay(FInt::FRMRecomputeDistanceFrameDelay),
	mAggressivePerformance(false)
{
	RBXASSERT(CRenderSettings::QualityLevelMax == ARRAYSIZE(kLockstepTable30FPS)); // If that fails, you probably added another quality level without syncing it with RenderSettings
	RBXASSERT(CRenderSettings::QualityLevelMax == ARRAYSIZE(kLockstepTable60FPS)); // If that fails, you probably added another quality level without syncing it with RenderSettings
	
#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
	LockstepTable = kLockstepTable30FPS;
#else
	LockstepTable = kLockstepTable60FPS;
#endif

	RBXASSERT(LockStepDelayDown <= LockStepDelayUp);

	// We need to have enough frames for averaging before we can step down again
	RBXASSERT(AveragingFrames + VarianceFrames <= LockStepDelayDown);

    for (unsigned i = 0; i < CRenderSettings::QualityLevelMax; ++i)
        mQualityCount[i] = 0;
    
    // Sensible defaults for culling
	mSqDistance = LockstepTable[0].distance*LockstepTable[0].distance;
	mSqRenderDistance = mSqDistance;
}

void FrameRateManager::configureFrameRateManager(CRenderSettings::FrameRateManagerMode mode, bool hasCharacter)
{
	if(hasCharacter){
		SetBlockCullingEnabled(mode == CRenderSettings::FrameRateManagerOff ? false : true);
	}
	else{
		SetBlockCullingEnabled(mode == CRenderSettings::FrameRateManagerOn ? true : false); 
	}
}

void FrameRateManager::setAggressivePerformance(bool value)
{
	mAggressivePerformance = value;
}

CRenderSettings::AntialiasingMode FrameRateManager::getAntialiasingMode()
{
	switch (mSettings->getAntialiasingMode()) {
		case CRenderSettings::AntialiasingAuto:
			//return mRenderCaps->getBestAntialiasingMode();
			return CRenderSettings::AntialiasingOff;
		// other settings simply override.
		default:
			return mSettings->getAntialiasingMode();
	}
}


void FrameRateManager::updateMaxSettings()
{
    mSSAOSupported = mRenderCaps->getSupportsGBuffer();
}

/////////////////////////////////////////////////////////////////////////////
// End of tweakable section
/////////////////////////////////////////////////////////////////////////////


FrameRateManager::~FrameRateManager(void)
{
    SendQualityLevelStats();
}

void FrameRateManager::SendQualityLevelStats()
{
    float avgQuality = GetAvarageQuality();
    if (avgQuality >= 1)
    {
        // Because we are reporting using timing function, we want one quality level to be 1sec (it accepts ms)
        int reportValue = (int)(avgQuality * 1000.0f); 
        RBX::RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_GAME, "GraphicsQualityLevel", reportValue, SystemUtil::osPlatform().c_str());
    }
}

float FrameRateManager::GetAvarageQuality()
{
    // compute average quality and send it to GA. Trying to keep the precision
    float floatCounts[CRenderSettings::QualityLevelMax];
    float freqSum = 0;
    for (unsigned i = 1; i < CRenderSettings::QualityLevelMax; ++i) //we ignore quality lvl = 0
    {
        floatCounts[i] = mQualityCount[i];
        freqSum += mQualityCount[i];
    }

    // if there is less then 100 samples, there is really nothing to report
    if (freqSum > 100)
    {
        float avgQuality = 0;
        float freqSumInv = 1.0f / freqSum;
        for (unsigned i = 0; i < CRenderSettings::QualityLevelMax; ++i)
            avgQuality += i * floatCounts[i] * freqSumInv;

        return avgQuality;
    }
    else
        return 0;
    
}

float FrameRateManager::GetTargetFrameTime(int level) const
{
	return mAggressivePerformance ? 19.f : LockstepTable[level].framerate;
}

void FrameRateManager::AddBlockQuota(int blocksInCluster, float sqDistanceToCamera, bool isInSpatialHash)
{
	if(!mIsGatheringDistance)
		return;

	mBlockCounter += blocksInCluster;
	if(mBlockCounter >= mBlockTarget)
	{
        // if cluster is in spatial hash, call order is done in roughly increasing camera distance so we can assume that the value is a valid cut-off
        // if cluster is not in spatial hash, calls are not ordered; we process all such clusters first, so if we ran out of blocks already, we have to
        // resort to the minimal culling distance for the current level
        if (isInSpatialHash)
            mSqDistance = std::max(LockstepTable[mCurrentQualityLevel].distance * LockstepTable[mCurrentQualityLevel].distance, sqDistanceToCamera + SqDistanceBump);
        else
            mSqDistance = LockstepTable[mCurrentQualityLevel].distance * LockstepTable[mCurrentQualityLevel].distance;

		mIsGatheringDistance = false;
	}
}

void FrameRateManager::SubmitCurrentFrame(double frameTime, double renderTime, double prepareTime, double bonusTime)
{
	updateMaxSettings(); // do this in a safe place. doesn't like being changed mid-frame?

	UpdateStats(frameTime, renderTime, prepareTime);

	// Use the distance from last frame for render distance this frame
	// If we were not gathering distance last frame they're the same
	// If we *were* then this is the cutoff distance where we reached the necessary block count
	mSqRenderDistance = mSqDistance;

	if(mSettings->getEnableFRM())
	{
		if(mRecomputeDistanceDelay > 0)
			mRecomputeDistanceDelay--;
		else
		{
			FASTLOG1(FLog::FRM, "Recomputing gathering distance on level %u", mCurrentQualityLevel);
			// Temporarily unlock the culling distance for one frame
			mSqDistance = LockstepTable[0].distance*LockstepTable[0].distance;
			mRecomputeDistanceDelay = FInt::FRMRecomputeDistanceFrameDelay;
			mIsGatheringDistance = true;
		}

		if (!mThrottlingOn)
		{
			// Initialize quality level for playing
			mThrottlingOn = true;
			CRenderSettings::QualityLevel qualityLevel = mSettings->getQualityLevel();
			if(qualityLevel == CRenderSettings::QualityAuto)
			{
				int autoQualityLevel = mSettings->getAutoQualityLevel();
				mCurrentQualityLevel = std::max(1,std::min(autoQualityLevel, (int)CRenderSettings::QualityLevelMax-1));
			}
			else
			{
				mCurrentQualityLevel = qualityLevel;
			}

			FASTLOG2(FLog::FRM, "Starting FRM, Quality setting: %u, starting level: %u", qualityLevel, mCurrentQualityLevel);
			UpdateQualitySettings();
		}
		else
		{
			CRenderSettings::QualityLevel qualityLevel = mSettings->getQualityLevel();

			bool bAdjusmentOn = false;
			if(qualityLevel == CRenderSettings::QualityAuto)
			{
				bAdjusmentOn = mAdjustmentOn;
			}
			else if(qualityLevel != mCurrentQualityLevel)
			{
				mCurrentQualityLevel = qualityLevel;
				UpdateQualitySettings();
			}
			AdjustQuality(frameTime, renderTime, bAdjusmentOn, bonusTime);
		}
	}
	else
	{
		mThrottlingOn = false;
		mCurrentQualityLevel = mSettings->getEditQualityLevel();
		UpdateQualitySettings();
	}

	mLastBlockCounter = mBlockCounter;
	if(mIsGatheringDistance)
		mBlockCounter = 0;
}

void FrameRateManager::StartCapturingMetrics()
{
	memset(&mMetrics, 0, sizeof(mMetrics));
	mIsStable = false;
	mSettleTimer.reset();
}

void FrameRateManager::UpdateStats(double frameTime, double renderTime, double prepareTime)
{
	if (fabs(frameTime) < 0.001 || fabs(renderTime) < 0.001)
	{
		return;
	}

	frameTimeAverage.sample(frameTime);
	renderTimeAverage.sample(renderTime);
	prepareTimeAverage.sample(prepareTime);
	fastBackoffAverage.sample(frameTime);

    mFPSCounter.Update(frameTime);
    
    if (mCurrentQualityLevel > 0)
    {
        // prevent overflow (really unlike, but still)
        if (mQualityCount[mCurrentQualityLevel] == UINT_MAX - 1)
            for (unsigned i = 0; i < CRenderSettings::QualityLevelMax; ++i)
                mQualityCount[i] /= 2;

        ++mQualityCount[mCurrentQualityLevel];
    }
}

float FrameRateManager::GetTargetFrameTimeForNextLevel() const
{
    RBXASSERT(mCurrentQualityLevel < (CRenderSettings::QualityLevelMax-1));

    return GetTargetFrameTime(mCurrentQualityLevel+1);
}

float FrameRateManager::GetTargetRenderTimeForNextLevel() const
{
    RBXASSERT(mCurrentQualityLevel < (CRenderSettings::QualityLevelMax-1));

    return GetTargetFrameTime(mCurrentQualityLevel+1)*MultiCoreRenderBottleneckFraction - LockstepTable[mCurrentQualityLevel+1].StepHill;
}


void FrameRateManager::AdjustQuality(double frameTime, double renderTime, bool adjustmentOn, double bonusTime)
{
	if(fabs(frameTime) < 0.001 || fabs(renderTime) < 0.001 || !adjustmentOn)
		return;

	// Don't adjust until we have delayed enough
	if(mQualityDelayDown > 0)
		mQualityDelayDown--;

	if(mQualityDelayUp > 0)
		mQualityDelayUp--;

    RBX::WindowAverage<double, double>::Stats frameStats = frameTimeAverage.getStats();
	RBX::WindowAverage<double, double>::Stats renderStats = renderTimeAverage.getStats();
    RBX::WindowAverage<double, double>::Stats prepareStats = prepareTimeAverage.getStats();

    frameTimeVarianceAverage.sample(frameStats.average);

    frameStats.average -= bonusTime;
    renderStats.average -= bonusTime;
    prepareStats.average -= bonusTime;


	FASTLOG3F(FLog::FRM, "FRM status. Frame time average: %f, Delay up %f, Delay down %f", frameStats.average, (float)mQualityDelayUp, (float)mQualityDelayDown);
    
	RBX::WindowAverage<double, double>::Stats fastBackoffStats = fastBackoffAverage.getStats();
    fastBackoffStats.average -= bonusTime;

	if(fastBackoffStats.average > FastBackoffMaxFrameLen &&
		FastBackoffMaxFrameLen > GetTargetFrameTime(mCurrentQualityLevel-1))
		mBadBackoffFrameCounter++;
	else
		mBadBackoffFrameCounter = 0;

	if (mBadBackoffFrameCounter >= FastBackoffWatchingFrames && mCurrentQualityLevel > 1)
	{
		FASTLOG(FLog::FRM, "FastBackoff, reducing quality");
		StepQuality(false, true);
	}

	if(mQualityDelayDown > 0 && mQualityDelayUp > 0)
		return;

	RBX::WindowAverage<double, double>::Stats frameAverageStats = frameTimeVarianceAverage.getStats();

	if(frameAverageStats.variance > VarianceLimit)
		return;
 
	bool bRenderLimited = renderStats.average > frameStats.average * RenderFraction;
	if (RBX::TaskScheduler::singleton().getThreadCount() > 1)
		bRenderLimited = renderStats.average > frameStats.average * MultiCoreRenderBottleneckFraction;

	// Check for going down:
	if ((mQualityDelayDown == 0) && (mCurrentQualityLevel > 1)
		&& (frameStats.average > GetTargetFrameTime(mCurrentQualityLevel)) && bRenderLimited)
		StepQuality(false, false);

	// Check for going up:
	else if((mQualityDelayUp == 0) && (mCurrentQualityLevel < (CRenderSettings::QualityLevelMax-1)) 
		&& frameStats.average < GetTargetFrameTimeForNextLevel())
	{
		bool renderingHasRoom = renderStats.average < GetTargetRenderTimeForNextLevel();

		if (RBX::TaskScheduler::singleton().getThreadCount() > 1)
		{
			renderingHasRoom = (renderStats.average < GetTargetRenderTimeForNextLevel()) && 
				prepareStats.average < GetTargetFrameTime(mCurrentQualityLevel+1)*MultiCorePrepareFraction;
		}

		if(renderingHasRoom)
			StepQuality(true, false);
	}

	if(!mIsStable && mSettleTimer.delta().seconds() > SettleDelay)
	{  
		mIsStable = true;

		mMetrics.NumberOfSettles++;
	}
}

void FrameRateManager::StepQuality(bool stepUp, bool isBackOff)
{
	int oldQualityLevel = mCurrentQualityLevel;

	mCurrentQualityLevel += stepUp ? 1 : -1;
	FASTLOG2(FLog::FRM, "Stepping FRM quality, old: %u, new : %u", oldQualityLevel, mCurrentQualityLevel);

	UpdateQualitySettings();
	frameTimeAverage.clear();
	renderTimeAverage.clear();

	// Make delay for stepping down constant
	mQualityDelayDown = LockStepDelayDown;
	mBadBackoffFrameCounter = 0;

	mQualityDelayUp = stepUp ? LockStepDelayUp : LockStepDelayUp * mSwitchCounter;

	if(mCurrentQualityLevel > 1)
	{
		// If last step was down, make going up harder
		if(stepUp != mWasQualityUp && mSwitchCounter < SwitchCounterMax)
		{
			// If we're stepping down from higher level immediately, bump the step (within the allowed range, of course)
			if(!stepUp)
			{
				RBXASSERT(mCurrentQualityLevel < (CRenderSettings::QualityLevelMax-1));
				int previousLevel = mCurrentQualityLevel+1;

				double StepHillAdd = isBackOff ? 0.1 : 1;
				LockstepTable[previousLevel].StepHill = std::min(LockstepTable[previousLevel].MaxStepHill, LockstepTable[previousLevel].StepHill+StepHillAdd);
			}

			mSwitchCounter++;
		}
		else if(mSwitchCounter > 1)
		{
			mSwitchCounter--;
		}
	}

	mWasQualityUp = stepUp;

	mSettings->setAutoQualityLevel(mCurrentQualityLevel);

	mSettleTimer.reset();
	mIsStable = false;

	// Accumulate number of switches here, average it on GetMetrics
	mMetrics.AverageSwitchesPerSettle++;
}

FrameRateManager::Metrics FrameRateManager::GetMetrics()
{
	Metrics result = mMetrics;

	CRenderSettings::QualityLevel qualityLevel = mSettings->getQualityLevel();
	result.AutoQuality = qualityLevel == CRenderSettings::QualityAuto;
	result.QualityLevel = Math::iRound(GetAvarageQuality());
    result.AverageFps = mFPSCounter.GetFPS();

	if(result.NumberOfSettles != 0)
		result.AverageSwitchesPerSettle /= result.NumberOfSettles;

	return result;
}


void FrameRateManager::UpdateQualitySettings()
{
	const THROTTLE_LOCKSTEP& lockstep = LockstepTable[mCurrentQualityLevel];

	if (mSettings->getFrameRateManagerMode() != CRenderSettings::FrameRateManagerOff)
		mBlockTarget = lockstep.blockCount;
	else
		mBlockTarget = LockstepTable[0].blockCount;

	mIsGatheringDistance = true;

	// Unlock the view distance but don't change rendering distance; we'll recompute it this frame
	mSqDistance = LockstepTable[0].distance*LockstepTable[0].distance;

	mRecomputeDistanceDelay = FInt::FRMRecomputeDistanceFrameDelay;
}

double FrameRateManager::getMetricValue(const std::string& metric)
{
	if (metric == "FRM") 
		return IsBlockCullingEnabled();
	else if (metric == "FRM Target") 
		return GetVisibleBlockTarget();
	else if (metric == "FRM Visible") 
		return GetVisibleBlockCounter();
	else if (metric == "FRM Distance") 
		return sqrt(GetViewCullSqDistance());
	else if (metric == "FRM Quality") 
		return GetQualityLevel();
	else if(metric == "FRM Auto Quality")
		return mSettings->getQualityLevel() == CRenderSettings::QualityAuto;
	else if(metric == "FRM Switch Counter")
		return mSwitchCounter;
	else if(metric == "FRM Step Hill")
	{
		// If Quality is not allowed to be adjusted, return -1
		if (mSettings->getQualityLevel() != CRenderSettings::QualityAuto)
			return -1;
		else
			return mCurrentQualityLevel+1 < CRenderSettings::QualityLevelMax ? LockstepTable[mCurrentQualityLevel+1].StepHill : 0;
	}
	else if (metric == "FRM Adjust Delay Up")
		return mQualityDelayUp;
	else if (metric == "FRM Adjust Delay Down")
		return mQualityDelayDown;
	else if(metric == "FRM Variance")
		return frameTimeVarianceAverage.getStats().variance;
	else if(metric == "FRM Backoff Counter")
		return mBadBackoffFrameCounter;
	else if(metric == "FRM Backoff Average")
		return fastBackoffAverage.getStats().average;

	return -1;
}

double FrameRateManager::GetFrameTimeAverage()
{
	return frameTimeAverage.getStats().average;
}

double FrameRateManager::GetPrepareTimeAverage()
{
	return prepareTimeAverage.getStats().average;
}

double FrameRateManager::GetRenderTimeAverage()
{
	return renderTimeAverage.getStats().average;
}

const WindowAverage<double, double>& FrameRateManager::GetFrameTimeStats()
{
	return frameTimeAverage;
}

const WindowAverage<double, double>& FrameRateManager::GetRenderTimeStats()
{
	return renderTimeAverage;
}

float FrameRateManager::GetRenderCullSqDistance()
{
	return mSqRenderDistance;
}

float FrameRateManager::GetViewCullSqDistance()
{
	return mSqDistance;
}

float FrameRateManager::getShadingDistance() const
{
	return LockstepTable[mCurrentQualityLevel].shadingDistance;
}

int FrameRateManager::getPhysicsThrottling() const 
{
	return LockstepTable[mCurrentQualityLevel].throttlingFactor;
}

float FrameRateManager::getShadingSqDistance() const
{
	return sqrf(LockstepTable[mCurrentQualityLevel].shadingDistance);
}

int FrameRateManager::getTextureAnisotropy() const
{
	return LockstepTable[mCurrentQualityLevel].textureAnisotropy;
}

float FrameRateManager::getLightGridRadius() const
{
	return LockstepTable[mCurrentQualityLevel].lightGridRadius;
}

bool FrameRateManager::getLightingNonFixedEnabled() const
{
	return LockstepTable[mCurrentQualityLevel].lightAllowNonFixed;
}

unsigned FrameRateManager::getLightingChunkBudget() const
{
	return LockstepTable[mCurrentQualityLevel].lightChunkBudget;
}

double FrameRateManager::GetMaxNextViewCullDistance() 
{
	return sqrt(mSqDistance) * 1.1;
}

SSAOLevel FrameRateManager::getSSAOLevel()
{
    if (FFlag::DebugSSAOForce)
        return ssaoFull;

	if (!mSSAOSupported)
		return ssaoNone;

	return LockstepTable[mCurrentQualityLevel].ssao;
}

void FrameRateManager::Configure(const RenderCaps* renderCaps, CRenderSettings* settings)
{
	mSettings = settings;
	mRenderCaps = renderCaps;

	updateMaxSettings();

	UpdateQualitySettings();
}

// returns overall particle throttle factor. Range ]0 .. 1] , 1 for full detail.
double FrameRateManager::GetParticleThrottleFactor()
{
    if(GetQualityLevel() == 0)
        return 1.0;

    return std::max(0.0, std::min(1.0, (double)GetQualityLevel()/CRenderSettings::QualityLevelMax) );
}

bool FrameRateManager::getGBufferSetting()
{
#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
    return false;
#else
    return (isSSAOSupported() && GetQualityLevel() >= FInt::RenderGBufferMinQLvl);
#endif
}

void FrameRateManager::PauseAutoAdjustment()
{
	mAdjustmentOn = false;
}

void FrameRateManager::ResumeAutoAdjustment()
{
	mAdjustmentOn = true;
}

}
