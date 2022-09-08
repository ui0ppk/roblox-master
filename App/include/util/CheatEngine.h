#pragma once

#include <ctime>
#include "rbx/TaskScheduler.Job.h"
#include <util/HeapValue.h>

namespace RBX
{
	bool vmProtectedDetectCheatEngineIcon();

    class HwndScanner
    {
        struct fullWindowInfo {
            DWORD winWidth;
            DWORD winHeight;
            DWORD pid;
            DWORD active;
            std::string title;
            bool kickEarly;
            static size_t compareByPid ( fullWindowInfo lhs, fullWindowInfo rhs);
        };
        std::vector<fullWindowInfo> hwndScanResults;
        static BOOL CALLBACK makeHwndVector(HWND hwnd, LPARAM lParam);

    public:
        HwndScanner();

        int scan();

        bool detectTitle() const;

        // This method will always return false on Windows8.
        bool detectFakeAttach() const;
        bool detectEarlyKick() const;

    };

    class FileScanner
    {
        std::time_t baseTime;
        std::string tempFolder;
    public:
        FileScanner();

        bool detectLogUpdate() const;
    };

    extern bool ceDetected;
    extern bool ceHwndChecks;
    static const unsigned int kCeStructKey = 0x23A7F;
    static const unsigned char kCeCharKey = 0x55;

    HANDLE setupCeLogWatcher();

    // A job to profile this detection method and optionally enable it.
    class VerifyConnectionJob : public RBX::TaskScheduler::Job
    {
    public:
        VerifyConnectionJob();
        /*override*/ RBX::Time::Interval sleepTime(const Stats& stats);
        /*override*/ Job::Error error(const Stats& stats);
        /*override*/ TaskScheduler::StepResult step(const Stats& stats);
        /*override*/ double getPriorityFactor();	

    };

    bool isSandboxie();
    bool isCeBadDll();

    // This class does dbvm detection and also breaks on certain dll injection methods.
    class DbvmCanary
    {
    private:
        HANDLE canaryCage;
        HANDLE canaryHandle;
        CONTEXT ctx;
        size_t hashValue;
        static void canary(HANDLE* mutex);
        inline size_t hashDbgRegs(CONTEXT& ctx)
        {
            return (ctx.Dr0*54321) + (ctx.Dr1*98765) ^ (ctx.Dr2*6543) - (ctx.Dr3*987);
        }
    public:
        DbvmCanary();
        void checkAndLocalUpdate();
        void kernelUpdate();
    };

    class SpeedhackDetect
    {
    private:
        DWORD k32base;
        DWORD k32size;
    public:
        SpeedhackDetect();
        bool isSpeedhack();
    };

    extern HeapValue<uintptr_t> vehHookLocationHv;
    extern HeapValue<uintptr_t> vehStubLocationHv;
    extern void* vehHookContinue;

    void addWriteBreakpoint(uintptr_t addr);
    void removeWriteBreakpoint(uintptr_t addr);

    __declspec(align(4096)) extern int writecopyTrap[4096];

} // namespace RBX