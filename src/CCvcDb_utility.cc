/*
 * CCvcDb-utility.cc
 *
 * Copyright 2014-2106 D. Mitch Bailey  cvc at shuharisystem dot com
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
//		CheckResistorOverflow_(minNet_v[theNetId].finalResistance, theNetId, logFile);
/*
		if ( isFixedMinNet ) {
			myVirtualNet = fixedMinNet_v[theNetId];
		} else {
			myVirtualNet(minNet_v, theNetId);
//			myVirtualNet.GetMasterNet(minNet_v, theNetId);
		}
*/
//		if ( isFixedMinNet && ! ( myVirtualNet == fixedMinNet_v[theNetId]) ) throw EDatabaseError("Fixed min net mismatch");
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			if ( netVoltagePtr_v[myVirtualNet.finalNetId] ) {
				if ( theSkipHiZFlag && netVoltagePtr_v[myVirtualNet.finalNetId]->type[HIZ_BIT] ) return UNKNOWN_VOLTAGE;
				return netVoltagePtr_v[myVirtualNet.finalNetId]->minVoltage;
			}
		}
	}
	return UNKNOWN_VOLTAGE;
}

voltage_t CCvcDb::MinSimVoltage(netId_t theNetId) {
	// limit min value to sim value
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(minNet_v, theNetId);
//		CheckResistorOverflow_(minNet_v[theNetId].finalResistance, theNetId, logFile);
/*
		if ( isFixedMinNet ) {
			myVirtualNet = fixedMinNet_v[theNetId];
		} else {
			myVirtualNet(minNet_v, theNetId);
//			myVirtualNet.GetMasterNet(minNet_v, theNetId);
		}
*/
//		if ( isFixedMinNet && ! ( myVirtualNet == fixedMinNet_v[theNetId]) ) throw EDatabaseError("Fixed min net mismatch");
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			voltage_t myMinVoltage = ( netVoltagePtr_v[myVirtualNet.finalNetId] ) ? netVoltagePtr_v[myVirtualNet.finalNetId]->minVoltage : UNKNOWN_VOLTAGE;
			netId_t myNetId = theNetId;
			while ( myNetId != minNet_v[myNetId].nextNetId ) {
				myNetId = minNet_v[myNetId].nextNetId;
				if ( netVoltagePtr_v[myNetId] && netVoltagePtr_v[myNetId]->simVoltage != UNKNOWN_VOLTAGE ) {
					return ((myMinVoltage == UNKNOWN_VOLTAGE) ? netVoltagePtr_v[myNetId]->simVoltage : max(netVoltagePtr_v[myNetId]->simVoltage, myMinVoltage));
				}
			}
			return myMinVoltage;
		}
	}
	return UNKNOWN_VOLTAGE;
}

voltage_t CCvcDb::MinLeakVoltage(netId_t theNetId) {
	if ( theNetId != UNKNOWN_NET && leakVoltageSet ) {
		netId_t myNetId = minLeakNet_v[theNetId].finalNetId;
//		CVirtualNet myVirtualNet(minLeakNet_v, theNetId);
		assert(theNetId == GetEquivalentNet(theNetId));
		if ( myNetId != UNKNOWN_NET ) {
			if ( leakVoltagePtr_v[myNetId] ) {
				return leakVoltagePtr_v[myNetId]->minVoltage;
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
//		CheckResistorOverflow_(simNet_v[theNetId].finalResistance, theNetId, logFile);
/*
		if ( isFixedSimNet ) {
			myVirtualNet = fixedSimNet_v[theNetId];
		} else {
			myVirtualNet(simNet_v, theNetId);
//			myVirtualNet.GetMasterNet(simNet_v, theNetId);
		}
*/
//		if ( isFixedSimNet && ! ( myVirtualNet == fixedSimNet_v[theNetId]) ) throw EDatabaseError("Fixed sim net mismatch");
//		CVirtualNet myVirtualNet(simNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			if ( netVoltagePtr_v[myVirtualNet.finalNetId] ) {
				return netVoltagePtr_v[myVirtualNet.finalNetId]->simVoltage;
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
	if ( myMinSourceNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMinSourceNetId] ) return false;
	if ( myMinDrainNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMinDrainNetId] ) return false;
	if ( myMaxSourceNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMaxSourceNetId] ) return false;
	if ( myMaxDrainNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMaxDrainNetId] ) return false;
	voltage_t myMinSourceVoltage = netVoltagePtr_v[myMinSourceNetId]->minVoltage;
	voltage_t myMinDrainVoltage = netVoltagePtr_v[myMinDrainNetId]->minVoltage;
	voltage_t myMaxSourceVoltage = netVoltagePtr_v[myMaxSourceNetId]->maxVoltage;
	voltage_t myMaxDrainVoltage = netVoltagePtr_v[myMaxDrainNetId]->maxVoltage;
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
//		CheckResistorOverflow_(simNet_v[theNetId].finalResistance, theNetId, logFile);
/*
		if ( isFixedSimNet ) {
			myVirtualNet = fixedSimNet_v[theNetId];
		} else {
			myVirtualNet(simNet_v, theNetId);
//			myVirtualNet.GetMasterNet(simNet_v, theNetId);
		}
*/
//		if ( isFixedSimNet && ! ( myVirtualNet == fixedSimNet_v[theNetId]) ) throw EDatabaseError("Fixed sim net mismatch");
//		CVirtualNet myVirtualNet(simNet_v, theNetId);
		return myVirtualNet.finalResistance;
	}
	return MAX_RESISTANCE;
}

voltage_t CCvcDb::MaxVoltage(netId_t theNetId, bool theSkipHiZFlag) {
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(maxNet_v, theNetId);
//		CheckResistorOverflow_(maxNet_v[theNetId].finalResistance, theNetId, logFile);

/*
		if ( isFixedMaxNet ) {
			myVirtualNet = fixedMaxNet_v[theNetId];
		} else {
			myVirtualNet(maxNet_v, theNetId);
//			myVirtualNet.GetMasterNet(maxNet_v, theNetId);
		}
*/
//		if ( isFixedMaxNet && ! ( myVirtualNet == fixedMaxNet_v[theNetId]) ) throw EDatabaseError("Fixed max net mismatch");
//		CVirtualNet myVirtualNet(maxNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			if ( netVoltagePtr_v[myVirtualNet.finalNetId] ) {
				if ( theSkipHiZFlag && netVoltagePtr_v[myVirtualNet.finalNetId]->type[HIZ_BIT] ) return UNKNOWN_VOLTAGE;
				return netVoltagePtr_v[myVirtualNet.finalNetId]->maxVoltage;
			}
		}
	}
	return UNKNOWN_VOLTAGE;
}

voltage_t CCvcDb::MaxSimVoltage(netId_t theNetId) {
	// limit max value to sim value
	if ( theNetId != UNKNOWN_NET ) {
		static CVirtualNet myVirtualNet;
		assert(theNetId == GetEquivalentNet(theNetId));
		myVirtualNet(maxNet_v, theNetId);
//		CheckResistorOverflow_(minNet_v[theNetId].finalResistance, theNetId, logFile);
/*
		if ( isFixedMinNet ) {
			myVirtualNet = fixedMinNet_v[theNetId];
		} else {
			myVirtualNet(minNet_v, theNetId);
//			myVirtualNet.GetMasterNet(minNet_v, theNetId);
		}
*/
//		if ( isFixedMinNet && ! ( myVirtualNet == fixedMinNet_v[theNetId]) ) throw EDatabaseError("Fixed min net mismatch");
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			voltage_t myMaxVoltage = ( netVoltagePtr_v[myVirtualNet.finalNetId] ) ? netVoltagePtr_v[myVirtualNet.finalNetId]->maxVoltage : UNKNOWN_VOLTAGE;
			netId_t myNetId = theNetId;
			while ( myNetId != maxNet_v[myNetId].nextNetId ) {
				myNetId = maxNet_v[myNetId].nextNetId;
				if ( netVoltagePtr_v[myNetId] && netVoltagePtr_v[myNetId]->simVoltage != UNKNOWN_VOLTAGE ) {
					return ((myMaxVoltage == UNKNOWN_VOLTAGE) ? netVoltagePtr_v[myNetId]->simVoltage : min(netVoltagePtr_v[myNetId]->simVoltage, myMaxVoltage));
				}
			}
			return myMaxVoltage;
		}
	}
	return UNKNOWN_VOLTAGE;
}

voltage_t CCvcDb::MaxLeakVoltage(netId_t theNetId) {
	if ( theNetId != UNKNOWN_NET && leakVoltageSet ) {
		assert(theNetId == GetEquivalentNet(theNetId));
		netId_t myNetId = maxLeakNet_v[theNetId].finalNetId;
//		CVirtualNet myVirtualNet(maxLeakNet_v, theNetId);
		if ( myNetId != UNKNOWN_NET ) {
			if ( leakVoltagePtr_v[myNetId] ) {
				return leakVoltagePtr_v[myNetId]->maxVoltage;
			}
		}
/*
		CVirtualNet myVirtualNet(maxLeakNet_v, theNetId);
		if ( myVirtualNet.finalNetId != UNKNOWN_NET ) {
			if ( leakVoltagePtr_v[myVirtualNet.finalNetId] ) {
				return leakVoltagePtr_v[myVirtualNet.finalNetId]->maxVoltage;
			}
		}
*/
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
	if ( netVoltagePtr_v[theNetId] && netVoltagePtr_v[theNetId]->netId != UNKNOWN_NET ) {
		return(netVoltagePtr_v[theNetId]->netId);
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
	CDevice * myDevice_p;
	CInstance * myInstance_p = instancePtr_v[deviceParent_v[theDeviceId]];
	myDevice_p = myInstance_p->master_p->devicePtr_v[theDeviceId - myInstance_p->firstDeviceId];
//	SetDeviceNets(myInstance_p, myDevice_p, theConnections.sourceId, theConnections.gateId, theConnections.drainId, theConnections.bulkId);
	SetConnections_(theConnections, theDeviceId);
	theConnections.masterSourceNet(theEventQueue.virtualNet_v, theConnections.sourceId);
	theConnections.masterGateNet(theEventQueue.virtualNet_v, theConnections.gateId);
	theConnections.masterDrainNet(theEventQueue.virtualNet_v, theConnections.drainId);
	theConnections.masterBulkNet(theEventQueue.virtualNet_v, theConnections.bulkId);
//	theConnections.masterSourceNet = CVirtualNet(theEventQueue.virtualNet_v, theConnections.sourceId);
//	theConnections.masterGateNet = CVirtualNet(theEventQueue.virtualNet_v, theConnections.gateId);
//	theConnections.masterDrainNet = CVirtualNet(theEventQueue.virtualNet_v, theConnections.drainId);
//	theConnections.masterBulkNet = CVirtualNet(theEventQueue.virtualNet_v, theConnections.bulkId);
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
		theConnections.gatePower_p = netVoltagePtr_v[theConnections.masterGateNet.finalNetId];
	}
	theConnections.sourcePower_p = netVoltagePtr_v[theConnections.masterSourceNet.finalNetId];
	if ( IsSCRCPower(theConnections.sourcePower_p) ) {
		theConnections.sourceVoltage = theConnections.sourcePower_p->minVoltage;  // min = max
	}
	theConnections.drainPower_p = netVoltagePtr_v[theConnections.masterDrainNet.finalNetId];
	if ( IsSCRCPower(theConnections.drainPower_p) ) {
		theConnections.drainVoltage = theConnections.drainPower_p->minVoltage;  // min = max
	}
	theConnections.device_p = myDevice_p;
	theConnections.deviceId = theDeviceId;
	theConnections.resistance = parameterResistanceMap[theConnections.device_p->parameters];
}

void CCvcDb::MapDeviceNets(deviceId_t theDeviceId, CFullConnection& theConnections) {
	CDevice * myDevice_p;
	CInstance * myInstance_p = instancePtr_v[deviceParent_v[theDeviceId]];
	myDevice_p = myInstance_p->master_p->devicePtr_v[theDeviceId - myInstance_p->firstDeviceId];
	SetConnections_(theConnections, theDeviceId);
	switch (myDevice_p->model_p->type) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			theConnections.originalDrainId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[0]];
			theConnections.originalGateId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[1]];
			theConnections.originalSourceId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[2]];
//			theConnections.sourceId = GetEquivalentNet(theConnections.originalSourceId);
//			theConnections.gateId = GetEquivalentNet(theConnections.originalGateId);
//			theConnections.drainId = GetEquivalentNet(theConnections.originalDrainId);
			if ( cvcParameters.cvcSOI ) {
				theConnections.originalBulkId = UNKNOWN_NET;
//				theConnections.bulkId = UNKNOWN_NET;
			} else {
				theConnections.originalBulkId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[3]];
//				theConnections.bulkId = GetEquivalentNet(theConnections.originalBulkId);
			}
			break; }
		case DIODE: case SWITCH_OFF: case FUSE_OFF: case CAPACITOR:	case SWITCH_ON:	case FUSE_ON: case RESISTOR: {
			theConnections.originalSourceId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[0]];
			theConnections.originalGateId = UNKNOWN_NET;
			theConnections.originalDrainId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[1]];
//			theConnections.sourceId = GetEquivalentNet(theConnections.originalSourceId);
//			theConnections.gateId = UNKNOWN_NET;
//			theConnections.drainId = GetEquivalentNet(theConnections.originalDrainId);
			if ( myDevice_p->signalId_v.size() == 3 ) {
				theConnections.originalBulkId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[2]];
//				theConnections.bulkId = GetEquivalentNet(theConnections.originalBulkId);
			} else {
				theConnections.originalBulkId = UNKNOWN_NET;
//				theConnections.bulkId = UNKNOWN_NET;
			}
			break; }
		case BIPOLAR: {
			theConnections.originalSourceId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[0]];
			theConnections.originalGateId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[1]];
			theConnections.originalDrainId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[2]];
			theConnections.originalBulkId = UNKNOWN_NET;
//			theConnections.sourceId = GetEquivalentNet(theConnections.originalSourceId);
//			theConnections.gateId = GetEquivalentNet(theConnections.originalGateId);
//			theConnections.drainId = GetEquivalentNet(theConnections.originalDrainId);
//			theConnections.bulkId = UNKNOWN_NET;
		break; }
		default: {
//			cout << "ERROR: Unknown model type in LinkDevices" << endl;
			myDevice_p->model_p->Print();
			throw EUnknownModel();
		}
	}
	theConnections.masterMaxSourceNet(maxNet_v, theConnections.sourceId);
	theConnections.masterMaxGateNet(maxNet_v, theConnections.gateId);
	theConnections.masterMaxDrainNet(maxNet_v, theConnections.drainId);
	theConnections.masterMaxBulkNet(maxNet_v, theConnections.bulkId);
//	theConnections.masterMaxSourceNet = CVirtualNet(maxNet_v, theConnections.sourceId);
//	theConnections.masterMaxGateNet = CVirtualNet(maxNet_v, theConnections.gateId);
//	theConnections.masterMaxDrainNet = CVirtualNet(maxNet_v, theConnections.drainId);
//	theConnections.masterMaxBulkNet = CVirtualNet(maxNet_v, theConnections.bulkId);
	theConnections.maxSourceVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxSourceNet.finalNetId);
	theConnections.maxGateVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxGateNet.finalNetId);
	theConnections.maxDrainVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxDrainNet.finalNetId);
	theConnections.maxBulkVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxBulkNet.finalNetId);
	theConnections.maxSourcePower_p = netVoltagePtr_v[theConnections.masterMaxSourceNet.finalNetId];
	theConnections.maxDrainPower_p = netVoltagePtr_v[theConnections.masterMaxDrainNet.finalNetId];
	if ( theConnections.masterMaxGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.maxGatePower_p = NULL;
	} else {
		theConnections.maxGatePower_p = netVoltagePtr_v[theConnections.masterMaxGateNet.finalNetId];
	}
	if ( theConnections.masterMaxBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.maxBulkPower_p = NULL;
	} else {
		theConnections.maxBulkPower_p = netVoltagePtr_v[theConnections.masterMaxBulkNet.finalNetId];
	}

	theConnections.masterMinSourceNet(minNet_v, theConnections.sourceId);
	theConnections.masterMinGateNet(minNet_v, theConnections.gateId);
	theConnections.masterMinDrainNet(minNet_v, theConnections.drainId);
	theConnections.masterMinBulkNet(minNet_v, theConnections.bulkId);
	theConnections.minSourceVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinSourceNet.finalNetId);
	theConnections.minGateVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinGateNet.finalNetId);
	theConnections.minDrainVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinDrainNet.finalNetId);
	theConnections.minBulkVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinBulkNet.finalNetId);
	theConnections.minSourcePower_p = netVoltagePtr_v[theConnections.masterMinSourceNet.finalNetId];
	theConnections.minDrainPower_p = netVoltagePtr_v[theConnections.masterMinDrainNet.finalNetId];
	if ( theConnections.masterMinGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.minGatePower_p = NULL;
	} else {
		theConnections.minGatePower_p = netVoltagePtr_v[theConnections.masterMinGateNet.finalNetId];
	}
	if ( theConnections.masterMinBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.minBulkPower_p = NULL;
	} else {
		theConnections.minBulkPower_p = netVoltagePtr_v[theConnections.masterMinBulkNet.finalNetId];
	}

	theConnections.masterSimSourceNet(simNet_v, theConnections.sourceId);
	theConnections.masterSimGateNet(simNet_v, theConnections.gateId);
	theConnections.masterSimDrainNet(simNet_v, theConnections.drainId);
	theConnections.masterSimBulkNet(simNet_v, theConnections.bulkId);
	theConnections.simSourceVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimSourceNet.finalNetId);
	theConnections.simGateVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimGateNet.finalNetId);
	theConnections.simDrainVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimDrainNet.finalNetId);
	theConnections.simBulkVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimBulkNet.finalNetId);
	theConnections.simSourcePower_p = netVoltagePtr_v[theConnections.masterSimSourceNet.finalNetId];
	theConnections.simDrainPower_p = netVoltagePtr_v[theConnections.masterSimDrainNet.finalNetId];
	if ( theConnections.masterSimGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.simGatePower_p = NULL;
	} else {
		theConnections.simGatePower_p = netVoltagePtr_v[theConnections.masterSimGateNet.finalNetId];
	}
	if ( theConnections.masterSimBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.simBulkPower_p = NULL;
	} else {
		theConnections.simBulkPower_p = netVoltagePtr_v[theConnections.masterSimBulkNet.finalNetId];
	}
	theConnections.device_p = myDevice_p;
	theConnections.deviceId = theDeviceId;
	theConnections.resistance = parameterResistanceMap[theConnections.device_p->parameters];
}

void CCvcDb::MapDeviceSourceDrainNets(deviceId_t theDeviceId, CFullConnection& theConnections) {
	CDevice * myDevice_p;
	CInstance * myInstance_p = instancePtr_v[deviceParent_v[theDeviceId]];
	myDevice_p = myInstance_p->master_p->devicePtr_v[theDeviceId - myInstance_p->firstDeviceId];
	SetConnections_(theConnections, theDeviceId);
	switch (myDevice_p->model_p->type) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			theConnections.originalDrainId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[0]];
//			theConnections.originalGateId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[1]];
			theConnections.originalSourceId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[2]];
//			theConnections.originalBulkId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[3]];
//			theConnections.sourceId = GetEquivalentNet(theConnections.originalSourceId);
//			theConnections.gateId = GetEquivalentNet(theConnections.originalGateId);
//			theConnections.drainId = GetEquivalentNet(theConnections.originalDrainId);
//			theConnections.bulkId = GetEquivalentNet(theConnections.originalBulkId);
			break; }
		case BIPOLAR: case DIODE: case SWITCH_OFF: case FUSE_OFF: case CAPACITOR:	case SWITCH_ON:	case FUSE_ON: case RESISTOR: {
			theConnections.originalSourceId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[0]];
//			theConnections.originalGateId = UNKNOWN_NET;
			theConnections.originalDrainId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[1]];
//			theConnections.originalBulkId = UNKNOWN_NET;
//			theConnections.sourceId = GetEquivalentNet(theConnections.originalSourceId);
//			theConnections.gateId = UNKNOWN_NET;
//			theConnections.drainId = GetEquivalentNet(theConnections.originalDrainId);
//			theConnections.bulkId = UNKNOWN_NET;
			break; }
		default: {
//			cout << "ERROR: Unknown model type in LinkDevices" << endl;
			myDevice_p->model_p->Print();
			throw EUnknownModel();
		}
	}
	theConnections.masterMaxSourceNet(maxNet_v, theConnections.sourceId);
//	theConnections.masterMaxGateNet = CVirtualNet(maxNet_v, theConnections.gateId);
	theConnections.masterMaxDrainNet(maxNet_v, theConnections.drainId);
//	theConnections.masterMaxBulkNet = CVirtualNet(maxNet_v, theConnections.bulkId);
	theConnections.maxSourceVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxSourceNet.finalNetId);
//	theConnections.maxGateVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxGateNet.finalNetId);
	theConnections.maxDrainVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxDrainNet.finalNetId);
//	theConnections.maxBulkVoltage = netVoltagePtr_v.MaxVoltage(theConnections.masterMaxBulkNet.finalNetId);
	theConnections.maxSourcePower_p = netVoltagePtr_v[theConnections.masterMaxSourceNet.finalNetId];
	theConnections.maxDrainPower_p = netVoltagePtr_v[theConnections.masterMaxDrainNet.finalNetId];
/*
	if ( theConnections.masterMaxGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.maxGatePower_p = NULL;
	} else {
		theConnections.maxGatePower_p = netVoltagePtr_v[theConnections.masterMaxGateNet.finalNetId];
	}
	if ( theConnections.masterMaxBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.maxBulkPower_p = NULL;
	} else {
		theConnections.maxBulkPower_p = netVoltagePtr_v[theConnections.masterMaxBulkNet.finalNetId];
	}
*/

	theConnections.masterMinSourceNet(minNet_v, theConnections.sourceId);
//	theConnections.masterMinGateNet = CVirtualNet(minNet_v, theConnections.gateId);
	theConnections.masterMinDrainNet(minNet_v, theConnections.drainId);
//	theConnections.masterMinBulkNet = CVirtualNet(minNet_v, theConnections.bulkId);
	theConnections.minSourceVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinSourceNet.finalNetId);
//	theConnections.minGateVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinGateNet.finalNetId);
	theConnections.minDrainVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinDrainNet.finalNetId);
//	theConnections.minBulkVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinBulkNet.finalNetId);
	theConnections.minSourcePower_p = netVoltagePtr_v[theConnections.masterMinSourceNet.finalNetId];
	theConnections.minDrainPower_p = netVoltagePtr_v[theConnections.masterMinDrainNet.finalNetId];
/*
	if ( theConnections.masterMinGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.minGatePower_p = NULL;
	} else {
		theConnections.minGatePower_p = netVoltagePtr_v[theConnections.masterMinGateNet.finalNetId];
	}
	if ( theConnections.masterMinBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.minBulkPower_p = NULL;
	} else {
		theConnections.minBulkPower_p = netVoltagePtr_v[theConnections.masterMinBulkNet.finalNetId];
	}
*/

	theConnections.masterSimSourceNet(simNet_v, theConnections.sourceId);
//	theConnections.masterSimGateNet = CVirtualNet(simNet_v, theConnections.gateId);
	theConnections.masterSimDrainNet(simNet_v, theConnections.drainId);
//	theConnections.masterSimBulkNet = CVirtualNet(simNet_v, theConnections.bulkId);
	theConnections.simSourceVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimSourceNet.finalNetId);
//	theConnections.simGateVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimGateNet.finalNetId);
	theConnections.simDrainVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimDrainNet.finalNetId);
//	theConnections.simBulkVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimBulkNet.finalNetId);
	theConnections.simSourcePower_p = netVoltagePtr_v[theConnections.masterSimSourceNet.finalNetId];
	theConnections.simDrainPower_p = netVoltagePtr_v[theConnections.masterSimDrainNet.finalNetId];
/*
	if ( theConnections.masterSimGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.simGatePower_p = NULL;
	} else {
		theConnections.simGatePower_p = netVoltagePtr_v[theConnections.masterSimGateNet.finalNetId];
	}
	if ( theConnections.masterSimBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.simBulkPower_p = NULL;
	} else {
		theConnections.simBulkPower_p = netVoltagePtr_v[theConnections.masterSimBulkNet.finalNetId];
	}
*/
	theConnections.device_p = myDevice_p;
	theConnections.deviceId = theDeviceId;
	theConnections.resistance = parameterResistanceMap[theConnections.device_p->parameters];
}

void CCvcDb::MapDeviceNets(CInstance * theInstance_p, CDevice * theDevice_p, CFullConnection& theConnections) {
	theConnections.device_p = theDevice_p;
	theConnections.deviceId = theInstance_p->firstDeviceId + theDevice_p->parent_p->localDeviceIdMap[theDevice_p->name];
	SetConnections_(theConnections, theConnections.deviceId);
	switch (theDevice_p->model_p->type) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			theConnections.originalDrainId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]];
			theConnections.originalGateId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]];
			theConnections.originalSourceId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]];
//			theConnections.sourceId = GetEquivalentNet(theConnections.originalSourceId);
//			theConnections.gateId = GetEquivalentNet(theConnections.originalGateId);
//			theConnections.drainId = GetEquivalentNet(theConnections.originalDrainId);
			if ( cvcParameters.cvcSOI ) {
				theConnections.originalBulkId = UNKNOWN_NET;
//				theConnections.bulkId = UNKNOWN_NET;
			} else {
				theConnections.originalBulkId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[3]];
//				theConnections.bulkId = GetEquivalentNet(theConnections.originalBulkId);
			}
			break; }
		case DIODE: case SWITCH_OFF: case FUSE_OFF: case CAPACITOR:	case SWITCH_ON:	case FUSE_ON: case RESISTOR: {
			theConnections.originalSourceId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]];
			theConnections.originalGateId = UNKNOWN_NET;
			theConnections.originalDrainId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]];
//			theConnections.sourceId = GetEquivalentNet(theConnections.originalSourceId);
//			theConnections.gateId = UNKNOWN_NET;
//			theConnections.drainId = GetEquivalentNet(theConnections.originalDrainId);
			if ( theDevice_p->signalId_v.size() == 3 ) {
				theConnections.originalBulkId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]];;
//				theConnections.bulkId = GetEquivalentNet(theConnections.originalBulkId);
			} else {
				theConnections.originalBulkId = UNKNOWN_NET;
//				theConnections.bulkId = UNKNOWN_NET;
			}
			break; }
		case BIPOLAR: {
			theConnections.originalSourceId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[0]];
			theConnections.originalGateId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[1]];
			theConnections.originalDrainId = theInstance_p->localToGlobalNetId_v[theDevice_p->signalId_v[2]];
			theConnections.originalBulkId = UNKNOWN_NET;
//			theConnections.sourceId = GetEquivalentNet(theConnections.originalSourceId);
//			theConnections.gateId = GetEquivalentNet(theConnections.originalGateId);;
//			theConnections.drainId = GetEquivalentNet(theConnections.originalDrainId);
//			theConnections.bulkId = UNKNOWN_NET;
			break; }
		default: {
//			cout << "ERROR: Unknown model type in LinkDevices" << endl;
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
	theConnections.maxSourcePower_p = netVoltagePtr_v[theConnections.masterMaxSourceNet.finalNetId];
	theConnections.maxDrainPower_p = netVoltagePtr_v[theConnections.masterMaxDrainNet.finalNetId];
	if ( theConnections.masterMaxGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.maxGatePower_p = NULL;
	} else {
		theConnections.maxGatePower_p = netVoltagePtr_v[theConnections.masterMaxGateNet.finalNetId];
	}
	if ( theConnections.masterMaxBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.maxBulkPower_p = NULL;
	} else {
		theConnections.maxBulkPower_p = netVoltagePtr_v[theConnections.masterMaxBulkNet.finalNetId];
	}

	theConnections.masterMinSourceNet(minNet_v, theConnections.sourceId);
	theConnections.masterMinGateNet(minNet_v, theConnections.gateId);
	theConnections.masterMinDrainNet(minNet_v, theConnections.drainId);
	theConnections.masterMinBulkNet(minNet_v, theConnections.bulkId);
	theConnections.minSourceVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinSourceNet.finalNetId);
	theConnections.minGateVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinGateNet.finalNetId);
	theConnections.minDrainVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinDrainNet.finalNetId);
	theConnections.minBulkVoltage = netVoltagePtr_v.MinVoltage(theConnections.masterMinBulkNet.finalNetId);
	theConnections.minSourcePower_p = netVoltagePtr_v[theConnections.masterMinSourceNet.finalNetId];
	theConnections.minDrainPower_p = netVoltagePtr_v[theConnections.masterMinDrainNet.finalNetId];
	if ( theConnections.masterMinGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.minGatePower_p = NULL;
	} else {
		theConnections.minGatePower_p = netVoltagePtr_v[theConnections.masterMinGateNet.finalNetId];
	}
	if ( theConnections.masterMinBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.minBulkPower_p = NULL;
	} else {
		theConnections.minBulkPower_p = netVoltagePtr_v[theConnections.masterMinBulkNet.finalNetId];
	}

	theConnections.masterSimSourceNet(simNet_v, theConnections.sourceId);
	theConnections.masterSimGateNet(simNet_v, theConnections.gateId);
	theConnections.masterSimDrainNet(simNet_v, theConnections.drainId);
	theConnections.masterSimBulkNet(simNet_v, theConnections.bulkId);
	theConnections.simSourceVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimSourceNet.finalNetId);
	theConnections.simGateVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimGateNet.finalNetId);
	theConnections.simDrainVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimDrainNet.finalNetId);
	theConnections.simBulkVoltage = netVoltagePtr_v.SimVoltage(theConnections.masterSimBulkNet.finalNetId);
	theConnections.simSourcePower_p = netVoltagePtr_v[theConnections.masterSimSourceNet.finalNetId];
	theConnections.simDrainPower_p = netVoltagePtr_v[theConnections.masterSimDrainNet.finalNetId];
	if ( theConnections.masterSimGateNet.finalNetId == UNKNOWN_NET ) {
		theConnections.simGatePower_p = NULL;
	} else {
		theConnections.simGatePower_p = netVoltagePtr_v[theConnections.masterSimGateNet.finalNetId];
	}
	if ( theConnections.masterSimBulkNet.finalNetId == UNKNOWN_NET ) {
		theConnections.simBulkPower_p = NULL;
	} else {
		theConnections.simBulkPower_p = netVoltagePtr_v[theConnections.masterSimBulkNet.finalNetId];
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
	if ( netVoltagePtr_v[theConnections.masterDrainNet.finalNetId] == NULL ) return theConnections.gateId == theConnections.drainId;
	if ( netVoltagePtr_v[theConnections.masterSourceNet.nextNetId] == NULL ) return theConnections.gateId == theConnections.sourceId;
	throw EDatabaseError("GateEqualsDrain: " + NetName(theConnections.gateId));
}

/*
void CCvcDb::RestoreQueue(CEventQueue& theBaseEventQueue, CEventQueue& theSavedEventQueue, deviceStatus_t theStatusBit) {
	assert(theBaseEventQueue.QueueSize() == 0);

	for (CEventSubQueue::iterator eventPair_pit = theSavedEventQueue.savedMainQueue.begin(); eventPair_pit != theSavedEventQueue.savedMainQueue.end(); eventPair_pit++ ) {
		eventKey_t myEventKey = eventPair_pit->first;
		for (CEventList::iterator device_pit = eventPair_pit->second.begin(); device_pit != eventPair_pit->second.end(); device_pit++ ) {
			if ( deviceStatus_v[*device_pit][SIM_INACTIVE] ) continue;
			theBaseEventQueue.mainQueue[myEventKey].push_back(*device_pit);
			theBaseEventQueue.enqueueCount++;
			deviceStatus_v[*device_pit][theStatusBit] = true;
		}
	}
	for (CEventSubQueue::iterator eventPair_pit = theSavedEventQueue.savedDelayQueue.begin(); eventPair_pit != theSavedEventQueue.savedDelayQueue.end(); eventPair_pit++ ) {
		eventKey_t myEventKey = eventPair_pit->first;
		for (CEventList::iterator device_pit = eventPair_pit->second.begin(); device_pit != eventPair_pit->second.end(); device_pit++ ) {
			if ( deviceStatus_v[*device_pit][SIM_INACTIVE] ) continue;
			theBaseEventQueue.delayQueue[myEventKey].push_back(*device_pit);
			theBaseEventQueue.enqueueCount++;
			deviceStatus_v[*device_pit][theStatusBit] = true;
		}
	}
}
*/

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
		if ( netVoltagePtr_v[myMinNet] && netVoltagePtr_v[myMinNet]->minVoltage != UNKNOWN_VOLTAGE ) {
			return(netVoltagePtr_v[myMinNet]->minVoltage);
		} else {
			reportFile << "DEBUG: Unexpected min voltage at net " << NetName(myMinNet) << endl;
			return(UNKNOWN_VOLTAGE);
		}
		//assert(netVoltagePtr_v[myMinNet]);
		//assert(netVoltagePtr_v[myMinNet]->minVoltage != UNKNOWN_VOLTAGE);
		//return(netVoltagePtr_v[myMinNet]->minVoltage);
    }
}

voltage_t CCvcDb::DefaultMaxVoltage(CPower * thePower_p) {
    if ( thePower_p->defaultMaxNet == UNKNOWN_NET ) {
		return(UNKNOWN_VOLTAGE);
    } else {
		netId_t myMaxNet = maxNet_v[thePower_p->defaultMaxNet].finalNetId;
		if ( netVoltagePtr_v[myMaxNet] && netVoltagePtr_v[myMaxNet]->maxVoltage != UNKNOWN_VOLTAGE ) {
			return(netVoltagePtr_v[myMaxNet]->maxVoltage);
		} else {
			reportFile << "DEBUG: Unexpected max voltage at net " << NetName(myMaxNet) << endl;
			return(UNKNOWN_VOLTAGE);
		}
		//assert(netVoltagePtr_v[myMaxNet]);
		//assert(netVoltagePtr_v[myMaxNet]->maxVoltage != UNKNOWN_VOLTAGE);
		//return(netVoltagePtr_v[myMaxNet]->maxVoltage);
    }
}

bool CCvcDb::HasLeakPath(CFullConnection & theConnections) {
	 if ( theConnections.minSourcePower_p ) {
		 if ( ! theConnections.minSourcePower_p->active[MIN_ACTIVE] ) return false;
		 if ( theConnections.maxDrainPower_p && theConnections.minSourcePower_p->type[HIZ_BIT] ) {
			 if ( theConnections.drainId == maxNet_v[theConnections.drainId].nextNetId && theConnections.maxDrainVoltage == UNKNOWN_VOLTAGE ) return false;
//			 if ( theConnections.maxDrainPower_p->type[HIZ_BIT] ) return false;
			 return ! theConnections.minSourcePower_p->IsRelatedPower(theConnections.maxDrainPower_p, netVoltagePtr_v, minNet_v, maxNet_v, false);
		 }
	 }
	 if ( theConnections.minDrainPower_p ) {
		 if ( ! theConnections.minDrainPower_p->active[MIN_ACTIVE] ) return false;
		 if ( theConnections.maxSourcePower_p && theConnections.minDrainPower_p->type[HIZ_BIT] ) {
			 if ( theConnections.sourceId == maxNet_v[theConnections.sourceId].nextNetId && theConnections.maxSourceVoltage == UNKNOWN_VOLTAGE ) return false;
//			 if ( theConnections.maxSourcePower_p->type[HIZ_BIT] ) return false;
			 return ! theConnections.minDrainPower_p->IsRelatedPower(theConnections.maxSourcePower_p, netVoltagePtr_v, minNet_v, maxNet_v, false);
		 }
	 }
	 if ( theConnections.maxSourcePower_p ) {
		 if ( ! theConnections.maxSourcePower_p->active[MAX_ACTIVE] ) return false;
		 if ( theConnections.minDrainPower_p && theConnections.maxSourcePower_p->type[HIZ_BIT] ) {
			 if ( theConnections.drainId == minNet_v[theConnections.drainId].nextNetId && theConnections.minDrainVoltage == UNKNOWN_VOLTAGE ) return false;
//			 if ( theConnections.minDrainPower_p->type[HIZ_BIT] ) return false;
			 return ! theConnections.maxSourcePower_p->IsRelatedPower(theConnections.minDrainPower_p, netVoltagePtr_v, maxNet_v, minNet_v, false);
		 }
	 }
	 if ( theConnections.maxDrainPower_p ) {
		 if ( ! theConnections.maxDrainPower_p->active[MAX_ACTIVE] ) return false;
		 if ( theConnections.minSourcePower_p && theConnections.maxDrainPower_p->type[HIZ_BIT] ) {
			 if ( theConnections.sourceId == minNet_v[theConnections.sourceId].nextNetId && theConnections.minSourceVoltage == UNKNOWN_VOLTAGE ) return false;
//			 if ( theConnections.minSourcePower_p->type[HIZ_BIT] ) return false;
			 return ! theConnections.maxDrainPower_p->IsRelatedPower(theConnections.minSourcePower_p, netVoltagePtr_v, maxNet_v, minNet_v, false);
		 }
	 }
	 if ( theConnections.simSourcePower_p && theConnections.simSourcePower_p->defaultSimNet == theConnections.drainId ) return false;  // ignore leaks to own calculated voltage
	 if ( theConnections.simDrainPower_p && theConnections.simDrainPower_p->defaultSimNet == theConnections.sourceId ) return false;  // ignore leaks to own calculated voltage
	 netId_t myMinSourceNet = minNet_v[theConnections.sourceId].nextNetId;
	 netId_t myMaxSourceNet = maxNet_v[theConnections.sourceId].nextNetId;
	 netId_t myMinDrainNet = minNet_v[theConnections.drainId].nextNetId;
	 netId_t myMaxDrainNet = maxNet_v[theConnections.drainId].nextNetId;
	 CPower * myPower_p = netVoltagePtr_v[theConnections.sourceId];
	 if ( myPower_p ) {
		 if ( myPower_p->type[MIN_CALCULATED_BIT] && myPower_p->defaultMinNet != UNKNOWN_NET ) myMinSourceNet = myPower_p->defaultMinNet;
		 if ( myPower_p->type[MAX_CALCULATED_BIT] && myPower_p->defaultMaxNet != UNKNOWN_NET ) myMaxSourceNet = myPower_p->defaultMaxNet;
	 }
	 myPower_p = netVoltagePtr_v[theConnections.drainId];
	 if ( myPower_p ) {
		 if ( myPower_p->type[MIN_CALCULATED_BIT] && myPower_p->defaultMinNet != UNKNOWN_NET ) myMinDrainNet = myPower_p->defaultMinNet;
		 if ( myPower_p->type[MAX_CALCULATED_BIT] && myPower_p->defaultMaxNet != UNKNOWN_NET ) myMaxDrainNet = myPower_p->defaultMaxNet;
	 }
	 if ( (myMinSourceNet == theConnections.drainId || theConnections.minSourceVoltage == UNKNOWN_VOLTAGE)
			 && (myMaxSourceNet == theConnections.drainId || theConnections.maxSourceVoltage == UNKNOWN_VOLTAGE) ) return false;  // min/max path same or non-existant
//			 && connectionCount_v[theConnections.sourceId].gateCount == 0) return false;  // min/max path same with no gate connections
	 if ( (myMinDrainNet == theConnections.sourceId || theConnections.minDrainVoltage == UNKNOWN_VOLTAGE)
		 	 && (myMaxDrainNet == theConnections.sourceId || theConnections.maxDrainVoltage == UNKNOWN_VOLTAGE) ) return false;  // min/max path same with no gate connections
//			 && connectionCount_v[theConnections.drainId].gateCount == 0) return false;  // min/max path same with no gate connections
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

size_t CCvcDb::IncrementDeviceError(deviceId_t theDeviceId) {
	CInstance * myInstance_p = instancePtr_v[deviceParent_v[theDeviceId]];
	CCircuit * myParent_p = myInstance_p->master_p;
	return(myParent_p->deviceErrorCount_v[theDeviceId - myInstance_p->firstDeviceId] ++);
}

list<string> * CCvcDb::SplitHierarchy(string theFullPath) {
	list<string> * myHierarchy = new list<string>;
	size_t myStringStart = 0;
	if ( theFullPath.substr(0, 1) == "/" ) {
		myHierarchy->push_back("");
		myStringStart = 1;
	}
	size_t myHierarchyOffset = theFullPath.find(HIERARCHY_DELIMITER, myStringStart);
	while ( myHierarchyOffset <= theFullPath.size() ) {
		myHierarchy->push_back(theFullPath.substr(myStringStart, myHierarchyOffset - myStringStart));
		myStringStart = myHierarchyOffset + 1;
		myHierarchyOffset = theFullPath.find(HIERARCHY_DELIMITER, myStringStart);
	}
	myHierarchy->push_back(theFullPath.substr(myStringStart));
	return myHierarchy;
}

void CCvcDb::SaveMinMaxLeakVoltages() {
	leakVoltageSet = true;
	cout << "Saving min/max voltages..." << endl;
	minLeakNet_v(minNet_v);
	maxLeakNet_v(maxNet_v);
	leakVoltagePtr_v = netVoltagePtr_v;
}

void CCvcDb::SaveInitialVoltages() {
// use minLeak/maxLeak as initial vectors
//	initialMinNet_v = minNet_v;
//	initialMaxNet_v = maxNet_v;
	cout << "Saving simulation voltages..." << endl;
	initialSimNet_v(simNet_v);
	initialVoltagePtr_v = netVoltagePtr_v;
}

/*
void CCvcDb::SaveLogicVoltages() {
	logicMinNet_v = minNet_v;
	logicMaxNet_v = maxNet_v;
	logicSimNet_v = simNet_v;
	logicVoltagePtr_v = netVoltagePtr_v;
}
*/

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
	return( netVoltagePtr_v[myVirtualNet.finalNetId] && netVoltagePtr_v[myVirtualNet.finalNetId]->type[HIZ_BIT] );
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
