/*
 * CConnection.cc
 *
 * Copyright 2014-2106 D. Mitch Bailey  d.mitch.bailey at gmail dot com
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

#include "CConnection.hh"
#include "CCvcDb.hh"

float CFullConnection::EstimatedCurrent() {
	voltage_t myMaxVoltage = max(simSourceVoltage, simDrainVoltage);
	voltage_t myMinVoltage = min(simSourceVoltage, simDrainVoltage);
	bool myVthFlag = ((IsNmos_(device_p->model_p->type) && simGateVoltage - myMinVoltage == device_p->model_p->Vth) ||
			(IsPmos_(device_p->model_p->type) && simGateVoltage - myMaxVoltage == device_p->model_p->Vth));
	return EstimatedCurrent(myVthFlag);
}

float CFullConnection::EstimatedMinimumCurrent() {
	if ( minSourceVoltage == UNKNOWN_VOLTAGE || minDrainVoltage == UNKNOWN_VOLTAGE || maxSourceVoltage == UNKNOWN_VOLTAGE || maxDrainVoltage == UNKNOWN_VOLTAGE ) return(0);
	voltage_t myMaxMinVoltage, myMinMaxVoltage;
	resistance_t myMinResistance, myMaxResistance;
	if (minSourceVoltage > minDrainVoltage) {
		myMaxMinVoltage = minSourceVoltage;
		myMaxResistance = masterMinSourceNet.finalResistance;
	} else {
		myMaxMinVoltage = minDrainVoltage;
		myMaxResistance = masterMinDrainNet.finalResistance;
	}
	if (maxSourceVoltage < maxDrainVoltage) {
		myMinMaxVoltage = maxSourceVoltage;
		myMinResistance = masterMaxSourceNet.finalResistance;
	} else {
		myMinMaxVoltage = maxDrainVoltage;
		myMinResistance = masterMaxDrainNet.finalResistance;
	}
	if ( myMaxMinVoltage <= myMinMaxVoltage ) return (0);
	bool myVthFlag = ((IsNmos_(device_p->model_p->type) && maxGateVoltage - myMinMaxVoltage == device_p->model_p->Vth) ||
			(IsPmos_(device_p->model_p->type) && minGateVoltage - myMaxMinVoltage == device_p->model_p->Vth));
	if ( IsNmos_(device_p->model_p->type) && maxGateVoltage != UNKNOWN_VOLTAGE ) {
		myMaxMinVoltage = max(myMinMaxVoltage, min(maxGateVoltage, myMaxMinVoltage));
	} else if ( IsPmos_(device_p->model_p->type) && minGateVoltage != UNKNOWN_VOLTAGE ) {
		myMinMaxVoltage = min(myMaxMinVoltage, max(minGateVoltage, myMinMaxVoltage));
	}
	return float(myMaxMinVoltage - myMinMaxVoltage) / (resistance + myMaxResistance + myMinResistance) / VOLTAGE_SCALE / ( (myVthFlag) ? 1000 : 1 ) ;
}

float CConnection::EstimatedMosDiodeCurrent(voltage_t theSourceVoltage, CConnection & theConnections) {
/*	voltage_t mySourceVoltage, myDrainVoltage;
//	float myCurrent;
	mySourceVoltage = sourceVoltage;
	myDrainVoltage = drainVoltage;

	if ( IsNmos_(device_p->model_p->type) && gateVoltage != UNKNOWN_VOLTAGE ) {
		mySourceVoltage = min(sourceVoltage, drainVoltage);
		myDrainVoltage = min(gateVoltage, max(sourceVoltage, drainVoltage));
	} else if ( IsPmos_(device_p->model_p->type) && simGateVoltage != UNKNOWN_VOLTAGE ) {
		mySourceVoltage = max(simSourceVoltage, simDrainVoltage);
		myDrainVoltage = max(simGateVoltage, min(simSourceVoltage, simDrainVoltage));
	}
*/
	// mos diodes are inherently low current. multiply resistance by 100 to simulate.
	if ( gateId == drainId ) {
		if ( sourcePower_p && sourcePower_p->type[HIZ_BIT] ) return float(0);  // no error for hi-z power
		if ( sourceVoltage != theConnections.sourceVoltage ) {
			cout << "ERROR: unexpected voltage in EstimatedModeDiodeCurrent " << endl;
			cout << "device " << deviceId << "; source voltage " << sourceVoltage << "!=" << theConnections.sourceVoltage << endl;
		}
	} else if ( gateId == sourceId ) {
		if ( drainPower_p && drainPower_p->type[HIZ_BIT] ) return float(0);  // no error for hi-z power
		if ( drainVoltage != theConnections.drainVoltage ) {
			cout << "ERROR: unexpected voltage in EstimatedModeDiodeCurrent " << endl;
			cout << "device " << deviceId << "; drain voltage " << drainVoltage << "!=" << theConnections.drainVoltage << endl;
		}
	}
//	cout << "estimated current " << float(abs(theSourceVoltage-gateVoltage)) / (resistance*100 + masterSourceNet.finalResistance + masterDrainNet.finalResistance) / VOLTAGE_SCALE << endl;
	return float(abs(theSourceVoltage-gateVoltage)) / (resistance + masterSourceNet.finalResistance + masterDrainNet.finalResistance) / VOLTAGE_SCALE ;
}

float CFullConnection::EstimatedCurrent(bool theVthFlag) {
	voltage_t mySourceVoltage, myDrainVoltage;
//	float myCurrent;
	mySourceVoltage = simSourceVoltage;
	myDrainVoltage = simDrainVoltage;
	if ( IsNmos_(device_p->model_p->type) && simGateVoltage != UNKNOWN_VOLTAGE ) {
		mySourceVoltage = min(simSourceVoltage, simDrainVoltage);
		myDrainVoltage = min(simGateVoltage, max(simSourceVoltage, simDrainVoltage));
	} else if ( IsPmos_(device_p->model_p->type) && simGateVoltage != UNKNOWN_VOLTAGE ) {
		mySourceVoltage = max(simSourceVoltage, simDrainVoltage);
		myDrainVoltage = max(simGateVoltage, min(simSourceVoltage, simDrainVoltage));
	}
	return float(abs(mySourceVoltage - myDrainVoltage)) / (resistance + masterSimSourceNet.finalResistance + masterSimDrainNet.finalResistance) / VOLTAGE_SCALE / ( (theVthFlag) ? 1000 : 1 ) ;
}

bool CFullConnection::CheckTerminalMinMaxVoltages(int theCheckTerminals, bool theCheckHiZFlag ) {
	if ( theCheckTerminals & GATE ) {
		if ( minGateVoltage == UNKNOWN_VOLTAGE || maxGateVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( minGateVoltage > maxGateVoltage ) return false;
		if ( ! theCheckHiZFlag && minGatePower_p && minGatePower_p->type[HIZ_BIT] ) return false;
		if ( ! theCheckHiZFlag && maxGatePower_p && maxGatePower_p->type[HIZ_BIT] ) return false;
	}
	if ( theCheckTerminals & SOURCE ) {
		if ( minSourceVoltage == UNKNOWN_VOLTAGE || maxSourceVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( minSourceVoltage > maxSourceVoltage ) return false;
		if ( ! theCheckHiZFlag && minSourcePower_p && minSourcePower_p->type[HIZ_BIT] ) return false;
		if ( ! theCheckHiZFlag && maxSourcePower_p && maxSourcePower_p->type[HIZ_BIT] ) return false;
	}
	if ( theCheckTerminals & DRAIN ) {
		if ( minDrainVoltage == UNKNOWN_VOLTAGE || maxDrainVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( minDrainVoltage > maxDrainVoltage ) return false;
		if ( ! theCheckHiZFlag && minDrainPower_p && minDrainPower_p->type[HIZ_BIT] ) return false;
		if ( ! theCheckHiZFlag && maxDrainPower_p && maxDrainPower_p->type[HIZ_BIT] ) return false;
	}
	if ( theCheckTerminals & BULK ) {
		if ( minBulkVoltage == UNKNOWN_VOLTAGE || maxBulkVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( minBulkVoltage > maxBulkVoltage ) return false;
		if ( ! theCheckHiZFlag && minBulkPower_p && minBulkPower_p->type[HIZ_BIT] ) return false;
		if ( ! theCheckHiZFlag && maxBulkPower_p && maxBulkPower_p->type[HIZ_BIT] ) return false;
	}
	return true;
}

bool CFullConnection::CheckTerminalMinMaxLeakVoltages(int theCheckTerminals) {
	if ( theCheckTerminals & GATE ) {
		if ( minGateLeakVoltage == UNKNOWN_VOLTAGE || maxGateLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( minGateLeakVoltage > maxGateLeakVoltage ) return false;
	}
	if ( theCheckTerminals & SOURCE ) {
		if ( minSourceLeakVoltage == UNKNOWN_VOLTAGE || maxSourceLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( minSourceLeakVoltage > maxSourceLeakVoltage ) return false;
	}
	if ( theCheckTerminals & DRAIN ) {
		if ( minDrainLeakVoltage == UNKNOWN_VOLTAGE || maxDrainLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( minDrainLeakVoltage > maxDrainLeakVoltage ) return false;
	}
	if ( theCheckTerminals & BULK ) {
		if ( minBulkLeakVoltage == UNKNOWN_VOLTAGE || maxBulkLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( minBulkLeakVoltage > maxBulkLeakVoltage ) return false;
	}
	return true;
}

bool CFullConnection::CheckTerminalMinVoltages(int theCheckTerminals) {
	if ( theCheckTerminals & GATE ) {
		if ( minGateVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	if ( theCheckTerminals & SOURCE ) {
		if ( minSourceVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	if ( theCheckTerminals & DRAIN ) {
		if ( minDrainVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	if ( theCheckTerminals & BULK ) {
		if ( minBulkVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	return true;
}

bool CFullConnection::CheckTerminalMaxVoltages(int theCheckTerminals) {
	if ( theCheckTerminals & GATE ) {
		if ( maxGateVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	if ( theCheckTerminals & SOURCE ) {
		if ( maxSourceVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	if ( theCheckTerminals & DRAIN ) {
		if ( maxDrainVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	if ( theCheckTerminals & BULK ) {
		if ( maxBulkVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	return true;
}

bool CFullConnection::CheckTerminalSimVoltages(int theCheckTerminals) {
	if ( theCheckTerminals & GATE ) {
		if ( simGateVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	if ( theCheckTerminals & SOURCE ) {
		if ( simSourceVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	if ( theCheckTerminals & DRAIN ) {
		if ( simDrainVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	if ( theCheckTerminals & BULK ) {
		if ( simBulkVoltage == UNKNOWN_VOLTAGE ) return false;
	}
	return true;
}

void CFullConnection::SetUnknownVoltageToSim() {
	if ( simBulkVoltage != UNKNOWN_VOLTAGE ) {
		if ( minBulkVoltage == UNKNOWN_VOLTAGE ) minBulkVoltage = simBulkVoltage;
		if ( maxBulkVoltage == UNKNOWN_VOLTAGE ) maxBulkVoltage = simBulkVoltage;
	}
	if ( simSourceVoltage != UNKNOWN_VOLTAGE ) {
		if ( minSourceVoltage == UNKNOWN_VOLTAGE ) minSourceVoltage = simSourceVoltage;
		if ( maxSourceVoltage == UNKNOWN_VOLTAGE ) maxSourceVoltage = simSourceVoltage;
	}
	if ( simDrainVoltage != UNKNOWN_VOLTAGE ) {
		if ( minDrainVoltage == UNKNOWN_VOLTAGE ) minDrainVoltage = simDrainVoltage;
		if ( maxDrainVoltage == UNKNOWN_VOLTAGE ) maxDrainVoltage = simDrainVoltage;
	}
	if ( simGateVoltage != UNKNOWN_VOLTAGE ) {
		if ( minGateVoltage == UNKNOWN_VOLTAGE ) minGateVoltage = simGateVoltage;
		if ( maxGateVoltage == UNKNOWN_VOLTAGE ) maxGateVoltage = simGateVoltage;
	}
}

void CFullConnection::SetUnknownVoltage() {
	if ( simBulkVoltage != UNKNOWN_VOLTAGE ) {
		if ( minBulkVoltage == UNKNOWN_VOLTAGE ) minBulkVoltage = simBulkVoltage;
		if ( maxBulkVoltage == UNKNOWN_VOLTAGE ) maxBulkVoltage = simBulkVoltage;
	}
	if ( simSourceVoltage != UNKNOWN_VOLTAGE ) {
		if ( minSourceVoltage == UNKNOWN_VOLTAGE ) minSourceVoltage = simSourceVoltage;
		if ( maxSourceVoltage == UNKNOWN_VOLTAGE ) maxSourceVoltage = simSourceVoltage;
	}
	if ( simDrainVoltage != UNKNOWN_VOLTAGE ) {
		if ( minDrainVoltage == UNKNOWN_VOLTAGE ) minDrainVoltage = simDrainVoltage;
		if ( maxDrainVoltage == UNKNOWN_VOLTAGE ) maxDrainVoltage = simDrainVoltage;
	}
	if ( simGateVoltage != UNKNOWN_VOLTAGE ) {
		if ( minGateVoltage == UNKNOWN_VOLTAGE ) minGateVoltage = simGateVoltage;
		if ( maxGateVoltage == UNKNOWN_VOLTAGE ) maxGateVoltage = simGateVoltage;
	}
	if ( minBulkVoltage != UNKNOWN_VOLTAGE && maxBulkVoltage == UNKNOWN_VOLTAGE ) maxBulkVoltage = minBulkVoltage;
	if ( minSourceVoltage != UNKNOWN_VOLTAGE && maxSourceVoltage == UNKNOWN_VOLTAGE ) maxSourceVoltage = minSourceVoltage;
	if ( minDrainVoltage != UNKNOWN_VOLTAGE && maxDrainVoltage == UNKNOWN_VOLTAGE ) maxDrainVoltage = minDrainVoltage;
	if ( minGateVoltage != UNKNOWN_VOLTAGE && maxGateVoltage == UNKNOWN_VOLTAGE ) maxGateVoltage = minGateVoltage;

	if ( maxBulkVoltage != UNKNOWN_VOLTAGE && minBulkVoltage == UNKNOWN_VOLTAGE ) minBulkVoltage = maxBulkVoltage;
	if ( maxSourceVoltage != UNKNOWN_VOLTAGE && minSourceVoltage == UNKNOWN_VOLTAGE ) minSourceVoltage = maxSourceVoltage;
	if ( maxDrainVoltage != UNKNOWN_VOLTAGE && minDrainVoltage == UNKNOWN_VOLTAGE ) minDrainVoltage = maxDrainVoltage;
	if ( maxGateVoltage != UNKNOWN_VOLTAGE && minGateVoltage == UNKNOWN_VOLTAGE ) minGateVoltage = maxGateVoltage;

	if ( simBulkVoltage == UNKNOWN_VOLTAGE && minBulkVoltage == maxBulkVoltage ) simBulkVoltage = minBulkVoltage;
	if ( simSourceVoltage == UNKNOWN_VOLTAGE && minSourceVoltage == maxSourceVoltage ) simSourceVoltage = minSourceVoltage;
	if ( simDrainVoltage == UNKNOWN_VOLTAGE && minDrainVoltage == maxDrainVoltage ) simDrainVoltage = minDrainVoltage;
	if ( simGateVoltage == UNKNOWN_VOLTAGE && minGateVoltage == maxGateVoltage ) simGateVoltage = minGateVoltage;
}

void CFullConnection::SetMinMaxLeakVoltages(CCvcDb * theCvcDb) {
	minGateLeakVoltage = theCvcDb->MinLeakVoltage(gateId);
	minSourceLeakVoltage = theCvcDb->MinLeakVoltage(sourceId);
	minDrainLeakVoltage = theCvcDb->MinLeakVoltage(drainId);
	minBulkLeakVoltage = theCvcDb->MinLeakVoltage(bulkId);
	maxGateLeakVoltage = theCvcDb->MaxLeakVoltage(gateId);
	maxSourceLeakVoltage = theCvcDb->MaxLeakVoltage(sourceId);
	maxDrainLeakVoltage = theCvcDb->MaxLeakVoltage(drainId);
	maxBulkLeakVoltage = theCvcDb->MaxLeakVoltage(bulkId);
}

/*
void AddTerminalConnections(deviceId_t theFirstDevice, list<deviceId_t>& myPmosToCheck,	list<deviceId_t>& myNmosToCheck,
		list<deviceId_t>& myResistorToCheck, CNetIdVector& theFirstDevice_v, CNetIdVector& theNextDevice_v, vector<modelType_t>& theDeviceType_v) {
	for ( deviceId_t device_it = theFirstDevice; device_it != UNKNOWN_DEVICE; device_it = theNextDevice_v[device_it] ) {
//		if ( theCheckedDevices.count(device_it) == 0 ) {
//				theCheckedDevices.insert(device_it);
			switch (theDeviceType_v[device_it]) {
				case NMOS:
				case LDDN: { myNmosToCheck.push_back(device_it); break; }
				case PMOS:
				case LDDP: { myPmosToCheck.push_back(device_it); break; }
				case RESISTOR:
				case FUSE_ON:
				case FUSE_OFF: { myResistorToCheck.push_back(device_it); break; }
				default: { break; }
			}
//		}
	}
}
*/

void AddConnectedDevices(netId_t theNetId, list<deviceId_t>& myPmosToCheck,	list<deviceId_t>& myNmosToCheck,
		list<deviceId_t>& myResistorToCheck, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v, vector<modelType_t>& theDeviceType_v ) {
		for ( deviceId_t device_it = theFirstDevice_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theNextDevice_v[device_it] ) {
//			if ( theCheckedDevices.count(device_it) == 0 ) {
//				theCheckedDevices.insert(device_it);
				switch (theDeviceType_v[device_it]) {
					case NMOS:
					case LDDN: { myNmosToCheck.push_back(device_it); break; }
					case PMOS:
					case LDDP: { myPmosToCheck.push_back(device_it); break; }
					case RESISTOR:
					case FUSE_ON:
					case FUSE_OFF: { myResistorToCheck.push_back(device_it); break; }
					default: { break; }
				}
//			}
		}
}

bool CFullConnection::IsPossibleHiZ(CCvcDb * theCvcDb) {
	const int myCheckLimit = 10;
	set<netId_t> myCheckedNets;
	list<netId_t> myNetsToCheck;
	set<deviceId_t> myCheckedDevices;
	list<deviceId_t> myPmosToCheck;
	list<deviceId_t> myNmosToCheck;
	list<deviceId_t> myResistorToCheck;
	set<long> myPmosInputs;
	set<long> myNmosInputs;
	if ( theCvcDb->connectionCount_v[this->gateId].SourceDrainCount() > myCheckLimit ) return(false);
	myNetsToCheck.push_back(this->gateId);
	AddConnectedDevices(this->gateId, myPmosToCheck, myNmosToCheck, myResistorToCheck, theCvcDb->firstSource_v, theCvcDb->nextSource_v, theCvcDb->deviceType_v);
	AddConnectedDevices(this->gateId, myPmosToCheck, myNmosToCheck, myResistorToCheck, theCvcDb->firstDrain_v, theCvcDb->nextDrain_v, theCvcDb->deviceType_v);
	if ( myResistorToCheck.size() > 0 || myNmosToCheck.size() != 1 || myPmosToCheck.size() != 1 ) return(false);
	for ( netId_t net_it = theCvcDb->maxNet_v[this->gateId].nextNetId; net_it != theCvcDb->maxNet_v[this->gateId].finalNetId; net_it = theCvcDb->maxNet_v[net_it].nextNetId ) {
		CConnectionCount myCounts = theCvcDb->connectionCount_v[net_it];
		if ( myCounts.SourceDrainCount() != 2 || myCounts.sourceDrainType != PMOS_ONLY ) return false;
		AddConnectedDevices(net_it, myPmosToCheck, myNmosToCheck, myResistorToCheck, theCvcDb->firstSource_v, theCvcDb->nextSource_v, theCvcDb->deviceType_v);
		AddConnectedDevices(net_it, myPmosToCheck, myNmosToCheck, myResistorToCheck, theCvcDb->firstDrain_v, theCvcDb->nextDrain_v, theCvcDb->deviceType_v);
	}
	for ( netId_t net_it = theCvcDb->minNet_v[this->gateId].nextNetId; net_it != theCvcDb->minNet_v[this->gateId].finalNetId; net_it = theCvcDb->minNet_v[net_it].nextNetId ) {
		CConnectionCount myCounts = theCvcDb->connectionCount_v[net_it];
		if ( myCounts.SourceDrainCount() != 2 || myCounts.sourceDrainType != NMOS_ONLY ) return false;
		AddConnectedDevices(net_it, myPmosToCheck, myNmosToCheck, myResistorToCheck, theCvcDb->firstSource_v, theCvcDb->nextSource_v, theCvcDb->deviceType_v);
		AddConnectedDevices(net_it, myPmosToCheck, myNmosToCheck, myResistorToCheck, theCvcDb->firstDrain_v, theCvcDb->nextDrain_v, theCvcDb->deviceType_v);
	}
	netId_t myGateNet;
//	cout << "Pmos gates to check:";
//	for ( auto device_pit = myPmosToCheck.begin(); device_pit != myPmosToCheck.end(); device_pit++ ) {
//		cout << " " << *device_pit << ":" << theCvcDb->gateNet_v[*device_pit];
//	}
//	cout << endl << "Nmos gates to check:";
//	for ( auto device_pit = myNmosToCheck.begin(); device_pit != myNmosToCheck.end(); device_pit++ ) {
//		cout << " " << *device_pit << ":" << theCvcDb->gateNet_v[*device_pit];
//	}
//	cout << endl;
	for ( auto device_pit = myPmosToCheck.begin(); device_pit != myPmosToCheck.end(); device_pit++ ) {
		myGateNet = theCvcDb->gateNet_v[*device_pit];
		if ( theCvcDb->inverterNet_v[myGateNet] != UNKNOWN_NET ) {
			myPmosInputs.insert(((theCvcDb->highLow_v[myGateNet]) ? 1 : -1) * (long)theCvcDb->inverterNet_v[myGateNet]);
//			cout << "** Pmos clocked inverter input:" << myGateNet << ":" << (int(theCvcDb->highLow_v[myGateNet]) ? 1 : -1) * theCvcDb->inverterNet_v[myGateNet] << endl;
		}
	}
	for ( auto device_pit = myNmosToCheck.begin(); device_pit != myNmosToCheck.end(); device_pit++ ) {
		myGateNet = theCvcDb->gateNet_v[*device_pit];
		if ( theCvcDb->inverterNet_v[myGateNet] != UNKNOWN_NET ) {
//			cout << "** Nmos clocked inverter input:" << myGateNet << ":" << ((theCvcDb->highLow_v[myGateNet]) ? 1 : -1) * theCvcDb->inverterNet_v[myGateNet] << endl;
			if ( myPmosInputs.count(int((theCvcDb->highLow_v[myGateNet]) ? -1 : 1) * (long)theCvcDb->inverterNet_v[myGateNet]) ) return true;
		}
	}
	return(false);
}

bool CFullConnection::IsPumpCapacitor() {
	if ( device_p->model_p->type != CAPACITOR ) return false;
	if ( minSourcePower_p == maxSourcePower_p && minSourceVoltage != maxSourceVoltage ) return true;
	if ( minDrainPower_p == maxDrainPower_p && minDrainVoltage != maxDrainVoltage ) return true;
	return false;
}

