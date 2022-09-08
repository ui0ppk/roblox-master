#include "stdafx.h"

#include "V8DataModel/DialogRoot.h"
#include "V8DataModel/DialogChoice.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/CollectionService.h"
#include "V8Tree/Service.h"
#include "Network/Player.h"

#ifdef RBX_RCC_SECURITY
// for debugging of an exploit
#include "Util/CheatEngine.h"
#endif

// Debug of a potential exploit.
FASTFLAGVARIABLE(US31006, false);

namespace RBX
{

const char* const sDialogRoot= "Dialog";

REFLECTION_BEGIN();
//static Reflection::PropDescriptor<DialogRoot, bool> prop_PublicChat("Public", category_Data, &DialogRoot::getPublicChat, &DialogRoot::setPublicChat);
static Reflection::PropDescriptor<DialogRoot, std::string> prop_InitialPrompt("InitialPrompt",category_Data,  &DialogRoot::getInitialPrompt, &DialogRoot::setInitialPrompt);
static Reflection::PropDescriptor<DialogRoot, std::string> prop_GoodbyeDialog("GoodbyeDialog",category_Data,  &DialogRoot::getGoodbyeDialog, &DialogRoot::setGoodbyeDialog);

static Reflection::EnumPropDescriptor<DialogRoot, DialogRoot::DialogPurpose> prop_DialogPurpose("Purpose", category_Data, &DialogRoot::getDialogPurpose, &DialogRoot::setDialogPurpose);
static Reflection::EnumPropDescriptor<DialogRoot, DialogRoot::DialogTone> prop_DialogTone("Tone", category_Data, &DialogRoot::getDialogTone, &DialogRoot::setDialogTone);
static Reflection::PropDescriptor<DialogRoot, float> prop_ConversationDistance("ConversationDistance", category_Data,  &DialogRoot::getConversationDistance, &DialogRoot::setConversationDistance);
static Reflection::PropDescriptor<DialogRoot, bool> prop_InUse("InUse", category_Data,  &DialogRoot::getInUse, &DialogRoot::setInUse, Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::BoundFuncDesc<DialogRoot, void(shared_ptr<Instance>, shared_ptr<Instance>)>  func_signalDialogChoice(&DialogRoot::signalDialogChoice, "SignalDialogChoiceSelected", "player", "dialogChoice", Security::RobloxScript);

static Reflection::RemoteEventDesc<DialogRoot, void(shared_ptr<Instance>, shared_ptr<Instance>)> event_DialogChoiceSelected(&DialogRoot::dialogChoiceSelected, "DialogChoiceSelected", "player", "dialogChoice", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<DialogRoot::DialogPurpose>::EnumDesc()
	:EnumDescriptor("DialogPurpose")
{
	addPair(DialogRoot::QUEST_PURPOSE, "Quest");
	addPair(DialogRoot::HELP_PURPOSE, "Help");
	addPair(DialogRoot::SHOP_PURPOSE, "Shop");
}
template<>
EnumDesc<DialogRoot::DialogTone>::EnumDesc()
	:EnumDescriptor("DialogTone")
{
	addPair(DialogRoot::NEUTRAL_TONE, "Neutral");
	addPair(DialogRoot::FRIENDLY_TONE, "Friendly");
	addPair(DialogRoot::ENEMY_TONE, "Enemy");
}
}//namespace Reflection

DialogRoot::DialogRoot() 
	: DescribedCreatable<DialogRoot, Instance, sDialogRoot>()
	, publicChat(true)
	, inUse(false)
	, conversationDistance(25.0f)
	, dialogPurpose(DialogRoot::HELP_PURPOSE)
	, dialogTone(DialogRoot::NEUTRAL_TONE)
{
	this->setName(sDialogRoot);
}

DialogRoot::~DialogRoot() 
{
#ifdef RBX_RCC_SECURITY
    if (FFlag::US31006 && initialPrompt.capacity() > 1000)
    {
        RBX::removeWriteBreakpoint(reinterpret_cast<uintptr_t>(&initialPrompt));
    }
#endif
}

void DialogRoot::signalDialogChoice(shared_ptr<Instance> player, shared_ptr<Instance> dialogChoice)
{
	if(!Instance::fastDynamicCast<Network::Player>(player.get()))
		throw std::runtime_error("Player object expected as first argument");
	if(!Instance::fastDynamicCast<DialogChoice>(dialogChoice.get()))
		throw std::runtime_error("DialogChoice object expected as second argument");
	
	if(!isAncestorOf(dialogChoice.get()))
		throw std::runtime_error("DialogChoice must be a child of this dialog root");

	event_DialogChoiceSelected.fireAndReplicateEvent(this, player, dialogChoice);
}

void DialogRoot::setDialogPurpose(DialogPurpose value)
{
	if(dialogPurpose != value)
	{
		dialogPurpose = value;
		raisePropertyChanged(prop_DialogPurpose);
	}
}
void DialogRoot::setDialogTone(DialogTone value)
{
	if(dialogTone != value)
	{
		dialogTone = value;
		raisePropertyChanged(prop_DialogTone);
	}
}
void DialogRoot::setPublicChat(bool value)
{
	if(publicChat != value)
	{
		publicChat = value;
		//raisePropertyChanged(prop_PublicChat);
	}
}
void DialogRoot::setConversationDistance(float value)
{
	if(conversationDistance != value)
	{
		conversationDistance = value;
		raisePropertyChanged(prop_ConversationDistance);
	}
}


void DialogRoot::setInUse(bool value)
{
	if(inUse != value)
	{
		inUse = value;
		raisePropertyChanged(prop_InUse);
	}
}

void DialogRoot::setInitialPrompt(std::string value)
{
#ifdef RBX_RCC_SECURITY
    if (FFlag::US31006 && initialPrompt.capacity() > 1000)
    {
        RBX::removeWriteBreakpoint(reinterpret_cast<uintptr_t>(&initialPrompt));
    }
#endif
	if(initialPrompt != value)
	{
		initialPrompt = value;
		raisePropertyChanged(prop_InitialPrompt);
	}
#ifdef RBX_RCC_SECURITY
    if (FFlag::US31006 && value.size() > 1000)
    {
        RBX::addWriteBreakpoint(reinterpret_cast<uintptr_t>(&initialPrompt));
    }
#endif
}

void DialogRoot::setGoodbyeDialog(std::string value)
{
	if(goodbyeDialog != value)
	{
		goodbyeDialog = value;
		raisePropertyChanged(prop_GoodbyeDialog);
	}
}

void DialogRoot::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if(oldProvider){
		oldProvider->create<CollectionService>()->removeInstance(shared_from(this));
	}

	Super::onServiceProvider(oldProvider, newProvider);

	if(newProvider){
		newProvider->create<CollectionService>()->addInstance(shared_from(this));
	}
}

bool DialogRoot::askSetParent(const Instance* instance) const
{
	return Instance::fastDynamicCast<PartInstance>(instance) != NULL;
}
	


}