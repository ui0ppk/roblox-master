#pragma once

#include "V8DataModel/GuiObject.h"
#include "V8DataModel/GuiText.h"
#include "rbx/rbxTime.h"
#include "Util/HeartbeatInstance.h"

namespace RBX
{
	extern const char* const sTextBox;

	class TextBox
		: public DescribedCreatable<TextBox, GuiObject, sTextBox>
		, public GuiTextMixin
		, public HeartbeatInstance
	{
	private:
		typedef DescribedCreatable<TextBox, GuiObject, sTextBox> Super;
		bool shouldCaptureFocus;
		bool iAmFocus;
		bool showingCursor;
		bool clearTextOnFocus;
		std::string bufferedText;
		RBX::Time lastSwap;
		bool shouldFocusFromInput;

		int cursorPos; // this represents the character the cursor should be drawn after
		
		struct RepeatKeyState
		{
			enum KeyType
			{
				TYPE_BACKSPACE,
				TYPE_DELETE,
				TYPE_PASTE,
				TYPE_CHARACTER,
				TYPE_LEFTARROW,
				TYPE_RIGHTARROW
			};
			KeyType keyType;
			RBX::KeyCode keyCode;
			char character;

			enum RepeatState
			{
				STATE_NONE,
				STATE_DEPRESSED, 
				STATE_REPEATING
			};
			double stateWallTime;
			RepeatState state;
			RepeatKeyState()
				:state(STATE_NONE)
			{}
		};


		RepeatKeyState repeatKeyState;
		bool multiLine;

		void doKey(RepeatKeyState::KeyType keyType, std::string key);
		void doKey(RepeatKeyState::KeyType keyType, char key);

		void keyUp(RepeatKeyState::KeyType keyType, RBX::KeyCode keyCode, char key);
		void keyDown(RepeatKeyState::KeyType keyType, RBX::KeyCode keyCode, char key);
		void textInput(RepeatKeyState::KeyType keyType, std::string textInput);

		int getCursorPos(RBX::Vector2 mousePos);

		void gainFocus(const shared_ptr<InputObject>& event);
        void setFocusLost(bool enterPressed, const shared_ptr<InputObject>& inputThatCausedFocusLoss);

		std::string getTextWithCursor();
		std::string getTextWithBlankCursor();
        
        bool isPasteCommand(const shared_ptr<InputObject>& event);

		void selectionToggled(const shared_ptr<InputObject>& event);
        
        rbx::signals::scoped_connection textBoxFinishedEditingConnection;

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IRenderable
		/*override*/ void render2d(Adorn* adorn);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// HeartbeatInstance
		/*override*/ void onHeartbeat(const Heartbeat& event);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		/*override*/ void onAncestorChanged(const AncestorChanged& event);

	protected:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiObject
		/*override*/ GuiResponse processTextInputEvent(const shared_ptr<InputObject>& event);
		/*override*/ GuiResponse processKeyEvent(const shared_ptr<InputObject>& event);
		/*override*/ GuiResponse processGamepadEvent(const shared_ptr<InputObject>& event);
		/*override*/ GuiResponse processMouseEvent(const shared_ptr<InputObject>& event);
		/*override*/ GuiResponse processTouchEvent(const shared_ptr<InputObject>& event);
		/*override*/ GuiResponse preProcessMouseEvent(const shared_ptr<InputObject>& event);

	public:
		TextBox();
    
		std::string getBufferedText() const { return bufferedText; }
		void setBufferedText(std::string value, int newCursorPosition);

		bool getMultiLine() const { return multiLine; }
		void setMultiLine(bool value);

		bool getClearTextOnFocus() const { return clearTextOnFocus; }
		void setClearTextOnFocus(bool value);

		rbx::signal<void(bool, const shared_ptr<Instance>)> focusLostSignal;
		rbx::signal<void()> focusGainedSignal;

		void captureFocus();
		void externalReleaseFocus(const char* externalReleaseText, bool enterPressed, const shared_ptr<InputObject>& inputThatCausedFocusLoss);
		void releaseFocusLua();
		void releaseFocus(bool enterPressed, const shared_ptr<InputObject>& inputThatCausedFocusLoss, Instance* contextLocalCharacter=NULL );
		bool getFocused();

		void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiObject
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
		/*override*/ GuiResponse preProcess(const shared_ptr<InputObject>& event);

		DECLARE_GUI_TEXT_MIXIN();
	};

}
