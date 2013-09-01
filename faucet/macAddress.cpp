#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <Iphlpapi.h>
#include <Wininet.h>
#include <cstdlib>
#include <cstdio>

#include <faucet/GmStringBuffer.hpp>

#define DLLEXPORT extern "C" __declspec(dllexport)

DLLEXPORT const char* mac_addrs() {
	PIP_ADAPTER_ADDRESSES addrs = NULL;
	DWORD dwRetVal = 0;
	ULONG flags = GAA_FLAG_SKIP_UNICAST|GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER;

	ULONG allocSize = 16384;
	int iterations = 0;

	do {
		addrs = (PIP_ADAPTER_ADDRESSES) realloc((void*)addrs, allocSize);
		if(addrs==NULL) {
			return "";
		}
		dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addrs, &allocSize);
	} while((dwRetVal == ERROR_BUFFER_OVERFLOW) && (iterations++ < 5));

	if(dwRetVal!=NO_ERROR) {
		free((void*)addrs);
		return "";
	}

	std::string result;
	bool firstAddress = true;
	char hexbuf[3];
	PIP_ADAPTER_ADDRESSES currAddr = addrs;
	while (currAddr) {
		if(currAddr->PhysicalAddressLength != 0 && currAddr->IfType != IF_TYPE_TUNNEL && currAddr->IfType != IF_TYPE_PPP && currAddr->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
			if(!firstAddress) {
				result.append(",");
			}
			firstAddress = false;
			bool firstByte = true;
            for (int i = 0; i < (int) currAddr->PhysicalAddressLength; i++) {
                if (!firstByte) {
                	result.append("-");
                }
                firstByte = false;
				snprintf(hexbuf, 3, "%02X", (int) currAddr->PhysicalAddress[i]);
				result.append(hexbuf);
            }
		}
		currAddr = currAddr->Next;
	}

	free((void*)addrs);
	return replaceStringReturnBuffer(result);
}
