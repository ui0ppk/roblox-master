#include "stdafx.h"

#include <boost/lexical_cast.hpp>

#include "v8xml/WebParser.h"

#include "Util/RobloxGoogleAnalytics.h"

#include "v8datamodel/PointsService.h"
#include "v8datamodel/ContentProvider.h"

#include "v8datamodel/ServerScriptService.h"
#include "v8datamodel/HttpRbxApiService.h"

#include "Network/Players.h"

#define AWARD_POINTS_METHOD_NAME "PointsService:AwardPoints"
#define AWARD_POINTS_RATE_LIMIT_RESET 60.0

DYNAMIC_FASTINTVARIABLE(PointBalanceCacheInvalidateTimeMs, 1000)

DYNAMIC_FASTINTVARIABLE(MaxAwardPointsHttpCallsPerMinute, 60)
DYNAMIC_FASTINTVARIABLE(SecondsPerBatchAwardPointsCall, 10)

namespace RBX
{
	const char* const sPointsService = "PointsService";

	static const char* kPointBalanceKey = "pointBalance";

    REFLECTION_BEGIN();
	static Reflection::BoundYieldFuncDesc<PointsService, int(int)>				func_getPointBalance(&PointsService::getUserPointBalance, "GetPointBalance", "userId", Security::None, Reflection::Descriptor::Attributes::deprecated());
	static Reflection::BoundYieldFuncDesc<PointsService, int(int)>				func_getPointBalanceUniverse(&PointsService::getUserPointBalanceInUniverse, "GetGamePointBalance", "userId", Security::None);
	static Reflection::BoundFuncDesc<PointsService, int(void)>					func_getAwardBalance(&PointsService::getAwardableBalance, "GetAwardablePoints", Security::None, Reflection::Descriptor::Attributes::deprecated());

	static Reflection::BoundYieldFuncDesc<PointsService, shared_ptr<const Reflection::Tuple>(int,int)>			func_awardPoints(&PointsService::awardPoints, "AwardPoints", "userId", "amount", Security::None);

	static Reflection::RemoteEventDesc<PointsService, void(int, int, int, int)>		event_PointsAwarded(&PointsService::pointsAwardedSignal, "PointsAwarded","userId","pointsAwarded","userBalanceInGame","userTotalBalance", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
    REFLECTION_END();

	PointsService::PointsService() :
		numOfAwardPointCallsLastMinute(0),
		timeSinceLastMaxAwardPointReset(0),
		timeSinceLastAwardPointCall(0),
		shouldBatchAwardPoints(false)
	{
		setName(sPointsService);
	}

	void PointsService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
		if (oldProvider)
		{
			heartbeatSignalConnection.disconnect();
		}

		Super::onServiceProvider(oldProvider, newProvider);
	}

	bool PointsService::canUseService()
	{
        if (DataModel* dm = DataModel::get(this))
        {
            ServerScriptService* sss = ServiceProvider::find<ServerScriptService>(dm);

            if (!sss || sss->getLoadStringEnabled())
            {
                return false;
            }
        }
		
		return true;
	}

	void PointsService::listenToHeartbeat()
	{
		if (RunService* runService = ServiceProvider::find<RunService>(this))
		{
			heartbeatSignalConnection = runService->heartbeatSignal.connect(boost::bind(&PointsService::onHeartbeat, this, _1));
		}
	}

	void PointsService::startAwardPointsBatching(weak_ptr<RBX::DataModel> weakDm)
	{
		if (shared_ptr<RBX::DataModel> dm = weakDm.lock())
		{
			if (PointsService* pointsService = ServiceProvider::find<PointsService>(dm.get()) )
			{
				pointsService->listenToHeartbeat();
			}
		}
	}

	void PointsService::onHeartbeat(const Heartbeat& heartbeat)
	{
		timeSinceLastMaxAwardPointReset += heartbeat.wallStep;
		timeSinceLastAwardPointCall += heartbeat.wallStep;

		if (timeSinceLastMaxAwardPointReset >= AWARD_POINTS_RATE_LIMIT_RESET)
		{
			resetPointAwardCount();
		}

		if ( (shouldBatchAwardPoints && (timeSinceLastAwardPointCall >= DFInt::SecondsPerBatchAwardPointsCall)) ||
			 !shouldBatchAwardPoints)
		{
			bool oldShouldBatch = shouldBatchAwardPoints;
			shouldBatchAwardPoints = doBatchAwardPoints();

			// if we just went over the limit and didn't do that last minute, then reset the time
			if (shouldBatchAwardPoints && !oldShouldBatch)
			{
				resetPointAwardCount();
			}

			timeSinceLastAwardPointCall = 0.0;
		}
	}

	bool PointsService::isAtAwardPointsLimit()
	{
		return (numOfAwardPointCallsLastMinute > DFInt::MaxAwardPointsHttpCallsPerMinute);
	}

	void PointsService::resetPointAwardCount()
	{
		boost::mutex::scoped_lock batchLock(batchAwardPointsMutex);
		numOfAwardPointCallsLastMinute = 0;
		timeSinceLastMaxAwardPointReset = 0.0;
	}

	///////////////////////////////////////////////////
	// Get Functions

	static void GetHelperSuccess(weak_ptr<DataModel> weakDm, shared_ptr<std::string> methodName, shared_ptr<std::string> keyToCheck, std::string response, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (shared_ptr<DataModel> dm = weakDm.lock())
		{
			RBX::PointsService* pointsService = ServiceProvider::find<RBX::PointsService>(dm.get());
			if (!pointsService)
			{
				errorFunction( RBX::format("%s failed because PointsService is unavailable.",methodName->c_str()) );
				return;
			}

			if (!response.empty())
			{
				shared_ptr<const Reflection::ValueTable> responseTable(rbx::make_shared<const Reflection::ValueTable>());
				if(!WebParser::parseJSONTable(response,responseTable))
				{
					errorFunction( RBX::format("%s failed because could not parse JSON response",methodName->c_str()) );
					return;
				}

				if (responseTable && responseTable->size() > 0)
				{
					int intToReturn = 0;
					try
					{					
						Reflection::Variant intVariant = responseTable->at(keyToCheck->c_str());
						intToReturn = intVariant.cast<int>();
					}
					catch (base_exception &e)
					{
						errorFunction( RBX::format("%s failed because %s",methodName->c_str(),e.what()) );
						return;
					}

					resumeFunction(intToReturn);
					return;
				}
				else
				{
					errorFunction( RBX::format("%s failed because JSON table is empty or null.",methodName->c_str()) );
					return;
				}
			}
			else
			{
				errorFunction( RBX::format("%s failed because no valid response or exception.", methodName->c_str()) );
				return;
			}
		}
		else
		{
			errorFunction( RBX::format("%s failed because could not lock datamodel in response.", methodName->c_str()) );
			return;
		}
	}
	static void GetHelperError(weak_ptr<DataModel> weakDm, shared_ptr<std::string> methodName, std::string error, boost::function<void(std::string)> errorFunction)
	{
		if (!error.empty())
		{
			errorFunction( RBX::format("%s failed because %s", methodName->c_str(), error.c_str()) );
		}
		else
		{
			errorFunction( RBX::format("%s failed because no valid response or exception.", methodName->c_str()) );
		}
	}

	void PointsService::internalGetPointBalance(int userId, int placeId, shared_ptr<std::string> methodName, shared_ptr<std::string> keyToReturn, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if ( !canUseService() )
		{
			errorFunction("PointsService requires beta access");
			return;
		}

		if (!Network::Players::backendProcessing(this))
		{
			errorFunction( RBX::format("%s failed because not called from server script.",methodName->c_str()) );
			return;
		}

		if (userId <= 0 )
		{
			errorFunction( RBX::format("%s failed because userId <= 0, please supply a positive userId.",methodName->c_str()) );
			return;
		}

		DataModel* dm = DataModel::get(this);
		if (!dm)
		{
			errorFunction( RBX::format("%s failed because of null datamodel.",methodName->c_str()) );
			return;
		}

		std::string parameters = RBX::format("?userId=%d",userId);
		std::string apiBaseUrl = ServiceProvider::create<ContentProvider>(this)->getUnsecureApiBaseUrl();

		if (placeId > 0)
		{
			parameters += RBX::format("&placeId=%d",placeId);
		}

		try
		{
			// now post to the website
			Http getPointBalanceHttp((apiBaseUrl + "points/get-point-balance" + parameters).c_str());
#ifndef _WIN32
			getPointBalanceHttp.setAuthDomain(apiBaseUrl);
#endif

			if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
			{
				apiService->getAsync( getPointBalanceHttp, RBX::PRIORITY_DEFAULT,
					boost::bind(&GetHelperSuccess, weak_from(dm), methodName, keyToReturn, _1, resumeFunction, errorFunction),
					boost::bind(&GetHelperError, weak_from(dm), methodName, _1, errorFunction) );
			}
		}
		catch (base_exception &e)
		{
			errorFunction( RBX::format("%s failed because %s", methodName->c_str(), e.what()) );
		}
	}

	void PointsService::getUserPointBalanceInUniverse(int userId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		shared_ptr<std::string> methodName(new std::string("PointsService:GetUserPointBalanceInUniverse"));

		DataModel* dm = DataModel::get(this);
		if (!dm)
		{
			errorFunction( RBX::format("%s failed because of null datamodel.",methodName->c_str()) );
			return;
		}

		shared_ptr<std::string> JSONkeyToReturn(new std::string(kPointBalanceKey));
		internalGetPointBalance(userId,dm->getPlaceID(),methodName,JSONkeyToReturn,resumeFunction,errorFunction);
	}

	void PointsService::getUserPointBalance(int userId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		shared_ptr<std::string> methodName(new std::string("PointsService:GetUserPointBalance"));
		shared_ptr<std::string> JSONkeyToReturn(new std::string(kPointBalanceKey));

		internalGetPointBalance(userId,0,methodName,JSONkeyToReturn,resumeFunction,errorFunction);
	}

	int PointsService::getAwardableBalance()
	{
		return std::numeric_limits<int>::max();
	}


	static void sendPointAwardStats(int placeId)
	{
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HasAwardedAPoint", format("%d", placeId).c_str());
	}

	static void fireErrorFunctions(PointsService::AwardPointYieldFunctions& yieldingFunctions, const char* error)
	{
		for (PointsService::AwardPointYieldFunctions::iterator iter = yieldingFunctions.begin(); iter != yieldingFunctions.end(); ++iter)
		{
			iter->second(error);
		}
	}

	static void fireResumeFunctions(PointsService::AwardPointYieldFunctions& yieldingFunctions, shared_ptr<const Reflection::Tuple> args)
	{
		for (PointsService::AwardPointYieldFunctions::iterator iter = yieldingFunctions.begin(); iter != yieldingFunctions.end(); ++iter)
		{
			iter->first(args);
		}
	}

	///////////////////////////////////////////////////
	// Set Functions

	static void processAwardPointsResponseBatchSuccess(weak_ptr<DataModel> weakDm, int userId, PointsService::AwardPointYieldFunctions yieldingFunctions, std::string response)
	{
		if (shared_ptr<DataModel> dm = weakDm.lock())
		{
			RBX::PointsService* pointsService = NULL;
			{
				pointsService = ServiceProvider::find<RBX::PointsService>(dm.get());
				if (!pointsService)
				{
					RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Processing PointsService:AwardPoints response and could not find PointsService, userId = %i",userId);
					return;
				}
			}

			if (!response.empty())
			{
				shared_ptr<const Reflection::ValueTable> awardPointsTable(rbx::make_shared<const Reflection::ValueTable>());
				WebParser::parseJSONTable(response,awardPointsTable);

				if (awardPointsTable && awardPointsTable->size() > 0)
				{
					Reflection::Variant successVariant = awardPointsTable->at("success");
					if (successVariant.isType<bool>())
					{
						bool success = successVariant.cast<bool>();
						if (success)
						{
							int universeBalance = 0;
							int userBalance = 0;
							int amountAwarded = 0;

							try
							{
								Reflection::Variant universeBalanceVariant = awardPointsTable->at("userGameBalance");
								universeBalance = universeBalanceVariant.cast<int>();

								Reflection::Variant userBalanceVariant = awardPointsTable->at("userBalance");
								userBalance = userBalanceVariant.cast<int>();

								Reflection::Variant amountAwardedVariant = awardPointsTable->at("pointsAwarded");
								amountAwarded = amountAwardedVariant.cast<int>();
							}
							catch(base_exception &e)
							{
								fireErrorFunctions(yieldingFunctions, RBX::format("Processing PointsService:AwardPoints response, but failed because %s. userId = %i",e.what(), userId).c_str());
								return;
							}

							shared_ptr<Reflection::Tuple> args = rbx::make_shared<Reflection::Tuple>();
							args->values.push_back(userId);
							args->values.push_back(amountAwarded);
							args->values.push_back(universeBalance);
							args->values.push_back(userBalance);

							fireResumeFunctions(yieldingFunctions, args);
							pointsService->firePointsAwardedSignal(userId, amountAwarded, universeBalance, userBalance);

							static boost::once_flag flag = BOOST_ONCE_INIT;
							boost::call_once(flag, boost::bind(sendPointAwardStats, dm->getPlaceID()));

							return;
						}
						else
						{
							fireErrorFunctions(yieldingFunctions, RBX::format("Processing PointsService:AwardPoints response, but not successful award. userId = %i", userId).c_str());
							return;
						}
					}
					else
					{
						fireErrorFunctions(yieldingFunctions, RBX::format("Processing PointsService:AwardPoints response, but JSON table is empty. userId = %i",userId).c_str());
						return;
					}
				}
				else
				{
					fireErrorFunctions(yieldingFunctions, RBX::format("Processing PointsService:AwardPoints response, but JSON table is null. userId = %i",userId).c_str());
					return;
				}
			}
			else
			{
				fireErrorFunctions(yieldingFunctions, RBX::format("Processing PointsService:AwardPoints response, but response has length of 0. userId = %i",userId).c_str());
				return;
			}
		}
		else
		{
			fireErrorFunctions(yieldingFunctions, RBX::format("Processing PointsService:AwardPoints but could not get lock on datamodel. userId = %i",userId).c_str());
			return;
		}
	}

	static void processAwardPointsResponseBatchError(weak_ptr<DataModel> weakDm, int userId, PointsService::AwardPointYieldFunctions yieldingFunctions, std::string error)
	{
		fireErrorFunctions(yieldingFunctions, RBX::format("Processing PointsService:AwardPoints error: %s. userId = %i", error.c_str(), userId).c_str());
	}

	void PointsService::firePointsAwardedSignal(int userId, int amount, int userBalanceInUniverse, int userBalance)
	{
		event_PointsAwarded.fireAndReplicateEvent(this, userId, amount, userBalanceInUniverse, userBalance);
	}

	bool PointsService::doBatchAwardPoints()
	{
		bool batchNextAwardCalls = false;

		if (DataModel* dm = DataModel::get(this))
		{
			const std::string parameters = "?placeId=%d&userId=%d&amount=%d";
			const std::string apiBaseUrl = ServiceProvider::create<ContentProvider>(this)->getUnsecureApiBaseUrl();
			std::string url = apiBaseUrl + "points/award-points" + parameters;

			std::string postData = "";

			boost::mutex::scoped_lock batchLock(batchAwardPointsMutex);
			for (PointRequestMap::iterator iter = batchAwardPointRequests.begin(); iter != batchAwardPointRequests.end(); ++iter)
			{	
				if (isAtAwardPointsLimit())
				{
					batchNextAwardCalls = true;
					fireErrorFunctions(iter->second.second, RBX::format("%s failed because the max number of calls per minute has been exceeded. Don't call this more than %i times per minute per user.", AWARD_POINTS_METHOD_NAME, DFInt::MaxAwardPointsHttpCallsPerMinute).c_str());
					continue;
				}

				try
				{
					numOfAwardPointCallsLastMinute++;

					const int userId = iter->first;
					const int amount = iter->second.first;

					if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
					{
						const std::string formattedPath = RBX::format(("points/award-points" + parameters).c_str(),dm->getPlaceID(),userId,amount);

						apiService->postAsync( formattedPath.c_str(), postData, false, RBX::PRIORITY_DEFAULT, HttpService::TEXT_PLAIN,
							boost::bind(&processAwardPointsResponseBatchSuccess, weak_from(dm), userId, iter->second.second, _1),
							boost::bind(&processAwardPointsResponseBatchError, weak_from(dm), userId, iter->second.second, _1) );
					}
				}
				catch (base_exception &e)
				{
					fireErrorFunctions(iter->second.second, RBX::format("%s failed because %s",e.what(), AWARD_POINTS_METHOD_NAME).c_str());
					return batchNextAwardCalls;
				}
			}
			batchAwardPointRequests.clear();
		}

		return batchNextAwardCalls;
	}

	void PointsService::awardPoints(int userId, int amount, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if( !canUseService() )
		{
			errorFunction("PointsService requires loadstring to be disabled.");
			return;
		}

		if (!Network::Players::backendProcessing(this))
		{
			errorFunction( RBX::format("%s failed because not called from server script.", AWARD_POINTS_METHOD_NAME) );
			return;
		}

		if (userId <= 0 )
		{
			errorFunction( RBX::format("%s failed because userId <= 0, please supply a positive userId.", AWARD_POINTS_METHOD_NAME) );
			return;
		}

		if (amount == 0)
		{
			errorFunction( RBX::format("%s failed because trying to award zero points, please supply a non-zero amount.", AWARD_POINTS_METHOD_NAME) );
			return;
		}

		DataModel* dm = DataModel::get(this);
		if (!dm)
		{
			errorFunction( RBX::format("%s failed because of null datamodel.", AWARD_POINTS_METHOD_NAME) );
			return;
		}

		boost::mutex::scoped_lock batchLock(batchAwardPointsMutex);

		PointRequestMap::iterator findIter = batchAwardPointRequests.find(userId);
		if (findIter != batchAwardPointRequests.end())
		{
			findIter->second.first += amount;
			findIter->second.second.push_back(AwardPointYieldFunction(resumeFunction,errorFunction));
		}
		else
		{
			AwardPointYieldFunctions yieldFunctions;
			yieldFunctions.push_back(AwardPointYieldFunction(resumeFunction,errorFunction));

			PointRequestValue pointRequestValue = PointRequestValue(amount,yieldFunctions);
			batchAwardPointRequests[userId] = pointRequestValue;
		}

		static boost::once_flag StartAwardPointsBatchingFlag = BOOST_ONCE_INIT;
		boost::call_once( StartAwardPointsBatchingFlag, boost::bind(&PointsService::startAwardPointsBatching, weak_from(dm)) );
	}
}
