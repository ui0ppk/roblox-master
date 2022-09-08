#pragma once

#include "V8Tree/Instance.h"

#include <atomic>

namespace RBX
{
    class DataModel;

    enum XboxKeyBoardType;
}


class KeyboardProvider
{
public:
    void registerTextBoxListener(RBX::DataModel* dm);
    void showKeyBoard(shared_ptr<RBX::Instance> instance);
    void showKeyBoardLua(std::string& title, std::string& description, std::string& defaultText, RBX::XboxKeyBoardType keyboardType, RBX::DataModel* dm);
private:
    void showKeyBoard(std::string& title, std::string& description, const std::string& defaultText, RBX::XboxKeyBoardType keyboardType, std::function<void(Platform::String^)>& successLambda, std::function<void(void)>& cancelLambda);

    rbx::signals::scoped_connection showKeyboardSignal;

    static volatile long keyboardOn;
};