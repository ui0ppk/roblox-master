/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/commonverbs.h"		// TODO - minimize these includes, and in the .h file
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/UserController.h"
#include "V8DataModel/JointsService.h"
#include "V8DataModel/GuiBuilder.h"
#include "V8DataModel/Sky.h"
#include "V8DataModel/Lighting.h"
#include "V8DataModel/Teams.h"
#include "V8DataModel/Hopper.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/Tool.h"

#include "Humanoid/Humanoid.h"
#include "Network/Player.h"
#include "Network/Players.h"

#include "V8Tree/verb.h"
#include "V8World/World.h"
#include "V8Kernel/Kernel.h"
#include "Gui/GUI.h"
#include "Script/luainstancebridge.h"
#include "Script/scriptcontext.h"
#include "Util/SoundWorld.h"
#include "Util/Sound.h"
#include "Util/StandardOut.h"
#include "rbx/Log.h"
#include "AppDraw/DrawPrimitives.h"
#include "util/http.h"

#include "g3d/format.h"

#include "V8Xml/Serializer.h"
#include "boost/cast.hpp"
#include "boost/scoped_ptr.hpp"



namespace RBX {

CommonVerbs::CommonVerbs(DataModel* dataModel) 
	: 
	joinCommand(dataModel),
	statsCommand(dataModel),
	renderStatsCommand(dataModel),
	engineStatsCommand(dataModel),
	networkStatsCommand(dataModel),
	physicsStatsCommand(dataModel),
	summaryStatsCommand(dataModel),
	customStatsCommand(dataModel),
	runCommand(dataModel),
	stopCommand(dataModel),
	resetCommand(dataModel),
	axisRotateToolVerb(dataModel),
	resizeToolVerb(dataModel),
	advMoveToolVerb(dataModel),
	advRotateToolVerb(dataModel),
	advArrowToolVerb(dataModel, false),
	turnOnManualJointCreationVerb(dataModel),	
	setDragGridToOneVerb(dataModel),
	setDragGridToOneFifthVerb(dataModel),
	setDragGridToOffVerb(dataModel),
	setGridSizeToTwoVerb(dataModel),
	setGridSizeToFourVerb(dataModel),
	setGridSizeToSixteenVerb(dataModel),
	setManualJointToWeakVerb(dataModel),
	setManualJointToStrongVerb(dataModel),
	setManualJointToInfiniteVerb(dataModel),

	flatToolVerb(dataModel),
	glueToolVerb(dataModel),
	weldToolVerb(dataModel),
	studsToolVerb(dataModel),
	inletToolVerb(dataModel),
	universalToolVerb(dataModel),
	hingeToolVerb(dataModel),
	rightMotorToolVerb(dataModel),
	leftMotorToolVerb(dataModel),
	oscillateMotorToolVerb(dataModel),
	smoothNoOutlinesToolVerb(dataModel),

	anchorToolVerb(dataModel),
	lockToolVerb(dataModel),
	fillToolVerb(dataModel, false),
	materialToolVerb(dataModel, false),
	materialVerb(dataModel),
	colorVerb(dataModel),
	anchorVerb(dataModel),
	dropperToolVerb(dataModel),

	firstPersonCommand(dataModel),
	selectChildrenVerb(dataModel),
	snapSelectionVerb(dataModel),
	playDeleteSelectionVerb(dataModel),
	deleteSelectionVerb(dataModel),
	moveUpPlateVerb(dataModel),
	moveUpBrickVerb(dataModel),
	moveDownSelectionVerb(dataModel),
	rotateSelectionVerb(dataModel),
	tiltSelectionVerb(dataModel),
	selectAllCommand(dataModel),
	allCanSelectCommand(dataModel),
	canNotSelectCommand(dataModel),
	translucentVerb(dataModel),
	canCollideVerb(dataModel),
	unlockAllVerb(dataModel),
	gameToolVerb(dataModel),
	grabToolVerb(dataModel),
	cloneToolVerb(dataModel),
	hammerToolVerb(dataModel)
{
}


}
