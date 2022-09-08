/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Hopper.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/GuiBuilder.h"
#include "V8Tree/Verb.h"
#include "AppDraw/DrawPrimitives.h"
#include "V8DataModel/Mouse.h"
#include "V8DataModel/ScriptMouseCommand.h"
#include "V8DataModel/Workspace.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "V8DataModel/ContentProvider.h"
#include "Gui/ProfanityFilter.h"

namespace RBX {

const char *const sBackpackItem = "BackpackItem";

// Two item types: HopperBin and Tool
const char *const sHopperBin = "HopperBin";

const char *const sStarterPackService = "StarterPack"; 

const char *const sLegacyHopperService = "Hopper"; // legacy name, not necessarily descriptive or apt.

const char *const sStarterGear = "StarterGear"; 

namespace Reflection {
template<>
EnumDesc<HopperBin::BinType>::EnumDesc()
	:EnumDescriptor("BinType")
{
	addPair(HopperBin::SCRIPT_BIN, "Script");
	addPair(HopperBin::GAME_TOOL, "GameTool");
	addPair(HopperBin::GRAB_TOOL, "Grab");
	addPair(HopperBin::CLONE_TOOL, "Clone");
	addPair(HopperBin::HAMMER_TOOL, "Hammer");
	addLegacy(5, "Slingshot", HopperBin::SCRIPT_BIN);
	addLegacy(6, "Rocket", HopperBin::SCRIPT_BIN);
	addLegacy(7, "Laser", HopperBin::SCRIPT_BIN);
}
}//namespace Reflection

REFLECTION_BEGIN();
// Backpack
static Reflection::PropDescriptor<BackpackItem, TextureId> desc_TextureId("TextureId", category_Data, &BackpackItem::getTextureId, &BackpackItem::setTextureId);

// Not legacy - Hopper Bin
static Reflection::EnumPropDescriptor<HopperBin, HopperBin::BinType> desc_BinType("BinType", category_Data, &HopperBin::getBinType, &HopperBin::setBinType);

//Used to convey the Active/Deactivated signal
static Reflection::BoundProp<bool> desc_Active("Active", category_Data, &HopperBin::active, &HopperBin::dataChanged);

static Reflection::EventDesc<HopperBin, void(shared_ptr<Instance>), rbx::remote_signal<void(shared_ptr<Instance>)> > desc_Selected(&HopperBin::selectedSignal, "Selected", "mouse");
static Reflection::RemoteEventDesc<HopperBin, void()> desc_ReplicatedSelected(&HopperBin::replicatedSelectedSignal, "ReplicatedSelected", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::EventDesc<HopperBin, void()> desc_Deselected(&HopperBin::deselectedSignal, "Deselected");

static const Reflection::BoundFuncDesc<HopperBin,void()> func_toggleSelect(&HopperBin::onLocalClicked,"ToggleSelect", Security::RobloxScript);
static const Reflection::BoundFuncDesc<HopperBin,void()> func_disable(&HopperBin::disable,"Disable", Security::RobloxScript);

// Legacy - This command string is now legacy, replaced with BinType - 
static Reflection::PropDescriptor<HopperBin, std::string> desc_legacyCommand("Command", category_Data, NULL, &HopperBin::setLegacyCommand, Reflection::PropertyDescriptor::LEGACY);
static Reflection::PropDescriptor<HopperBin, std::string> desc_legacyTextureName("TextureName", category_Data, NULL, &HopperBin::setLegacyTextureName, Reflection::PropertyDescriptor::LEGACY);
REFLECTION_END();


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
StarterGear::StarterGear() 
{
	Instance::setName("StarterGear");
}

bool StarterGear::askSetParent(const Instance* instance) const
{
	return false /*(Instance::fastDynamicCast<RBX::Network::Player>(instance) != NULL)*/;
}

bool StarterGear::askAddChild(const Instance* instance) const
{
	return (Instance::fastDynamicCast<BackpackItem>(instance)!=NULL);
}


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void BackpackItem::setName(const std::string& value)
{
	if (!ProfanityFilter::ContainsProfanity(value))
		Instance::setName(value);	
}

int BackpackItem::getBinId() const
{
	if (const Instance* parent = getParent()) {
		return parent->findChildIndex(this);
	}
	else {
		RBXASSERT(0);
		return -1;
	}
}

void BackpackItem::setTextureId(const TextureId& value)
{
	if (value != textureId) {
		textureId = value;

		raisePropertyChanged(desc_TextureId);
	}
}

const TextureId BackpackItem::getTextureId() const
{
	return textureId;
}

bool BackpackItem::inBackpack()
{
	return Instance::fastDynamicCast<Backpack>(this->getParent()) != NULL;
}

bool BackpackItem::askAddChild(const Instance* instance) const
{
	return true;	// Anything is OK as a child
}

bool BackpackItem::askSetParent(const Instance* instance) const
{
	return true;
}

Vector2 BackpackItem::getSize(Canvas canvas) const
{
	float width = canvas.toPixelSize(Vector2(10, 10)).x;
	return Vector2(width, width);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////



HopperBin::HopperBin() 
	: binType(SCRIPT_BIN)
	, active(false)
	, replicationInitialized(false)
{
	Instance::setName("HopperBin");
}

void HopperBin::selectedConnectionShimFunction()
{
	onSelectScript();
}

void HopperBin::reverseSelectedConnectionShimFunction(shared_ptr<Instance>& instance)
{
	desc_ReplicatedSelected.replicateEvent(this);
}



void HopperBin::onAncestorChanged(const AncestorChanged& event)
{
	const ServiceProvider* newProvider = ServiceProvider::findServiceProvider(event.newParent);

	Super::onAncestorChanged(event);

	if(newProvider)
	{
		if(Workspace::serverIsPresent(this)){
			if(!replicationInitialized){
				//We're server, so replicate from our SHIM to the real event
				RBXASSERT(!selectedConnectionShim.connected());
				selectedConnectionShim = replicatedSelectedSignal.connect(boost::bind(&HopperBin::selectedConnectionShimFunction,this));
				
				replicationInitialized = true;
			}
		}
		else{
			if(!replicationInitialized){
				//We're client, so replicate from the real event to our SHIM
				RBXASSERT(!selectedConnectionShim.connected());
				selectedConnectionShim = selectedSignal.connect(boost::bind(&HopperBin::reverseSelectedConnectionShimFunction,this, _1));

				replicationInitialized = true;
			}
		}
	}
}

void HopperBin::onSelectScript()
{
	if(Workspace* workspace = ServiceProvider::find<Workspace>(this)){
		{
			//We're running on the client
			shared_ptr<ScriptMouseCommand> scriptMouseCommand = Creatable<MouseCommand>::create<ScriptMouseCommand>(workspace);
			workspace->setMouseCommand(scriptMouseCommand);
			selectedSignal(scriptMouseCommand->getMouse());
		}
	}
}

void HopperBin::dataChanged(const Reflection::PropertyDescriptor& prop) 
{
}

void HopperBin::onSelectCommand()
{
	RBXASSERT(binType != SCRIPT_BIN);
	
	DataModel* dataModel = rbx_static_cast<DataModel*>(getRootAncestor());

	std::string command = Reflection::EnumDesc<BinType>::singleton().convertToString(binType) + "Tool";

	Verb* verb = dataModel->getVerb(command);

	RBXASSERT(verb);

	if (verb && verb->isEnabled()) 
	{
		if (Workspace* workspace = ServiceProvider::find<Workspace>(this))
		{
			verb->doIt(&workspace->getDataState());
		}
		else
		{
			RBXASSERT(0);
		}
	}
}

// This is happening local only
void HopperBin::onLocalClicked()
{
	if (!isEnabled())
		return;

	if (!active)
	{
		active = true;
		raisePropertyChanged(desc_Active);
		if (binType == SCRIPT_BIN)
		{
			onSelectScript();
		}
		else
		{
			onSelectCommand();
		}
	}
	else
	{
		onLocalOtherClicked();
	}
}

void HopperBin::onLocalOtherClicked()
{
	disable();
}

void HopperBin::disable()
{
	if (active)
	{
		if (binType == SCRIPT_BIN)
		{
			deselectedSignal();
		}
		active = false;
		raisePropertyChanged(desc_Active);
		if (Workspace* workspace = ServiceProvider::find<Workspace>(this))
		{
			workspace->setDefaultMouseCommand();
		}
	}
}

void HopperBin::setBinType(const BinType value)
{
	if (value != binType) {
		binType = value;
		raisePropertyChanged(desc_BinType);
		if (binType != SCRIPT_BIN) 
		{
			std::string textureName = Reflection::EnumDesc<BinType>::singleton().convertToString(binType);
			setLegacyTextureName(textureName);
		}
	}
}

// LEGACY - 1/15/07 approximately
void HopperBin::setLegacyCommand(const std::string& text)
{
	BinType newBinType;
	if (!Reflection::EnumDesc<BinType>::singleton().convertToValue(text.c_str(), newBinType)) 
	{
		newBinType = SCRIPT_BIN;
	}
	setBinType(newBinType);
}


void HopperBin::setLegacyTextureName(const std::string& value)
{
	std::string asset = "Textures/" + value + ".png";
	TextureId textureId = ContentId::fromAssets(asset.c_str());
	setTextureId(textureId);
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

Hopper::Hopper()
{
	yLocation = Rect::BOTTOM;		// relative panel
	xLocation = Rect::CENTER;
}

bool Hopper::askSetParent(const Instance* instance) const
{
	return true;
}

bool Hopper::askAddChild(const Instance* instance) const
{
	return (Instance::fastDynamicCast<BackpackItem>(instance)!=NULL);
}

////////////////////////////////////////////////////////////////////////////////////
    
StarterPackService::StarterPackService()
{
	setName(sStarterPackService);
}
    
////////////////////////////////////////////////////////////////////////////////////

LegacyHopperService::LegacyHopperService() 
{
	setName(sLegacyHopperService);
}

// Check - all children should be given to the StarterPackService before deletion
LegacyHopperService::~LegacyHopperService()
{
	RBXASSERT(this->numChildren() == 0);
}

// Assumes that children will be read and hooked up before the onServiceProvider call is made
void LegacyHopperService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		StarterPackService* starterPack = ServiceProvider::find<StarterPackService>(newProvider);
		RBXASSERT(starterPack);

		while (numChildren() > 0)
		{
			getChild(0)->setParent(starterPack);
		}

		this->setParent(NULL);		// delete myself
	}
}

}	// namespace
