/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Verb.h"
#include "Util/RunStateOwner.h"
#include "Tool/ToolsArrow.h"
#include "v8datamodel/Workspace.h"
#include "PartOperation.h"

DYNAMIC_FASTFLAG(UseRemoveTypeIDTricks)

namespace RBX {
	class PVInstance;
	class Workspace;
	class DataModel;
	class Camera;

	// Utility function
	void AddChildToRoot(XmlElement* root, shared_ptr<Instance> wsi, const boost::function<bool(Instance*)>& isInScope, RBX::CreatorRole creatorRole);
	void AddSelectionToRoot(XmlElement* root, Selection* sel, RBX::CreatorRole creatorRole);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BASE COMMAND TYPES
//

class RunStateVerb :
	public Verb
{
protected:
	DataModel* dataModel;
	ServiceClient<RunService> runService;
	RunStateVerb(std::string name, DataModel* dataModel, bool blacklisted = false);
	void playActionSound();
public:
	virtual ~RunStateVerb();
};

// Edit commands are only enabled when the workspace is at Frame 0
// and when something is selected
class EditSelectionVerb
	: public Verb
{
protected:
	shared_ptr<Workspace> workspace;
	ServiceClient<Selection> selection;
	DataModel* dataModel;
	EditSelectionVerb(std::string name, DataModel* dataModel);
	EditSelectionVerb(VerbContainer* container, std::string name, DataModel* dataModel);
public:
	virtual ~EditSelectionVerb();
	virtual bool isEnabled() const;
};

// Toggles the value of a bool property
class BoolPropertyVerb 
	: public EditSelectionVerb
{
private:
	const Name& propertyName;

protected:
	BoolPropertyVerb(
		const std::string& name, 
		DataModel* dataModel,
		const char* propertyName );
	/*override*/ void doIt(IDataState* dataState);
	/*override*/ bool isChecked() const;
};





// Camera commands - always enabled
class CameraVerb  :	public Verb
{
protected:
	Workspace* workspace;
	Camera* getCamera();
	const Camera* getCamera() const;
	ServiceClient<Selection> selection;

public:
	CameraVerb(std::string name, Workspace* _workspace);
	virtual bool isEnabled() const			{return true;}
	virtual void doIt(IDataState* dataState);	
};



//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//
// EDIT MENU - descend from RunFrameZero commands
class DeleteBase : public EditSelectionVerb
{
private:
	bool rewardHopper;
protected:
	DeleteBase(VerbContainer* container, DataModel* dataModel, std::string name);
	DeleteBase(DataModel* dataModel, std::string name, bool rewardHopper = false);
public:
	virtual void doIt(IDataState* dataState);
};

class DeleteSelectionVerb : public DeleteBase
{
protected:
	DeleteSelectionVerb(VerbContainer* container, DataModel* dataModel, std::string name)
		: DeleteBase(container, dataModel, name) {}
public:
	DeleteSelectionVerb(DataModel* dataModel)
		: DeleteBase(dataModel, "Delete") {}
};

class PlayDeleteSelectionVerb : public DeleteBase
{
public:
	PlayDeleteSelectionVerb(DataModel* dataModel)
		: DeleteBase(dataModel, "PlayDelete", true) {}
};


class SelectAllCommand : public RunStateVerb
{
public:
	SelectAllCommand(DataModel* dataModel) : 
		RunStateVerb("SelectAll", dataModel) {}
	virtual void doIt(IDataState* dataState);
};

class SelectChildrenVerb : public EditSelectionVerb
{
private:
	typedef EditSelectionVerb Super;

public:
	SelectChildrenVerb(DataModel* dataModel);
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

class SnapSelectionVerb : public EditSelectionVerb
{
private:
	typedef EditSelectionVerb Super;

public:
	SnapSelectionVerb(DataModel* dataModel);
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

class UnlockAllVerb : public RunStateVerb
{
public:
	UnlockAllVerb(DataModel* dataModel) : 
	  RunStateVerb("UnlockAll", dataModel) {}
	  virtual void doIt(IDataState* dataState);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// VIEW MENU
//

class CameraTiltUpCommand : public CameraVerb
{
public:
	CameraTiltUpCommand(Workspace* workspace) : CameraVerb("CameraTiltUp", workspace)	{}
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

class CameraTiltDownCommand : public CameraVerb
{
public:
	CameraTiltDownCommand(Workspace* workspace) : CameraVerb("CameraTiltDown", workspace)	{}
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

class CameraPanLeftCommand : public CameraVerb
{
public:
	CameraPanLeftCommand(Workspace* workspace) : CameraVerb("CameraPanLeft", workspace)	{}
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

class CameraPanRightCommand : public CameraVerb
{
public:
	CameraPanRightCommand(Workspace* workspace) : CameraVerb("CameraPanRight", workspace)	{}
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};


class CameraZoomInCommand : public CameraVerb
{
public:
	CameraZoomInCommand(Workspace* workspace) : CameraVerb("CameraZoomIn", workspace)	{}
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

class CameraZoomOutCommand : public CameraVerb
{
public:
	CameraZoomOutCommand(Workspace* workspace) : CameraVerb("CameraZoomOut", workspace)	{}
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};


class CameraZoomExtentsCommand : public CameraVerb
{
public:
	CameraZoomExtentsCommand(Workspace* workspace);
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

class CameraCenterCommand : public CameraVerb
{
public:
	CameraCenterCommand(Workspace* workspace);
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};



class FirstPersonCommand : public Verb
{
private:
	DataModel* dataModel;
public:
	FirstPersonCommand(DataModel* dataModel);
	/*override*/ bool isEnabled() const;
	/*override*/ void doIt(IDataState* dataState) {}	
};

class ToggleViewMode : public Verb
{
private:
	DataModel* dataModel;
public:
	ToggleViewMode(RBX::DataModel* dm);

	/*override*/ bool isChecked() const;
	/*override*/ bool isEnabled() const;
	/*override*/ bool isSelected() const;
	/*override*/ void doIt(RBX::IDataState* dataState);
};


/////////////////////////////////////////////////////////////////////

class StatsCommand : public Verb
{
protected:
	DataModel* dataModel;
public:
	StatsCommand(DataModel* dataModel);
	/*override*/ bool isEnabled() const;
	/*override*/ bool isChecked() const;
	/*override*/ void doIt(IDataState* dataState);	
};

class RenderStatsCommand : public Verb
{
protected:
	DataModel* dataModel;
public:
	RenderStatsCommand(DataModel* dataModel);
	/*override*/ bool isEnabled() const;
	/*override*/ bool isChecked() const;
	/*override*/ void doIt(IDataState* dataState);	
};

class SummaryStatsCommand : public Verb
{
protected:
	DataModel* dataModel;
public:
	SummaryStatsCommand(DataModel* dataModel);
	/*override*/ bool isEnabled() const;
	/*override*/ bool isChecked() const;
	/*override*/ void doIt(IDataState* dataState);	
};

class CustomStatsCommand : public Verb
{
protected:
	DataModel* dataModel;
public:
	CustomStatsCommand(DataModel* dataModel);
	/*override*/ bool isEnabled() const;
	/*override*/ bool isChecked() const;
	/*override*/ void doIt(IDataState* dataState);	
};

class NetworkStatsCommand : public Verb
{
protected:
	DataModel* dataModel;

public:
	NetworkStatsCommand(DataModel* dataModel);
	/*override*/ bool isEnabled() const;
	/*override*/ bool isChecked() const;
	/*override*/ void doIt(IDataState* dataState);	
};

class PhysicsStatsCommand : public Verb
{
protected:
	DataModel* dataModel;
public:
	PhysicsStatsCommand(DataModel* dataModel);
	/*override*/ bool isEnabled() const;
	/*override*/ bool isChecked() const;
	/*override*/ void doIt(IDataState* dataState);	
};


class EngineStatsCommand : public Verb
{
private:
	DataModel* dataModel;
public:
	EngineStatsCommand(DataModel* dataModel);
	/*override*/ bool isEnabled() const {return true;}
	/*override*/ void doIt(IDataState* dataState);	
};



class JoinCommand : public Verb
{
protected:
	DataModel* dataModel;
public:
	JoinCommand(DataModel* dataModel);
	/*override*/ bool isEnabled() const;
	/*override*/ void doIt(IDataState* dataState);	
};


class ChatMenuCommand : public Verb
{
private:
	int menu1;
	int menu2;
	int menu3;
public:
	ChatMenuCommand(DataModel* dataModel, int menu1, int menu2, int menu3);
	/*override*/ void doIt(IDataState* dataState) {}

	static std::string getChatString(int menu1, int menu2, int menu3);
};


class MouseCommand;

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//
// TOOL Launchers


template <class MouseCommandClass, class ParentClass = RunStateVerb>
class TToolVerb : public ParentClass
{
private:
	bool toggle;
public:
	TToolVerb(DataModel* dataModel, bool toggle = true, bool blacklisted = true) :
	  ParentClass(
		  MouseCommandClass::name().toString() + "Tool",	// MouseCommand should be a RBX::Named<> descendant, or must implement a static name() function
		  dataModel,
          blacklisted
		 ),
	 toggle(toggle)
	{}
	bool sameType(MouseCommand* mouseCommand) const
	{
		if (DFFlag::UseRemoveTypeIDTricks)
		{
			return mouseCommand ? MouseCommandClass::name() == mouseCommand->getName() : false;
		}
		else
		{
			return mouseCommand ? typeid(MouseCommandClass) == typeid(*mouseCommand) : false;
		}
	}
	void doIt(IDataState* dataState)
	{
		if (isChecked() && toggle) {
			if (!MouseCommand::isAdvArrowToolEnabled())
				ParentClass::dataModel->getWorkspace()->setNullMouseCommand();			// Toggle on / off if already on
			else
				ParentClass::dataModel->getWorkspace()->setDefaultMouseCommand();			// Toggle on / off if already on
		}
		else {
			ParentClass::dataModel->getWorkspace()->setMouseCommand(newMouseCommand());
		}
	}
	bool isChecked() const
	{
		return sameType(ParentClass::dataModel->getWorkspace()->getCurrentMouseCommand());
	}
	virtual shared_ptr<MouseCommand> newMouseCommand()
	{
		return Creatable<MouseCommand>::create<MouseCommandClass>(ParentClass::dataModel->getWorkspace());
	}
};










//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//
// FORMAT MENU

class AnchorVerb : public EditSelectionVerb
{
public:
	AnchorVerb(DataModel* dataModel);
	virtual void doIt(IDataState* dataState);
	virtual bool isChecked() const;
private:
	FilteredSelection<Instance>* m_selection;
};

class MaterialVerb : public EditSelectionVerb
{
public:
	MaterialVerb(DataModel* dataModel, std::string name="MaterialVerb")
		: EditSelectionVerb(name, dataModel) {}
	virtual void doIt(IDataState* dataState);

	static PartMaterial getCurrentMaterial() { return m_currentMaterial; }
	static void setCurrentMaterial(PartMaterial val) { m_currentMaterial = val; }
	static PartMaterial parseMaterial(const std::string materialString);

private:
	static PartMaterial m_currentMaterial;
};

class ColorVerb : public EditSelectionVerb
{
public:
	ColorVerb(DataModel* dataModel, std::string name="ColorVerb")
		: EditSelectionVerb(name, dataModel) {}
	virtual void doIt(IDataState* dataState);

	static RBX::BrickColor getCurrentColor() { return m_currentColor; }
	static void setCurrentColor(RBX::BrickColor val) { m_currentColor = val; }

private:
	static RBX::BrickColor m_currentColor;
};


class TranslucentVerb : public BoolPropertyVerb
{
public:
	TranslucentVerb(DataModel* dataModel) : 
	  BoolPropertyVerb("TranslucentVerb", dataModel, "Transparent") {}
};

class CanCollideVerb : public BoolPropertyVerb
{
public:
	CanCollideVerb(DataModel* dataModel) : 
	  BoolPropertyVerb("CanCollideVerb", dataModel, "CanCollide") {}
};




class AllCanSelectCommand : public RunStateVerb
{
public:
	AllCanSelectCommand(DataModel* dataModel) : 
		RunStateVerb("AllCanSelect", dataModel) {}
	virtual void doIt(IDataState* dataState);
};

class CanNotSelectCommand : public EditSelectionVerb
{
public:
	CanNotSelectCommand(DataModel* dataModel) :
	  EditSelectionVerb("CanNotSelect", dataModel) {}
	virtual void doIt(IDataState* dataState);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//

class MoveUpSelectionVerb : public EditSelectionVerb
{
private:
	float moveUpHeight;
public:
	MoveUpSelectionVerb(DataModel* dataModel, const std::string& name, float moveUpHeight) 
		: EditSelectionVerb(name, dataModel)
		, moveUpHeight(moveUpHeight)
	{}
	/*override*/ void doIt(IDataState* dataState);
};

class MoveUpPlateVerb : public MoveUpSelectionVerb
{
public:
	MoveUpPlateVerb(DataModel* dataModel) 
		: MoveUpSelectionVerb(dataModel, "SelectionUpPlate", 0.4f)
	{}
};

class MoveUpBrickVerb : public MoveUpSelectionVerb
{
public:
	MoveUpBrickVerb(DataModel* dataModel) 
		: MoveUpSelectionVerb(dataModel, "SelectionUpBrick", 1.2f)
	{}
};

class MoveDownSelectionVerb : public EditSelectionVerb
{
public:
	MoveDownSelectionVerb(DataModel* dataModel);
	virtual void doIt(IDataState* dataState);
};
class RotateAxisCommand : public EditSelectionVerb
{
protected:
	RotateAxisCommand(std::string name, DataModel* dataModel) : EditSelectionVerb(name, dataModel) {}
	void rotateAboutAxis(const G3D::Matrix3& rotMatrix, const std::vector<PVInstance*>& selectedInstances);
	virtual G3D::Matrix3 getRotationAxis() = 0;
public:
	virtual void doIt(IDataState* dataState);
};

class RotateSelectionVerb : public RotateAxisCommand
{
public:
	RotateSelectionVerb(DataModel* dataModel);
protected:
	virtual G3D::Matrix3 getRotationAxis();
};
class TiltSelectionVerb : public RotateAxisCommand
{
public:
	TiltSelectionVerb(DataModel* dataModel);
protected:
	virtual G3D::Matrix3 getRotationAxis();
};


class CharacterCommand : public EditSelectionVerb
{
protected:
	CharacterCommand(const std::string& name, DataModel* dataModel) : 
		EditSelectionVerb(name, dataModel) {}
public:
	virtual bool isEnabled() const;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//
// RUN MENU
//
class RunCommand : public RunStateVerb
{
public:
	RunCommand(DataModel* dataModel) : RunStateVerb("Run", dataModel) {}
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

class StopCommand : public RunStateVerb
{
public:
	StopCommand(DataModel* dataModel) : RunStateVerb("Stop", dataModel) {}
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

class ResetCommand : public RunStateVerb
{
public:
	ResetCommand(DataModel* dataModel) : RunStateVerb("Reset", dataModel) {}
	virtual bool isEnabled() const;
	virtual void doIt(IDataState* dataState);
};

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//
// Advanced Build Related
//

class TurnOnManualJointCreation : public Verb
{
private:
	DataModel* dataModel;
public:
	TurnOnManualJointCreation(DataModel* dataModel);
	virtual bool isEnabled() const {return true;}
	virtual bool isChecked() const {return (AdvArrowTool::advCreateJointsMode && AdvArrowTool::advManualJointMode);}
	virtual void doIt(IDataState* dataState);
};

class SetDragGridToOne : public Verb
{
private:
	DataModel* dataModel;
public:
	SetDragGridToOne(DataModel* dataModel);
	virtual bool isEnabled() const {return true;}
	virtual bool isChecked() const {return AdvArrowTool::advGridMode == DRAG::ONE_STUD;}
	virtual void doIt(IDataState* dataState) {AdvArrowTool::advGridMode = DRAG::ONE_STUD;}
};

class SetDragGridToOneFifth : public Verb
{
private:
	DataModel* dataModel;
public:
	SetDragGridToOneFifth(DataModel* dataModel);
	virtual bool isEnabled() const {return true;}
	virtual bool isChecked() const {return AdvArrowTool::advGridMode == DRAG::QUARTER_STUD;}
	virtual void doIt(IDataState* dataState) {AdvArrowTool::advGridMode = DRAG::QUARTER_STUD;}
};

class SetDragGridToOff : public Verb
{
private:
	DataModel* dataModel;
public:
	SetDragGridToOff(DataModel* dataModel);
	virtual bool isEnabled() const {return true;}
	virtual bool isChecked() const {return AdvArrowTool::advGridMode == DRAG::OFF;}
	virtual void doIt(IDataState* dataState) {AdvArrowTool::advGridMode = DRAG::OFF;}
};

class SetGridSizeToTwo : public Verb
{
private:
	Workspace* workspace;
public:
	SetGridSizeToTwo(DataModel* dataModel);
	virtual bool isEnabled() const { return workspace->getShow3DGrid(); }
    virtual bool isChecked() const { return (int)Workspace::gridSizeModifier == 2; }
	virtual void doIt(IDataState* dataState) { Workspace::gridSizeModifier = 2.0f; }
};

class SetGridSizeToFour : public Verb
{
private:
	Workspace* workspace;
public:
	SetGridSizeToFour(DataModel* dataModel);
	virtual bool isEnabled() const { return workspace->getShow3DGrid(); }
    virtual bool isChecked() const { return (int)Workspace::gridSizeModifier == 4; }
	virtual void doIt(IDataState* dataState) { Workspace::gridSizeModifier = 4.0f; }
};

class SetGridSizeToSixteen : public Verb
{
private:
	Workspace* workspace;
public:
	SetGridSizeToSixteen(DataModel* dataModel);
	virtual bool isEnabled() const { return workspace->getShow3DGrid(); }
    virtual bool isChecked() const { return (int)Workspace::gridSizeModifier == 16; }
	virtual void doIt(IDataState* dataState) { Workspace::gridSizeModifier = 16.0f; }
};

class SetManualJointToWeak : public Verb
{
private:
	DataModel* dataModel;
public:
	SetManualJointToWeak(DataModel* dataModel);
	virtual bool isEnabled() const {return false;}
	virtual bool isChecked() const {return false;}
	virtual void doIt(IDataState* dataState) {AdvArrowTool::advManualJointType = DRAG::WEAK_MANUAL_JOINT;}
};

class SetManualJointToStrong : public Verb
{
private:
	DataModel* dataModel;
public:
	SetManualJointToStrong(DataModel* dataModel);
	virtual bool isEnabled() const {return AdvArrowTool::advManualJointMode;}
	virtual bool isChecked() const {return AdvArrowTool::advManualJointType == DRAG::STRONG_MANUAL_JOINT;}
	virtual void doIt(IDataState* dataState) {AdvArrowTool::advManualJointType = DRAG::STRONG_MANUAL_JOINT;}
};

class SetManualJointToInfinite : public Verb
{
private:
	DataModel* dataModel;
public:
	SetManualJointToInfinite(DataModel* dataModel);
	virtual bool isEnabled() const {return false;}
	virtual bool isChecked() const {return false;}
	virtual void doIt(IDataState* dataState) {AdvArrowTool::advManualJointType = DRAG::INFINITE_MANUAL_JOINT;}
};


} // namespace
