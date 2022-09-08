#pragma once

#include "v8tree/instance.h"
#include "v8tree/service.h"
#include "v8datamodel/team.h"
#include "G3D/Color3.h"
#include "Util/BrickColor.h"

namespace RBX { 

	class Humanoid;

	namespace Network { class Player; }
	extern const char *const sTeams;

	class Teams
		: public DescribedCreatable<Teams, Instance, sTeams, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
		, public Service
	{
	public:
		Teams();
		~Teams();
		void assignNewPlayerToTeam(Network::Player *p);
		int getNumPlayersInTeam(BrickColor color);
		bool teamExists(BrickColor color);
		bool isTeamGame();
		void rebalanceTeams();
		BrickColor getUnusedTeamColor();
		Team *getTeamFromTeamColor(BrickColor c);
		Team *getTeamFromPlayer(Network::Player *p);

		shared_ptr<const Instances> getTeams() {return teams.read();}

		G3D::Color3 getTeamColorForHumanoid(Humanoid *h);
	
	private:
		copy_on_write_ptr<Instances> teams;

	protected:
		/*override*/ void onChildAdded(Instance* child);
		/*override*/ void onChildRemoving(Instance* child);
		/* override */ bool askAddChild(const Instance* child) const {return Instance::fastDynamicCast<Team>(child)!=NULL;}

	};

} 