#include "stdafx.h"

#include "v8datamodel/team.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Teams.h"
#include "Humanoid/Humanoid.h"
#include "Network/Players.h"
#include "Network/Player.h"


namespace RBX {

const char *const sTeam = "Team";

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<Team, int> prop_Score("Score", category_Data, &Team::getScore, &Team::setScore, Reflection::PropertyDescriptor::Attributes::deprecated());
static const Reflection::PropDescriptor<Team, BrickColor> prop_Color("TeamColor", category_Data, &Team::getTeamColor, &Team::setTeamColor);
static const Reflection::PropDescriptor<Team, bool> prop_AutoAssignable("AutoAssignable", category_Data, &Team::getAutoAssignable, &Team::setAutoAssignable);
Reflection::BoundProp<bool> Team::prop_AutoColorCharacters("AutoColorCharacters", category_Data, &Team::autoColorCharacters, Reflection::PropertyDescriptor::Attributes::deprecated());
REFLECTION_END();

Team::Team() :
autoAssignable(true),
score(0),
autoColorCharacters(true)
{
	setName(sTeam);
	color = BrickColor::brickWhite();
}

Team::~Team() 
{
}

bool Team::askSetParent(const Instance* parent) const 
{
	return Instance::fastDynamicCast<Teams>(parent)!=NULL;
}

int Team::getScore() const 
{
	return score; 
}

void Team::setScore(int score) 
{
	if (this->score != score) {
		this->score = score; 
		raisePropertyChanged(prop_Score);
	}
}

BrickColor Team::getTeamColor() const 
{ 
	return color; 
}

void Team::setTeamColor(BrickColor color) 
{
	if (this->color != color) {
		this->color = color;
		raisePropertyChanged(prop_Color);
	}
}

bool Team::getAutoAssignable() const {return autoAssignable;}
void Team::setAutoAssignable(bool autoAssign) {
	if (this->autoAssignable != autoAssign) 
	{
		this->autoAssignable = autoAssign;
		raisePropertyChanged(prop_AutoAssignable);
	}
}

	

} // namespace
