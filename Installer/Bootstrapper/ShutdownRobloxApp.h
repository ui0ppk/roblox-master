#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>

class ShutdownRobloxApp : public boost::enable_shared_from_this<ShutdownRobloxApp>
{
	int timeoutInSeconds;
	typedef boost::function<bool(int, int)> Callback;
	Callback callback;
	HINSTANCE hInstance;
	boost::function<HWND()> parent;
	bool result;
	std::wstring appExeName;
public:
	ShutdownRobloxApp(HINSTANCE hInstance, std::wstring name, boost::function<HWND()> parent, int timeoutInSeconds, Callback callback):hInstance(hInstance), appExeName(name), parent(parent),timeoutInSeconds(timeoutInSeconds),callback(callback) {}
	~ShutdownRobloxApp(void);
	bool run();
private:
	void terminateApp(DWORD pid);
};
