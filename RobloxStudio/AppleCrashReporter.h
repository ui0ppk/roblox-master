//
//  AppleCrashReporter.h
//  RobloxStudio
//

#pragma once

class AppleCrashReporter
{
	
public:
	static void check();
	
	
private:
	static void getReports(QStringList& fileList);
	static void submitFile(QString fileName);
	static void uploadAndDeletFileAsync(std::string url, std::string file);
	
private:
	static const int maxReport;
	
};