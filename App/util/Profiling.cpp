/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/Profiling.h"
#include "G3D/System.h"
#include "rbx/Debug.h"
#include "v8datamodel/DebugSettings.h"

#ifndef _WIN32
#include <pthread.h>
static pthread_key_t tsd_key;
#endif

static long markTlsIndex = 0;
static bool profilingEnabled = false;

void RBX::Profiling::init(bool enabled)
{
	profilingEnabled = enabled;
#ifdef _WIN32
	markTlsIndex = TlsAlloc();
#else
	
	/* with full error check EL
	if( pthread_key_create(&tsd_key, NULL) ) err_abort(status, Error creating key); 
	if( pthread_setspecific( tsd_key, value))  
		err_abort(status, Error in pthread_setspecific); 
	*/
	pthread_key_create(&tsd_key, NULL);
	
	markTlsIndex = reinterpret_cast<uintptr_t>(pthread_getspecific( tsd_key ));
#endif
}

void RBX::Profiling::setEnabled(bool enabled)
{
	profilingEnabled = enabled;
#ifdef _WIN32
	// This is only valid in windows. For mac, the markTlsIndex
	// is always 0 unless profiling is init()ed inside the context
	// of a Mark.
	RBXASSERT(markTlsIndex);
#endif
}

bool RBX::Profiling::isEnabled()
{
	return profilingEnabled ||  DebugSettings::singleton().enableProfiling;
}

using namespace RBX;
using namespace Profiling;


Bucket::Bucket()
	:sampleTimeElapsed(0)
	,wallTimeSpan(0)
	,frames(0)
{
}

double Profiler::profilingWindow = 3.0;

Profiler::Profiler(const char* name)
	:name(name)
	,currentBucket(0)
	,bucketTimeSpan(0.003 * profilingWindow)	// in second
{
}

CodeProfiler::CodeProfiler(const char* name)
	:Profiler(name)
{
}


void CodeProfiler::unlog(double wallTimeElapsed)
{
	buckets[currentBucket].wallTimeSpan -= wallTimeElapsed;
}

void CodeProfiler::log(bool frameTick, double wallTimeElapsed)
{
	// TODO: Do we need to modify our tick code to be monotonically increasing???
	// Sometimes the wall time can be slightly < 0!
	//RBXASSERT(wallTimeElapsed>=-0.001);
	Time time = Time::now<Time::Benchmark>();
	Time::Interval sampleTimeElapsed = time - lastSampleTime;
	if (sampleTimeElapsed >= bucketTimeSpan)
	{
		// We're done with the current bucket. Create a new one
		currentBucket++;
		currentBucket %= buckets.size();
		buckets[currentBucket].frames = frameTick ? 1 : 0;
		buckets[currentBucket].sampleTimeElapsed = sampleTimeElapsed.seconds();
		buckets[currentBucket].wallTimeSpan = wallTimeElapsed;
	}
	else
	{
		if (frameTick)
			buckets[currentBucket].frames++;
		buckets[currentBucket].sampleTimeElapsed += sampleTimeElapsed.seconds();
		buckets[currentBucket].wallTimeSpan += wallTimeElapsed;
	}
	lastSampleTime = time;
}


Bucket Profiler::getWindow(double window) const
{
	Bucket result;

	if (!Profiling::isEnabled() && !DebugSettings::singleton().enableProfiling)
		return result;

	const int firstBucket = currentBucket - 1 + buckets.size();	// We add bucketCount to ensure the % operator wraps to >= 0 index

	// Iterate backwards through previous buckets
	const size_t count = std::min((size_t)std::max((int)(window / bucketTimeSpan.seconds()), 1), buckets.size()-2);
	for (size_t i=0; i<count; ++i)
	{
		int index = (firstBucket-i) % buckets.size();
		RBXASSERT(index>=0);
		result += buckets[index];
		if (result.sampleTimeElapsed>=window)
			break;
	}

	return result;
}

Bucket Profiler::getFrames(int frames) const
{
	Bucket result;

	if (!Profiling::isEnabled() && !DebugSettings::singleton().enableProfiling)
		return result;

	const int firstBucket = currentBucket - 1 + buckets.size();	// We add bucketCount to ensure the % operator wraps to >= 0 index

	// Iterate backwards through previous buckets
	const size_t count = std::min((size_t)frames, buckets.size()-2);
	for (size_t i=0; i<count; ++i)
	{
		int index = (firstBucket-i) % buckets.size();
		RBXASSERT(index>=0);
		result += buckets[index];
	}

	return result;
}
Bucket& Bucket::operator+=(const Bucket& b)
{
	sampleTimeElapsed += b.sampleTimeElapsed;
	wallTimeSpan += b.wallTimeSpan;
	frames += b.frames;
	return *this;
}


double Bucket::getActualFPS() const
{
	return sampleTimeElapsed>0.0 ? frames / sampleTimeElapsed : 0;
}

double Bucket::getNominalFPS() const
{
	return wallTimeSpan>0.0 ? frames / wallTimeSpan : 0;
}


double Bucket::getNominalFramePeriod() const
{
	return frames>0 ? wallTimeSpan / frames : 0;
}





Mark::Mark(CodeProfiler& section, bool frameTick, bool logInclusive):section(section),frameTick(frameTick),enabled(profilingEnabled || DebugSettings::singleton().enableProfiling), logInclusive(logInclusive)
{
#if 1
	if (enabled)
	{
		startTime = Time::now<Time::Benchmark>();

#ifdef _WIN32
		enclosingMark = (Mark*) TlsGetValue(markTlsIndex);
		TlsSetValue(markTlsIndex, this);
#else
		enclosingMark = (Mark *) pthread_getspecific( tsd_key );
		pthread_setspecific( tsd_key, (void *)(this));
#endif
	}
#endif
}

Mark::~Mark()
{
#if 1
	if (enabled)
	{
#ifdef _WIN32
		TlsSetValue(markTlsIndex, enclosingMark);
#else
		pthread_setspecific( tsd_key, (void *)(enclosingMark));
#endif
		Time now = Time::now<Time::Benchmark>();
		Time::Interval wallTimeInclusive = now - startTime;
		
		double logtime = wallTimeInclusive.seconds();
		if(!logInclusive)
		{
			logtime = wallTimeInclusive.seconds() - childrenElapsed.seconds();
		}

		section.log(frameTick, logtime);
		
		// Subtract times from enclosing Mark
		if (enclosingMark)
			enclosingMark->childrenElapsed += wallTimeInclusive;
	}
#endif
}

BucketProfile::BucketProfile(const int* bucketLimits, int size) : bucketLimits(bucketLimits), total(0)
{
	data.resize(size+1, 0);	
}

BucketProfile::BucketProfile(const BucketProfile& orig) : data(orig.data), bucketLimits(orig.bucketLimits), total(0)
{
}

BucketProfile::BucketProfile() : bucketLimits(NULL), total(0)
{}

unsigned BucketProfile::findBucket(int v)
{
	unsigned numBuckets = data.size();
	for(unsigned i = 0; i < numBuckets-1; i++)
	{
		if (v < bucketLimits[i])
			return i;
	}
	return numBuckets-1;
}

const BucketProfile& BucketProfile::operator = (const BucketProfile& rhs)
{
	data = rhs.data;
	bucketLimits = rhs.bucketLimits;
	total = rhs.total;

	return *this;
}

void BucketProfile::clear()
{
	for(std::vector<int>::iterator it = data.begin(); it != data.end(); ++it)
	{
		*it = 0;
	}

	total = 0;
}

void BucketProfile::addValue(int v)
{
	data[findBucket(v)]++;

	total += v;
}

void BucketProfile::removeValue(int v)
{	
	unsigned bucket = findBucket(v);
	data[bucket]--;
	RBXASSERT(data[bucket] >= 0);
}

