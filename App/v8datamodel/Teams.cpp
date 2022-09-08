#include "stdafx.h"

#include "v8tree/instance.h"
#include "v8tree/service.h"
#include "v8datamodel/teams.h"
#include "V8Datamodel/DataModel.h"
#include "network/player.h"
#include "network/players.h"
#include "v8DataModel/workspace.h"
#include "Humanoid/Humanoid.h"

namespace RBX {

const char *const sTeams = "Teams";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<Teams, void()> teams_rebalanceTeamsFunction(&Teams::rebalanceTeams, "RebalanceTeams", Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<Teams, shared_ptr<const Instances>()> func_teams(&Teams::getTeams, "GetTeams", Security::None);
REFLECTION_END();

Teams::Teams() :
	teams(Instances())		// Initialize to a non-NULL value
{
	setName(sTeams);
}
Teams::~Teams() 
{

}



bool Teams::isTeamGame()
{
	/* returns true if any player is non-neutral */
	if(Network::Players *players = ServiceProvider::find<Network::Players>(this)){
		for(unsigned int n = 0; n < players->numChildren(); n++)
		{
			Network::Player* p = Instance::fastDynamicCast<Network::Player>(players->getChild(n));
			if (p == NULL) continue; // only care about player objects
			if (p->getNeutral() == false) 
				return true;
		}
	}
	return false;
}

void Teams::rebalanceTeams()
{
	// Do the elegant but slow thing - O(n^2)
	
	/*
	Network::Players *players = ServiceProvider::find<Network::Players>(this);
	RBXASSERT(players);	

	for(unsigned int n = 0; n < players->numChildren(); n++)
	{
		Network::Player* p = Instance::fastDynamicCast<Network::Player>(players->getChild(n));
		p->setTeam(NULL);
		assignNewPlayerToTeam(p);
	}
	*/
}

void Teams::assignNewPlayerToTeam(Network::Player *p)
{
	// find all team children of this service with the assignable property == true
	// put the player on the team that has the least people.
	BrickColor best = BrickColor::brickGreen();
	int best_count = 10000;
	bool found = false; // found one


	for(unsigned int i = 0; i < this->numChildren(); i++)
	{
		Team* child = Instance::fastDynamicCast<Team>(this->getChild(i));
		if(child != NULL) {
			if(child->getAutoAssignable() == true) {
				int count = getNumPlayersInTeam(child->getTeamColor());
				if (count < best_count) {
					best = child->getTeamColor();
					best_count = count;
					found = true;
				}
			}
		}
	}

	if (found)
	{
		p->setTeamColor(best);
		p->setNeutral(false);
	}

}

int Teams::getNumPlayersInTeam(BrickColor color)
{
	int result = 0;
	Network::Players *players = ServiceProvider::find<Network::Players>(this);
	RBXASSERT(players);	

	for(unsigned int n = 0; n < players->numChildren(); n++)
	{
		Network::Player* p = Instance::fastDynamicCast<Network::Player>(players->getChild(n));
		if (p == NULL) continue; // only care about player objects
		if (p->getNeutral() == false && p->getTeamColor() == color) result++;
		
	}
	return result;
}

bool Teams::teamExists(BrickColor color)
{
	return (getTeamFromTeamColor(color) != NULL);
}


Team *Teams::getTeamFromTeamColor(BrickColor c)
{
	for(unsigned int i = 0; i < numChildren(); i ++)
	{
		Team *t = Instance::fastDynamicCast<Team>(getChild(i));
		if (t && t->getTeamColor() == c) 
				return t;
	}
	return NULL;
}

Team *Teams::getTeamFromPlayer(Network::Player *p)
{
	if(p->getNeutral()) 
		return NULL;

	return getTeamFromTeamColor(p->getTeamColor());
}

BrickColor Teams::getUnusedTeamColor()
{
	BrickColor::Colors vec(BrickColor::allColors());
	
	BrickColor::Colors::iterator iter;

	for(iter = vec.begin(); iter != vec.end(); iter++)
	{
		BrickColor c = *iter;

		for(unsigned int i = 0; i < numChildren(); i ++)
		{
			if (Team *t = Instance::fastDynamicCast<Team>(getChild(i)))
				if (t->getTeamColor() == c) 
					vec.erase(iter);
		}
	}

	RBXASSERT(vec.size() > 0);

	return vec[rand() % vec.size()];
}

G3D::Color3 Teams::getTeamColorForHumanoid(Humanoid *h)
{
	Network::Players *players = ServiceProvider::find<Network::Players>(this);
	RBXASSERT(players);	

	if (players) 
	{
		for(unsigned int n = 0; n < players->numChildren(); n++)
		{
			if (Network::Player* p = Instance::fastDynamicCast<Network::Player>(players->getChild(n)))
				if (p->getNeutral() == false)
					if (ModelInstance *m = p->getCharacter())
						if (m->findFirstChildOfType<Humanoid>() == h)	
							return p->getTeamColor().color3();
		}
	}
	return G3D::Color3::white();
}

void Teams::onChildAdded(Instance* child)
{
	if (Team* t = Instance::fastDynamicCast<Team>(child))
	{
		boost::shared_ptr<Instances>& c = teams.write();
		c->push_back(shared_from(t));
	}
}

void Teams::onChildRemoving(Instance* child)
{
	if (Team* t = Instance::fastDynamicCast<Team>(child))
	{
		boost::shared_ptr<Instances>& c = teams.write();
		c->erase(std::find(c->begin(), c->end(), shared_from(t)));
	}
}

} // namespace
