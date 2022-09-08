//
//  AppleCrashReporter.cpp
//  RobloxStudio
//

// standard C/C++ Headers
#include <fstream>

// 3rd Party Headers
#include "boost/filesystem.hpp"

// Roblox Headers
#include "util/standardout.h"
#include "util/Http.h"
#include "RobloxServicesTools.h"

// Qt Headers
#include <QApplication>
#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QStringBuilder>

// Roblox Studio Headers
#include "AppleCrashReporter.h"
#include "RobloxSettings.h"

const int AppleCrashReporter::maxReport = 1;

void AppleCrashReporter::check()
{
	QString plistPath = QApplication::applicationDirPath() % "/../info.plist";
	QSettings settings(plistPath, QSettings::NativeFormat);
						
	if (settings.value("ENABLE_APPLE_CRASH_REPORTER").toString() == "YES") 
	{
		QStringList fileList;
		
		getReports(fileList);
		
		int max = qMin(maxReport,fileList.count());
		if (max == 0) max = fileList.count();

		for (int i = 0; i < max; i++)
			submitFile(QDir::homePath() % "/Library/Logs/DiagnosticReports/" % fileList.at(i));
		
	}

}

void AppleCrashReporter::getReports(QStringList& fileList)
{
	QStringList filters;
	filters << "RobloxStudio_*.crash";
	QDir crashReportDir(QDir::homePath() % "/Library/Logs/DiagnosticReports/");
	fileList = crashReportDir.entryList(filters);
}

	
void AppleCrashReporter::submitFile(QString fileName)
{
	QFile file(fileName);
	
	if(file.exists())
		uploadAndDeletFileAsync(GetDmpUrl(RobloxSettings::getBaseURL().toStdString(), true), fileName.toStdString());
}

static void handler(std::exception* ex, std::string file)
{
	if (ex)
		return;
	
	std::remove(file.c_str());
	
	// Remove the link as well in CrashReporter Dir
	file.replace(file.find("DiagnosticReports"), 17, "CrashReporter");
	std::remove(file.c_str());
}


void AppleCrashReporter::uploadAndDeletFileAsync(std::string url, std::string file)
{
	boost::shared_ptr<std::fstream> data(new std::fstream(file.c_str(), std::ios_base::in | std::ios_base::binary));
	size_t begin = data->tellg();
	data->seekg (0, std::ios::end);
	size_t end = data->tellg();
	if (end > begin)
	{
		data->seekg (0, std::ios::beg);
		
		std::string version;
		while (*data)
		{
			char buff[255];
			data->getline(buff, 255);
			std::string line = buff;
			
			if (line.substr(0, 8) == "Version:")
			{
				// "Version:         0.34.0.107 (107)"
				for (size_t i = 8; i < line.size(); ++i)
					if (line[i] != ' ')
					{
						line = line.substr(i);
						break;
					}
				
				// "0.34.0.107 (107)"
				for (size_t i = 0; i < line.size(); ++i)
					if (line[i] == ' ')
					{
						line = line.substr(0, i);
						// "0.34.0.107"
						while (true)
						{
							size_t j = line.find('.');
							if (j == std::string::npos)
								break;
							line = line.replace(j, 1, ",%20");
						}
						// "0,%2034,%200,%20107"
						version = line;
						break;
					}
			}
		}
		

		
		boost::filesystem::path p(file);
		std::string filename = "RobloxStudio_log_";
		// extract file "guid"
		filename += p.filename().string().substr(13, 17);
		filename += "%20";
		filename += version;
		filename += ".crash";
		
		std::string fullUrl = url;
		fullUrl += "?filename=" + filename;
		
		boost::shared_ptr<std::fstream> dataPost(new std::fstream(file.c_str(), std::ios_base::in));
		dataPost->seekg (0, std::ios::beg);

		
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, "Uploading %s", fullUrl.c_str());	
		RBX::Http(fullUrl).post(dataPost, RBX::Http::kContentTypeUrlEncoded, true, boost::bind(handler, _2, p.string()));
	}
}

