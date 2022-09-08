
#ifndef JNI_MAIN_H
#define JNI_MAIN_H

#include <jni.h>
#include <string>

namespace RBX
{
namespace JNI
{

    void sendAppEvent( void* pClosure );
    void postAppEvent( void* pClosure );
    void processAppEvents( void );

    jclass getCallbackClass();
    jmethodID getShowKeyboardJMethod();
    jmethodID getHideKeyboardJMethod();
    jmethodID getPromptNativePurchaseJMethod();
    jmethodID getShowVideoAd_AdColonyJMethod();
    jmethodID getShowVideoAd_GoogleJMethod();
    jmethodID getMotionEventListeningJMethod();

    // TODO: Have this actually hooked by the engine.
    void handleBackPressed( void );
    void exitGame( void );
    void exitGameWithError( std::string errId );

    std::string getApiUrl();

} // JNI
} // RBX


#endif // JNI_MAIN_H
