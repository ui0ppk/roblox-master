/* Copyright 2003-2010 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"

namespace RBX { 
	extern const char* const sDialogChoice;

	class DialogChoice
		: public DescribedCreatable<DialogChoice, Instance, sDialogChoice>
	{
	private:
		std::string userDialog;
		std::string responseDialog;
		std::string goodbyeDialog;

		rbx::signal<void(shared_ptr<Instance>)> selected;
	public:
		DialogChoice();

		std::string getUserDialog() const { return userDialog; }
		void setUserDialog(std::string value);

		std::string getResponseDialog() const { return responseDialog; }
		void setResponseDialog(std::string value);

		std::string getGoodbyeDialog() const { return goodbyeDialog; }
		void setGoodbyeDialog(std::string value);

		///////////////////////////////////////////////////////////////////////////
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;

	};
}