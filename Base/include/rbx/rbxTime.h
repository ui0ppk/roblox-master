#pragma once

#include <limits>
#include <iosfwd>

#ifdef _WIN32
#undef min
#undef max
#endif

namespace RBX {

	/*
	Records time in a variety of ways: CPU counters, OS Times, Multimedia Timers, etc.

	The purpose of this class is to shield the user from OS-specific time functions. This
	is why in the original design, the "sec" field was not exposed publicly. Unfortunately,
	some functions have started to expose this, which breaks the intended encapsulation.

	The design intent is that to get a numerical time value you subtract two Time instances
	to get an Interval.

	For performance reasons you generally want to use the "Fast" timer. However, there
	is a "preciseOverride" if you want to have runtime control over certain time queries
	for benchmarking purposes.
	*/
	class Time {
	public:
		//! Relative time Interval.
		class Interval {
			double sec;
		public:
			inline Interval() : sec(0) {};

			static inline Interval max() { return Interval(std::numeric_limits<double>::max()); }
			static inline Interval zero() { return Interval(0); }

			static inline Interval from_milliseconds(double milliseconds) { return Interval(0.001 * milliseconds); }
			static inline Interval from_seconds(double seconds) { return Interval(seconds); }
			static inline Interval from_minutes(double minutes) { return Interval(60.0 * minutes); }
			static inline Interval from_hours(double hours) { return Interval(60.0 * 60.0 * hours); }

			inline explicit Interval( double seconds ):sec(seconds) {}

			inline double seconds() const { return sec; }

			inline double msec() const { return sec*1000; }

			inline bool isZero() const { return sec==0; }

			friend class Time;

			friend Interval operator-( const Time& t1, const Time& t0 );

			friend Interval operator+( const Interval& i, const Interval& j ) {
				return Interval(i.sec+j.sec);
			}

			friend Interval operator-( const Interval& i, const Interval& j ) {
				return Interval(i.sec-j.sec);
			}

			Interval& operator+=( const Interval& i ) {sec += i.sec; return *this;}

			Interval& operator-=( const Interval& i ) {sec -= i.sec; return *this;}

			bool operator>( const Interval& j ) const { return sec > j.sec; }
			bool operator<( const Interval& j ) const { return sec < j.sec; }
			bool operator>=( const Interval& j ) const { return sec >= j.sec; }
			bool operator<=( const Interval& j ) const { return sec <= j.sec; }
			bool operator==( const Interval& j ) const { return sec == j.sec; }
			bool operator!=( const Interval& j ) const { return sec != j.sec; }

			void sleep();

			template<class charT, class traits>
			friend std::basic_ostream<charT, traits>&
				operator<< (std::basic_ostream<charT, traits> &out, 
				Interval interval)
			{
				out << interval.sec;
				return out;
			}
		};

		//! Construct an absolute timestamp initialized to zero.
		inline Time() : sec(0) {};

		inline static Time max() { return Time(std::numeric_limits<double>::max()); }

		typedef enum { Fast, Benchmark, Precise, Multimedia } SampleMethod;
		// If preciseOverride==Fast then Fast and Benchmark are precise
		// If preciseOverride==Benchmark then Benchmark is precise
		static SampleMethod preciseOverride;	

		static bool isSpeedCheater();
		static bool isDebugged();

		//! Return current time.
		template<SampleMethod sampleMethod>
		static Time now();

		static Time now(SampleMethod sampleMethod);

		static long long getTickCount();
        static long long getStart();

		// Avoid using this! Instead, sample two Time::now() instances and subtract them
		static double nowFastSec();

		static Time nowFast();

		bool isZero() const { return sec==0; }

		Time operator+( const Interval& j ) const {
			return Time(sec + j.sec);
		}
		Time operator-( const Interval& j ) const {
			return Time(sec - j.sec);
		}

		Time& operator+=( const Interval& j ) {
			this->sec += j.sec;
			return *this;
		}
		Time& operator-=( const Interval& j ) {
			this->sec -= j.sec;
			return *this;
		}

		bool operator>( const Time& j ) const { return sec > j.sec; }
		bool operator<( const Time& j ) const { return sec < j.sec; }
		bool operator>=( const Time& j ) const { return sec >= j.sec; }
		bool operator<=( const Time& j ) const { return sec <= j.sec; }
		bool operator==( const Time& j ) const { return sec == j.sec; }
		bool operator!=( const Time& j ) const { return sec != j.sec; }

        template<class charT, class traits>
        friend std::basic_ostream<charT, traits>&
        operator<< (std::basic_ostream<charT, traits> &out, 
                    Time time)
        {
            out << time.sec;
            return out;
        }

		//! Subtract two timestamps to get the time Interval between
		friend Interval operator-( const Time& t1, const Time& t0 );

		// Avoid using this! Instead, sample two Time::now() instances and subtract them
		double timestampSeconds() const
		{
			return sec;
		}

	private:
		double sec;

	protected:
		Time(double sec) : sec(sec) {};
	};

	template<Time::SampleMethod sampleMethod>
	class Timer
	{
		Time start;
	public:
		Timer():start(Time::now<sampleMethod>()) {}
		Time::Interval delta() const { return Time::now<sampleMethod>() - start; }
		Time::Interval reset() 
		{ 
			Time now = Time::now<sampleMethod>();
			Time::Interval result = now - start;
			start = now; 
			return result;
		}
	};


	class RemoteTime : public Time {
	public:
		inline RemoteTime() : Time() {};
		inline RemoteTime(double value) : Time(value) {};
		RemoteTime(const Time& value) : Time(value) {};
	};
}