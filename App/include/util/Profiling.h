#pragma once

#include "rbx/boost.hpp"
#include "rbx/rbxTime.h"
#include "boost/array.hpp"
#include <map>
#include <vector>

namespace RBX
{
	namespace Profiling
	{
		void init(bool enabled);
		void setEnabled(bool enabled);
		bool isEnabled();

		struct Bucket
		{
			float sampleTimeElapsed;  // System time span that the bucket sampled for
			float wallTimeSpan;
			int frames;		// The number of "frames" recorded in the bucket

			double getActualFPS() const;	        // frames/sec
			double getNominalFPS() const;	        // frames/sec
			double getNominalFramePeriod() const;	// secs/frame

			double getSampleTime() const { return sampleTimeElapsed; }
			double getWallTime() const { return wallTimeSpan; }
			int    getFrames() const { return frames; }

			Bucket();
			Bucket& operator+=(const Bucket& b);
		};

		class Profiler : public boost::noncopyable
		{
		protected:
			const Time::Interval bucketTimeSpan;	// The minimum amount of time per bucket
			int currentBucket;
			boost::array<Bucket, 512> buckets;	// TODO: Use boost::circular_buffer
			Time lastSampleTime;
		public:
			const std::string name;
			Profiler(const char* name);
			virtual ~Profiler() {};
			Bucket getWindow(double window) const; // get last samples based on elapsed time
			Bucket getFrames(int frames) const;    // get n last frames.
			static double profilingWindow;
		};

		// Profiles sections of code using the Mark class
		class CodeProfiler : public Profiler
		{
			friend class Mark;
		public:
			CodeProfiler(const char* name);
		private:
			void log(bool frameTick, double wallTimeElapsed);
			void unlog(double wallTimeElapsed);
		};

		// Mark a section of code as belonging to a CodeProfiler
		// (Enclosing Mark will be disabled for the lifetime of this object)
		class Mark
		{
			CodeProfiler& section;
			Mark* enclosingMark;
			bool frameTick;
			Time startTime;
			const bool enabled;
			Time::Interval childrenElapsed; // sum of elapsed time (inclusive) in all Markers that have _this_ as a enclosingMark.
			const bool logInclusive; // true if you want to log time including children. false will subtract time spent in children
		public:
			Mark(CodeProfiler& section, bool frameTick, bool logInclusive = false);
			~Mark();
		};


		// BucketProfile
		class BucketProfile
		{
			std::vector<int> data;
			const int* bucketLimits;

			unsigned int findBucket(int v);
			int total;

		public:
			// WARNING: assumes pointer to bucketLimits is static and saves it directly
			BucketProfile(const int* bucketLimits, int size);

			BucketProfile(const BucketProfile& rhs);
			BucketProfile();
			const BucketProfile& operator = (const BucketProfile& rhs);

			void addValue(int v);
			void removeValue(int v);
			void clear();

			int getTotal() { return total; }

			const std::vector<int>& getData() const { return data; }; 
			const int* getLimits() const { return bucketLimits; }; 
		};
	}
};
