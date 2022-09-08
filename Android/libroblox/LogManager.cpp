#include "LogManager.h"
#include "RobloxUtilities.h"
#include "JNIMain.h"

#include <fstream>
#include <utility>

#include <client/linux/handler/exception_handler.h> // google breakpad

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "util/FileSystem.h"
#include "util/Guid.h"
#include "util/standardout.h"
#include "util/RobloxGoogleAnalytics.h"
#include "util/Http.h"
#include "util/Statistics.h"
#include "util/MemoryStats.h"
#include "v8datamodel/Stats.h"

#include "rbx/signal.h"

#include "FastLog.h"
#include "RbxAssert.h"

#include <android/log.h>

DYNAMIC_FASTINTVARIABLE(AndroidInfluxHundredthsPercentage, 0)


// Traditional Android macros.
#define LOG_TAG "roblox_jni"
#define LOG_INFO(...)  __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOG_WARNING(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define CHANNEL_OUTPUT 1

using namespace RBX;

namespace RBX
{
namespace JNI
{
    extern std::string exceptionReasonFilename;  // set in JNIRobloxSettings.cpp
    std::string platformUserAgent;               // set in JNIRobloxSettings.cpp
    std::string robloxVersion;                   // set in JNIRobloxSettings.cpp
    extern int lastPlaceId;                         // set in JNIGLActivity.cpp
}
}

namespace
{
static boost::shared_ptr<google_breakpad::ExceptionHandler> breakpadExceptionHandler;
static bool cleanupCrashDumps;

std::string getFastLogGuid()
{
    std::string logGuid;
    RBX::Guid::generateStandardGUID(logGuid);
    logGuid = logGuid.substr(1, 6);
    return logGuid;
}

    static void ResponseFunc(std::string* response, std::exception* exception) { }


    bool breakpadDumpCallback(
        const google_breakpad::MinidumpDescriptor& descriptor,
        void* context,
        bool succeeded)
{
    {
        char placeId[20]; // if we ever get more than a quintillion places uploaded, someone better update this
        sprintf(placeId, "%d", JNI::lastPlaceId);
        std::string crashReportUrl = JNI::getApiUrl() + "game/sessions/report/?placeId=" + placeId + "&eventType=AppStatusCrash";

        std::string response;
        Http http(crashReportUrl);
        http.post("", Http::kContentTypeUrlEncoded, true, &ResponseFunc, false);

        RBX::Time::Interval gamePlayInterval = RobloxUtilities::getRobloxUtilities().getGameTimer().reset();

        RBX::Analytics::InfluxDb::Points points;
        points.addPoint("SessionReport" , "AppStatusCrash");
        points.addPoint("FreeMemoryKB" , RBX::MemoryStats::freeMemoryBytes() * 1024);
        points.addPoint("UsedMemoryKB" , RBX::MemoryStats::usedMemoryBytes() * 1024);
        points.addPoint("PlayTime" , gamePlayInterval.seconds());

        points.report("Android-RobloxPlayer-SessionReport", DFInt::AndroidInfluxHundredthsPercentage);

        RBX::Analytics::EphemeralCounter::reportCounter("Android-ROBLOXPlayer-Crash", 1, true);
        RBX::Analytics::EphemeralCounter::reportCounter("ROBLOXPlayer-Crash", 1, true);
        boost::filesystem::path path = FileSystem::getCacheDirectory(false, NULL);
        path /= JNI::exceptionReasonFilename;
        std::ifstream fin(path.string().c_str());
        if (fin)
        {
            std::string reason;
            std::getline(fin, reason);
            RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "CrashReason", reason.c_str(), 0, true);
        }

        std::string label = JNI::platformUserAgent.empty() ? "Unknown" : JNI::platformUserAgent;
        RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "Crash", label.c_str(), 0, true);
    }

    if (!succeeded)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "Dump failed for %s", descriptor.path());
        return false;
    }

    StandardOut::singleton()->printf(MESSAGE_ERROR, "Dump written to %s", descriptor.path());

    std::ifstream fin(descriptor.path(), std::ios::in | std::ios::binary);
    if (!fin)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "Failed to open %s for reading.", descriptor.path());
        return false;
    }
    
    std::string robloxVersionWithComma = JNI::robloxVersion;
    boost::replace_all(robloxVersionWithComma, ".", ", ");
    

    std::string dmpFilename = "log_" + getFastLogGuid() + " 0, " + robloxVersionWithComma + ".Android.dmp";
    dmpFilename = RBX::Http::urlEncode(dmpFilename);
    
    std::string url = GetBaseURL() + "error/Dmp.ashx?filename=" + dmpFilename;

    StandardOut::singleton()->printf(MESSAGE_ERROR, "Sending dump to %s", url.c_str());

    std::string response;
    boost::filesystem::path path(descriptor.path());
    Http http(url);
    http.post(fin, Http::kContentTypeUrlEncoded, true, response, false);

    if (response.find("OK") == 0)
    {
        StandardOut::singleton()->printf(MESSAGE_INFO, "Crash dump upload successful.");
    }
    else
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "Crash dump website returned error.");
    }

    if (cleanupCrashDumps)
    {
        boost::filesystem::remove(descriptor.path());
    }

    return true;
}

static void standardOutCallback(const StandardOutMessage &msg)
{
    switch (msg.type)
    {
    case MESSAGE_OUTPUT:
    case MESSAGE_INFO:
        LOG_INFO("%s", msg.message.c_str());
        break;
    case MESSAGE_SENSITIVE:
    case MESSAGE_WARNING:
        LOG_WARNING("%s", msg.message.c_str());
        break;
    case MESSAGE_ERROR:
        LOG_ERROR("%s", msg.message.c_str());
        break;
    default:
        LOG_ERROR("Standard Message Out set with incorrect Message Type");
        break;
    }
}

static bool assertionHook(const char *expr, const char *filename, int lineNumber)
{
    StandardOut::singleton()->printf(MESSAGE_ERROR, "%s (%s:%d)", expr, filename, lineNumber);
    return true; // allow assertion handler to perform rawBreak.
}

class LogManager
{
    typedef boost::unordered_map<FLog::Channel, boost::shared_ptr<std::ostream> > Channels;

    boost::filesystem::path kLogPath;
    std::string kLogGuid;
    bool kInitialized;

    Channels channels;
    boost::mutex mutex;

    rbx::signals::scoped_connection messageOutConnection;
public:
    LogManager() : kInitialized(false)
    {
        messageOutConnection = StandardOut::singleton()->messageOut.connect(&standardOutCallback);
        StandardOut::singleton()->printf(MESSAGE_INFO, "StandardOut ready.");

        setAssertionHook(&assertionHook);
    }

    ~LogManager()
    {
        boost::mutex::scoped_lock lock(mutex);
        channels.clear(); // close all files and forget the file handles

        messageOutConnection.disconnect();
        LOG_INFO("StandardOut disconnected.");
    }

    void init(
            const boost::filesystem::path &logPath,
            const std::string &guid)
    {
        boost::mutex::scoped_lock lock(mutex);
        kLogPath = logPath;
        kLogGuid = guid;
        kInitialized = true;
    }

    void writeEntry(const FLog::Channel &channelId, const char *message)
    {
        RBXASSERT(kInitialized);

        boost::mutex::scoped_lock lock(mutex);

        const Channels::iterator it = channels.find(channelId);
        typename Channels::mapped_type stream;

        if (channels.end() == it)
        {
            const int kLogChannel = channelId;

            std::stringstream fileNameSS;
            fileNameSS << "log_" << kLogGuid << "_" << kLogChannel << ".txt";
            boost::filesystem::path path = kLogPath / fileNameSS.str();
            StandardOut::singleton()->printf(MESSAGE_INFO, "Opening log file at %s", path.c_str());
            typename Channels::value_type pair = std::make_pair(channelId, boost::shared_ptr<std::ostream>(new std::ofstream(path.c_str())));
            if (!pair.second)
            {
                std::stringstream ss;
                ss << "Could not open file: " << path.c_str();
                throw RBX::runtime_error(ss.str().c_str());
            }
            else
            {
                channels.insert(pair);
                stream = pair.second;
            }
        }
        else
        {
            stream = it->second;
        }

        *stream << message << '\n';
    }
}; // LogManager

static LogManager channels;

static void fastLogMessageCallback(FLog::Channel channelId, const char* message)
{
    switch (channelId)
    {
    case CHANNEL_OUTPUT:
        StandardOut::singleton()->printf(MESSAGE_INFO, "%s", message);
        break;
    default:
        channels.writeEntry(channelId, message);
        break;
    }
}
} // namespace

namespace RBX
{
namespace JNI
{
namespace LogManager
{
void setupFastLog()
{
    std::string logGuid = getFastLogGuid();
    StandardOut::singleton()->printf(MESSAGE_INFO, "LogManager::kLogGuid = %s", logGuid.c_str());

    std::string logPath = FileSystem::getCacheDirectory(true, "Log").string();
    StandardOut::singleton()->printf(MESSAGE_INFO, "LogManager::kLogPath = %s", logPath.c_str());

    channels.init(logPath, logGuid);

    FLog::SetExternalLogFunc(&fastLogMessageCallback);
    StandardOut::singleton()->printf(MESSAGE_INFO, "FastLog system ready.");
}

void tearDownFastLog()
{
    FLog::SetExternalLogFunc(NULL);
    StandardOut::singleton()->printf(MESSAGE_INFO, "FastLog system offline.");
}

void setupBreakpad(bool cleanup)
{
    StandardOut::singleton()->printf(MESSAGE_INFO, "Initializing breakpad.");
    cleanupCrashDumps = cleanup;
    google_breakpad::MinidumpDescriptor descriptor(FileSystem::getCacheDirectory(true, "breakpad").string());
    breakpadExceptionHandler.reset(new google_breakpad::ExceptionHandler(
            descriptor,
            NULL,
            &breakpadDumpCallback,
            NULL,
            true,
            -1));
}
} // namespace LogManager
} // namespace JNI
} // namespace RBX
