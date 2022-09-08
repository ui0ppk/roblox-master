/* Copyright 2003-2010 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"

namespace RBX { 
	extern const char* const sDialogRoot;

	class DialogRoot
		: public DescribedCreatable<DialogRoot, Instance, sDialogRoot>
	{
	public:
		enum DialogPurpose
		{
			QUEST_PURPOSE,
			HELP_PURPOSE,
			SHOP_PURPOSE
		};
		
		enum DialogTone
		{
			NEUTRAL_TONE,
			FRIENDLY_TONE,
			ENEMY_TONE
		};
	private:
		typedef DescribedCreatable<DialogRoot, Instance, sDialogRoot> Super;

		bool publicChat;
		bool inUse;
		float conversationDistance;

		std::string initialPrompt;
		std::string goodbyeDialog;
		DialogPurpose dialogPurpose;
		DialogTone dialogTone;

		rbx::signal<void(shared_ptr<Instance>)> dialogChoice;
	public:
		DialogRoot();
		~DialogRoot();
		
		DialogPurpose getDialogPurpose() const { return dialogPurpose; }
		void setDialogPurpose(DialogPurpose value);

		DialogTone getDialogTone() const { return dialogTone; }
		void setDialogTone(DialogTone value);

		bool getPublicChat() const { return publicChat; }
		void setPublicChat(bool value);

		bool getInUse() const { return inUse; }
		void setInUse(bool value);

		float getConversationDistance() const { return conversationDistance; }
		void setConversationDistance(float value);

		std::string getInitialPrompt() const { return initialPrompt; }
		void setInitialPrompt(std::string value);

		std::string getGoodbyeDialog() const { return goodbyeDialog; }
		void setGoodbyeDialog(std::string value);

		void signalDialogChoice(shared_ptr<Instance> player, shared_ptr<Instance> dialogChoice);

		rbx::remote_signal<void(shared_ptr<Instance>, shared_ptr<Instance>)> dialogChoiceSelected;
		
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		/*override*/ bool askSetParent(const Instance* instance) const;

	};
}