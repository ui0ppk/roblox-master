/**
 * MachineConfiguration.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "MachineConfiguration.h"

#include "reflection/Property.h"
#include "v8datamodel/DebugSettings.h"
#include "util/Http.h"
#include "RenderSettingsItem.h"
#include "format_string.h"
#include "RobloxServicesTools.h"

FASTFLAG(UseBuildGenericGameUrl)

static void appendProperty(const char* category, std::stringstream& data, const RBX::Reflection::Property& prop)
{
	if (prop.getDescriptor().category == category)
	{
		data << prop.getName().c_str();
		data << ':';
		data << prop.getStringValue();
		data << ';';
	}
}

static void HandleAsyncHttp(std::string *response, std::exception *exception)
{
	// TODO: log?
}

namespace RBX
{
    void postMachineConfiguration(const char* baseURL, int lastGfxMode)
	{
        std::string url;
		if (FFlag::UseBuildGenericGameUrl)
		{
			url = BuildGenericGameUrl(baseURL, "Game/MachineConfiguration.ashx");
		}
		else
		{
			url = ::trim_trailing_slashes(std::string(baseURL)) + "/Game/MachineConfiguration.ashx";
		}
		std::string response;
		try
		{
			std::stringstream data;
			G3D::Vector2int16 displayResolution(800, 600);
			G3D::Vector2int16 fullscreenResolution(800, 600);
			RBX::DebugSettings& debugsettings = RBX::DebugSettings::singleton();
			CRenderSettingsItem& rendersettings = CRenderSettingsItem::singleton();

            debugsettings.setVertexShaderModel(-1);
            debugsettings.setPixelShaderModel(-1);

// DirectX Related Stuff Windows Only
#ifdef _WIN32
			int x,y;
			if(2 == sscanf_s(debugsettings.resolution().c_str(), "%d x%d", &x, &y))
			{
				displayResolution.x = x;
				displayResolution.y = y;
			}
			fullscreenResolution = rendersettings.getFullscreenSize();
#endif // DirectX Related Stuff Windows Only
			
			std::for_each(
				debugsettings.properties_begin(), debugsettings.properties_end(), 
				boost::bind(&appendProperty, "Profile", boost::ref(data), _1)
				);

			std::for_each(
				rendersettings.properties_begin(), rendersettings.properties_end(), 
				boost::bind(&appendProperty, "General", boost::ref(data), _1)
				);

            data << "lastGfxMode:" << lastGfxMode << ";";

			data.flush();
			RBX::Http(url).post(data.str(), Http::kContentTypeDefaultUnspecified, false, boost::bind(&HandleAsyncHttp, _1, _2));
		}
		catch (const std::exception&)
		{
		}
    }
}
