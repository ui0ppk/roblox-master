#include "stdafx.h"

// NOTE!
// Include order here is very important. boost/property_tree needs to come
// early to prevent errors on mac builds, but on windows builds RbxPlatform
// needs to come before property_tree, to prevent a winsock conflict.
#include "RbxPlatform.h"
#include "Util/MachineIdUploader.h"
#include "Util/Http.h"
#include "V8DataModel/Stats.h"
#include "FastLog.h"
#include "format_string.h"
#include "RobloxServicesTools.h"
#include <rapidjson/document.h>
#include "v8xml/WebParser.h"

#include <sstream>

using namespace RBX;

const char* MachineIdUploader::kBannedMachineMessage = "Roblox cannot startup. User code = 0x1.";
static const char* kMacAddressKey = "macAddresses";

LOGGROUP(MachineIdUploader)
FASTFLAG(UseBuildGenericGameUrl)

std::string MachineIdUploader::MacAddress::asString() const {
	std::string result(2 * kBytesInMacAddress, 0);
	static const char* kToHex = "0123456789abcdef";

	for (int i = 0; i < kBytesInMacAddress; ++i) {
		result[2 * i      ] = kToHex[address[i] >> 4];
		result[(2 * i) + 1] = kToHex[address[i] & 0xf];
	}

	return result;
}

bool MachineIdUploader::buildMacAddressContent(bool needsLeadingAmpersand, const MachineId& id, std::stringstream& stream) {
	for (size_t i = 0; i < id.macAddresses.size(); ++i) {
		if (needsLeadingAmpersand) {
			stream << "&";
		}
		stream << kMacAddressKey << "=" << 
			//"badmacaddress"; // use this line to test rejection
			id.macAddresses[i].asString();

		needsLeadingAmpersand = true;
	}
	return needsLeadingAmpersand;
}

void MachineIdUploader::buildContent(const MachineId& id, std::stringstream& stream) {
	bool needsAmp = false;

	needsAmp = buildMacAddressContent(needsAmp, id, stream) || needsAmp;

	FASTLOGS(FLog::MachineIdUploader, "Machine id key: [%s]", stream.str());
}

std::string getPostUrlFromBaseUrl(const char* baseUrl) {
	if (FFlag::UseBuildGenericGameUrl)
	{
		return BuildGenericGameUrl(baseUrl, "game/validate-machine");
	}
	else
	{
		return ::trim_trailing_slashes(std::string(baseUrl)) + "/game/validate-machine";
	}
}

bool responseIndicatesMachineIsBad(const std::string& response) {
	FASTLOGS(FLog::MachineIdUploader, "Raw json: %s", response);

	static const char* kStatusField = "success";
	bool machineIsBad = false;

	
	rapidjson::Document doc;
    doc.Parse<rapidjson::kParseDefaultFlags>(response.c_str());

	if (doc.HasMember(kStatusField))
		machineIsBad = !doc[kStatusField].GetBool();
	

	if (machineIsBad) {
		FASTLOG(FLog::MachineIdUploader, "Machine is banned");
	} else {
		FASTLOG(FLog::MachineIdUploader, "Machine is not banned");
	}

	return machineIsBad;
}

MachineIdUploader::Result MachineIdUploader::uploadMachineId(const char* baseUrl) {
	MachineId id;
	if (!fillMachineId(&id)) {
		return RESULT_MachineAccepted;
	}

	if (id.macAddresses.empty()) {
		return RESULT_MachineAccepted;
	}

    std::stringstream stream;
    buildContent(id, stream);

    std::istringstream stringInStream(stream.str());
    
    Http http(getPostUrlFromBaseUrl(baseUrl));
    std::string response;
    
    bool machineBanned = false;

	try {
		http.post(stringInStream, Http::kContentTypeUrlEncoded, false/*compress*/, response);
		machineBanned = responseIndicatesMachineIsBad(response);
	} catch (const RBX::base_exception&) {
	}

	if (machineBanned) {
		return RESULT_MachineBanned;
	} else {
		return RESULT_MachineAccepted;
	}
}

std::string MachineIdUploader::getMachineId()
{
	MachineId id;

	if (!fillMachineId(&id))
        return "";

    std::string result;

	for (size_t i = 0; i < id.macAddresses.size(); ++i)
	{
        result += (i > 0) ? "," : "";
		result += id.macAddresses[i].asString();
	}

    return result;
}