/* Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved */

#include "stdafx.h"
#include "V8DataModel/DataModel.h"
#include "Util/http.h"

namespace RBX {

	//
	// The purpose of this file is to enable DataModel to compile but to NOT
	// include any XML serializing code.  this makes "place stealing hacks"
	// more difficult to implement.
	//
	shared_ptr<std::stringstream> DataModel::serializeDataModel(const Instance::SaveFilter saveFilter)
	{
		return shared_ptr<std::stringstream>();
	}

	void DataModel::serverSave()
	{
	}

	static void HandleAsyncSaveResult(std::string *response, std::exception *exception, boost::function<void(bool)> resumeFunction)
	{
	}

	void DataModel::internalSaveAsync(ContentId contentId, boost::function<void(bool)> resumeFunction)
	{
	}

	void DataModel::internalSave(ContentId contentId)
	{
	}

	void DataModel::AsyncUploadPlaceResponseHandler(std::string* response, std::exception* exception, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
	}

	bool DataModel::uploadPlaceReturn(const bool succeeded, const std::string error, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
        return true;
	}

	bool DataModel::uploadPlace(const std::string& uploadUrl, const SaveFilter saveFilter, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
        return true;
	}

}