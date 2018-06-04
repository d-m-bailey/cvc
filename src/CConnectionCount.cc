/*
 * CConnectionCount.cc
 *
 * Copyright 2014-2107 D. Mitch Bailey  cvc at shuharisystem dot com
 *
 * This file is part of cvc.
 *
 * cvc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cvc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cvc.  If not, see <http://www.gnu.org/licenses/>.
 *
 * You can download cvc from https://github.com/d-m-bailey/cvc.git
 */

#include "CConnectionCount.hh"

#include "CCvcDb.hh"

CDeviceCount::CDeviceCount(netId_t theNetId, CCvcDb * theCvcDb_p, instanceId_t theInstanceId) {
// count devices attached to theNet
	netId = theNetId;
	uintmax_t index_it = theCvcDb_p->firstDeviceIndex_v[theNetId];
	do {
		deviceId_t device_it = theCvcDb_p->connectedDevice_v[index_it];
		if ( theCvcDb_p->IsSubcircuitOf(theCvcDb_p->deviceParent_v[device_it], theInstanceId) ) {
			switch( theCvcDb_p->deviceType_v[device_it] ) {
			case NMOS:
			case LDDN: {
				if ( theCvcDb_p->sourceNet_v[device_it] == theNetId || theCvcDb_p->drainNet_v[device_it] == theNetId ) nmosCount++;
				if ( theCvcDb_p->gateNet_v[device_it] == theNetId ) nmosGateCount++;
				if ( theCvcDb_p->bulkNet_v[device_it] == theNetId ) nmosBulkCount++;
				break;
			}
			case PMOS:
			case LDDP: {
				if ( theCvcDb_p->sourceNet_v[device_it] == theNetId || theCvcDb_p->drainNet_v[device_it] == theNetId ) pmosCount++;
				if ( theCvcDb_p->gateNet_v[device_it] == theNetId ) pmosGateCount++;
				if ( theCvcDb_p->bulkNet_v[device_it] == theNetId ) pmosBulkCount++;
				break;
			}
			case RESISTOR: {
				if ( theCvcDb_p->sourceNet_v[device_it] == theNetId || theCvcDb_p->drainNet_v[device_it] == theNetId ) resistorCount++;
				break;
			}
			case CAPACITOR: {
				if ( theCvcDb_p->sourceNet_v[device_it] == theNetId || theCvcDb_p->drainNet_v[device_it] == theNetId ) capacitorCount++;
				break;
			}
			case DIODE: {
				if ( theCvcDb_p->sourceNet_v[device_it] == theNetId || theCvcDb_p->drainNet_v[device_it] == theNetId ) diodeCount++;
				break;
			}
			default: break;
			}
		}
		index_it++;
	} while (theCvcDb_p->connectedDevice_v[index_it-1] > theCvcDb_p->connectedDevice_v[index_it]);
	/*
	for ( auto device_it = theCvcDb_p->firstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb_p->nextSource_v[device_it] ) {
		if ( theCvcDb_p->IsSubcircuitOf(theCvcDb_p->deviceParent_v[device_it], theInstanceId) ) {
			switch( theCvcDb_p->deviceType_v[device_it] ) {
			case NMOS:
			case LDDN: { nmosCount++; break; }
			case PMOS:
			case LDDP: { pmosCount++; break; }
			case RESISTOR: { resistorCount++; break; }
			case CAPACITOR: { capacitorCount++; break; }
			case DIODE: { diodeCount++; break; }
			default: break;
			}
		}
	}
	for ( auto device_it = theCvcDb_p->firstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb_p->nextDrain_v[device_it] ) {
		if ( theCvcDb_p->IsSubcircuitOf(theCvcDb_p->deviceParent_v[device_it], theInstanceId) ) {
			switch( theCvcDb_p->deviceType_v[device_it] ) {
			case NMOS:
			case LDDN: { nmosCount++; break; }
			case PMOS:
			case LDDP: { pmosCount++; break; }
			case RESISTOR: { resistorCount++; break; }
			case CAPACITOR: { capacitorCount++; break; }
			case DIODE: { diodeCount++; break; }
			default: break;
			}
		}
	}
	for ( auto device_it = theCvcDb_p->firstGate_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb_p->nextGate_v[device_it] ) {
		if ( theCvcDb_p->IsSubcircuitOf(theCvcDb_p->deviceParent_v[device_it], theInstanceId) ) {
			switch( theCvcDb_p->deviceType_v[device_it] ) {
			case NMOS:
			case LDDN: { nmosGateCount++; break; }
			case PMOS:
			case LDDP: { pmosGateCount++; break; }
			default: break;
			}
		}
	}
	for ( auto device_it = theCvcDb_p->firstBulk_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb_p->nextBulk_v[device_it] ) {
		if ( theCvcDb_p->IsSubcircuitOf(theCvcDb_p->deviceParent_v[device_it], theInstanceId) ) {
			switch( theCvcDb_p->deviceType_v[device_it] ) {
			case NMOS:
			case LDDN: { nmosBulkCount++; break; }
			case PMOS:
			case LDDP: { pmosBulkCount++; break; }
			default: break;
			}
		}
	}
	*/
}

void CDeviceCount::Print(CCvcDb * theCvcDb_p) {
	cout << "Device counts for net " << theCvcDb_p->NetName(netId) << endl;
	cout << "Source/Drain counts" << endl;
	cout << "NMOS: " << nmosCount << "; PMOS: " << pmosCount << "; RESISTOR: " << resistorCount << "; CAPACITOR: " << capacitorCount << "; DIODE: " << diodeCount << endl;
	cout << "Gate counts" << endl;
	cout << "NMOS: " << nmosGateCount << "; PMOS: " << pmosGateCount << endl;
	cout << "Bulk counts" << endl;
	cout << "NMOS: " << nmosBulkCount << "; PMOS: " << pmosBulkCount << endl;
}
