/*
 *  MD5.cpp - boost unit test for RBX::MD5
 *
 *  Copyright 2010 ROBLOX, INC. All rights reserved.
 *
 */

#include <boost/test/unit_test.hpp>
#include <boost/scoped_ptr.hpp>

#include "rbx/test/DataModelFixture.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/Test.h"
#include "v8datamodel/PathfindingService.h"
#include "v8xml/Serializer.h"

#include "rbx/test/ScopedFastFlagSetting.h"

#include "rbx/MathUtil.h"
#include "util/Statistics.h"
#include "GetTime.h"

#include "rbx/test/App.UnitTest.Lib.h"

void dummyUnlock(boost::mutex* mutex, shared_ptr<RBX::Instance>)
{
	mutex->unlock();
}

void dummyErrorUnlock(boost::mutex* mutex, std::string)
{
	mutex->unlock();
}

void dummyOK(shared_ptr<RBX::Instance>)
{
}

void dummyError(std::string)
{
}

#if 0

BOOST_FIXTURE_TEST_SUITE(PathfindingPerf, DataModelFixture)

BOOST_AUTO_TEST_CASE(PathfindingStress)
{
	std::ifstream stream("C:\\GDrive\\Work\\Crossroads_path.rbxl", std::ios_base::in | std::ios_base::binary);
	boost::mutex mutex;
	ScopedFastFlagSetting flag ("PathfindingEnabled", true);

	RBX::Security::Impersonator impersonate(RBX::Security::COM);
	{
		RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
		Serializer serializer;

		serializer.load(stream,dataModel.get());

		RBX::PathfindingService* service = RBX::ServiceProvider::create<RBX::PathfindingService>(dataModel.get());

		RBX::PartInstance* start = RBX::Instance::fastDynamicCast<RBX::PartInstance>(dataModel->getWorkspace()->findFirstChildByName("Start"));
		RBX::PartInstance* finish = RBX::Instance::fastDynamicCast<RBX::PartInstance>(dataModel->getWorkspace()->findFirstChildByName("Finish"));

		mutex.lock();
		service->computePathAsync(start->getTranslationUi(), finish->getTranslationUi(), 512.0f, true, true, boost::bind(&dummyUnlock, &mutex, _1), boost::bind(&dummyErrorUnlock, &mutex, _1));
	}

	mutex.lock();
	{
		RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
		RBX::PartInstance* start = RBX::Instance::fastDynamicCast<RBX::PartInstance>(dataModel->getWorkspace()->findFirstChildByName("Start"));
		RBX::PartInstance* finish = RBX::Instance::fastDynamicCast<RBX::PartInstance>(dataModel->getWorkspace()->findFirstChildByName("Finish"));
		RBX::PathfindingService* service = RBX::ServiceProvider::create<RBX::PathfindingService>(dataModel.get());

		unsigned statsRuns = 100;

		RakNet::Time start_time = RakNet::GetTime();

		RBX::WindowAverage<double, double> runAverage(statsRuns);
		for(unsigned iRuns = 0; iRuns < statsRuns; iRuns++)
		{
			RakNet::Time start_test_time = RakNet::GetTime();
			for(int i = 0; i < 10; i++)
				service->computePathAsync(start->getTranslationUi(), finish->getTranslationUi(), 512.0f, true, true, dummyOK, dummyError);
			runAverage.sample(RakNet::GetTime() - start_test_time);
		}

		printf("------------------ Elapsed Time : %ums --------------------\n", (unsigned int)(RakNet::GetTime() - start_time));

		if(statsRuns > 0)
		{
			RBX::WindowAverage<double, double>::Stats runStats = runAverage.getSanitizedStats();
			double confMin, confMax;
			GetConfidenceInterval(runStats.average, runStats.variance, RBX::C90, &confMin, &confMax);

			printf("Average: %.2f ms, std: %.2f ms, Confidence: %.2f-%.2f ms\n", 
				runStats.average, sqrt(runStats.variance), confMin, confMax);
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()

#endif
