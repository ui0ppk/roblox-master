/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once
#include "V8Tree/Instance.h"
#include "V8Tree/Verb.h"

namespace RBX {
	class Workspace;
	class Mouse;

	extern const char *const sStudioTool;

	class StudioTool 
		: public DescribedNonCreatable<StudioTool, Instance, sStudioTool>
	{
	protected:
		shared_ptr<Mouse> onEquipping(Workspace* workspace);
		bool enabled;
	public:
		StudioTool();

		bool getEnabled() const { return enabled; }
		void setEnabled(bool);

		void activate();
		void deactivate();

		void equip(Workspace*);
		void unequip();

		rbx::signal<void(shared_ptr<Instance>)> equippedSignal;
		rbx::signal<void()> activatedSignal;
		rbx::signal<void()> unequippedSignal;
		rbx::signal<void()> deactivatedSignal;
	};
}
