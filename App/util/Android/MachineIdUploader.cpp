#include "Util/MachineIdUploader.h"

#include <jni.h>

using namespace RBX;

bool MachineIdUploader::fillMachineId(MachineId *out) {
    // Fetch the device unique id via JNI.
//    out->macAddresses.push_back("device_id");
//    return true;

    // some incomplete, possible incorrect example code from the web
//    JNIEnv &env = ...
//    jobject activity = ... (android.app.Activity object)
//                               //TelephonyManager telephony_manager =
// getSystemService(Context.TELEPHONY_SERVICE);
//    jmethodID mid = env.GetMethodID(env.GetObjectClass(activity),
// "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
//    jobject telephony_manager = env.CallObjectMethod(activity, mid,
// env.NewStringUTF("phone"));
//                               //String s_imei = tm.getDeviceId();
//    mid = env.GetMethodID(env.GetObjectClass(telephony_manager),
// "getDeviceId", "()Ljava/lang/String;");
//    jstring s_imei = (jstring)env.CallObjectMethod(telephony_manager,
// mid);
//    if(env.ExceptionCheck()){
//       env.ExceptionClear();
//       return false;
//    }
//    const jchar *imei = env.GetStringCritical(s_imei, NULL);
// // copy imei string anywhere you need
//    env.ReleaseStringCritical(s_imei, imei);
    return false;
}
