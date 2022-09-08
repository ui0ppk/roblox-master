#include "JNIGLActivity.h"



LOGVARIABLE(Android, 6)
FASTINTVARIABLE(AdColonyPercentage, 100)
FASTSTRINGVARIABLE(GoogleVideoAdUrl, "")

using namespace RBX;
using namespace RBX::JNI;

JavaVM* gJvm = NULL;
static ANativeWindow* aNativeWindow = NULL;

namespace RBX {
namespace SystemUtil { extern std::string mOSVersion; extern std::string mDeviceName; } // from Android/SystemUtil.cpp
namespace JNI {

	int lastPlaceId;

void motionEventListening(std::string type) {
	JNIEnv *env = NULL;
	if (gJvm->AttachCurrentThread(&env, NULL) == JNI_OK) {
		jclass callbackClass = JNI::getCallbackClass();
		if (!callbackClass) {
			return;
		}

		jmethodID motionEventListeningJMethod = JNI::getMotionEventListeningJMethod();
		if (!motionEventListeningJMethod) {
			return;
		}

		jstring typeString = env->NewStringUTF(type.c_str());

		env->CallStaticVoidMethod(callbackClass, motionEventListeningJMethod, typeString);

		if (env->ExceptionOccurred()) {
			env->ExceptionDescribe();
			return;
		}
	}
}

void textBoxFocused(shared_ptr<RBX::Instance> textbox) {
	JNIEnv *env = NULL;

	if (gJvm->AttachCurrentThread(&env, NULL) == JNI_OK) {
		jclass callbackClass = getCallbackClass();
		if (!callbackClass) {
			return;
		}

		jmethodID showKeyboardJMethod = getShowKeyboardJMethod();
		if (!showKeyboardJMethod) {
			return;
		}

		jstring textBoxString = NULL;
		RBX::TextBox* textBox = RBX::Instance::fastDynamicCast<RBX::TextBox>(
				textbox.get());
		if (textBox) {
			textBoxString = env->NewStringUTF(
					textBox->getBufferedText().c_str());
		}

		env->CallStaticVoidMethod(callbackClass, showKeyboardJMethod,
				(jlong) (intptr_t) textBox, textBoxString);

		if (env->ExceptionOccurred()) {
			env->ExceptionDescribe();
			return;
		}
	}
}

// Called by the engine's textBoxReleaseFocus event (connected in setupDatamodelCallbacks)
void textBoxFocusLost(shared_ptr<RBX::Instance> textBox) {
    JNIEnv *env = NULL;

    if (gJvm->AttachCurrentThread(&env, NULL) == JNI_OK) {
        jclass callbackClass = getCallbackClass();

        // Get the pointer to the hideKeyboard method in ActivityGlView
        jmethodID hideKeyboardJMethod = getHideKeyboardJMethod();
        if (!hideKeyboardJMethod)
        {
            RBX::StandardOut::singleton()->printf(MESSAGE_INFO, "JNI ERROR: Could not find hideKeyboard method.");
            return;
        }

        env->CallStaticVoidMethod(callbackClass, hideKeyboardJMethod);

        if (env->ExceptionOccurred())
        {
            env->ExceptionDescribe();
            return;
        }
    }
}
}
}


static void doNativePurchaseRequest(shared_ptr<RBX::DataModel> dm,
		shared_ptr<RBX::Instance> player, std::string productId) {
	RBX::MarketplaceService* marketService = RBX::ServiceProvider::find<
			RBX::MarketplaceService>(dm.get());

	if (!marketService)
		return;

	JNIEnv *env = NULL;
	if (gJvm->AttachCurrentThread(&env, NULL) == JNI_OK) {
		jclass callbackClass = JNI::getCallbackClass();
		if (!callbackClass) {
			marketService->signalPromptNativePurchaseFinished(player,
					productId, false);
			return;
		}

		jmethodID promptNativePurchaseMethod =
				JNI::getPromptNativePurchaseJMethod();
		if (!promptNativePurchaseMethod) {
			marketService->signalPromptNativePurchaseFinished(player,
					productId, false);
			return;
		}

		jstring productIdString = env->NewStringUTF(productId.c_str());
		jstring userNameString = env->NewStringUTF(player->getName().c_str());
		RBX::Network::Player* rawPlayer = RBX::Instance::fastDynamicCast<
				RBX::Network::Player>(player.get());

		env->CallStaticVoidMethod(callbackClass, promptNativePurchaseMethod,
				(jlong) (intptr_t) rawPlayer, productIdString, userNameString);

		if (env->ExceptionOccurred()) {
			env->ExceptionDescribe();
			marketService->signalPromptNativePurchaseFinished(player,
					productId, false);
			return;
		}
	} else {
		marketService->signalPromptNativePurchaseFinished(player, productId, false);
	}
}

static void nativePartyPurchaseRequested(weak_ptr<RBX::DataModel> weakDm,
		shared_ptr<RBX::Instance> player, std::string productId) {
	shared_ptr<RBX::DataModel> dm = weakDm.lock();

	if (!dm)
		return;
	if (!dm.get())
		return;

	RBX::MarketplaceService* marketService = RBX::ServiceProvider::find<
			RBX::MarketplaceService>(dm.get());

	if (!marketService)
		return;

	if (!player) {
		marketService->signalPromptNativePurchaseFinished(player, productId, false);
		return;
	}

	if (RBX::Network::Player* thePlayer = RBX::Instance::fastDynamicCast<
			RBX::Network::Player>(player.get())) {
		RBX::Network::Player* localPlayer =
				RBX::Network::Players::findLocalPlayer(
						RBX::DataModel::get(player.get()));
		if (localPlayer == thePlayer) {
			doNativePurchaseRequest(dm, player, productId);
		} else {
			marketService->signalPromptNativePurchaseFinished(player,
					productId, false);
		}
	} else {
		marketService->signalPromptNativePurchaseFinished(player, productId, false);
	}
}

static void playVideoAd(weak_ptr<RBX::DataModel> weakDm)
{
    RBX::StandardOut::singleton()->printf(MESSAGE_INFO, "SAM in playVideoAd");
	shared_ptr<RBX::DataModel> dm = weakDm.lock();

	if (!dm)
		return;
	if (!dm.get())
		return;

	// Mute the audio
    if(RBX::Soundscape::SoundService* soundService = RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(dm.get()))
    {
        soundService->muteAllChannels(true);
    }

    // Notify the activity
	JNIEnv *env = NULL;
	if (gJvm->AttachCurrentThread(&env, NULL) == JNI_OK)
	{
		jclass callbackClass = JNI::getCallbackClass();
		if (!callbackClass) {
			return;
		}

		srand(time(NULL));
		int rNum = rand() % 100 + 1;
		jmethodID method;

		if (rNum <= FInt::AdColonyPercentage)
		{
            method = JNI::getShowVideoAd_AdColonyJMethod();

            if (!method)
                return;

            env->CallStaticVoidMethod(callbackClass, method);

            if (env->ExceptionOccurred())
            {
                env->ExceptionDescribe();
                return;
            }
		}
		else
		{
		    method = JNI::getShowVideoAd_GoogleJMethod();

            if (!method)
                return;

            jstring typeString = env->NewStringUTF(FString::GoogleVideoAdUrl.c_str());

		    env->CallStaticVoidMethod(callbackClass, method, typeString);

            if (env->ExceptionOccurred())
            {
                env->ExceptionDescribe();
                return;
            }
		}
	}
}

static void onVideoAdFinished(bool adShown) {
	weak_ptr<RBX::Game> weakGame = PlaceLauncher::getPlaceLauncher().getCurrentGame();

	if (shared_ptr<RBX::Game> game = weakGame.lock())
	{
		if (shared_ptr<RBX::DataModel> dm = game->getDataModel())
		{
		    if(RBX::Soundscape::SoundService* soundService = RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(dm.get()))
		    {
		        soundService->muteAllChannels(false);
		    }
		    if(RBX::AdService* adService = RBX::ServiceProvider::find<RBX::AdService>(dm.get()))
		    {
		        adService->videoAdClosed(adShown);
		    }
		}
	}
}

extern "C" {

static void setupDatamodelCallbacks(weak_ptr<RBX::DataModel> dm) {

	if (RBX::UserInputService* inputService = RBX::ServiceProvider::find<
			RBX::UserInputService>(dm.lock().get())) {
		inputService->textBoxGainFocus.connect( boost::bind(&textBoxFocused, _1) );
        inputService->motionEventListeningStarted.connect( boost::bind(&motionEventListening, _1) );
        inputService->textBoxReleaseFocus.connect( boost::bind(&textBoxFocusLost, _1) );
	}
	if (RBX::MarketplaceService* marketplaceService =
			RBX::ServiceProvider::create<RBX::MarketplaceService>(dm.lock().get())) {
		//todo: standalone purchasing!
	//	marketplaceService->promptThirdPartyPurchaseRequested.connect(
		//		boost::bind(&thirdPartyPurchaseRequested, dm, _1, _2));

		marketplaceService->promptNativePurchaseRequested.connect(
						boost::bind(&nativePartyPurchaseRequested, dm, _1, _2));
	}
    if(RBX::AdService* adService = dm.lock()->find<RBX::AdService>()) {
        adService->playVideoAdSignal.connect( boost::bind(playVideoAd, dm) );
    }

}

// This is always called once after nativeOnStart.
JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeStartGame( JNIEnv* jenv, jclass obj, jobject surface, jint jPlaceId, jint jUserId, jstring jsAccessCode,
        jstring jsGameId, jint jJoinRequestType, jstring jsAssetFolderString, jfloat density, jboolean isTouchDevice, jstring josVersion, jstring jDeviceName, jstring jAppVersion, jstring model) {


	FASTLOG(FLog::Android, "nativeStartGame");

    RBX::SystemUtil::mOSVersion = jstringToStdString(jenv, josVersion);
    RBX::SystemUtil::mDeviceName = jstringToStdString(jenv, jDeviceName);
    RBX::Analytics::setReporter("Android");
    RBX::Analytics::setAppVersion(jstringToStdString(jenv, jAppVersion));
    
	jboolean isCopy;
	int placeId = (int)jPlaceId;
	int userId = (int)jUserId;
	int joinRequestType = (int)jJoinRequestType;

	lastPlaceId = placeId;

    const char* accessCodeStr = jenv->GetStringUTFChars(jsAccessCode, &isCopy);
    const char* gameIdStr = jenv->GetStringUTFChars(jsGameId, &isCopy);

	FASTLOG1(FLog::Android, "Place ID: %d", placeId);

	std::string assetFolderPath = jstringToStdString(jenv, jsAssetFolderString);
	StandardOut::singleton()->printf(MESSAGE_INFO, "Asset Folder Path: %s",
			assetFolderPath.c_str());

	RobloxInfo::getRobloxInfo().setAssetFolderPath(assetFolderPath);

	RBXASSERT( aNativeWindow == NULL );
	aNativeWindow = ANativeWindow_fromSurface(jenv, surface);
	FASTLOG1(FLog::Android, "Created ANativeWindow at %p", aNativeWindow);

	int width = ANativeWindow_getWidth(aNativeWindow) / density;
	int height = ANativeWindow_getHeight(aNativeWindow) / density;
    
    // See ActivityGLView.java:initSurfaceView for explanation
    std::string deviceModel = jstringToStdString(jenv, model);
    if (deviceModel == "SM-T230NU") {
        width = 960;
        height = 600;
    }
    
    ANativeWindow_setBuffersGeometry(aNativeWindow, width, height, 0);

	jenv->GetJavaVM(&gJvm);

	StartGameParams sgp;
	sgp.viewWidth = width;
	sgp.viewHeight = height;
	sgp.view = aNativeWindow;

	sgp.placeId = placeId;
	sgp.userId = userId;
    sgp.accessCode = std::string(accessCodeStr, strlen(accessCodeStr));
    sgp.gameId = std::string(gameIdStr, strlen(gameIdStr));
    // 0 = Simple PlaceId join (placeId only) = JOIN_GAME_REQUEST_PLACEID
    // 1 = Follow user join (by userId only) = JOIN_GAME_REQUEST_USERID
    // 2 = Private server join (placeId & accessCode) = JOIN_GAME_REQUEST_PRIVATE_SERVER
    // 3 = Game instance join (placeId & gameId) = JOIN_GAME_REQUEST_GAME_INSTANCE
    sgp.joinRequestType = (JoinGameRequest)joinRequestType;

    StandardOut::singleton()->printf(MESSAGE_INFO, "AccessCode: %s, length = %d", accessCodeStr, strlen(accessCodeStr));
    StandardOut::singleton()->printf(MESSAGE_INFO, "GameId: %s, length = %d", gameIdStr, strlen(gameIdStr));
    StandardOut::singleton()->printf(MESSAGE_INFO, "RequestTypeId: %d", sgp.joinRequestType);

	sgp.assetFolderPath = assetFolderPath;
	sgp.isTouchDevice = isTouchDevice;

    jenv->ReleaseStringUTFChars(jsAccessCode, accessCodeStr);
    jenv->ReleaseStringUTFChars(jsGameId, gameIdStr);


	if (PlaceLauncher::getPlaceLauncher().startGame(sgp)) {
		weak_ptr<RBX::Game> weakGame =
				PlaceLauncher::getPlaceLauncher().getCurrentGame();
		if (shared_ptr<RBX::Game> game = weakGame.lock()) {
			if (shared_ptr<RBX::DataModel> dm = game->getDataModel()) {
				setupDatamodelCallbacks(dm);
			}
		}
	}
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativePassInput(
		JNIEnv* jenv, jclass obj, jint eventId, jint xPos, jint yPos,
		jint eventType, jint windowWidth, jint windowHeight) {
	RobloxInput::getRobloxInput().processEvent(eventId, xPos, yPos, eventType,
			windowWidth, windowHeight);
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativePassPinchGesture(
		JNIEnv* jenv, jclass obj, jint state, jfloat pinchDelta,
		jfloat velocity, jint position1X, jint position1Y, jint position2X,
		jint position2Y) {
	shared_ptr<RBX::Reflection::ValueArray> touchLocations(
			rbx::make_shared<RBX::Reflection::ValueArray>());
	touchLocations->push_back(RBX::Vector2(position1X, position1Y));
	touchLocations->push_back(RBX::Vector2(position2X, position2Y));

	shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<
			RBX::Reflection::Tuple>(3);
	args->values[0] = pinchDelta;
	args->values[1] = velocity;

	switch (state) {
	case 0:
		args->values[2] = RBX::InputObject::INPUT_STATE_BEGIN;
		break;
	case 1:
		args->values[2] = RBX::InputObject::INPUT_STATE_CHANGE;
		break;
	case 2:
		args->values[2] = RBX::InputObject::INPUT_STATE_END;
		break;
	default:
		args->values[2] = RBX::InputObject::INPUT_STATE_NONE;
		break;
	}

	RobloxInput::getRobloxInput().sendGestureEvent(
			RBX::UserInputService::GESTURE_PINCH, args, touchLocations);
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativePassTapGesture(
		jint positionX, jint positionY) {
	shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<
			RBX::Reflection::Tuple>(0);

	shared_ptr<RBX::Reflection::ValueArray> touchLocations(
			rbx::make_shared<RBX::Reflection::ValueArray>());
	touchLocations->push_back(RBX::Vector2(positionX, positionY));

	RobloxInput::getRobloxInput().sendGestureEvent(
			RBX::UserInputService::GESTURE_TAP, args, touchLocations);
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeStopGame(
		JNIEnv* jenv, jclass obj) {
	FASTLOG(FLog::Android, "nativeStopGame");
	PlaceLauncher::getPlaceLauncher().leaveGame(true);
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativePassText(
		JNIEnv* jenv, jclass obj, jlong textBoxInFocus, jstring text, jboolean enterPressed, jint cursorPosition) {
	std::string nativeString = jstringToStdString(jenv, text);

	long textBoxPtr = (long) textBoxInFocus;
	RBX::TextBox* textBoxToPass = (RBX::TextBox*) textBoxPtr;

	RobloxInput::getRobloxInput().passTextInput(textBoxToPass,
			nativeString.c_str(), enterPressed, int(cursorPosition));
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeReleaseFocus(
		JNIEnv* jenv, jclass obj, jlong textBoxInFocus) {
	long textBoxPtr = (long) textBoxInFocus;
	RBX::TextBox* textBoxToPass = (RBX::TextBox*) textBoxPtr;

	RobloxInput::getRobloxInput().externalReleaseFocus(textBoxToPass);
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeOnLowMemory(
		JNIEnv* jenv, jclass obj) {
	FASTLOG(FLog::Android, "nativeOnLowMemory");
	uint64_t preClearBytes = RBX::MemoryStats::usedMemoryBytes();

	RobloxView* rbxView = PlaceLauncher::getPlaceLauncher().getRbxView();
	rbxView->getView()->garbageCollect();

	uint64_t postClearBytes = RBX::MemoryStats::usedMemoryBytes();
	FASTLOG1F(FLog::Android, "PlaceLauncher::clearCachedContent: %.02fMB",
			(preClearBytes - postClearBytes) / (1048576.0));
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeHandleBackPressed(
		JNIEnv* jenv, jclass obj)
{
	weak_ptr<RBX::Game> weakGame = PlaceLauncher::getPlaceLauncher().getCurrentGame();

	if (shared_ptr<RBX::Game> game = weakGame.lock())
	{
		if (shared_ptr<RBX::DataModel> dm = game->getDataModel())
		{
			if (RBX::GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(dm.get()))
			{
				guiService->showLeaveConfirmationSignal();
			}
		}
	}
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeInGamePurchaseFinished(JNIEnv* jenv, jclass obj, jboolean success, jlong player, jstring productId)
{
	weak_ptr<RBX::Game> weakGame = PlaceLauncher::getPlaceLauncher().getCurrentGame();

	if (shared_ptr<RBX::Game> game = weakGame.lock())
	{
		if (shared_ptr<RBX::DataModel> dm = game->getDataModel())
		{
			if( RBX::MarketplaceService* marketService = RBX::ServiceProvider::find<RBX::MarketplaceService>(dm.get()) )
			{
				std::string productIdString = jstringToStdString(jenv, productId);

				long playerLong = (long) player;
				RBX::Network::Player* rawPlayer = (RBX::Network::Player*) playerLong;

				marketService->signalPromptNativePurchaseFinished(shared_from(rawPlayer), productIdString, success);
			}
		}
	}
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeShutDownGraphics( JNIEnv* jenv, jobject surface, jclass obj) {
	RBXASSERT( aNativeWindow != NULL );
	PlaceLauncher::getPlaceLauncher().shutDownGraphics();
	FASTLOG1(FLog::Android, "Destroying ANativeWindow at %p", aNativeWindow);
	ANativeWindow_release( aNativeWindow );
	aNativeWindow = NULL;
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeStartUpGraphics( JNIEnv* jenv, jclass obj, jobject surface, jfloat density, jstring model) {
	RBXASSERT( aNativeWindow == NULL );
	aNativeWindow = ANativeWindow_fromSurface(jenv, surface);
	FASTLOG1(FLog::Android, "Created ANativeWindow at %p", aNativeWindow);
    
	int width = ANativeWindow_getWidth(aNativeWindow) / density;
	int height = ANativeWindow_getHeight(aNativeWindow) / density;
    
    // See ActivityGLView.java:initSurfaceView for explanation
    std::string deviceModel = jstringToStdString(jenv, model);
    if (deviceModel == "SM-T230NU") {
        width = 960;
        height = 600;
    }
    
    // When resuming a game (i.e. coming back from the home screen or activity viewer), we sometimes pull the
	// window's properties while the device is in portrait mode (this happens especially often on phones).
	// If this happens, we would then recreate the GL surface with a flipped height/width.

	// To stop this, we will swap the height and width if we start in portrait mode.
	if (height > width)
	{
	    int temp = width;
	    width = height;
	    height = temp;
	}

	ANativeWindow_setBuffersGeometry(aNativeWindow, width, height, 0);
	PlaceLauncher::getPlaceLauncher().startUpGraphics( aNativeWindow, width, height );
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityGlView_nativeVideoAdFinished( JNIEnv* jenv, jclass obj, jboolean adShown) {
	onVideoAdFinished(adShown);
}

} //extern "C"
