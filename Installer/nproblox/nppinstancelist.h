#ifndef _NPP_INSTANCE_LIST_H_
#define _NPP_INSTANCE_LIST_H_
#include <list> 
#include <windows.h> 
#include <fstream>

typedef struct {
	NPP         instance;
	NPObject  * so;
	ILauncher * IRobloxLauncher;
} NP_ROBLOX_INSTANCE;

using namespace std ;
typedef list<NP_ROBLOX_INSTANCE *> NppRobloxInstance;

class NPInstanceList
{
private:
	std::string getTempFileName(const char* ext)
	{
		TCHAR szPath[_MAX_DIR]; 
		if (!GetTempPath(_MAX_DIR, szPath))
		{
			dontLog = true;
			return std::string("");

		}

		GUID g;
		::CoCreateGuid(&g);
		char szFilePath[MAX_PATH];
		sprintf(szFilePath, "%SRBX-%08X.%s", szPath, g.Data1, ext);

		return std::string(szFilePath);
	}

	std::string logFileName;
	std::ofstream log;
	// true only if we were not able to get temp path
	bool dontLog;

	NppRobloxInstance queue; 
	NppRobloxInstance::iterator i;
	NppRobloxInstance::iterator index;
	int code;
	
	HANDLE lockMutex;
	CRITICAL_SECTION logLock;

	void lock(void){
		WaitForSingleObject(lockMutex, INFINITE);
	}

	void unlock(void){
		ReleaseMutex(lockMutex);
	}

	void writeLogEntryRaw(char *entry);
public:
	NPInstanceList(void) :
		dontLog(false),
		logFileName(getTempFileName("log")),
		log(logFileName.c_str())
	{
		lockMutex = CreateMutex(NULL, false, NULL);
		InitializeCriticalSection(&logLock);
	}

	~NPInstanceList(void){
		if( lockMutex ) {
			CloseHandle(lockMutex);
		}
		DeleteCriticalSection(&logLock);
	}

	void writeLogEntry(char *format, ...);
	
	void clear(void) {
		lock();
		if( queue.size() >0 ) {
			queue.clear();
		}
		unlock();
	}

	void add(NP_ROBLOX_INSTANCE *item) {
		lock();
		queue.push_back(item);
		unlock();
	}
 
	ILauncher * find(NPObject *obj) {
		ILauncher *item = NULL;
		NP_ROBLOX_INSTANCE  * tmp = NULL;
		lock();
		if( queue.size()>0 ){
			for (i =  queue.begin(); i != queue.end(); ++i){
				tmp = (NP_ROBLOX_INSTANCE *)(*i);
				if( tmp->so == obj ){
					item = tmp->IRobloxLauncher;
					break;
				}
			}
		}
		unlock();
		return item;
	}

	NP_ROBLOX_INSTANCE * find(NPP  activeInstance){
		NP_ROBLOX_INSTANCE *item = NULL, * tmp = NULL;
		lock();
		if( queue.size()>0 ){
			for (i =  queue.begin(); i != queue.end(); ++i){
				tmp = (NP_ROBLOX_INSTANCE *)(*i);
				if( tmp->instance == activeInstance ){
					item = tmp;
					break;
				}
			}
		}

		unlock();
		return item;
	}

	NP_ROBLOX_INSTANCE * findAndRemove(NPP  activeInstance){
		NP_ROBLOX_INSTANCE *item = NULL, * tmp = NULL;
		lock();
		if( queue.size()>0 ){
			for (i =  queue.begin(); i != queue.end(); ++i){
				tmp = (NP_ROBLOX_INSTANCE *)(*i);
				if( tmp->instance == activeInstance ){
					queue.erase(i);
					item = tmp;
					break;
				}
			}
		}
		unlock();
		return item;
	}

	int size(void){
		int count;
		lock();
		count = (int)queue.size();
		unlock();
		return count;
	}
};





#endif

