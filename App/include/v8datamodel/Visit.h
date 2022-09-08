/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

#include "rbx/boost.hpp"
#include <boost/thread/condition.hpp>

namespace RBX {
	namespace Network {
		class Player;
	}

	extern const char* const sVisit;

	class ModelInstance;
	class PartInstance;
	class Workspace;

	class Visit 
		: public DescribedCreatable<Visit, Instance, sVisit, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
	private:
		std::string uploadUrl;
		scoped_ptr<RBX::worker_thread> pingThread;

	public:
		Visit();
		~Visit();

		void setPing(std::string url, int interval);
		static worker_thread::work_result ping(std::string url, int interval);

		void setUploadUrl(std::string value);
		std::string getUploadUrl() { return uploadUrl; }
	};

} // namespace