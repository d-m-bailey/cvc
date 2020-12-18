/*
 * CCvcDb-utility.cc
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

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/mman.h>

#include "CCircuit.hh"
#include "CConnection.hh"
#include "CCvcDb.hh"
#include "CCvcExceptions.hh"
#include "CCvcParameters.hh"
#include "CDevice.hh"
#include "CEventQueue.hh"
#include "CInstance.hh"
#include "CModel.hh"
#include "CPower.hh"
#include "Cvc.hh"
#include "CvcTypes.hh"
#include "CVirtualNet.hh"

voltage_t CCvcDb::MinVoltage(netId_t theNetId, bool theSkipHiZFlag) {
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(minNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			CPower * myFinalPower_p = netVoltagePtr_v[myVirtualNet.finalNetId].full;
			if ( myFinalPower_p ) {
				if ( theSkipHiZFlag && myFinalPower_p->type[HIZ_BIT] ) return UNKNOWN_VOLTAGE;
				return myFinalPower_p->minVoltage;
			}
		}
	}
	return UNKNOWN_VOLTAGE;
}

voltage_t CCvcDb::MinSimVoltage(netId_t theNetId) {
	// limit min value to calculated sim value
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(minNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			voltage_t myMinVoltage = ( netVoltagePtr_v[myVirtualNet.finalNetId].full ) ? netVoltagePtr_v[myVirtualNet.finalNetId].full->minVoltage : UNKNOWN_VOLTAGE;
			netId_t myNetId = theNetId;
			while ( myNetId != minNet_v[myNetId].nextNetId ) {
				myNetId = minNet_v[myNetId].nextNetId;
				CPower * myPower_p = netVoltagePtr_v[myNetId].full;
				if ( myPower_p && myPower_p->type[SIM_CALCULATED_BIT] && myPower_p->simVoltage != UNKNOWN_VOLTAGE ) {
					return ((myMinVoltage == UNKNOWN_VOLTAGE) ? myPower_p->simVoltage : max(myPower_p->simVoltage, myMinVoltage));
				}
			}
			return myMinVoltage;
		}
	}
	return UNKNOWN_VOLTAGE;
}

resistance_t CCvcDb::MinResistance(netId_t theNetId) {
	// resistance to minimum master net
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(minNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			return myVirtualNet.finalResistance;
		}
	}
	return UNKNOWN_VOLTAGE;
}

voltage_t CCvcDb::MinLeakVoltage(netId_t theNetId) {
	if ( theNetId != UNKNOWN_NET && leakVoltageSet ) {
		netId_t myNetId = minNet_v[theNetId].backupNetId;
//		CVirtualNet myVirtualNet(minLeakNet_v, theNetId);
		assert(theNetId == GetEquivalentNet(theNetId));
		if ( myNetId != UNKNOWN_NET ) {
			while ( myNetId != minNet_v[myNetId].backupNetId ) {
				myNetId = minNet_v[myNetId].backupNetId;
			}
			if ( leakVoltagePtr_v[myNetId].full ) {
				return leakVoltagePtr_v[myNetId].full->minVoltage;
			}
		}
	}
	return UNKNOWN_VOLTAGE;
}

voltage_t CCvcDb::SimVoltage(netId_t theNetId) {
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(simNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			CPower * myFinalPower_p = netVoltagePtr_v[myVirtualNet.finalNetId].full;
			if ( myFinalPower_p ) {
				return myFinalPower_p->simVoltage;
			}
		}
	}
	return UNKNOWN_VOLTAGE;
}

bool CCvcDb::IsAlwaysOnCandidate(deviceId_t theDeviceId, shortDirection_t theDirection) {
	if ( theDeviceId == UNKNOWN_DEVICE ) return false;
	static CVirtualNet myMinVirtualSourceNet;
	static CVirtualNet myMinVirtualDrainNet;
	static CVirtualNet myMaxVirtualSourceNet;
	static CVirtualNet myMaxVirtualDrainNet;
	netId_t mySourceNetId = GetEquivalentNet(sourceNet_v[theDeviceId]);
	netId_t myDrainNetId = GetEquivalentNet(drainNet_v[theDeviceId]);
	if ( theDirection == DRAIN_TO_MASTER_SOURCE && connectionCount_v[myDrainNetId].sourceDrainType != NMOS_PMOS ) return true;  // non-output devices are always true
	if ( theDirection == SOURCE_TO_MASTER_DRAIN && connectionCount_v[mySourceNetId].sourceDrainType != NMOS_PMOS ) return true;  // non-output devices are always true
	myMinVirtualSourceNet(minNet_v, mySourceNetId);
	myMinVirtualDrainNet(minNet_v, myDrainNetId);
	myMaxVirtualSourceNet(maxNet_v, mySourceNetId);
	myMaxVirtualDrainNet(maxNet_v, myDrainNetId);
	netId_t myMinSourceNetId = myMinVirtualSourceNet.finalNetId;
	netId_t myMinDrainNetId = myMinVirtualDrainNet.finalNetId;
	netId_t myMaxSourceNetId = myMaxVirtualSourceNet.finalNetId;
	netId_t myMaxDrainNetId = myMaxVirtualDrainNet.finalNetId;
	if ( myMinSourceNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMinSourceNetId].full ) return false;
	if ( myMinDrainNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMinDrainNetId].full ) return false;
	if ( myMaxSourceNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMaxSourceNetId].full ) return false;
	if ( myMaxDrainNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMaxDrainNetId].full ) return false;
	voltage_t myMinSourceVoltage = netVoltagePtr_v[myMinSourceNetId].full->minVoltage;
	voltage_t myMinDrainVoltage = netVoltagePtr_v[myMinDrainNetId].full->minVoltage;
	voltage_t myMaxSourceVoltage = netVoltagePtr_v[myMaxSourceNetId].full->maxVoltage;
	voltage_t myMaxDrainVoltage = netVoltagePtr_v[myMaxDrainNetId].full->maxVoltage;
	terminal_t myTargetTerminal;
	if ( myMinSourceVoltage < myMinDrainVoltage ) {
		myTargetTerminal = SOURCE;
	} else if ( myMinSourceVoltage > myMinDrainVoltage ) {
		myTargetTerminal = DRAIN;
	} else if ( myMinSourceVoltage == myMaxSourceVoltage && myMinDrainVoltage == myMaxDrainVoltage ) {  // all voltages the same: no leak tie hi/lo
		return true;
	} else if ( myMinVirtualSourceNet.finalResistance > myMinVirtualDrainNet.finalResistance ) {
		myTargetTerminal = SOURCE;
	} else {
		myTargetTerminal = DRAIN;
	}
	if ( myMaxSourceVoltage > myMaxDrainVoltage ) {
		if ( myTargetTerminal != SOURCE ) return false;
	} else if ( myMaxSourceVoltage < myMaxDrainVoltage ) {
		if ( myTargetTerminal != DRAIN ) return false;
	} else if ( myMaxVirtualSourceNet.finalResistance > myMaxVirtualDrainNet.finalResistance ) {
		if ( myTargetTerminal != SOURCE ) return false;
	} else {
		if ( myTargetTerminal != DRAIN ) return false;
	}
	// Returns true if min/max resistance with 128 times
	if ( myTargetTerminal == SOURCE && myMinVirtualSourceNet.finalResistance < MAX_RESISTANCE >> 7 && myMaxVirtualSourceNet.finalResistance < MAX_RESISTANCE >> 7 ) {
		if ( myMinVirtualSourceNet.finalResistance << 7 > myMaxVirtualSourceNet.finalResistance
				&& myMinVirtualSourceNet.finalResistance < myMaxVirtualSourceNet.finalResistance << 7 ) {
			return true;
		}
	} else if ( myMinVirtualDrainNet.finalResistance < MAX_RESISTANCE >> 7 && myMaxVirtualDrainNet.finalResistance < MAX_RESISTANCE >> 7 ) {
		if ( myMinVirtualDrainNet.finalResistance << 7 > myMaxVirtualDrainNet.finalResistance
				&& myMinVirtualDrainNet.finalResistance < myMaxVirtualDrainNet.finalResistance << 7 ) {
			return true;
		}
	}
	return false;
}

resistance_t CCvcDb::SimResistance(netId_t theNetId) {
//	resistance_t myResistance;
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(simNet_v, theNetId);
		return myVirtualNet.finalResistance;
	}
	return MAX_RESISTANCE;
}

voltage_t CCvcDb::MaxVoltage(netId_t theNetId, bool theSkipHiZFlag) {
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(maxNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			CPower * myFinalPower_p = netVoltagePtr_v[myVirtualNet.finalNetId].full;
			if ( myFinalPower_p ) {
				if ( theSkipHiZFlag && myFinalPower_p->type[HIZ_BIT] ) return UNKNOWN_VOLTAGE;
				return myFinalPower_p->maxVoltage;
			}
		}
	}
	return UNKNOWN_VOLTAGE;
}

voltage_t CCvcDb::MaxSimVoltage(netId_t theNetId) {
	// limit max value to calculated sim value
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(maxNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			voltage_t myMaxVoltage = ( netVoltagePtr_v[myVirtualNet.finalNetId].full ) ? netVoltagePtr_v[myVirtualNet.finalNetId].full->maxVoltage : UNKNOWN_VOLTAGE;
			netId_t myNetId = theNetId;
			while ( myNetId != maxNet_v[myNetId].nextNetId ) {
				myNetId = maxNet_v[myNetId].nextNetId;
				CPower * myPower_p = netVoltagePtr_v[myNetId].full;
				if ( myPower_p && myPower_p->type[SIM_CALCULATED_BIT] && myPower_p->simVoltage != UNKNOWN_VOLTAGE ) {
					return ((myMaxVoltage == UNKNOWN_VOLTAGE) ? myPower_p->simVoltage : min(myPower_p->simVoltage, myMaxVoltage));
				}
			}
			return myMaxVoltage;
		}
	}
	return UNKNOWN_VOLTAGE;
}

resistance_t CCvcDb::MaxResistance(netId_t theNetId) {
	// resistance to maximum master net
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(maxNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			return myVirtualNet.finalResistance;
		}
	}
	return UNKNOWN_VOLTAGE;
}

voltage_t CCvcDb::MaxLeakVoltage(netId_t theNetId) {
	if ( theNetId != UNKNOWN_NET && leakVoltageSet ) {
		assert(theNetId == GetEquivalentNet(theNetId));
		netId_t myNetId = maxNet_v[theNetId].backupNetId;
		if ( myNetId != UNKNOWN_NET ) {
			while ( myNetId != maxNet_v[myNetId].backupNetId ) {
				myNetId = maxNet_v[myNetId].backupNetId;
			}
			if ( leakVoltagePtr_v[myNetId].full ) {
				return leakVoltagePtr_v[myNetId].full->maxVoltage;
			}
		}
	}
	return UNKNOWN_VOLTAGE;
}

// equivalency list is descending order loop; return node is greatest
netId_t CCvcDb::GetGreatestEquivalentNet(netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) return UNKNOWN_NET;
	while ( equivalentNet_v[theNetId] < theNetId ) {
		theNetId = equivalentNet_v[theNetId];
	}
	return equivalentNet_v[theNetId];
}

// equivalency list is descending order loop; return node is least
netId_t CCvcDb::GetLeastEquivalentNet(netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) return UNKNOWN_NET;
	while ( equivalentNet_v[theNetId] < theNetId ) {
		theNetId = equivalentNet_v[theNetId];
	}
	return theNetId;
}

netId_t CCvcDb::GetEquivalentNet(netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) return UNKNOWN_NET;
	if ( isFixedEquivalentNet ) {
		theNetId = equivalentNet_v[theNetId];
	} else {
		theNetId = GetLeastEquivalentNet(theNetId);
	}
	CPower * myPower_p = netVoltagePtr_v[theNetId].full;
	if ( myPower_p && myPower_p->netId != UNKNOWN_NET ) {
		return(myPower_p->netId);
	}
	return theNetId;
}

text_t CCvcDb::DeviceParameters(const deviceId_t theDeviceId) {
	instanceId_t myParentId = deviceParent_v[theDeviceId];
	deviceId_t myDeviceOffset = theDeviceId - instancePtr_v[myParentId]->firstDeviceId;
	return(instancePtr_v[myParentId]->master_p->devicePtr_v[myDeviceOffset]->parameters);
}


void CCvcDb::SetDeviceNets(CInstance * theInstance_p, CDevice * theDevice_p, netId_t& theSourceId, netId_t& theGateId, netId_t& theDrainId, netId_t& theBulkId) {
	// sets a element in each of the sourceNet_v, gateNet_v, drainNet_v, bulkNet_v arrays
	switch (theDevice_p->model_p->type) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			theDrainId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]]);
			theGateId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]]);
			theSourceId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]]);
			if ( cvcParameters.cvcSOI ) {
				theBulkId = UNKNOWN_NET;
			} else {
				theBulkId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[3]]);
			}
		break; }
		case DIODE: case SWITCH_OFF: case FUSE_OFF: case CAPACITOR:	case SWITCH_ON:	case FUSE_ON: case RESISTOR: {
			theSourceId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]]);
			theGateId = UNKNOWN_NET;
			theDrainId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]]);
			theBulkId = UNKNOWN_NET;
			if ( theDevice_p->signalId_v.size() == 3 ) {
				theBulkId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]]);
			} else {
				theBulkId = UNKNOWN_NET;
			}
		break; }
/*
		case NRESISTOR: case PRESISTOR: {
			theSourceId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]]);
			theGateId = UNKNOWN_NET;
			theDrainId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]]);
			theBulkId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]]);
		break; }
*/
		case BIPOLAR: /* case NPN: case PNP: */ {
			theSourceId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]]);
			theGateId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]]);
			theDrainId = GetEquivalentNet(theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]]);
			theBulkId = UNKNOWN_NET;
		break; }
		default: {
//			cout << "ERROR: Unknown model type in LinkDevices" << endl;
			theDevice_p->model_p->Print();
			throw EUnknownModel();
		}
	}
}

void CCvcDb::SetDeviceNets(deviceId_t theDeviceId, CDevice * theDevice_p, netId_t& theSourceId, netId_t& theGateId, netId_t& theDrainId, netId_t& theBulkId) {
	CInstance * myInstance_p = instancePtr_v[deviceParent_v[theDeviceId]];
	theDevice_p = myInstance_p->master_p->devicePtr_v[theDeviceId - myInstance_p->firstDeviceId];
	SetDeviceNets(myInstance_p, theDevice_p, theSourceId, theGateId, theDrainId, theBulkId);
}

void CCvcDb::MapDeviceNets(deviceId_t theDeviceId, CEventQueue& theEventQueue, CConnection& theConnections) {
	/// Get connections for device at one sample point (min/sim/max)
	CDevice * myDevice_p;
	CInstance * myInstance_p = instancePtr_v[deviceParent_v[theDeviceId]];
	myDevice_p = myInstance_p->master_p->devicePtr_v[theDeviceId - myInstance_p->firstDeviceId];
	SetConnections_(theConnections, theDeviceId);
	theConnections.masterSourceNet(theEventQueue.virtualNet_v, theConnections.sourceId);
	theConnections.masterGateNet(theEventQueue.virtualNet_v, theConnections.gateId);
	theConnections.masterDrainNet(theEventQueue.virtualNet_v, theConnections.drainId);
	theConnections.masterBulkNet(theEventQueue.virtualNet_v, theConnections.bulkId);
	if ( theEventQueue.queueType == MAX_QUEUE ) {
		theConnections.sourceVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterSourceNet.finalNetId);
		theConnections.gateVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterGateNet.finalNetId);
		theConnections.drainVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterDrainNet.finalNetId);
		theConnections.bulkVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterBulkNet.finalNetId);
	} else if ( theEventQueue.queueType == MIN_QUEUE ) {
		theConnections.sourceVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterSourceNet.finalNetId);
		theConnections.gateVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterGateNet.finalNetId);
		theConnections.drainVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterDrainNet.finalNetId);
		theConnections.bulkVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterBulkNet.finalNetId);
	} else if ( theEventQueue.queueType == SIM_QUEUE ) {
		theConnections.sourceVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSourceNet.finalNetId);
		theConnections.gateVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterGateNet.finalNetId);
		theConnections.drainVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterDrainNet.finalNetId);
		theConnections.bulkVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterBulkNet.finalNetId);
	}
	if ( theConnections.masterGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.gatePower_p = NULL;
	} else {
		theConnections.gatePower_p = netVoltagePtr_v[theConnections.masterGateNet.finalNetId].full;
	}
	theConnections.sourcePower_p = netVoltagePtr_v[theConnections.masterSourceNet.finalNetId].full;
	if ( IsSCRCPower(theConnections.sourcePower_p) ) {
		theConnections.sourceVoltage = theConnections.sourcePower_p->minVoltage;  // min = max
	}
	theConnections.drainPower_p = netVoltagePtr_v[theConnections.masterDrainNet.finalNetId].full;
	if ( IsSCRCPower(theConnections.drainPower_p) ) {
		theConnections.drainVoltage = theConnections.drainPower_p->minVoltage;  // min = max
	}
	theConnections.device_p = myDevice_p;
	theConnections.deviceId = theDeviceId;
	theConnections.resistance = parameterResistanceMap[theConnections.device_p->parameters];
}

void CCvcDb::MapDeviceNets(deviceId_t theDeviceId, CFullConnection& theConnections) {
	/// Get connections for device at all sample points (min/sim/max) from deviceId
	CInstance * myInstance_p = instancePtr_v[deviceParent_v[theDeviceId]];
	CDevice * myDevice_p = myInstance_p->master_p->devicePtr_v[theDeviceId - myInstance_p->firstDeviceId];
	MapDeviceNets(myInstance_p, myDevice_p, theConnections);
}

void CCvcDb::MapDeviceSourceDrainNets(deviceId_t theDeviceId, CFullConnection& theConnections) {
	CDevice * myDevice_p;
	CInstance * myInstance_p = instancePtr_v[deviceParent_v[theDeviceId]];
	myDevice_p = myInstance_p->master_p->devicePtr_v[theDeviceId - myInstance_p->firstDeviceId];
	SetConnections_(theConnections, theDeviceId);
	switch (myDevice_p->model_p->type) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			theConnections.originalDrainId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[0]];
			theConnections.originalSourceId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[2]];
			break; }
		case BIPOLAR: case DIODE: case SWITCH_OFF: case FUSE_OFF: case CAPACITOR:	case SWITCH_ON:	case FUSE_ON: case RESISTOR: {
			theConnections.originalSourceId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[0]];
			theConnections.originalDrainId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[1]];
			break; }
		default: {
			myDevice_p->model_p->Print();
			throw EUnknownModel();
		}
	}
	theConnections.masterMaxSourceNet(maxNet_v, theConnections.sourceId);
	theConnections.masterMaxDrainNet(maxNet_v, theConnections.drainId);
	theConnections.maxSourceVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxSourceNet.finalNetId);
	theConnections.maxDrainVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxDrainNet.finalNetId);
	theConnections.maxSourcePower_p = netVoltagePtr_v[theConnections.masterMaxSourceNet.finalNetId].full;
	theConnections.maxDrainPower_p = netVoltagePtr_v[theConnections.masterMaxDrainNet.finalNetId].full;

	theConnections.masterMinSourceNet(minNet_v, theConnections.sourceId);
	theConnections.masterMinDrainNet(minNet_v, theConnections.drainId);
	theConnections.minSourceVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinSourceNet.finalNetId);
	theConnections.minDrainVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinDrainNet.finalNetId);
	theConnections.minSourcePower_p = netVoltagePtr_v[theConnections.masterMinSourceNet.finalNetId].full;
	theConnections.minDrainPower_p = netVoltagePtr_v[theConnections.masterMinDrainNet.finalNetId].full;

	theConnections.masterSimSourceNet(simNet_v, theConnections.sourceId);
	theConnections.masterSimDrainNet(simNet_v, theConnections.drainId);
	theConnections.simSourceVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimSourceNet.finalNetId);
	theConnections.simDrainVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimDrainNet.finalNetId);
	theConnections.simSourcePower_p = netVoltagePtr_v[theConnections.masterSimSourceNet.finalNetId].full;
	theConnections.simDrainPower_p = netVoltagePtr_v[theConnections.masterSimDrainNet.finalNetId].full;

	theConnections.device_p = myDevice_p;
	theConnections.deviceId = theDeviceId;
	theConnections.resistance = parameterResistanceMap[theConnections.device_p->parameters];
}

void CCvcDb::MapDeviceNets(CInstance * theInstance_p, CDevice * theDevice_p, CFullConnection& theConnections) {
	theConnections.device_p = theDevice_p;
	theConnections.deviceId = theInstance_p->firstDeviceId + theDevice_p->offset;
	SetConnections_(theConnections, theConnections.deviceId);
	switch (theDevice_p->model_p->type) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			theConnections.originalDrainId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]];
			theConnections.originalGateId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]];
			theConnections.originalSourceId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]];
			if ( cvcParameters.cvcSOI ) {
				theConnections.originalBulkId = UNKNOWN_NET;
			} else {
				theConnections.originalBulkId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[3]];
			}
			break; }
		case DIODE: case SWITCH_OFF: case FUSE_OFF: case CAPACITOR:	case SWITCH_ON:	case FUSE_ON: case RESISTOR: {
			theConnections.originalSourceId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]];
			theConnections.originalGateId = UNKNOWN_NET;
			theConnections.originalDrainId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]];
			if ( theDevice_p->signalId_v.size() == 3 ) {
				theConnections.originalBulkId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]];;
			} else {
				theConnections.originalBulkId = UNKNOWN_NET;
			}
			break; }
		case BIPOLAR: {
			theConnections.originalSourceId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]];
			theConnections.originalGateId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]];
			theConnections.originalDrainId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]];
			theConnections.originalBulkId = UNKNOWN_NET;
			break; }
		default: {
			theDevice_p->model_p->Print();
			throw EUnknownModel();
		}
	}
	theConnections.masterMaxSourceNet(maxNet_v, theConnections.sourceId);
	theConnections.masterMaxGateNet(maxNet_v, theConnections.gateId);
	theConnections.masterMaxDrainNet(maxNet_v, theConnections.drainId);
	theConnections.masterMaxBulkNet(maxNet_v, theConnections.bulkId);
	theConnections.maxSourceVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxSourceNet.finalNetId);
	theConnections.maxGateVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxGateNet.finalNetId);
	theConnections.maxDrainVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxDrainNet.finalNetId);
	theConnections.maxBulkVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxBulkNet.finalNetId);
	theConnections.maxSourcePower_p = netVoltagePtr_v[theConnections.masterMaxSourceNet.finalNetId].full;
	theConnections.maxDrainPower_p = netVoltagePtr_v[theConnections.masterMaxDrainNet.finalNetId].full;
	if ( theConnections.masterMaxGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.maxGatePower_p = NULL;
	} else {
		theConnections.maxGatePower_p = netVoltagePtr_v[theConnections.masterMaxGateNet.finalNetId].full;
	}
	if ( theConnections.masterMaxBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.maxBulkPower_p = NULL;
	} else {
		theConnections.maxBulkPower_p = netVoltagePtr_v[theConnections.masterMaxBulkNet.finalNetId].full;
	}

	theConnections.masterMinSourceNet(minNet_v, theConnections.sourceId);
	theConnections.masterMinGateNet(minNet_v, theConnections.gateId);
	theConnections.masterMinDrainNet(minNet_v, theConnections.drainId);
	theConnections.masterMinBulkNet(minNet_v, theConnections.bulkId);
	theConnections.minSourceVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinSourceNet.finalNetId);
	theConnections.minGateVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinGateNet.finalNetId);
	theConnections.minDrainVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinDrainNet.finalNetId);
	theConnections.minBulkVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinBulkNet.finalNetId);
	theConnections.minSourcePower_p = netVoltagePtr_v[theConnections.masterMinSourceNet.finalNetId].full;
	theConnections.minDrainPower_p = netVoltagePtr_v[theConnections.masterMinDrainNet.finalNetId].full;
	if ( theConnections.masterMinGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.minGatePower_p = NULL;
	} else {
		theConnections.minGatePower_p = netVoltagePtr_v[theConnections.masterMinGateNet.finalNetId].full;
	}
	if ( theConnections.masterMinBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.minBulkPower_p = NULL;
	} else {
		theConnections.minBulkPower_p = netVoltagePtr_v[theConnections.masterMinBulkNet.finalNetId].full;
	}

	theConnections.masterSimSourceNet(simNet_v, theConnections.sourceId);
	theConnections.masterSimGateNet(simNet_v, theConnections.gateId);
	theConnections.masterSimDrainNet(simNet_v, theConnections.drainId);
	theConnections.masterSimBulkNet(simNet_v, theConnections.bulkId);
	theConnections.simSourceVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimSourceNet.finalNetId);
	theConnections.simGateVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimGateNet.finalNetId);
	theConnections.simDrainVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimDrainNet.finalNetId);
	theConnections.simBulkVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimBulkNet.finalNetId);
	theConnections.simSourcePower_p = netVoltagePtr_v[theConnections.masterSimSourceNet.finalNetId].full;
	theConnections.simDrainPower_p = netVoltagePtr_v[theConnections.masterSimDrainNet.finalNetId].full;
	if ( theConnections.masterSimGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.simGatePower_p = NULL;
	} else {
		theConnections.simGatePower_p = netVoltagePtr_v[theConnections.masterSimGateNet.finalNetId].full;
	}
	if ( theConnections.masterSimBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.simBulkPower_p = NULL;
	} else {
		theConnections.simBulkPower_p = netVoltagePtr_v[theConnections.masterSimBulkNet.finalNetId].full;
	}
	theConnections.resistance = parameterResistanceMap[theConnections.device_p->parameters];
}

void CCvcDb::IgnoreDevice(deviceId_t theDeviceId) {
	static CStatus myIgnoredDevice = 0;
	if ( myIgnoredDevice == 0 ) {
		myIgnoredDevice[MAX_INACTIVE] = true;
		myIgnoredDevice[MAX_PENDING] = false;
		myIgnoredDevice[MIN_INACTIVE] = true;
		myIgnoredDevice[MIN_PENDING] = false;
		myIgnoredDevice[SIM_INACTIVE] = true;
		myIgnoredDevice[SIM_PENDING] = false;
	}
	deviceStatus_v[theDeviceId] = myIgnoredDevice;
}

bool CCvcDb::EqualMasterNets(CVirtualNetVector& theVirtualNet_v, netId_t theFirstNetId, netId_t theSecondNetId) {
	resistance_t myFirstResistance = 0, mySecondResistance = 0;
	if ( theFirstNetId == theSecondNetId ) return true;
	while ( theFirstNetId != theVirtualNet_v[theFirstNetId].nextNetId ) {
		AddResistance(myFirstResistance, theVirtualNet_v[theFirstNetId].resistance);
		CheckResistorOverflow_(myFirstResistance, theFirstNetId, logFile);
		theFirstNetId = theVirtualNet_v[theFirstNetId].nextNetId;
	}
	while ( theSecondNetId != theVirtualNet_v[theSecondNetId].nextNetId ) {
		AddResistance(mySecondResistance, theVirtualNet_v[theSecondNetId].resistance);
		CheckResistorOverflow_(mySecondResistance, theSecondNetId, logFile);
		theSecondNetId = theVirtualNet_v[theSecondNetId].nextNetId;
	}
	return ( (theFirstNetId == theSecondNetId) && (mySecondResistance == myFirstResistance) );
}

bool CCvcDb::GateEqualsDrain(CConnection& theConnections) {
	if ( netVoltagePtr_v[theConnections.masterDrainNet.finalNetId].full == NULL ) return theConnections.gateId == theConnections.drainId;
	if ( netVoltagePtr_v[theConnections.masterSourceNet.nextNetId].full == NULL ) return theConnections.gateId == theConnections.sourceId;
	throw EDatabaseError("GateEqualsDrain: " + NetName(theConnections.gateId));
}

bool CCvcDb::PathContains(CVirtualNetVector& theSearchVector, netId_t theSearchNet, netId_t theTargetNet) {
	netId_t myTargetNet = GetEquivalentNet(theTargetNet);
	if (theSearchNet == myTargetNet) return true;
	netId_t net_it = theSearchNet;
	do {
		net_it = theSearchVector[net_it].nextNetId;
		if ( net_it != myTargetNet ) return true;
	} while ( ! theSearchVector.IsTerminal(net_it) );
	return false;
}

bool CCvcDb::PathCrosses(CVirtualNetVector& theSearchVector, netId_t theSearchNet, CVirtualNetVector& theTargetVector, netId_t theTargetNet) {
	set<netId_t>	myTargetSet;
	myTargetSet.insert(theTargetNet);
	netId_t net_it = theTargetNet;
	do {
		net_it = theTargetVector[net_it].nextNetId;
		myTargetSet.insert(net_it);
	} while ( ! theTargetVector.IsTerminal(net_it) );
	if ( myTargetSet.find(theSearchNet) != myTargetSet.end() ) return true;
	net_it = theSearchNet;
	do {
		net_it = theSearchVector[net_it].nextNetId;
		if ( myTargetSet.find(net_it) != myTargetSet.end() ) return true;
	} while ( ! theSearchVector.IsTerminal(net_it) );
	return false;
}

bool CCvcDb::HasActiveConnection(netId_t theNetId) {
	for ( deviceId_t device_it = firstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
		if ( deviceStatus_v[device_it][SIM_INACTIVE] == false ) return true;
	}
	for ( deviceId_t device_it = firstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
		if ( deviceStatus_v[device_it][SIM_INACTIVE] == false ) return true;
	}
	return false;
}

voltage_t CCvcDb::DefaultMinVoltage(CPower * thePower_p) {
	if ( thePower_p->defaultMinNet == UNKNOWN_NET ) {
		return(UNKNOWN_VOLTAGE);
	} else {
		netId_t myMinNet = minNet_v[thePower_p->defaultMinNet].finalNetId;
		CPower * myMinPower_p = netVoltagePtr_v[myMinNet].full;
		if ( myMinPower_p && myMinPower_p->minVoltage != UNKNOWN_VOLTAGE ) {
			return(myMinPower_p->minVoltage);
		} else {
			reportFile << "DEBUG: Unexpected min voltage at net " << NetName(myMinNet) << endl;
			return(UNKNOWN_VOLTAGE);
		}
	}
}

voltage_t CCvcDb::DefaultMaxVoltage(CPower * thePower_p) {
	if ( thePower_p->defaultMaxNet == UNKNOWN_NET ) {
		return(UNKNOWN_VOLTAGE);
	} else {
		netId_t myMaxNet = maxNet_v[thePower_p->defaultMaxNet].finalNetId;
		CPower * myMaxPower_p = netVoltagePtr_v[myMaxNet].full;
		if ( myMaxPower_p && myMaxPower_p->maxVoltage != UNKNOWN_VOLTAGE ) {
			return(myMaxPower_p->maxVoltage);
		} else {
			reportFile << "DEBUG: Unexpected max voltage at net " << NetName(myMaxNet) << endl;
			return(UNKNOWN_VOLTAGE);
		}
	}
}

bool CCvcDb::HasLeakPath(CFullConnection & theConnections) {
	if ( theConnections.minSourcePower_p ) {
		if ( ! theConnections.minSourcePower_p->active[MIN_ACTIVE] ) return false;
		if ( theConnections.maxDrainPower_p && theConnections.minSourcePower_p->type[HIZ_BIT] ) {
			if ( theConnections.drainId == maxNet_v[theConnections.drainId].nextNetId && theConnections.maxDrainVoltage == UNKNOWN_VOLTAGE ) return false;
//			if ( theConnections.maxDrainPower_p->type[HIZ_BIT] ) return false;
			return ! theConnections.minSourcePower_p->IsRelatedPower(theConnections.maxDrainPower_p, netVoltagePtr_v, minNet_v, maxNet_v, false);
		}
	}
	if ( theConnections.minDrainPower_p ) {
		if ( ! theConnections.minDrainPower_p->active[MIN_ACTIVE] ) return false;
		if ( theConnections.maxSourcePower_p && theConnections.minDrainPower_p->type[HIZ_BIT] ) {
			if ( theConnections.sourceId == maxNet_v[theConnections.sourceId].nextNetId && theConnections.maxSourceVoltage == UNKNOWN_VOLTAGE ) return false;
//			if ( theConnections.maxSourcePower_p->type[HIZ_BIT] ) return false;
			return ! theConnections.minDrainPower_p->IsRelatedPower(theConnections.maxSourcePower_p, netVoltagePtr_v, minNet_v, maxNet_v, false);
		}
	}
	if ( theConnections.maxSourcePower_p ) {
		if ( ! theConnections.maxSourcePower_p->active[MAX_ACTIVE] ) return false;
		if ( theConnections.minDrainPower_p && theConnections.maxSourcePower_p->type[HIZ_BIT] ) {
			if ( theConnections.drainId == minNet_v[theConnections.drainId].nextNetId && theConnections.minDrainVoltage == UNKNOWN_VOLTAGE ) return false;
//			if ( theConnections.minDrainPower_p->type[HIZ_BIT] ) return false;
			return ! theConnections.maxSourcePower_p->IsRelatedPower(theConnections.minDrainPower_p, netVoltagePtr_v, maxNet_v, minNet_v, false);
		}
	}
	if ( theConnections.maxDrainPower_p ) {
		if ( ! theConnections.maxDrainPower_p->active[MAX_ACTIVE] ) return false;
		if ( theConnections.minSourcePower_p && theConnections.maxDrainPower_p->type[HIZ_BIT] ) {
			if ( theConnections.sourceId == minNet_v[theConnections.sourceId].nextNetId && theConnections.minSourceVoltage == UNKNOWN_VOLTAGE ) return false;
//			if ( theConnections.minSourcePower_p->type[HIZ_BIT] ) return false;
			return ! theConnections.maxDrainPower_p->IsRelatedPower(theConnections.minSourcePower_p, netVoltagePtr_v, maxNet_v, minNet_v, false);
		}
	}
	if ( theConnections.simSourcePower_p && theConnections.simSourcePower_p->defaultSimNet == theConnections.drainId ) return false;  // ignore leaks to own calculated voltage
	if ( theConnections.simDrainPower_p && theConnections.simDrainPower_p->defaultSimNet == theConnections.sourceId ) return false;  // ignore leaks to own calculated voltage
	if ( theConnections.simSourceVoltage != UNKNOWN_VOLTAGE
			&& abs(theConnections.simSourceVoltage - theConnections.simDrainVoltage) < cvcParameters.cvcFloatingErrorThreshold ) return false;  // ignore leaks less than threshold
	netId_t myMinSourceNet = minNet_v[theConnections.sourceId].nextNetId;
	netId_t myMaxSourceNet = maxNet_v[theConnections.sourceId].nextNetId;
	netId_t myMinDrainNet = minNet_v[theConnections.drainId].nextNetId;
	netId_t myMaxDrainNet = maxNet_v[theConnections.drainId].nextNetId;
	CPower * myPower_p = netVoltagePtr_v[theConnections.sourceId].full;
	if ( myPower_p ) {
		if ( myPower_p->type[MIN_CALCULATED_BIT] && myPower_p->defaultMinNet != UNKNOWN_NET ) myMinSourceNet = myPower_p->defaultMinNet;
		if ( myPower_p->type[MAX_CALCULATED_BIT] && myPower_p->defaultMaxNet != UNKNOWN_NET ) myMaxSourceNet = myPower_p->defaultMaxNet;
	}
	myPower_p = netVoltagePtr_v[theConnections.drainId].full;
	if ( myPower_p ) {
		if ( myPower_p->type[MIN_CALCULATED_BIT] && myPower_p->defaultMinNet != UNKNOWN_NET ) myMinDrainNet = myPower_p->defaultMinNet;
		if ( myPower_p->type[MAX_CALCULATED_BIT] && myPower_p->defaultMaxNet != UNKNOWN_NET ) myMaxDrainNet = myPower_p->defaultMaxNet;
	}
	if ( (myMinSourceNet == theConnections.drainId || theConnections.minSourceVoltage == UNKNOWN_VOLTAGE)
			&& (myMaxSourceNet == theConnections.drainId || theConnections.maxSourceVoltage == UNKNOWN_VOLTAGE) ) return false;  // min/max path same or non-existant
//			&& connectionCount_v[theConnections.sourceId].gateCount == 0) return false;  // min/max path same with no gate connections
	if ( (myMinDrainNet == theConnections.sourceId || theConnections.minDrainVoltage == UNKNOWN_VOLTAGE)
			&& (myMaxDrainNet == theConnections.sourceId || theConnections.maxDrainVoltage == UNKNOWN_VOLTAGE) ) return false;  // min/max path same with no gate connections
//			&& connectionCount_v[theConnections.drainId].gateCount == 0) return false;  // min/max path same with no gate connections
	voltage_t myMinSourceVoltage = theConnections.minSourceVoltage;
	if ( theConnections.minSourcePower_p && theConnections.minSourcePower_p->type[MIN_CALCULATED_BIT] ) {
		myMinSourceVoltage = DefaultMinVoltage(theConnections.minSourcePower_p);
	}
	voltage_t myMaxSourceVoltage = theConnections.maxSourceVoltage;
	if ( theConnections.maxSourcePower_p && theConnections.maxSourcePower_p->type[MAX_CALCULATED_BIT] ) {
		myMaxSourceVoltage = DefaultMaxVoltage(theConnections.maxSourcePower_p);
	}
	voltage_t myMinDrainVoltage = theConnections.minDrainVoltage;
	if ( theConnections.minDrainPower_p && theConnections.minDrainPower_p->type[MIN_CALCULATED_BIT] ) {
		myMinDrainVoltage = DefaultMinVoltage(theConnections.minDrainPower_p);
	}
	voltage_t myMaxDrainVoltage = theConnections.maxDrainVoltage;
	if ( theConnections.maxDrainPower_p && theConnections.maxDrainPower_p->type[MAX_CALCULATED_BIT] ) {
		myMaxDrainVoltage = DefaultMaxVoltage(theConnections.maxDrainPower_p);
	}
	if ( myMinSourceVoltage != UNKNOWN_VOLTAGE && myMaxDrainVoltage != UNKNOWN_VOLTAGE && myMinSourceVoltage < myMaxDrainVoltage ) return true;
	if ( myMaxSourceVoltage != UNKNOWN_VOLTAGE && myMinDrainVoltage != UNKNOWN_VOLTAGE && myMaxSourceVoltage > myMinDrainVoltage ) return true;
	return false;
}

size_t CCvcDb::IncrementDeviceError(deviceId_t theDeviceId, int theErrorIndex) {
	CInstance * myInstance_p = instancePtr_v[deviceParent_v[theDeviceId]];
	CCircuit * myParent_p = myInstance_p->master_p;
	int myErrorSubIndex = 0;
	if ( theErrorIndex >= OVERVOLTAGE_VBG && theErrorIndex <= MODEL_CHECK ) {
		myErrorSubIndex = theErrorIndex - OVERVOLTAGE_VBG;
	}
	int myMFactor = CalculateMFactor(deviceParent_v[theDeviceId]);
	size_t myReturnCount = myParent_p->devicePrintCount_v[theDeviceId - myInstance_p->firstDeviceId][myErrorSubIndex] + 1;
	string myKey = "";
	deviceId_t myLimit = UNKNOWN_DEVICE;
	if ( ! IsEmpty(cvcParameters.cvcCellErrorLimitFile) ) {
		instanceId_t myAncestor = deviceParent_v[theDeviceId];
		while ( myAncestor > 0 && instancePtr_v[myAncestor]->master_p->errorLimit == UNKNOWN_DEVICE ) {
			myAncestor = instancePtr_v[myAncestor]->parentId;
		}
		if (myAncestor > 0) {
			myLimit = instancePtr_v[myAncestor]->master_p->errorLimit;
			if ( myLimit > 0 ) {
				myKey = string(instancePtr_v[myAncestor]->master_p->name) + " " + string(myParent_p->name) + " "
						+ to_string<deviceId_t>(theDeviceId - myInstance_p->firstDeviceId) + " " + to_string<int>(myErrorSubIndex);
				cellErrorCountMap[myKey]++;
				if ( cellErrorCountMap[myKey] > myLimit ) {
					myReturnCount = UNKNOWN_DEVICE;
				} else {
					myReturnCount = cellErrorCountMap[myKey];  // if limit is set, always print up to error limit
				}
			} else {  // no map entries needed if limit is 0
				myReturnCount = UNKNOWN_DEVICE;
			}
		}
	}
	if ( myLimit > 0 ) {
		myParent_p->deviceErrorCount_v[theDeviceId - myInstance_p->firstDeviceId][myErrorSubIndex] += myMFactor;
		errorCount[theErrorIndex] += myMFactor;
	}
	if ( myReturnCount < UNKNOWN_DEVICE ) {
		myParent_p->devicePrintCount_v[theDeviceId - myInstance_p->firstDeviceId][myErrorSubIndex]++;
	}
	return(myReturnCount);
}

list<string> * CCvcDb::SplitHierarchy(string theFullPath) {
	list<string> * myHierarchy = new list<string>;
	size_t myStringStart = 0;
	if ( theFullPath.substr(0, 1) == "/" ) {
		myHierarchy->push_back("");
		myStringStart = 1;
	}
	size_t myHierarchyOffset = theFullPath.find(cvcParameters.cvcHierarchyDelimiters, myStringStart);
	while ( myHierarchyOffset <= theFullPath.size() ) {
		myHierarchy->push_back(theFullPath.substr(myStringStart, myHierarchyOffset - myStringStart));
		myStringStart = myHierarchyOffset + 1;
		myHierarchyOffset = theFullPath.find(cvcParameters.cvcHierarchyDelimiters, myStringStart);
	}
	myHierarchy->push_back(theFullPath.substr(myStringStart));
	return myHierarchy;
}

void CCvcDb::SaveMinMaxLeakVoltages() {
	leakVoltageSet = true;
	cout << "Saving min/max voltages..." << endl;
	minNet_v.BackupVirtualNets();
	maxNet_v.BackupVirtualNets();
	leakVoltagePtr_v = netVoltagePtr_v;
}

void CCvcDb::SaveInitialVoltages() {
	cout << "Saving simulation voltages..." << endl;
	simNet_v.BackupVirtualNets();
	initialVoltagePtr_v = netVoltagePtr_v;
}

list<string> * CCvcDb::ExpandBusNet(string theBusName) {
	list<string> * myNetList = new(list<string>);
	size_t myBusLength = theBusName.length();
	string myBusTerminator;
	size_t myBusBegin = theBusName.find_first_of("<");
	size_t myBusDelimiter = theBusName.find_first_of(":");
	myBusTerminator = ">";
	if ( myBusBegin > myBusLength ) { // '<' not found
		myBusBegin = theBusName.find_first_of("[");
		myBusTerminator = "]";
	}
	if ( myBusBegin > myBusLength ) { // '[' not found
		myBusBegin = theBusName.find_first_of("(");
		myBusTerminator = ")";
	}
	if ( myBusBegin > myBusLength ) { // '(' not found
		myBusBegin = theBusName.find_first_of("{");
		myBusTerminator = "}";
	}
	size_t myBusEnd = theBusName.find_first_of(myBusTerminator);
	if ( myBusBegin < myBusLength && myBusDelimiter < myBusLength && myBusEnd <= myBusLength ) {
		string myBaseSignal = theBusName.substr(0, myBusBegin + 1);
		int myFirstBusIndex = from_string<int>(theBusName.substr(myBusBegin + 1, myBusDelimiter - myBusBegin - 1));
		int myLastBusIndex = from_string<int>(theBusName.substr(myBusDelimiter + 1, myBusEnd - myBusDelimiter - 1));
		for ( int myBusIndex = min(myFirstBusIndex, myLastBusIndex); myBusIndex <= max(myFirstBusIndex, myLastBusIndex); myBusIndex++ ) {
			string myPowerDefinition = myBaseSignal + to_string<int>(myBusIndex) + theBusName.substr(myBusEnd);
			if ( myFirstBusIndex < myLastBusIndex ) {
				myNetList->push_back(myPowerDefinition); // ascending
			} else {
				myNetList->push_front(myPowerDefinition); // descending
			}
		}
	} else {
		myNetList->push_front(theBusName);
	}

	return(myNetList);
}

#define EVENT_CUTOFF (1<<24)
#define LARGE_EVENT_SCALE 8

eventKey_t CCvcDb::SimKey(eventKey_t theCurrentKey, resistance_t theIncrement) {
	long myEventKey;
	if ( long(theCurrentKey) < EVENT_CUTOFF ) {
		myEventKey = theCurrentKey;
	} else {
		myEventKey = EVENT_CUTOFF + ((long(theCurrentKey) - EVENT_CUTOFF) << LARGE_EVENT_SCALE);
	}
	myEventKey += theIncrement;
	if ( myEventKey > EVENT_CUTOFF ) {
		myEventKey = EVENT_CUTOFF + ((myEventKey - EVENT_CUTOFF) >> LARGE_EVENT_SCALE);
	}
	assert(myEventKey < MAX_EVENT_TIME);
	return(myEventKey);
}

bool CCvcDb::IsDerivedFromFloating(CVirtualNetVector& theVirtualNet_v, netId_t theNetId) {
	static CVirtualNet	myVirtualNet;
	assert(theNetId == GetEquivalentNet(theNetId));
	myVirtualNet(theVirtualNet_v, theNetId);
	return( netVoltagePtr_v[myVirtualNet.finalNetId].full && netVoltagePtr_v[myVirtualNet.finalNetId].full->type[HIZ_BIT] );
}

bool CCvcDb::HasActiveConnections(netId_t theNetId) {
	for ( deviceId_t device_it = firstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
		if ( ! deviceStatus_v[device_it][SIM_INACTIVE] ) return true;
	}
	for ( deviceId_t device_it = firstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
		if ( ! deviceStatus_v[device_it][SIM_INACTIVE] ) return true;
	}
	return false;
}

size_t CCvcDb::InstanceDepth(instanceId_t theInstanceId) {
	size_t myDepth = 0;
	while ( theInstanceId != 0 ) {
		assert(myDepth < subcircuitCount);
		assert(theInstanceId != UNKNOWN_INSTANCE);
		myDepth++;
		theInstanceId = instancePtr_v[theInstanceId]->parentId;
	}
	return(myDepth);
}

bool CCvcDb::IsSubcircuitOf(instanceId_t theInstanceId, instanceId_t theParentId) {
	if ( theParentId == 0 ) return true;  // everything is subcircuit of root
	if ( theInstanceId == theParentId ) return true;
	if ( instancePtr_v[theInstanceId]->parentId == theParentId ) return true;
	size_t myCount = 0;
	while ( instancePtr_v[theInstanceId]->parentId != 0 ) {
		assert(myCount++ < subcircuitCount);
		theInstanceId = instancePtr_v[theInstanceId]->parentId;
		if ( theInstanceId == theParentId ) return true;
	}
	return false;
}

void CCvcDb::RemoveInvalidPower(netId_t theNetId, size_t & theRemovedCount) {
	if ( theNetId == UNKNOWN_NET || ! netVoltagePtr_v[theNetId].full ) return;
	if ( minNet_v[theNetId].finalNetId > theNetId ) {
		RemoveInvalidPower(minNet_v[theNetId].finalNetId, theRemovedCount);
	}
	if ( maxNet_v[theNetId].finalNetId > theNetId ) {
		RemoveInvalidPower(maxNet_v[theNetId].finalNetId, theRemovedCount);
	}
	voltage_t myMaxVoltage = MaxVoltage(theNetId, true);
	voltage_t myMinVoltage = MinVoltage(theNetId, true);
	if ( netStatus_v[theNetId][NEEDS_MIN_CHECK] && ! netStatus_v[theNetId][NEEDS_MAX_CHECK] && ! IsDerivedFromFloating(minNet_v, theNetId) ) {
		voltage_t myMinCheckVoltage = ( myMinVoltage == UNKNOWN_VOLTAGE ) ? MinLeakVoltage(theNetId) : myMinVoltage;
		// intentionally not parallel with following check. only need to check high value for unknown.
		if ( myMaxVoltage != UNKNOWN_VOLTAGE && myMinCheckVoltage < myMaxVoltage ) {
			netStatus_v[theNetId][NEEDS_MIN_CHECK] = false;
		}
	}
	if ( netStatus_v[theNetId][NEEDS_MAX_CHECK] && ! netStatus_v[theNetId][NEEDS_MIN_CHECK] && ! IsDerivedFromFloating(maxNet_v, theNetId) ) {
		voltage_t myMaxCheckVoltage = ( myMaxVoltage == UNKNOWN_VOLTAGE ) ? MaxLeakVoltage(theNetId) : myMaxVoltage;
		if ( myMaxCheckVoltage != UNKNOWN_VOLTAGE && myMaxCheckVoltage > myMinVoltage ) {
			netStatus_v[theNetId][NEEDS_MAX_CHECK] = false;
		}
	}
	if ( myMinVoltage != UNKNOWN_VOLTAGE && myMaxVoltage != UNKNOWN_VOLTAGE && myMinVoltage <= myMaxVoltage
			&& netStatus_v[theNetId][NEEDS_MIN_CHECK] && netStatus_v[theNetId][NEEDS_MAX_CHECK] ) {
		// both min and max are calculated values
		netStatus_v[theNetId][NEEDS_MIN_CHECK] = netStatus_v[theNetId][NEEDS_MAX_CHECK] = false;
	}
	if ( gDebug_cvc && netStatus_v[theNetId][NEEDS_MIN_CHECK] ) cout << "DEBUG: unverified min net " << theNetId << endl;
	if ( gDebug_cvc && netStatus_v[theNetId][NEEDS_MAX_CHECK] ) cout << "DEBUG: unverified max net " << theNetId << endl;
	if ( IsCalculatedVoltage_(netVoltagePtr_v[theNetId].full) == false ) { // handle partial power specification
		if ( netStatus_v[theNetId][NEEDS_MIN_CHECK] || netStatus_v[theNetId][NEEDS_MAX_CONNECTION] ) {
			// NEEDS_MIN_CHECK means that max prop through nmos occurred (Vth drop), but no lower voltage was found
			// NEEDS_MAX_CONNECTION means that max estimated voltage through nmos diode, does not have pull up connections
			if (gDebug_cvc) cout << "INFO: Remove max voltage at net " << theNetId << " Min: " << myMinVoltage << " Max: " << myMaxVoltage << endl;
			// maybe use default min here
			netVoltagePtr_v[theNetId].full->maxVoltage = netVoltagePtr_v[netVoltagePtr_v[theNetId].full->defaultMaxNet].full->maxVoltage;
			netStatus_v[theNetId][NEEDS_MIN_CHECK] = netStatus_v[theNetId][NEEDS_MAX_CONNECTION] = false;
		}
		if ( netStatus_v[theNetId][NEEDS_MAX_CHECK] || netStatus_v[theNetId][NEEDS_MIN_CONNECTION] ) {
			// NEEDS_MAX_CHECK means that min prop through pmos occurred (Vth step up), but no higher voltage was found
			// NEEDS_MIN_CONNECTION means that min estimated voltage through pmos diode, does not have pull down connections
			if (gDebug_cvc) cout << "INFO: Remove min voltage at net " << theNetId << " Min: " << myMinVoltage << " Max: " << myMaxVoltage << endl;
			// maybe use default max here
			netVoltagePtr_v[theNetId].full->minVoltage = netVoltagePtr_v[netVoltagePtr_v[theNetId].full->defaultMinNet].full->minVoltage;
			netStatus_v[theNetId][NEEDS_MAX_CHECK] = netStatus_v[theNetId][NEEDS_MIN_CONNECTION] = false;
		}
		return;
	}
	if ( myMinVoltage == UNKNOWN_VOLTAGE || myMaxVoltage == UNKNOWN_VOLTAGE || myMinVoltage > myMaxVoltage ||
			netStatus_v[theNetId][NEEDS_MIN_CHECK] || netStatus_v[theNetId][NEEDS_MAX_CHECK] ||
			netStatus_v[theNetId][NEEDS_MIN_CONNECTION] || netStatus_v[theNetId][NEEDS_MAX_CONNECTION] ) {
		// invalid calculation
		if (gDebug_cvc) cout << "INFO: Checking " << theNetId
				<< " MIN/MAX_CHECK " << (netStatus_v[theNetId][NEEDS_MIN_CHECK] ? "1" : "0") << "|" << (netStatus_v[theNetId][NEEDS_MAX_CHECK] ? "1" : "0")
				<< " MIN/MAX_CONNECTION " << (netStatus_v[theNetId][NEEDS_MIN_CONNECTION] ? "1" : "0") << "|" << (netStatus_v[theNetId][NEEDS_MAX_CONNECTION] ? "1" : "0") << endl;
			netStatus_v[theNetId][NEEDS_MIN_CHECK] = netStatus_v[theNetId][NEEDS_MAX_CHECK] = false;
			netStatus_v[theNetId][NEEDS_MIN_CONNECTION] = netStatus_v[theNetId][NEEDS_MAX_CONNECTION] = false;
			if ( leakVoltageSet && leakVoltagePtr_v[theNetId].full ) {
				voltage_t myMaxLeakVoltage = MaxLeakVoltage(theNetId);
				if ( myMaxLeakVoltage != UNKNOWN_VOLTAGE && myMinVoltage != UNKNOWN_VOLTAGE && myMaxVoltage != UNKNOWN_VOLTAGE
						&& myMinVoltage <= myMaxVoltage && myMaxVoltage <= myMaxLeakVoltage && myMinVoltage >= MinLeakVoltage(theNetId) ) {
					return;  // first pass leak voltages mean current calculated voltages ok.
			}
		}
		if (gDebug_cvc) cout << "INFO: Remove calculated voltage at net " << theNetId << " Min: " << myMinVoltage << " Max: " << myMaxVoltage << endl;
		if ( theNetId < instancePtr_v[0]->master_p->portCount ) {
			reportFile << "WARNING: Invalid Min/Max on top level port: " << NetName(theNetId) << " Min/Max: " << myMinVoltage << "/" << myMaxVoltage << endl;
		}
		CPower * myPower_p = netVoltagePtr_v[theNetId].full;
		if ( ! ( (myPower_p->type[MIN_CALCULATED_BIT] || myPower_p->minVoltage == UNKNOWN_VOLTAGE)
				&& (myPower_p->type[MAX_CALCULATED_BIT] || myPower_p->maxVoltage == UNKNOWN_VOLTAGE) ) ) {
			reportFile << "WARNING: ignoring setting for " << myPower_p->powerSignal() << " at ";
			myPower_p->Print(reportFile, "", NetName(theNetId));
		}
		netVoltagePtr_v[theNetId].full = NULL;
		theRemovedCount++;
		// replace deleted voltage with known voltage
		myMaxVoltage = MaxVoltage(theNetId);
		myMinVoltage = MinVoltage(theNetId);
		if ( myMinVoltage == UNKNOWN_VOLTAGE ) {
			if ( myPower_p->defaultMinNet != UNKNOWN_NET ) {
				// do not increment lastUpdate
				minNet_v.Set(theNetId, myPower_p->defaultMinNet, DEFAULT_UNKNOWN_RESISTANCE, minNet_v.lastUpdate);
				CheckResistorOverflow_(minNet_v[theNetId].finalResistance, theNetId, logFile);
			}
		}
		if ( myMaxVoltage == UNKNOWN_VOLTAGE ) {
			if ( myPower_p->defaultMaxNet != UNKNOWN_NET ) {
				// do not increment lastUpdate
				maxNet_v.Set(theNetId, myPower_p->defaultMaxNet, DEFAULT_UNKNOWN_RESISTANCE, maxNet_v.lastUpdate);
				CheckResistorOverflow_(maxNet_v[theNetId].finalResistance, theNetId, logFile);
			}
		}
		if ( ! leakVoltageSet || leakVoltagePtr_v[theNetId].full != myPower_p ) {  // delete unless leak voltage
				delete myPower_p;
		}
	}
}

calculationType_t CCvcDb::GetCalculationType(CPower * thePower_p, eventQueue_t theQueueType) {
	switch ( theQueueType ) {
	case MIN_QUEUE: { return thePower_p->minCalculationType; break; }
	case SIM_QUEUE: { return thePower_p->simCalculationType; break; }
	case MAX_QUEUE: { return thePower_p->maxCalculationType; break; }
	default: return UNKNOWN_CALCULATION;
	}
}

deviceId_t CCvcDb::GetSeriesConnectedDevice(deviceId_t theDeviceId, netId_t theNetId) {
	modelType_t myDeviceType = deviceType_v[theDeviceId];
	deviceId_t myReturnDevice = UNKNOWN_DEVICE;
	for ( deviceId_t device_it = firstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
		switch ( deviceType_v[device_it] ) {
		case SWITCH_OFF: case FUSE_OFF: case CAPACITOR: case DIODE: case BIPOLAR: {
			break;
		}
		default: {
			if ( theDeviceId == device_it ) continue;
			if ( deviceType_v[device_it] != myDeviceType ) return UNKNOWN_DEVICE;  // different type
			if ( myReturnDevice != UNKNOWN_DEVICE ) return UNKNOWN_DEVICE;  // more than one
			myReturnDevice = device_it;
		}
		}
	}
	for ( deviceId_t device_it = firstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
		switch ( deviceType_v[device_it] ) {
		case SWITCH_OFF: case FUSE_OFF: case CAPACITOR: case DIODE: case BIPOLAR: {
			break;
		}
		default: {
			if ( theDeviceId == device_it ) continue;
			if ( deviceType_v[device_it] != myDeviceType ) return UNKNOWN_DEVICE;  // different type
			if ( myReturnDevice != UNKNOWN_DEVICE ) return UNKNOWN_DEVICE;  // more than one
			myReturnDevice = device_it;
		}
		}
	}
	return(myReturnDevice);
}

void CCvcDb::Cleanup() {
	if ( logFile.is_open() ) logFile.close();
	if ( errorFile.is_open() ) errorFile.close();
	if ( debugFile.is_open() ) debugFile.close();
	RemoveLock();
	try {
		cvcParameters.cvcPowerPtrList.Clear(leakVoltagePtr_v, netVoltagePtr_v, netCount);  // defined power deleted here
		if ( gDebug_cvc ) cout << "Cleared power pointer list" << endl;
		int myDeleteCount = 0;
		for ( netId_t net_it = 0; net_it < netCount; net_it++ ) {
			CPower * myPower_p = netVoltagePtr_v[net_it].full;
			if ( leakVoltageSet ) {
				CPower * myLeakPower_p = leakVoltagePtr_v[net_it].full;
				if ( leakVoltagePtr_v.size() > net_it && myLeakPower_p && myPower_p != myLeakPower_p ) {  // unique leak power deleted here
					if ( myLeakPower_p->extraData ) {
						if ( gDebug_cvc ) cout << "DEBUG: extra data at net " << net_it << endl;
					}
					delete myLeakPower_p;
					myDeleteCount++;
				}
			}
			if ( netVoltagePtr_v.size() > net_it && myPower_p ) {  // calculated power deleted here
				if ( myPower_p->extraData ) {
					if ( gDebug_cvc ) cout << "DEBUG: extra data at net " << net_it << endl;
				}
				delete myPower_p;
				myDeleteCount++;
			}
		}
		if ( gDebug_cvc ) cout << "DEBUG: Deleted " << myDeleteCount << " power objects" << endl;
		cvcParameters.cvcExpectedLevelPtrList.Clear();
		cvcParameters.cvcPowerMacroPtrMap.Clear();
		CPower::powerDefinitionText.Clear();
		cvcParameters.cvcModelListMap.Clear();
	}
	catch (...) {  // ignore errors freeing malloc memory
		cout << "INFO: problem with memory cleanup" << endl;
	}
}

deviceId_t CCvcDb::CountBulkConnections(netId_t theNetId) {
	deviceId_t myDeviceCount = 0;
	for (deviceId_t device_it = 0; device_it < deviceCount; device_it++) {
		if ( bulkNet_v[device_it] == theNetId ) {
			myDeviceCount++;
		}
	}
	return myDeviceCount;
}

bool CCvcDb::IsAnalogNet(netId_t theNetId) {
	assert(GetEquivalentNet(theNetId) == theNetId);
	return(netStatus_v[theNetId][ANALOG]);
	CVirtualNet myMinNet;
	CVirtualNet myMaxNet;
	myMinNet(minNet_v, theNetId);
	myMaxNet(maxNet_v, theNetId);
	CPower * myMinNet_p = netVoltagePtr_v[myMinNet.finalNetId].full;
	CPower * myMaxNet_p = netVoltagePtr_v[myMaxNet.finalNetId].full;
	bool myIsAnalogNet = ( myMinNet_p && myMinNet_p->type[ANALOG_BIT] ) || ( myMaxNet_p && myMaxNet_p->type[ANALOG_BIT] );
	deviceId_t device_it = firstSource_v[theNetId];
	while ( ! myIsAnalogNet && device_it != UNKNOWN_DEVICE ) {
		if ( GetEquivalentNet(drainNet_v[device_it]) != theNetId ) {
			switch( deviceType_v[device_it] ) {
			case NMOS: case LDDN: case PMOS: case LDDP: {
				if ( GetEquivalentNet(gateNet_v[device_it]) == theNetId) myIsAnalogNet = true;
				break;
			}
			case RESISTOR: { myIsAnalogNet = true; break; }
			default: break;
			}
		}
		device_it = nextSource_v[device_it];
	}
	device_it = firstDrain_v[theNetId];
	while ( ! myIsAnalogNet && device_it != UNKNOWN_DEVICE ) {
		if ( GetEquivalentNet(sourceNet_v[device_it]) != theNetId ) {
			switch( deviceType_v[device_it] ) {
			case NMOS: case LDDN: case PMOS: case LDDP: {
				if ( GetEquivalentNet(gateNet_v[device_it]) == theNetId) myIsAnalogNet = true;
				break;
			}
			case RESISTOR: { myIsAnalogNet = true; break; }
			default: break;
			}
		}
		device_it = nextDrain_v[device_it];
	}
	return (myIsAnalogNet);
}

bool CCvcDb::IsAlwaysOff(CFullConnection& theConnections) {
	/// True if gate is always off
	theConnections.SetMinMaxLeakVoltagesAndFlags(this);
	switch (deviceType_v[theConnections.deviceId]) {
	case NMOS:
	case LDDN: {
		if ( theConnections.maxGateLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( theConnections.minSourceLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( theConnections.minDrainLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		return ( theConnections.maxGateLeakVoltage < min(theConnections.minSourceLeakVoltage, theConnections.minDrainLeakVoltage) + theConnections.device_p->model_p->Vth );
		break;
	}
	case PMOS:
	case LDDP: {
		if ( theConnections.minGateLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( theConnections.maxSourceLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( theConnections.maxDrainLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		return ( theConnections.minGateLeakVoltage > max(theConnections.maxSourceLeakVoltage, theConnections.maxDrainLeakVoltage) + theConnections.device_p->model_p->Vth );
		break;
	}
	case RESISTOR:
	case SWITCH_ON:
	case FUSE_ON:
	case FUSE_OFF: {
		return false;
		break;
	}
	case CAPACITOR:
	case SWITCH_OFF: {
		return true;
		break;
	}
	case DIODE: {
		if ( theConnections.minDrainLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		if ( theConnections.maxSourceLeakVoltage == UNKNOWN_VOLTAGE ) return false;
		return theConnections.maxSourceLeakVoltage > theConnections.minDrainLeakVoltage;
		break;
	}
	case BIPOLAR: {
		return false;
		break;
	}
	default:
		return false;
	}
}

bool CCvcDb::IsOneConnectionNet(netId_t theNetId) {
	// WARNING: Doesn't count bulk connections.
	netId_t myNetId = GetEquivalentNet(theNetId);
	deviceId_t myDevice;
	if ( firstGate_v[myNetId] != UNKNOWN_DEVICE ) {
		myDevice = firstGate_v[myNetId];
	} else if ( firstSource_v[myNetId] != UNKNOWN_DEVICE ) {
		myDevice = firstSource_v[myNetId];
	} else if ( firstDrain_v[myNetId] != UNKNOWN_DEVICE ) {
		myDevice = firstDrain_v[myNetId];
	} else {
		return true;  // nets not connected to any devices are also 'one connection', i.e. no leak.
	}
	if ( firstGate_v[myNetId] != UNKNOWN_DEVICE ) {
		if ( firstGate_v[myNetId] != myDevice || nextGate_v[myDevice] != UNKNOWN_DEVICE ) return false;
	}
	if ( firstSource_v[myNetId] != UNKNOWN_DEVICE ) {
		if ( firstSource_v[myNetId] != myDevice || nextSource_v[myDevice] != UNKNOWN_DEVICE ) return false;
	}
	if ( firstDrain_v[myNetId] != UNKNOWN_DEVICE ) {
		if ( firstDrain_v[myNetId] != myDevice || nextDrain_v[myDevice] != UNKNOWN_DEVICE ) return false;
	}
	return true;
}

void CCvcDb::SetDiodeConnections(pair<int, int> theDiodePair, CFullConnection & theConnections, CFullConnection & theDiodeConnections) {
	theDiodeConnections = theConnections;
	int myAnnode, myCathode;
	if (theConnections.device_p->model_p->baseType == "M" ) {
		switch(theDiodePair.first){
			case 1: { myAnnode = DRAIN; break; }
			case 2: { myAnnode = GATE; break; }
			case 3: { myAnnode = SOURCE; break; }
			case 4: { myAnnode = BULK; break; }
			default: {
				theConnections.device_p->model_p->Print();
				throw EInvalidTerminal("base type " + theConnections.device_p->model_p->baseType + " does not have terminal " + to_string<int>(theDiodePair.first));
			}
		}
		switch(theDiodePair.second){
			case 1: { myCathode = DRAIN; break; }
			case 2: { myCathode = GATE; break; }
			case 3: { myCathode = SOURCE; break; }
			case 4: { myCathode = BULK; break; }
			default: {
				theConnections.device_p->model_p->Print();
				throw EInvalidTerminal("base type " + theConnections.device_p->model_p->baseType + " does not have terminal " + to_string<int>(theDiodePair.first));
			}
		}
	} else if (theConnections.device_p->model_p->baseType == "D"
			|| theConnections.device_p->model_p->baseType == "R"
			|| theConnections.device_p->model_p->baseType == "C") {
		switch(theDiodePair.first){
			case 1: { myAnnode = SOURCE; break; }
			case 2: { myAnnode = DRAIN; break; }
			case 3: { myAnnode = BULK; break; }
			default: {
				theConnections.device_p->model_p->Print();
				throw EInvalidTerminal("base type " + theConnections.device_p->model_p->baseType + " does not have terminal " + to_string<int>(theDiodePair.first));
			}
		}
		switch(theDiodePair.second){
			case 1: { myCathode = SOURCE; break; }
			case 2: { myCathode = DRAIN; break; }
			case 3: { myCathode = BULK; break; }
			default: {
				theConnections.device_p->model_p->Print();
				throw EInvalidTerminal("base type " + theConnections.device_p->model_p->baseType + " does not have terminal " + to_string<int>(theDiodePair.first));
			}
		}
	} else if (theConnections.device_p->model_p->baseType == "Q" ) {
		switch(theDiodePair.first){
			case 1: { myAnnode = SOURCE; break; }
			case 2: { myAnnode = GATE; break; }
			case 3: { myAnnode = DRAIN; break; }
			default: {
				theConnections.device_p->model_p->Print();
				throw EInvalidTerminal("base type " + theConnections.device_p->model_p->baseType + " does not have terminal " + to_string<int>(theDiodePair.first));
			}
		}
		switch(theDiodePair.second){
			case 1: { myCathode = SOURCE; break; }
			case 2: { myCathode = GATE; break; }
			case 3: { myCathode = DRAIN; break; }
			default: {
				theConnections.device_p->model_p->Print();
				throw EInvalidTerminal("base type " + theConnections.device_p->model_p->baseType + " does not have terminal " + to_string<int>(theDiodePair.first));
			}
		}
	} else {
		theConnections.device_p->model_p->Print();
		throw EUnknownModel();
	}
	switch(myAnnode) {
		case SOURCE: {
			theDiodeConnections.sourceId = theConnections.sourceId;
			theDiodeConnections.masterMaxSourceNet = theConnections.masterMaxSourceNet;
			theDiodeConnections.masterMinSourceNet = theConnections.masterMinSourceNet;
			theDiodeConnections.masterSimSourceNet = theConnections.masterSimSourceNet;
			theDiodeConnections.maxSourceLeakVoltage = theConnections.maxSourceLeakVoltage;
			theDiodeConnections.maxSourcePower_p = theConnections.maxSourcePower_p;
			theDiodeConnections.maxSourceVoltage = theConnections.maxSourceVoltage;
			theDiodeConnections.minSourceLeakVoltage = theConnections.minSourceLeakVoltage;
			theDiodeConnections.minSourcePower_p = theConnections.minSourcePower_p;
			theDiodeConnections.minSourceVoltage = theConnections.minSourceVoltage;
			theDiodeConnections.originalSourceId = theConnections.originalSourceId;
			theDiodeConnections.simSourcePower_p = theConnections.simSourcePower_p;
			theDiodeConnections.simSourceVoltage = theConnections.simSourceVoltage;
			theDiodeConnections.validMaxSource = theConnections.validMaxSource;
			theDiodeConnections.validMaxSourceLeak = theConnections.validMaxSourceLeak;
			theDiodeConnections.validMinSource = theConnections.validMinSource;
			theDiodeConnections.validMinSourceLeak = theConnections.validMinSourceLeak;
			theDiodeConnections.validSimSource = theConnections.validSimSource;
			break;
		}
		case GATE: {
			theDiodeConnections.sourceId = theConnections.gateId;
			theDiodeConnections.masterMaxSourceNet = theConnections.masterMaxGateNet;
			theDiodeConnections.masterMinSourceNet = theConnections.masterMinGateNet;
			theDiodeConnections.masterSimSourceNet = theConnections.masterSimGateNet;
			theDiodeConnections.maxSourceLeakVoltage = theConnections.maxGateLeakVoltage;
			theDiodeConnections.maxSourcePower_p = theConnections.maxGatePower_p;
			theDiodeConnections.maxSourceVoltage = theConnections.maxGateVoltage;
			theDiodeConnections.minSourceLeakVoltage = theConnections.minGateLeakVoltage;
			theDiodeConnections.minSourcePower_p = theConnections.minGatePower_p;
			theDiodeConnections.minSourceVoltage = theConnections.minGateVoltage;
			theDiodeConnections.originalSourceId = theConnections.originalGateId;
			theDiodeConnections.simSourcePower_p = theConnections.simGatePower_p;
			theDiodeConnections.simSourceVoltage = theConnections.simGateVoltage;
			theDiodeConnections.validMaxSource = theConnections.validMaxGate;
			theDiodeConnections.validMaxSourceLeak = theConnections.validMaxGateLeak;
			theDiodeConnections.validMinSource = theConnections.validMinGate;
			theDiodeConnections.validMinSourceLeak = theConnections.validMinGateLeak;
			theDiodeConnections.validSimSource = theConnections.validSimGate;
			break;
		}
		case DRAIN: {
			theDiodeConnections.sourceId = theConnections.drainId;
			theDiodeConnections.masterMaxSourceNet = theConnections.masterMaxDrainNet;
			theDiodeConnections.masterMinSourceNet = theConnections.masterMinDrainNet;
			theDiodeConnections.masterSimSourceNet = theConnections.masterSimDrainNet;
			theDiodeConnections.maxSourceLeakVoltage = theConnections.maxDrainLeakVoltage;
			theDiodeConnections.maxSourcePower_p = theConnections.maxDrainPower_p;
			theDiodeConnections.maxSourceVoltage = theConnections.maxDrainVoltage;
			theDiodeConnections.minSourceLeakVoltage = theConnections.minDrainLeakVoltage;
			theDiodeConnections.minSourcePower_p = theConnections.minDrainPower_p;
			theDiodeConnections.minSourceVoltage = theConnections.minDrainVoltage;
			theDiodeConnections.originalSourceId = theConnections.originalDrainId;
			theDiodeConnections.simSourcePower_p = theConnections.simDrainPower_p;
			theDiodeConnections.simSourceVoltage = theConnections.simDrainVoltage;
			theDiodeConnections.validMaxSource = theConnections.validMaxDrain;
			theDiodeConnections.validMaxSourceLeak = theConnections.validMaxDrainLeak;
			theDiodeConnections.validMinSource = theConnections.validMinDrain;
			theDiodeConnections.validMinSourceLeak = theConnections.validMinDrainLeak;
			theDiodeConnections.validSimSource = theConnections.validSimDrain;
			break;
		}
		case BULK: {
			theDiodeConnections.sourceId = theConnections.bulkId;
			theDiodeConnections.masterMaxSourceNet = theConnections.masterMaxBulkNet;
			theDiodeConnections.masterMinSourceNet = theConnections.masterMinBulkNet;
			theDiodeConnections.masterSimSourceNet = theConnections.masterSimBulkNet;
			theDiodeConnections.maxSourceLeakVoltage = theConnections.maxBulkLeakVoltage;
			theDiodeConnections.maxSourcePower_p = theConnections.maxBulkPower_p;
			theDiodeConnections.maxSourceVoltage = theConnections.maxBulkVoltage;
			theDiodeConnections.minSourceLeakVoltage = theConnections.minBulkLeakVoltage;
			theDiodeConnections.minSourcePower_p = theConnections.minBulkPower_p;
			theDiodeConnections.minSourceVoltage = theConnections.minBulkVoltage;
			theDiodeConnections.originalSourceId = theConnections.originalBulkId;
			theDiodeConnections.simSourcePower_p = theConnections.simBulkPower_p;
			theDiodeConnections.simSourceVoltage = theConnections.simBulkVoltage;
			theDiodeConnections.validMaxSource = theConnections.validMaxBulk;
			theDiodeConnections.validMaxSourceLeak = theConnections.validMaxBulkLeak;
			theDiodeConnections.validMinSource = theConnections.validMinBulk;
			theDiodeConnections.validMinSourceLeak = theConnections.validMinBulkLeak;
			theDiodeConnections.validSimSource = theConnections.validSimBulk;
			break;
		}
		default: {
			theConnections.device_p->model_p->Print();
			throw EInvalidTerminal(to_string<int>(myAnnode) + " is not a valid terminal");
		}
	}
	switch(myCathode) {
		case SOURCE: {
			theDiodeConnections.drainId = theConnections.sourceId;
			theDiodeConnections.masterMaxDrainNet = theConnections.masterMaxSourceNet;
			theDiodeConnections.masterMinDrainNet = theConnections.masterMinSourceNet;
			theDiodeConnections.masterSimDrainNet = theConnections.masterSimSourceNet;
			theDiodeConnections.maxDrainLeakVoltage = theConnections.maxSourceLeakVoltage;
			theDiodeConnections.maxDrainPower_p = theConnections.maxSourcePower_p;
			theDiodeConnections.maxDrainVoltage = theConnections.maxSourceVoltage;
			theDiodeConnections.minDrainLeakVoltage = theConnections.minSourceLeakVoltage;
			theDiodeConnections.minDrainPower_p = theConnections.minSourcePower_p;
			theDiodeConnections.minDrainVoltage = theConnections.minSourceVoltage;
			theDiodeConnections.originalDrainId = theConnections.originalSourceId;
			theDiodeConnections.simDrainPower_p = theConnections.simSourcePower_p;
			theDiodeConnections.simDrainVoltage = theConnections.simSourceVoltage;
			theDiodeConnections.validMaxDrain = theConnections.validMaxSource;
			theDiodeConnections.validMaxDrainLeak = theConnections.validMaxSourceLeak;
			theDiodeConnections.validMinDrain = theConnections.validMinSource;
			theDiodeConnections.validMinDrainLeak = theConnections.validMinSourceLeak;
			theDiodeConnections.validSimDrain = theConnections.validSimSource;
			break;
		}
		case GATE: {
			theDiodeConnections.drainId = theConnections.gateId;
			theDiodeConnections.masterMaxDrainNet = theConnections.masterMaxGateNet;
			theDiodeConnections.masterMinDrainNet = theConnections.masterMinGateNet;
			theDiodeConnections.masterSimDrainNet = theConnections.masterSimGateNet;
			theDiodeConnections.maxDrainLeakVoltage = theConnections.maxGateLeakVoltage;
			theDiodeConnections.maxDrainPower_p = theConnections.maxGatePower_p;
			theDiodeConnections.maxDrainVoltage = theConnections.maxGateVoltage;
			theDiodeConnections.minDrainLeakVoltage = theConnections.minGateLeakVoltage;
			theDiodeConnections.minDrainPower_p = theConnections.minGatePower_p;
			theDiodeConnections.minDrainVoltage = theConnections.minGateVoltage;
			theDiodeConnections.originalDrainId = theConnections.originalGateId;
			theDiodeConnections.simDrainPower_p = theConnections.simGatePower_p;
			theDiodeConnections.simDrainVoltage = theConnections.simGateVoltage;
			theDiodeConnections.validMaxDrain = theConnections.validMaxGate;
			theDiodeConnections.validMaxDrainLeak = theConnections.validMaxGateLeak;
			theDiodeConnections.validMinDrain = theConnections.validMinGate;
			theDiodeConnections.validMinDrainLeak = theConnections.validMinGateLeak;
			theDiodeConnections.validSimDrain = theConnections.validSimGate;
			break;
		}
		case DRAIN: {
			theDiodeConnections.drainId = theConnections.drainId;
			theDiodeConnections.masterMaxDrainNet = theConnections.masterMaxDrainNet;
			theDiodeConnections.masterMinDrainNet = theConnections.masterMinDrainNet;
			theDiodeConnections.masterSimDrainNet = theConnections.masterSimDrainNet;
			theDiodeConnections.maxDrainLeakVoltage = theConnections.maxDrainLeakVoltage;
			theDiodeConnections.maxDrainPower_p = theConnections.maxDrainPower_p;
			theDiodeConnections.maxDrainVoltage = theConnections.maxDrainVoltage;
			theDiodeConnections.minDrainLeakVoltage = theConnections.minDrainLeakVoltage;
			theDiodeConnections.minDrainPower_p = theConnections.minDrainPower_p;
			theDiodeConnections.minDrainVoltage = theConnections.minDrainVoltage;
			theDiodeConnections.originalDrainId = theConnections.originalDrainId;
			theDiodeConnections.simDrainPower_p = theConnections.simDrainPower_p;
			theDiodeConnections.simDrainVoltage = theConnections.simDrainVoltage;
			theDiodeConnections.validMaxDrain = theConnections.validMaxDrain;
			theDiodeConnections.validMaxDrainLeak = theConnections.validMaxDrainLeak;
			theDiodeConnections.validMinDrain = theConnections.validMinDrain;
			theDiodeConnections.validMinDrainLeak = theConnections.validMinDrainLeak;
			theDiodeConnections.validSimDrain = theConnections.validSimDrain;
			break;
		}
		case BULK: {
			theDiodeConnections.drainId = theConnections.bulkId;
			theDiodeConnections.masterMaxDrainNet = theConnections.masterMaxBulkNet;
			theDiodeConnections.masterMinDrainNet = theConnections.masterMinBulkNet;
			theDiodeConnections.masterSimDrainNet = theConnections.masterSimBulkNet;
			theDiodeConnections.maxDrainLeakVoltage = theConnections.maxBulkLeakVoltage;
			theDiodeConnections.maxDrainPower_p = theConnections.maxBulkPower_p;
			theDiodeConnections.maxDrainVoltage = theConnections.maxBulkVoltage;
			theDiodeConnections.minDrainLeakVoltage = theConnections.minBulkLeakVoltage;
			theDiodeConnections.minDrainPower_p = theConnections.minBulkPower_p;
			theDiodeConnections.minDrainVoltage = theConnections.minBulkVoltage;
			theDiodeConnections.originalDrainId = theConnections.originalBulkId;
			theDiodeConnections.simDrainPower_p = theConnections.simBulkPower_p;
			theDiodeConnections.simDrainVoltage = theConnections.simBulkVoltage;
			theDiodeConnections.validMaxDrain = theConnections.validMaxBulk;
			theDiodeConnections.validMaxDrainLeak = theConnections.validMaxBulkLeak;
			theDiodeConnections.validMinDrain = theConnections.validMinBulk;
			theDiodeConnections.validMinDrainLeak = theConnections.validMinBulkLeak;
			theDiodeConnections.validSimDrain = theConnections.validSimBulk;
			break;
		}
		default: {
			theConnections.device_p->model_p->Print();
			throw EInvalidTerminal(to_string<int>(myAnnode) + " is not a valid terminal");
		}
	}
}

int CCvcDb::CalculateMFactor(instanceId_t theInstanceId) {
	if (  instancePtr_v[theInstanceId]->IsParallelInstance() ) return 0;  // parallel/empty instances
	int myMFactor = 1;
	instanceId_t myInstanceId = theInstanceId;
	if ( instancePtr_v[myInstanceId]->parallelInstanceCount > 1 ) {
		myMFactor = instancePtr_v[myInstanceId]->parallelInstanceCount;
	}
	return myMFactor;
}


deviceId_t CCvcDb::GetAttachedDevice(netId_t theNetId, modelType_t theType, terminal_t theTerminal) {
	/// Return the first non-shorted device of type theType with terminal theTerminal connected to theNetId
	assert(GetEquivalentNet(theNetId) == theNetId);

	deviceId_t device_it;
	if ( theTerminal & GATE ) {
		device_it = firstGate_v[theNetId];
		while ( device_it != UNKNOWN_DEVICE ) {
			if ( sourceNet_v[device_it] != drainNet_v[device_it] ) {
				if ( theType == NMOS && IsNmos_(deviceType_v[device_it]) ) return device_it;

				if ( theType == PMOS && IsPmos_(deviceType_v[device_it]) ) return device_it;

				if ( theType == deviceType_v[device_it] ) return device_it;

			}
			device_it = nextGate_v[device_it];
		}
	}
	if ( theTerminal & SOURCE ) {
		device_it = firstSource_v[theNetId];
		while ( device_it != UNKNOWN_DEVICE ) {
			if ( sourceNet_v[device_it] != drainNet_v[device_it] ) {
				if ( theType == NMOS && IsNmos_(deviceType_v[device_it]) ) return device_it;

				if ( theType == PMOS && IsPmos_(deviceType_v[device_it]) ) return device_it;

				if ( theType == deviceType_v[device_it] ) return device_it;

			}
			device_it = nextSource_v[device_it];
		}
	}
	if ( theTerminal & DRAIN ) {
		device_it = firstDrain_v[theNetId];
		while ( device_it != UNKNOWN_DEVICE ) {
			if ( sourceNet_v[device_it] != drainNet_v[device_it] ) {
				if ( theType == NMOS && IsNmos_(deviceType_v[device_it]) ) return device_it;

				if ( theType == PMOS && IsPmos_(deviceType_v[device_it]) ) return device_it;

				if ( theType == deviceType_v[device_it] ) return device_it;

			}
			device_it = nextDrain_v[device_it];
		}
	}
	return (UNKNOWN_DEVICE);
}

deviceId_t CCvcDb::FindInverterDevice(netId_t theInputNet, netId_t theOutputNet, modelType_t theType) {
	/// Return the first device of theType with input theInputNet and output theOutputNet
	///
	/// Note: Will not find any device if inverter contains both nmos/pmos clamps
	unordered_set<deviceId_t> myDeviceList;
	deviceId_t device_it = firstSource_v[theOutputNet];
	while ( device_it != UNKNOWN_DEVICE ) {
		if ( sourceNet_v[device_it] != drainNet_v[device_it] ) {
			if ( theType == NMOS && IsNmos_(deviceType_v[device_it] ) ) {
				myDeviceList.insert(device_it);
			} if ( theType == PMOS && IsPmos_(deviceType_v[device_it] ) ) {
				myDeviceList.insert(device_it);
			}
		}
		device_it = nextSource_v[device_it];
	}
	device_it = firstDrain_v[theOutputNet];
	while ( device_it != UNKNOWN_DEVICE ) {
		if ( sourceNet_v[device_it] != drainNet_v[device_it] ) {
			if ( theType == NMOS && IsNmos_(deviceType_v[device_it] ) ) {
				myDeviceList.insert(device_it);
			} if ( theType == PMOS && IsPmos_(deviceType_v[device_it] ) ) {
				myDeviceList.insert(device_it);
			}
		}
		device_it = nextDrain_v[device_it];
	}
	device_it = firstGate_v[theInputNet];
	while ( device_it != UNKNOWN_DEVICE ) {
		if ( myDeviceList.count(device_it) > 0 ) return device_it;

		device_it = nextGate_v[device_it];
	}
	return(UNKNOWN_DEVICE);
}

returnCode_t CCvcDb::FindUniqueMosInputs(netId_t theOutputNet, netId_t theGroundNet, netId_t thePowerNet,
		CDeviceIdVector &theFirst_v, CDeviceIdVector &theNext_v, CNetIdVector &theSourceNet_v, CNetIdVector &theDrainNet_v,
		netId_t &theNmosInput, netId_t &thePmosInput) {
	/// Return the mos gates for devices with source connected to theOutputNet
	///
	/// Modifies theNmosInput, thePmosInput
	CPower * myGround_p = netVoltagePtr_v[theGroundNet].full;
	CPower * myPower_p = netVoltagePtr_v[thePowerNet].full;
	for ( deviceId_t device_it = theFirst_v[theOutputNet]; device_it != UNKNOWN_DEVICE; device_it = theNext_v[device_it] ) {
		if ( theSourceNet_v[device_it] == theDrainNet_v[device_it] ) continue;  // ignore inactive devices

		if ( IsNmos_(deviceType_v[device_it]) ) {
			if ( theDrainNet_v[device_it] == theGroundNet ) {  // expected ground
				if ( theNmosInput == UNKNOWN_NET ) {
					theNmosInput = gateNet_v[device_it];
				} else if ( theNmosInput != gateNet_v[device_it] ) return(FAIL);  // multiple NMOS inputs yield unknown result

			} else if ( IsOnGate(device_it, myGround_p) ) {  // series gate on
				deviceId_t myNextNmos = GetNextInSeries(device_it, theDrainNet_v[device_it]);
				if ( myNextNmos == UNKNOWN_DEVICE ) return(FAIL);  // not series

				if ( OppositeNet(myNextNmos, theDrainNet_v[device_it]) == theGroundNet ) {
					if ( theNmosInput == UNKNOWN_NET ) {
						theNmosInput = gateNet_v[myNextNmos];
					} else if ( theNmosInput != gateNet_v[myNextNmos] ) return(FAIL);  // multiple NMOS inputs yield unknown result

				} else return(FAIL);  // more than 2 in series

			} else return(FAIL);  // no connection to ground detected

		}
		if ( IsPmos_(deviceType_v[device_it]) ) {
			if ( theDrainNet_v[device_it] == thePowerNet ) {  // expected power
				if ( thePmosInput == UNKNOWN_NET ) {
					thePmosInput = gateNet_v[device_it];
				} else if ( thePmosInput != gateNet_v[device_it] ) return(FAIL);  // multiple PMOS inputs yield unknown result

			} else if ( IsOnGate(device_it, myPower_p) ) {  // series gate on
				deviceId_t myNextPmos = GetNextInSeries(device_it, theDrainNet_v[device_it]);
				if ( myNextPmos == UNKNOWN_DEVICE ) return(FAIL);  // not series

				if ( OppositeNet(myNextPmos, theDrainNet_v[device_it]) == thePowerNet ) {
					if ( thePmosInput == UNKNOWN_NET ) {
						thePmosInput = gateNet_v[myNextPmos];
					} else if ( thePmosInput != gateNet_v[myNextPmos] ) return(FAIL);  // multiple PMOS inputs yield unknown result

				} else return(FAIL);  // more than 2 in series

			} else return(FAIL);  // no connection to power detected

		}
	}
	return(OK);
}

netId_t CCvcDb::FindInverterInput(netId_t theOutputNet) {
	/// Find the input of inverter with output theOutputNet
	CVirtualNet myMinOutput;
	CVirtualNet myMaxOutput;
	myMinOutput(minNet_v, theOutputNet);
	myMaxOutput(maxNet_v, theOutputNet);
	netId_t myGroundNet = myMinOutput.finalNetId;
	netId_t myPowerNet = myMaxOutput.finalNetId;
	CPower * myGround_p = netVoltagePtr_v[myGroundNet].full;
	CPower * myPower_p = netVoltagePtr_v[myPowerNet].full;
	netId_t myNmosInput = UNKNOWN_NET;
	netId_t myPmosInput = UNKNOWN_NET;
	if ( ! IsPower_(myGround_p) || ! IsPower_(myPower_p) ) {
		logFile << "DEBUG: missing power for inverter at " << NetName(theOutputNet, true) << endl;
		return(UNKNOWN_NET);

	}
	returnCode_t mySourceCheck = FindUniqueMosInputs(theOutputNet, myGroundNet, myPowerNet, firstSource_v, nextSource_v, sourceNet_v, drainNet_v,
			myNmosInput, myPmosInput);
	returnCode_t myDrainCheck = FindUniqueMosInputs(theOutputNet, myGroundNet, myPowerNet, firstDrain_v, nextDrain_v, drainNet_v, sourceNet_v,
			myNmosInput, myPmosInput);
	return( ( mySourceCheck == OK && myDrainCheck == OK
			&& myNmosInput != UNKNOWN_NET && myNmosInput == myPmosInput ) ? myNmosInput : UNKNOWN_NET);
}

bool CCvcDb::IsOnGate(deviceId_t theDevice, CPower * thePower_p) {
	// Return true if gate is always on
	CPower * myGatePower_p = netVoltagePtr_v[gateNet_v[theDevice]].full;
	if ( ! IsPower_(myGatePower_p) ) return false;  // gate is not power

	if ( IsNmos_(deviceType_v[theDevice]) ) return( myGatePower_p->minVoltage > thePower_p->minVoltage );

	if ( IsPmos_(deviceType_v[theDevice]) ) return( myGatePower_p->maxVoltage < thePower_p->maxVoltage );

	assert(false);  // error if called with device that is not NMOS or PMOS
}

netId_t CCvcDb::OppositeNet(deviceId_t theDevice, netId_t theNet) {
	// return the opposite net of a device
	assert((sourceNet_v[theDevice] == theNet) || (drainNet_v[theDevice] == theNet));

	return((sourceNet_v[theDevice] == theNet) ? drainNet_v[theDevice] : sourceNet_v[theDevice]);
}

deviceId_t CCvcDb::GetNextInSeries(deviceId_t theDevice, netId_t theNet) {
	// Return the next device in series
	deviceId_t myNextDevice = UNKNOWN_DEVICE;
	for ( deviceId_t device_it = firstSource_v[theNet]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
		if ( sourceNet_v[device_it] != drainNet_v[device_it] ) continue;  // ignore inactive devices

		if ( device_it == theDevice ) continue;  // looking for device connected to this one

		if ( myNextDevice != UNKNOWN_DEVICE ) return(UNKNOWN_DEVICE);  // not in series, return error

		myNextDevice = device_it;
	}
	for ( deviceId_t device_it = firstDrain_v[theNet]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
		if ( sourceNet_v[device_it] != drainNet_v[device_it] ) continue;  // ignore inactive devices

		if ( device_it == theDevice ) continue;  // looking for device connected to this one

		if ( myNextDevice != UNKNOWN_DEVICE ) return(UNKNOWN_DEVICE);  // not in series, return error

		myNextDevice = device_it;
	}
	if ( myNextDevice != UNKNOWN_DEVICE ) {
		if ( deviceType_v[theDevice] != deviceType_v[myNextDevice] ) {  // should be the same type
			if ( IsNmos_(deviceType_v[theDevice]) && IsNmos_(deviceType_v[myNextDevice]) ) {  // type mismatch ok if both nmos
				;
			} else if ( IsPmos_(deviceType_v[theDevice]) && IsPmos_(deviceType_v[myNextDevice]) ) {  // type mismatch ok if both pmos
				;
			} else return(UNKNOWN_DEVICE);

		}
	}
	return(myNextDevice);
}

bool CCvcDb::IsInstanceNet(netId_t theNetId, instanceId_t theInstance) {
	// return true if theNet is used in theInstance
	if ( theInstance == UNKNOWN_INSTANCE || theNetId == UNKNOWN_NET ) return false;  // bad instance or net
	instanceId_t myParent = netParent_v[theNetId];
	if ( IsSubcircuitOf(myParent, theInstance) ) return true;
	CInstance * myInstance_p = instancePtr_v[theInstance];
	CCircuit * myCircuit_p = myInstance_p->master_p;
	for ( netId_t port_it = 0; port_it < myCircuit_p->portCount; port_it++ ) {
		if ( theNetId == GetEquivalentNet(myInstance_p->localToGlobalNetId_v[port_it]) ) return true;
	}
	return false;
}

instanceId_t CCvcDb::FindNetInstance(netId_t theNetId, instanceId_t theInstance) {
	// return highest instance using net in theInstance
	if ( theInstance == UNKNOWN_INSTANCE || theNetId == UNKNOWN_NET ) return UNKNOWN_INSTANCE;  // bad instance or net
	instanceId_t myParent = netParent_v[theNetId];
	if ( IsSubcircuitOf(myParent, theInstance) ) return myParent;
	CInstance * myInstance_p = instancePtr_v[theInstance];
	CCircuit * myCircuit_p = myInstance_p->master_p;
	for ( netId_t port_it = 0; port_it < myCircuit_p->portCount; port_it++ ) {
		if ( theNetId == GetEquivalentNet(myInstance_p->localToGlobalNetId_v[port_it]) ) return theInstance;
	}
	return UNKNOWN_INSTANCE;
}

bool CCvcDb::IsInternalNet(netId_t theNetId, instanceId_t theInstance) {
	// true if net is internal to instance
	if ( theInstance == UNKNOWN_INSTANCE || theNetId == UNKNOWN_NET ) return false;  // bad instance or net
	instanceId_t myParent = netParent_v[theNetId];
	if ( myParent == 0 && theInstance == 0 ) return false;  // top level nets are not internal
	return( IsSubcircuitOf(myParent, theInstance) );
}

text_t CCvcDb::GetLocalNetName(instanceId_t theInstance, netId_t theNet) {
	// requires 2 reverse searches
	CInstance * myInstance_p = instancePtr_v[theInstance];
	CCircuit * myMaster_p = myInstance_p->master_p;
	netId_t net_it = 0;
	while ( net_it < myInstance_p->localToGlobalNetId_v.size() && myInstance_p->localToGlobalNetId_v[net_it] != theNet ) {
		net_it++;
	}
	if ( net_it >= myInstance_p->localToGlobalNetId_v.size() ) return(NULL);
	for( auto signalMap_pit = myMaster_p->localSignalIdMap.begin(); signalMap_pit != myMaster_p->localSignalIdMap.end(); signalMap_pit++ ) {
		if ( signalMap_pit->second == net_it ) return signalMap_pit->first;
	}
	return(NULL);
}
