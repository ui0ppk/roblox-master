#include "stdafx.h"

#include "V8DataModel/FastLogSettings.h"

// Client
LOGVARIABLE(VideoCapture, 0)
LOGVARIABLE(Verbs, 0)
LOGVARIABLE(Plugins, 0)
LOGVARIABLE(COMCalls, 1)
LOGVARIABLE(FullWindowMessages, 0)
LOGVARIABLE(RobloxWndInit, 1)
LOGVARIABLE(CrashReporterInit, 1)

// Studio
LOGVARIABLE(Explorer, 0)

// User input
LOGVARIABLE(HardwareMouse, 2)
LOGVARIABLE(UserInputProfile, 0)
LOGVARIABLE(DragProfile, 0)
LOGVARIABLE(GuiTargetLifetime, 0)
LOGVARIABLE(MouseCommandLifetime, 0)

// RCCService
LOGVARIABLE(GoldenHashes, 1)

// Rendering
LOGVARIABLE(DeviceLost, 0)
LOGVARIABLE(FRM, 0)
LOGVARIABLE(AdornableLifetime, 0)
LOGVARIABLE(AdornRenderStats, 0)
LOGVARIABLE(ViewRbxBase, 2)
LOGVARIABLE(ViewRbxInit, 1)
LOGVARIABLE(ThumbnailRender, 0)
LOGVARIABLE(AdornVB, 0)
LOGVARIABLE(SSAO, 0)
LOGVARIABLE(PushPixels, 0)
LOGVARIABLE(RenderStatsOnLogs, 2)
LOGVARIABLE(VisualEngineInit, 0)

// DataModel
LOGVARIABLE(PartInstanceLifetime, 0)
LOGVARIABLE(JointInstanceLifetime, 0)

// Physics
LOGVARIABLE(PrimitiveLifetime, 0)
LOGVARIABLE(JointLifetime, 0)

// Perf
LOGVARIABLE(RenderBreakdown, 2)
LOGVARIABLE(RenderBreakdownDetails, 2)
LOGVARIABLE(DataModelJobs, 2)

// Render.Drawcalls
LOGVARIABLE(FullRenderObjects, 0)

// Cluster

// Rbx.Megacluster
LOGVARIABLE(RbxMegaClustersUpdate, 2)
LOGVARIABLE(RbxMegaClusterInit, 2)

// Rbx.Clusters
LOGVARIABLE(GfxClusters, 2)
LOGVARIABLE(GfxClustersFull, 0)

// Cluster.DataModel
LOGVARIABLE(MegaClusters, 1)
LOGVARIABLE(MegaClusterInit, 1)
LOGVARIABLE(MegaClusterDirty, 0)
LOGVARIABLE(MegaClusterDecodeStream, 0)

// Cluster.Network
LOGVARIABLE(MegaClusterNetwork, 2)
LOGVARIABLE(MegaClusterNetworkInit, 1)

// Switches
LOGVARIABLE(DisableInvalidateOnUnbind, 0)
// Whether mouse goes through locked parts and terrain.

FASTFLAGVARIABLE(DebugHumanoidRendering, false);
DYNAMIC_FASTFLAGVARIABLE(DebugDisableTimeoutDisconnect, false);

FASTFLAGVARIABLE(DebugLocalRccServerConnection, false);

// Base
class BaseLogInitter
{
public:
	BaseLogInitter()
	{
		extern void initBaseLog();
		initBaseLog();
	}
};
BaseLogInitter initter;

// Data Model
LOGVARIABLE(CloseDataModel, 1)
LOGVARIABLE(TouchedSignal, 0)
LOGVARIABLE(TextureContentProvider, 0)
LOGVARIABLE(LegacyLock, 0)

// Physics Logs
LOGVARIABLE(Physics, 0)

// Content load
LOGVARIABLE(ContentProviderRequests, 0)

// UI events
LOGVARIABLE(MouseCommand, 0)

// Scripts
LOGVARIABLE(ScriptContextAdd, 0)
LOGVARIABLE(ScriptContextRemove, 0)
LOGVARIABLE(ScriptContext, 0)
LOGVARIABLE(ScriptContextClose, 1)
LOGVARIABLE(WeakThreadRef, 0)

LOGVARIABLE(CoreScripts, 1)
LOGVARIABLE(UseLuaMemoryPool, 0)
LOGVARIABLE(LuaMemoryPool, 0)
LOGVARIABLE(LuaProfiler, 0)
LOGVARIABLE(LuaScriptTimeoutSeconds, 0)

// Network
LOGVARIABLE(Network, 0)
LOGVARIABLE(HttpQueue, 0)
LOGVARIABLE(Player, 1)
LOGVARIABLE(NetworkCache, 1)
LOGVARIABLE(ReplicationDataLifetime, 0)
LOGVARIABLE(NetworkInstances, 0)
LOGVARIABLE(NetworkReadItem, 0)
LOGVARIABLE(JoinSendExtraItemCount, 1)
LOGVARIABLE(MaxNetworkReadTimeInCS, 0)
LOGVARIABLE(DistanceMetricP1, 0)
LOGVARIABLE(ttMetricP1, 0)

DYNAMIC_LOGVARIABLE(MaxJoinDataSizeKB, 100) // Default 100KB
DYNAMIC_LOGVARIABLE(NetworkJoin, 0)

// Security
LOGVARIABLE(US14116, 0)
LOGVARIABLE(MachineIdUploader, 1)

// RakNet
LOGVARIABLE(RakNetDisconnect, 0)
DYNAMIC_FASTINTVARIABLE(RakNetMaxSplitPacketCount, 1400) // about 2MB if packet size is around 1492 bytes (max mtu)

LOGVARIABLE(Http, 0)

// Other
LOGVARIABLE(Legacy, 0)
LOGVARIABLE(ScriptPrint, 3)
LOGVARIABLE(ClientSettings, 1)
LOGVARIABLE(PlayerShutdownLuaTimeoutSeconds, 0)

namespace RBX
{
	DATA_MAP_IMPL_START(ClientAppSettings)
		IMPL_DATA(AllowVideoPreRoll, false);
		IMPL_DATA(VideoPreRollWaitTimeSeconds, 30);
		IMPL_DATA(StartPageUrl, "");
		IMPL_DATA(WebDocAddressBarEnabled, true);
		IMPL_DATA(CaptureQTStudioCountersEnabled, false);
		IMPL_DATA(CaptureMFCStudioCountersEnabled, false);
		IMPL_DATA(CaptureCountersIntervalInMinutes, 5);
		IMPL_DATA(CaptureSlowCountersIntervalInSeconds, 300);
		IMPL_DATA(PublishedProjectsPageUrl, "");
		IMPL_DATA(PublishedProjectsPageWidth, 800);
		IMPL_DATA(PublishedProjectsPageHeight, 600);
		IMPL_DATA(AxisAdornmentGrabSize, 5);
		IMPL_DATA(PrizeAwarderURL, "");
		IMPL_DATA(PrizeAssetIDs, "");
		IMPL_DATA(MinNumberScriptExecutionsToGetPrize, 500);
		IMPL_DATA(MinPartsForOptDragging, 200);
		IMPL_DATA(GoogleAnalyticsAccountPropertyID, "UA-43420590-2");
		IMPL_DATA(GoogleAnalyticsAccountPropertyIDPlayer, "UA-43420590-13");
		IMPL_DATA(GoogleAnalyticsThreadPoolMaxScheduleSize, 500);
		IMPL_DATA(GoogleAnalyticsLoadPlayer, 1); // percent probability of using google analytics
        IMPL_DATA(GoogleAnalyticsLoadStudio, 100); // percent probability of using google analytics
		IMPL_DATA(GoogleAnalyticsInitFix, true);
        IMPL_DATA(HttpUseCurlPercentageMacClient, 0);
        IMPL_DATA(HttpUseCurlPercentageMacStudio, 0);
        IMPL_DATA(HttpUseCurlPercentageWinClient, 0);
        IMPL_DATA(HttpUseCurlPercentageWinStudio, 0);
	DATA_MAP_IMPL_END()
    
	ClientAppSettings ClientAppSettings::m_ClientAppSettings;
    
	void ClientAppSettings::Initialize()
	{
		FetchClientSettingsData(CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY, &ClientAppSettings::singleton());
	}
    
	ClientAppSettings& ClientAppSettings::singleton()
	{
		
		return m_ClientAppSettings;
	}
    
	void FastLogJSON::ProcessVariable(const std::string& valueName, const std::string& valueData, FastVarType fastVarType)
	{
		FLog::SetValue(valueName, valueData, fastVarType);
	}
   
	bool FastLogJSON::DefaultHandler(const std::string& valueName, const std::string& valueData)
	{
		bool processed = false;
		static std::string FLogPattern="FLog";
		static std::string FFlagPattern="FFlag";
		static std::string FIntPattern="FInt";
		static std::string FStringPattern="FString";

        static std::string SFLogPattern="SFLog";
        static std::string SFFlagPattern="SFFlag";
        static std::string SFIntPattern="SFInt";
		static std::string SFStringPattern="SFString";

		static std::string DFLogPattern="DFLog";
		static std::string DFFlagPattern="DFFlag";
		static std::string DFIntPattern="DFInt";
		static std::string DFStringPattern="DFString";

		static std::string ABNewUsersPattern="ABNewUsers";
		static std::string ABNewStudioUsersTest="ABNewStudioUsers";
		static std::string ABAllUsersTest="ABAllUsers";

        if (valueName[0] == 'D')
		{
			if(valueName.compare(0, DFLogPattern.length(), DFLogPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + DFLogPattern.length(), valueData, FASTVARTYPE_DYNAMIC);
			}
			else if(valueName.compare(0, DFFlagPattern.length(), DFFlagPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + DFFlagPattern.length(), valueData, FASTVARTYPE_DYNAMIC);
			}
			else if(valueName.compare(0, DFIntPattern.length(), DFIntPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + DFIntPattern.length(), valueData, FASTVARTYPE_DYNAMIC);
			}
			else if(valueName.compare(0, DFStringPattern.length(), DFStringPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + DFStringPattern.length(), valueData, FASTVARTYPE_DYNAMIC);
			}
		}
        else if (valueName[0] == 'S') // synchronized variables
        {
            if (valueName.compare(0, SFLogPattern.length(), SFLogPattern) == 0)
            {
                processed = true;
                ProcessVariable(valueName.c_str() + SFLogPattern.length(), valueData, FASTVARTYPE_SYNC);
            }
            else if(valueName.compare(0, SFFlagPattern.length(), SFFlagPattern) == 0)
            {
                processed = true;
                ProcessVariable(valueName.c_str() + SFFlagPattern.length(), valueData, FASTVARTYPE_SYNC);
            }
            else if(valueName.compare(0, SFIntPattern.length(), SFIntPattern) == 0)
            {
                processed = true;
                ProcessVariable(valueName.c_str() + SFIntPattern.length(), valueData, FASTVARTYPE_SYNC);
            }
			else if(valueName.compare(0, SFStringPattern.length(), SFStringPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + SFStringPattern.length(), valueData, FASTVARTYPE_SYNC);
			}
        }
        else
		{
			if(valueName.compare(0, FLogPattern.length(), FLogPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + FLogPattern.length(), valueData, FASTVARTYPE_STATIC);
			}
			else if(valueName.compare(0, FFlagPattern.length(), FFlagPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + FFlagPattern.length(), valueData, FASTVARTYPE_STATIC);
			}
			else if(valueName.compare(0, FIntPattern.length(), FIntPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + FIntPattern.length(), valueData, FASTVARTYPE_STATIC);
			}
			else if(valueName.compare(0, FStringPattern.length(), FStringPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + FStringPattern.length(), valueData, FASTVARTYPE_STATIC);
			}

			if(valueName.compare(0, ABNewUsersPattern.length(), ABNewUsersPattern) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + ABNewUsersPattern.length(), valueData, FASTVARTYPE_AB_NEWUSERS);
			} 
			else if(valueName.compare(0, ABNewStudioUsersTest.length(), ABNewStudioUsersTest) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + ABNewStudioUsersTest.length(), valueData, FASTVARTYPE_AB_NEWSTUDIOUSERS);
			}
			else if(valueName.compare(0, ABAllUsersTest.length(), ABAllUsersTest) == 0)
			{
				processed = true;
				ProcessVariable(valueName.c_str() + ABAllUsersTest.length(), valueData, FASTVARTYPE_AB_ALLUSERS);
			}
		}
        
		return processed;
	}
}
