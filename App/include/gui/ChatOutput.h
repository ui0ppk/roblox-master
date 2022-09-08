/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Gui/Gui.h"
#include "Util/RunStateOwner.h"
#include "V8DataModel/ChatService.h"
#include "Util/UDim.h"


namespace RBX {

	class SafeChat;
	class GuiObject;
	class BillboardGui;
	class ModelInstance;

	namespace Network {
		class Player;
		class Players;
		class ChatMessage;
	}

	class ChatLine {
	protected:
		static float ComputeBubbleLifetime(const std::string& msg, bool isSelf);

	public:				
		static const char* ELIPSES;
		static const int CchMaxChatMessageLength; // max chat message length, including null terminator and elipses.
		enum BubbleColor
		{
			WHITE,
			BLUE,
			GREEN, 
			RED,
		};

		enum ChatType
		{
			PLAYER_CHAT,
			PLAYER_TEAM_CHAT,
			PLAYER_WHISPER_CHAT,

			GAME_MESSAGE,
			PLAYER_GAME_CHAT,
			BOT_CHAT,
		};

		std::string	message;
		const BubbleColor bubbleColor;
		const ChatType chatType;
		float startTime;
		float bubbleDieDelay;
		bool isLocalPlayer;
		boost::weak_ptr<Instance> origin;

		const Instance* getOrigin() const { return origin.lock().get(); }

		ChatLine(ChatType chatType, const std::string& message, float startTime, BubbleColor bubbleColor, bool isLocalPlayer);
		virtual ~ChatLine() {}

		bool isPlayerChat() const
		{
			switch(chatType)
			{
			case PLAYER_CHAT:
			case PLAYER_TEAM_CHAT:
			case PLAYER_WHISPER_CHAT:
				return true;
			default:
				return false;
			}
		}
	};

	class PlayerChatLine : public ChatLine {
	public:

		static const char* ROBLOXNAME;
		Color3 userColor;
		
		std::string user;
		float historyDieDelay;
		
		const ModelInstance* getCharacter() const;

		PlayerChatLine(ChatType chatType, boost::shared_ptr<Network::Player> player, const std::string& message, float startTime, bool isLocalPlayer);
	};
	class GameChatLine: public ChatLine 
	{
	public:
		GameChatLine(boost::shared_ptr<Instance> origin, const std::string& message, float startTime, bool isLocalPlayer, BubbleColor bubbleColor);
	};

	struct CharacterChats
	{
		CharacterChats() 
			: isVisible(false)
			, isMoving(false) 
		{}
		std::deque<boost::shared_ptr<ChatLine> > fifo;
		bool isVisible; 
		bool isMoving;
		weak_ptr<BillboardGui> billboardGui;
	};

	class ChatOutput 
		: public GuiItem
	{
	private:
		static const int kMaxTextSize = 16;
		static const int kMaxCharsInLine = 20;

		void createBillboardGuiHelper(Instance* instance, bool character);

		void renderBubbleImposters(Adorn* adorn, weak_ptr<const Instance> owner, weak_ptr<PartInstance> head);
		void renderBubbles(Adorn* adorn, weak_ptr<const Instance> owner, weak_ptr<PartInstance> head, bool playerAndGameChat, 
						   Vector3 extentsOffset, Vector3 studsOffset);
		typedef GuiItem Super;
		static const int MaxChatBubblesPerPlayer;
		static const int MaxChatLinesPerBubble;

		RBX::Network::Players* players;	
		std::map<ChatLine::BubbleColor, shared_ptr<GuiObject> > chatBubble;
		std::map<ChatLine::BubbleColor, shared_ptr<GuiObject> > chatBubbleWithTail;
		std::map<ChatLine::BubbleColor, shared_ptr<GuiObject> > scalingChatBubbleWithTail;

		struct ScalingInfo
		{
			Vector2 scalingCutoff;
			UDim2 fixedPosition;
			UDim2 fixedSize;

			UDim2 scalingPosition;
			UDim2 scalingSize;

			UDim2 getPosition(Vector2 size)
			{

				return UDim2(size.x < scalingCutoff.x ? scalingPosition.x : fixedPosition.x, 
					         size.y < scalingCutoff.y ? scalingPosition.y : fixedPosition.y);
			}
			UDim2 getSize(Vector2 size)
			{
				return UDim2(size.x < scalingCutoff.x ? scalingSize.x : fixedSize.x, 
					         size.y < scalingCutoff.y ? scalingSize.y : fixedSize.y);
			}

			ScalingInfo(Vector2 scalingCutoff, UDim2 fixedPosition, UDim2 fixedSize, UDim2 scalingPosition, UDim2 scalingSize)
				:scalingCutoff(scalingCutoff)
				,fixedPosition(fixedPosition)
				,fixedSize(fixedSize)
				,scalingPosition(scalingPosition)
				,scalingSize(scalingSize)
			{}
			ScalingInfo()
			{}
		};
		std::map<ChatLine::BubbleColor, ScalingInfo> scalingInfo;
		
		std::map<ChatLine::BubbleColor, shared_ptr<GuiObject> > chatPlaceholder;

		std::deque<boost::shared_ptr<ChatLine> >	fifo;
		typedef std::map<const Instance*, CharacterChats> CharacterChatMap;
		CharacterChatMap characterSortedMsg;
		float time;

		void acceleratedBubbleDecay(ChatLine* line, float wallStep, bool isMoving, bool isVisible);

		void removeOldest();
		bool removeExpired();

		bool bubbleChatEnabled();

		std::string SanitizeChatLine(const std::string& msg); // truncate and make safe.

		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		// Listener
		rbx::signals::scoped_connection heartbeatConnection;
		rbx::signals::scoped_connection playerChatMessageConnection;
		rbx::signals::scoped_connection gameChatMessageConnection;
		void onHeartbeat(const Heartbeat& heartbeat);

		void onPlayerChatMessage(const Network::ChatMessage& event);
		void onGameChatMessage(boost::shared_ptr<Instance> origin, const std::string& message, ChatService::ChatColor color);

		// GuiItem
		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void render2d_bubbleStyle(Adorn* adorn, bool playerBubbleChat);

	public:
		ChatOutput();
		~ChatOutput();
	};
	
} // namespace