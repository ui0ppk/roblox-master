/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/GuiBuilder.h"
#include "V8DataModel/Commands.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/SafeChat.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/PhysicsSettings.h"
#include "V8Tree/Verb.h"
#include "Network/Players.h"
#include "Network/../../NetworkSettings.h"
#include "Util/Color.h"
#include "Util/FileSystem.h"
#include "Util/profiling.h"

#include "Gui/GUI.h"
#include "Gui/EquationDisplay.h"
#include "Gui/ChatOutput.h"
#include "Gui/ChatWidget.h"
#include "Humanoid/Humanoid.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "Script/ScriptContext.h"
#include "V8DataModel/Frame.h"
#include "V8DataModel/TextLabel.h"
#include "V8DataModel/ScreenGui.h"
#include "V8DataModel/ImageLabel.h"
#include "V8DataModel/ImageButton.h"
#include "V8DataModel/PlayerGui.h"

#include "SimpleJSON.h"
#include "v8datamodel/Stats.h"

FASTFLAGVARIABLE(DebugDisplayFPS, false)
FASTFLAGVARIABLE(LuaBasedBubbleChat, false)

using namespace G3D;

namespace RBX {

static GuiBuilder::Display gDebugDisplay = GuiBuilder::DISPLAY_NONE;

static boost::filesystem::path GetCustomStatsFilename()
{
	return RBX::FileSystem::getUserDirectory(false, RBX::DirExe, "ClientSettings") / "StatDisplaySettings.json";
}

class CustomStatsGuiJSON : public SimpleJSON
{
public:
	CustomStatsGuiJSON(GuiBuilder* guiBuilder);

	virtual bool DefaultHandler(const std::string& valueName, const std::string& valueData);

	void WriteFile();

private:
	CustomStatsGuiJSON() {};

	GuiBuilder* mGuiBuilder;
};

CustomStatsGuiJSON::CustomStatsGuiJSON(GuiBuilder* guiBuilder)
: mGuiBuilder(guiBuilder)
{
	// this space intentionally left blank
}

bool CustomStatsGuiJSON::DefaultHandler(const std::string& valueName, const std::string& valueData)
{
	shared_ptr<TextDisplay> item = Creatable<Instance>::create<EquationDisplay>(valueName + " : ", valueData);

	GuiBuilder::Data data;
	data.stat = valueData;
	data.item = item;

	mGuiBuilder->customStatsCont.insert(make_pair(valueName, data));

	return true;
}

void CustomStatsGuiJSON::WriteFile()
{
	std::ofstream localConfigFile(GetCustomStatsFilename().native().c_str());

	if(localConfigFile.is_open())
	{
		std::stringstream localConfigBuffer;
		localConfigFile.write("{\n", 2);

		for (GuiBuilder::CustomStatsContainer::iterator itr = mGuiBuilder->customStatsCont.begin(); itr != mGuiBuilder->customStatsCont.end(); ++itr)
		{
			std::string output = "\t\"" + itr->first + "\": \"" + itr->second.stat + "\",\n";
			localConfigFile.write(output.c_str(), output.length());
		}
		localConfigFile.write("}", 1);
	}
}

GuiBuilder::Display GuiBuilder::getDebugDisplay()
{
    return gDebugDisplay;
}

void GuiBuilder::setDebugDisplay(Display display)
{
    gDebugDisplay = display;
}

Verb* GuiBuilder::getWhitelistVerb(const std::string& name)
{
    RBXASSERT(workspace);
    Verb* answer = workspace->getWhitelistVerb(name);
    return answer;
}

void GuiBuilder::buildSimpleStatsOutput(shared_ptr<Instance> instance, std::string* output)
{
	if (Stats::Item* item = instance->fastDynamicCast<Stats::Item>())
	{
		std::string itemName = item->getName();
		if (itemName.size()<15)
		{
			for (size_t i=0; i<15-itemName.size(); i++)
			{
				itemName+=" ";
			}
		}
		(*output) += RBX::format("\n%s: %.2f", itemName.c_str(), item->getValue());
	}
}

void GuiBuilder::buildNetworkStatsOutput(shared_ptr<Instance> instance, std::string* output)
{
    if (Stats::Item* item = instance->fastDynamicCast<Stats::Item>())
    {
        std::string itemName = item->getName();
        if (itemName.size()<15)
        {
            for (size_t i=0; i<15-itemName.size(); i++)
            {
                itemName+=" ";
            }
        }
        float numPacket = item->getValue();
        float avgSize = item->findFirstChildByName("Size")->fastDynamicCast<Stats::Item>()->getValue();
        float kBps = numPacket * avgSize / 1000.f;
        (*output) += RBX::format("\n%s: %.2f, %.2fB, %.2f", itemName.c_str(), kBps, avgSize, numPacket);
    }
}
    
void GuiBuilder::toggleGeneralStats()
{
	FASTLOG(FLog::Verbs, "Gui:Stats");
    
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("StatsHud1"));
	if (statsMenu!=NULL)
		statsMenu->setVisible(!statsMenu->isVisible());
	statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("StatsHud2"));
	if (statsMenu!=NULL)
		statsMenu->setVisible(!statsMenu->isVisible());
}
    
void GuiBuilder::toggleRenderStats()
{
    FASTLOG(FLog::Verbs, "Gui:RenderStats");
    
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("RenderStats"));
	if (statsMenu!=NULL)
	{
		statsMenu->setVisible(!statsMenu->isVisible());
		if (FFlag::DebugDisplayFPS)
		{
			TopMenuBar* fpsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("FPS"));
			if (fpsMenu!=NULL)
				fpsMenu->setVisible(!statsMenu->isVisible());
		}
	}
}
    
void GuiBuilder::toggleNetworkStats()
{
    FASTLOG(FLog::Verbs, "Gui:NetworkStats");
    
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("NetworkStats"));
	if (statsMenu!=NULL)
		statsMenu->setVisible(!statsMenu->isVisible());
	statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("NetworkStats2"));
	if (statsMenu!=NULL)
	{
		statsMenu->setVisible(!statsMenu->isVisible());
        
        dataModel->setNetworkStatsWindowsOn(statsMenu->isVisible());
		if (statsMenu->isVisible())
		{
			oldTrackDataTypesValue = NetworkSettings::singleton().trackDataTypes;
            oldTrackPhysicsDetailsValue = NetworkSettings::singleton().trackPhysicsDetails;
			NetworkSettings::singleton().trackDataTypes = true;
            NetworkSettings::singleton().trackPhysicsDetails = true;
		}
		else
        {
			NetworkSettings::singleton().trackDataTypes = oldTrackDataTypesValue;
            NetworkSettings::singleton().trackPhysicsDetails = oldTrackPhysicsDetailsValue;
        }
	}
}

void GuiBuilder::togglePhysicsStats()
{
    FASTLOG(FLog::Verbs, "Gui:PhysicsStats");
    
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("PhysicsStats"));
	if (statsMenu!=NULL)
		statsMenu->setVisible(!statsMenu->isVisible());
    
	statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("PhysicsStats2"));
	if (statsMenu!=NULL)
	{
		statsMenu->setVisible(!statsMenu->isVisible());
		RBX::Profiling::setEnabled(statsMenu->isVisible());
	}
}
    
void GuiBuilder::toggleSummaryStats()
{
    FASTLOG(FLog::Verbs, "Gui:SummaryStats");
    
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("SummaryStats"));
    if (statsMenu)
	    statsMenu->setVisible(!statsMenu->isVisible());
}
    
void GuiBuilder::toggleCustomStats()
{
    FASTLOG(FLog::Verbs, "Gui:CustomStats");
    
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("CustomStats"));
	if (statsMenu)
        statsMenu->setVisible(!statsMenu->isVisible());
}
    


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//
//  TOP LEVEL - BUILD THE GUI
//
void GuiBuilder::buildGui(
	Workspace* _workspace,
	bool buildInGameGui)
{
	Vector2 size;

	workspace = _workspace;
	
	dataModel->getGuiRoot()->addGuiItem(buildStatsHud1());
	dataModel->getGuiRoot()->addGuiItem(buildStatsHud2());
	dataModel->getGuiRoot()->addGuiItem(buildRenderStats());
	dataModel->getGuiRoot()->addGuiItem(buildNetworkStats());
	dataModel->getGuiRoot()->addGuiItem(buildNetworkStats2(true));
	dataModel->getGuiRoot()->addGuiItem(buildPhysicsStats());
	dataModel->getGuiRoot()->addGuiItem(buildPhysicsStats2());
    dataModel->getGuiRoot()->addGuiItem(buildFPS());
    dataModel->getGuiRoot()->addGuiItem(buildSummaryStats());
	dataModel->getGuiRoot()->addGuiItem(buildCustomStats());
    
	if (gDebugDisplay == DISPLAY_FPS)
	{
        TopMenuBar* fps = Instance::fastDynamicCast<TopMenuBar>(dataModel->getGuiRoot()->findFirstChildByName("FPS"));
        RBXASSERT(fps != NULL);
		if (fps)
        {
            fps->setVisible(true);
        }
	}
    
    if (gDebugDisplay == DISPLAY_PHYSICS || gDebugDisplay == DISPLAY_PHYSICS_AND_OWNER)
	{
        TopMenuBar* physicsStats = Instance::fastDynamicCast<TopMenuBar>(dataModel->getGuiRoot()->findFirstChildByName("PhysicsStats"));
        RBXASSERT(physicsStats != NULL);
		if (physicsStats)
        {
            physicsStats->setVisible(true);
        }
		TopMenuBar* physicsStats2 = Instance::fastDynamicCast<TopMenuBar>(dataModel->getGuiRoot()->findFirstChildByName("PhysicsStats2"));
		RBXASSERT(physicsStats2 != NULL);
		if (physicsStats2)
		{
			physicsStats2->setVisible(true);
			RBX::Profiling::setEnabled(physicsStats2->isVisible());
		}
	}
    
    if (gDebugDisplay == DISPLAY_PHYSICS_AND_OWNER)
    {
        RBX::PhysicsSettings &ps = RBX::PhysicsSettings::singleton();
        ps.setShowEPhysicsOwners(!ps.getShowEPhysicsOwners());
    }
    
    if (gDebugDisplay == DISPLAY_SUMMARY)
    {
        TopMenuBar* summaryStats = Instance::fastDynamicCast<TopMenuBar>(dataModel->getGuiRoot()->findFirstChildByName("SummaryStats"));
        RBXASSERT(summaryStats != NULL);
		if (summaryStats)
        {
            summaryStats->setVisible(true);
        }
    }

    if (gDebugDisplay == DISPLAY_RENDER)
	{
        TopMenuBar* renderStats = Instance::fastDynamicCast<TopMenuBar>(dataModel->getGuiRoot()->findFirstChildByName("RenderStats"));
        RBXASSERT(renderStats != NULL);
		if (renderStats)
        {
            renderStats->setVisible(true);
        }
	}


	if(buildInGameGui)
	{
		if (shared_ptr<TopMenuBar> topMenuBar = buildChatHud())
		{
			dataModel->getGuiRoot()->addGuiItem(topMenuBar);
		}

		dataModel->getGuiRoot()->addGuiItem(buildRightPalette());
		buildLuaGui();
	}
}

void GuiBuilder::Initialize(DataModel* _dataModel)
{
	dataModel = _dataModel;
}

// lua gui into CoreGuiService
void GuiBuilder::buildLuaGui()
{
	//////////////////////////////////// TOP LEVEL LUA GUI ////////////////////////////////////////
	shared_ptr<Frame> controls = Creatable<Instance>::create<Frame>();
	controls->setName("ControlFrame");
	controls->setBackgroundTransparency(1);
	controls->setSize(UDim2(1,0,1,0));
	controls->setRobloxLocked(true);

	shared_ptr<Frame> bottomLeftControl = Creatable<Instance>::create<Frame>();
	bottomLeftControl->setSize(UDim2(0,130,0,46));
	bottomLeftControl->setPosition(UDim2(0,0,1,-46));
	bottomLeftControl->setBackgroundTransparency(1);
	bottomLeftControl->setName("BottomLeftControl");
	bottomLeftControl->setRobloxLocked(true);

	shared_ptr<Frame> bottomRightControl = Creatable<Instance>::create<Frame>();
	bottomRightControl->setSize(UDim2(0,180,0,41));
	bottomRightControl->setPosition(UDim2(1,-180,1,-41));
	bottomRightControl->setBackgroundTransparency(1);
	bottomRightControl->setName("BottomRightControl");
	bottomRightControl->setRobloxLocked(true);

	shared_ptr<Frame> topLeftControl = Creatable<Instance>::create<Frame>();
	topLeftControl->setSize(UDim2(0.05f,0,0.05f,0));
	topLeftControl->setBackgroundTransparency(1);
	topLeftControl->setName("TopLeftControl");
	Instance::propRobloxLocked.set(topLeftControl.get(),true);
    

	//////////////////////// COREGUI SETUP //////////////////////////////////////

	// Add gui into CoreGuiService
	shared_ptr<CoreGuiService> coreGuiService = shared_from(dataModel->find<CoreGuiService>());
	
	bottomLeftControl->setParent(controls.get());
	bottomRightControl->setParent(controls.get());
	topLeftControl->setParent(controls.get());

	coreGuiService->addChild(controls.get());
}
    
void GuiBuilder::updateGui()
{
    TopMenuBar* summaryStats = Instance::fastDynamicCast<TopMenuBar>(dataModel->getGuiRoot()->findFirstChildByName("SummaryStats"));
    if (summaryStats && summaryStats->isVisible())
    {
        updateSummaryStats(summaryStats);
    }
}

void GuiBuilder::addCustomStat(const std::string& name, const std::string& value)
{
	CustomStatsContainer::iterator itr = customStatsCont.find(name);
	if (itr == customStatsCont.end())
	{
 		TopMenuBar* customStats = Instance::fastDynamicCast<TopMenuBar>(dataModel->getGuiRoot()->findFirstChildByName("CustomStats"));
 		if (customStats)
		{
			shared_ptr<TextDisplay> item = Creatable<Instance>::create<EquationDisplay>(name + " : ", value);
			Data data;
			data.stat = value;
			data.item = item;
			customStatsCont.insert(make_pair(name, data));
			customStats->addGuiItem(item);

			item->setFontSize(12);
			item->setFontColor(G3D::Color3::purple());
			item->setBorderColor(G3D::Color4::clear());
			item->setGuiSize(Vector2(440, 22));
		}
	}
}

void GuiBuilder::removeCustomStat(const std::string& name)
{
	CustomStatsContainer::iterator itr = customStatsCont.find(name);
	if (itr != customStatsCont.end())
	{
		TopMenuBar* customStats = Instance::fastDynamicCast<TopMenuBar>(dataModel->getGuiRoot()->findFirstChildByName("CustomStats"));
		if (customStats)
		{
			shared_ptr<TextDisplay> item = itr->second.item;

			item->setParent(NULL);
			item->setVisible(false);

			customStatsCont.erase(itr);
		}
	}
}

void GuiBuilder::saveCustomStats()
{
	CustomStatsGuiJSON json(this);
	json.WriteFile();
}

shared_ptr<TopMenuBar> GuiBuilder::buildRightPalette()
{
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::RIGHT;
	layout.yLocation = Rect::BOTTOM;
	shared_ptr<RelativePanel> rightPalette = Creatable<Instance>::create<RelativePanel>(layout);
	rightPalette->setName("RightPalette");

	return rightPalette;
}

shared_ptr<TopMenuBar> GuiBuilder::buildChatHud()
{
	if (!FFlag::LuaBasedBubbleChat)
	{
		Layout layout;
		layout.layoutStyle = Layout::VERTICAL;
		layout.xLocation = Rect::LEFT;
		layout.yLocation = Rect::TOP;
		layout.offset.x = 15;
		layout.offset.y = 100;

		shared_ptr<RelativePanel> chatHud = Creatable<Instance>::create<RelativePanel>(layout);
		chatHud->setName("ChatHud");
		chatHud->addGuiItem( Creatable<Instance>::create<ChatOutput>());
		return chatHud;
	}

	return shared_ptr<RelativePanel>();
}

void GuiBuilder::removeSafeChatMenu()
{
	if(safeChatMenu)
		safeChatMenu->setParent(NULL);
}
	
void GuiBuilder::addSafeChatMenu()
{
	if(safeChatMenu && dataModel)
		safeChatMenu->setParent(dataModel->getGuiRoot());
}

void GuiBuilder::nextNetworkStats()
{
    int counter = (int)networkStatsCounter;
    counter++;
    networkStatsCounter = (NetworkStats)counter;
    if (networkStatsCounter >= NETWORKSTATS_COUNT)
    {
        networkStatsCounter = NETWORKSTATS_FIRST;
    }
    GuiItem* item = Instance::fastDynamicCast<GuiItem>(dataModel->getGuiRoot()->findFirstChildByName("NetworkStats2"));
    if (item)
    {
        item->setParent(NULL);
    }
    dataModel->getGuiRoot()->addGuiItem(buildNetworkStats2(false));
}

void GuiBuilder::buildChatMenu(ChatOption *cur, std::string code, shared_ptr<UnifiedWidget> parentWidget)
{
	// depth-first recursive construction of the templated chat menu tree
	for(unsigned int i = 0; i < cur->children.size(); i++)
	{
		// Distinguish between regular nodes and the ROOT chat option node, which is first
		// and for which we don't create an option.
		ChatOption *child = cur->children[i];
		std::ostringstream strstream;
		strstream << code << ' ' << i;
		std::string myCode = strstream.str();
		shared_ptr<ChatWidget> menu = Creatable<Instance>::create<ChatWidget>(child->text, myCode);
		parentWidget->addGuiItem(menu);
		buildChatMenu(child, myCode , menu);
		
	}

}

shared_ptr<TopMenuBar> GuiBuilder::buildChatMenu()
{
    Layout layout;
    layout.layoutStyle = Layout::VERTICAL;
    layout.xLocation = Rect::LEFT;
    layout.yLocation = Rect::TOP;
    layout.offset.x = 15;
    layout.offset.y = 130;

    shared_ptr<RelativePanel> chatMenuPanel = Creatable<Instance>::create<RelativePanel>(layout);
    chatMenuPanel->setName("ChatMenuPanel");

    shared_ptr<ChatButton> button = Creatable<Instance>::create<ChatButton>("Chat", GuiDrawImage::NORMAL | GuiDrawImage::DOWN | GuiDrawImage::DISABLE | GuiDrawImage::HOVER);
    chatMenuPanel->addGuiItem(button);

    ChatOption *cur = SafeChat::singleton().getChatRoot();

    std::string code = "";

    buildChatMenu(cur, code, button);

    return chatMenuPanel;
}


shared_ptr<TopMenuBar> GuiBuilder::buildStatsHud1()
{
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::LEFT;
	layout.yLocation = Rect::BOTTOM;
	layout.offset.x = 50;
	layout.offset.y = 40;
	layout.backdropColor = GuiItem::translucentDebugBackdrop();

	shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);
	hudArray->setName("StatsHud1");

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- World ----"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Instances     : ", "numInstances"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Physics Mode  : ", "physicsMode"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Primitives    : ", "numPrimitives"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Moving Prims  : ", "numMovingPrimitives"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Joints        : ", "numJoints"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Contacts      : ", "numContacts"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Tree          : ", "maxTreeDepth"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Timing -----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Physics       : ", "Physics"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Mouse Move    : ", "MouseMove"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Render        : ", "Render"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Delta bw render   : ", "Delta Between Renders"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Total Render   : ", "Total Render"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Render Nom     : ", "Render Nominal FPS"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Graphics ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Graphic Mode  : ", "Graphics Mode"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Video Memory  : ", "Video Memory"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Anti-Aliasing : ", "Anti-Aliasing"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Frame Id      : ", "drawId"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("World Id      : ", "worldId"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Throttles ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("EnviroSpeed % : ", "environmentSpeed"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM	          : ", "FRM"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Target  : ", "FRM Target"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Visible   : ", "FRM Visible"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Quality   : ", "FRM Quality"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Network ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("RequestQueue  : ", "RequestQueueSize"));

	for (size_t i = 0; i < hudArray->numChildren(); ++i) {
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(12);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(310, 22));
	}
	hudArray->setVisible(false);
	return hudArray;
}

shared_ptr<TopMenuBar> GuiBuilder::buildRenderStats()
{
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::LEFT;
	layout.yLocation = Rect::BOTTOM;
	layout.offset.x = 50;
	layout.offset.y = 40;
	layout.backdropColor = GuiItem::translucentDebugBackdrop();

	shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);
	hudArray->setName("RenderStats");

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Graphics ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Physics       : ", "Physics"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Heartbeat     : ", "Heartbeat"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Network send  : ", "Network Send"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Network receive : ", "Network Receive"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Video Memory  : ", "Video Memory"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Resolution: ", "RenderStatsResolution"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Performance -----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Config: ", "RenderStatsFRMConfig"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Blocks: ", "RenderStatsFRMBlocks"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Adjust: ", "RenderStatsFRMAdjust"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Target Time: ", "RenderStatsFRMTargetTime"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Distance: ", "RenderStatsFRMDistance"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Framerate Variance: ", "FRM Variance"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Timing -----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Scheduler Render        : ", "Render"));
	
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("CPU: ", "RenderStatsTimeTotal"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("GPU: ", "RenderStatsGPU"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Delta: ", "RenderStatsDelta"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (total): ", "RenderStatsPassTotal"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (scene): ", "RenderStatsPassScene"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (shadow): ", "RenderStatsPassShadow"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (UI): ", "RenderStatsPassUI"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (3D Adorns): ", "RenderStatsPass3dAdorns"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Light grid: ", "RenderStatsLightGrid"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Geometry Gen: ", "RenderStatsGeometryGen"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Clusters: ", "RenderStatsClusters"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Textures: ", "RenderStatsTM"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("TC: ", "RenderStatsTC"));

	for (size_t i = 0; i < hudArray->numChildren(); ++i) {
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(12);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(440, 22));
	}
	hudArray->setVisible(false);
	return hudArray;
}

shared_ptr<TopMenuBar> GuiBuilder::buildNetworkStats()
{
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::LEFT;
	layout.yLocation = Rect::BOTTOM;
	layout.offset.x = 50;
	layout.offset.y = 40;
	layout.backdropColor = GuiItem::translucentDebugBackdrop();

	shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);
	hudArray->setName("NetworkStats");

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- HTTP ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Avg Time In Queue: "  , "HttpTimeInQueue"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Avg Process Time: "  , "HttpProcessTime"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Request Queue Size: "  , "RequestQueueSize"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("# slow requests: "  , "HttpSlowReq"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Replicator ----"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "General  (MTU Size, Ping) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "GeneralStats"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Incoming ----"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "Overall (KB/s, Pkt/s, Queue) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "IncomingStats"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "In Data (KB/s, Pkt/s, Avg Size) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "InData"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "In Physics (KB/s, Pkt/s, Avg Size, Avg Lag) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "InPhysics"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "In Touches (KB/s, Pkt/s, Avg Size) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "InTouches"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "In Clusters (KB/s, Pkt/s, Avg Size) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "InClusters"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Outgoing ----"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>(                   "Overall (KB/s) :" , "OutgoingStats"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "Out Data (KB/s, Pkt/s, Avg Size, Throttle) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "OutData"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "Out Physics (KB/s, Pkt/s, Avg Size, Throttle) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "OutPhysics"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "Out Touches (KB/s, Pkt/s, Avg Size, #Waiting) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "OutTouches"));
    hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""                   , "Out Clusters (KB/s, Pkt/s, Avg Size) :"));
    hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("              " , "OutClusters"));

	for (size_t i = 0; i < hudArray->numChildren(); ++i) {
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(12);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(420, 22));
	}
	hudArray->setVisible(false);
	return hudArray;
}

shared_ptr<TopMenuBar> GuiBuilder::buildFPS()
{
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::LEFT;
	layout.yLocation = Rect::TOP;
	layout.offset.x = 50;
	layout.offset.y = 40;
	layout.backdropColor = GuiItem::translucentDebugBackdrop();

	shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);
	hudArray->setName("FPS");

	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FPS              : ", "Render"));

	for (size_t i = 0; i < hudArray->numChildren(); ++i) {
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(24);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(400, 22));
	}
	hudArray->setVisible(FFlag::DebugDisplayFPS);
	return hudArray;
}

shared_ptr<TopMenuBar> GuiBuilder::buildNetworkStats2(bool init)
{
    if (init)
    {
        networkStatsCounter = NETWORKSTATS_INVALID;
    }
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::RIGHT;
	layout.yLocation = Rect::BOTTOM;
	layout.offset.x = 50;
	layout.offset.y = 40;
	layout.backdropColor = GuiItem::translucentDebugBackdrop();

	shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);
	hudArray->setName("NetworkStats2");

    switch (networkStatsCounter)
    {
    case NETWORKSTATS_RAKNET:
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Raknet ----"));
		hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("ping : ", "RakNetPing"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("msgTotalBytesPushed/s : ", "messageDataBytesSentPerSec"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("msgTotalBytesSent/s : ", "messageTotalBytesSentPerSec"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("msgDataBytesResent/s : ", "messageDataBytesResentPerSec"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("msgBytesRecv/s : ", "messagesBytesReceivedPerSec"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("msgBytesRecvAndIgnored/s : ", "messagesBytesReceivedAndIgnoredPerSec"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("bytesSent/s : ", "bytesSentPerSec"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("bytesReceived/s : ", "bytesReceivedPerSec"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("outBandwidthLimitBytes/s : ", "outgoingBandwidthLimitBytesPerSecond"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("limitedByOutBandwidthLimit : ", "isLimitedByOutgoingBandwidthLimit"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("congestionCtrlLimitBytes/s : ", "congestionControlLimitBytesPerSecond"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("limitedByCongestionControl : ", "isLimitedByCongestionControl"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("messagesInResendQueue : ", "messagesInResendQueue"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("bytesInResendQueue : ", "bytesInResendQueue"));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("packetlossLastSecond : ", "packetlossLastSecond"));
        break;
    case NETWORKSTATS_PHYSICS:
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       , "----- In Physics ----"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       , "Class) Type (KB/s, Avg Size, Count/s)"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>(""   , "InPhysicsDetails"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       , "----- Out Physics ----"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       , "Class) Type (KB/s, Avg Size, Count/s)"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>(""   , "OutPhysicsDetails"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>("", "(PBM Enabled)"));
        break;
    case NETWORKSTATS_DATATYPE:
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       , "----- In Data ----"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       , "Details (KB/s, Avg Size, Count/s)"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>(""   , "InDataDetails"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       , "----- Out Data ----"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       , "Details (KB/s, Avg Size, Count/s)"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>(""   , "OutDataDetails"));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       ,  ""));
        break;
	case NETWORKSTATS_STREAMING:
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""       , "----- Streaming ----"));
		hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Free Memory:"   , "FreeMemory"));
		hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Memory Level:"   , "MemoryLevel"));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""   , ""));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""   , ""));
		hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>(""   , "ReceivedStreamData"));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""   , ""));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""   , ""));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""   , ""));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""   , ""));
		hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""   , ""));

    default:
        break;
    }



	for (size_t i = 0; i < hudArray->numChildren(); ++i) {
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(12);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(280, 22));
	}
    if (init)
    {
	    hudArray->setVisible(false);
    }
	return hudArray;
}

shared_ptr<TopMenuBar> GuiBuilder::buildStatsHud2()
{
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::RIGHT;
	layout.yLocation = Rect::BOTTOM;
	layout.offset.x = 50;
	layout.offset.y = 40;
	layout.backdropColor = GuiItem::translucentDebugBackdrop();

	shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);
	hudArray->setName("StatsHud2");

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Contacts ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("CtctStageCtcts: ", "numContactsInCollisionStage"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("SteppingCtcts : ", "numSteppingContacts"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("TouchingCtcts : ", "numTouchingContacts"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("% Pair Hit    : ", "contactPairHitRatio"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("% Feature Hit : ", "contactFeatureHitRatio"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("# link(p)     : ", "numLinkCalls"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("sH Nodes Out  : ", "numHashNodes"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("sH Max Bucket : ", "maxBucketSize"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Kernel ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Solver Iteratn: ", "solverIterations")); // This is for fun / deception - we don't use matrices!
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Matrix Size   : ", "matrixSize"));		// This is for fun / deception - we don't use matrices!
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Bodies        : ", "numBodies"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("MaxBodies     : ", "maxBodies"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Leaves        : ", "numLeafBodies"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Constraints   : ", "numConstraints"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Points        : ", "numPoints"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("% Active CC's : ", "percentConnectorsActive"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Energy B      : ", "energyBody"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Energy C      : ", "energyConnector"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Energy T      : ", "energyTotal"));

	for (size_t i = 0; i < hudArray->numChildren(); ++i) {
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(12);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(260, 22));
	}
	hudArray->setVisible(false);
	return hudArray;
}

shared_ptr<TopMenuBar> GuiBuilder::buildPhysicsStats()
{
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::LEFT;
	layout.yLocation = Rect::TOP;
	layout.offset.x = 50;
	layout.offset.y = 40;
	layout.backdropColor = GuiItem::translucentDebugBackdrop();

	shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);
	hudArray->setName("PhysicsStats");

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- FPS ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Physics       : ", "Physics"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("PhysicsReal   : ", "PhysicsReal"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Render        : ", "Render"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Heartbeat     : ", "Heartbeat"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Network send  : ", "Network Send"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Network recv  : ", "Network Receive"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Rx data pkts  : ", "Received Data Packets"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Rx phys pkts  : ", "Received Physics Packets"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- World ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Player Radius : ", "Player Radius"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Parts		  : ", "numPrimitives"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Moving parts  : ", "numMovingPrimitives"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Joints        : ", "numJoints"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Contacts      : ", "numContactsAll"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("% Pair Hit    : ", "contactPairHitRatio"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("% Feature Hit : ", "contactFeatureHitRatio"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("sH Nodes      : ", "numHashNodes"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Kernel ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("PGS Solver On : ", "pGSSolverActive"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Bodies        : ", "numBodiesAll"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Connectors    : ", "numConnectorsAll"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Energy T      : ", "energyTotal"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Iterations    : ", "numIterations"));

	for (size_t i = 0; i < hudArray->numChildren(); ++i) {
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(12);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(440, 22));
	}
	hudArray->setVisible(false);
	return hudArray;
}
    
void GuiBuilder::updatePerformanceBasedStat(shared_ptr<TextDisplay> item, float value, float greenCutoff, float yellowCutoff, float orangeCutoff, bool isBottleneck)
{
    if (value >= greenCutoff)
    {
        item->setFontColor(G3D::Color3::green());
    }
    else if (value >= yellowCutoff)
    {
        item->setFontColor(G3D::Color3::yellow());
    }
    else if (value >= orangeCutoff)
    {
        item->setFontColor(G3D::Color3::orange());
    }
    else
    {
        item->setFontColor(G3D::Color3::red());
    }
    if (!isBottleneck)
    {
        item->setFontSize(12);
    }
    else
    {
        item->setFontSize(18);
    }
    item->setBorderColor(G3D::Color4::clear());
    item->setGuiSize(Vector2(440, 22));
}
    
// this function is called while we have a read lock on the DataModel but modifies it
// Things that need to be addressed to stop this:
// modifying the colour property of the text
// modifying the font size property of the text
// modifying the visibility of the text
void GuiBuilder::updateSummaryStats(RBX::TopMenuBar* hudArray)
{
    // parameters for adjusting the displayed stats
    const float startSlowRendering = 15.f;
    const float endSlowRendering = 20.f;
    const float startSlowPhysics = 15.f;
    const float endSlowPhysics = 20.f;
    const float startSlowNetworkCPU = 25.f;
    const float endSlowNetworkCPU = 20.f;
	const float startSlowNetworkPhysics = 15.f;
	const float endSlowNetworkPhysics = 17.f;
    const float totalGreenFPS = 28.f;
    const float totalYellowFPS = 20.f;
    const float totalOrangeFPS = 10.f;
    const float physicsGreenFPS = 28.f;
    const float physicsYellowFPS = 20.f;
    const float physicsOrangeFPS = 10.f;
    const float renderGreenFPS = 28.f;
    const float renderYellowFPS = 20.f;
    const float renderOrangeFPS = 10.f;
    const float networkGreenCPU = 15.f;
    const float networkYellowCPU = 20.f;
    const float networkOrangeCPU = 25.f;
	const float networkGreenPhysicsPacketsReceived = 19.f;
	const float networkYellowPhysicsPacketsReceived = 16.f;
	const float networkOrangePhysicsPacketsReceived = 12.f;
    
    enum 
	{
		DefaultStat_Total, 
		DefaultStat_Physics, 
		DefaultStat_Render, 
		DefaultStat_Network, 
		DefaultStat_Count,

		RenderStat_Start = DefaultStat_Count,
		RenderStat_End = RenderStat_Start + 13,
		PhysicsStat_Start = RenderStat_End,
		PhysicsStat_End = PhysicsStat_Start + 8,
		NetworkStat_Start = PhysicsStat_End,
		NetworkStat_End = NetworkStat_Start + 11
	};
	RBXASSERT(NetworkStat_End == hudArray->numChildren());
    
    float totalFps = dataModel->getMetricValue("Effective FPS");
    float renderFps = dataModel->getMetricValue("Render FPS");
    float physicsFps = dataModel->getMetricValue("Physics FPS");
    float networkReceivePercentage = dataModel->getMetricValue("Network Receive CPU");
	float physicsPacketsReceived = dataModel->getMetricValue("Received Physics Packets");
    
    static bool slowRendering = false;
    static bool slowPhysics = false;
    static bool slowNetworking = false;
	bool isNetworkGame;
    
    bool isRenderingBottleneck = false, isPhysicsBottleneck = false, isNetworkingBottleneck = false;
    
    if (totalFps < 30.f)
    {
        float frameTime = dataModel->getMetricValue("Frame Time");
        float renderTime = dataModel->getMetricValue("Render Time");
        float physicsTime = dataModel->getMetricValue("Physics Time");
        float networkTime = dataModel->getMetricValue("Network Receive Time");
        
        if (renderTime > physicsTime && renderTime > networkTime && renderTime > frameTime * 0.4f)
        {
            isRenderingBottleneck = true;
        }
        else if (physicsTime > renderTime && physicsTime > networkTime && physicsTime > frameTime * 0.4f)
        {
            isPhysicsBottleneck = true;
        }
        else
        {
            // if networking is taking longer than rendering or physics assume it is the bottleneck
            if ((networkTime > physicsTime && networkTime > renderTime) || networkReceivePercentage > 0.5f)
            {
                isNetworkingBottleneck = true;
            }
        }
    }
    
    slowRendering = renderFps < startSlowRendering || (slowRendering && renderFps < endSlowRendering);
    slowPhysics = physicsFps < startSlowPhysics || (slowPhysics && physicsFps < endSlowPhysics);

	// We ignore networking unless we're playing a networked game
	// networking checks to see if we spend too much time processing incoming packets or
	// receive physics packets slower than expected
	isNetworkGame = networkReceivePercentage > 0.f || physicsPacketsReceived > 0.f;
    slowNetworking = isNetworkGame &&
		(networkReceivePercentage > startSlowNetworkCPU || 
		 physicsPacketsReceived < startSlowNetworkPhysics ||
		 (slowNetworking && 
		  (networkReceivePercentage > endSlowNetworkCPU ||
		   physicsPacketsReceived < endSlowNetworkPhysics)));
    
    updatePerformanceBasedStat(shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(DefaultStat_Total)), totalFps, totalGreenFPS, totalYellowFPS, totalOrangeFPS, false);
    updatePerformanceBasedStat(shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(DefaultStat_Physics)), physicsFps, physicsGreenFPS, physicsYellowFPS, physicsOrangeFPS, isPhysicsBottleneck);
    updatePerformanceBasedStat(shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(DefaultStat_Render)), renderFps, renderGreenFPS, renderYellowFPS, renderOrangeFPS, isRenderingBottleneck);
    shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(DefaultStat_Network));
    if ((networkReceivePercentage < networkGreenCPU && physicsPacketsReceived >= networkGreenPhysicsPacketsReceived) ||
		!isNetworkGame)
    {
        item->setFontColor(G3D::Color3::green());
    }
    else if (networkReceivePercentage < networkYellowCPU && physicsPacketsReceived >= networkYellowPhysicsPacketsReceived)
    {
        item->setFontColor(G3D::Color3::yellow());
    }
    else if (networkReceivePercentage < networkOrangeCPU && physicsPacketsReceived >= networkOrangePhysicsPacketsReceived)
    {
        item->setFontColor(G3D::Color3::orange());
    }
    else
    {
        item->setFontColor(G3D::Color3::red());
    }
    if (!isNetworkingBottleneck)
    {
        item->setFontSize(12);
    }
    else
    {
        item->setFontSize(18);
    }
    item->setBorderColor(G3D::Color4::clear());
    item->setGuiSize(Vector2(440, 22));

	// hide all of the bottleneck specific stats
	for (size_t i = DefaultStat_Count; i < hudArray->numChildren(); ++i) {
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setVisible(false);
	}
    
    if (slowRendering)
    {
		for (size_t i = RenderStat_Start; i < RenderStat_End; ++i) {
			shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
			RBXASSERT(item);
			item->setVisible(true);
		}
    }
    
    if (slowPhysics)
    {
		for (size_t i = PhysicsStat_Start; i < PhysicsStat_End; ++i) {
			shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
			RBXASSERT(item);
			item->setVisible(true);
		}
    }
    
    if (slowNetworking)
    {
		for (size_t i = NetworkStat_Start; i < NetworkStat_End; ++i) {
			shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
			RBXASSERT(item);
			item->setVisible(true);
		}
    }
}
    
shared_ptr<TopMenuBar> GuiBuilder::buildSummaryStats()
{
    Layout layout;
    layout.layoutStyle = Layout::VERTICAL;
    layout.xLocation = Rect::LEFT;
    layout.yLocation = Rect::TOP;
    layout.offset.x = 50;
    layout.offset.y = 40;
    layout.backdropColor = GuiItem::translucentDebugBackdrop();
        
    shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);

    hudArray->setName("SummaryStats");

	hudArray->addGuiItem(Creatable<Instance>::create<EquationDisplay>("FPS         : ", "Effective FPS"));
	hudArray->addGuiItem(Creatable<Instance>::create<EquationDisplay>("Physics         : ", "Physics"));
	hudArray->addGuiItem(Creatable<Instance>::create<EquationDisplay>("Render          : ", "Render"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Network receive : ", "Network Receive"));

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>("", "----- Rendering -----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Video Memory  : ", "Video Memory"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Auto Quality: ", "Auto Quality"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Quality     : ", "FRM Quality"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("FRM Visible     : ", "FRM Visible"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Particles     : ", "Particles"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Scheduler Render        : ", "Render"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Present time            : ", "Present Time"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (total): ", "RenderStatsPassTotal"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (scene): ", "RenderStatsPassScene"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (shadow): ", "RenderStatsPassShadow"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (UI): ", "RenderStatsPassUI"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Draw (3D Adorns): ", "RenderStatsPass3dAdorns"));

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>("", "----- Physics -----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Parts		  : ", "numPrimitives"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Moving parts  : ", "numMovingPrimitives"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Joints        : ", "numJoints"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Contacts      : ", "numContacts"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("EnviroSpeed % : ", "environmentSpeed"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Energy T      : ", "energyTotal"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("TouchingCtcts : ", "numTouchingContacts"));

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>("", "----- Network -----"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Data Ping       : ", "Data Ping"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Send KBytesPerSec       : ", "Send kBps"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Receive KBytesPerSec       : ", "Receive kBps"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Incoming Packet Queue       : ", "Packet Queue"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Received Data Packets      : ", "Received Data Packets"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Received Physics Packets       : ", "Received Physics Packets"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Received Cluster Packets      : ", "Received Cluster Packets"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("messagesInResendQueue      : ", "messagesInResendQueue"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Sent Data Packets       : ", "Sent Data Packets"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("Sent Physics Packets       : ", "Sent Physics Packets"));

	for (size_t i = 0; i < hudArray->numChildren(); ++i) 
	{
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(12);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(440, 22));

	}

    hudArray->setVisible(false);
    return hudArray;
}

shared_ptr<TopMenuBar> GuiBuilder::buildCustomStats()
{
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::LEFT;
	layout.yLocation = Rect::TOP;
	layout.offset.x = 50;
	layout.offset.y = 40;
	layout.backdropColor = GuiItem::translucentDebugBackdrop();

	shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);

	hudArray->setName("CustomStats");

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>("", "----- Custom Stats -----"));

	// load custom stats JSON file
	CustomStatsGuiJSON json(this);
	std::string localSettingsData;

	std::ifstream localConfigFile(GetCustomStatsFilename().native().c_str());

	if(localConfigFile.is_open())
	{
		std::stringstream localConfigBuffer;
		localConfigBuffer << localConfigFile.rdbuf();
		localSettingsData = localConfigBuffer.str();
		if(localSettingsData.length() > 0) 
		{
			json.ReadFromStream(localSettingsData.c_str());
		}
	}

	for (CustomStatsContainer::iterator itr = customStatsCont.begin(); itr != customStatsCont.end(); ++itr)
	{
		hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>(itr->first + " : ", itr->second.stat));
	}

	for (size_t i = 0; i < hudArray->numChildren(); ++i) 
	{
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(12);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(440, 22));
	}

	hudArray->setVisible(false);
	return hudArray;
}

shared_ptr<TopMenuBar> GuiBuilder::buildPhysicsStats2()
{
	Layout layout;
	layout.layoutStyle = Layout::VERTICAL;
	layout.xLocation = Rect::RIGHT;
	layout.yLocation = Rect::BOTTOM;
	layout.offset.x = 5;
	layout.offset.y = 220;
	layout.backdropColor = GuiItem::translucentDebugBackdrop();

	shared_ptr<RelativePanel> hudArray = Creatable<Instance>::create<RelativePanel>(layout);
	hudArray->setName("PhysicsStats2");

	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "----- Physics Profiling ----"));
	hudArray->addGuiItem( Creatable<Instance>::create<TextDisplay>(""					, "World Step -"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Break Time:			", "Break Time"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Assembler Time:	", "Assembler Time"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Filter Time:            ", "Filter Time"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  UI Step:                  ", "UI Step"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Broadphase:           ", "Broadphase"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Collision:                 ", "Collision"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Joint Sleep:           ", "Joint Sleep"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Wake:                    ", "Wake"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Sleep:                     ", "Sleep"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Joint Update:          ", "Joint Update"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Bodies:                   ", "Kernel Bodies"));
	hudArray->addGuiItem( Creatable<Instance>::create<EquationDisplay>("  Connectors:             ", "Kernel Connectors"));

	for (size_t i = 0; i < hudArray->numChildren(); ++i) {
		shared_ptr<TextDisplay> item = shared_from_dynamic_cast<TextDisplay>(hudArray->getChild(i));
		RBXASSERT(item);
		item->setFontSize(12);
		item->setFontColor(G3D::Color3::white());
		item->setBorderColor(G3D::Color3::black());
		item->setGuiSize(Vector2(260, 22));
	}
	hudArray->setVisible(false);
	return hudArray;
}


} // namespace
