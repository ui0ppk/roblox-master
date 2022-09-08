#include "KeyboardProvider.h"

#include "XboxService.h"
#include "XboxUtils.h"
#include "async.h"

#include <thread>

#include "v8datamodel/TextBox.h"

using namespace Windows::Xbox::UI;
using namespace RBX;

volatile long KeyboardProvider::keyboardOn = 0;

void KeyboardProvider::registerTextBoxListener(RBX::DataModel* dm)
{
    if (auto uis = RBX::ServiceProvider::create<UserInputService>(dm))
    {
        showKeyboardSignal = uis->textBoxGainFocus.connect(boost::bind(&KeyboardProvider::showKeyBoard, this, _1));
    }
}

void KeyboardProvider::showKeyBoard(std::string& title, std::string& description, const std::string& defaultText, RBX::XboxKeyBoardType keyboardType, std::function<void(Platform::String^)>& successLambda, std::function<void(void)>& cancelLambda)
{
    if( InterlockedCompareExchange(&keyboardOn, 1, 0 ) ) 
        return;


    VirtualKeyboardInputScope inputScope = VirtualKeyboardInputScope::Default;

    switch (keyboardType)
    {
    case RBX::xbKeyBoard_Default:
        inputScope = VirtualKeyboardInputScope::Default;
        break;
    case RBX::xbKeyBoard_EmailSmtpAddress:
        inputScope = VirtualKeyboardInputScope::EmailSmtpAddress;
        break;
    case RBX::xbKeyBoard_Number:
        inputScope = VirtualKeyboardInputScope::Number;
        break;
    case RBX::xbKeyBoard_Password:
        inputScope = VirtualKeyboardInputScope::Password;
        break;
    case RBX::xbKeyBoard_Search:
        inputScope = VirtualKeyboardInputScope::Search;
        break;
    case RBX::xbKeyBoard_TelephoneNumber:
        inputScope = VirtualKeyboardInputScope::TelephoneNumber;
        break;
    case RBX::xbKeyBoard_Url:
        inputScope = VirtualKeyboardInputScope::Url;
        break;
    default:
        RBXASSERT(false); // new type?
        break;
    }

    try
    {
        async(SystemUI::ShowVirtualKeyboardAsync(ref new Platform::String(s2ws(&defaultText).data()), ref new Platform::String(s2ws(&title).data()), ref new Platform::String(s2ws(&description).data()), inputScope))
        .complete(successLambda)
        .cancelled(cancelLambda)
        .error(cancelLambda)
        .detach();
    }
    catch(Platform::Exception^ e)
    {
        dprintf("ShowVirtualKeyboard exception: [%u] %S\n", (unsigned)e->HResult, e->Message->Data() );
        cancelLambda();
    }
}

void KeyboardProvider::showKeyBoardLua(std::string& title, std::string& description, std::string& defaultText, RBX::XboxKeyBoardType keyboardType, RBX::DataModel* dm)
{
    std::function<void(void)> cancelLambda = [=]() -> void
    {
        dm->submitTask([=](...)
        {
            if(PlatformService* p = ServiceProvider::find<PlatformService>(dm))
                p->keyboardClosedSignal(defaultText.data());
        }
        , DataModelJob::Write);

        InterlockedExchange(&keyboardOn, 0);
    };

    std::function<void(Platform::String^ defaultText)> successLambda = [=](Platform::String^ defaultText) -> void
    {
        dm->submitTask([=](...)
        {
            std::string textBoxValue = ws2s(defaultText->Data());
            if(PlatformService* p = ServiceProvider::find<PlatformService>(dm))
                p->keyboardClosedSignal(textBoxValue.data());
        }
        , DataModelJob::Write);

        InterlockedExchange(&keyboardOn, 0);
    };

    showKeyBoard(title, description, defaultText, keyboardType, successLambda, cancelLambda);
}

void KeyboardProvider::showKeyBoard(shared_ptr<RBX::Instance> instance)
{
    shared_ptr<RBX::TextBox> textBox =  RBX::Instance::fastSharedDynamicCast<RBX::TextBox>(instance);
    RBX::DataModel* dm = DataModel::get(instance.get());

    if (dm && textBox)
    {
        std::string text = textBox->getText();
        std::wstring wstr(text.begin(), text.end());
        Platform::String^ defaultText = ref new Platform::String(wstr.c_str());

        std::function<void(void)> cancelLambda = [=]() -> void
        {
            if (textBox)
            {
                dm->submitTask([=](...)
                {
                    if (auto uis = RBX::ServiceProvider::create<UserInputService>(dm))
                        uis->textboxDidFinishEditing(text.c_str(), false);
                }
                , DataModelJob::Write);
            }

            InterlockedExchange(&keyboardOn, 0);
        };

        std::function<void(Platform::String^ defaultText)> successLambda = [=](Platform::String^ defaultText) -> void
        {
            if (textBox)
            {
                std::string textBoxValue = ws2s(defaultText->Data());

                dm->submitTask([=](...)
                {
                    if (auto uis = RBX::ServiceProvider::create<UserInputService>(dm))
                        uis->textboxDidFinishEditing(textBoxValue.c_str(), true);
                }
                , DataModelJob::Write);
            }

            InterlockedExchange(&keyboardOn, 0);
        };

        std::string title = "Text Entry";
        std::string desc = "Please input text";
        showKeyBoard(title, desc, textBox->getText(), RBX::xbKeyBoard_Default, successLambda, cancelLambda);
    }    
}