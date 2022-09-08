/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/DataModel.h"
#include "V8DataModel/Commands.h"
#include "V8DataModel/ToolsPart.h"
#include "V8DataModel/ToolsSurface.h"
#include "V8DataModel/ToolsModel.h"
#include "Tool/ToolsArrow.h"
#include "Tool/ResizeTool.h"
#include "Tool/HammerTool.h"
#include "Tool/GrabTool.h"
#include "Tool/CloneTool.h"
#include "Tool/NullTool.h"
#include "Tool/GameTool.h"
#include "Tool/AxisMoveTool.h"
#include "Tool/AxisRotateTool.h"
#include "Tool/MoveResizeJoinTool.h"
#include "Tool/AdvMoveTool.h"
#include "Tool/AdvRotateTool.h"
#include "V8DataModel/UndoRedo.h"
#include "V8DataModel/InputObject.h"
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "Util/Runstateowner.h"

#include "Util/InsertMode.h"

class XmlElement;


namespace RBX {

class Fonts;
class GuiRoot;
class GuiItem;
class ContentProvider;
class TimeState;
class Hopper;
class PlayerHopper;
class StarterPackService;
class Adorn;

// Contain a set of verbs used by Roblox
class CommonVerbs
{
public:
	CommonVerbs(DataModel* dataModel);
	///////////////////////////////////////////////////////////////////////
	//
	// Play Mode Commands
	PlayDeleteSelectionVerb playDeleteSelectionVerb;

	// Edit Menu
	DeleteSelectionVerb deleteSelectionVerb;
	SelectAllCommand selectAllCommand;
	SelectChildrenVerb selectChildrenVerb;
	SnapSelectionVerb snapSelectionVerb;
	UnlockAllVerb unlockAllVerb;
	ColorVerb colorVerb;
	MaterialVerb materialVerb;
	
	// Format Menu
	AnchorVerb anchorVerb;
	TranslucentVerb translucentVerb;
	CanCollideVerb canCollideVerb;
	CanNotSelectCommand canNotSelectCommand;
	AllCanSelectCommand allCanSelectCommand;
	MoveUpPlateVerb moveUpPlateVerb;
	MoveUpBrickVerb moveUpBrickVerb;
	MoveDownSelectionVerb moveDownSelectionVerb;
	RotateSelectionVerb rotateSelectionVerb;
	TiltSelectionVerb tiltSelectionVerb;

	// Run Menu
	RunCommand runCommand;
	StopCommand stopCommand;
	ResetCommand resetCommand;

	// Test Menu
	FirstPersonCommand firstPersonCommand;
	StatsCommand statsCommand;
	RenderStatsCommand renderStatsCommand;
	EngineStatsCommand engineStatsCommand;
	NetworkStatsCommand networkStatsCommand;
	PhysicsStatsCommand physicsStatsCommand;
	SummaryStatsCommand summaryStatsCommand;
	CustomStatsCommand customStatsCommand;
	JoinCommand joinCommand;

	// Adv Build Related
	TurnOnManualJointCreation turnOnManualJointCreationVerb;
    
	SetDragGridToOne setDragGridToOneVerb;
	SetDragGridToOneFifth setDragGridToOneFifthVerb;
	SetDragGridToOff setDragGridToOffVerb;

    SetGridSizeToTwo setGridSizeToTwoVerb;
    SetGridSizeToFour setGridSizeToFourVerb;
    SetGridSizeToSixteen setGridSizeToSixteenVerb;

	SetManualJointToWeak setManualJointToWeakVerb;
	SetManualJointToStrong setManualJointToStrongVerb;
	SetManualJointToInfinite setManualJointToInfiniteVerb;

	// Tools
	TToolVerb<AxisRotateTool> axisRotateToolVerb;
	TToolVerb<AdvMoveTool> advMoveToolVerb;
	TToolVerb<AdvRotateTool> advRotateToolVerb;
	TToolVerb<AdvArrowTool> advArrowToolVerb;
	TToolVerb<MoveResizeJoinTool> resizeToolVerb;

	TToolVerb<FlatTool> flatToolVerb;
	TToolVerb<GlueTool> glueToolVerb;
	TToolVerb<WeldTool> weldToolVerb;
	TToolVerb<StudsTool> studsToolVerb;
	TToolVerb<InletTool> inletToolVerb;
	TToolVerb<UniversalTool> universalToolVerb;
	TToolVerb<HingeTool> hingeToolVerb;
	TToolVerb<RightMotorTool> rightMotorToolVerb;
	TToolVerb<LeftMotorTool> leftMotorToolVerb;
	TToolVerb<OscillateMotorTool> oscillateMotorToolVerb;
	TToolVerb<SmoothNoOutlinesTool> smoothNoOutlinesToolVerb;

	TToolVerb<AnchorTool> anchorToolVerb;
	TToolVerb<LockTool> lockToolVerb;

	TToolVerb<FillTool> fillToolVerb;
	TToolVerb<MaterialTool> materialToolVerb;
	TToolVerb<DropperTool> dropperToolVerb;

	// Runtime Tools
	TToolVerb<GameTool> gameToolVerb;
	TToolVerb<GrabTool> grabToolVerb;
	TToolVerb<CloneTool> cloneToolVerb;
	TToolVerb<HammerTool> hammerToolVerb;
};

} // namespace
