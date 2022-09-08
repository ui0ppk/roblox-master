
#pragma once

#include "rbx/atomic.h"
#include "rbx/boost.hpp"
#include "rbx/rbxTime.h"
#include "rbx/Debug.h"
#include "rbx/MathUtil.h"

#include <boost/static_assert.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>

#include <cmath>

namespace RBX
{

	template<typename ValueType = double, typename AverageType = double>
	class RunningAverage
	{
		boost::shared_ptr<boost::circular_buffer<ValueType> > buffer;
	public:
		RunningAverage(double lerp = 0.05, ValueType initialValue = 0, unsigned int bufferSize = 0)
			:lerp(lerp),
			lastSampleValue(initialValue),
			averageValue(initialValue),
			averageVariance(0),
			firstTime(true) 
		{
			if (bufferSize)
				buffer.reset(new boost::circular_buffer<ValueType>(bufferSize));
		}

		void sample(ValueType value)
		{			
			if (isFinite(value))
			{
				sampleValue(value);
				sampleVariance(value);

				if (buffer)
					buffer->push_back(value);
			}
		}

		// Get the current value
		AverageType value() const { return averageValue; }

		AverageType variance() const { return averageVariance; }
		AverageType standard_deviation() const 
		{ 
			// TODO: Cache this value?
			return std::sqrt(averageVariance); 
		}
		AverageType variance_to_mean_ratio() const { return averageVariance / (averageValue * averageValue); }
		AverageType coefficient_of_variation() const { return standard_deviation() / averageValue; }

		// Get the last sampled value
		ValueType lastSample() const { return lastSampleValue; }

		template<class F>
		void iter(F& f) const
		{
			if (buffer)
			{
				for(typename boost::circular_buffer<ValueType>::const_iterator it = buffer->begin(); it != buffer->end(); ++it)
				{
					f(*it);
				}
			}
		}

		void reset(ValueType resetValue = 0)
		{
			lastSampleValue = resetValue;
			averageValue = resetValue;
			averageVariance = 0;
			firstTime = true;

			if (buffer)
				buffer->clear();
		}

        bool hasSampled() {return !firstTime;}

		const double lerp;
	private:
		ValueType lastSampleValue;
		AverageType averageValue;
		AverageType averageVariance;
		bool firstTime;

		inline void sampleValue(ValueType value)
		{
			averageValue = firstTime ? value : (1.0 - lerp) * averageValue + lerp * (AverageType)value;
			lastSampleValue = value;
			firstTime = false;
		}

		inline void sampleVariance(ValueType value)
		{
			const double diff = value - averageValue;
			const double variance = diff * diff;
			averageVariance = (1.0 - lerp) * averageVariance + lerp * variance;
		}
	};

	template<typename ValueType = double, typename AverageType = double>
	class WindowAverage 
	{
	protected:
		boost::circular_buffer<ValueType> buffer;
	public:

		struct Stats
		{
			Stats(size_t samples, const AverageType& average, const AverageType& variance)
				: samples(samples), average(average), variance(variance) {};
			size_t samples;
			AverageType average;
			AverageType variance;
		};

		WindowAverage(size_t maxSamples) : buffer(maxSamples) 	{	}
		void setMaxSamples(size_t maxSamples) { buffer.set_capacity(maxSamples); }
		size_t getMaxSamples() const { return buffer.capacity(); }

		void sample(ValueType value)
		{			
			buffer.push_back(value);
		}

		// calls fonbeforedrop if an item is about to be dropped
		template<class F>
		void sample(ValueType value, F& fonbeforedrop)
		{			
			if(buffer.full())
			{
				fonbeforedrop(buffer.front());
			}
			
			sample(value);
		}

		Stats getSanitizedStats(Confidence conf = C90) const
		{
			Stats regularStats = getStats();
			if(regularStats.samples <= 1)
				return regularStats;

			Stats result(0, AverageType(), AverageType());

			AverageType std = sqrt(regularStats.variance);

			for(typename boost::circular_buffer<ValueType>::const_iterator it = buffer.begin(); it != buffer.end(); ++it)
			{
				double value = *it;

				if (IsValueOutlier(value, regularStats.samples, regularStats.average, std, conf))
					continue;

				result.samples++;
				result.average += value;
			}

			result.average /= result.samples;

			for(typename boost::circular_buffer<ValueType>::const_iterator it = buffer.begin(); it != buffer.end(); ++it)
			{
				double value = *it;   

				if (IsValueOutlier(value, regularStats.samples, regularStats.average, std, conf))
					continue;

				AverageType diff = (result.average - value);
				result.variance = result.variance + diff * diff;
			}

			result.variance /= (result.samples - 1);

			return result;
		}

		Stats getStats(size_t samples = ~0) const    // get n last frames.
		{
			samples = std::min(buffer.size(), samples);
			Stats result(samples, AverageType(), AverageType());

			typename boost::circular_buffer<ValueType>::const_reverse_iterator it;
			size_t ii;
			for(it = buffer.rbegin(), ii = 0; ii < samples; ++it, ++ii)
			{
				result.average = result.average + *it;
			}

			if (samples != 0)
			{
				result.average /= samples;
			}

			for(it = buffer.rbegin(), ii = 0; ii < samples; ++it, ++ii)
			{
				AverageType diff = (result.average - *it);
				result.variance = result.variance +  diff * diff;
			}
			if(samples > 1)
			{
				result.variance /= (samples -1);
			}

			return result;
		}

		AverageType getLatest() const // get data from the last frame
		{
			return buffer.empty() ? 0 : *(buffer.rbegin());
		}

		template<class F>
		void iter(F& f) const
		{
			for(typename boost::circular_buffer<ValueType>::const_iterator it = buffer.begin(); it != buffer.end(); ++it)
			{
				f(*it);
			}
		}

		void clear()
		{
			buffer.clear();
		}

		size_t size() const
		{
			return buffer.size();
		}
	};


	// A class that follows the pattern of RunningAverage, but keeps track of step frequency.
	template<Time::SampleMethod sampleMethod = Time::Benchmark>
	class RunningAverageTimeInterval
	{
	public:
		RunningAverageTimeInterval(double lerp = 0.05)
			:firstTime(true),average(lerp) {}

		void sample()
		{
			if (firstTime)
			{
				timer.reset();
				firstTime = false;
			}
			else
			{
				average.sample(timer.reset().seconds());
			}
		}

		// Get the current value
		Time::Interval value() const
		{ 
			Time::Interval timeSinceLastSample = timer.delta();
			double v = average.value();
			if (timeSinceLastSample.seconds() > 2.0 * v)
				return timeSinceLastSample;
			else
				return Time::Interval(v); 
		}

		double rate() const
		{ 
			double v = value().seconds();
			return v>0.0 ? 1.0/v : 0.0; 
		}

		double variance() const { return average.variance(); }
		double standard_deviation() const { return average.standard_deviation(); }
		double variance_to_mean_ratio() const { return average.variance_to_mean_ratio(); }
		double coefficient_of_variation() const { return average.coefficient_of_variation(); }
        double getLerp() const { return average.lerp;}

		Time::Interval lastSample() const { return Time::Interval(average.lastSample()); }

	private:
		Timer<sampleMethod> timer;
		bool firstTime;
		RunningAverage<> average;
	};

	struct FOnBeforeDrop
	{
		WindowAverage<>& average;
		Time::Interval& currentWindow;
		Time::Interval& maxWindow;
		FOnBeforeDrop(WindowAverage<>& average, Time::Interval& currentWindow, Time::Interval& maxWindow) : average(average), currentWindow(currentWindow), maxWindow(maxWindow) {};
		
		void operator()(double sample)
		{
			if(currentWindow.seconds() < maxWindow.seconds())
			{
				// prevent dropping.
				// grow window size.
				average.setMaxSamples(average.getMaxSamples() * 2);
			}
			else
			{
				// allow drop. adjust total counter.
				currentWindow -= Time::Interval(sample);
			}
		}
	} ;
	
	

	// A class that follows the pattern of RunningAverage, but keeps track of step frequency.
	template<Time::SampleMethod sampleMethod = Time::Benchmark>
	class WindowAverageTimeInterval
	{
	public:
		WindowAverageTimeInterval(Time::Interval maxWindow)
			:maxWindow(maxWindow), average(16) {}

		void setMaxWindow(Time::Interval maxWindow) 
		{ 
			this->maxWindow = maxWindow; 
			if(maxWindow.seconds() == 0.0)
			{
				// special case, release memory
				average.setMaxSamples(16);
			}
		};
		Time::Interval getMaxWindow() const { return maxWindow; };
		size_t getCapacity() const { return average.getMaxSamples(); };

		void sample()
		{
			if (firstTime)
			{
				timer.reset();
				firstTime = false;
			}
			else
			{
				double dt = timer.reset().seconds();
				currentWindow += Time::Interval(dt);

				// this functor will allow our ring buffer to grow geometrically 
				// as long as it doesn't contain maxWindow worth of interval measurments.
				FOnBeforeDrop fonbeforedrop(average, currentWindow, maxWindow);
				average.sample(dt, fonbeforedrop);
				
			}
		}

		struct Stats
		{
			Stats(	size_t samples, 
					Time::Interval averagedt,
					Time::Interval variancedt,
					Time::Interval totalt,
					double samplespersecond)
					: samples(samples)
					, average(averagedt)
					, variance(variancedt)
					, sum(totalt)
					, samplespersecond(samplespersecond)
			{};
			size_t samples;
			Time::Interval average;
			Time::Interval variance;
			Time::Interval sum;
			double samplespersecond;
		};

		struct FSum
		{
			FSum() : vsum(0.0) {};

			double vsum;
			void operator() (double v) { vsum += v; };
		};

		Stats getStats(size_t samples = ~0) const
		{
			WindowAverage<>::Stats basicstats = average.getStats(samples);

			FSum fsum;

			average.iter(fsum);

			return Stats(basicstats.samples, Time::Interval(basicstats.average), Time::Interval(basicstats.variance), Time::Interval(fsum.vsum), samples / fsum.vsum);
		}
		
		template<class F>
		void iter(F& f) const
		{
			average.iter(f);
		}

		void clear()
		{
			firstTime = true;
			average.clear();
		}

		size_t size() const
		{
			return average.size();
		}

	private:
		Time::Interval currentWindow;
		Time::Interval maxWindow;
		WindowAverage<> average;
		Timer<sampleMethod> timer;
		bool firstTime;
	};

	template<typename ValueType = int, Time::SampleMethod sampleMethod = Time::Benchmark>
	class TotalCountTimeInterval
	{
		Timer<sampleMethod> timer;
		double interval;
		ValueType valueLastInterval;
		ValueType valueCurrentInterval;
	public:
		TotalCountTimeInterval(double interval = 1.0f) : interval(interval), valueCurrentInterval(0), valueLastInterval(0) {}
		void increment(ValueType count = 1) 
		{
			if (timer.delta().seconds() >= interval)
			{
				valueLastInterval = valueCurrentInterval;
				valueCurrentInterval = 0;
				timer.reset();
			}
			valueCurrentInterval += count; 
		}
		void decrement(ValueType count = 1) 
		{
			if (timer.delta().seconds() >= interval)
			{
				valueLastInterval = valueCurrentInterval;
				valueCurrentInterval = 0;
				timer.reset();
			}
			valueCurrentInterval -= count; 
		}

		ValueType getCount() const 
		{
			return (timer.delta().seconds() <= interval) ? valueLastInterval : 0; 
		}
	};


	class ThrottlingHelper
	{
		int* eventsPerMinute;
		int* eventsPerObjectPerMinute;
		int requestCounter;
		int maxObjectCount;
		Time lastTimestamp;

	public:
		ThrottlingHelper(int* eventsPerMinute, int* eventsPerObjectPerMinute = NULL) : // Designed to pass FInt
			eventsPerMinute(eventsPerMinute),
			eventsPerObjectPerMinute(eventsPerObjectPerMinute),
			requestCounter(0),
			maxObjectCount(0),
			lastTimestamp(Time::nowFast())
		{
		}

		bool checkLimit(int objectCount = 0)
		{
			Time now = Time::nowFast();

			if((now - lastTimestamp).seconds() > 60)
			{
				requestCounter = 0;
				lastTimestamp = now;
				maxObjectCount = 0;
			} 

			requestCounter++;
			maxObjectCount = std::max(objectCount, maxObjectCount);

			int totalCount = *eventsPerMinute;
			if(eventsPerObjectPerMinute)
				totalCount += maxObjectCount * (*eventsPerObjectPerMinute);

			if(requestCounter > totalCount)
				return false;

			return true;
		}
	};

	class BudgetedThrottlingHelper
	{
		float currentBudget;

	public:
		BudgetedThrottlingHelper() : currentBudget(0.0) {}

		void addBudget(float budget, float maxBudget)
		{
			currentBudget = std::min(currentBudget + budget, maxBudget);
		}

		bool checkAndReduceBudget()
		{
			if (currentBudget < 0)
				return false;

			currentBudget--;
			return true;
		}

		float getBudget() { return currentBudget; };
	};
	  
	// A class that follows the pattern of RunningAverage, but keeps track of time spent in a cyclical task.
	template<Time::SampleMethod sampleMethod = Time::Benchmark>
	class RunningAverageDutyCycle
	{
	public:
		RunningAverageDutyCycle(double lerp)
			:time(lerp),interval(lerp) {}

		RunningAverageDutyCycle(double lerp, int timeBufferSize)
			:time(lerp, 0, timeBufferSize),interval(lerp) {}

		void sample(Time::Interval elapsedTime)
		{
			interval.sample();
			time.sample(elapsedTime.seconds());
		}

		RBX::Time startSample() const
		{
			return Time::now<sampleMethod>();
		}

		void stopSample(RBX::Time start)
		{
			sample(Time::now<sampleMethod>() - start);
		}

		double dutyCycle() const 
		{
			double averageInterval = interval.value().seconds();
			double averageTime = time.value();
			return averageInterval!=0 ? averageTime/averageInterval : (averageTime>0 ? 1 : 0);
		}

		double rate() const 
		{
			return interval.rate();
		}

		const RunningAverageTimeInterval<sampleMethod>& stepInterval() const 
		{
			return interval;
		}

		Time::Interval lastStepInterval()
		{
			return interval.lastSample();
		}

		const RunningAverage<double>& stepTime() const 
		{
			return time;
		}

        double getIntervalLerp() const
        {
            return interval.getLerp();
        }

	private:
		RunningAverage<double> time;
		RunningAverageTimeInterval<sampleMethod> interval;
	};


	// A class that follows the pattern of WindowAverage, but keeps track of time spent in a cyclical task.
	template<Time::SampleMethod sampleMethod = Time::Benchmark>
	class WindowAverageDutyCycle
	{
	public:
		WindowAverageDutyCycle(Time::Interval maxWindow)
			:time(16),interval(maxWindow) {}

		void setMaxWindow(Time::Interval maxWindow) 
		{ 
			interval.setMaxWindow(maxWindow); 
			if(maxWindow.seconds() == 0.0)
			{
				// special case, release memory
				time.setMaxSamples(16);
			}
		};
		Time::Interval getMaxWindow() const { return interval.getMaxWindow(); };

		void sample(Time::Interval elapsedTime)
		{
			interval.sample();

			// make sure time's buffer size tracks interval's buffer size.
			if(time.getMaxSamples() != interval.getCapacity())
			{
				time.setMaxSamples(interval.getCapacity());
			}

			time.sample(elapsedTime.seconds());
		}

		struct Stats
		{
			Stats(	const typename WindowAverageTimeInterval<sampleMethod>::Stats& interval,
					const WindowAverage<double>::Stats& time,
					double dutyfraction )
					: interval(interval)
					, time(time)
					, dutyfraction(dutyfraction)
				{};
			typename WindowAverageTimeInterval<sampleMethod>::Stats interval;
			WindowAverage<double>::Stats time;
			double dutyfraction; // 1.0: duty time is 100% of interval time
		};

		Stats getStats(size_t samples = ~0) const
		{
			typename WindowAverage<>::Stats timestats = time.getStats(samples);
			typename WindowAverageTimeInterval<sampleMethod>::Stats intervalstats = interval.getStats(samples);

			double interval = intervalstats.average.seconds();
			return Stats(intervalstats, timestats, interval ? timestats.average / interval : 0);
		}

		template<class F>
		void iterTimes(F& f) const
		{
			time.iter(f);
		}

		template<class F>
		void iterIntervals(F& f) const
		{
			interval.iter(f);
		}

		struct GTCounter
		{
			GTCounter(double gt) : c(0), gtValue(gt) {};
			size_t c;
			double gtValue;
			void operator()(double dt)
			{
				if(dt > gtValue)
					c++;
			}
		};

		size_t countTimesGreaterThan(Time::Interval dt) const
		{
			GTCounter count(dt.seconds());
			iterTimes(count);
			return count.c;
		}

		size_t countIntervalsGreaterThan(Time::Interval dt) const
		{
			GTCounter count(dt.seconds());
			iterIntervals(count);
			return count.c;
		}

		void clear()
		{
			time.clear();
			interval.clear();
		}

		size_t timesamples() const
		{
			return time.size();
		}

		size_t intervalsamples() const
		{
			return interval.size();
		}

	private:
		WindowAverage<double> time;
		WindowAverageTimeInterval<sampleMethod> interval;
	};


	// A thread-safe, lock-free DutyCycle meter
	template<int windowSeconds>
	class ActivityMeter
	{
		static const int bucketCount = windowSeconds * 1024;
		boost::array<char, bucketCount> buckets; 
        rbx::atomic<int> currentTime;
        rbx::atomic<int> currentValue;
        rbx::atomic<int> totalValue;
		RBX::Time startTime;
		RBX::Time lastSampleTime;

	public:
		ActivityMeter()
			:currentTime(-1)
			,currentValue(0)
			,totalValue(0)
			,startTime(RBX::Time::now<Time::Fast>())
		{
			for (size_t i=0; i<buckets.size(); ++i)
				buckets[i] = 0;
		}

		double averageValue()
		{
			updateBuckets();
			return (double)totalValue / (double)bucketCount;
		}

		void increment()
		{
			updateBuckets();
			++currentValue;
		}

		void decrement()
		{
			updateBuckets();
			--currentValue;
		}

		void updateBuckets()
		{
			RBX::Time now = Time::now<Time::Fast>();
			if (lastSampleTime == now)
				return;

			lastSampleTime = now;
			unsigned long newTime = ((unsigned long)(bucketCount * (lastSampleTime - startTime).seconds()));

			unsigned long oldTime = currentTime.swap(newTime);

			if (oldTime < newTime)
			{
				long newValue = currentValue;
				for (unsigned long i = oldTime + 1; i <= newTime; ++i)
				{
					int index = i % bucketCount;
					int oldBucketValue = buckets[index];

					for (int j = 0; j < oldBucketValue; ++j)
						--totalValue;
					buckets[index] = (char)newValue;

					for (int j = 0; j < newValue; ++j)
						++totalValue;
				}
			}
		}
	};

	template<int windowSeconds>
	class InvocationMeter
	{
		static const int bucketCount = windowSeconds * 1024;
		boost::array<char, bucketCount> buckets; 
        rbx::atomic<int> currentTime;
        rbx::atomic<int> totalValue;
		RBX::Time startTime;
		RBX::Time lastSampleTime;

	public:
		InvocationMeter()
			:currentTime(-1)
			,totalValue(0)
			,startTime(RBX::Time::now<Time::Fast>())
		{
			for (size_t i=0; i<buckets.size(); ++i)
				buckets[i] = 0;
		}

		double getTotalValuePerSecond()
		{
			updateBuckets(false);
			return totalValue/windowSeconds;
		}

		void increment()
		{
			updateBuckets(true);
		}

		void updateBuckets(bool increment)
		{
			RBX::Time now = Time::now<Time::Fast>();
			if (lastSampleTime == now)
				return;

			lastSampleTime = now;
			unsigned long newTime = ((unsigned long)(bucketCount * (lastSampleTime - startTime).seconds()));
			unsigned long oldTime = currentTime.swap(newTime);
//			RBXASSERT(oldTime <= newTime);
//			if (oldTime != newTime)
			if (oldTime < newTime)		// changed per Erik 11/18/09

			{
				for (unsigned long i = oldTime + 1; i <= newTime; ++i)
				{
					int index = i % bucketCount;
					int oldBucketValue = buckets[index];

					for (int j = 0; j < oldBucketValue; ++j)
						--totalValue;
					buckets[index] = 0;
				}
			}
			if(increment){
				int newValue = 1;
				int newIndex = newTime % bucketCount;
				buckets[newIndex] = newValue;

				for (int j = 0; j < newValue; ++j)
						++totalValue;
			}
		}
	};
}
