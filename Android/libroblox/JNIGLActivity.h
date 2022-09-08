#ifndef JNI_GLACTIVITY_H
#define JNI_GLACTIVITY_H

#include <stdint.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "RobloxInfo.h"
#include "FastLog.h"
#include "PlaceLauncher.h"
#include "JNIMain.h"
#include "JNIUtil.h"
#include "RobloxInput.h"
#include "RobloxView.h"
#include "util/MemoryStats.h"
#include "Util/SoundService.h"
#include "GfxBase/ViewBase.h"
#include "v8datamodel/MarketplaceService.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/AdService.h"
#include "network/Player.h"
#include "v8datamodel/FastLogSettings.h"

namespace RBX
{
namespace JNI
{
    void motionEventListening(std::string);
    void textBoxFocused(shared_ptr<RBX::Instance>);
    void textBoxFocusLost(shared_ptr<RBX::Instance>);

} // end JNI
} // end RBX

#endif