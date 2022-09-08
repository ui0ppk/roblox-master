#include "JNIMain.h"

#include "LogManager.h"
#include "FunctionMarshaller.h"
#include "RobloxInfo.h"
#include "JNIUtil.h"

#include "util/Guid.h"
#include "util/FileSystem.h"
#include "util/standardout.h"
#include "util/RobloxGoogleAnalytics.h"

#include <exception>

#include <cstddef>
#include <curl/curl.h>
#include <pthread.h>

using namespace RBX;

FASTFLAGVARIABLE(JNIEnvScopeOptimization, false)

namespace RBX
{
namespace JNI
{
std::string exceptionReasonFilename; // set in JNIRobloxSettings.cpp

namespace // anonymous namespace
{
static JavaVM *jvm = NULL;
static jmethodID sendAppEventJMethod = 0;
static jmethodID postAppEventJMethod = 0;
static jmethodID showKeyboardJMethod = 0;
static jmethodID hideKeyboardJMethod = 0;
static jmethodID promptNativePurchaseJMethod = 0;
static jmethodID exitGameCallbackJMethod = 0;
static jmethodID exitGameWithErrorCallbackJMethod = 0;
static jmethodID showVideoAd_AdColonyJMethod = 0;
static jmethodID showVideoAd_GoogleJMethod = 0;
static jmethodID motionEventListeningJMethod = 0;
    static jmethodID getApiUrlJMethod = 0;

static jclass callbackClass = 0;

static pthread_mutex_t messagesLock;
static std::deque<RBX::FunctionMarshaller::Closure*> messages;

static std::terminate_handler last_terminate_handler = NULL;

static void writeTerminateLog(const char* reason)
{
    boost::filesystem::path path = FileSystem::getCacheDirectory(true, NULL);
    path /= exceptionReasonFilename;
    std::ofstream fout(path.c_str());
    fout << reason;
    fout.flush();
}

static void terminateHandler()
{
    try
    {
        throw;
    }
    catch (const std::exception& e)
    {
        writeTerminateLog(e.what());
        StandardOut::singleton()->printf(MESSAGE_ERROR, "Exception thrown: %s", e.what());
        RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "Uncaught Exception", e.what(), 0, true);
    }
    catch (...)
    {
        writeTerminateLog("Unknown exception");
        StandardOut::singleton()->printf(MESSAGE_ERROR, "Unknown exception");
        RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "Uncaught Exception", "Unknown exception", 0, true);
    }
    abort();
}

static pthread_key_t getEnvKey;
static pthread_once_t getEnvOnce = PTHREAD_ONCE_INIT;

static void getEnvDtor(void* p)
{
	if (JNIEnv* env = static_cast<JNIEnv*>(p))
	{
		JavaVM *jvm = 0;
		env->GetJavaVM(&jvm);

		jint ret = jvm->DetachCurrentThread();
		RBXASSERT(ret == JNI_OK);
	}
}

static void getEnvKeyCreate()
{
	pthread_key_create(&getEnvKey, getEnvDtor);
}

static JNIEnv* getEnvForCurrentThread()
{
	pthread_once(&getEnvOnce, getEnvKeyCreate);

	if (void* p = pthread_getspecific(getEnvKey))
		return static_cast<JNIEnv*>(p);

	JNIEnv* env = NULL;
	jint ret = jvm->AttachCurrentThread(&env, NULL);
	RBXASSERT(ret == JNI_OK);

	// Register for detach when thread exits
	pthread_setspecific(getEnvKey, env);

	return env;
}

class JNIEnvScope
{
public:
	JNIEnv* env;
	bool wasAttached;

	JNIEnvScope( void )
	{
		env = NULL;
		wasAttached = false;

		if (FFlag::JNIEnvScopeOptimization)
		{
			env = getEnvForCurrentThread();
			return;
		}

	    jint ret = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
	    switch(ret)
	    {
	    	// UI thread.
	        case JNI_OK:
	            break;

	    	// All other threads.
	        case JNI_EDETACHED:
	        {
                // Make up a name to quell:  W/art ... attached without supplying a name.

                char buf[20];
                sprintf(buf,"%x",gettid());
                JavaVMAttachArgs args = { JNI_VERSION_1_6, buf, NULL };
                
	            jint isOk = jvm->AttachCurrentThread(&env, &args) == JNI_OK;
	            RBXASSERT(isOk);
            	wasAttached = true;
	            break;
	        }

	        default:
	            RBXASSERT(0);
	            break;
	    }
	}
	~JNIEnvScope( void )
	{
		if( wasAttached )
		{
			jvm->DetachCurrentThread();
		}
	}
};

jint onLoad(JavaVM *vm, void *reserved)
{
    last_terminate_handler = std::set_terminate(&terminateHandler);

    JNI::jvm = vm;
    JNIEnvScope scope;

    jclass c = scope.env->FindClass("com/roblox/client/ActivityGlView");
    JNI::callbackClass = (jclass)scope.env->NewGlobalRef(c);
    JNI::sendAppEventJMethod = scope.env->GetStaticMethodID( c, "sendAppEvent", "(Z)V");
    JNI::postAppEventJMethod = scope.env->GetStaticMethodID( c, "postAppEvent", "()V");
    JNI::showKeyboardJMethod = scope.env->GetStaticMethodID( c, "showKeyboard", "(JLjava/lang/String;)V");
    JNI::hideKeyboardJMethod = scope.env->GetStaticMethodID( c, "hideKeyboard", "()V");
    JNI::showVideoAd_AdColonyJMethod = scope.env->GetStaticMethodID( c, "showAdColonyAd", "()V");
    JNI::showVideoAd_GoogleJMethod = scope.env->GetStaticMethodID( c, "showGoogleAd", "(Ljava/lang/String;)V");
    JNI::promptNativePurchaseJMethod = scope.env->GetStaticMethodID( c, "promptNativePurchase", "(JLjava/lang/String;Ljava/lang/String;)V");
    JNI::exitGameCallbackJMethod = scope.env->GetStaticMethodID( c, "exitGame", "()V");
    JNI::exitGameWithErrorCallbackJMethod = scope.env->GetStaticMethodID( c, "exitGameWithError", "(Ljava/lang/String;)V");
    JNI::motionEventListeningJMethod = scope.env->GetStaticMethodID( c, "listenToMotionEvents", "(Ljava/lang/String;)V");
    JNI::getApiUrlJMethod = scope.env->GetStaticMethodID( c, "getApiUrl", "()Ljava/lang/String;");

    int pthreadError = ::pthread_mutex_init(&JNI::messagesLock, NULL);
    RBXASSERT( pthreadError==0 );

    // libcurl must be initialized while no other threads are running
    curl_global_init(CURL_GLOBAL_DEFAULT | CURL_GLOBAL_ACK_EINTR);

    return JNI_VERSION_1_6;
}

void onUnload(JavaVM *vm, void *reserved)
{
    JNIEnvScope scope;
    scope.env->DeleteGlobalRef( callbackClass );

    int pthreadError = ::pthread_mutex_destroy( &JNI::messagesLock );
    RBXASSERT( pthreadError==0 );

    JNI::jvm = NULL;

	// libcurl must be cleaned up while no other threads are running
    curl_global_cleanup();

    StandardOut::singleton()->printf(MESSAGE_INFO, "Tearing down FastLog.");
    JNI::LogManager::tearDownFastLog(); // this is setup in JNIRobloxSettings.cpp

    std::set_terminate(last_terminate_handler);
}

void pushMessage( RBX::FunctionMarshaller::Closure* pClosure )
{
	int pthreadError = 0;
	pthreadError |= pthread_mutex_lock(&messagesLock);

	messages.push_back( pClosure );

	pthreadError |= pthread_mutex_unlock(&messagesLock);
    RBXASSERT( pthreadError==0 );
}

RBX::FunctionMarshaller::Closure* popMessage( void )
{
	RBX::FunctionMarshaller::Closure* p = NULL;

	int pthreadError = 0;
	pthreadError |= pthread_mutex_lock(&messagesLock);

	if( !messages.empty() )
	{
		p = messages.front();
		messages.pop_front();
	}
	else
	{
		p = NULL;
	}

	pthreadError |= pthread_mutex_unlock(&messagesLock);
    RBXASSERT( pthreadError==0 );

    return p;
}

void callMessage( RBX::FunctionMarshaller::Closure* closure )
{
    RBX::CEvent *pWaitEvent = closure->waitEvent;
    try
    {
        boost::function<void()>* pF = closure->f;
        (*pF)();

        delete pF;
        delete closure;
    }
    catch (RBX::base_exception& e)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, e.what());
        closure->errorMessage = e.what();
    }

    // If a task is waiting on an event, set it
    if (pWaitEvent)
    {
        pWaitEvent->Set();
    }
}

}  // anonymous namespace

// Send callback to the main thread, possibly waiting for it.
void sendAppEvent( void* pClosure )
{
    RBX::FunctionMarshaller::Closure* closure = (RBX::FunctionMarshaller::Closure*)pClosure;
    RBX::CEvent* waitEvent = closure->waitEvent; // closure gets deleted.
	pushMessage(closure);

    // NOTE:  A null wait event means the caller must wait instead.  This is done in Java.
    jboolean waitFlag = (waitEvent == NULL);

    JNIEnvScope scope;
    scope.env->CallStaticVoidMethod(callbackClass, sendAppEventJMethod, waitFlag);

    if (scope.env->ExceptionCheck()) {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "sendAppEvent exception");
        scope.env->ExceptionDescribe();
        scope.env->ExceptionClear();
    }

    if(waitEvent != NULL)
    {

    	waitEvent->Wait();
    }
}

// Send callback to the main thread, not waiting for it.
void postAppEvent( void* pClosure )
{
    RBX::FunctionMarshaller::Closure* closure = (RBX::FunctionMarshaller::Closure*)pClosure;
	pushMessage(closure);

	JNIEnvScope scope;
    scope.env->CallStaticVoidMethod(callbackClass, postAppEventJMethod);

    if (scope.env->ExceptionCheck()) {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "postAppEvent exception!");
        scope.env->ExceptionDescribe();
        scope.env->ExceptionClear();
    }
}

// Wait for all main thread callbacks to complete.
void processAppEvents( void )
{
	while (RBX::FunctionMarshaller::Closure* closure = popMessage())
	{
		callMessage(closure);
	}
}

jclass getCallbackClass()
{
	return JNI::callbackClass;
}

jmethodID getMotionEventListeningJMethod()
{
	return JNI::motionEventListeningJMethod;
}

jmethodID getShowKeyboardJMethod()
{
	return JNI::showKeyboardJMethod;
}

jmethodID getHideKeyboardJMethod()
{
    return JNI::hideKeyboardJMethod;
}

jmethodID getPromptNativePurchaseJMethod()
{
	return JNI::promptNativePurchaseJMethod;
}

jmethodID getShowVideoAd_AdColonyJMethod()
{
    return JNI::showVideoAd_AdColonyJMethod;
}

jmethodID getShowVideoAd_GoogleJMethod()
{
    return JNI::showVideoAd_GoogleJMethod;
}

void exitGameWithError(std::string errId)
{
    JNI::JNIEnvScope scope;
    jstring errorjs = scope.env->NewStringUTF(errId.c_str());
    scope.env->CallStaticVoidMethod(callbackClass, exitGameWithErrorCallbackJMethod , errorjs);
}

void exitGame( void )
{
	JNI::JNIEnvScope scope;
    scope.env->CallStaticVoidMethod(callbackClass, exitGameCallbackJMethod);
}

void handleBackPressed( void )
{
	exitGame();
}

    std::string getApiUrl() {
        JNIEnvScope scope;
        jstring url = (jstring)scope.env->CallStaticObjectMethod(callbackClass, getApiUrlJMethod);

        if (scope.env->ExceptionCheck()) {
            jclass throwable_class = scope.env->FindClass("java/lang/Throwable");
            jmethodID mid_throwable_toString = scope.env->GetMethodID(throwable_class, "toString", "()Ljava/lang/String;");
            jstring msg_obj = (jstring) scope.env->CallObjectMethod(scope.env->ExceptionOccurred(), mid_throwable_toString);

            StandardOut::singleton()->printf(MESSAGE_INFO, "getApiUrl exception: %s", jstringToStdString(scope.env, msg_obj).c_str());
            scope.env->ExceptionClear();
            return "";
        } else {
            return jstringToStdString(scope.env, url);
        }
    }

} // JNI
} // RBX

extern "C" {

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	return JNI::onLoad( vm, reserved );
}

void JNI_OnUnload(JavaVM *vm, void *reserved)
{
	JNI::onUnload( vm, reserved );
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeCallMessagesFromMainThread(JNIEnv* jenv, jclass obj )
{
	JNI::processAppEvents();
}

} // extern "C"

