#include "broadcastAddrs.hpp"

#define _WIN32_WINNT 0x0501
#include <Iphlpapi.h>
#include <cstdlib>

using namespace boost::asio::ip;
using namespace std;

std::vector<address_v4> findLocalBroadcastAddresses() {
	std::vector<address_v4> result;
	PMIB_IPADDRTABLE pTable = (PMIB_IPADDRTABLE) malloc(sizeof(MIB_IPADDRTABLE));
	DWORD dwSize = 0;

	if(!pTable) {
		return result;
	}

	// The size of pTable is probably too small, so this call will tell us how much memory we actually need.
	if (GetIpAddrTable(pTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
		free(pTable);
		pTable = (PMIB_IPADDRTABLE) malloc(dwSize);
		if(!pTable) {
			return result;
		}
	}

	if (GetIpAddrTable(pTable, &dwSize, 0) != NO_ERROR) {
		free(pTable);
		return result;
	}

	for(DWORD i=0; i<pTable->dwNumEntries; i++) {
		if(!(pTable->table[i].wType & (0x0008 | 0x0040))) // MIB_IPADDR_DISCONNECTED | MIB_IPADDR_DELETED
			result.push_back(address_v4(ntohl(pTable->table[i].dwAddr | ~pTable->table[i].dwMask)));
	}

	free(pTable);
	return result;
}
