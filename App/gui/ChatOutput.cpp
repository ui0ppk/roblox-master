/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Gui/ChatOutput.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "Util/Hash.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/Teams.h"
#include "V8DataModel/Scale9Frame.h"
#include "V8DataModel/ImageLabel.h"
#include "V8DataModel/GuiObject.h"
#include "V8DataModel/BillboardGui.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/TextLabel.h"
#include "Gui/ProfanityFilter.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "GfxBase/ViewportBillboarder.h"
#include "GfxBase/AdornBillboarder.h"
#include "FastLog.h"

namespace RBX {

using RBX::Network::Players;

const char* PlayerChatLine::ROBLOXNAME = "(ROBLOX)";
const char* ChatLine::ELIPSES = "...";
const int ChatOutput::MaxChatBubblesPerPlayer = 10;
const int ChatOutput::MaxChatLinesPerBubble = 5; // the number of lines each bubble can display
const int ChatLine::CchMaxChatMessageLength = 128; // max chat message length, including null terminator and elipses.
const int CchMaxChatMessageLengthExclusive = ChatLine::CchMaxChatMessageLength - strlen(ChatLine::ELIPSES) - 1;

static float lerpLength(const std::string& msg, float min, float max)
{
	return min + (max-min)*std::min(msg.length()/75.0f, 1.0f);
}

float ChatLine::ComputeBubbleLifetime(const std::string& msg, bool isSelf)
{
	if(isSelf)
		return lerpLength(msg,8,15);
	else
		return lerpLength(msg,12,20);
}

ChatLine::ChatLine(ChatType chatType, const std::string& message, float startTime, BubbleColor bubbleColor, bool isLocalPlayer)
	: chatType(chatType)
	, origin()
	, message(message)
	, startTime(startTime)
	, bubbleDieDelay(ChatLine::ComputeBubbleLifetime(message, isLocalPlayer)) // mutable.
	, bubbleColor(bubbleColor)
	, isLocalPlayer(isLocalPlayer)
{}

PlayerChatLine::PlayerChatLine(ChatType chatType, boost::shared_ptr<Network::Player> player, const std::string& message, float startTime, bool isLocalPlayer) 
	: ChatLine(chatType, message, startTime, ChatLine::WHITE, isLocalPlayer)
	, user("")
	, historyDieDelay(60.0f)
{
	if (player)
	{
		user = player->getName();
		origin = player->getSharedCharacter();
		
		// If this is a team game, make the color of the text be the team color of the speaker
		Teams *teams = ServiceProvider::find<Teams>(player.get());

		if(teams != NULL && teams->isTeamGame())
		{
			if(player->getNeutral() == false) 
			{
				Team *t = teams->getTeamFromPlayer(player.get());
				if (t != NULL) 
					userColor = t->getTeamColor().color3(); 
				else
					userColor = G3D::Color3::black();
			}
			else
				userColor = G3D::Color3::white();

		} else {
			unsigned int hash = Hash::hash(user);
			userColor = Color::colorFromIndex8(hash % 8);
		}
	}
	else{
		user = ROBLOXNAME;
		userColor = Color::black();
	}
}

GameChatLine::GameChatLine(boost::shared_ptr<Instance> origin, const std::string& message, float startTime, bool isLocalPlayer, BubbleColor bubbleColor)
:ChatLine(Instance::fastDynamicCast<ModelInstance>(origin.get()) ? ChatLine::PLAYER_GAME_CHAT : ChatLine::BOT_CHAT, message, startTime, bubbleColor, isLocalPlayer)
{
	this->origin = origin;
}



static shared_ptr<Scale9Frame> createChatBubbleMain(const std::string& filePrefix)
{
	shared_ptr<Scale9Frame> chatBubbleMain = Creatable<Instance>::create<Scale9Frame>();
	chatBubbleMain->setName("ChatBubble");
	chatBubbleMain->setScaleEdgeSize(Vector2int16(8,8));
	chatBubbleMain->setSlicePrefix("rbxasset://textures/"+filePrefix+".png");
	chatBubbleMain->setBackgroundTransparency(0.0f);
	chatBubbleMain->setBorderSizePixel(0);
	chatBubbleMain->setSize(UDim2(1.0f, 0, 1.0f, 0));
	chatBubbleMain->setPosition(UDim2(0,0,0,-30));

	return chatBubbleMain;
}

static shared_ptr<Scale9Frame> createChatBubbleWithTail(const std::string& filePrefix, const UDim2& position, const UDim2& size)
{
	shared_ptr<Scale9Frame> chatBubbleMain;
	chatBubbleMain = createChatBubbleMain(filePrefix);
	shared_ptr<ImageLabel> chatBubbleTail = Creatable<Instance>::create<ImageLabel>();
	chatBubbleTail->setName("ChatBubbleTail");
	chatBubbleTail->setImage("rbxasset://textures/ui/dialog_tail.png");
	chatBubbleTail->setBackgroundTransparency(1.0f);
	chatBubbleTail->setBorderSizePixel(0);
	chatBubbleTail->setPosition(position);
	chatBubbleTail->setSize(size);
	chatBubbleTail->setParent(chatBubbleMain.get());

	return chatBubbleMain;
}

static shared_ptr<Scale9Frame> createScaledChatBubbleWithTail(const std::string& filePrefix, float frameScaleSize, const UDim2& position)
{
	shared_ptr<Scale9Frame> chatBubbleMain;
	chatBubbleMain = createChatBubbleMain(filePrefix);
	
	shared_ptr<Frame> frame = Creatable<Instance>::create<Frame>();
	frame->setName("ChatBubbleTailFrame");
	frame->setBackgroundTransparency(1.0f);
	frame->setSizeConstraint(GuiObject::RELATIVE_XX);
	frame->setPosition(UDim2(0.5f, 0, 1, 0));
	frame->setSize(UDim2(frameScaleSize, 0, frameScaleSize, 0));
	frame->setParent(chatBubbleMain.get());

	shared_ptr<ImageLabel> chatBubbleTail = Creatable<Instance>::create<ImageLabel>();
	chatBubbleTail->setName("ChatBubbleTail");
	chatBubbleTail->setImage("rbxasset://textures/ui/dialog_tail.png");
	chatBubbleTail->setBackgroundTransparency(1.0f);
	chatBubbleTail->setBorderSizePixel(0);
	chatBubbleTail->setPosition(position);
	chatBubbleTail->setSize(UDim2(1,0,0.5,0));
	chatBubbleTail->setParent(frame.get());

	return chatBubbleMain;
}

static shared_ptr<GuiObject> createChatImposter(const std::string& filePrefix, const std::string& dotDotDot, float yOffset)
{
	shared_ptr<ImageLabel> result = Creatable<Instance>::create<ImageLabel>();
	result->setName("DialogPlaceholder");
	result->setImage("rbxasset://textures/" + filePrefix + ".png");
	result->setBackgroundTransparency(1.0f);
	result->setBorderSizePixel(0);
	result->setPosition(UDim2(0, 0, -1.25, 0));
	result->setSize(UDim2(1, 0, 1, 0));

	shared_ptr<ImageLabel> image = Creatable<Instance>::create<ImageLabel>();
	image->setName("DotDotDot");
	image->setImage("rbxasset://textures/"+dotDotDot+".png");
	image->setBackgroundTransparency(1.0f);
	image->setBorderSizePixel(0);
	image->setPosition(UDim2(0.001, 0, yOffset, 0));
	image->setSize(UDim2(1, 0, 0.7, 0));
	image->setParent(result.get());

	return result;
}


ChatOutput::ChatOutput() 
: time(0.0)
, players(NULL)
{
	{
		chatBubble[ChatLine::WHITE] = createChatBubbleMain("ui/dialog_white");
		chatBubbleWithTail[ChatLine::WHITE] = createChatBubbleWithTail("ui/dialog_white", UDim2(0.5f, -14, 1.0f, 0), UDim2(0.0f, 30, 0.0f, 14));
		scalingChatBubbleWithTail[ChatLine::WHITE] = createScaledChatBubbleWithTail("ui/dialog_white", 0.5f, UDim2(-0.5f, 0, 0.0f, 0));
	}

	{      
		chatBubble[ChatLine::BLUE] = createChatBubbleMain("ui/dialog_blue");
		chatBubbleWithTail[ChatLine::BLUE] = createChatBubbleWithTail("ui/dialog_blue", UDim2(0.5f, -14, 1.0f, -1), UDim2(0.0f, 30, 0.0f, 14));
		scalingChatBubbleWithTail[ChatLine::BLUE] = createScaledChatBubbleWithTail("ui/dialog_blue", 0.5f, UDim2(-0.5f, 0, 0.0f, -1));
	}

	{
		chatBubble[ChatLine::RED] = createChatBubbleMain("ui/dialog_red");
		chatBubbleWithTail[ChatLine::RED] = createChatBubbleWithTail("ui/dialog_red", UDim2(0.5f, -14, 1.0f, -1), UDim2(0.0f, 30, 0.0f, 14));
		scalingChatBubbleWithTail[ChatLine::RED] = createScaledChatBubbleWithTail("ui/dialog_red", 0.5f, UDim2(-0.5f, 0, 0.0f, -1));
	}

	{
		chatBubble[ChatLine::GREEN] = createChatBubbleMain("ui/dialog_green");
		chatBubbleWithTail[ChatLine::GREEN] = createChatBubbleWithTail("ui/dialog_green", UDim2(0.5f, -14, 1.0f, -1), UDim2(0.0f, 30, 0.0f, 14));
		scalingChatBubbleWithTail[ChatLine::GREEN] = createScaledChatBubbleWithTail("ui/dialog_green", 0.5f, UDim2(-0.5f, 0, 0.0f, -1));
	}

	chatPlaceholder[ChatLine::WHITE] = createChatImposter("ui/chatBubble_white_notify_bkg", "chatBubble_bot_notifyGray_dotDotDot", -0.05f);
	chatPlaceholder[ChatLine::BLUE] = createChatImposter("ui/chatBubble_blue_notify_bkg", "chatBubble_bot_notifyGray_dotDotDot", -0.12);
	chatPlaceholder[ChatLine::GREEN] = createChatImposter("ui/chatBubble_green_notify_bkg", "chatBubble_bot_notifyGray_dotDotDot",-0.12);
	chatPlaceholder[ChatLine::RED] = createChatImposter("ui/chatBubble_red_notify_bkg", "chatBubble_bot_notifyGray_dotDotDot", -0.12);
}


ChatOutput::~ChatOutput()
{
	while (!fifo.empty()) {
		removeOldest();
	}
}

const ModelInstance* PlayerChatLine::getCharacter() const
{
	return Instance::fastDynamicCast<ModelInstance>(getOrigin()); 
}
void ChatOutput::acceleratedBubbleDecay(ChatLine* line, float wallStep, bool isMoving, bool isVisible)
{
	if(line->isLocalPlayer && isMoving)
	{
		line->bubbleDieDelay -= 3* wallStep; // effectively quarters delay time.
	}
	else if(isVisible)
	{
		line->bubbleDieDelay -= wallStep; // effectively halfs delay time.
	}

}

std::string ChatOutput::SanitizeChatLine(const std::string& msg)
{
	if(msg.size() > (size_t)CchMaxChatMessageLengthExclusive)
	{
		return msg.substr(0, CchMaxChatMessageLengthExclusive) + ChatLine::ELIPSES;
	}
	else
	{
		return msg;
	}
}
void ChatOutput::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	heartbeatConnection.disconnect();
	playerChatMessageConnection.disconnect();
	gameChatMessageConnection.disconnect();

	Super::onServiceProvider(oldProvider, newProvider);

	if (RunService* runService = ServiceProvider::create<RunService>(newProvider))
		heartbeatConnection = runService->heartbeatSignal.connect(boost::bind(&ChatOutput::onHeartbeat, this, _1));

	players = ServiceProvider::create<Network::Players>(newProvider);

	if(newProvider)
	{
		if (players)
			playerChatMessageConnection = players->chatMessageSignal.connect(boost::bind(&ChatOutput::onPlayerChatMessage, this, _1));

		if(ChatService* chatService = ServiceProvider::create<ChatService>(newProvider))
			gameChatMessageConnection = chatService->chattedSignal.connect(boost::bind(&ChatOutput::onGameChatMessage, this, _1, _2, _3));
	}
}


void ChatOutput::removeOldest()
{
	fifo.pop_front();
}

bool ChatOutput::removeExpired()
{
	bool bRemovedSomething = false;
	int maxBubblesPerPlayer = std::min(RBX::GameSettings::singleton().bubbleChatMaxBubbles, MaxChatBubblesPerPlayer);

	if (!fifo.empty())
	{
		// fifo contains only PlayerChatLine objects, its only using generic ChatLine for use in helper functions
		PlayerChatLine* line = boost::polymorphic_downcast<PlayerChatLine*>(fifo.front().get());
		if((line->historyDieDelay + line->startTime) < time)
		{
			fifo.pop_front();
			bRemovedSomething = true;
		}
	}
	for(CharacterChatMap::iterator it = characterSortedMsg.begin(); it != characterSortedMsg.end();)
	{
		std::deque<boost::shared_ptr<ChatLine> >& playerfifo(it->second.fifo);
		if(!playerfifo.empty())
		{
			ChatLine* line = playerfifo.front().get();
			if((line->bubbleDieDelay + line->startTime) < time || (int) playerfifo.size() > maxBubblesPerPlayer)
			{
				playerfifo.pop_front();
				bRemovedSomething = true;
			}
		}
		// remove if empty
		if(playerfifo.empty())
		{
			if(shared_ptr<BillboardGui> billlboardGui = it->second.billboardGui.lock())
				billlboardGui->setParent(NULL);

			characterSortedMsg.erase(it++);
		}
		else
		{
			it++;
		}
	}

	return bRemovedSomething;
}

void ChatOutput::onHeartbeat(const Heartbeat& heartbeat)
{
	float wallStep = (float)heartbeat.wallStep;
	time += wallStep;


	for(CharacterChatMap::iterator it = characterSortedMsg.begin(); it != characterSortedMsg.end(); ++it)
	{
		std::deque<boost::shared_ptr<ChatLine> >& playerfifo(it->second.fifo);
		for (size_t i = 0; i < playerfifo.size(); ++i) 
		{
			acceleratedBubbleDecay(playerfifo[i].get(), wallStep, it->second.isMoving, it->second.isVisible);
		}
	}


	// delete chatLines with dieTime younger than heartbeat time;
	while(removeExpired());
}

void ChatOutput::createBillboardGuiHelper(Instance* instance, bool onlyCharacter)
{
	if(!characterSortedMsg[instance].billboardGui.lock())
	{
		if(CoreGuiService* coreGui = ServiceProvider::create<CoreGuiService>(instance))
		{
			if(!onlyCharacter)
			{
				if(PartInstance* part = instance->fastDynamicCast<PartInstance>())
				{
					//Create a new billboardGui object attached to this player
					shared_ptr<BillboardGui> billboardGui = Creatable<Instance>::create<BillboardGui>();
					billboardGui->setAdornee(part);	
					billboardGui->setRobloxLocked(true);
					billboardGui->setParent(coreGui);

					characterSortedMsg[instance].billboardGui = billboardGui;
					return;
				}
			}

			if(ModelInstance* character = instance->fastDynamicCast<ModelInstance>())
			{
				if(PartInstance* head  = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Head")))
				{
					//Create a new billboardGui object attached to this player
					shared_ptr<BillboardGui> billboardGui = Creatable<Instance>::create<BillboardGui>();
					billboardGui->setAdornee(head);	
					billboardGui->setRobloxLocked(true);
					billboardGui->setParent(coreGui);

					characterSortedMsg[instance].billboardGui = billboardGui;
				}
			}
		}
	}
}

void ChatOutput::onGameChatMessage(boost::shared_ptr<Instance> origin, const std::string& message, ChatService::ChatColor color)
{
	if (ProfanityFilter::ContainsProfanity(message))
		return;

	Network::Player* local = players->getLocalPlayer();
	bool fromOthers = local != NULL && (local->getCharacter() != origin.get());

	ChatLine::BubbleColor bubbleColor = ChatLine::WHITE;
	switch(color)
	{
	case ChatService::CHAT_BLUE:	bubbleColor = ChatLine::BLUE;	break;
	case ChatService::CHAT_GREEN:	bubbleColor = ChatLine::GREEN;	break;
	case ChatService::CHAT_RED:		bubbleColor = ChatLine::RED;	break;
	}
	std::string safemessage = SanitizeChatLine(message);
	shared_ptr<ChatLine> line(new GameChatLine(origin, safemessage, time, !fromOthers, bubbleColor));
	characterSortedMsg[line->getOrigin()].fifo.push_back(line);
	createBillboardGuiHelper(origin.get(), false);
}

void ChatOutput::onPlayerChatMessage(const Network::ChatMessage& event)
{
	// eliminate display of emotes
	if (boost::starts_with(event.message, "/e ") || boost::starts_with(event.message, "/emote ")) 
	{
		return;
	}

	while (fifo.size() > (size_t)RBX::GameSettings::singleton().chatScrollLength) 
	{
		removeOldest();
	}

	Network::Player* local = players->getLocalPlayer();
	bool fromOthers = local != NULL && (event.source.get() != local);

	ChatLine::ChatType chatType = ChatLine::PLAYER_CHAT;
	switch(event.chatType)
	{
	case Network::ChatMessage::CHAT_TYPE_TEAM:
		chatType = ChatLine::PLAYER_TEAM_CHAT;
		break;
	case Network::ChatMessage::CHAT_TYPE_ALL:
		chatType = ChatLine::PLAYER_CHAT;
		break;
	case Network::ChatMessage::CHAT_TYPE_WHISPER:
		chatType = ChatLine::PLAYER_WHISPER_CHAT;
		break;
	case Network::ChatMessage::CHAT_TYPE_GAME:
		chatType = ChatLine::GAME_MESSAGE;
		break;
	}

	std::string safemessage = SanitizeChatLine(event.message);

	shared_ptr<PlayerChatLine> line(new PlayerChatLine(chatType, event.source, safemessage, time, !fromOthers));
	fifo.push_back(line);
	characterSortedMsg[line->getOrigin()].fifo.push_back(line);

	if(event.source)
	{
		//Game chat (badges) won't show up here
		createBillboardGuiHelper(event.source->getCharacter(), true);
	}
}

bool ChatOutput::bubbleChatEnabled()
{
	if(players)
	{
		return players->getBubbleChat();
	}
	return false;
}

void ChatOutput::render2d(Adorn* adorn)
{
	render2d_bubbleStyle(adorn, bubbleChatEnabled());
}

void ChatOutput::renderBubbleImposters(Adorn* adorn, weak_ptr<const Instance> weakOwner, weak_ptr<PartInstance> weakHead)
{
	shared_ptr<PartInstance> head = weakHead.lock();
	if(!head)
		return;

	shared_ptr<const Instance> owner = weakOwner.lock();
	if(!owner)
		return;

	Workspace* workspace = ServiceProvider::find<Workspace>(head.get());
	if(!workspace)
		return;

	ViewportBillboarder viewportBillboarder(Vector3(0,0,0), Vector3(0,0,0), Vector2(0,0), UDim2(3,0, 3.6, 0), NULL /*work in actual pixel space*/);
    viewportBillboarder.update(adorn->getViewport(), *workspace->getConstCamera(), head.get()->getPartSizeXml(), head.get()->calcRenderingCoordinateFrame());
	if(!viewportBillboarder.isVisibleAndValid())
	{
		return;
	}

	AdornBillboarder adornView(adorn, viewportBillboarder);

	std::deque<boost::shared_ptr<ChatLine> >& playerfifo(characterSortedMsg[owner.get()].fifo);
	for (size_t i = playerfifo.size()-1; i != ~0; --i) 
	{
		if(playerfifo[i]->chatType != ChatLine::GAME_MESSAGE)
		{
			chatPlaceholder[playerfifo[i]->bubbleColor]->legacyRender2d(&adornView, adornView.getViewport());
			break;
		}
	}
}

void ChatOutput::renderBubbles(Adorn* adorn, weak_ptr<const Instance> weakOwner, weak_ptr<PartInstance> weakHead, bool playerBubbleChat,
							   Vector3 extentsOffset, Vector3 studsOffset)
{
	shared_ptr<PartInstance> head = weakHead.lock();
	if(!head)
		return;

	shared_ptr<const Instance> owner = weakOwner.lock();
	if(!owner)
		return;

	Workspace* workspace = ServiceProvider::find<Workspace>(head.get());
	if(!workspace)
		return;

	Text::Font font;
	TextService::Font tsFont;
	tsFont = TextService::FONT_SOURCESANS;
	font = Text::FONT_SOURCESANS;

	const UDim2 size(UDim(22, 25), UDim(3, 0)); /// we need some size, but we are not constrained by this size.
												/// we will use billboard witdh as maximum width, but we will allow ourselves to spill out upwards.
												/// note also: bubble is drawn in actual pixel space. but size of bubble is set using some stud size contribution (will get smaller
												/// as we get farther). We will maintain character width so that maximum width always corresponds to same number of chars. this
												/// is to ensure that there is no re-layouting of text as we move away
										 		/// we will align bottom of text to top of viewport. this means that the vertical dimention is only usefull for adjusting the size of the bubble arrow.

	ViewportBillboarder viewportBillboarder(extentsOffset, studsOffset, Vector2::zero(), size, NULL /*work in actual pixel space*/);
    viewportBillboarder.update(adorn->getViewport(), *workspace->getConstCamera(), head.get()->getPartSizeXml(), head.get()->calcRenderingCoordinateFrame());

	AdornBillboarder adornView(adorn, viewportBillboarder);

	std::deque<boost::shared_ptr<ChatLine> >& playerfifo(characterSortedMsg[owner.get()].fifo);

	Rect2D viewsize = adornView.getViewport();
	// clamp max size of bubble.
	float viewsizewidth = std::min(viewsize.width(), (float)kMaxTextSize * (kMaxCharsInLine + 1));
	float sizeadjust = viewsize.width() - viewsizewidth;
	viewsize = Rect2D::xywh(viewsize.x0() + sizeadjust * 0.5f, viewsize.y0(), viewsize.width() - sizeadjust, viewsize.height());

	double textSize = floor(viewsize.width() + 0.5f) / (kMaxCharsInLine + 1); // margin is half a char each side.
	double textSizeHeight = textSize *1.5f; //magic number, we just know this.
	double textMargin = textSize /2;
	double spaceBetweenBubbles = 2;
	Vector2 availableTextSpace((float) (viewsize.width() - textMargin * 2), (float)(textSizeHeight * MaxChatLinesPerBubble + 0.01f));
	double lineCursor = -textSizeHeight; // start at the top of the viewport + textsize, (note: we go up from there!)
	Vector2 pointOfBubble(viewsize.center().x, 0);  // point at the top of the viewport

	textSize *= 1.8f; // source sans is smaller than legacy
	spaceBetweenBubbles = 4.0f;
	lineCursor = 0.0f;


	//Allocate memory on STACK for this data structure
	//Rect2D* textrect = (Rect2D*)alloca(sizeof(Rect2D) * playerfifo.size());
	Rect2D* bubbleTextRect = (Rect2D*)alloca(sizeof(Rect2D) * playerfifo.size());

	float newBubbleBorder = 5.0f;
	unsigned chatCount = 0;
	for (size_t i = playerfifo.size()-1; i != ~0; --i) 
	{
		if(!playerBubbleChat && playerfifo[i]->isPlayerChat())
			continue;

		// grab head, and try to get the workspace from the first one we find.
		TextService* textService = ServiceProvider::create<TextService>(this);
		if(!textService)
			continue;

		Vector2 textBounds = textService->getTypesetter(tsFont)->measure(playerfifo[i]->message, (float)textSize, availableTextSpace);
        
		float scale = 1.0f;
		scale = std::min(scale, textBounds.x / 60);
		scale = std::min(scale, textBounds.y / 60);

		float border = 7 + 13*scale;
		float yborder = border;
		float olderChatScale = 3;
		border = yborder = newBubbleBorder;
		yborder /= olderChatScale;
		float x = (float)(viewsize.center().x - textBounds.x /2);
		float y = (float)(lineCursor - textBounds.y);
		bubbleTextRect[chatCount] = Rect2D::xywh(x-(2.5f*border), y-yborder, textBounds.x+5*border, textBounds.y+2*yborder);
		//textrect[chatCount] = Rect2D::xywh((float)(viewsize.center().x - textBounds.x /2), (float)(lineCursor - textBounds.y), textBounds.x, textBounds.y).border((float)-textMargin);
		lineCursor -= (bubbleTextRect[chatCount].height() + spaceBetweenBubbles);

		chatCount++;
	}


	for (size_t i = 0; i < playerfifo.size(); ++i) 
	{
		if(!playerBubbleChat && playerfifo[i]->isPlayerChat())
			continue;
		
		Color3 textColor = Color3::black();
		Text::XAlign xAlin = Text::XALIGN_CENTER;

		ChatLine::BubbleColor bubbleColor = playerfifo[i]->bubbleColor;
		const Rect2D& rect = bubbleTextRect[chatCount-1];
		if(chatCount == 1)
		{
			if(rect.width() < 60 || rect.height() < 60)
			{
				if(GuiObject* tail = Instance::fastDynamicCast<GuiObject>(scalingChatBubbleWithTail[bubbleColor]->findFirstChildByName("ChatBubbleTailFrame"))){
					tail->setSizeConstraint(rect.width() < rect.height() ? GuiObject::RELATIVE_XX : GuiObject::RELATIVE_YY);
					scalingChatBubbleWithTail[bubbleColor]->legacyRender2d(&adornView, rect);
				}
			}
			else{
				//straight up render, no scaling funniness
				chatBubbleWithTail[bubbleColor]->legacyRender2d(&adornView, rect);
			}
			textColor = Color3(75/255.0f, 75/255.0f, 75/255.0f);
			xAlin = Text::XALIGN_LEFT;
			chatBubble[bubbleColor]->setBackgroundTransparency(0.0f);
		}
		else
		{
			chatBubble[bubbleColor]->legacyRender2d(&adornView, rect);

			textColor = Color3(184/255.0f, 184/255.0f, 184/255.0f);
			xAlin = Text::XALIGN_LEFT;
		}

		Vector2 center = rect.center() + Vector2(0, -30);
		Color4 tColor = textColor;
		center.x -= (rect.width() / 2) - (newBubbleBorder * 2.5f);
		center.y -= 2;
		tColor = Color4(textColor, 1.0f - chatBubble[bubbleColor]->getBackgroundTransparency());
		adornView.drawFont2D(	playerfifo[i]->message,
								center,
								(float)textSize,			// size
                                false,
								tColor,
								Color4(Color3::black(), 0), // no outline
								font,
								xAlin,
								Text::YALIGN_CENTER,
								availableTextSpace,
								Rect2D::xyxy(-1,-1,-1,-1));

		chatCount--;
	}
}
void ChatOutput::render2d_bubbleStyle(Adorn* adorn, bool playerBubbleChat)
{
	Vector3 extentsOffset;
	Vector3 studsOffset;

	Workspace* workspace = NULL;

	for(CharacterChatMap::iterator it = characterSortedMsg.begin(); it != characterSortedMsg.end(); ++it)
	{
		std::deque<boost::shared_ptr<ChatLine> >& playerfifo(it->second.fifo);

		if(shared_ptr<BillboardGui> billboardGui = it->second.billboardGui.lock())
		{
			billboardGui->setRenderFunction(NULL);
		}

		it->second.isVisible = false;
		it->second.isMoving = false;

		if(playerfifo.empty())
			continue;

		PartInstance* head = NULL;
		if(boost::shared_ptr<Instance> instance = playerfifo[0]->origin.lock())
		{
			switch(playerfifo[0]->chatType)
			{
			case ChatLine::PLAYER_CHAT:
			case ChatLine::PLAYER_TEAM_CHAT:
			case ChatLine::PLAYER_WHISPER_CHAT:
			case ChatLine::PLAYER_GAME_CHAT:
				extentsOffset = Vector3(0,0,0);

				if(playerfifo[0]->chatType == ChatLine::PLAYER_GAME_CHAT)
					studsOffset = Vector3(0, 0, 2);   //towards camera (so that gear doesn't block readability)
				else
					studsOffset = Vector3(0,0,2);   //towards camera (so that gear doesn't block readability)
				if(ModelInstance* character = instance->fastDynamicCast<ModelInstance>())
				{
					head  = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Head"));

					if(head && !workspace)
					{
						workspace = ServiceProvider::find<Workspace>(head);
					}
				}
				else
				{
					RBXASSERT(0);
				}
				break;
			case ChatLine::GAME_MESSAGE:
				RBXASSERT(0);
				break;
			case ChatLine::BOT_CHAT:
				extentsOffset = Vector3(0,1,0);
				
				head = instance->fastDynamicCast<PartInstance>();
				if(head)
				{
					if(head->getName() == "Head" && Instance::fastDynamicCast<ModelInstance>(head->getParent()))
						studsOffset = Vector3(0, 0, 0);   //towards camera (so that gear doesn't block readability)
					else
						studsOffset = Vector3(0.0, 0, 0);

					if(!workspace)
					{
						workspace = ServiceProvider::find<Workspace>(head);
					}
				}
				break;
			}
		}

		if(!workspace || !head)
		{
			continue;
		}

		if(!playerBubbleChat)
		{
			bool foundGameChat = false;
			//Game chat only
			for (size_t i = playerfifo.size()-1; i != ~0; --i) 
			{
				if(!playerfifo[0]->isPlayerChat())
				{
					foundGameChat = true;
					break;
				}
			}
			if(!foundGameChat)
			{
				continue;
			}
		}

        CoordinateFrame headCFrame = head->calcRenderingCoordinateFrame();

		Vector3 distanceToObjectCenter = workspace->getCamera()->coordinateFrame().pointToObjectSpace(headCFrame.translation);
		if(distanceToObjectCenter.z > 0)
			continue;

		if(distanceToObjectCenter.z < -90.0f)
		{
			if(shared_ptr<BillboardGui> billboardGui = it->second.billboardGui.lock())
			{
				billboardGui->setRenderFunction(boost::bind(&ChatOutput::renderBubbleImposters, this, _2, weak_from(it->first), weak_from(head)));
			}
		}
		else
		{
			RBX::Frustum frustum(workspace->getCamera()->frustum());
			it->second.isVisible = frustum.containsPoint(headCFrame.translation);
			it->second.isMoving = !head->getVelocity().linear.isZero();
			if(shared_ptr<BillboardGui> billboardGui = it->second.billboardGui.lock())
			{
				billboardGui->setRenderFunction(boost::bind(&ChatOutput::renderBubbles, this, _2, weak_from(it->first), weak_from(head), playerBubbleChat, extentsOffset, studsOffset));
			}
		}
	}

}

} // namespace


