/*
 * CConnectionCount.cc
 *
 * Copyright 2014-2020 D. Mitch Bailey  cvc at shuharisystem dot com
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
	for ( auto device_it = theCvcDb_p->firstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb_p->nextSource_v[device_it] ) {
		if ( theCvcDb_p->IsSubcircuitOf(theCvcDb_p->deviceParent_v[device_it], theInstanceId) ) {
			switch( theCvcDb_p->deviceType_v[device_it] ) {
			case NMOS:
			case LDDN: {
				nmosCount++;
				if ( theCvcDb_p->equivalentNet_v[theCvcDb_p->gateNet_v[device_it]] != theCvcDb_p->equivalentNet_v[theCvcDb_p->drainNet_v[device_it]] ) {
					activeNmosCount++;
				}
				break;
			}
			case PMOS:
			case LDDP: {
				pmosCount++;
				if ( theCvcDb_p->equivalentNet_v[theCvcDb_p->gateNet_v[device_it]] != theCvcDb_p->equivalentNet_v[theCvcDb_p->drainNet_v[device_it]] ) {
					activePmosCount++;
				}
				break;
			}
			case RESISTOR: { resistorCount++; break; }
			case CAPACITOR: { capacitorCount++; break; }
			case DIODE: { diodeCount++; break; }
			default: break;
			}
		}
	}
	for ( auto device_it = theCvcDb_p->firstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb_p->nextDrain_v[device_it] ) {
		if ( theCvcDb_p->IsSubcircuitOf(theCvcDb_p->deviceParent_v[device_it], theInstanceId) ) {
			if ( theCvcDb_p->equivalentNet_v[theCvcDb_p->sourceNet_v[device_it]] != theCvcDb_p->equivalentNet_v[theCvcDb_p->drainNet_v[device_it]] ) {
				// only count devices with source != drain (avoid double count)
				switch( theCvcDb_p->deviceType_v[device_it] ) {
				case NMOS:
				case LDDN: {
					nmosCount++;
					if ( theCvcDb_p->equivalentNet_v[theCvcDb_p->gateNet_v[device_it]] != theCvcDb_p->equivalentNet_v[theCvcDb_p->sourceNet_v[device_it]] ) {
						activeNmosCount++;
					}
					break;
				}
				case PMOS:
				case LDDP: {
					pmosCount++;
					if ( theCvcDb_p->equivalentNet_v[theCvcDb_p->gateNet_v[device_it]] != theCvcDb_p->equivalentNet_v[theCvcDb_p->sourceNet_v[device_it]] ) {
						activePmosCount++;
					}
					break;
				}
				case RESISTOR: { resistorCount++; break; }
				case CAPACITOR: { capacitorCount++; break; }
				case DIODE: { diodeCount++; break; }
				default: break;
				}
			}
		}
	}
	for ( auto device_it = theCvcDb_p->firstGate_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb_p->nextGate_v[device_it] ) {
		if ( theCvcDb_p->IsSubcircuitOf(theCvcDb_p->deviceParent_v[device_it], theInstanceId) ) {
			if ( theCvcDb_p->equivalentNet_v[theCvcDb_p->sourceNet_v[device_it]] != theCvcDb_p->equivalentNet_v[theCvcDb_p->drainNet_v[device_it]] ) {
				// does not count mos capacitors
				switch( theCvcDb_p->deviceType_v[device_it] ) {
				case NMOS:
				case LDDN: { nmosGateCount++; break; }
				case PMOS:
				case LDDP: { pmosGateCount++; break; }
				default: break;
				}
			}
		}
	}
/*
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
	cout << "NMOS: " << activeNmosCount << "/" << nmosCount << "; PMOS: " << activePmosCount << "/" << pmosCount;
	cout << "; RESISTOR: " << resistorCount << "; CAPACITOR: " << capacitorCount << "; DIODE: " << diodeCount << endl;
	cout << "Gate counts" << endl;
	cout << "NMOS: " << nmosGateCount << "; PMOS: " << pmosGateCount << endl;
//	cout << "Bulk counts" << endl;
//	cout << "NMOS: " << nmosBulkCount << "; PMOS: " << pmosBulkCount << endl;
}
