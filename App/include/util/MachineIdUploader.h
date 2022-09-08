#pragma once

#include <string>
#include <vector>

namespace RBX {

// Helper class to gather identifying information about this machine and for
// communicating with the web banned machine database.
class MachineIdUploader {
public:
	static const char* kBannedMachineMessage;
	enum Result {
		RESULT_MachineAccepted = 1,
		RESULT_MachineBanned = 0
	};
	// Gather identifying info for this machine, send it out, and return
	// weather this machine has been banned or not.
	static Result uploadMachineId(const char* baseUrl);

    static std::string getMachineId();

private:
	struct MacAddress {
		static const int kBytesInMacAddress = 6;
		unsigned char address[kBytesInMacAddress];
		std::string asString() const;
	};

	struct MachineId {
		std::vector<MacAddress> macAddresses;
	};

	static bool fillMachineId(MachineId* out);
	static bool buildMacAddressContent(bool needsLeadingAmp, const MachineId& id, std::stringstream& stream);
	static void buildContent(const MachineId& id, std::stringstream& stream);
};

}
