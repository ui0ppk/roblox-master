#pragma once

#include <string>

#include "util/object.h"

namespace RBX {

    class Instance;
	class TextDisplay;
	class GuiRoot;
	class TopMenuBar;
	class Verb;
	class DataModel;
	class Workspace;
	class ChatOption;
	class UnifiedWidget;
	class CustomStatsGuiJSON;

	class GuiBuilder
	{
	public:
        enum Display
        {
            DISPLAY_NONE,
            DISPLAY_FPS,
            DISPLAY_SUMMARY,
            DISPLAY_PHYSICS,
            DISPLAY_PHYSICS_AND_OWNER,
            DISPLAY_RENDER
        };

        enum NetworkStats
        {
            NETWORKSTATS_INVALID = 0,
            NETWORKSTATS_FIRST,
            NETWORKSTATS_RAKNET = NETWORKSTATS_FIRST,
            NETWORKSTATS_PHYSICS,
            NETWORKSTATS_DATATYPE,
			NETWORKSTATS_STREAMING,
            NETWORKSTATS_COUNT
        };

        static Display getDebugDisplay();
        static void setDebugDisplay(Display display);

		void buildGui(
			Workspace* workspace,
			bool buildInGameGui);

		// uses CoreGuiService and CoreScript to inject a lua gui into the game
		void buildLuaGui();

		void updateGui();

		void addCustomStat(const std::string& name, const std::string& value);
		void removeCustomStat(const std::string& name);
		void saveCustomStats();

		void Initialize(DataModel* dataModel);

		friend class CustomStatsGuiJSON;

		void removeSafeChatMenu();
		void addSafeChatMenu();

        void nextNetworkStats();
        NetworkStats getDisplayingNetworkStats() {return networkStatsCounter;}

        static void buildNetworkStatsOutput(shared_ptr<Instance> instance, std::string* output);
		static void buildSimpleStatsOutput(shared_ptr<Instance> instance, std::string* output);
        
        
        
        /// Stats Menu
        void toggleGeneralStats();
        void toggleRenderStats();
        void toggleNetworkStats();
        void togglePhysicsStats();
        void toggleSummaryStats();
        void toggleCustomStats();
        

    private:
		
		// Main Elements
		shared_ptr<TopMenuBar> buildRightPalette();

		shared_ptr<TopMenuBar> buildChatHud();
		shared_ptr<TopMenuBar> buildChatMenu();
		shared_ptr<TopMenuBar> buildStatsHud1();
		shared_ptr<TopMenuBar> buildStatsHud2();
		shared_ptr<TopMenuBar> buildRenderStats();
		shared_ptr<TopMenuBar> buildNetworkStats();
		shared_ptr<TopMenuBar> buildNetworkStats2(bool);
        shared_ptr<TopMenuBar> buildFPS();
		shared_ptr<TopMenuBar> buildPhysicsStats();
		shared_ptr<TopMenuBar> buildPhysicsStats2();
        shared_ptr<TopMenuBar> buildSummaryStats();
		shared_ptr<TopMenuBar> buildCustomStats();
        
        static void updatePerformanceBasedStat(shared_ptr<TextDisplay> item, float value, float greenCutoff, float yellowCutoff, float orangeCutoff, bool isBottleneck);
        void updateSummaryStats(TopMenuBar* hudArray);
		void updateCustomStats(TopMenuBar* hudArray);

        Verb* getWhitelistVerb(const std::string& name);

		void buildChatMenu(ChatOption *cur, std::string code, shared_ptr<UnifiedWidget> parentWidget);

		DataModel* dataModel;
		Workspace* workspace;

		shared_ptr<TopMenuBar> safeChatMenu;

		struct Data
		{
			std::string stat;
			shared_ptr< TextDisplay > item;
		};

		typedef std::map< std::string, Data > CustomStatsContainer;
		CustomStatsContainer customStatsCont;

        NetworkStats networkStatsCounter;
        
        bool oldTrackDataTypesValue;
        bool oldTrackPhysicsDetailsValue;

	};
}	// namespace
