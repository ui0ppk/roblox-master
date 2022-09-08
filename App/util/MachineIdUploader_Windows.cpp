#include "stdafx.h"

#include "Util/MachineIdUploader.h"

#include "Rbx/Debug.h"

#include <winsock2.h>
#include <Iphlpapi.h>

#pragma comment(lib, "Iphlpapi.lib")

using namespace RBX;

bool MachineIdUploader::fillMachineId(MachineId* out) {

	// Query number of addresses will be returned
	ULONG addressBufferSize = 0;
	bool success = ERROR_BUFFER_OVERFLOW == GetAdaptersAddresses(
		AF_UNSPEC, 0 /*don't filter type*/, NULL /*reserved*/,
		NULL, &addressBufferSize);
	
	if (!success) { return false; }

	// Do query for data 
	PIP_ADAPTER_ADDRESSES addressBuffer =
		(PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), 0, addressBufferSize);
	success = ERROR_SUCCESS == GetAdaptersAddresses(
		AF_UNSPEC, 0 /*don't filter type*/, NULL /*reserved*/,
		&addressBuffer[0], &addressBufferSize);

	if (success) {
		for (PIP_ADAPTER_ADDRESSES currentAdapter = &addressBuffer[0];
				currentAdapter != NULL;
				currentAdapter = currentAdapter->Next) {
			
			if (currentAdapter->IfType == IF_TYPE_ETHERNET_CSMACD) {
				RBXASSERT(MacAddress::kBytesInMacAddress ==
					currentAdapter->PhysicalAddressLength);
				
				out->macAddresses.resize(out->macAddresses.size() + 1);
				size_t bytesToCopy = std::min((DWORD)MacAddress::kBytesInMacAddress,
					currentAdapter->PhysicalAddressLength);

				for (size_t i = 0; i < bytesToCopy; ++i) {
					out->macAddresses.back().address[i] = currentAdapter->PhysicalAddress[i];
				}
			}
		}
	}

	HeapFree(GetProcessHeap(), 0, addressBuffer);

	return success;
}
