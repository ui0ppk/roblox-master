/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "V8Tree/Instance.h"
#include "V8DataModel/Tool.h"
#include "Util/BrickColor.h"
#include "V8DataModel/PartInstance.h"

namespace RBX {


	extern const char *const sFlag;

	class FlagStand;
	class TimerService;
	namespace Network { class Player; }

	class Flag 
		: public DescribedCreatable<Flag, Tool, sFlag>
	{
	private:
		typedef DescribedCreatable<Flag, Tool, sFlag> Super;
		rbx::signals::scoped_connection_logged flagTouched;
		void onEvent_flagTouched(shared_ptr<Instance> other);

	protected:
		/*override*/
		virtual bool canUnequip() {return false;} // The flag cannot be unequipped
		virtual bool canBePickedUpByPlayer(Network::Player *p); // The flag cannot be picked up by a member of the same team as the flag.

	public:
		BrickColor teamColor;

		BrickColor getTeamColor() const;
		void setTeamColor(BrickColor color);

		void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		
		FlagStand* getJoinedStand();

		Flag();
		~Flag();
	};

} // namespace
