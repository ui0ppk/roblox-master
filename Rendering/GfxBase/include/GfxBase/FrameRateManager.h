#pragma once

#include <vector>
#pragma warning (push)
#pragma warning( disable:4996 ) // disable -D_SCL_SECURE_NO_WARNING in ublas. 
#include <boost/numeric/ublas/vector.hpp>
#pragma warning (pop)

#include "GfxBase/RenderSettings.h"
#include "rbx/RunningAverage.h"
#include <map>

namespace RBX { 
	class RenderCaps;
	class Log;

enum SSAOLevel
{
	ssaoNone = 0,
	ssaoFullBlank,
	ssaoFull
};

struct THROTTLE_LOCKSTEP;

class FrameRateManager
{
public:
	FrameRateManager(void);
	~FrameRateManager(void);

	void configureFrameRateManager(CRenderSettings::FrameRateManagerMode mode, bool hasCharacter);
	void setAggressivePerformance(bool value);

	struct Metrics
	{
		bool AutoQuality;
		int QualityLevel;
		int NumberOfSettles;
		double AverageSwitchesPerSettle;
        double AverageFps;
	};

	// add to current frame counter.
	void AddBlockQuota(int blocksInCluster, float sqDistanceToCamera, bool isInSpatialHash);

    bool getGBufferSetting();

	SSAOLevel getSSAOLevel();
    bool isSSAOSupported() { return mSSAOSupported; }

	float getShadingDistance() const;
	float getShadingSqDistance() const;
    int getTextureAnisotropy() const;

	int getPhysicsThrottling() const;

	float getLightGridRadius() const;
	bool getLightingNonFixedEnabled() const;
	unsigned getLightingChunkBudget() const;
	
	void SubmitCurrentFrame(double frameTime, double renderTime, double prepareTime, double bonusTime);

	// adjusts quality to try to fit rendering to this timespan.
	void ThrottleTo(double rendertime_ms);

	double getMetricValue(const std::string& metric);

	int GetRecomputeDistanceDelay() { return mRecomputeDistanceDelay; }
    
	float GetViewCullSqDistance();
	float GetRenderCullSqDistance();
    
	double GetMaxNextViewCullDistance(); // farthest cull distance possible in next frame.

	int GetQualityLevel() { return mCurrentQualityLevel; }

	bool IsBlockCullingEnabled() { return mBlockCullingEnabled; };
	void SetBlockCullingEnabled(bool enabled) { mBlockCullingEnabled = enabled; };

	// supply the framerate manager with some special information that can be
	// used to formulate exceptions.
	void Configure(const RenderCaps* renderCaps, CRenderSettings* settings);

	// after calling Configure, this gives our determination of the best
	// possible quality we can acheive with certain features, and with current settings
	CRenderSettings::AntialiasingMode getAntialiasingMode();
 	void updateMaxSettings();

	double GetVisibleBlockTarget() const { return mBlockTarget; }; // smoothed block target
	double GetVisibleBlockCounter() const { return mLastBlockCounter; }; 

	float GetTargetFrameTimeForNextLevel() const; 
	float GetTargetRenderTimeForNextLevel() const;

	// counter that indicates how many frames have elapsed with the block count in a stable state.
	void ResetStableFramesCounter() { mStableFramesCounter = 0; };
	const int& GetStableFramesCounter() { return mStableFramesCounter; };

	// returns overall particle throttle factor. Range ]0 .. 1] , 1 for full detail.
	double GetParticleThrottleFactor();

	double GetRenderTimeAverage();
	double GetPrepareTimeAverage();
	double GetFrameTimeAverage();

	const WindowAverage<double,double>& GetRenderTimeStats();
	const WindowAverage<double,double>& GetFrameTimeStats();

	void StartCapturingMetrics();
	Metrics GetMetrics();

	void PauseAutoAdjustment();
	void ResumeAutoAdjustment();
	
	int GetQualityDelayUp() const { return mQualityDelayUp; }
	int GetQualityDelayDown() const { return mQualityDelayDown; }
	int GetBackoffCounter() const { return mBadBackoffFrameCounter; }
	double GetBackoffAverage() const { return fastBackoffAverage.getStats().average; }

protected:
	bool mSSAOSupported;

	bool mAdjustmentOn;

	CRenderSettings* mSettings;
	const RenderCaps* mRenderCaps;

	bool mBlockCullingEnabled;
	bool mAggressivePerformance;

	int mStableFramesCounter;

	bool mThrottlingOn;

	int mCurrentQualityLevel;
    unsigned mQualityCount[CRenderSettings::QualityLevelMax];

	int mQualityDelayUp;
	int mQualityDelayDown;
	int mRecomputeDistanceDelay;

	bool mWasQualityUp;
	int  mSwitchCounter;

private:
	float mSqDistance;
	float mSqRenderDistance;

	void UpdateStats(double frameTime, double renderTime, double prepareTime);
	void AdjustQuality(double frameTime, double renderTime, bool adjustmentOn, double bonusTime);
	void StepQuality(bool direction, bool isBackOff);
	void UpdateQualitySettings();
    void SendQualityLevelStats();
    float GetAvarageQuality();

	float GetTargetFrameTime(int level) const;

	RBX::WindowAverage<double, double> frameTimeAverage;
	RBX::WindowAverage<double, double> renderTimeAverage;
	RBX::WindowAverage<double, double> prepareTimeAverage;
	RBX::WindowAverage<double, double> frameTimeVarianceAverage;

	RBX::WindowAverage<double, double> fastBackoffAverage;

	int mBadBackoffFrameCounter;

	Metrics mMetrics;
	RBX::Timer<RBX::Time::Fast> mSettleTimer;
	bool mIsStable;
	bool mIsGatheringDistance;
	int mBlockCounter;
	int mBlockTarget;
	int mLastBlockCounter;

    class  AvgFpsCounter
    {
        public:
            AvgFpsCounter(): timeSumSec(0), frameCnt(0) {}

            void Update(double deltaTimeMs)
            {
                if (deltaTimeMs < 1000)
                {
                    timeSumSec += deltaTimeMs * 0.001;
                    ++frameCnt;
                }
            }

            double GetFPS() { return frameCnt ? 1.0 / (timeSumSec / frameCnt) : 0 ; }
        private: 
            double timeSumSec;
            unsigned frameCnt;
    };

    AvgFpsCounter mFPSCounter;
	
	THROTTLE_LOCKSTEP* LockstepTable;
};

} // namespaces
