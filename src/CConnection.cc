/*
 * CConnection.cc
 *
 * Copyright 2014-2018 D. Mitch Bailey  cvc at shuharisystem dot com
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
	bool mySelfShort = false;  // for voltage drops across mosfet
	if (minSourceVoltage > minDrainVoltage) {
		if ( minSourcePower_p->type[MIN_CALCULATED_BIT] && minSourcePower_p->defaultMinNet == drainId ) mySelfShort = true;
		myMaxMinVoltage = minSourceVoltage;
		myMaxResistance = masterMinSourceNet.finalResistance;
	} else {
		if ( minDrainPower_p->type[MIN_CALCULATED_BIT] && minDrainPower_p->defaultMinNet == sourceId ) mySelfShort = true;
		myMaxMinVoltage = minDrainVoltage;
		myMaxResistance = masterMinDrainNet.finalResistance;
	}
	if (maxSourceVoltage < maxDrainVoltage) {
		if ( maxSourcePower_p->type[MAX_CALCULATED_BIT] && maxSourcePower_p->defaultMaxNet == drainId ) mySelfShort = true;
		myMinMaxVoltage = maxSourceVoltage;
		myMinResistance = masterMaxSourceNet.finalResistance;
	} else {
		if ( maxDrainPower_p->type[MAX_CALCULATED_BIT] && maxDrainPower_p->defaultMaxNet == sourceId ) mySelfShort = true;
		myMinMaxVoltage = maxDrainVoltage;
		myMinResistance = masterMaxDrainNet.finalResistance;
	}
	if ( myMaxMinVoltage <= myMinMaxVoltage ) return (0);
	bool myVthFlag = ( ( IsNmos_(device_p->model_p->type) &&
				( maxGateVoltage - myMinMaxVoltage == device_p->model_p->Vth ||
						myMinMaxVoltage - myMaxMinVoltage <= device_p->model_p->Vth ) ) ||
			( IsPmos_(device_p->model_p->type) &&
				( minGateVoltage - myMaxMinVoltage == device_p->model_p->Vth ||
						myMaxMinVoltage - myMinMaxVoltage >= device_p->model_p->Vth ) ) );
	if ( mySelfShort && myVthFlag ) return (0);  // zero current for calculated voltage drops
	if ( IsNmos_(device_p->model_p->type) && maxGateVoltage != UNKNOWN_VOLTAGE ) {
		myMaxMinVoltage = max(myMinMaxVoltage, min(maxGateVoltage, myMaxMinVoltage));
	} else if ( IsPmos_(device_p->model_p->type) && minGateVoltage != UNKNOWN_VOLTAGE ) {
		myMinMaxVoltage = min(myMaxMinVoltage, max(minGateVoltage, myMinMaxVoltage));
	}
	return float(myMaxMinVoltage - myMinMaxVoltage) / (resistance + myMaxResistance + myMinResistance) / VOLTAGE_SCALE / ( (myVthFlag) ? 1000 : 1 ) ;
}

float CConnection::EstimatedMosDiodeCurrent(voltage_t theSourceVoltage, CConnection & theConnections) {
	// mos diodes are inherently low current. multiply resistance by 100 to simulate.
	if ( gateId == drainId ) {
		if ( sourcePower_p && sourcePower_p->type[HIZ_BIT] ) return float(0);  // no error for hi-z power
		if ( sourceVoltage != theConnections.sourceVoltage ) {
			if (gDebug_cvc) cout << "ERROR: unexpected voltage in EstimatedMosDiodeCurrent " << endl;
			if (gDebug_cvc) cout << "device " << deviceId << "; source voltage " << sourceVoltage << "!=" << theConnections.sourceVoltage << endl;
		}
	} else if ( gateId == sourceId ) {
		if ( drainPower_p && drainPower_p->type[HIZ_BIT] ) return float(0);  // no error for hi-z power
		if ( drainVoltage != theConnections.drainVoltage ) {
			if (gDebug_cvc) cout << "ERROR: unexpected voltage in EstimatedMosDiodeCurrent " << endl;
			if (gDebug_cvc) cout << "device " << deviceId << "; drain voltage " << drainVoltage << "!=" << theConnections.drainVoltage << endl;
		}
	}
//	cout << "estimated current " << float(abs(theSourceVoltage-gateVoltage)) / (resistance*100 + masterSourceNet.finalResistance + masterDrainNet.finalResistance) / VOLTAGE_SCALE << endl;
	return float(abs(theSourceVoltage-gateVoltage)) / (resistance + masterSourceNet.finalResistance + masterDrainNet.finalResistance) / VOLTAGE_SCALE ;
}

float CConnection::EstimatedCurrent(voltage_t theSourceVoltage, voltage_t theDrainVoltage, resistance_t theSourceResistance, resistance_t theDrainResistance) {
	return float(abs(theSourceVoltage - theDrainVoltage)) / (resistance + theSourceResistance + theDrainResistance) / VOLTAGE_SCALE ;
}

float CFullConnection::EstimatedCurrent(bool theVthFlag) {
	voltage_t mySourceVoltage, myDrainVoltage;
//	float myCurrent;
	// ignore leaks through mos devices where gate is connected to power and difference between source and drain is exactly Vth
	// and at least one of the source or drain is a calculated net
	// this prevents false short errors in transfer gates and complex logic gates
	if ( simGatePower_p && ! simGatePower_p->type[SIM_CALCULATED_BIT] && theVthFlag \
			&& (simDrainPower_p->type[SIM_CALCULATED_BIT] || simSourcePower_p->type[SIM_CALCULATED_BIT]) ) return 0;
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

void CFullConnection::SetMinMaxLeakVoltagesAndFlags(CCvcDb * theCvcDb) {
	minGateLeakVoltage = theCvcDb->MinLeakVoltage(gateId);
	minSourceLeakVoltage = theCvcDb->MinLeakVoltage(sourceId);
	minDrainLeakVoltage = theCvcDb->MinLeakVoltage(drainId);
	minBulkLeakVoltage = theCvcDb->MinLeakVoltage(bulkId);
	maxGateLeakVoltage = theCvcDb->MaxLeakVoltage(gateId);
	maxSourceLeakVoltage = theCvcDb->MaxLeakVoltage(sourceId);
	maxDrainLeakVoltage = theCvcDb->MaxLeakVoltage(drainId);
	maxBulkLeakVoltage = theCvcDb->MaxLeakVoltage(bulkId);

	validMinGate = ! ( minGateVoltage == UNKNOWN_VOLTAGE || (minGatePower_p && minGatePower_p->type[HIZ_BIT]) );
	validMinSource = ! ( minSourceVoltage == UNKNOWN_VOLTAGE || (minSourcePower_p && minSourcePower_p->type[HIZ_BIT]) );
	validMinDrain = ! ( minDrainVoltage == UNKNOWN_VOLTAGE || (minDrainPower_p && minDrainPower_p->type[HIZ_BIT]) );
	validMinBulk = ! ( minBulkVoltage == UNKNOWN_VOLTAGE || (minBulkPower_p && minBulkPower_p->type[HIZ_BIT]) );
	validSimGate = simGateVoltage != UNKNOWN_VOLTAGE;
	validSimSource = simSourceVoltage != UNKNOWN_VOLTAGE;
	validSimDrain = simDrainVoltage != UNKNOWN_VOLTAGE;
	validSimBulk = simBulkVoltage != UNKNOWN_VOLTAGE;
	validMaxGate = ! ( maxGateVoltage == UNKNOWN_VOLTAGE || (maxGatePower_p && maxGatePower_p->type[HIZ_BIT]) );
	validMaxSource = ! ( maxSourceVoltage == UNKNOWN_VOLTAGE || (maxSourcePower_p && maxSourcePower_p->type[HIZ_BIT]) );
	validMaxDrain = ! ( maxDrainVoltage == UNKNOWN_VOLTAGE || (maxDrainPower_p && maxDrainPower_p->type[HIZ_BIT]) );
	validMaxBulk = ! ( maxBulkVoltage == UNKNOWN_VOLTAGE || (maxBulkPower_p && maxBulkPower_p->type[HIZ_BIT]) );
	validMinGateLeak = minGateLeakVoltage != UNKNOWN_VOLTAGE;
	validMinSourceLeak = minSourceLeakVoltage != UNKNOWN_VOLTAGE;
	validMinDrainLeak = minDrainLeakVoltage != UNKNOWN_VOLTAGE;
	validMinBulkLeak = minBulkLeakVoltage != UNKNOWN_VOLTAGE;
	validMaxGateLeak = maxGateLeakVoltage != UNKNOWN_VOLTAGE;
	validMaxSourceLeak = maxSourceLeakVoltage != UNKNOWN_VOLTAGE;
	validMaxDrainLeak = maxDrainLeakVoltage != UNKNOWN_VOLTAGE;
	validMaxBulkLeak = maxBulkLeakVoltage != UNKNOWN_VOLTAGE;
	if ( validMinGate && validMaxGate ) {
		validMinGate = validMaxGate = minGateVoltage <= maxGateVoltage;
	}
	if ( validMinSource && validMaxSource ) {
		validMinSource = validMaxSource = minSourceVoltage <= maxSourceVoltage;
	}
	if ( validMinDrain && validMaxDrain ) {
		validMinDrain = validMaxDrain = minDrainVoltage <= maxDrainVoltage;
	}
	if ( validMinBulk && validMaxBulk ) {
		validMinBulk = validMaxBulk = minBulkVoltage <= maxBulkVoltage;
	}
	if ( validMinGateLeak && validMaxGateLeak ) {
		validMinGateLeak = validMaxGateLeak = minGateLeakVoltage <= maxGateLeakVoltage;
	}
	if ( validMinSourceLeak && validMaxSourceLeak ) {
		validMinSourceLeak = validMaxSourceLeak = minSourceLeakVoltage <= maxSourceLeakVoltage;
	}
	if ( validMinDrainLeak && validMaxDrainLeak ) {
		validMinDrainLeak = validMaxDrainLeak = minDrainLeakVoltage <= maxDrainLeakVoltage;
	}
	if ( validMinBulkLeak && validMaxBulkLeak ) {
		validMinBulkLeak = validMaxBulkLeak = minBulkLeakVoltage <= maxBulkLeakVoltage;
	}
	if ( validMinGate && validMinGateLeak ) {
		validMinGate = minGateVoltage >= minGateLeakVoltage;
	}
	if ( validMinSource && validMinSourceLeak ) {
		validMinSource = minSourceVoltage >= minSourceLeakVoltage;
	}
	if ( validMinDrain && validMinDrainLeak ) {
		validMinDrain = minDrainVoltage >= minDrainLeakVoltage;
	}
	if ( validMinBulk && validMinBulkLeak ) {
		validMinBulk = minBulkVoltage >= minBulkLeakVoltage;
	}
	if ( validMaxGate && validMaxGateLeak ) {
		validMaxGate = maxGateVoltage <= maxGateLeakVoltage;
	}
	if ( validMaxSource && validMaxSourceLeak ) {
		validMaxSource = maxSourceVoltage <= maxSourceLeakVoltage;
	}
	if ( validMaxDrain && validMaxDrainLeak ) {
		validMaxDrain = maxDrainVoltage <= maxDrainLeakVoltage;
	}
	if ( validMaxBulk && validMaxBulkLeak ) {
		validMaxBulk = maxBulkVoltage <= maxBulkLeakVoltage;
	}

	// following lines handled elsewhere
//	if ( minGateVoltage == UNKNOWN_VOLTAGE || ( minGateLeakVoltage != UNKNOWN_VOLTAGE && minGateLeakVoltage > minGateVoltage ) ) minGateVoltage = minGateLeakVoltage;
//	if ( minSourceVoltage == UNKNOWN_VOLTAGE || ( minSourceLeakVoltage != UNKNOWN_VOLTAGE && minSourceLeakVoltage > minSourceVoltage ) ) minSourceVoltage = minSourceLeakVoltage;
//	if ( minDrainVoltage == UNKNOWN_VOLTAGE || ( minDrainLeakVoltage != UNKNOWN_VOLTAGE && minDrainLeakVoltage > minDrainVoltage ) ) minDrainVoltage = minDrainLeakVoltage;
//	if ( minBulkVoltage == UNKNOWN_VOLTAGE || ( minBulkLeakVoltage != UNKNOWN_VOLTAGE && minBulkLeakVoltage > minBulkVoltage ) ) minBulkVoltage = minBulkLeakVoltage;
//	if ( maxGateVoltage == UNKNOWN_VOLTAGE || ( maxGateLeakVoltage != UNKNOWN_VOLTAGE && maxGateLeakVoltage < maxGateVoltage ) ) maxGateVoltage = maxGateLeakVoltage;
//	if ( maxSourceVoltage == UNKNOWN_VOLTAGE || ( maxSourceLeakVoltage != UNKNOWN_VOLTAGE && maxSourceLeakVoltage < maxSourceVoltage ) ) maxSourceVoltage = maxSourceLeakVoltage;
//	if ( maxDrainVoltage == UNKNOWN_VOLTAGE || ( maxDrainLeakVoltage != UNKNOWN_VOLTAGE && maxDrainLeakVoltage < maxDrainVoltage ) ) maxDrainVoltage = maxDrainLeakVoltage;
//	if ( maxBulkVoltage == UNKNOWN_VOLTAGE || ( maxBulkLeakVoltage != UNKNOWN_VOLTAGE && maxBulkLeakVoltage < maxBulkVoltage ) ) maxBulkVoltage = maxBulkLeakVoltage;
}

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
	set<string> myPmosInputs;
	set<string> myNmosInputs;
	bool myDebug = false;
	if ( theCvcDb->connectionCount_v[this->gateId].SourceDrainCount() > myCheckLimit ) return(false);
	if ( theCvcDb->netVoltagePtr_v[gateId].full && theCvcDb->netVoltagePtr_v[gateId].full->type[INPUT_BIT] ) return(false);  // input ports not possible Hi-Z
	myNetsToCheck.push_back(this->gateId);
	AddConnectedDevices(this->gateId, myPmosToCheck, myNmosToCheck, myResistorToCheck, theCvcDb->firstSource_v, theCvcDb->nextSource_v, theCvcDb->deviceType_v);
	AddConnectedDevices(this->gateId, myPmosToCheck, myNmosToCheck, myResistorToCheck, theCvcDb->firstDrain_v, theCvcDb->nextDrain_v, theCvcDb->deviceType_v);
	if ( myResistorToCheck.size() > 0 || myNmosToCheck.size() != 1 || myPmosToCheck.size() != 1 ) return(false);
	if ( theCvcDb->minNet_v[gateId].nextNetId == theCvcDb->maxNet_v[gateId].nextNetId ) {  // transfer gates
		return IsTransferGate(myNmosToCheck.front(), myPmosToCheck.front(), theCvcDb);
	} 
	// check clocked inverters
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
	if ( myDebug ) {
		cout << "For device " << deviceId << " net " << gateId << endl;
		cout << "Pmos gates to check:";
		for ( auto device_pit = myPmosToCheck.begin(); device_pit != myPmosToCheck.end(); device_pit++ ) {
			cout << " " << *device_pit << ":" << theCvcDb->gateNet_v[*device_pit];
		}
		cout << endl << "Nmos gates to check:";
		for ( auto device_pit = myNmosToCheck.begin(); device_pit != myNmosToCheck.end(); device_pit++ ) {
			cout << " " << *device_pit << ":" << theCvcDb->gateNet_v[*device_pit];
		}
		cout << endl;
	}
	for ( auto device_pit = myPmosToCheck.begin(); device_pit != myPmosToCheck.end(); device_pit++ ) {
		myGateNet = theCvcDb->gateNet_v[*device_pit];
		string myGoal = ( theCvcDb->inverterNet_v[myGateNet] == UNKNOWN_NET ) \
			? ("+" + to_string<netId_t>(myGateNet)) \
			: (((theCvcDb->highLow_v[myGateNet]) ? "+" : "-") + to_string<netId_t>(theCvcDb->inverterNet_v[myGateNet]));
		myPmosInputs.insert(myGoal);
		if (myDebug) cout << "** Pmos clocked inverter input:" << myGateNet << ":" << (theCvcDb->highLow_v[myGateNet] ? "+" : "-") << theCvcDb->inverterNet_v[myGateNet] << endl;
	}
	for ( auto device_pit = myNmosToCheck.begin(); device_pit != myNmosToCheck.end(); device_pit++ ) {
		myGateNet = theCvcDb->gateNet_v[*device_pit];
		string myGoal = ( theCvcDb->inverterNet_v[myGateNet] == UNKNOWN_NET ) \
			? ("-" + to_string<netId_t>(myGateNet)) \
			: (((theCvcDb->highLow_v[myGateNet]) ? "-" : "+") + to_string<netId_t>(theCvcDb->inverterNet_v[myGateNet]));
		if (myDebug) cout << "** Nmos clocked inverter input:" << myGateNet << ":" << (theCvcDb->highLow_v[myGateNet] ? "+" : "-") << theCvcDb->inverterNet_v[myGateNet] << endl;
		if ( myPmosInputs.count(myGoal) ) {
			CFullConnection myTristateConnections;
			theCvcDb->MapDeviceNets(*device_pit, myTristateConnections);
			if ( myTristateConnections.simGateVoltage == UNKNOWN_VOLTAGE )  return true;  // look for unknown opposite logic
		}
	}
	return(false);
}

/**
 * \brief HasParallelShort: Check connected devices for parallel short.
 *
 * Only parallel devices of the same type that are fully on are considered shorted.
 */
bool CFullConnection::HasParallelShort(CCvcDb * theCvcDb) {
	netId_t myDrainId, mySourceId;
	CFullConnection myParallelDeviceConnections;
	// Choose the least connected pin as the drain
	if ( theCvcDb->connectionCount_v[drainId].SourceDrainCount() < theCvcDb->connectionCount_v[sourceId].SourceDrainCount() ) {
		myDrainId = drainId;
		mySourceId = sourceId;
	} else {
		myDrainId = sourceId;
		mySourceId = drainId;
	}
	modelType_t myModelType = device_p->model_p->type;
	assert( IsMos_(myModelType) );
	myModelType = IsNmos_(myModelType) ? NMOS : PMOS;
	for ( auto device_it = theCvcDb->firstSource_v[myDrainId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb->nextSource_v[device_it] ) {
		if ( device_it == deviceId ) continue;  // same device
		if ( myModelType == NMOS && ! IsNmos_(theCvcDb->deviceType_v[device_it] )) continue;  // skip non-matching device types
		if ( myModelType == PMOS && ! IsPmos_(theCvcDb->deviceType_v[device_it] )) continue;  // skip non-matching device types
		if ( theCvcDb->drainNet_v[device_it] != mySourceId ) continue;  // not parallel
		theCvcDb->MapDeviceNets(device_it, myParallelDeviceConnections);
		if ( myModelType == NMOS && IsKnownVoltage_(myParallelDeviceConnections.simGateVoltage)
				&& myParallelDeviceConnections.simGateVoltage > myParallelDeviceConnections.minDrainVoltage + myParallelDeviceConnections.device_p->model_p->Vth ) {
			return true;
		} else if ( myModelType == PMOS && IsKnownVoltage_(myParallelDeviceConnections.simGateVoltage)
				&& myParallelDeviceConnections.simGateVoltage < myParallelDeviceConnections.maxDrainVoltage + myParallelDeviceConnections.device_p->model_p->Vth ) {
			return true;
		}
	}
	for ( auto device_it = theCvcDb->firstDrain_v[myDrainId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb->nextDrain_v[device_it] ) {
		if ( device_it == deviceId ) continue;  // same device
		if ( myModelType == NMOS && ! IsNmos_(theCvcDb->deviceType_v[device_it] )) continue;  // skip non-matching device types
		if ( myModelType == PMOS && ! IsPmos_(theCvcDb->deviceType_v[device_it] )) continue;  // skip non-matching device types
		if ( theCvcDb->sourceNet_v[device_it] != mySourceId ) continue;  // not parallel
		theCvcDb->MapDeviceNets(device_it, myParallelDeviceConnections);
		if ( myModelType == NMOS && IsKnownVoltage_(myParallelDeviceConnections.simGateVoltage)
				&& myParallelDeviceConnections.simGateVoltage > myParallelDeviceConnections.minSourceVoltage + myParallelDeviceConnections.device_p->model_p->Vth ) {
			return true;
		} else if ( myModelType == PMOS && IsKnownVoltage_(myParallelDeviceConnections.simGateVoltage)
				&& myParallelDeviceConnections.simGateVoltage < myParallelDeviceConnections.maxSourceVoltage + myParallelDeviceConnections.device_p->model_p->Vth ) {
			return true;
		}
	}
	return false;

}

bool CFullConnection::IsTransferGate(deviceId_t theNmos, deviceId_t thePmos, CCvcDb * theCvcDb) {
	netId_t myPmosGateNet = theCvcDb->gateNet_v[thePmos];
	string myOrigin = ( theCvcDb->inverterNet_v[myPmosGateNet] == UNKNOWN_NET )
		? ("+" + to_string<netId_t>(myPmosGateNet))
		: (((theCvcDb->highLow_v[myPmosGateNet]) ? "+" : "-") + to_string<netId_t>(theCvcDb->inverterNet_v[myPmosGateNet]));
	netId_t myNmosGateNet =  theCvcDb->gateNet_v[theNmos];
	string myInvertedOrigin = ( theCvcDb->inverterNet_v[myNmosGateNet] == UNKNOWN_NET )
		? ("-" + to_string<netId_t>(myNmosGateNet))
		: (((theCvcDb->highLow_v[myNmosGateNet]) ? "-" : "+") + to_string<netId_t>(theCvcDb->inverterNet_v[myNmosGateNet]));
	return ( myOrigin == myInvertedOrigin );
}

bool CFullConnection::IsPumpCapacitor() {
	if ( device_p->model_p->type != CAPACITOR ) return false;
	if ( minSourcePower_p == maxSourcePower_p && minSourceVoltage != maxSourceVoltage ) return true;
	if ( minDrainPower_p == maxDrainPower_p && minDrainVoltage != maxDrainVoltage ) return true;
	return false;
}

