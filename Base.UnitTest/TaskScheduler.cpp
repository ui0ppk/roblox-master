#include <boost/test/unit_test.hpp>

#include "rbx/TaskScheduler.h"
#include "rbx/TaskScheduler.Job.h"

BOOST_AUTO_TEST_SUITE(TaskScheduler)

class ParallelTestJob : public RBX::TaskScheduler::Job
{
public:
	int maxConcurrency;
	ParallelTestJob()
		:Job("ParallelTestJob", shared_ptr<RBX::TaskScheduler::Arbiter>())
	{
		maxConcurrency = 0;
	}
	RBX::Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, 10);
	}

	virtual Job::Error error(const Stats& stats)
	{
		Job::Error result = computeStandardErrorCyclicExecutiveSleeping(stats, 10);	
		return result;
	}

	virtual RBX::TaskScheduler::StepResult step(const Stats& stats)
	{
		RBX::Time::Interval(0.1).sleep();
		if (this->allotedConcurrency > maxConcurrency)
			maxConcurrency = this->allotedConcurrency;
		return RBX::TaskScheduler::Stepped;
	}
	virtual int getDesiredConcurrencyCount() const 
	{
		return 4;
	}
protected:
	/*implement*/ double getPriorityFactor() { return 1.0; }

};

BOOST_AUTO_TEST_CASE(ParallelTest)
{
	shared_ptr<ParallelTestJob> job(new ParallelTestJob());

	RBX::TaskScheduler::singleton().add(job);

	for (int i = 0; i < 20; ++i)
	{
		RBX::Time::Interval(1).sleep();
		if (job->maxConcurrency > 1)
			break;
	}
	BOOST_CHECK_GT(job->maxConcurrency, 1);

	RBX::TaskScheduler::singleton().removeBlocking(job);
}

BOOST_AUTO_TEST_SUITE_END()

