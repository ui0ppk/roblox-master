#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>

namespace RBX {

	class DataModel;
	class Instance;

	class GamePerfMonitor
	{
		std::string baseUrl;
		std::string jobId;
		int placeId;
		int userId;
		bool shouldPostDiagStats;

		boost::shared_ptr<Instance> frmStats; // keep a shared pointer here so we can still use it after RenderView shuts down

		void collectAndPostStats(boost::weak_ptr<DataModel> dataModel);
		void postDiagStats(boost::shared_ptr<DataModel> dataModel);

		void onGameClose(boost::weak_ptr<DataModel> dataModel);


	public:
		GamePerfMonitor(const std::string& baseUrl, const std::string& jobId, int placeId, int userId) :
			baseUrl(baseUrl),
			jobId(jobId),
			placeId(placeId),
			userId(userId),
			shouldPostDiagStats(false)
		{};
		~GamePerfMonitor();

		void start(DataModel* dataModel);

		void setPostDiagStats(bool shouldPost) { shouldPostDiagStats = shouldPost; }
	};




} // namespace

