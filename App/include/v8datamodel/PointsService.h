#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

namespace RBX
{
	extern const char* const sPointsService;

	class PointsService
		: public DescribedCreatable<PointsService, Instance, sPointsService, Reflection::ClassDescriptor::INTERNAL>
		, public Service
	{
	public:
		typedef std::pair<boost::function<void(shared_ptr<const Reflection::Tuple>)>, boost::function<void(std::string)> > AwardPointYieldFunction;
		typedef std::vector<AwardPointYieldFunction> AwardPointYieldFunctions;
		typedef std::pair<int, AwardPointYieldFunctions> PointRequestValue;
		typedef boost::unordered_map<int, PointRequestValue> PointRequestMap;

	private:
		typedef DescribedCreatable<PointsService, Instance, sPointsService, Reflection::ClassDescriptor::INTERNAL> Super;

		rbx::signals::scoped_connection heartbeatSignalConnection;

		boost::mutex batchAwardPointsMutex;
		PointRequestMap batchAwardPointRequests;

		int numOfAwardPointCallsLastMinute;
		double timeSinceLastMaxAwardPointReset;
		double timeSinceLastAwardPointCall;
		bool shouldBatchAwardPoints;

		bool isAtAwardPointsLimit();

		void onHeartbeat(const Heartbeat& heartbeat);
		static void startAwardPointsBatching(weak_ptr<RBX::DataModel> weakDm);

		////////////////////////////////////////////////////
		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		bool canUseService();
		void internalGetPointBalance(int userId, int placeId, shared_ptr<std::string> methodName, shared_ptr<std::string> keyToReturn, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
	public:
		PointsService();

		rbx::remote_signal<void(int, int, int, int)> pointsAwardedSignal;

		void getUserPointBalanceInUniverse(int userId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getUserPointBalance(int userId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);

		int getAwardableBalance();

		void awardPoints(int userId, int amount, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void firePointsAwardedSignal(int userId, int amount, int userBalanceInUniverse, int userBalance);

		bool doBatchAwardPoints();
		void resetPointAwardCount();

		void listenToHeartbeat();
	};
}