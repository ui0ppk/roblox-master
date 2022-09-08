/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#include "stdafx.h"

#include "Network/Players.h"
#include "Util/http.h"
#include "Util/Statistics.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/Workspace.h"
#include "V8Xml/Serializer.h"
#include "V8Xml/XmlSerializer.h"
#include "V8Xml/SerializerBinary.h"

#define MIN_HTTP_COMPRESSION_SIZE 256

DYNAMIC_FASTINTVARIABLE(HttpResponseExtendedTimeoutMillis, 600000)
DYNAMIC_FASTINTVARIABLE(HttpSendExtendedTimeoutMillis, 600000)
DYNAMIC_FASTINTVARIABLE(HttpConnectExtendedTimeoutMillis, 600000)
DYNAMIC_FASTINTVARIABLE(HttpDataSendExtendedTimeoutMillis, 600000)

namespace RBX {

shared_ptr<std::stringstream> DataModel::serializeDataModel(const Instance::SaveFilter saveFilter)
{
    shared_ptr<std::stringstream> stream(new std::stringstream());

    SerializerBinary::serialize(*stream, this, 0 , saveFilter);

    return stream;
}

void DataModel::serverSave()
{
	shared_ptr<std::stringstream> stream = serializeDataModel();

	std::string response;
	Http(serverSaveUrl).post(*stream, Http::kContentTypeDefaultUnspecified, true, response);

	Network::Players* players = ServiceProvider::find<Network::Players>(this);
	if (players && !Network::Players::isCloudEdit(this))
	{
		players->gamechat("Save Level Complete");
	}
}

static void HandleAsyncSaveResult(std::string *response, std::exception *exception, boost::function<void(bool)> resumeFunction)
{
	if(exception)
		resumeFunction(false);
	else
		resumeFunction(true);
}

void DataModel::internalSaveAsync(ContentId contentId, boost::function<void(bool)> resumeFunction)
{
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
	std::string assetId = contentId.toString().substr(contentId.toString().find("assetid=") + std::string("assetid=").length());

	try
	{
		shared_ptr<std::stringstream> stream = serializeDataModel();

		Http saveRequest(contentId.toString());

        saveRequest.post(stream, Http::kContentTypeDefaultUnspecified, true, boost::bind(&HandleAsyncSaveResult, _1, _2, resumeFunction));

	}
	catch(std::bad_alloc&)
	{
		resumeFunction(false);
	}
	catch(std::exception&)
	{
		resumeFunction(false);
	}
}


void DataModel::internalSave(ContentId contentId)
{
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
	std::string assetId = contentId.toString().substr(contentId.toString().find("assetid=") + std::string("assetid=").length());
	time_t startTime = time(NULL);
	ReportStatisticWithMessage(GetBaseURL(),"SaveLevel Begin","","assetId",assetId.c_str());
	std::stringstream stream;
    std::string response;
	try
	{
		shared_ptr<std::stringstream> stream = serializeDataModel();

		Http saveRequest(contentId.toString());
        saveRequest.setResponseTimeout(DFInt::HttpResponseExtendedTimeoutMillis);
        saveRequest.setSendTimeout(DFInt::HttpSendExtendedTimeoutMillis);
        saveRequest.setDataSendTimeout(DFInt::HttpDataSendExtendedTimeoutMillis);
        saveRequest.setConnectionTimeout(DFInt::HttpConnectExtendedTimeoutMillis);

        saveRequest.post(*stream, Http::kContentTypeDefaultUnspecified, true, response);

		time_t endTime = time(NULL);
		std::stringstream message;
		message << "double" << "\t" << "size" << "\t" << stream->tellp()/1000 << "\n";
		message << "double" << "\t" << "time" << "\t" << (endTime - startTime) / 30 << "\n";
		message << char(0);
		ReportStatisticPost(GetBaseURL(),"SaveLevel Complete",message.str(),"assetId",assetId.c_str());
	}
	catch(std::bad_alloc&)
	{
		throw;
	}
	catch(std::exception&)
	{
		time_t endTime = time(NULL);
		std::string logFile = "";
		{
			dataModelReportingData.clear();
			workspace.get()->visitDescendants(boost::bind(&DataModel::traverseDataModelReporting,this,_1));

			std::stringstream logFileStr;
			logFileStr << "Error when uploading assetId=" << assetId << std::endl;
			logFileStr << "size=" << stream.tellp() << std::endl;
			logFileStr << "time=" << (endTime-startTime) << std::endl;
			for(std::map<std::string,int>::iterator iter = dataModelReportingData.begin();
				iter != dataModelReportingData.end(); ++iter){
					logFileStr << "DataModel[" << iter->first << "] = " << iter->second << std::endl;
			}
			logFile = UploadLogFile(GetBaseURL(),logFileStr.str());
		}

		{
			//Report to incremental statistics
			std::stringstream message;
			message << "double" << "\t" << "size" << "\t" << stream.tellp()/1000 << "\n";
			message << "double" << "\t" << "time" << "\t" << (endTime - startTime) / 30 << "\n";
			if(logFile != ""){
				message << "string" << "\t" << "save-log-file" << "\t" << logFile << "\n";
			}
			message << char(0);
			ReportStatisticPost(GetBaseURL(),"SaveLevel Error",message.str(),"assetId",assetId.c_str());
        }

        throw SerializationException(response);
	}
}

void DataModel::AsyncUploadPlaceResponseHandler(std::string* response, std::exception* exception, 
												boost::function<void()> resumeFunction,
												boost::function<void(std::string)> errorFunction)
{
	DataModel::LegacyLock lock(this, DataModelJob::Write);

	if( exception || !response)
	{
		errorFunction(exception ? exception->what() : "Error on upload");
		return;
	}
	else
	{
		resumeFunction();
		return;
	}
}

bool DataModel::uploadPlaceReturn(const bool succeeded, const std::string error, 
																 boost::function<void()> resumeFunction,
																 boost::function<void(std::string)> errorFunction)
{
	if(succeeded)
		resumeFunction();
	else
		errorFunction(error);

	return succeeded;
}

bool DataModel::uploadPlace(const std::string& uploadUrl, const SaveFilter saveFilter, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	shared_ptr<Reflection::Tuple> tuple(new Reflection::Tuple(2));

	if(uploadUrl.empty())
		return uploadPlaceReturn(false, "Url is empty", resumeFunction, errorFunction);

	try
	{

		if( !RBX::Http::isRobloxSite(uploadUrl.c_str()) )
			return uploadPlaceReturn(false, "Url is invalid", resumeFunction, errorFunction);

		// serialize the datamodel
		shared_ptr<std::stringstream> stream = serializeDataModel(saveFilter);

		// now post to the website
		Http uploadHttp(uploadUrl.c_str());

#ifndef _WIN32
		uploadHttp.setAuthDomain(GetBaseURL().c_str());
#endif

		if (stream)
		{
			if(stream->rdbuf()->in_avail() == 0)
				return uploadPlaceReturn(false, "No data to save", resumeFunction, errorFunction);

			std::string uploadString = stream->str();
			if(!uploadString.empty())
			{
				const bool shouldCompress = (uploadString.size() > MIN_HTTP_COMPRESSION_SIZE);
				if(!resumeFunction) 
				{
					std::string response;
					uploadHttp.post(*stream, Http::kContentTypeDefaultUnspecified, shouldCompress, response);
					return uploadPlaceReturn(true, response, resumeFunction, errorFunction);
				}
				else
				{
					uploadHttp.post(uploadString, Http::kContentTypeDefaultUnspecified, shouldCompress,
						boost::bind(&DataModel::AsyncUploadPlaceResponseHandler,this, _1, _2, resumeFunction, errorFunction));
					return false;
				}
			}
			else
				return uploadPlaceReturn(false, "No data was available to save", resumeFunction, errorFunction);
		}
		else
			return uploadPlaceReturn(false, "No data was streamed to save", resumeFunction, errorFunction);
	}
	catch (base_exception &e)
	{
		return uploadPlaceReturn(false, e.what(), resumeFunction, errorFunction);
	}
}

} // namespace RBX
