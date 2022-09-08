#pragma once

#include "ErrorUploader.h"

class ScriptErrorUploader : public ErrorUploader
{
public:
	ScriptErrorUploader(bool backgroundUpload)
	{
		if (backgroundUpload)
			thread.reset(new RBX::worker_thread(boost::bind(&ScriptErrorUploader::run, _data), "ScriptErrorUploader"));
	}
	/*override*/ void Upload(std::string url);
private:
	static RBX::worker_thread::work_result run(shared_ptr<data> _cseData);

};

