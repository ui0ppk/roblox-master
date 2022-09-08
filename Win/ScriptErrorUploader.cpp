/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#include "stdafx.h"

#undef min
#undef max

#include "ScriptErrorUploader.h"
#include "LogManager.h"

#include "util/http.h"
#include "util/StandardOut.h"

namespace io = boost::iostreams;

void ScriptErrorUploader::Upload(std::string url)
{
    HANDLE hMutex; 

    hMutex = CreateMutex( 
        NULL,                        // default security descriptor
        TRUE,                       // own the mutex
        TEXT("RobloxCrashScriptErrorUploaderMutex"));  // object name

    if (hMutex == NULL) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "CreateMutex error: %d\n", GetLastError() );
	}
    else 
	{
        if ( GetLastError() == ERROR_ALREADY_EXISTS ) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "RobloxCrashScriptErrorUploaderMutex already exists. Not uploading logs.");
			return;
		}
	}

	std::vector<std::string> f = MainLogManager::getMainLogManager()->gatherScriptCrashLogs();

	{
		boost::recursive_mutex::scoped_lock lock(_data->sync);
		_data->url = url;
		_data->hInterprocessMutex = hMutex;
		for (size_t i = 0; i<f.size(); ++i)
			_data->files.push(f[i]);
	}
	if (thread)
	{
		thread->wake();
	}
	else
	{
		while (run(_data)!=RBX::worker_thread::done) 
		{}
	}
}

RBX::worker_thread::work_result ScriptErrorUploader::run(shared_ptr<data> _cseData)
{
	std::string file;
	{
		boost::recursive_mutex::scoped_lock lock(_cseData->sync);
		if (_cseData->files.empty())
		{
			if(_cseData->hInterprocessMutex)
			{
				CloseHandle(_cseData->hInterprocessMutex);
				_cseData->hInterprocessMutex = NULL;
			}
			return RBX::worker_thread::done;
		}
		file = _cseData->files.front();
	}

	try
	{
		printf("Uploading %s\n", file.c_str());
		bool isCseFile = file.substr(file.size()-4)==".cse";
		if (isCseFile)
			_cseData->dmpFileCount++;

		// TODO: put the filename in the header rather than the query string??
		std::string url = _cseData->url;
		url += "?filename=";
		url += file;

		if (isCseFile && _cseData->dmpFileCount>3)
		{
			std::stringstream data("Too many cse files");
			std::string response;
			RBX::Http(url).post(data, RBX::Http::kContentTypeDefaultUnspecified, true, response);
		}
		else
		{
			std::fstream data(file.c_str(), std::ios_base::in | std::ios_base::binary);
			size_t begin = data.tellg();
			data.seekg (0, std::ios::end);
			size_t end = data.tellg();
			if (end > begin)
			{
				data.seekg (0, std::ios::beg);
				std::string response;
				RBX::Http(url).post(data, RBX::Http::kContentTypeDefaultUnspecified, true, response);
			}
			else
			{
				// Some cse files are empty. Post it anyway so that we can report it
				std::stringstream data("Empty!!!");
				std::string response;
				RBX::Http(url).post(data, RBX::Http::kContentTypeDefaultUnspecified, true, response);
			}
		}

		ErrorUploader::MoveRelative(file.c_str(), "archive\\");
	}
	catch (std::exception& e)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
	}

	{
		boost::recursive_mutex::scoped_lock lock(_cseData->sync);
		_cseData->files.pop();
	}
	return RBX::worker_thread::more;
}