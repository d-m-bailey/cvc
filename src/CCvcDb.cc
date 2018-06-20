/*
 * CCvcDb.cc
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

#include "CCvcDb.hh"

#include "CCircuit.hh"
#include "CModel.hh"
#include "CDevice.hh"
#include "CPower.hh"
#include "CInstance.hh"
#include "CEventQueue.hh"
#include "CVirtualNet.hh"
#include "CConnection.hh"

extern set<modelType_t> FUSE_MODELS;
char RESISTOR_TEXT[] = "resistor";

void CCvcDb::ReportSimShort(deviceId_t theDeviceId, voltage_t theMainVoltage, voltage_t theShortVoltage, string theCalculation) {
	static CFullConnection myConnections;
//	CCircuit * myParent_p = instancePtr_v[deviceParent_v[theDeviceId]]->master_p;
	MapDeviceNets(theDeviceId, myConnections);
	voltage_t myMaxVoltage = max(theMainVoltage, theShortVoltage);
	voltage_t myMinVoltage = min(theMainVoltage, theShortVoltage);
//	float myLeakCurrent;
//	bool myVthFlag;
//	if ( myMaxVoltage == UNKNOWN_VOLTAGE || myMinVoltage == UNKNOWN_VOLTAGE ) {
//		myLeakCurrent = 0;
//		myVthFlag = false;
//	} else {
//		myVthFlag = (IsNmos_(deviceType_v[theDeviceId]) && myConnections.simGateVoltage - myMinVoltage == myConnections.device_p->model_p->Vth) ||
//				(IsPmos_(deviceType_v[theDeviceId]) && myConnections.simGateVoltage - myMaxVoltage == myConnections.device_p->model_p->Vth);
//		myLeakCurrent = myConnections.EstimatedCurrent(myVthFlag);
//	}
//	if ( rint((myLeakCurrent - cvcParameters.cvcLeakLimit) * 1e6) / 1e6 > 0 ||
//			! myConnections.simSourcePower_p->IsRelatedPower(myConnections.simDrainPower_p, netVoltagePtr_v, simNet_v, simNet_v, true) ||
//			(myConnections.simSourcePower_p->type[POWER_BIT] && myConnections.simDrainPower_p->type[POWER_BIT] && myMaxVoltage != myMinVoltage && ! myVthFlag) ) {
	if ( abs(theMainVoltage - theShortVoltage) != abs(myConnections.device_p->model_p->Vth) &&
			abs(theMainVoltage - theShortVoltage) > cvcParameters.cvcShortErrorThreshold ) {
		errorCount[LEAK]++;
		if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(theDeviceId) < cvcParameters.cvcCircuitErrorLimit ) {
			errorFile << "! Short Detected: " << PrintVoltage(myMaxVoltage) << " to " << PrintVoltage(myMinVoltage) << theCalculation << endl;
//			if ( myVthFlag ) errorFile << " at Vth ";
//			errorFile << " Estimated current: " << myLeakCurrent << endl;
//			if ( IsNmos_(deviceType_v[theDeviceId]) && myConnections.simGateVoltage == UNKNOWN_VOLTAGE && myConnections.minGateVoltage != UNKNOWN_VOLTAGE ) {
//				PrintDeviceWithMinGateAndSimConnections(deviceParent_v[theDeviceId], myConnections, errorFile);
//			} else if ( IsPmos_(deviceType_v[theDeviceId]) && myConnections.simGateVoltage == UNKNOWN_VOLTAGE && myConnections.maxGateVoltage != UNKNOWN_VOLTAGE ) {
//				PrintDeviceWithMaxGateAndSimConnections(deviceParent_v[theDeviceId], myConnections, errorFile);
//			} else {
				PrintDeviceWithAllConnections(deviceParent_v[theDeviceId], myConnections, errorFile);
//			}
			errorFile << endl;
		}
	}
//	}
}

void CCvcDb::ReportShort(deviceId_t theDeviceId) {
	static CFullConnection myConnections;
//	CCircuit * myParent_p = instancePtr_v[deviceParent_v[theDeviceId]]->master_p;
	MapDeviceNets(theDeviceId, myConnections);
	voltage_t myMaxVoltage = max(myConnections.simSourceVoltage, myConnections.simDrainVoltage);
	voltage_t myMinVoltage = min(myConnections.simSourceVoltage, myConnections.simDrainVoltage);
	float myLeakCurrent;
	bool myVthFlag;
	if ( myConnections.simSourcePower_p->type[HIZ_BIT]
		&& myConnections.simSourcePower_p->IsRelatedPower(myConnections.simDrainPower_p, netVoltagePtr_v, simNet_v, simNet_v, false) ) return;
		// no shorts from Hi-Z to expected leak path
	if ( myConnections.simDrainPower_p->type[HIZ_BIT]
		&& myConnections.simDrainPower_p->IsRelatedPower(myConnections.simSourcePower_p, netVoltagePtr_v, simNet_v, simNet_v, false) ) return;
		// no shorts from Hi-Z to expected leak path
	if ( myMaxVoltage == UNKNOWN_VOLTAGE || myMinVoltage == UNKNOWN_VOLTAGE ) {
		myLeakCurrent = 0;
		myVthFlag = false;
	} else {
		myVthFlag = (IsNmos_(deviceType_v[theDeviceId]) && myConnections.simGateVoltage - myMinVoltage == myConnections.device_p->model_p->Vth) ||
				(IsPmos_(deviceType_v[theDeviceId]) && myConnections.simGateVoltage - myMaxVoltage == myConnections.device_p->model_p->Vth);
		myLeakCurrent = myConnections.EstimatedCurrent(myVthFlag);
	}
	if ( ExceedsLeakLimit_(myLeakCurrent)  // flag all leaks with large current
			|| ! myConnections.simSourcePower_p->IsRelatedPower(myConnections.simDrainPower_p, netVoltagePtr_v, simNet_v, simNet_v, true)  // flag leaks between unrelated power
			|| ( myMaxVoltage != myMinVoltage && ! myVthFlag  // ignore leaks between same voltages or at Vth
				&& ( ( IsExternalPower_(myConnections.simSourcePower_p)
						&& IsExternalPower_(myConnections.simDrainPower_p)
						&& myMaxVoltage - myMinVoltage > cvcParameters.cvcShortErrorThreshold )  // flag leaks between power nets
					|| myConnections.simSourcePower_p->flagAllShorts || myConnections.simDrainPower_p->flagAllShorts ) ) ) {  // flag nets for calculated logic levels
		errorCount[LEAK]++;
		if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(theDeviceId) < cvcParameters.cvcCircuitErrorLimit ) {
			errorFile << "! Short Detected: " << PrintVoltage(myMaxVoltage) << " to " << PrintVoltage(myMinVoltage);
			if ( myVthFlag ) errorFile << " at Vth ";
			errorFile << " Estimated current: " << AddSiSuffix(myLeakCurrent) << "A" << endl;
			if ( IsNmos_(deviceType_v[theDeviceId]) && myConnections.simGateVoltage == UNKNOWN_VOLTAGE && myConnections.minGateVoltage != UNKNOWN_VOLTAGE ) {
				PrintDeviceWithMinGateAndSimConnections(deviceParent_v[theDeviceId], myConnections, errorFile);
			} else if ( IsPmos_(deviceType_v[theDeviceId]) && myConnections.simGateVoltage == UNKNOWN_VOLTAGE && myConnections.maxGateVoltage != UNKNOWN_VOLTAGE ) {
				PrintDeviceWithMaxGateAndSimConnections(deviceParent_v[theDeviceId], myConnections, errorFile);
			} else {
				PrintDeviceWithSimConnections(deviceParent_v[theDeviceId], myConnections, errorFile);
			}
			errorFile << endl;
		}
	}
}

/**
 * \brief Check virtual net connection to see if reroute is necessary.
 *
 * When the source and drain final virtual net connections both point to the same net,
 * this routine determines whether or not to change the drain connection
 * by comparing the source connection to the next drain connection.
 * NOTE: this calculation is NOT propagated to subsequent devices in the voltage path.
 * Require:
 *   source net is defined terminal net
 *   drain net is not terminal net
 *   drain final net is defined net
 */
bool CCvcDb::CheckConnectionReroute(CEventQueue & theEventQueue, CConnection& theConnections, shortDirection_t theDirection) {
	// return true if connection should be rerouted
	netId_t myNextDrainNet;
	CPower * mySourcePower_p;
	resistance_t myDrainResistance;
//	if ( theEventQueue.queueType == MIN_QUEUE && ! IsNmos_(deviceType_v[theConnections.deviceId]) ) return false;
//	if ( theEventQueue.queueType == MAX_QUEUE && ! IsPmos_(deviceType_v[theConnections.deviceId]) ) return false;
	if ( theDirection == DRAIN_TO_MASTER_SOURCE ) {
		myNextDrainNet = theEventQueue.virtualNet_v[theConnections.drainId].nextNetId;
		mySourcePower_p = netVoltagePtr_v[theConnections.sourceId];
		myDrainResistance = theConnections.masterDrainNet.finalResistance;
	} else {  // swap source and drain
		assert(theDirection == SOURCE_TO_MASTER_DRAIN);
		myNextDrainNet = theEventQueue.virtualNet_v[theConnections.sourceId].nextNetId;
		mySourcePower_p = netVoltagePtr_v[theConnections.drainId];
		myDrainResistance = theConnections.masterSourceNet.finalResistance;
	}
	CPower * myNextDrainPower_p = netVoltagePtr_v[myNextDrainNet];
	if ( ! theEventQueue.virtualNet_v.IsTerminal(myNextDrainNet) ) return true;  // reroute if source is terminal, next drain not terminal
	// both terminal
	if ( myNextDrainPower_p->type[POWER_BIT] ) {
		if ( ! mySourcePower_p->type[POWER_BIT] ) return false;  // do not reroute if next drain is power, source not power
		// both power. reroute if less resistance.
		return (theConnections.resistance < myDrainResistance);
	}
	// next drain not power
	if ( mySourcePower_p->type[POWER_BIT] ) return true;  // reroute if source is power, next drain not power
	// both not power. reroute if drain is calculated, source not calculated.
	return (myNextDrainPower_p->type[theEventQueue.virtualNet_v.calculatedBit] && ! mySourcePower_p->type[theEventQueue.virtualNet_v.calculatedBit]);
}

bool CCvcDb::IsPriorityDevice(CEventQueue& theEventQueue, modelType_t theModel) {
	if ( theEventQueue.queueType == MIN_QUEUE && ! IsNmos_(theModel) ) return false;
	if ( theEventQueue.queueType == MAX_QUEUE && ! IsPmos_(theModel) ) return false;
	return true;
}

void CCvcDb::AlreadyShorted(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections) {
	// add logic for equivalent voltage shorts.
	static CVirtualNet myLastVirtualNet;
	if ( theConnections.masterSourceNet.finalNetId == theConnections.masterDrainNet.finalNetId ) {
		// no shifting on second pass causes looping. Only reroute direct connections.
		if ( IsPriorityDevice(theEventQueue, deviceType_v[theConnections.deviceId]) ) {
			if ( leakVoltageSet ) {  // second pass. Only reroute direct connections
				if ( ! theEventQueue.virtualNet_v.IsTerminal(theConnections.drainId)
						&& theEventQueue.virtualNet_v.IsTerminal(theConnections.sourceId)
						&& CheckConnectionReroute(theEventQueue, theConnections, DRAIN_TO_MASTER_SOURCE) ) {  // direct connections have precedence
					// lastUpdate changed due to virtual net rerouting
	//				if (gDebug_cvc) cout << "DEBUG: rerouting " << theConnections.drainId << " to " << theConnections.sourceId << " at " << theConnections.deviceId << endl;
					theEventQueue.virtualNet_v.lastUpdate++;
					ShortNets(theEventQueue, theDeviceId, theConnections, DRAIN_TO_MASTER_SOURCE);
				} else if ( ! theEventQueue.virtualNet_v.IsTerminal(theConnections.sourceId)
						&& theEventQueue.virtualNet_v.IsTerminal(theConnections.drainId)
						&& CheckConnectionReroute(theEventQueue, theConnections, SOURCE_TO_MASTER_DRAIN) ) {  // direct connections have precedence
					// lastUpdate changed due to virtual net rerouting
	//				if (gDebug_cvc) cout << "DEBUG: rerouting " << theConnections.sourceId << " to " << theConnections.drainId << " at " << theConnections.deviceId << endl;
					theEventQueue.virtualNet_v.lastUpdate++;
					ShortNets(theEventQueue, theDeviceId, theConnections, SOURCE_TO_MASTER_DRAIN);
				}
			} else if ( ! theEventQueue.virtualNet_v.IsTerminal(theConnections.drainId)
					&& ( theEventQueue.virtualNet_v.IsTerminal(theConnections.sourceId)  // direct connections have precedence
						|| theConnections.masterSourceNet.finalResistance + theConnections.resistance < theConnections.masterDrainNet.finalResistance ) ) {
				myLastVirtualNet = theEventQueue.virtualNet_v[theConnections.drainId];
	//			bool myDirectReroute = theConnections.masterSourceNet.finalResistance == 0;
				// lastUpdate changed due to virtual net rerouting
				theEventQueue.virtualNet_v.lastUpdate++;
				ShortNets(theEventQueue, theDeviceId, theConnections, DRAIN_TO_MASTER_SOURCE);
	//			if ( ! myDirectReroute ) {
					ShiftVirtualNets(theEventQueue, theConnections.drainId, myLastVirtualNet,
						theConnections.masterSourceNet.finalResistance + theConnections.resistance, theConnections.masterDrainNet.finalResistance);
	//			}
			} else if ( ! theEventQueue.virtualNet_v.IsTerminal(theConnections.sourceId)
					&& ( theEventQueue.virtualNet_v.IsTerminal(theConnections.drainId)  // direct connections have precedence
							|| theConnections.masterDrainNet.finalResistance + theConnections.resistance < theConnections.masterSourceNet.finalResistance ) ) {
				myLastVirtualNet = theEventQueue.virtualNet_v[theConnections.sourceId];
	//			bool myDirectReroute = theConnections.masterDrainNet.finalResistance == 0;
				// lastUpdate changed due to virtual net rerouting
				theEventQueue.virtualNet_v.lastUpdate++;
				ShortNets(theEventQueue, theDeviceId, theConnections, SOURCE_TO_MASTER_DRAIN);
	//			if ( ! myDirectReroute ) {
					ShiftVirtualNets(theEventQueue, theConnections.sourceId, myLastVirtualNet,
						theConnections.masterDrainNet.finalResistance + theConnections.resistance, theConnections.masterSourceNet.finalResistance);
	//			}
			}
		}
	} else {
		if ( theEventQueue.queueType == SIM_QUEUE ) {
			if ( deviceType_v[theDeviceId] != FUSE_OFF ) {
				// TODO: should do relative check and/or alias check
				if ( theConnections.sourcePower_p->type[HIZ_BIT] && theConnections.sourcePower_p->relativeFriendly
						&& theConnections.sourcePower_p->family() == theConnections.drainPower_p->powerSignal() ) return;
				if ( theConnections.drainPower_p->type[HIZ_BIT] && theConnections.drainPower_p->relativeFriendly
						&& theConnections.drainPower_p->family() == theConnections.sourcePower_p->powerSignal() ) return;
				ReportShort(theDeviceId);
			}
		} else {
			if ( theEventQueue.queueType == MIN_QUEUE ) {
				// TODO: IsUnknown check may not be needed. Checked in calling function
				if ( netStatus_v[theConnections.sourceId][NEEDS_MIN_CONNECTION] && IsKnownVoltage_(theConnections.drainVoltage)
						&& IsVerifiedPower(theConnections.masterDrainNet.finalNetId) ) {
					voltage_t myMaxVoltage = MaxVoltage(theConnections.sourceId);
					assert(netVoltagePtr_v[theConnections.sourceId]);
					CPower * myPower_p = netVoltagePtr_v[theConnections.sourceId];
					if ( theConnections.sourceVoltage > theConnections.drainVoltage && ! ( leakVoltageSet && theConnections.drainPower_p->type[HIZ_BIT] ) ) {
						// ignore pull downs to HI-Z power in second pass
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullDownVoltage = theConnections.drainVoltage;
						netStatus_v[theConnections.sourceId][NEEDS_MIN_CONNECTION] = false;
						if ( gDebug_cvc ) cout << "DEBUG: removed min connection check for net: " << theConnections.sourceId << " "
								<< theConnections.sourceVoltage << ">" << theConnections.drainVoltage << endl;
					} else if ( theConnections.sourceVoltage == theConnections.drainVoltage
							&& theConnections.drainVoltage == myMaxVoltage && ! netStatus_v[theConnections.drainId][NEEDS_MIN_CONNECTION] ) {
						// handles propagation through mos switch that might get deleted in second pass for HiZ power. (needs verification)
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullDownVoltage = theConnections.drainVoltage;
						netStatus_v[theConnections.sourceId][NEEDS_MIN_CONNECTION] = false;
						if ( gDebug_cvc ) cout << "DEBUG: removed matching min connection check for net: " << theConnections.sourceId << " "
								<< theConnections.sourceVoltage << endl;
					} else if ( ! leakVoltageSet && IsNmos_(deviceType_v[theDeviceId]) && LastNmosConnection(MIN_PENDING, theConnections.sourceId) ) {
						// remove calculated net and reroute to drain. only for initial power prop.
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullDownVoltage = theConnections.drainVoltage;
						theEventQueue.virtualNet_v.Set(theConnections.sourceId, theConnections.drainId, theConnections.resistance, ++theEventQueue.virtualNet_v.lastUpdate);
						CheckResistorOverflow_(theEventQueue.virtualNet_v[theConnections.sourceId].finalResistance, theConnections.sourceId, logFile);
//						theEventQueue.virtualNet_v[theConnections.sourceId].nextNetId = theConnections.drainId;
//						theEventQueue.virtualNet_v[theConnections.sourceId].resistance = theConnections.resistance;
//						theEventQueue.virtualNet_v.SetFinalNet(theConnections.sourceId);
						if ( theConnections.sourceVoltage > theConnections.drainVoltage ) {
							netStatus_v[theConnections.sourceId][NEEDS_MIN_CONNECTION] = false;
							if ( gDebug_cvc ) cout << "DEBUG: rerouted min connection check for net: " <<  theConnections.sourceId << endl;
						}
					}
				}
				if ( netStatus_v[theConnections.drainId][NEEDS_MIN_CONNECTION] && IsKnownVoltage_(theConnections.sourceVoltage)
						&& IsVerifiedPower(theConnections.masterSourceNet.finalNetId) ) {
					voltage_t myMaxVoltage = MaxVoltage(theConnections.drainId);
					assert(netVoltagePtr_v[theConnections.drainId]);
					CPower * myPower_p = netVoltagePtr_v[theConnections.drainId];
					if ( theConnections.drainVoltage > theConnections.sourceVoltage && ! ( leakVoltageSet && theConnections.sourcePower_p->type[HIZ_BIT] ) ) {
						// ignore pull downs to HI-Z power in second pass
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullDownVoltage = theConnections.sourceVoltage;
						netStatus_v[theConnections.drainId][NEEDS_MIN_CONNECTION] = false;
						if ( gDebug_cvc ) cout << "DEBUG: removed min connection check for net: " << theConnections.drainId << " "
								<< theConnections.drainVoltage << ">" << theConnections.sourceVoltage << endl;
					} else if ( theConnections.sourceVoltage == theConnections.drainVoltage
							&& theConnections.sourceVoltage == myMaxVoltage && ! netStatus_v[theConnections.sourceId][NEEDS_MIN_CONNECTION] ) {
						// handles propagation through mos switch that might get deleted in second pass for HiZ power. (needs verification)
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullDownVoltage = theConnections.sourceVoltage;
						netStatus_v[theConnections.drainId][NEEDS_MIN_CONNECTION] = false;
						if ( gDebug_cvc ) cout << "DEBUG: removed matching min connection check for net: " << theConnections.drainId << " "
								<< theConnections.drainVoltage << endl;
					} else if ( ! leakVoltageSet && IsNmos_(deviceType_v[theDeviceId]) && LastNmosConnection(MIN_PENDING, theConnections.drainId) ) {
						// remove calculated net and reroute to drain. only for initial power prop.
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullDownVoltage = theConnections.sourceVoltage;
						theEventQueue.virtualNet_v.Set(theConnections.drainId, theConnections.sourceId, theConnections.resistance, ++theEventQueue.virtualNet_v.lastUpdate);
						CheckResistorOverflow_(theEventQueue.virtualNet_v[theConnections.drainId].finalResistance, theConnections.drainId, logFile);
//						theEventQueue.virtualNet_v[theConnections.drainId].nextNetId = theConnections.sourceId;
//						theEventQueue.virtualNet_v[theConnections.drainId].resistance = theConnections.resistance;
//						theEventQueue.virtualNet_v.SetFinalNet(theConnections.drainId);
						if ( theConnections.drainVoltage > theConnections.sourceVoltage ) {
							netStatus_v[theConnections.drainId][NEEDS_MIN_CONNECTION] = false;
							if ( gDebug_cvc ) cout << "DEBUG: rerouted min connection check for net: " <<  theConnections.drainId << endl;
						}
					}

				}
			}
			if ( theEventQueue.queueType == MAX_QUEUE ) {
				if ( netStatus_v[theConnections.sourceId][NEEDS_MAX_CONNECTION] && IsKnownVoltage_(theConnections.drainVoltage)
						&& IsVerifiedPower(theConnections.masterDrainNet.finalNetId) ) {
					voltage_t myMinVoltage = MinVoltage(theConnections.sourceId);
					assert(netVoltagePtr_v[theConnections.sourceId]);
					CPower * myPower_p = netVoltagePtr_v[theConnections.sourceId];
					if ( theConnections.sourceVoltage < theConnections.drainVoltage && ! ( leakVoltageSet && theConnections.drainPower_p->type[HIZ_BIT] ) ) {
						// ignore pull ups to HI-Z power in second pass
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullUpVoltage = theConnections.drainVoltage;
						netStatus_v[theConnections.sourceId][NEEDS_MAX_CONNECTION] = false;
						if ( gDebug_cvc ) cout << "DEBUG: removed max connection check for net: " << theConnections.sourceId << " "
								<< theConnections.sourceVoltage << "<" << theConnections.drainVoltage << endl;
					} else if ( theConnections.sourceVoltage == theConnections.drainVoltage
							&& theConnections.drainVoltage == myMinVoltage && ! netStatus_v[theConnections.drainId][NEEDS_MAX_CONNECTION] ) {
						// handles propagation through mos switch that might get deleted in second pass for HiZ power. (needs verification)
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullUpVoltage = theConnections.drainVoltage;
						netStatus_v[theConnections.sourceId][NEEDS_MAX_CONNECTION] = false;
						if ( gDebug_cvc ) cout << "DEBUG: removed matching max connection check for net: " << theConnections.sourceId << " "
								<< theConnections.sourceVoltage << endl;
					} else if ( ! leakVoltageSet && IsPmos_(deviceType_v[theDeviceId]) && LastPmosConnection(MAX_PENDING, theConnections.sourceId) ) {
						// remove calculated net and reroute to drain. only for initial power prop.
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullUpVoltage = theConnections.drainVoltage;
						theEventQueue.virtualNet_v.Set(theConnections.sourceId, theConnections.drainId, theConnections.resistance, ++theEventQueue.virtualNet_v.lastUpdate);
						CheckResistorOverflow_(theEventQueue.virtualNet_v[theConnections.sourceId].finalResistance, theConnections.sourceId, logFile);
//						theEventQueue.virtualNet_v[theConnections.sourceId].nextNetId = theConnections.drainId;
//						theEventQueue.virtualNet_v[theConnections.sourceId].resistance = theConnections.resistance;
//						theEventQueue.virtualNet_v.SetFinalNet(theConnections.sourceId);
						if ( theConnections.sourceVoltage < theConnections.drainVoltage ) {
							netStatus_v[theConnections.sourceId][NEEDS_MAX_CONNECTION] = false;
							if ( gDebug_cvc ) cout << "DEBUG: rerouted max connection check for net: " <<  theConnections.sourceId << endl;
						}
					}
				}
				if ( netStatus_v[theConnections.drainId][NEEDS_MAX_CONNECTION] && IsKnownVoltage_(theConnections.sourceVoltage)
						&& IsVerifiedPower(theConnections.masterSourceNet.finalNetId) ) {
					voltage_t myMinVoltage = MinVoltage(theConnections.drainId);
					assert(netVoltagePtr_v[theConnections.drainId]);
					CPower * myPower_p = netVoltagePtr_v[theConnections.drainId];
					if ( theConnections.drainVoltage < theConnections.sourceVoltage && ! ( leakVoltageSet && theConnections.sourcePower_p->type[HIZ_BIT] ) ) {
						// ignore pull ups to HI-Z power in second pass
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullUpVoltage = theConnections.sourceVoltage;
						netStatus_v[theConnections.drainId][NEEDS_MAX_CONNECTION] = false;
						if ( gDebug_cvc ) cout << "DEBUG: removed max connection check for net: " << theConnections.drainId << " "
								<< theConnections.drainVoltage << "<" << theConnections.sourceVoltage << endl;
					} else if ( theConnections.sourceVoltage == theConnections.drainVoltage
							&& theConnections.drainVoltage == myMinVoltage && ! netStatus_v[theConnections.sourceId][NEEDS_MAX_CONNECTION] ) {
						// handles propagation through mos switch that might get deleted in second pass for HiZ power. (needs verification)
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullUpVoltage = theConnections.sourceVoltage;
						netStatus_v[theConnections.drainId][NEEDS_MAX_CONNECTION] = false;
						if ( gDebug_cvc ) cout << "DEBUG: removed matching max connection check for net: " << theConnections.drainId << " "
								<< theConnections.drainVoltage << endl;
					} else if ( ! leakVoltageSet && IsPmos_(deviceType_v[theDeviceId]) && LastPmosConnection(MAX_PENDING, theConnections.drainId) ) {
						// remove calculated net and reroute to drain. only for initial power prop.
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						myPower_p->extraData->pullUpVoltage = theConnections.sourceVoltage;
						theEventQueue.virtualNet_v.Set(theConnections.drainId, theConnections.sourceId, theConnections.resistance, ++theEventQueue.virtualNet_v.lastUpdate);
						CheckResistorOverflow_(theEventQueue.virtualNet_v[theConnections.drainId].finalResistance, theConnections.drainId, logFile);
//						theEventQueue.virtualNet_v[theConnections.drainId].nextNetId = theConnections.sourceId;
//						theEventQueue.virtualNet_v[theConnections.drainId].resistance = theConnections.resistance;
//						theEventQueue.virtualNet_v.SetFinalNet(theConnections.drainId);
						if ( theConnections.drainVoltage < theConnections.sourceVoltage ) {
							netStatus_v[theConnections.drainId][NEEDS_MAX_CONNECTION] = false;
							if ( gDebug_cvc ) cout << "DEBUG: rerouted max connection check for net: " <<  theConnections.drainId << endl;
						}
					}
				}
			}
			// TODO: family check not correct. need to check for relatives
			if ( theConnections.sourcePower_p->family() != theConnections.drainPower_p->family() ) {
				if (gDebug_cvc) cout << "adding leak" << endl;
				theEventQueue.AddLeak(theDeviceId, theConnections);
			}
		}
	}
	deviceStatus_v[theDeviceId][theEventQueue.inactiveBit] = true;
}

bool CCvcDb::VoltageConflict(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections) {
/*
	if ( theQueueType == MAX_QUEUE ) {
		myTestSource
	}
*/
	// first MIN/MAX pass propagate to unknown open nets. SIM and second MIN/MAX cause error.
	if ( ! leakVoltageSet && ( theConnections.sourceVoltage == UNKNOWN_VOLTAGE || theConnections.drainVoltage == UNKNOWN_VOLTAGE ) ) return false;
	if ( theConnections.IsUnknownSourceVoltage() || theConnections.IsUnknownDrainVoltage() ) return false;
	// check voltage family conflicts later
	if ( theEventQueue.queueType == SIM_QUEUE && IsMos_(deviceType_v[theDeviceId]) && theConnections.gateVoltage == UNKNOWN_VOLTAGE ) return false;

	if (gDebug_cvc) cout << "already shorted VC " << gEventQueueTypeMap[theEventQueue.queueType] << " " << theDeviceId << endl;
	// activate voltages
	deviceStatus_v[theDeviceId][theEventQueue.pendingBit] = true;
	if ( theEventQueue.queueType == MIN_QUEUE ) {
		if ( theConnections.drainPower_p && theConnections.drainPower_p->minVoltage != UNKNOWN_VOLTAGE &&
				! theConnections.drainPower_p->active[MIN_ACTIVE] && ! theConnections.drainPower_p->active[MIN_IGNORE] ) {
			if ( theConnections.sourceVoltage <= theConnections.drainVoltage ) {
				theConnections.drainPower_p->active[MIN_ACTIVE] = true;
				EnqueueAttachedDevices(theEventQueue, theConnections.drainId, theConnections.drainVoltage);
			} else if ( IsGreaterVoltage_(MinLeakVoltage(theConnections.drainId), theConnections.drainVoltage) ) {
				reportFile << "WARNING: min voltage " << theConnections.drainPower_p->minVoltage << " ignored for " << NetName(theConnections.drainId, PRINT_CIRCUIT_OFF) \
						<< " at " << DeviceName(theDeviceId, PRINT_CIRCUIT_ON) << endl;
//				theConnections.drainVoltage = UNKNOWN_VOLTAGE;
//				return false;
			}
		} else if ( theConnections.sourcePower_p && theConnections.sourcePower_p->minVoltage != UNKNOWN_VOLTAGE &&
				! theConnections.sourcePower_p->active[MIN_ACTIVE] && ! theConnections.sourcePower_p->active[MIN_IGNORE] ) {
			if ( theConnections.drainVoltage <= theConnections.sourceVoltage ) {
				theConnections.sourcePower_p->active[MIN_ACTIVE] = true;
				EnqueueAttachedDevices(theEventQueue, theConnections.sourceId, theConnections.sourceVoltage);
			} else if ( IsGreaterVoltage_(MinLeakVoltage(theConnections.sourceId), theConnections.sourceVoltage) ) {
				reportFile << "WARNING: min voltage " << theConnections.sourcePower_p->minVoltage << " ignored for " << NetName(theConnections.sourceId, PRINT_CIRCUIT_OFF) \
						<< " at " << DeviceName(theDeviceId, PRINT_CIRCUIT_ON) << endl;
//				theConnections.sourceVoltage = UNKNOWN_VOLTAGE;
//				return false;
			}
		}
		// Unset min checks
		CStatus & myDrainStatus = netStatus_v[theConnections.drainId];
		CStatus & mySourceStatus = netStatus_v[theConnections.sourceId];
		bool mySourceVerified = ! (mySourceStatus[NEEDS_MIN_CHECK] || mySourceStatus[NEEDS_MIN_CONNECTION]);
			//|| mySourceStatus[NEEDS_MAX_CHECK] || mySourceStatus[NEEDS_MAX_CONNECTION]);
		bool myDrainVerified = ! (myDrainStatus[NEEDS_MIN_CHECK] || myDrainStatus[NEEDS_MIN_CONNECTION]);
			//|| myDrainStatus[NEEDS_MAX_CHECK] || myDrainStatus[NEEDS_MAX_CONNECTION]);
		if ( ( myDrainStatus[NEEDS_MIN_CHECK] || myDrainStatus[NEEDS_MIN_CONNECTION] )
				&& theConnections.sourceVoltage != UNKNOWN_VOLTAGE && theConnections.drainVoltage != UNKNOWN_VOLTAGE
				&& mySourceVerified
				&& theConnections.drainVoltage > theConnections.sourceVoltage ) {
			if ( gDebug_cvc ) cout << "DEBUG: Removed min check for net " << theConnections.drainId
					<< "(" << theConnections.drainVoltage << ") from " << theConnections.sourceId << "(" << theConnections.sourceVoltage << ")" << endl;
			assert(netVoltagePtr_v[theConnections.drainId]);
			CPower * myPower_p = netVoltagePtr_v[theConnections.drainId];
			if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
			myPower_p->extraData->pullDownVoltage = theConnections.sourceVoltage;
			myDrainStatus[NEEDS_MIN_CHECK] = myDrainStatus[NEEDS_MIN_CONNECTION] = false;
//			netStatus_v[theConnections.drainId] = myDrainStatus;
		}
		if ( ( mySourceStatus[NEEDS_MIN_CHECK] || mySourceStatus[NEEDS_MIN_CONNECTION] )
				&& theConnections.sourceVoltage != UNKNOWN_VOLTAGE && theConnections.drainVoltage != UNKNOWN_VOLTAGE
				&& myDrainVerified
				&& theConnections.sourceVoltage > theConnections.drainVoltage ) {
			if ( gDebug_cvc ) cout << "DEBUG: Removed min check for net " << theConnections.sourceId
					<< "(" << theConnections.sourceVoltage << ") from " << theConnections.drainId << "(" << theConnections.drainVoltage << ")" << endl;
			assert(netVoltagePtr_v[theConnections.sourceId]);
			CPower * myPower_p = netVoltagePtr_v[theConnections.sourceId];
			if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
			myPower_p->extraData->pullDownVoltage = theConnections.drainVoltage;
			mySourceStatus[NEEDS_MIN_CHECK] = mySourceStatus[NEEDS_MIN_CONNECTION] = false;
//			netStatus_v[theConnections.sourceId] = mySourceStatus;
		}
		// for mos connected as diode with source/drain known. Add max voltage propagation/check
		if ( IsMos_(deviceType_v[theDeviceId]) ) {
			voltage_t mySourceVoltage;
			netId_t myTargetNetId = UNKNOWN_NET;
			calculationType_t myCalculationType;
			if ( theConnections.sourceId == theConnections.gateId ) {
				myTargetNetId = theConnections.sourceId;
				mySourceVoltage = theConnections.drainVoltage;
				myCalculationType = theConnections.drainPower_p->minCalculationType;
			} else if ( theConnections.drainId == theConnections.gateId ) {
				myTargetNetId = theConnections.drainId;
				mySourceVoltage = theConnections.sourceVoltage;
				myCalculationType = theConnections.sourcePower_p->minCalculationType;
			}
			if ( myTargetNetId != UNKNOWN_NET ) {
				voltage_t myExpectedVoltage = mySourceVoltage + theConnections.device_p->model_p->Vth;
//				CVirtualNet myMaxNet(maxNet_v, theConnections.gateId);
				CConnection myMaxConnections;
				MapDeviceNets(theConnections.deviceId, maxEventQueue, myMaxConnections);
				if ( myCalculationType != DOWN_CALCULATION ) {  // do not cross propagate calculated voltages
					if ( myMaxConnections.gateVoltage == UNKNOWN_VOLTAGE ) {
						EnqueueAttachedDevices(maxEventQueue, myTargetNetId, myExpectedVoltage); // enqueue in opposite queue
					} else if ( ! myMaxConnections.gatePower_p->type[HIZ_BIT]
							&& ((myMaxConnections.gateVoltage > myExpectedVoltage && IsNmos_(deviceType_v[theConnections.deviceId]))
								|| (myMaxConnections.gateVoltage < myExpectedVoltage && IsPmos_(deviceType_v[theConnections.deviceId])) ) ) {
						float myLeakCurrent = myMaxConnections.EstimatedMosDiodeCurrent(mySourceVoltage, theConnections);
						if ( ExceedsLeakLimit_(myLeakCurrent) ) {
							PrintMaxVoltageConflict(myTargetNetId, myMaxConnections, myExpectedVoltage, myLeakCurrent);
	//						reportFile << "WARNING: Max voltage already set for " << NetName(myTargetNetId, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF);
	//						reportFile << " at mos diode " << DeviceName(theConnections.deviceId, PRINT_CIRCUIT_ON);
	//						reportFile << " expected/found " << myExpectedVoltage << "/" << myMaxConnections.gateVoltage;
	//						reportFile << " estimated current " << AddSiSuffix(myLeakCurrent) << "A" << endl;
						}
					}
				}
			}
		}
	} else if ( theEventQueue.queueType == MAX_QUEUE ) {
		if ( theConnections.drainPower_p && theConnections.drainPower_p->maxVoltage != UNKNOWN_VOLTAGE &&
				! theConnections.drainPower_p->active[MAX_ACTIVE] && ! theConnections.drainPower_p->active[MAX_IGNORE] ) {
			if ( theConnections.drainVoltage <= theConnections.sourceVoltage ) {
				theConnections.drainPower_p->active[MAX_ACTIVE] = true;
				EnqueueAttachedDevices(theEventQueue, theConnections.drainId, theConnections.drainVoltage);
			} else if ( IsLesserVoltage_(MaxLeakVoltage(theConnections.drainId), theConnections.drainVoltage) ) {
				reportFile << "WARNING: max voltage " << theConnections.drainPower_p->maxVoltage << " ignored for " << NetName(theConnections.drainId, PRINT_CIRCUIT_OFF, PRINT_HIERARCHY_OFF) \
						<< " at " << DeviceName(theDeviceId, PRINT_CIRCUIT_ON) << endl;
//				theConnections.drainVoltage = UNKNOWN_VOLTAGE;
//				return false;
			}
		} else if ( theConnections.sourcePower_p && theConnections.sourcePower_p->maxVoltage != UNKNOWN_VOLTAGE &&
				! theConnections.sourcePower_p->active[MAX_ACTIVE] && ! theConnections.sourcePower_p->active[MAX_IGNORE] ) {
			if ( theConnections.sourceVoltage <= theConnections.drainVoltage ) {
				theConnections.sourcePower_p->active[MAX_ACTIVE] = true;
				EnqueueAttachedDevices(theEventQueue, theConnections.sourceId, theConnections.sourceVoltage);
			} else if ( IsLesserVoltage_(MaxLeakVoltage(theConnections.sourceId), theConnections.sourceVoltage) ) {
				reportFile << "WARNING: max voltage " << theConnections.sourcePower_p->maxVoltage << " ignored for " << NetName(theConnections.sourceId, PRINT_CIRCUIT_OFF, PRINT_HIERARCHY_OFF) \
						<< " at " << DeviceName(theDeviceId, PRINT_CIRCUIT_ON) << endl;
//				theConnections.sourceVoltage = UNKNOWN_VOLTAGE;
//				return false;
			}
		}
		// Unset max checks
		CStatus & myDrainStatus = netStatus_v[theConnections.drainId];
		CStatus & mySourceStatus = netStatus_v[theConnections.sourceId];
		bool mySourceVerified = ! (mySourceStatus[NEEDS_MAX_CHECK] || mySourceStatus[NEEDS_MAX_CONNECTION]);
		bool myDrainVerified = ! (myDrainStatus[NEEDS_MAX_CHECK] || myDrainStatus[NEEDS_MAX_CONNECTION]);
		if ( ( myDrainStatus[NEEDS_MAX_CHECK] || myDrainStatus[NEEDS_MAX_CONNECTION] )
				&& theConnections.sourceVoltage != UNKNOWN_VOLTAGE && theConnections.drainVoltage != UNKNOWN_VOLTAGE
				&& mySourceVerified
				&& theConnections.drainVoltage < theConnections.sourceVoltage ) {
			if ( gDebug_cvc ) cout << "DEBUG: Removed max check for net " << theConnections.drainId
					<< "(" << theConnections.drainVoltage << ") from " << theConnections.sourceId << "(" << theConnections.sourceVoltage << ")" << endl;
			assert(netVoltagePtr_v[theConnections.drainId]);
			CPower * myPower_p = netVoltagePtr_v[theConnections.drainId];
			if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
			myPower_p->extraData->pullUpVoltage = theConnections.sourceVoltage;
			myDrainStatus[NEEDS_MAX_CHECK] = myDrainStatus[NEEDS_MAX_CONNECTION] = false;
//			netStatus_v[theConnections.drainId] = myDrainStatus;
		}
		if ( ( mySourceStatus[NEEDS_MAX_CHECK] || mySourceStatus[NEEDS_MAX_CONNECTION] )
				&& theConnections.sourceVoltage != UNKNOWN_VOLTAGE && theConnections.drainVoltage != UNKNOWN_VOLTAGE
				&& myDrainVerified
				&& theConnections.sourceVoltage < theConnections.drainVoltage ) {
			if ( gDebug_cvc ) cout << "DEBUG: Removed max check for net " << theConnections.sourceId
					<< "(" << theConnections.sourceVoltage << ") from " << theConnections.drainId << "(" << theConnections.drainVoltage << ")" << endl;
			assert(netVoltagePtr_v[theConnections.sourceId]);
			CPower * myPower_p = netVoltagePtr_v[theConnections.sourceId];
			if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
			myPower_p->extraData->pullUpVoltage = theConnections.drainVoltage;
			mySourceStatus[NEEDS_MAX_CHECK] = mySourceStatus[NEEDS_MAX_CONNECTION] = false;
//			netStatus_v[theConnections.sourceId] = mySourceStatus;
		}

		// for mos connected as diode with source/drain known. Add min voltage propagation/check
		if ( IsMos_(deviceType_v[theDeviceId]) ) {
			voltage_t mySourceVoltage;
			netId_t myTargetNetId = UNKNOWN_NET;
			calculationType_t myCalculationType;
			if ( theConnections.sourceId == theConnections.gateId ) {
				myTargetNetId = theConnections.sourceId;
				mySourceVoltage = theConnections.drainVoltage;
				myCalculationType = theConnections.drainPower_p->maxCalculationType;
			} else if ( theConnections.drainId == theConnections.gateId ) {
				myTargetNetId = theConnections.drainId;
				mySourceVoltage = theConnections.sourceVoltage;
				myCalculationType = theConnections.sourcePower_p->maxCalculationType;
			}
			if ( myTargetNetId != UNKNOWN_NET ) {
				voltage_t myExpectedVoltage = mySourceVoltage + theConnections.device_p->model_p->Vth;
	//				CVirtualNet myMaxNet(maxNet_v, theConnections.gateId);
				CConnection myMinConnections;
				MapDeviceNets(theConnections.deviceId, minEventQueue, myMinConnections);
				if ( myCalculationType != UP_CALCULATION ) {  // do not cross propagate calculated voltages
					if ( myMinConnections.gateVoltage == UNKNOWN_VOLTAGE ) {
						EnqueueAttachedDevices(minEventQueue, myTargetNetId, myExpectedVoltage); // enqueue in opposite queue
					} else if ( ! myMinConnections.gatePower_p->type[HIZ_BIT]
							&& ((myMinConnections.gateVoltage > myExpectedVoltage && IsNmos_(deviceType_v[theConnections.deviceId]))
								|| (myMinConnections.gateVoltage < myExpectedVoltage && IsPmos_(deviceType_v[theConnections.deviceId])) ) ) {
						float myLeakCurrent = myMinConnections.EstimatedMosDiodeCurrent(mySourceVoltage, theConnections);
						if ( ExceedsLeakLimit_(myLeakCurrent) ) {
							PrintMinVoltageConflict(myTargetNetId, myMinConnections, myExpectedVoltage, myLeakCurrent);
	//						reportFile << "WARNING: Min voltage already set for " << NetName(myTargetNetId, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF);
	//						reportFile << " at mos diode " << DeviceName(theConnections.deviceId, PRINT_CIRCUIT_ON);
	//						reportFile << " expected/found " << myExpectedVoltage << "/" << myMinConnections.gateVoltage;
	//						reportFile << " estimated current " << AddSiSuffix(myLeakCurrent) << "A" << endl;
						}
					}
				}
			}
		}
	}
	deviceStatus_v[theDeviceId][theEventQueue.pendingBit] = false;
	AlreadyShorted(theEventQueue, theDeviceId, theConnections);

	return true;
}

/*
bool CCvcDb::BulkError(queueType_t theQueueType, CDevice * theDevice_p, netId_t theMasterBulkId, voltage_t theVoltage) {
	if ( theMasterBulkId == UNKNOWN_NET ) return false;
	if ( netVoltagePtr_v[theMasterBulkId] == NULL ) return false;
	if ( theQueueType == MAX_QUEUE && theDevice_p->model_p->type == PMOS ) {
		if ( netVoltagePtr_v[theMasterBulkId]->maximumVoltage == UNKNOWN_NET ) return false;
		if ( theVoltage > netVoltagePtr_v[theMasterBulkId]->maximumVoltage ) return true;

		if ( netVoltage_v[theMasterBulkId] == NULL ) return false;

		break; }
	default: { return false; }
	if ( netVoltage_v[theBulkId] == NULL ) return false;
	}
}
*/
bool CCvcDb::TopologicallyOffMos(eventQueue_t theQueueType, modelType_t theModelType, CConnection& theConnections) {
	if ( (theQueueType == MAX_QUEUE && IsPmos_(theModelType)) || (theQueueType == MIN_QUEUE && IsNmos_(theModelType)) ) {
		if ( theConnections.drainVoltage == UNKNOWN_VOLTAGE ) return theConnections.gateId == theConnections.sourceId;
		if ( theConnections.sourceVoltage == UNKNOWN_VOLTAGE ) return theConnections.gateId == theConnections.drainId;
	} else if ( (theQueueType == MAX_QUEUE && IsNmos_(theModelType)) || (theQueueType == MIN_QUEUE && IsPmos_(theModelType)) ) {
		if ( theConnections.drainVoltage == UNKNOWN_VOLTAGE ) return theConnections.gateId == theConnections.drainId;
		if ( theConnections.sourceVoltage == UNKNOWN_VOLTAGE ) return theConnections.gateId == theConnections.sourceId;
	}
	return false;
}

bool CCvcDb::IsOffMos(eventQueue_t theQueueType, deviceId_t theDeviceId, CConnection& theConnections, voltage_t theVoltage) {
	if ( TopologicallyOffMos(theQueueType, deviceType_v[theDeviceId], theConnections) ) {
		// TODO: possibly ignore for queue type
//		IgnoreDevice(theDeviceId);
		return true;
	}
	if ( theConnections.sourceVoltage == theVoltage || theConnections.drainVoltage == theVoltage ) {
		// TODO: Possible ordering problem. change to min/max Leak?
		if ( theQueueType == MAX_QUEUE && IsPmos_(deviceType_v[theDeviceId]) ) {
			voltage_t myMinGate = MinVoltage(theConnections.gateId);
			if ( myMinGate != UNKNOWN_VOLTAGE && myMinGate > theVoltage + theConnections.device_p->model_p->Vth ) return true;
		}
		if ( theQueueType == MIN_QUEUE && IsNmos_(deviceType_v[theDeviceId]) ) {
			voltage_t myMaxGate = MaxVoltage(theConnections.gateId);
			if ( myMaxGate != UNKNOWN_VOLTAGE && myMaxGate < theVoltage + theConnections.device_p->model_p->Vth ) return true;
		}
	}
	if ( theQueueType == SIM_QUEUE ) {
		if ( IsPmos_(deviceType_v[theDeviceId]) ) {
			voltage_t myGateVoltage = ( theConnections.gateVoltage == UNKNOWN_VOLTAGE ) ? MinVoltage(theConnections.gateId) : theConnections.gateVoltage;
			voltage_t mySourceVoltage = ( theConnections.sourceVoltage == UNKNOWN_VOLTAGE ) ? MaxLeakVoltage(theConnections.sourceId) : theConnections.sourceVoltage;
			voltage_t myDrainVoltage = ( theConnections.drainVoltage == UNKNOWN_VOLTAGE ) ? MaxLeakVoltage(theConnections.drainId) : theConnections.drainVoltage;
			if ( myGateVoltage == UNKNOWN_VOLTAGE || mySourceVoltage == UNKNOWN_VOLTAGE || myDrainVoltage == UNKNOWN_VOLTAGE ) return false;
			bool mySourceOff = myGateVoltage > mySourceVoltage + theConnections.device_p->model_p->Vth;
			bool myDrainOff = myGateVoltage > myDrainVoltage + theConnections.device_p->model_p->Vth;
			if ( theConnections.sourceVoltage == UNKNOWN_VOLTAGE && myDrainOff && ! mySourceOff && maxNet_v[theConnections.sourceId].nextNetId == theConnections.drainId ) return true;  // prevents vth drops back through mos.
			if ( theConnections.drainVoltage == UNKNOWN_VOLTAGE && ! myDrainOff && mySourceOff && maxNet_v[theConnections.drainId].nextNetId == theConnections.sourceId ) return true;  // prevents vth drops back through mos.
			if ( mySourceOff && myDrainOff ) return true;
		}
		if ( IsNmos_(deviceType_v[theDeviceId]) ) {
			voltage_t myGateVoltage = ( theConnections.gateVoltage == UNKNOWN_VOLTAGE ) ? MaxVoltage(theConnections.gateId) : theConnections.gateVoltage;
			voltage_t mySourceVoltage = ( theConnections.sourceVoltage == UNKNOWN_VOLTAGE ) ? MinLeakVoltage(theConnections.sourceId) : theConnections.sourceVoltage;
			voltage_t myDrainVoltage = ( theConnections.drainVoltage == UNKNOWN_VOLTAGE ) ? MinLeakVoltage(theConnections.drainId) : theConnections.drainVoltage;
			if ( myGateVoltage == UNKNOWN_VOLTAGE || mySourceVoltage == UNKNOWN_VOLTAGE || myDrainVoltage == UNKNOWN_VOLTAGE ) return false;
			bool mySourceOff = myGateVoltage < mySourceVoltage + theConnections.device_p->model_p->Vth;
			bool myDrainOff = myGateVoltage < myDrainVoltage + theConnections.device_p->model_p->Vth;
			if ( theConnections.sourceVoltage == UNKNOWN_VOLTAGE && myDrainOff && ! mySourceOff && minNet_v[theConnections.sourceId].nextNetId == theConnections.drainId ) return true;  // prevents vth drops back through mos.
			if ( theConnections.drainVoltage == UNKNOWN_VOLTAGE && ! myDrainOff && mySourceOff && minNet_v[theConnections.drainId].nextNetId == theConnections.sourceId ) return true;  // prevents vth drops back through mos.
			if ( mySourceOff && myDrainOff ) return true;		}
	}
	return false;
}

bool CCvcDb::LastNmosConnection(deviceStatus_t thePendingBit, netId_t theNetId) {
	int myConnectionCount = 0;
	for ( deviceId_t device_it = firstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
		if ( IsNmos_(deviceType_v[device_it]) && deviceStatus_v[device_it][thePendingBit] ) {
			if ( ++myConnectionCount > 1 ) return false;
		}
	}
	for ( deviceId_t device_it = firstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
		if ( IsNmos_(deviceType_v[device_it]) && deviceStatus_v[device_it][thePendingBit] ) {
			if ( ++myConnectionCount > 1 ) return false;
		}
	}
	return ( myConnectionCount < 2 );
}

bool CCvcDb::LastPmosConnection(deviceStatus_t thePendingBit, netId_t theNetId) {
	int myConnectionCount = 0;
	for ( deviceId_t device_it = firstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
		if ( IsPmos_(deviceType_v[device_it]) && deviceStatus_v[device_it][thePendingBit] ) {
			if ( ++myConnectionCount > 1 ) return false;
		}
	}
	for ( deviceId_t device_it = firstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
		if ( IsPmos_(deviceType_v[device_it]) && deviceStatus_v[device_it][thePendingBit] ) {
			if ( ++myConnectionCount > 1 ) return false;
		}
	}
	return ( myConnectionCount < 2 );
}

bool CCvcDb::IsIrrelevant(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, voltage_t theVoltage, queueStatus_t theQueueStatus) {
	// Already shorted nets are irrelevant
	if ( theConnections.masterSourceNet == theConnections.masterDrainNet) return true;
	// mos is off
	if ( theQueueStatus == ENQUEUE && theEventQueue.queueType != SIM_QUEUE ) return false; // MIN/MAX queues don't check for off mos because not in order
	if ( IsOffMos(theEventQueue.queueType, theDeviceId, theConnections, theVoltage) ) {
		if ( leakVoltageSet ) return true;
		// first time, propagate thru off mos if only chance
		netId_t myTargetNet;
		if ( theConnections.sourceVoltage == UNKNOWN_VOLTAGE ) {
			myTargetNet = theConnections.sourceId;
		} else if ( theConnections.drainVoltage == UNKNOWN_VOLTAGE ) {
			myTargetNet = theConnections.drainId;
		} else {
			// Voltage conflict with off mos
			if ( theEventQueue.queueType == MIN_QUEUE ) {
				if ( netStatus_v[theConnections.sourceId][NEEDS_MIN_CONNECTION] && theConnections.sourceVoltage > theConnections.drainVoltage ) {
					netStatus_v[theConnections.sourceId][NEEDS_MIN_CONNECTION] = false;
				} else if ( netStatus_v[theConnections.drainId][NEEDS_MIN_CONNECTION] && theConnections.drainVoltage > theConnections.sourceVoltage ) {
					netStatus_v[theConnections.drainId][NEEDS_MIN_CONNECTION] = false;
				}
			} else if  ( theEventQueue.queueType == MAX_QUEUE ) {
				if ( netStatus_v[theConnections.sourceId][NEEDS_MAX_CONNECTION] && theConnections.sourceVoltage < theConnections.drainVoltage ) {
					netStatus_v[theConnections.sourceId][NEEDS_MAX_CONNECTION] = false;
				} else if ( netStatus_v[theConnections.drainId][NEEDS_MAX_CONNECTION] && theConnections.drainVoltage < theConnections.sourceVoltage ) {
					netStatus_v[theConnections.drainId][NEEDS_MAX_CONNECTION] = false;
				}
			}
			return true;
		}
		// first time enqueue all off mos, subsequently only process last connections
		if ( theEventQueue.queueType == MIN_QUEUE && IsNmos_(deviceType_v[theDeviceId]) ) {
			if ( theEventQueue.dequeueCount > 0 && ! LastNmosConnection(MIN_PENDING, myTargetNet) ) return true; // LDDN is NMOS in connection count
		} else if ( theEventQueue.queueType == MAX_QUEUE && IsPmos_(deviceType_v[theDeviceId]) ) {
			if ( theEventQueue.dequeueCount > 0 && ! LastPmosConnection(MAX_PENDING, myTargetNet) ) return true; // LDDP is PMOS in connection count
		} else {
			return true;
		}
	}
	// devices connecting 2 powers to be handled later

	// Skip short checks on enqueue (common for MIN/MAX/SIM).
	if ( theQueueStatus == DEQUEUE ) {
//		if ( theEventQueue.queueType == SIM_QUEUE ) {
//			cout << "WARNING: unexpected event" << endl;
//		}
		if ( VoltageConflict(theEventQueue, theDeviceId, theConnections) ) {
			/*
			if ( ! leakVoltageSet ) { // first pass only
				if ( ( deviceType_v[theDeviceId] == LDDN && theConnections.sourceVoltage > theConnections.drainVoltage ) ||
						( deviceType_v[theDeviceId] == LDDP && theConnections.sourceVoltage < theConnections.drainVoltage ) ) {
					ReportBadLddConnection(theEventQueue, theConnections.deviceId);
				}
			}
			*/
			return true;
		}
	}

	// wrong bias: delays, does not prevent
//	CVirtualNet myBulkMaster(theVirtualNet_v, myBulkId);
//	if ( BulkError(theEventQueue.queueType, myDevice_p, myBulkMaster.nextNetId, theVoltage) ) {

	// other conditions
	return false;
}

string CCvcDb::AdjustMaxPmosKey(CConnection& theConnections, netId_t theDrainId, voltage_t theMaxSource, eventKey_t& theEventKey, queuePosition_t& theQueuePosition) {
//	theQueuePosition = 0;
	string myAdjustment = "";
	if ( theConnections.gateId == theDrainId ) {
		if ( theConnections.device_p->model_p->Vth < 0 ) {
			myAdjustment = " PMOS Vth drop " + PrintParameter(theEventKey, VOLTAGE_SCALE) + PrintParameter(theConnections.device_p->model_p->Vth, VOLTAGE_SCALE);
			theEventKey = theEventKey + theConnections.device_p->model_p->Vth;
		}
		theQueuePosition = MOS_DIODE; // mos diode
	} else {
		if ( connectionCount_v[theDrainId].sourceDrainType[NMOS] ) {
			theQueuePosition = DELAY_FRONT; // front of delay queue
		}
		if ( theConnections.bulkVoltage == UNKNOWN_VOLTAGE ) {
			theQueuePosition = DELAY_BACK; // back of delay queue
		} else if ( theMaxSource > theConnections.bulkVoltage ) {
			theQueuePosition = DELAY_BACK; // back of delay queue
//			theEventKey = min(theConnections.bulkVoltage, theEventKey); // delay propagation until bulk voltage
//			theDelay = max(eventKey_t(0), (theConnections.bulkVoltage - theEventKey)); // bug: delay values are used as flags
		}
	}
	return(myAdjustment);
}

string CCvcDb::AdjustMaxNmosKey(CConnection& theConnections, voltage_t theSimGate, voltage_t theMaxSource, eventKey_t& theEventKey, queuePosition_t& theQueuePosition, bool theWarningFlag) {
	theQueuePosition = DELAY_BACK; // default: back of delay queue
	string myAdjustment = "";
	// do not use sim on first pass
	voltage_t myGateVoltage = ( ! isFixedSimNet || theSimGate == UNKNOWN_VOLTAGE ) ? MaxVoltage(theConnections.gateId) : theSimGate;
	if ( myGateVoltage == UNKNOWN_VOLTAGE ) {
//		if (theMinDrain == UNKNOWN_VOLTAGE ) {
		if ( connectionCount_v[theConnections.gateId].SourceDrainCount() == 0 ) {
			if (theWarningFlag) reportFile << "WARNING: unknown max gate at net->device: " << NetName(theConnections.gateId) << " -> " << HierarchyName(deviceParent_v[theConnections.deviceId], PRINT_CIRCUIT_ON) << "/" << theConnections.device_p->name << endl;
			if ( theConnections.device_p->model_p->Vth > 0 ) {
				myAdjustment = " NMOS Vth drop " + PrintParameter(theEventKey, VOLTAGE_SCALE) + "-" + PrintParameter(theConnections.device_p->model_p->Vth, VOLTAGE_SCALE);
				theEventKey = theEventKey - theConnections.device_p->model_p->Vth;
			}
		} else {
			theQueuePosition = SKIP_QUEUE; // no enqueue with unknown gate in maxNmos
		}
//		} else {
//			theEventKey = min(theEventKey - theConnections.device_p->model_p->Vth, eventKey_t(theMinDrain + theConnections.device_p->model_p->Vth));
//			theQueuePosition = -1;
//		}
	} else if ( myGateVoltage - theConnections.device_p->model_p->Vth <= theMaxSource ) {
		myAdjustment = " NMOS gate limited " + PrintParameter(myGateVoltage, VOLTAGE_SCALE) + "-" + PrintParameter(theConnections.device_p->model_p->Vth, VOLTAGE_SCALE);
		theEventKey = myGateVoltage - theConnections.device_p->model_p->Vth;
	} else {
		theEventKey = theMaxSource;
	}
	return(myAdjustment);
}

string CCvcDb::AdjustMinNmosKey(CConnection& theConnections, netId_t theDrainId, voltage_t theMinSource, eventKey_t& theEventKey, queuePosition_t& theQueuePosition) {
//	theQueuePosition = MAIN_BACK;
	string myAdjustment = "";
	if ( theConnections.gateId == theDrainId ) {
		if ( theConnections.device_p->model_p->Vth > 0 ) {
			myAdjustment = " NMOS Vth drop " + PrintParameter(theEventKey, VOLTAGE_SCALE) + "+" + PrintParameter(theConnections.device_p->model_p->Vth, VOLTAGE_SCALE);
			theEventKey = theEventKey + theConnections.device_p->model_p->Vth;
		}
		theQueuePosition = MOS_DIODE; // mos diode
	} else {
		if ( connectionCount_v[theDrainId].sourceDrainType[PMOS] ) {
			theQueuePosition = DELAY_FRONT; // front of delay queue
		}
		if ( theConnections.bulkVoltage == UNKNOWN_VOLTAGE ) {
			theQueuePosition = DELAY_BACK; // back of delay queue
		} else if ( theMinSource < theConnections.bulkVoltage ) {
			theQueuePosition = DELAY_BACK; // back of delay queue
//			theEventKey = theConnections.bulkVoltage;
//			theQueuePosition = max(eventKey_t(0), (theConnections.bulkVoltage - theEventKey)); // bug: delay values are used as flags
//			theEventKey = max(theConnections.bulkVoltage, theEventKey); // delay propagation until bulk voltage
		}
	}
	return(myAdjustment);
}

string CCvcDb::AdjustMinPmosKey(CConnection& theConnections, voltage_t theSimGate, voltage_t theMinSource, eventKey_t& theEventKey, queuePosition_t& theQueuePosition, bool theWarningFlag) {
	theQueuePosition = DELAY_BACK; // default: back of delay queue
	string myAdjustment = "";
	// do not use sim on first pass
	voltage_t myGateVoltage = ( ! isFixedSimNet || theSimGate == UNKNOWN_VOLTAGE ) ? MinVoltage(theConnections.gateId) : theSimGate;
	if ( myGateVoltage == UNKNOWN_VOLTAGE ) {
//		if (theMaxDrain == UNKNOWN_VOLTAGE ) {
//			if (theWarningFlag) cout << "WARNING: unknown min gate at device " << HierarchyName(deviceParent_v[theConnections.deviceId], PRINT_CIRCUIT_ON) << "/" << theConnections.device_p->name << endl;
//			theEventKey = max(theEventKey, theEventKey - theConnections.device_p->model_p->Vth);
		if ( connectionCount_v[theConnections.gateId].SourceDrainCount() == 0 ) {
			if (theWarningFlag) reportFile << "WARNING: unknown min gate at net->device: " << NetName(theConnections.gateId) << " -> " << HierarchyName(deviceParent_v[theConnections.deviceId], PRINT_CIRCUIT_ON) << "/" << theConnections.device_p->name << endl;
			if ( theConnections.device_p->model_p->Vth < 0 ) {
				myAdjustment = " PMOS Vth drop " + PrintParameter(theEventKey, VOLTAGE_SCALE) + "+" + PrintParameter(-theConnections.device_p->model_p->Vth, VOLTAGE_SCALE);
				theEventKey = theEventKey - theConnections.device_p->model_p->Vth;
			}
		} else {
			theQueuePosition = SKIP_QUEUE; // no enqueue with unknown gate in minPmos
		}
//		} else {
//			theEventKey = max(theEventKey - theConnections.device_p->model_p->Vth, eventKey_t(theMaxDrain + theConnections.device_p->model_p->Vth));
//			theDelay = -1;
//		}
	} else if ( myGateVoltage - theConnections.device_p->model_p->Vth >= theMinSource ) {
		myAdjustment = " PMOS gate limited " + PrintParameter(myGateVoltage, VOLTAGE_SCALE) + "+" + PrintParameter(-theConnections.device_p->model_p->Vth, VOLTAGE_SCALE);
		theEventKey = myGateVoltage - theConnections.device_p->model_p->Vth;
	} else {
		theEventKey = theMinSource;
	}
	return(myAdjustment);
}

string CCvcDb::AdjustKey(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, eventKey_t& theEventKey, queuePosition_t& theQueuePosition, shortDirection_t theDirection, bool theWarningFlag) {
	// MinMax queue Population+Propagation, Sim queue population only
//	theQueuePosition = 0; moved delay initialization to calling routine
	assert( (theConnections.sourceVoltage == UNKNOWN_VOLTAGE && theConnections.drainVoltage != UNKNOWN_VOLTAGE) \
			|| (theConnections.sourceVoltage != UNKNOWN_VOLTAGE && theConnections.drainVoltage == UNKNOWN_VOLTAGE) \
			|| (theEventQueue.queueType == SIM_QUEUE && theConnections.gateVoltage == UNKNOWN_VOLTAGE) );
	if ( deviceType_v[theDeviceId] == FUSE_ON || deviceType_v[theDeviceId] == FUSE_OFF ) {  // process fuses last
		theQueuePosition = DELAY_BACK;
	}
	if ( ! IsMos_(deviceType_v[theDeviceId]) ) return("");
	netId_t myLinkNetId;
	voltage_t myLinkVoltage;
	bool myIsCalculatedPower; // all SIM_QUEUE calculated power goes to delay queue
	bool myIsHiZPower;
	string myAdjustedVoltage = "";
	if ( theDirection == SOURCE_TO_MASTER_DRAIN ) { // propagate from drain
		myLinkNetId = theConnections.sourceId;
		myLinkVoltage = theConnections.drainVoltage;
		myIsCalculatedPower = theConnections.drainPower_p->type[SIM_CALCULATED_BIT];
		myIsHiZPower = theConnections.drainPower_p->type[HIZ_BIT];
		theQueuePosition = DefaultQueuePosition_(theConnections.drainPower_p->type[POWER_BIT], theEventQueue);
	} else {
		myLinkNetId = theConnections.drainId;
		myLinkVoltage = theConnections.sourceVoltage;
		myIsCalculatedPower = theConnections.sourcePower_p->type[SIM_CALCULATED_BIT];
		myIsHiZPower = theConnections.sourcePower_p->type[HIZ_BIT];
		theQueuePosition = DefaultQueuePosition_(theConnections.sourcePower_p->type[POWER_BIT], theEventQueue);
	}
//	if ( leakVoltageSet && myIsHiZPower && theEventQueue.queueType != SIM_QUEUE ) return; //
	if ( theEventQueue.queueType == MAX_QUEUE ) {
		if ( IsPmos_(deviceType_v[theDeviceId]) ) {
			if ( leakVoltageSet && myIsHiZPower ) { // no voltage change for HiZ nets in second min/max, high priority
//			if ( myIsHiZPower ) { // no voltage change for HiZ nets in min/max, high priority
				theQueuePosition = QUEUE_HIZ;
			} else {
				myAdjustedVoltage = AdjustMaxPmosKey(theConnections, myLinkNetId, myLinkVoltage, theEventKey, theQueuePosition);
			}
		} else { // NMOS
			if ( leakVoltageSet && myIsHiZPower ) { // no voltage change for HiZ nets in second min/max
//			if ( myIsHiZPower ) { // no voltage change for HiZ nets in  min/max
				theQueuePosition = DELAY_BACK;
			} else {
				myAdjustedVoltage = AdjustMaxNmosKey(theConnections, SimVoltage(theConnections.gateId), myLinkVoltage, theEventKey, theQueuePosition, theWarningFlag);
			}
/*
			if ( MinVoltage(myLinkNetId) == UNKNOWN_VOLTAGE ) {
				netStatus_v[myLinkNetId][MIN_ESTIMATE] = true;
			}
*/
		}
	} else if ( theEventQueue.queueType == MIN_QUEUE ) {
		if ( IsNmos_(deviceType_v[theDeviceId]) ) {
			if ( leakVoltageSet && myIsHiZPower ) { // no voltage change for HiZ nets in second min/max, high priority
//			if ( myIsHiZPower ) { // no voltage change for HiZ nets in  min/max, high priority
				theQueuePosition = QUEUE_HIZ;
			} else {
				myAdjustedVoltage = AdjustMinNmosKey(theConnections, myLinkNetId, myLinkVoltage, theEventKey, theQueuePosition);
			}
		} else { // PMOS
			if ( leakVoltageSet && myIsHiZPower ) { // no voltage change for HiZ nets in second min/max
//			if ( myIsHiZPower ) { // no voltage change for HiZ nets in  min/max
				theQueuePosition = DELAY_BACK;
			} else {
				myAdjustedVoltage = AdjustMinPmosKey(theConnections, SimVoltage(theConnections.gateId), myLinkVoltage, theEventKey, theQueuePosition, theWarningFlag);
			}
/*
			if ( MaxVoltage(myLinkNetId) == UNKNOWN_VOLTAGE ) {
				netStatus_v[myLinkNetId][MAX_ESTIMATE] = true;
			}
*/
		}
	} else if ( theEventQueue.queueType == SIM_QUEUE ) {
		theEventKey = SimKey(theEventQueue.QueueTime(), theConnections.resistance);
		if ( theConnections.gateId == theConnections.sourceId || theConnections.gateId == theConnections.drainId ) {
			theQueuePosition = DELAY_FRONT;
		} else if ( theConnections.gateVoltage == UNKNOWN_VOLTAGE ) { // check always on
			voltage_t myMinGateVoltage = MinVoltage(theConnections.gateId);
			voltage_t myMaxGateVoltage = MaxVoltage(theConnections.gateId);
			if ( IsNmos_(deviceType_v[theDeviceId]) && myMinGateVoltage != UNKNOWN_VOLTAGE
					&& myMinGateVoltage > myLinkVoltage + theConnections.device_p->model_p->Vth ) {
				; // always on. default delay
			} else if ( IsPmos_(deviceType_v[theDeviceId]) && myMaxGateVoltage != UNKNOWN_VOLTAGE
					&& myMaxGateVoltage < myLinkVoltage + theConnections.device_p->model_p->Vth ) {
				; // always on. default delay
			} else { // skip
				theQueuePosition = SKIP_QUEUE;
			}
		} else if ( IsNmos_(deviceType_v[theDeviceId]) && theConnections.gateVoltage < myLinkVoltage + theConnections.device_p->model_p->Vth ) { // off devices delayed
			theQueuePosition = DELAY_FRONT;
		} else if ( IsPmos_(deviceType_v[theDeviceId]) && theConnections.gateVoltage > myLinkVoltage + theConnections.device_p->model_p->Vth ) { // off devices delayed
			theQueuePosition = DELAY_FRONT;
		} else if ( myIsCalculatedPower ) {
			theQueuePosition = DELAY_FRONT;
		}
	}
	return(myAdjustedVoltage);
}

string CCvcDb::AdjustSimVoltage(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, voltage_t& theVoltage, shortDirection_t theDirection,
		propagation_t thePropagationType) {
	assert( (theConnections.sourceVoltage == UNKNOWN_VOLTAGE && theConnections.drainVoltage != UNKNOWN_VOLTAGE)
			|| (theConnections.sourceVoltage != UNKNOWN_VOLTAGE && theConnections.drainVoltage == UNKNOWN_VOLTAGE) );
	CPower * myPower_p = ( theDirection == SOURCE_TO_MASTER_DRAIN ) ? theConnections.sourcePower_p : theConnections.drainPower_p;
	netId_t myTargetNet = ( theDirection == SOURCE_TO_MASTER_DRAIN ) ? theConnections.sourceId : theConnections.drainId;
	voltage_t myMinTargetVoltage = MinSimVoltage(myTargetNet);
	voltage_t myMaxTargetVoltage = MaxSimVoltage(myTargetNet);
	voltage_t myOriginalVoltage = theVoltage;
	if ( myMinTargetVoltage > myMaxTargetVoltage ) {
		voltage_t myTrueMinVoltage = myMaxTargetVoltage;
		myMaxTargetVoltage = myMinTargetVoltage;
		myMinTargetVoltage = myTrueMinVoltage;
	}
	string myCalculation = "";
	if ( IsMos_(deviceType_v[theDeviceId]) ) {
		if ( theConnections.gateId == myTargetNet ) { // mos as diodes
			if ( myMinTargetVoltage == myMaxTargetVoltage && ( myPower_p && ! myPower_p->type[HIZ_BIT] ) ) { //may cause problem in nets not directly connected to power
				// only propagate if min = max
				myCalculation = " MOS sim drop " + PrintParameter(theVoltage, VOLTAGE_SCALE) + "V->" + PrintParameter(myMaxTargetVoltage, VOLTAGE_SCALE) + "V";
				theVoltage = myMaxTargetVoltage;
			} else {
				theVoltage = UNKNOWN_VOLTAGE;
			}
/*
			if ( deviceType_v[theDeviceId] == PMOS ) {
				theVoltage = min(theVoltage, theVoltage + theConnections.device_p->model_p->Vth);
			} else if ( deviceType_v[theDeviceId] == NMOS ) {
				theVoltage = max(theVoltage, theVoltage + theConnections.device_p->model_p->Vth);
			}
*/
		} else {
			if ( theConnections.gateVoltage == UNKNOWN_VOLTAGE ) {
				voltage_t myMinGateVoltage = MinVoltage(theConnections.gateId);
				voltage_t myMaxGateVoltage = MaxVoltage(theConnections.gateId);
				if ( IsNmos_(deviceType_v[theDeviceId]) && myMinGateVoltage != UNKNOWN_VOLTAGE
						&& myMinGateVoltage > theVoltage + theConnections.device_p->model_p->Vth ) {
					theConnections.gateVoltage = myMinGateVoltage; // always on
				} else if ( IsPmos_(deviceType_v[theDeviceId]) && myMaxGateVoltage != UNKNOWN_VOLTAGE
						&& myMaxGateVoltage < theVoltage + theConnections.device_p->model_p->Vth ) {
					theConnections.gateVoltage = myMaxGateVoltage; // always on
				}
			}
			assert(theConnections.gateVoltage != UNKNOWN_VOLTAGE);
			if ( theConnections.gateVoltage == theVoltage + theConnections.device_p->model_p->Vth ) {
				theVoltage = UNKNOWN_VOLTAGE; // skip simulation propagation
/* prevents MIN > SIM, MAX < SIM in NMOS or PMOS. However causes where NMOS meets PMOS.
				if ( deviceType_v[theDeviceId] == PMOS ) {
					myTargetVoltageLimit = MinVoltage(myTargetNet);
					theVoltage = (myTargetVoltageLimit == UNKNOWN_VOLTAGE) ? theConnections.gateVoltage : max(myTargetVoltageLimit, theConnections.gateVoltage);
				} else if ( deviceType_v[theDeviceId] == NMOS ) {
					myTargetVoltageLimit = MaxVoltage(myTargetNet);
					theVoltage = (myTargetVoltageLimit == UNKNOWN_VOLTAGE) ? theConnections.gateVoltage : min(myTargetVoltageLimit, theConnections.gateVoltage);
				}
*/
			} else if ( IsNmos_(deviceType_v[theDeviceId]) && myMinTargetVoltage != UNKNOWN_VOLTAGE ) {
				voltage_t myGateStepDown = theConnections.gateVoltage - theConnections.device_p->model_p->Vth;
				if ( myPower_p && myPower_p->type[HIZ_BIT] ) {
					;  // Don't adjust open voltages
				} else if ( myGateStepDown < theVoltage ) {
					myCalculation = " NMOS sim Vth drop " + PrintParameter(theConnections.gateVoltage, VOLTAGE_SCALE) + "-" + PrintParameter(theConnections.device_p->model_p->Vth, VOLTAGE_SCALE);
					myCalculation += " limited by " + PrintParameter(myMinTargetVoltage, VOLTAGE_SCALE);
					theVoltage = max(myGateStepDown, myMinTargetVoltage);
				}
			} else if ( IsPmos_(deviceType_v[theDeviceId]) && myMaxTargetVoltage != UNKNOWN_VOLTAGE) {
				voltage_t myGateStepUp = theConnections.gateVoltage - theConnections.device_p->model_p->Vth;
				if ( myPower_p && myPower_p->type[HIZ_BIT] ) {
					;  // Don't adjust open voltages
				} else if ( myGateStepUp > theVoltage ) {
					myCalculation = " PMOS sim Vth drop " + PrintParameter(theConnections.gateVoltage, VOLTAGE_SCALE) + "+" + PrintParameter(-theConnections.device_p->model_p->Vth, VOLTAGE_SCALE);
					myCalculation += " limited by " + PrintParameter(myMaxTargetVoltage, VOLTAGE_SCALE);
					theVoltage = min(myGateStepUp, myMaxTargetVoltage);
				}
			}
		}
	/*
		if ( theDirection == SOURCE_TO_MASTER_DRAIN ) {
			assert(theVoltage >= MinVoltage(theConnections.sourceId) || MinVoltage(theConnections.sourceId) == UNKNOWN_VOLTAGE);
			assert(theVoltage <= MaxVoltage(theConnections.sourceId) || MaxVoltage(theConnections.sourceId) == UNKNOWN_VOLTAGE);
		} else {
			assert(theVoltage >= MinVoltage(theConnections.drainId) || MinVoltage(theConnections.drainId) == UNKNOWN_VOLTAGE);
			assert(theVoltage <= MaxVoltage(theConnections.drainId) || MaxVoltage(theConnections.drainId) == UNKNOWN_VOLTAGE);
		}
*/
	}
	if ( theVoltage == UNKNOWN_VOLTAGE ) return("");
	if ( myMinTargetVoltage != UNKNOWN_VOLTAGE && theVoltage < myMinTargetVoltage ) {
		myCalculation += " Limited sim to min";
		if ( thePropagationType != POWER_NETS_ONLY  && myOriginalVoltage < myMinTargetVoltage ) ReportSimShort(theDeviceId, theVoltage, myMinTargetVoltage, myCalculation);
		// voltage drops in first pass are skipped, only report original voltage limits
		theVoltage = myMinTargetVoltage;
	}
	if ( myMaxTargetVoltage != UNKNOWN_VOLTAGE && theVoltage > myMaxTargetVoltage ) {
		myCalculation += " Limited sim to max";
		if ( thePropagationType != POWER_NETS_ONLY && myOriginalVoltage > myMaxTargetVoltage ) ReportSimShort(theDeviceId, theVoltage, myMaxTargetVoltage, myCalculation);
			// voltage drops in first pass are skipped, only report original voltage limits
		theVoltage = myMaxTargetVoltage;
	}
	return(myCalculation);
}

/*
bool CCvcDb::NeedsSwap(eventQueue_t theQueueType, modelType_t theModelType, shortDirection_t theDirection) {
	// do not use for simulation queue
	if ( theQueueType == MAX_QUEUE ) {
		if ( theModelType == NMOS ) {
			if ( theDirection == DRAIN_TO_MASTER_SOURCE ) {
				return true;
			}
		} else if ( theDirection == SOURCE_TO_MASTER_DRAIN ) { // PMOS | RES
			return true;
		}
	} else if ( theModelType == NMOS ) { // MIN_QUEUE
		if ( theDirection == SOURCE_TO_MASTER_DRAIN ) {
			return true;
		}
	} else if ( theDirection == DRAIN_TO_MASTER_SOURCE ) { // PMOS | RES
		return true;
	}
	return false;
}
*/

/*
void CCvcDb::SwapSourceDrain(CDevice * theDevice_p) {
	netId_t mySwapId;
	if ( IsMos_(theDevice_p->model_p->type) ) {
		mySwapId = theDevice_p->signalId_v[2];
		theDevice_p->signalId_v[2] = theDevice_p->signalId_v[0];
	} else {
		mySwapId = theDevice_p->signalId_v[1];
		theDevice_p->signalId_v[1] = theDevice_p->signalId_v[0];
	}
	theDevice_p->signalId_v[0] = mySwapId;
}
*/

/*
void CCvcDb::CheckSourceDrain(eventQueue_t theQueueType, CConnection& theConnections, shortDirection_t theDirection) {
	if ( theConnections.device_p->sourceDrainSet ) {
		if ( NeedsSwap(theQueueType, theConnections.device_p->model_p->type, theDirection) ) { // error: set devices should not need swap
			theConnections.device_p->sourceDrainSwapOk = false; // error
			theConnections.device_p->sourceDrainSet = false;
		}
	} else if ( theConnections.device_p->sourceDrainSwapOk ) {
		if ( NeedsSwap(theQueueType, theConnections.device_p->model_p->type, theDirection) ) {
			SwapSourceDrain(theConnections.device_p);
			theConnections.device_p->sourceDrainSwapOk = false;
		}
		theConnections.device_p->sourceDrainSet = true;
	}
}
*/

void CCvcDb::EnqueueAttachedDevicesByTerminal(CEventQueue& theEventQueue, netId_t theNetId, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v, eventKey_t theEventKey) {
	// TODO: possibly remove inactive devices from connection lists
	if ( theFirstDevice_v[theNetId] == UNKNOWN_DEVICE ) return;
	static CConnection myConnections;
	queuePosition_t myQueuePosition;
	eventKey_t myEventKey;
//	int myDeviceCount = 0;
	string myAdjustedCalculation;
	calculationType_t myCalculationType = UNKNOWN_CALCULATION;
	for (deviceId_t device_it = theFirstDevice_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theNextDevice_v[device_it]) {
		if ( deviceStatus_v[device_it][theEventQueue.inactiveBit] || deviceStatus_v[device_it][theEventQueue.pendingBit] ) {
//		if ( deviceStatus_v[device_it][theEventQueue.inactiveBit] ) { // TODO: possibly ignore pending only if later event
			continue; // skip inactive and pending devices
		} else {
//			bool myJustSetFlag = false;
			MapDeviceNets(device_it, theEventQueue, myConnections);
//			if ( myConnections.sourceVoltage == UNKNOWN_VOLTAGE && myConnections.drainVoltage == UNKNOWN_VOLTAGE ) continue;

			if ( IsIrrelevant(theEventQueue, device_it, myConnections, theEventKey, ENQUEUE) ) {
				deviceStatus_v[device_it][theEventQueue.inactiveBit] = true;
			} else if ( myConnections.sourceVoltage != UNKNOWN_VOLTAGE || myConnections.drainVoltage != UNKNOWN_VOLTAGE ) { // ignore bias/gate only changes
//				if ( theEventQueue.queueType == SIM_QUEUE && myConnections.gateVoltage == UNKNOWN_VOLTAGE && IsMos_(deviceType_v[device_it]) ) continue;
				myEventKey = theEventKey;
				if ( myConnections.sourceVoltage == UNKNOWN_VOLTAGE || myConnections.drainVoltage == UNKNOWN_VOLTAGE ) { // Adjust key for non-shorts
					shortDirection_t myDirection;
					if ( myConnections.sourceVoltage == UNKNOWN_VOLTAGE ) {
						myDirection = SOURCE_TO_MASTER_DRAIN;
						myQueuePosition = DefaultQueuePosition_(myConnections.drainPower_p->type[POWER_BIT], theEventQueue);
						myCalculationType = GetCalculationType(myConnections.drainPower_p, theEventQueue.queueType);
					} else {
						myDirection = DRAIN_TO_MASTER_SOURCE;
						myQueuePosition = DefaultQueuePosition_(myConnections.sourcePower_p->type[POWER_BIT], theEventQueue);
						myCalculationType = GetCalculationType(myConnections.sourcePower_p, theEventQueue.queueType);
					}
					myAdjustedCalculation = AdjustKey(theEventQueue, device_it, myConnections, myEventKey, myQueuePosition, myDirection, IGNORE_WARNINGS);
					// sets myQueuePosition, myEventKey
				} else {
					myQueuePosition = DefaultQueuePosition_(myConnections.sourcePower_p->type[POWER_BIT] || myConnections.drainPower_p->type[POWER_BIT], theEventQueue);
				}
				if ( myQueuePosition == SKIP_QUEUE ) continue; // SIM queues/maxNmos/minPmos with unknown mos gate return SKIP_QUEUE, skip for now
				if ( myEventKey < theEventKey && myCalculationType == UP_CALCULATION ) {
					cout << "DEBUG: Skipped down propagation at " << DeviceName(myConnections.deviceId);
					cout << " voltage " << PrintVoltage(myEventKey) << " from " << PrintVoltage(theEventKey) << endl;
					continue;  // do not propagate opposite calculations
				}
				if ( myEventKey > theEventKey && myCalculationType == DOWN_CALCULATION ) {
					cout << "DEBUG: Skipped up propagation at " << DeviceName(myConnections.deviceId);
					cout << " voltage " << PrintVoltage(myEventKey) << " from " << PrintVoltage(theEventKey) << endl;
					continue;  // do not propagate opposite calculations
				}
				if ( theEventQueue.queueType == SIM_QUEUE ) {
					myEventKey = SimKey(theEventQueue.QueueTime(), myConnections.resistance);
				}
				theEventQueue.AddEvent(myEventKey, device_it, myQueuePosition);
				deviceStatus_v[device_it][theEventQueue.pendingBit] = true;
				if ( myQueuePosition == MOS_DIODE && theEventQueue.queueType != SIM_QUEUE
						&& ! (myConnections.gatePower_p && myConnections.gatePower_p->type[HIZ_BIT]) ) { // mos diode connections in min/max queues
					netId_t myDrainNetId, mySourceNetId;
					voltage_t myMinSourceVoltage, myMaxSourceVoltage;
					voltage_t myMinDrainVoltage, myMaxDrainVoltage;
					if ( myConnections.sourceVoltage == UNKNOWN_VOLTAGE ) {
						assert ( myConnections.drainVoltage != UNKNOWN_VOLTAGE );
						myDrainNetId = myConnections.sourceId;
						mySourceNetId = myConnections.drainId;
					} else if ( myConnections.drainVoltage == UNKNOWN_VOLTAGE ) {
						myDrainNetId = myConnections.drainId;
						mySourceNetId = myConnections.sourceId;
					} else {
						throw EDatabaseError("EnqueueAttachedDevicesByTerminal: " + DeviceName(device_it));
					}
					myMinSourceVoltage = MinVoltage(mySourceNetId);
					myMaxSourceVoltage = MaxVoltage(mySourceNetId);
					myMinDrainVoltage = MinVoltage(myDrainNetId);
					myMaxDrainVoltage = MaxVoltage(myDrainNetId);
					if ( theEventQueue.queueType == MAX_QUEUE && myMinSourceVoltage == theEventKey ) { // set MIN voltage too
						// theEventKey = before drop, myEventKey = after drop
						voltage_t myMinLeakVoltage = MinLeakVoltage(myDrainNetId);
//						if ( myMinDrainVoltage == UNKNOWN_VOLTAGE && myMinLeakVoltage != UNKNOWN_VOLTAGE && myMinLeakVoltage <= myEventKey ) {
						if ( myMinDrainVoltage == UNKNOWN_VOLTAGE ) {
//							netVoltagePtr_v[myDrainNetId] = cvcParameters.cvcPowerPtrList.FindCalculatedPower(minEventQueue, netVoltagePtr_v[myDrainNetId], myEventKey, myDrainNetId, mySourceNetId, this);
							netVoltagePtr_v.CalculatePower(minEventQueue, myEventKey, myDrainNetId, mySourceNetId, this, myAdjustedCalculation);
							if ( gDebug_cvc ) cout << "DEBUG: Calculated power at net: " << myDrainNetId << " MIN " << myEventKey << endl;
							// do not increment lastUpdate
							minNet_v.Set(myDrainNetId, myDrainNetId, parameterResistanceMap[myConnections.device_p->parameters] * 100, minNet_v.lastUpdate);
							CheckResistorOverflow_(minNet_v[myDrainNetId].finalResistance, myDrainNetId, logFile);
//							minNet_v[myDrainNetId].resistance = parameterResistanceMap[myConnections.device_p->parameters] * 100;
							if ( leakVoltageSet && myMinLeakVoltage != UNKNOWN_VOLTAGE && myMinLeakVoltage <= myEventKey ) continue;
							assert(netVoltagePtr_v[myDrainNetId]);
							if ( netVoltagePtr_v[myDrainNetId]->pullDownVoltage() != UNKNOWN_VOLTAGE && netVoltagePtr_v[myDrainNetId]->pullDownVoltage() <= myEventKey ) continue;
							// pull down not known
							netStatus_v[myDrainNetId][NEEDS_MIN_CONNECTION] = true;
							if ( gDebug_cvc ) cout << "DEBUG: min estimate net: " << myDrainNetId << endl;
							if ( netStatus_v[mySourceNetId][NEEDS_MIN_CONNECTION] == true ) {
								if ( gDebug_cvc ) cout << "DEBUG: min estimate dependency net: " << mySourceNetId << endl;
								minConnectionDependencyMap[mySourceNetId].push_front(myDrainNetId);
							}
//							EnqueueAttachedDevices(maxEventQueue, myDrainNetId, myEventKey);
							// calculated power only
//							assert(netVoltagePtr_v[myDrainNetId]->type[CALCULATED_BIT]);
						} else {
/*
							if ( myMinDrainVoltage != myEventKey ) {
								reportFile << "WARNING: Min voltage already set for " << NetName(myDrainNetId, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF);
								reportFile << " at " << DeviceName(myConnections.deviceId, PRINT_CIRCUIT_ON)  << " expected/found " << myEventKey << "/" << myMinDrainVoltage << endl;
							}
*/
							if ( myMinDrainVoltage < myEventKey ) {
								reportFile << "WARNING: Min voltage already set for " << NetName(myDrainNetId, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF);
								reportFile << " at " << DeviceName(myConnections.deviceId, PRINT_CIRCUIT_ON)  << " expected/found " << myEventKey << "/" << myMinDrainVoltage << endl;
								netStatus_v[myDrainNetId][NEEDS_MIN_CONNECTION] = false;
								netStatus_v[mySourceNetId][NEEDS_MIN_CONNECTION] = false; // reset parent also
							}
						}
					} else if ( theEventQueue.queueType == MIN_QUEUE && myMaxSourceVoltage == theEventKey ) { // set MAX voltage too
						voltage_t myMaxLeakVoltage = MaxLeakVoltage(myDrainNetId);
//						if ( myMaxDrainVoltage == UNKNOWN_VOLTAGE && myMaxLeakVoltage != UNKNOWN_VOLTAGE && myMaxLeakVoltage >= myEventKey ) {
						if ( myMaxDrainVoltage == UNKNOWN_VOLTAGE ) {
//							netVoltagePtr_v[myDrainNetId] = cvcParameters.cvcPowerPtrList.FindCalculatedPower(maxEventQueue, netVoltagePtr_v[myDrainNetId], myEventKey, myDrainNetId, mySourceNetId, this);
							netVoltagePtr_v.CalculatePower(maxEventQueue, myEventKey, myDrainNetId, mySourceNetId, this, myAdjustedCalculation);
							if ( gDebug_cvc ) cout << "DEBUG: Calculated power at net: " << myDrainNetId << " MAX " << myEventKey << endl;
							// do not increment lastUpdate
							maxNet_v.Set(myDrainNetId, myDrainNetId, parameterResistanceMap[myConnections.device_p->parameters] * 100, maxNet_v.lastUpdate);
							CheckResistorOverflow_(maxNet_v[myDrainNetId].finalResistance, myDrainNetId, logFile);
//							maxNet_v[myDrainNetId].resistance = parameterResistanceMap[myConnections.device_p->parameters] * 100;
							if ( leakVoltageSet && myMaxLeakVoltage != UNKNOWN_VOLTAGE && myMaxLeakVoltage >= myEventKey ) continue;
							assert(netVoltagePtr_v[myDrainNetId]);
							if ( netVoltagePtr_v[myDrainNetId]->pullUpVoltage() != UNKNOWN_VOLTAGE && netVoltagePtr_v[myDrainNetId]->pullUpVoltage() >= myEventKey ) continue;
							// pull up not known
							netStatus_v[myDrainNetId][NEEDS_MAX_CONNECTION] = true;
							if ( gDebug_cvc ) cout << "DEBUG: max estimate net: " << myDrainNetId << endl;
							if ( netStatus_v[mySourceNetId][NEEDS_MAX_CONNECTION] == true ) {
								if ( gDebug_cvc ) cout << "DEBUG: max estimate dependency net: " << mySourceNetId << endl;
								maxConnectionDependencyMap[mySourceNetId].push_front(myDrainNetId);
							}
//							EnqueueAttachedDevices(minEventQueue, myDrainNetId, myEventKey);
							// calculated power only
//							assert(netVoltagePtr_v[myDrainNetId]->type[CALCULATED_BIT]);
						} else {
/*
							if ( myMaxDrainVoltage != myEventKey ) {
								reportFile << "WARNING: Max voltage already set for " << NetName(myDrainNetId, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF);
								reportFile << " at " << DeviceName(myConnections.deviceId, PRINT_CIRCUIT_ON)  << " expected/found " << myEventKey << "/" << myMaxDrainVoltage << endl;
							}
*/
							if ( myMaxDrainVoltage > myEventKey ) {
								reportFile << "WARNING: Max voltage already set for " << NetName(myDrainNetId, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF);
								reportFile << " at " << DeviceName(myConnections.deviceId, PRINT_CIRCUIT_ON)  << " expected/found " << myEventKey << "/" << myMaxDrainVoltage << endl;
								netStatus_v[myDrainNetId][NEEDS_MAX_CONNECTION] = false;
								netStatus_v[mySourceNetId][NEEDS_MAX_CONNECTION] = false; // reset parent also
							}
						}
					}
				}
			}
		}
/*
		if ( myDeviceCount++ > 50000 ) {
			cout << "ERROR: exceeded connected device limit for net# " << theNetId << endl;
			cout << "	name: " << NetName(theNetId) << endl;
			assert(myDeviceCount < 50000 );
		}
*/
	}
}

void CCvcDb::EnqueueAttachedDevices(CEventQueue& theEventQueue, netId_t theNetId, eventKey_t theEventKey) {
	EnqueueAttachedDevicesByTerminal(theEventQueue, theNetId, firstSource_v, nextSource_v, theEventKey);
	EnqueueAttachedDevicesByTerminal(theEventQueue, theNetId, firstDrain_v, nextDrain_v, theEventKey);
//	EnqueueAttachedDevicesByTerminal(theEventQueue, theNetId, firstBulk_v, nextBulk_v, theEventKey);
	EnqueueAttachedDevicesByTerminal(theEventQueue, theNetId, firstGate_v, nextGate_v, theEventKey);
}

void CCvcDb::EnqueueAttachedResistorsByTerminal(CEventQueue& theEventQueue, netId_t theNetId, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v, eventKey_t theEventKey, queuePosition_t theQueuePosition) {
	// TODO: possibly remove inactive devices from connection lists
	for (deviceId_t device_it = theFirstDevice_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theNextDevice_v[device_it]) {
		if ( deviceType_v[device_it] == RESISTOR ) {
			if ( deviceStatus_v[device_it][theEventQueue.inactiveBit] || deviceStatus_v[device_it][theEventQueue.pendingBit] ) {
//			if ( deviceStatus_v[device_it][theEventQueue.inactiveBit] ) {
				continue; // skip inactive and pending devices
			} else {
				theEventQueue.AddEvent(theEventKey, device_it, theQueuePosition);
				deviceStatus_v[device_it][theEventQueue.pendingBit] = true;
			}
		}
	}
}

void CCvcDb::EnqueueAttachedResistors(CEventQueue& theEventQueue, netId_t theNetId, eventKey_t theEventKey, queuePosition_t theQueuePosition) {
	EnqueueAttachedResistorsByTerminal(theEventQueue, theNetId, firstSource_v, nextSource_v, theEventKey, theQueuePosition);
	EnqueueAttachedResistorsByTerminal(theEventQueue, theNetId, firstDrain_v, nextDrain_v, theEventKey, theQueuePosition);
}

void CCvcDb::ShiftVirtualNets(CEventQueue& theEventQueue, netId_t theNewNetId, CVirtualNet& theLastVirtualNet, resistance_t theSourceResistance, resistance_t theDrainResistance) {
	netId_t myNetId;
	resistance_t myNewResistance;
//	RecalculateFinalResistance(theEventQueue, theNewNetId);
	int myShiftCount = 0;
	while ( long(theSourceResistance) + theLastVirtualNet.resistance < long(theDrainResistance) - theLastVirtualNet.resistance ) {
		if ( myShiftCount++ > 10000 ) {
			cout << "ERROR: exceeded shift limit for net# " << theNewNetId << endl;
			cout << "	name: " << NetName(theNewNetId) << endl;
			cout << "	next net is " << theEventQueue.virtualNet_v[theNewNetId].nextNetId << endl;
			assert(myShiftCount < 10000 );
		}
		if (gDebug_cvc) cout << gEventQueueTypeMap[theEventQueue.queueType] << " Shifted Net " << theNewNetId << endl;
		assert(long(theSourceResistance) + theLastVirtualNet.resistance < MAX_RESISTANCE);
		theSourceResistance += theLastVirtualNet.resistance;
		assert(theDrainResistance >= theLastVirtualNet.resistance);
		theDrainResistance -= theLastVirtualNet.resistance;
		myNetId = theLastVirtualNet.nextNetId;
		myNewResistance = theLastVirtualNet.resistance;
		theLastVirtualNet(theEventQueue.virtualNet_v, myNetId);
		if ( theEventQueue.virtualNet_v[myNetId].finalResistance != theDrainResistance ) {
			cout << "Shift: unexpected drain resistance at net " << myNetId
					<< " found " << theEventQueue.virtualNet_v[myNetId].finalResistance << " expected " << theDrainResistance << endl;
		}
		theEventQueue.virtualNet_v.Set(myNetId, theNewNetId, myNewResistance, ++theEventQueue.virtualNet_v.lastUpdate);
		if ( theEventQueue.virtualNet_v[myNetId].finalResistance != theSourceResistance ) {
			cout << "Shift: unexpected source resistance at net " << myNetId
					<< " found " << theEventQueue.virtualNet_v[myNetId].finalResistance << " expected " << theSourceResistance << endl;
		}
		CheckResistorOverflow_(theEventQueue.virtualNet_v[myNetId].finalResistance, myNetId, logFile);
//		theEventQueue.virtualNet_v[myNetId].nextNetId = theNewNetId;
//		theEventQueue.virtualNet_v[myNetId].resistance = myNewResistance;
//		theEventQueue.virtualNet_v.SetFinalNet(myNetId);
		theNewNetId = myNetId;
//		RecalculateFinalResistance(theEventQueue, theNewNetId);
	}
}

void CCvcDb::RecalculateFinalResistance(CEventQueue& theEventQueue, netId_t theNetId, bool theRecursingFlag) {
	netId_t mySourceId, myDrainId;
//	resistance_t myResistance;
	CConnection myConnections;
	CDevice myDevice;
	static unordered_set<netId_t> myProccessedNets;
	if ( ! theRecursingFlag ) {
		myProccessedNets.clear();
	}
//	cout << "Recalculating " << theNetId << endl;
	myProccessedNets.insert(theNetId);
	netId_t mySearchNetId = theNetId;
	while ( theEventQueue.virtualNet_v[mySearchNetId].nextNetId != mySearchNetId && theEventQueue.virtualNet_v[mySearchNetId].resistance == 0 ) {
		// find the master net for non-conducting resistors
		mySearchNetId = theEventQueue.virtualNet_v[mySearchNetId].nextNetId;
	}
	for (deviceId_t device_it = firstSource_v[mySearchNetId]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it]) {
//		if ( sourceNet_v[device_it] != theNetId ) continue; // ignore merged device terminal (process source but not drain)
		if ( deviceStatus_v[device_it][theEventQueue.inactiveBit] ) { // only process shorted nets
			SetDeviceNets(device_it, &myDevice, myConnections.sourceId, myConnections.gateId, myConnections.drainId, myConnections.bulkId);
//			MapDeviceNets(device_it, theEventQueue, myConnections);
			if ( myConnections.sourceId != theNetId ) continue; // skip devices that aren't connected to the net we want
			myDrainId = myConnections.drainId;
			if ( theNetId == theEventQueue.virtualNet_v[myDrainId].nextNetId ) { // only process nets that need to be recalculated
				theEventQueue.virtualNet_v.Set(myDrainId, theNetId, theEventQueue.virtualNet_v[myDrainId].resistance, ++theEventQueue.virtualNet_v.lastUpdate);
				CheckResistorOverflow_(theEventQueue.virtualNet_v[myDrainId].finalResistance, myDrainId, logFile);
//				cout << "Setting drain " << myDrainId << " to " << theNetId << endl;
//				if ( myProccessedNets.count(myDrainId) == 0 ) {
//					RecalculateFinalResistance(theEventQueue, myDrainId, true);
//				}
			}
		}
	}
	for (deviceId_t device_it = firstDrain_v[mySearchNetId]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it]) {
		if ( deviceStatus_v[device_it][theEventQueue.inactiveBit] ) { // only process shorted nets (process source but not drain)
//			if ( drainNet_v[device_it] != theNetId ) continue; // ignore merged device terminal
			SetDeviceNets(device_it, &myDevice, myConnections.sourceId, myConnections.gateId, myConnections.drainId, myConnections.bulkId);
			if ( myConnections.drainId != theNetId ) continue; // skip devices that aren't connected to the net we want
//			MapDeviceNets(device_it, theEventQueue, myConnections);
			mySourceId = myConnections.sourceId;
			if ( theNetId == theEventQueue.virtualNet_v[mySourceId].nextNetId ) { // only process nets that need to be recalculated
				theEventQueue.virtualNet_v.Set(mySourceId, theNetId, theEventQueue.virtualNet_v[mySourceId].resistance, ++theEventQueue.virtualNet_v.lastUpdate);
				CheckResistorOverflow_(theEventQueue.virtualNet_v[mySourceId].finalResistance, mySourceId, logFile);
//				cout << "Setting source " << mySourceId << " to " << theNetId << endl;
//				if ( myProccessedNets.count(mySourceId) == 0 ) {
//					RecalculateFinalResistance(theEventQueue, mySourceId, true);
//				}
			}
		}
	}
}

void CCvcDb::ShortNets(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, shortDirection_t theDirection) {
//	CheckSourceDrain(theEventQueue.queueType, theConnections, theDirection);
	if ( theDirection == DRAIN_TO_MASTER_SOURCE ) {
		// do not increment lastUpdate (increment in calling routine if necessary)
		theEventQueue.virtualNet_v.Set(theConnections.drainId, theConnections.sourceId, parameterResistanceMap[theConnections.device_p->parameters], theEventQueue.virtualNet_v.lastUpdate);
		CheckResistorOverflow_(theEventQueue.virtualNet_v[theConnections.drainId].finalResistance, theConnections.drainId, logFile);
//		theEventQueue.virtualNet_v[theConnections.drainId].nextNetId = theConnections.sourceId;
//		theEventQueue.virtualNet_v[theConnections.drainId].resistance = parameterResistanceMap[theConnections.device_p->parameters];
//		theEventQueue.virtualNet_v.SetFinalNet(theConnections.drainId);
		if ( deviceType_v[theDeviceId] == RESISTOR ) {
			PropagateConnectionType(theEventQueue.virtualNet_v, connectionCount_v[theConnections.drainId].sourceDrainType, theConnections.sourceId);
		}
	} else if ( theDirection == SOURCE_TO_MASTER_DRAIN ) {
		// do not increment lastUpdate (increment in calling routine if necessary)
		theEventQueue.virtualNet_v.Set(theConnections.sourceId, theConnections.drainId, parameterResistanceMap[theConnections.device_p->parameters], theEventQueue.virtualNet_v.lastUpdate);
		CheckResistorOverflow_(theEventQueue.virtualNet_v[theConnections.sourceId].finalResistance, theConnections.sourceId, logFile);
//		theEventQueue.virtualNet_v[theConnections.sourceId].nextNetId = theConnections.drainId;
//		theEventQueue.virtualNet_v[theConnections.sourceId].resistance = parameterResistanceMap[theConnections.device_p->parameters];
//		theEventQueue.virtualNet_v.SetFinalNet(theConnections.sourceId);
		if ( deviceType_v[theDeviceId] == RESISTOR ) {
			PropagateConnectionType(theEventQueue.virtualNet_v, connectionCount_v[theConnections.sourceId].sourceDrainType, theConnections.drainId);
		}
	} else {
		throw EDatabaseError("ShortNets: " + DeviceName(theDeviceId));
	}
	deviceStatus_v[theDeviceId][theEventQueue.inactiveBit] = true;
}

void CCvcDb::ShortNets(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, shortDirection_t theDirection, voltage_t theShortVoltage, string theCalculation) {
	// for min/max queues.
//	CheckSourceDrain(theEventQueue.queueType, theConnections, theDirection);
	netId_t myMasterNet, mySlaveNet;
	voltage_t myMasterVoltage;
	voltage_t mySimVoltage;
	if ( theDirection == DRAIN_TO_MASTER_SOURCE ) {
		myMasterNet = theConnections.sourceId;
		mySlaveNet = theConnections.drainId;
		myMasterVoltage = theConnections.sourceVoltage;
	} else if ( theDirection == SOURCE_TO_MASTER_DRAIN ) {
		myMasterNet = theConnections.drainId;
		mySlaveNet = theConnections.sourceId;
		myMasterVoltage = theConnections.drainVoltage;
	} else {
		throw EDatabaseError("ShortNets " + DeviceName(theDeviceId));
	}
//	string myCalculation = "";
	if ( isFixedSimNet ) {
		mySimVoltage = SimVoltage(mySlaveNet);
		if ( theEventQueue.queueType == MIN_QUEUE ) {
			if ( mySimVoltage != UNKNOWN_VOLTAGE && theShortVoltage > mySimVoltage ) {
				if (gDebug_cvc) cout << "DEBUG: min > sim (" << theShortVoltage << ">" << mySimVoltage << ")" << endl;
				theShortVoltage = mySimVoltage;
				theCalculation = " Limited min to sim";
			}
		} else if ( theEventQueue.queueType == MAX_QUEUE ) {
			if ( mySimVoltage != UNKNOWN_VOLTAGE && theShortVoltage < mySimVoltage ) {
				if (gDebug_cvc) cout << "DEBUG: max < sim (" << theShortVoltage << "<" << mySimVoltage << ")" << endl;
				theShortVoltage = mySimVoltage;
				theCalculation = " Limited max to sim";
			}
		}
	}
	if ( theShortVoltage == myMasterVoltage ) {
		// do not increment lastUpdate (increment in calling routine if necessary)
		theEventQueue.virtualNet_v.Set(mySlaveNet, myMasterNet, parameterResistanceMap[theConnections.device_p->parameters], theEventQueue.virtualNet_v.lastUpdate);
		CheckResistorOverflow_(theEventQueue.virtualNet_v[mySlaveNet].finalResistance, mySlaveNet, logFile);
//		theEventQueue.virtualNet_v[mySlaveNet].nextNetId = myMasterNet;
//		theEventQueue.virtualNet_v[mySlaveNet].resistance = parameterResistanceMap[theConnections.device_p->parameters];
	} else {
//		netVoltagePtr_v[mySlaveNet] = cvcParameters.cvcPowerPtrList.FindCalculatedPower(theEventQueue, netVoltagePtr_v[mySlaveNet], theShortVoltage, mySlaveNet, myMasterNet, this);
		netVoltagePtr_v.CalculatePower(theEventQueue, theShortVoltage, mySlaveNet, myMasterNet, this, theCalculation);
		if ( gDebug_cvc ) cout << "DEBUG: Calculated power at net: " << mySlaveNet << " " << gEventQueueTypeMap[theEventQueue.queueType] << " " << theShortVoltage << endl;
		// do not increment lastUpdate (increment in calling routine if necessary)
		theEventQueue.virtualNet_v.Set(mySlaveNet, mySlaveNet, parameterResistanceMap[theConnections.device_p->parameters] + theEventQueue.virtualNet_v[myMasterNet].finalResistance, theEventQueue.virtualNet_v.lastUpdate);
		CheckResistorOverflow_(theEventQueue.virtualNet_v[mySlaveNet].finalResistance, mySlaveNet, logFile);
	}
//	theEventQueue.virtualNet_v.SetFinalNet(mySlaveNet);
	if ( deviceType_v[theDeviceId] == RESISTOR ) {
		PropagateConnectionType(theEventQueue.virtualNet_v, connectionCount_v[mySlaveNet].sourceDrainType, myMasterNet);
	} else if ( deviceType_v[theDeviceId] == FUSE_ON || deviceType_v[theDeviceId] == FUSE_OFF ) {
//		voltage_t myOtherVoltage = UNKNOWN_VOLTAGE;
		netId_t myNextOtherNet;
		if ( theEventQueue.queueType == MIN_QUEUE ) {
//			myOtherVoltage = MaxVoltage(mySlaveNet);
			myNextOtherNet = maxNet_v[mySlaveNet].nextNetId;
		} else {
			assert( theEventQueue.queueType == MAX_QUEUE );
//			myOtherVoltage = MinVoltage(mySlaveNet);
			myNextOtherNet = minNet_v[mySlaveNet].nextNetId;
		}
//		if ( myOtherVoltage == theShortVoltage ) {
		if ( myNextOtherNet == myMasterNet ) {
			PrintFuseError(mySlaveNet, theConnections);
			//logFile << "WARNING: MIN = MAX " << theShortVoltage << " through FUSE at " << NetName(mySlaveNet, PRINT_CIRCUIT_ON) << endl;
		}
	}
/*
	if ( theDirection == DRAIN_TO_MASTER_SOURCE ) {
		if ( theShortVoltage == theConnections.sourceVoltage ) {
			theEventQueue.virtualNet_v[theConnections.drainId].nextNetId = theConnections.sourceId;
			theEventQueue.virtualNet_v[theConnections.drainId].resistance = parameterResistanceMap[theConnections.device_p->parameters];
		} else {
			netVoltagePtr_v[theConnections.drainId] = cvcParameters.cvcPowerPtrList.FindCalculatedPower(theEventQueue, netVoltagePtr_v[theConnections.drainId], theShortVoltage, theConnections.drainId, this);
		}
		if ( deviceType_v[theDeviceId] == RESISTOR ) {
			PropagateConnectionType(theEventQueue.virtualNet_v, connectionCount_v[theConnections.drainId].sourceDrainType, theConnections.sourceId);
		}
	} else if ( theDirection == SOURCE_TO_MASTER_DRAIN ) {
		if ( theShortVoltage == theConnections.drainVoltage ) {
			theEventQueue.virtualNet_v[theConnections.sourceId].nextNetId = theConnections.drainId;
			theEventQueue.virtualNet_v[theConnections.sourceId].resistance = parameterResistanceMap[theConnections.device_p->parameters];
		} else {
			netVoltagePtr_v[theConnections.sourceId] = cvcParameters.cvcPowerPtrList.FindCalculatedPower(theEventQueue, netVoltagePtr_v[theConnections.sourceId], theShortVoltage, theConnections.sourceId, this);
		}
		if ( deviceType_v[theDeviceId] == RESISTOR ) {
			PropagateConnectionType(theEventQueue.virtualNet_v, connectionCount_v[theConnections.sourceId].sourceDrainType, theConnections.drainId);
		}
	} else {
		throw EDatabaseError("ShortNets " + DeviceName(theDeviceId));
	}
*/
	deviceStatus_v[theDeviceId][theEventQueue.inactiveBit] = true;
}

void CCvcDb::ShortSimNets(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, shortDirection_t theDirection, voltage_t theShortVoltage, string theCalculation) {
//	CheckSourceDrain(theEventQueue.queueType, theConnections, theDirection);
	netId_t myMasterNet, mySlaveNet;
	voltage_t myMasterVoltage;
	CPower * myMasterPower_p;
	if ( theDirection == DRAIN_TO_MASTER_SOURCE ) {
		myMasterPower_p = theConnections.sourcePower_p;
		myMasterNet = theConnections.sourceId;
		mySlaveNet = theConnections.drainId;
		myMasterVoltage = theConnections.sourceVoltage;
	} else if ( theDirection == SOURCE_TO_MASTER_DRAIN ) {
		myMasterPower_p = theConnections.drainPower_p;
		myMasterNet = theConnections.drainId;
		mySlaveNet = theConnections.sourceId;
		myMasterVoltage = theConnections.drainVoltage;
	} else {
		throw EDatabaseError("ShortSimNets " + DeviceName(theDeviceId));
	}
	voltage_t myMaxSlaveVoltage = MaxLeakVoltage(mySlaveNet);
	voltage_t myMinSlaveVoltage = MinLeakVoltage(mySlaveNet);
	// 2013.11.13 added min max limits for sim voltages
	string myCalculation = "";
	if ( myMaxSlaveVoltage != UNKNOWN_VOLTAGE && myMaxSlaveVoltage < theShortVoltage ) {
		theShortVoltage = myMaxSlaveVoltage;
		myCalculation = " Limited to max";
	}
	if ( myMinSlaveVoltage != UNKNOWN_VOLTAGE && myMinSlaveVoltage > theShortVoltage ) {
		theShortVoltage = myMinSlaveVoltage;
		myCalculation = " Limited to min";
	}
	if ( theShortVoltage == myMasterVoltage ) {
		if ( IsSCRCPower(myMasterPower_p) && connectionCount_v[mySlaveNet].sourceDrainType[NMOS] && connectionCount_v[mySlaveNet].sourceDrainType[PMOS] ) {
			errorCount[LEAK]++;
			if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(theDeviceId) < cvcParameters.cvcCircuitErrorLimit ) {
				errorFile << "! Short Detected: SCRC " << PrintVoltage(myMasterVoltage) << " to output" << endl;
				static CFullConnection myConnections;
				MapDeviceNets(theDeviceId, myConnections);
				PrintDeviceWithSimConnections(deviceParent_v[theDeviceId], myConnections, errorFile);
				errorFile << endl;
			}
		}
		// do not increment lastUpdate
		theEventQueue.virtualNet_v.Set(mySlaveNet, myMasterNet, parameterResistanceMap[theConnections.device_p->parameters], theEventQueue.virtualNet_v.lastUpdate);
//		theEventQueue.virtualNet_v[mySlaveNet].nextNetId = myMasterNet;
//		theEventQueue.virtualNet_v[mySlaveNet].resistance = parameterResistanceMap[theConnections.device_p->parameters];
	} else {
//		netVoltagePtr_v[mySlaveNet] = cvcParameters.cvcPowerPtrList.FindCalculatedPower(theEventQueue, netVoltagePtr_v[mySlaveNet], theShortVoltage, mySlaveNet, myMasterNet, this);
		netVoltagePtr_v.CalculatePower(theEventQueue, theShortVoltage, mySlaveNet, myMasterNet, this, theCalculation + myCalculation);
		if ( gDebug_cvc ) cout << "DEBUG: Calculated power at net: " << mySlaveNet << " " << gEventQueueTypeMap[theEventQueue.queueType] << " " << theShortVoltage << endl;
//		theEventQueue.virtualNet_v.Set(mySlaveNet, mySlaveNet, parameterResistanceMap[theConnections.device_p->parameters]);
//		theEventQueue.virtualNet_v[mySlaveNet].resistance = parameterResistanceMap[theConnections.device_p->parameters];
		if ( IsMos_(deviceType_v[theDeviceId]) && theConnections.gateId == mySlaveNet ) { // gate/drain connections increase effective resistance (trace is broken so add master final resistance)
			// do not increment lastUpdate
			theEventQueue.virtualNet_v.Set(mySlaveNet, mySlaveNet, parameterResistanceMap[theConnections.device_p->parameters] * 100 + theEventQueue.virtualNet_v[myMasterNet].finalResistance, theEventQueue.virtualNet_v.lastUpdate);
		} else {
			// do not increment lastUpdate
			theEventQueue.virtualNet_v.Set(mySlaveNet, mySlaveNet, parameterResistanceMap[theConnections.device_p->parameters] + theEventQueue.virtualNet_v[myMasterNet].finalResistance, theEventQueue.virtualNet_v.lastUpdate);
		}
	}
	CheckResistorOverflow_(theEventQueue.virtualNet_v[mySlaveNet].finalResistance, mySlaveNet, logFile);
	if ( deviceType_v[theDeviceId] == RESISTOR ) {
		PropagateConnectionType(theEventQueue.virtualNet_v, connectionCount_v[mySlaveNet].sourceDrainType, myMasterNet);
	}
	deviceStatus_v[theDeviceId][theEventQueue.inactiveBit] = true;
}

void CCvcDb::PropagateConnectionType(CVirtualNetVector& theVirtualNet_v, CStatus theSourceDrainType, netId_t theNetId) {
	CStatus myNewStatus;
	// TODO: Propagate upstream??
	int myPropagationCount = 0;
	while ( theNetId != theVirtualNet_v[theNetId].nextNetId ) {
		assert(theNetId != UNKNOWN_NET );
		myNewStatus = connectionCount_v[theNetId].sourceDrainType | theSourceDrainType;
		if ( myNewStatus == connectionCount_v[theNetId].sourceDrainType ) break;
		connectionCount_v[theNetId].sourceDrainType = myNewStatus;
		theNetId = theVirtualNet_v[theNetId].nextNetId;
		if ( myPropagationCount++ > 10000 ) {
			reportFile << "ERROR: exceeded connection type propagation limit for net# " << theNetId << endl;
			reportFile << "	name: " << NetName(theNetId) << endl;
			reportFile << "	next net is " << theVirtualNet_v[theNetId].nextNetId << endl;
			assert(myPropagationCount < 10000 );
		}
	}
}

void CCvcDb::PropagateResistorVoltages(CEventQueue& theEventQueue) {
	deviceId_t myDeviceId = theEventQueue.GetEvent();
	deviceStatus_v[myDeviceId][theEventQueue.pendingBit] = false;
	queuePosition_t myQueuePosition;
	if ( deviceStatus_v[myDeviceId][theEventQueue.inactiveBit] ) return;
	static CConnection myConnections;
	MapDeviceNets(myDeviceId, theEventQueue, myConnections);
	if ( myConnections.IsUnknownSourceVoltage() ) {
		assert (myConnections.drainVoltage != UNKNOWN_VOLTAGE);
		ShortNets(theEventQueue, myDeviceId, myConnections, SOURCE_TO_MASTER_DRAIN);
		myQueuePosition = ( myConnections.drainPower_p->type[POWER_BIT] ) ? MAIN_BACK : DELAY_FRONT;
		EnqueueAttachedResistors(theEventQueue, myConnections.sourceId, myConnections.drainVoltage, myQueuePosition);
	} else if ( myConnections.IsUnknownDrainVoltage() ) { // sourceVoltage != UNKNOWN_VOLTAGE
		ShortNets(theEventQueue, myDeviceId, myConnections, DRAIN_TO_MASTER_SOURCE);
		myQueuePosition = ( myConnections.sourcePower_p->type[POWER_BIT] ) ? MAIN_BACK : DELAY_FRONT;
		EnqueueAttachedResistors(theEventQueue, myConnections.drainId, myConnections.sourceVoltage, myQueuePosition);
	} else {
		if (gDebug_cvc) cout << "already shorted PRV " << gEventQueueTypeMap[theEventQueue.queueType] << " " << myDeviceId << endl;
		AlreadyShorted(theEventQueue, myDeviceId, myConnections);
		if ( theEventQueue.virtualNet_v[myConnections.drainId].nextNetId != myConnections.drainId ) { // not terminal
			PropagateConnectionType(theEventQueue.virtualNet_v, connectionCount_v[myConnections.drainId].sourceDrainType, myConnections.sourceId);
		}
		if ( theEventQueue.virtualNet_v[myConnections.sourceId].nextNetId != myConnections.sourceId ) { // not terminal
			PropagateConnectionType(theEventQueue.virtualNet_v, connectionCount_v[myConnections.sourceId].sourceDrainType, myConnections.drainId);
		}
	}
}

void CCvcDb::PropagateMinMaxVoltages(CEventQueue& theEventQueue) {
	eventKey_t myQueueKey = theEventQueue.QueueTime();
	deviceId_t myDeviceId = theEventQueue.GetEvent();
	deviceStatus_v[myDeviceId][theEventQueue.pendingBit] = false;
	if ( deviceStatus_v[myDeviceId][theEventQueue.inactiveBit] ) return;
	eventKey_t myOriginalEventKey, myEventKey;
	queuePosition_t myQueuePosition;
	netId_t myDrainId, mySourceId;
	shortDirection_t myDirection;
	static CConnection myConnections;
	MapDeviceNets(myDeviceId, theEventQueue, myConnections);
	// BUG: 20140307 if both nets are known, need highest for max, lowest for min.
	if ( myConnections.sourceVoltage != UNKNOWN_VOLTAGE ) {
		if ( myConnections.drainVoltage != UNKNOWN_VOLTAGE ) {
			if ( theEventQueue.queueType == MIN_QUEUE ) {
				if ( myConnections.sourceVoltage <= myConnections.drainVoltage ) {
					myDirection = DRAIN_TO_MASTER_SOURCE;
				} else {
					myDirection = SOURCE_TO_MASTER_DRAIN;
				}
			} else { // MAX_QUEUE
				assert(theEventQueue.queueType == MAX_QUEUE);
				if ( myConnections.sourceVoltage >= myConnections.drainVoltage ) {
					myDirection = DRAIN_TO_MASTER_SOURCE;
				} else {
					myDirection = SOURCE_TO_MASTER_DRAIN;
				}
			}
		} else {
			myDirection = DRAIN_TO_MASTER_SOURCE;
		}
	} else if ( myConnections.drainVoltage != UNKNOWN_VOLTAGE ) {
		myDirection = SOURCE_TO_MASTER_DRAIN;
	} else return; // enqueued due to gate/bias changes. ignore for now.
	if ( myDirection == DRAIN_TO_MASTER_SOURCE ) {
		myOriginalEventKey = myConnections.sourceVoltage;
		myDrainId = myConnections.drainId;
		mySourceId = myConnections.sourceId;
		myQueuePosition = DefaultQueuePosition_(myConnections.sourcePower_p->type[POWER_BIT], theEventQueue);
/*
		if ( ! isFixedSimNet ) { // only on first pass
			if ( theEventQueue.queueType == MIN_QUEUE && deviceType_v[myConnections.deviceId] == LDDP && myConnections.sourceVoltage != myConnections.drainVoltage ) {
				ReportBadLddConnection(theEventQueue, myDeviceId);
			}
			if ( theEventQueue.queueType == MAX_QUEUE && deviceType_v[myConnections.deviceId] == LDDN && myConnections.sourceVoltage != myConnections.drainVoltage ) {
				ReportBadLddConnection(theEventQueue, myDeviceId);
			}
		}
*/
	} else { // myDirection = SOURCE_TO_MASTER_DRAIN;
		myOriginalEventKey = myConnections.drainVoltage;
		myDrainId = myConnections.sourceId;
		mySourceId = myConnections.drainId;
		myQueuePosition = DefaultQueuePosition_(myConnections.drainPower_p->type[POWER_BIT], theEventQueue);
/*
		if ( ! isFixedSimNet ) { // only on first pass
			if ( theEventQueue.queueType == MIN_QUEUE && deviceType_v[myConnections.deviceId] == LDDN && myConnections.sourceVoltage != myConnections.drainVoltage ) {
				ReportBadLddConnection(theEventQueue, myDeviceId);
			}
			if ( theEventQueue.queueType == MAX_QUEUE && deviceType_v[myConnections.deviceId] == LDDP && myConnections.sourceVoltage != myConnections.drainVoltage ) {
				ReportBadLddConnection(theEventQueue, myDeviceId);
			}
		}
*/
	}
//	if ( isFixedSimNet &&
//		( ( myConnections.sourcePower_p && myConnections.sourcePower_p->type[HIZ_BIT] ) ||
//				( myConnections.drainPower_p && myConnections.drainPower_p->type[HIZ_BIT] ) ) ) return; // don't propagate open power after initial min/max
	myEventKey = myOriginalEventKey;
	string myAdjustedCalculation = "";
	if ( IsIrrelevant(theEventQueue, myDeviceId, myConnections, myEventKey, DEQUEUE) ) { // both source/drain defined handled here
		deviceStatus_v[myDeviceId][theEventQueue.inactiveBit] = true;
	} else {
		if ( myDirection == DRAIN_TO_MASTER_SOURCE) assert (myConnections.drainVoltage == UNKNOWN_VOLTAGE);
/*
		if ( ! isFixedSimNet ) { // only on first pass
			if ( (deviceType_v[myConnections.deviceId] == LDDN && theEventQueue.queueType == MIN_QUEUE && myDirection == SOURCE_TO_MASTER_DRAIN) ||
					(deviceType_v[myConnections.deviceId] == LDDN && theEventQueue.queueType == MAX_QUEUE && myDirection == DRAIN_TO_MASTER_SOURCE) ||
					(deviceType_v[myConnections.deviceId] == LDDP && theEventQueue.queueType == MIN_QUEUE && myDirection == DRAIN_TO_MASTER_SOURCE) ||
					(deviceType_v[myConnections.deviceId] == LDDP && theEventQueue.queueType == MAX_QUEUE && myDirection == SOURCE_TO_MASTER_DRAIN) ) {
				ReportBadLddConnection(theEventQueue, myDeviceId);
			}
		}
*/

		myAdjustedCalculation = AdjustKey(theEventQueue, myDeviceId, myConnections, myEventKey, myQueuePosition, myDirection, PRINT_WARNINGS);
		if ( myQueuePosition == SKIP_QUEUE ) return;
		if (gDebug_cvc) cout << "DEBUG propagation:(" << gEventQueueTypeMap[theEventQueue.queueType] << ") device: " << myDeviceId << " QueueKey: " << myQueueKey << " EventKey: " << myEventKey << endl;
		if ( theEventQueue.Later(myEventKey, myQueueKey) ) {
			theEventQueue.AddEvent(myEventKey, myDeviceId, myQueuePosition);
			deviceStatus_v[myDeviceId][theEventQueue.pendingBit] = true;
			theEventQueue.requeueCount++;
			if (gDebug_cvc) cout << "requeueing" << endl;
		} else {
			ShortNets(theEventQueue, myDeviceId, myConnections, myDirection, myEventKey, myAdjustedCalculation);
			if ( theEventQueue.queueType == MIN_QUEUE && myOriginalEventKey < myEventKey && IsPmos_(deviceType_v[myConnections.deviceId]) ) {
				voltage_t myMaxVoltage = MaxVoltage(myDrainId);
				if ( IsDerivedFromFloating(maxNet_v, myDrainId) || myMaxVoltage == UNKNOWN_VOLTAGE || myMaxVoltage == myEventKey || netStatus_v[mySourceId][NEEDS_MAX_CHECK] ) {
					//assert(netVoltagePtr_v[myDrainId]);
					if (netVoltagePtr_v[myDrainId] ) {
						if ( netVoltagePtr_v[myDrainId]->pullUpVoltage() == UNKNOWN_VOLTAGE || netVoltagePtr_v[myDrainId]->pullUpVoltage() < myEventKey ) {
							if (gDebug_cvc) cout << "DEBUG: Adding max check at net " << myDrainId << endl;
							netStatus_v[myDrainId][NEEDS_MAX_CHECK] = true;
						}
					} else cout << "DEBUG: missing voltage for max check " << NetName(myDrainId) << endl;
				}
			} else if ( theEventQueue.queueType == MAX_QUEUE && myOriginalEventKey > myEventKey && IsNmos_(deviceType_v[myConnections.deviceId]) ) {
				voltage_t myMinVoltage = MinVoltage(myDrainId);
				if ( IsDerivedFromFloating(minNet_v, myDrainId) || myMinVoltage == UNKNOWN_VOLTAGE || myMinVoltage == myEventKey || netStatus_v[mySourceId][NEEDS_MIN_CHECK] ) {
					//assert(netVoltagePtr_v[myDrainId]);
					if ( netVoltagePtr_v[myDrainId] ) {
						if ( netVoltagePtr_v[myDrainId]->pullDownVoltage() == UNKNOWN_VOLTAGE || netVoltagePtr_v[myDrainId]->pullDownVoltage() > myEventKey ) {
							if (gDebug_cvc) cout << "DEBUG: Adding min check at net " << myDrainId << endl;
							netStatus_v[myDrainId][NEEDS_MIN_CHECK] = true;
						}
					} else cout << "DEBUG: missing voltage for min check " << NetName(myDrainId) << endl;
				}
			}
			EnqueueAttachedDevices(theEventQueue, myDrainId, myEventKey);
			if ( deviceType_v[myDeviceId] == RESISTOR && myOriginalEventKey == myEventKey ) {
				// resistors propagating unchanged voltages
				CDeviceCount myDeviceCount(myDrainId, this);
				if ( theEventQueue.queueType == MIN_QUEUE && myDeviceCount.resistorCount == 1 && myDeviceCount.pmosCount == 0 ) {
					// one resistor connected to no pmos
					ShortNets(minEventQueue, myDeviceId, myConnections, myDirection, myEventKey, "power through resistor");
					EnqueueAttachedDevices(minEventQueue, myDrainId, myEventKey);
				} else if (theEventQueue.queueType == MAX_QUEUE && myDeviceCount.resistorCount == 1 && myDeviceCount.nmosCount == 0 ) {
					// one resistor connected to no nmos
					ShortNets(maxEventQueue, myDeviceId, myConnections, myDirection, myEventKey, "power through resistor");
					EnqueueAttachedDevices(maxEventQueue, myDrainId, myEventKey);
				}
			}
		}
	}
}

void CCvcDb::PropagateSimVoltages(CEventQueue& theEventQueue, propagation_t thePropagationType) {
	eventKey_t myQueueKey = theEventQueue.QueueTime();
	deviceId_t myDeviceId = theEventQueue.GetEvent();
	deviceStatus_v[myDeviceId][theEventQueue.pendingBit] = false;
	if ( deviceStatus_v[myDeviceId][theEventQueue.inactiveBit] ) return;
	voltage_t mySimVoltage;
	netId_t myNextNetId;
	shortDirection_t myDirection;
	static CConnection myConnections;
	MapDeviceNets(myDeviceId, theEventQueue, myConnections);
	// 20140522A: when both source and drain are known, choose high for pmos, low for nmos
	if ( myConnections.IsUnknownSourceVoltage() ) {
		if ( myConnections.IsUnknownDrainVoltage() ) return; // enqueued to do gate/bias changes. ignore for now.
		myDirection = SOURCE_TO_MASTER_DRAIN;
		if ( myConnections.drainPower_p && myConnections.drainPower_p->type[HIZ_BIT] && ! IsSCRCPower(myConnections.drainPower_p) ) return;  // Don't propagate non-SCRC Hi-Z power
	} else if ( myConnections.IsUnknownDrainVoltage() ) {
		myDirection = DRAIN_TO_MASTER_SOURCE;
		if ( myConnections.sourcePower_p && myConnections.sourcePower_p->type[HIZ_BIT] && ! IsSCRCPower(myConnections.sourcePower_p) ) return;  // Don't propagate non-SCRC Hi-Z power
	} else if ( ( IsPmos_(deviceType_v[myDeviceId]) && myConnections.drainVoltage >= myConnections.sourceVoltage ) \
			|| ( IsNmos_(deviceType_v[myDeviceId]) && myConnections.drainVoltage <= myConnections.sourceVoltage ) ) { // both source drain known
		myDirection = SOURCE_TO_MASTER_DRAIN;
	} else {
		myDirection = DRAIN_TO_MASTER_SOURCE;
	}
	if ( myDirection == SOURCE_TO_MASTER_DRAIN ) {
		mySimVoltage = myConnections.drainVoltage;
		myNextNetId = myConnections.sourceId;
		// TODO: Possibly not necessary. Only primed voltages are processed.
		if ( thePropagationType == POWER_NETS_ONLY && ! IsPriorityPower_(myConnections.drainPower_p) ) return;
	} else {
		mySimVoltage = myConnections.sourceVoltage;
		myNextNetId = myConnections.drainId;
		// TODO: Possibly not necessary. Only primed voltages are processed.
		if ( thePropagationType == POWER_NETS_ONLY && ! IsPriorityPower_(myConnections.sourcePower_p) ) return;
	}
	if ( IsMos_(deviceType_v[myDeviceId]) ) {
		if ( myConnections.gateVoltage == UNKNOWN_VOLTAGE ) {
			// check always on devices
			voltage_t myMaxGateVoltage = MaxVoltage(myConnections.masterGateNet.finalNetId);
			voltage_t myMinGateVoltage = MinVoltage(myConnections.masterGateNet.finalNetId);
			if ( IsPmos_(deviceType_v[myDeviceId]) && myMaxGateVoltage != UNKNOWN_VOLTAGE
					&& myMaxGateVoltage < mySimVoltage + myConnections.device_p->model_p->Vth ) {
				if ( ! IsAlwaysOnCandidate(myDeviceId, myDirection) ) return;
				myConnections.gateVoltage = myMaxGateVoltage;
			} else if ( IsNmos_(deviceType_v[myDeviceId]) && myMinGateVoltage != UNKNOWN_VOLTAGE
					&& myMinGateVoltage > mySimVoltage + myConnections.device_p->model_p->Vth ) {
				if ( ! IsAlwaysOnCandidate(myDeviceId, myDirection) ) return;
				myConnections.gateVoltage = myMinGateVoltage;
			} else if ( thePropagationType == POWER_NETS_ONLY ||  // no mos-diode propagation for fixed-only
					(myConnections.gateId != myConnections.sourceId && myConnections.gateId != myConnections.drainId)) return; // skip unknown non-diode mos gates
		} else {
			if ( thePropagationType == POWER_NETS_ONLY && ! IsPriorityPower_(myConnections.gatePower_p) ) return;
		}
	}
	if (deviceType_v[myDeviceId] == FUSE_ON	&& thePropagationType != ALL_NETS_AND_FUSE)	return;
	if ( deviceType_v[myDeviceId] == FUSE_OFF /* && thePropagationType == ALL_NETS_AND_FUSE */ ) {
		deviceStatus_v[myDeviceId][theEventQueue.inactiveBit] = true;
	} else if ( IsIrrelevant(theEventQueue, myDeviceId, myConnections, mySimVoltage, DEQUEUE) ) {
		deviceStatus_v[myDeviceId][theEventQueue.inactiveBit] = true;
/*
	} else if ( IsMos_(deviceType_v[myDeviceId] == NMOS && myConnections.gateVoltage < myEventKey + myConnections.device_p->model_p->Vth) ) {
		return;
	} else if ( IsMos_(deviceType_v[myDeviceId] == PMOS && myConnections.gateVoltage > myEventKey + myConnections.device_p->model_p->Vth) ) {
		return;
*/
	} else {
		if ( myDirection == DRAIN_TO_MASTER_SOURCE ) {
			assert (myConnections.IsUnknownDrainVoltage());
			// unknown gates not enqueued
//			if ( IsMos_(deviceType_v[myDeviceId]) && myConnections.gateVoltage == UNKNOWN_VOLTAGE && myConnections.gateId != myConnections.drainId ) return;
		} else {
//			if ( IsMos_(deviceType_v[myDeviceId]) && myConnections.gateVoltage == UNKNOWN_VOLTAGE && myConnections.gateId != myConnections.sourceId ) return;
		}

		if (gDebug_cvc) cout << "DEBUG propagation:(" << gEventQueueTypeMap[theEventQueue.queueType] << ") device: " << myDeviceId << " QueueKey: " << myQueueKey << " EventKey: " << mySimVoltage << endl;
		string myCalculation;
		myCalculation = AdjustSimVoltage(theEventQueue, myDeviceId, myConnections, mySimVoltage, myDirection, thePropagationType);
		if ( thePropagationType == POWER_NETS_ONLY && ! IsEmpty(myCalculation) ) return;  // skip adjusted sim voltages with fixed-only
		if ( mySimVoltage == UNKNOWN_VOLTAGE ) return; // gate input = Vth (non mos diode)
		ShortSimNets(theEventQueue, myDeviceId, myConnections, myDirection, mySimVoltage, myCalculation);
		EnqueueAttachedDevices(theEventQueue, myNextNetId, mySimVoltage);
	}
}

void CCvcDb::CalculateResistorVoltage(netId_t theNetId, voltage_t theMinVoltage, resistance_t theMinResistance,
			voltage_t theMaxVoltage, resistance_t theMaxResistance ) {
//	bool myExplicitResistanceFlag = ( netVoltagePtr_v[theNetId] && netVoltagePtr_v[theNetId]->type[RESISTOR_BIT] );
//	bool myExplicitResistanceFlag = ( netVoltagePtr_v[theNetId] && netVoltagePtr_v[theNetId]->type[RESISTOR_BIT] );
//	bool myIgnoreResistorFlag = false;
	if ( theMinVoltage == UNKNOWN_VOLTAGE || theMaxVoltage == UNKNOWN_VOLTAGE ) return; // myIgnoreResistorFlag = true; // can't propagate unknown voltage
	if ( theMinResistance == 0 || theMaxResistance == 0 ) return; //myIgnoreResistorFlag = true; // don't recalculate 0 resistance
	netId_t myMinNet = minNet_v[theNetId].finalNetId;
	netId_t myMaxNet = maxNet_v[theNetId].finalNetId;
	if ( ! netVoltagePtr_v[myMinNet] || netVoltagePtr_v[myMinNet]->type[HIZ_BIT] ) return;  // don't calculate non-power or open
	if ( ! netVoltagePtr_v[myMaxNet] || netVoltagePtr_v[myMaxNet]->type[HIZ_BIT] ) return;  // don't calculate non-power or open
	if (  // myIgnoreResistorFlag == false &&
			minNet_v[theNetId].nextNetId != maxNet_v[theNetId].nextNetId && // different paths to min/max
			( minNet_v.IsTerminal(minNet_v[theNetId].nextNetId) || theNetId == maxNet_v[minNet_v[theNetId].nextNetId].nextNetId ) &&
			( maxNet_v.IsTerminal(maxNet_v[theNetId].nextNetId) || theNetId == minNet_v[maxNet_v[theNetId].nextNetId].nextNetId ) ) {
		if ( ! netVoltagePtr_v[theNetId] ) {
			cvcParameters.cvcPowerPtrList.push_back(new CPower(theNetId));
			netVoltagePtr_v[theNetId] = cvcParameters.cvcPowerPtrList.back();
			netVoltagePtr_v[theNetId]->extraData = new CExtraPowerData;
			netVoltagePtr_v[theNetId]->extraData->powerSignal = CPower::powerDefinitionText.SetTextAddress(RESISTOR_TEXT);
		}
		CPower * myPower_p = netVoltagePtr_v[theNetId];
		voltage_t myNewVoltage = theMinVoltage + long( theMaxVoltage - theMinVoltage ) * theMinResistance / ( theMinResistance + theMaxResistance );
		calculatedResistanceInfo_v[theNetId] = NetName(theNetId, PRINT_CIRCUIT_ON) + " " + to_string<voltage_t>(myNewVoltage);
		calculatedResistanceInfo_v[theNetId] += " MinV:" + to_string<voltage_t>(theMinVoltage) + " MaxV:" + to_string<voltage_t>(theMaxVoltage);
		calculatedResistanceInfo_v[theNetId] += " MinR:" + to_string<resistance_t>(theMinResistance) + " MaxR:" + to_string<resistance_t>(theMaxResistance);
		calculatedResistanceInfo_v[theNetId] += (netVoltagePtr_v[theNetId]->type[RESISTOR_BIT] ? " explicit" : " implicit");
//		logFile << "INFO: Resistor voltage calculation " << NetName(theNetId, PRINT_CIRCUIT_ON) << " " << myNewVoltage << " MinV:" << to_string<voltage_t>(theMinVoltage) << " MaxV:" << to_string<voltage_t>(theMaxVoltage);
//		logFile << " MinR:" << to_string<resistance_t>(theMinResistance) << " MaxR:" << to_string<resistance_t>(theMaxResistance);
//		logFile << (myExplicitResistanceFlag ? "explicit" : "implicit") << endl;
		string myCalculation = " R: " + PrintParameter(theMaxVoltage, 1000) + "V -> " + AddSiSuffix((float) theMaxResistance) + " ohm -> ";
		myCalculation += PrintParameter(myNewVoltage, 1000) + "V -> ";
		myCalculation += AddSiSuffix((float) theMinResistance) + " ohm -> " + PrintParameter(theMinVoltage, 1000) + "V";

		myPower_p->simVoltage = myNewVoltage;
		myPower_p->simCalculationType = RESISTOR_CALCULATION;
		myPower_p->maxVoltage = myNewVoltage;
		myPower_p->maxCalculationType = RESISTOR_CALCULATION;
		myPower_p->minVoltage = myNewVoltage;
		myPower_p->minCalculationType = RESISTOR_CALCULATION;
		myPower_p->defaultMinNet = minNet_v[theNetId].finalNetId;
		myPower_p->defaultMaxNet = maxNet_v[theNetId].finalNetId;
		myPower_p->netId = theNetId;
		myPower_p->type[MIN_CALCULATED_BIT] = true;
	//	myPower_p->type[SIM_CALCULATED_BIT] = true;
		myPower_p->type[MAX_CALCULATED_BIT] = true;
		string myDefinition = myPower_p->definition + (" calculation=> " + myCalculation);
		myPower_p->definition = CPower::powerDefinitionText.SetTextAddress((text_t)myDefinition.c_str());
//		cvcParameters.cvcPowerPtrList.push_back(new CPower(theNetId, NetName(theNetId), myNewVoltage, minNet_v[theNetId].finalNetId, maxNet_v[theNetId].finalNetId, myCalculation));
//		netVoltagePtr_v[theNetId] = cvcParameters.cvcPowerPtrList.back();
		myPower_p->type[RESISTOR_BIT] = true;
		minNet_v.lastUpdate += 1;
		simNet_v.lastUpdate += 1;
		maxNet_v.lastUpdate += 1;
//		minNet_v.Set(theNetId, theNetId, theMinResistance); // reset calculated voltage
//		maxNet_v.Set(theNetId, theNetId, theMaxResistance); // reset calculated voltage
//		RecalculateFinalResistance(minEventQueue, theNetId);
//		RecalculateFinalResistance(simEventQueue, theNetId);
//		RecalculateFinalResistance(maxEventQueue, theNetId);
//	} else if ( myExplicitResistanceFlag ) {
//		reportFile << "WARNING: Could not calculate explicit resistor voltage for " << NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
	}
}

void CCvcDb::PropagateResistorCalculations(netId_t theNetId, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v) {
	CFullConnection myConnections;
	for ( deviceId_t device_it = theFirstDevice_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theNextDevice_v[device_it] ) {
		if ( deviceType_v[device_it] == RESISTOR ) {
			MapDeviceNets(device_it, myConnections);
			if ( netVoltagePtr_v[myConnections.drainId] == NULL ) {
				cvcParameters.cvcPowerPtrList.push_back(new CPower(myConnections.drainId));
				netVoltagePtr_v[myConnections.drainId] = cvcParameters.cvcPowerPtrList.back();
				netVoltagePtr_v[myConnections.drainId]->type[RESISTOR_BIT] = true;
				netVoltagePtr_v[myConnections.drainId]->extraData = new CExtraPowerData;
				netVoltagePtr_v[myConnections.drainId]->extraData->powerSignal = CPower::powerDefinitionText.SetTextAddress(RESISTOR_TEXT);
			}
			if ( netVoltagePtr_v[myConnections.sourceId] == NULL ) {
				cvcParameters.cvcPowerPtrList.push_back(new CPower(myConnections.sourceId));
				netVoltagePtr_v[myConnections.sourceId] = cvcParameters.cvcPowerPtrList.back();
				netVoltagePtr_v[myConnections.sourceId]->type[RESISTOR_BIT] = true;
				netVoltagePtr_v[myConnections.sourceId]->extraData = new CExtraPowerData;
				netVoltagePtr_v[myConnections.sourceId]->extraData->powerSignal = CPower::powerDefinitionText.SetTextAddress(RESISTOR_TEXT);
			}
		}
	}
}

void CCvcDb::CalculateResistorVoltages() {
	CFullConnection myConnections;
	calculatedResistanceInfo_v.clear();
	for (auto power_ppit = cvcParameters.cvcPowerPtrList.begin(); power_ppit != cvcParameters.cvcPowerPtrList.end(); power_ppit++) {
		// since new entries are appended to the power list, this routine is effectively recursive
		if ( (*power_ppit)->type[RESISTOR_BIT] ) { // && (*power_ppit)->simVoltage == UNKNOWN_VOLTAGE ) {
			netId_t myPowerNet = (*power_ppit)->netId;
			if ( myPowerNet != UNKNOWN_NET && netVoltagePtr_v[myPowerNet] == *power_ppit ) { // process only used power nodes (ununsed power kept in powerPtrList)
				PropagateResistorCalculations(myPowerNet, firstSource_v, nextSource_v);
				PropagateResistorCalculations(myPowerNet, firstDrain_v, nextDrain_v);
			}
		}
	}
	for (deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		if ( deviceType_v[device_it] == RESISTOR && deviceStatus_v[device_it][SIM_INACTIVE] == false ) {
			MapDeviceNets(device_it, myConnections);
			if ( ( netVoltagePtr_v[myConnections.sourceId] == NULL && connectionCount_v[myConnections.sourceId].sourceDrainType == RESISTOR_ONLY ) ||
					( netVoltagePtr_v[myConnections.sourceId] && netVoltagePtr_v[myConnections.sourceId]->type[RESISTOR_BIT] && netVoltagePtr_v[myConnections.sourceId]->simVoltage == UNKNOWN_VOLTAGE ) ) {
				if ( myConnections.minSourceVoltage != myConnections.maxSourceVoltage ) {
					CalculateResistorVoltage(myConnections.sourceId, myConnections.minSourceVoltage, myConnections.masterMinSourceNet.finalResistance,
							myConnections.maxSourceVoltage, myConnections.masterMaxSourceNet.finalResistance);

				}
			}
			if ( ( netVoltagePtr_v[myConnections.drainId] == NULL && connectionCount_v[myConnections.drainId].sourceDrainType == RESISTOR_ONLY ) ||
					( netVoltagePtr_v[myConnections.drainId] && netVoltagePtr_v[myConnections.drainId]->type[RESISTOR_BIT] && netVoltagePtr_v[myConnections.drainId]->simVoltage == UNKNOWN_VOLTAGE  ) ) {
				if ( myConnections.minDrainVoltage != myConnections.maxDrainVoltage ) {
					CalculateResistorVoltage(myConnections.drainId, myConnections.minDrainVoltage, myConnections.masterMinDrainNet.finalResistance,
							myConnections.maxDrainVoltage, myConnections.masterMaxDrainNet.finalResistance);
				}
			}
			// set min/max for overvoltage checks
			// removed. both implicit/explicit resistors will use calculated voltages for overvoltage checks. MAY NOT DETECT TRUE ERRORS but prevents unexpected diode warnings
//			deviceStatus_v[device_it][MIN_INACTIVE] = false;
//			deviceStatus_v[device_it][MAX_INACTIVE] = false;
		}
	}
	netId_t myLastPowerNet;
	vector<bool> myPrintedPower(netCount, false);
	for (auto power_ppit = cvcParameters.cvcPowerPtrList.begin(); power_ppit != cvcParameters.cvcPowerPtrList.end(); power_ppit++) {
		if ( (*power_ppit)->type[RESISTOR_BIT] && IsCalculatedVoltage_((*power_ppit)) && ! myPrintedPower[(*power_ppit)->netId] ) {
			netId_t myPowerNet = (*power_ppit)->netId;
			if ( netVoltagePtr_v[myPowerNet]->simVoltage == UNKNOWN_VOLTAGE ) {
				logFile << "WARNING: Could not calculate explicit resistor voltage for " << NetName((*power_ppit)->netId, PRINT_CIRCUIT_ON) << endl;
			} else {
				if ( netVoltagePtr_v[myPowerNet] != *power_ppit ) continue; // skip unused power nodes
				netId_t myNextNet = minNet_v[myPowerNet].nextNetId;
				while ( netVoltagePtr_v[myNextNet] && netVoltagePtr_v[myNextNet]->simVoltage != UNKNOWN_VOLTAGE && ! minNet_v.IsTerminal(myNextNet) ) {
					myPowerNet = myNextNet;
					myNextNet = minNet_v[myNextNet].nextNetId;
				}
				do {
					logFile << "INFO: Resistor voltage calculation " << calculatedResistanceInfo_v[myPowerNet] << endl;
					myPrintedPower[(*power_ppit)->netId] = true;
					myLastPowerNet = myPowerNet;
					myPowerNet = maxNet_v[myPowerNet].nextNetId;
					minNet_v[myLastPowerNet].nextNetId = maxNet_v[myLastPowerNet].nextNetId = myLastPowerNet;
					minNet_v[myLastPowerNet].finalNetId = maxNet_v[myLastPowerNet].finalNetId = myLastPowerNet;
				} while ( netVoltagePtr_v[myPowerNet] && netVoltagePtr_v[myPowerNet]->simVoltage != UNKNOWN_VOLTAGE && ! maxNet_v.IsTerminal(myPowerNet));
				logFile << endl;
			}
		}
	}
}

void CCvcDb::SetResistorVoltagesByPower() {
	reportFile << "CVC: Calculating resistor voltages..." << endl;
	queuePosition_t myQueuePosition;
	for (CPowerPtrList::iterator power_ppit = cvcParameters.cvcPowerPtrList.begin(); power_ppit != cvcParameters.cvcPowerPtrList.end(); power_ppit++) {
		// should only be defined voltages at this time.
		CPower * myPower_p = * power_ppit;
//		if ( ! myPower_p->type[EXPECTED_ONLY_BIT] ) {
		netId_t myNetId = myPower_p->netId;
		if ( myPower_p->minVoltage != UNKNOWN_VOLTAGE ) {
			myQueuePosition = DefaultQueuePosition_(myPower_p->type[POWER_BIT], minEventQueue);
			EnqueueAttachedResistors(minEventQueue, myNetId, myPower_p->minVoltage, myQueuePosition);
		}
		if ( myPower_p->maxVoltage != UNKNOWN_VOLTAGE ) {
			myQueuePosition = DefaultQueuePosition_(myPower_p->type[POWER_BIT], maxEventQueue);
			EnqueueAttachedResistors(maxEventQueue, myNetId, myPower_p->maxVoltage, myQueuePosition);
		}
//			if ( myPower_p->simulationVoltage != UNKNOWN_VOLTAGE ) {
//				EnqueueAttachedResistors(simulationEventQueue, myNetId, myPower_p->simulationVoltage);
//			}
//		}
	}
//	minimumEventQueue.Print();
//	maximumEventQueue.Print();
	minEventQueue.queueStart = true;
	maxEventQueue.queueStart = true;
	while (minEventQueue.QueueSize() + maxEventQueue.QueueSize() > 0) {
		if (minEventQueue.QueueSize() > maxEventQueue.QueueSize()) {
//			cout << "Min:" << minimumEventQueue.GetEvent() << endl;
			PropagateResistorVoltages(minEventQueue);
		} else {
//			cout << "Max:" << maximumEventQueue.GetEvent() << endl;
			PropagateResistorVoltages(maxEventQueue);
		}
	}

//	minimumEventQueue.Print();
//	maximumEventQueue.Print();
	CalculateResistorVoltages();
	cvcCircuitList.PrintAndResetCircuitErrors(cvcParameters.cvcCircuitErrorLimit, errorFile, "! Resistor Errors");
	if ( gDebug_cvc ) {
		PrintVirtualNets(minNet_v, "(minimum r)");
		PrintVirtualNets(maxNet_v, "(maximum r)");
		PrintVirtualNets(simNet_v, "(simulation r)");
	}
}

bool CCvcDb::CheckEstimateDependency(CDependencyMap& theDependencyMap, size_t theEstimateType, list<netId_t>& theDependencyList) {
	assert(! theDependencyList.empty());
	for (auto net_pit = theDependencyList.begin(); net_pit != theDependencyList.end(); net_pit++) {
		if ( netStatus_v[*net_pit][theEstimateType] == false ) return false;
		if ( theDependencyMap.count(*net_pit) == 0 ) continue;
		if ( CheckEstimateDependency(theDependencyMap, theEstimateType, theDependencyMap[*net_pit]) == false ) return false;
	}
	return(true);
}

void CCvcDb::CheckEstimateDependencies() {
	for(auto netListPair_pit = minConnectionDependencyMap.begin(); netListPair_pit != minConnectionDependencyMap.end(); netListPair_pit++) {
		netId_t myNetId = netListPair_pit->first;
		if ( gDebug_cvc ) cout << "DEBUG: Min dependency queue " << myNetId << "(" << netListPair_pit->second.front() << "-" << netListPair_pit->second.size() << ")" << endl;
		if ( netStatus_v[myNetId][NEEDS_MIN_CONNECTION] == false ) continue;
		netStatus_v[myNetId][NEEDS_MIN_CONNECTION] = CheckEstimateDependency(minConnectionDependencyMap, NEEDS_MIN_CONNECTION, netListPair_pit->second);
	}
	for(auto netListPair_pit = maxConnectionDependencyMap.begin(); netListPair_pit != maxConnectionDependencyMap.end(); netListPair_pit++) {
		netId_t myNetId = netListPair_pit->first;
		if ( gDebug_cvc ) cout << "DEBUG: Max dependency queue " << myNetId << "(" << netListPair_pit->second.front() << "-" << netListPair_pit->second.size() << ")" << endl;
		if ( netStatus_v[myNetId][NEEDS_MAX_CONNECTION] == false ) continue;
		netStatus_v[myNetId][NEEDS_MAX_CONNECTION] = CheckEstimateDependency(maxConnectionDependencyMap, NEEDS_MAX_CONNECTION, netListPair_pit->second);
	}
}
/*
void CCvcDb::SetNetType() {
	cout << "Setting net type";
	int myAnalogNetCount = 0, myDigitalNetCount = 0;
	for ( netId_t net_it = 0; net_it < netCount; net_it++ ) {

	}
	cout << myAnalogNetCount << " analog nets, " << myDigitalNetCount << " digital nets" << endl;
}
*/
void CCvcDb::SetTrivialMinMaxPower() {
	reportFile << "Processing trivial nets";
	cout.flush();
	deviceId_t myNmos, myPmos;
	CConnection myConnections;
	netId_t myGroundNet, myPowerNet, myGateNet;
	voltage_t myMinPower, myMaxPower;
	resistance_t myNmosResistance, myPmosResistance;
	int	myTrivialCount = 0;
	int myPrintCount = 1000000;
	for ( netId_t net_it = 0; net_it < netCount; net_it++ ) {
		if ( myPrintCount-- <= 0 ) {
			cout << "."; cout.flush();
			myPrintCount = 1000000;
		}
		if ( minNet_v[net_it].nextNetId != net_it || maxNet_v[net_it].nextNetId != net_it ) continue; // already assigned
		if ( netVoltagePtr_v[net_it] && netVoltagePtr_v[net_it]->type[POWER_BIT] ) continue;  // skip power definitions
		if ( connectionCount_v[net_it].sourceCount + connectionCount_v[net_it].drainCount == 2 ) { // only hits master equivalent nets
			if ( connectionCount_v[net_it].sourceDrainType[NMOS] && connectionCount_v[net_it].sourceDrainType[PMOS] ) {
				myNmos = UNKNOWN_DEVICE;
				myPmos = UNKNOWN_DEVICE;
				for ( deviceId_t device_it = firstDrain_v[net_it]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
					if ( sourceNet_v[device_it] == gateNet_v[device_it] ) continue; // skip mos diodes
					if ( IsNmos_(deviceType_v[device_it]) ) myNmos = device_it;
					if ( IsPmos_(deviceType_v[device_it]) ) myPmos = device_it;
				}
				for ( deviceId_t device_it = firstSource_v[net_it]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
					if ( drainNet_v[device_it] == gateNet_v[device_it] ) continue; // skip mos diodes
					if ( IsNmos_(deviceType_v[device_it]) ) myNmos = device_it;
					if ( IsPmos_(deviceType_v[device_it]) ) myPmos = device_it;
				}
				if ( myNmos == UNKNOWN_DEVICE ) continue;
				if ( myPmos == UNKNOWN_DEVICE ) continue;
				if ( deviceStatus_v[myNmos][MIN_INACTIVE] ) continue;
				if ( deviceStatus_v[myPmos][MAX_INACTIVE] ) continue;
				MapDeviceNets(myNmos, minEventQueue, myConnections);
				myGateNet = myConnections.gateId;
				if ( myConnections.sourceId == net_it ) {
					myGroundNet = myConnections.drainId;
				} else if ( myConnections.drainId == net_it ) {
					myGroundNet = myConnections.sourceId;
				} else {
					continue; // net already assigned
				}
				if ( myConnections.gateId == net_it ) continue; // mos diodes are not trivial
				myNmosResistance = myConnections.resistance;
				MapDeviceNets(myPmos, maxEventQueue, myConnections);
				if ( myConnections.sourceId == net_it ) {
					myPowerNet = myConnections.drainId;
				} else if ( myConnections.drainId == net_it ) {
					myPowerNet = myConnections.sourceId;
				} else {
					continue; // net already assigned
				}
				if ( myConnections.gateId == net_it ) continue; // mos diodes are not trivial
				myPmosResistance = myConnections.resistance;
				if ( myGateNet != myConnections.gateId ) continue; // ignore non-inverters
//				if ( netVoltagePtr_v[myGateNet] && netVoltagePtr_v[myGateNet]->simVoltage != UNKNOWN_VOLTAGE ) continue; // ignore fixed input
//				if ( simNet_v[myGateNet].nextNetId != myGateNet) continue; // ignore logic input
				if ( netVoltagePtr_v[myGroundNet] && netVoltagePtr_v[myPowerNet] && netVoltagePtr_v[myGroundNet]->type[POWER_BIT] && netVoltagePtr_v[myPowerNet]->type[POWER_BIT] ) {
					if ( netVoltagePtr_v[myGroundNet]->minVoltage != netVoltagePtr_v[myGroundNet]->maxVoltage ) continue; // ignore variable power
					if ( netVoltagePtr_v[myPowerNet]->minVoltage != netVoltagePtr_v[myPowerNet]->maxVoltage ) continue; // ignore variable power
					// if ( netVoltagePtr_v[myGroundNet]->type[HIZ_BIT] || netVoltagePtr_v[myPowerNet]->type[HIZ_BIT] ) continue; // ignore open power
					myMinPower = netVoltagePtr_v[myGroundNet]->minVoltage;
					myMaxPower = netVoltagePtr_v[myPowerNet]->maxVoltage;
					if ( myMaxPower != UNKNOWN_VOLTAGE && myMinPower < myMaxPower ) { // implies myMinPower != UNKNOWN_VOLTAGE
						minNet_v.Set(net_it, myGroundNet, myNmosResistance, 0);
						CheckResistorOverflow_(minNet_v[net_it].resistance, net_it, logFile);
						maxNet_v.Set(net_it, myPowerNet, myPmosResistance, 0);
						CheckResistorOverflow_(maxNet_v[net_it].resistance, net_it, logFile);
//						minNet_v[net_it].nextNetId = myGroundNet;
//						minNet_v[net_it].resistance = myNmosResistance;
//						maxNet_v[net_it].nextNetId = myPowerNet;
//						maxNet_v[net_it].resistance = myPmosResistance;
//						minNet_v.SetFinalNet(net_it);
//						maxNet_v.SetFinalNet(net_it);
						deviceStatus_v[myNmos][MIN_INACTIVE] = true;
						deviceStatus_v[myNmos][MAX_INACTIVE] = true;
						deviceStatus_v[myPmos][MIN_INACTIVE] = true;
						deviceStatus_v[myPmos][MAX_INACTIVE] = true;
						myTrivialCount++;
						if ( inverterNet_v[net_it] == UNKNOWN_NET ) {
							inverterNet_v[net_it] = myGateNet;
						} else if ( inverterNet_v[net_it] != myGateNet ) {
							reportFile << "WARNING: could not set " << myGateNet << " ->inv-> " << net_it;
							reportFile << " because " << inverterNet_v[net_it] << " ->inv-> " << net_it << " already set" << endl;
						}
					}
				}
			}
		}
	}
	reportFile << " found " << myTrivialCount << " trivial nets" << endl;
}
/*
void CCvcDb::SetSemiTrivialMinMaxPower() {
	cout << "Processing semi-trivial nets";
	cout.flush();
	CStatus myNmosOnlyStatus, myPmosOnlyStatus;
	myNmosOnlyStatus[NMOS] = true;
	myPmosOnlyStatus[PMOS] = true;
	pair<deviceId_t, netId_t> myNmosList[10], myPmosList[10];
	int myNmosCount = 0, myPmosCount = 0;
	CConnection myConnections;
	netId_t mySourceNet, myGroundNet, myPowerNet, myGateNet;
	voltage_t myMinPower, myMaxPower;
	resistance_t myNmosResistance, myPmosResistance;
	int	myTrivialCount = 0;
	int myPrintCount = 1000000;
	for ( netId_t net_it = 0; net_it < netCount; net_it++ ) {
		if ( myPrintCount-- <= 0 ) {
			cout << "."; cout.flush();
			myPrintCount = 1000000;
		}
		if ( minNet_v[net_it].nextNetId != net_it || maxNet_v[net_it].nextNetId != net_it ) continue; // already assigned
		int myConnectionCount = connectionCount_v[net_it].sourceCount + connectionCount_v[net_it].drainCount;
		if ( myConnectionCount > 0 && myConnectionCount < 10 ) { // only hits master equivalent nets
			if ( connectionCount_v[net_it].sourceDrainType[NMOS] && connectionCount_v[net_it].sourceDrainType[PMOS] ) {
				ResetPendingDevices(myNmosList, myNmosCount);
				ResetPendingDevices(myPmosList, myPmosCount);
				myGroundNet = UNKNOWN_NET;
				myPowerNet = UNKNOWN_NET;
				myNmosCount = 0;
				myPmosCount = 0;
				for ( deviceId_t device_it = firstDrain_v[net_it]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
					if ( deviceType_v[device_it] == NMOS && ! deviceStatus_v[device_it][MIN_INACTIVE] ) {
						myNmosList[myNmosCount++] = pair<deviceId_t, netId_t> (device_it, net_it);
						deviceStatus_v[device_it][MIN_PENDING] = true;
					}
					if ( deviceType_v[device_it] == PMOS && ! deviceStatus_v[device_it][MAX_INACTIVE] ) {
						myPmosList[myPmosCount++] = pair<deviceId_t, netId_t> (device_it, net_it);
						deviceStatus_v[device_it][MAX_PENDING] = true;
					}
				}
				for ( deviceId_t device_it = firstSource_v[net_it]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
					if ( deviceType_v[device_it] == NMOS && ! deviceStatus_v[device_it][MIN_INACTIVE] ) {
						myNmosList[myNmosCount++] = pair<deviceId_t, netId_t> (device_it, net_it);
						deviceStatus_v[device_it][MIN_PENDING] = true;
					}
					if ( deviceType_v[device_it] == PMOS && ! deviceStatus_v[device_it][MAX_INACTIVE] ) {
						myPmosList[myPmosCount++] = pair<deviceId_t, netId_t> (device_it, net_it);
						deviceStatus_v[device_it][MAX_PENDING] = true;
					}
				}
				if ( myNmosCount == 0 || myPmosCount == 0 ) continue; // not trivial net
				int list_it;
				net_it myLastDrain = UNKNOWN_NET;
				resistance_t myMinResistance, myNmosResistance = 0;
				for ( list_it = 0; list_it < myNmosCount && list_it < 10; list_it++ ) {

					MapDeviceNets(myNmosList[list_it].first, minEventQueue, myConnections);
	//				myGateNet = myConnections.gateId;
					if ( myConnections.gateId == myNmosList[list_it].second || myConnections.gateId == net_it ) break; // mos diodes are not trivial
					if ( myConnections.sourceId == myNmosList[list_it].second ) {
						mySourceNet = myConnections.drainId;
					} else {
						assert (myConnections.drainId == myNmosList[list_it].second);
						mySourceNet = myConnections.sourceId;
					}
					if ( myLastDrain != myNmosList[list_it].second ) {
						if ( myLastDrain != UNKNOWN_NET ) {
							myNmosResistance += myMinResistance;
						}
						myLastDrain = myNmosList[list_it].second;
						myMinResistance = INFINITE_RESISTANCE;
					}
					if ( myMinResistance > myConnections.resistance ) {
						myMinResistance = myConnections.resistance;
					}
					if ( netVoltagePtr_v[mySourceNet] && netVoltagePtr_v[mySourceNet]->minVoltage != UNKNOWN_VOLTAGE ) {
						if ( myGroundNet == UNKNOWN_NET ) myGroundNet = mySourceNet;
						if ( netVoltagePtr_v[myGroundNet]->minVoltage != netVoltagePtr_v[mySourceNet]->minVoltage ) break; // power mismatch
						continue;
					}
					if ( connectionCount_v[mySourceNet].sourceDrainType != myNmosOnlyStatus ) break; // only nmos
					if ( connectionCount_v[mySourceNet].sourceCount + connectionCount_v[mySourceNet].drainCount + myNmosCount > 10 ) break; // too many devices
					for ( deviceId_t device_it = firstDrain_v[mySourceNet]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
						if ( ! deviceStatus_v[device_it][MIN_PENDING] && ! deviceStatus_v[device_it][MIN_INACTIVE] ) {
							myNmosList[myNmosCount++] = pair<deviceId_t, netId_t> (device_it, mySourceNet);
							deviceStatus_v[device_it][MIN_PENDING] = true;
						}
					}
					for ( deviceId_t device_it = firstSource_v[mySourceNet]; next_device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
						if ( ! deviceStatus_v[device_it][MIN_PENDING] && ! deviceStatus_v[device_it][MIN_INACTIVE] ) {
							myNmosList[myNmosCount++] = pair<deviceId_t, netId_t> (device_it, mySourceNet);
							deviceStatus_v[device_it][MIN_PENDING] = true;
						}
					}
				}
				if ( list_it < myNmosCount || list_it >= 10 ) continue;
				if ( myMinResistance < INFINITE_RESISTANCE ) myNmosResistance += myMinResistance;
				MapDeviceNets(myPmosList, maxEventQueue, myConnections);
				if ( myConnections.sourceId == net_it ) {
					myPowerNet = myConnections.drainId;
				} else if ( myConnections.drainId == net_it ) {
					myPowerNet = myConnections.sourceId;
				} else {
					continue; // net already assigned
				}
				if ( myConnections.gateId == net_it ) continue; // mos diodes are not trivial
				myPmosResistance = myConnections.resistance;
//				if ( myGateNet != myConnections.gateId ) continue; // ignore non-inverters
//				if ( netVoltagePtr_v[myGateNet] && netVoltagePtr_v[myGateNet]->simVoltage != UNKNOWN_VOLTAGE ) continue; // ignore fixed input
//				if ( simNet_v[myGateNet].nextNetId != myGateNet) continue; // ignore logic input
				if ( netVoltagePtr_v[myGroundNet] && netVoltagePtr_v[myPowerNet] ) {
					myMinPower = netVoltagePtr_v[myGroundNet]->minVoltage;
					myMaxPower = netVoltagePtr_v[myPowerNet]->maxVoltage;
					if ( myMaxPower != UNKNOWN_VOLTAGE && myMinPower < myMaxPower ) { // implies myMinPower != UNKNOWN_VOLTAGE
						minNet_v[net_it].nextNetId = myGroundNet;
						minNet_v[net_it].resistance = myNmosResistance;
						maxNet_v[net_it].nextNetId = myPowerNet;
						maxNet_v[net_it].resistance = myPmosResistance;
						deviceStatus_v[myNmosList][minEventQueue.inactiveBit] = true;
						deviceStatus_v[myNmosList][maxEventQueue.inactiveBit] = true;
						deviceStatus_v[myPmosList][minEventQueue.inactiveBit] = true;
						deviceStatus_v[myPmosList][maxEventQueue.inactiveBit] = true;
						myTrivialCount++;
					}
				}
			}
		}
	}
	ResetPendingDevices(myNmosList, myNmosCount);
	ResetPendingDevices(myPmosList, myPmosCount);
	cout << " found " << myTrivialCount << " trivial nets" << endl;
}
*/

void CCvcDb::ResetMinMaxActiveStatus() {
	// possibly change to netVoltagePtr_v
	for (auto power_ppit = cvcParameters.cvcPowerPtrList.begin(); power_ppit != cvcParameters.cvcPowerPtrList.end(); power_ppit++) {
		(*power_ppit)->active[MIN_ACTIVE] = (*power_ppit)->active[MAX_ACTIVE] = false;
		if ( (*power_ppit)->type[INPUT_BIT] // always activate min/max for inputs
				|| (*power_ppit)->type[POWER_BIT] // always activate min/max for power
 //				|| (! leakVoltageSet && (*power_ppit)->type[HIZ_BIT]) // only activate min/max for HIZ on first pass
//				|| (*power_ppit)->type[HIZ_BIT]
				|| ! ((*power_ppit)->simVoltage == UNKNOWN_VOLTAGE || (*power_ppit)->type[SIM_CALCULATED_BIT]) ) { // activate min/max if sim set but not calculated
			if ( (*power_ppit)->minVoltage != UNKNOWN_VOLTAGE && ! (*power_ppit)->active[MIN_IGNORE] ) {
				(*power_ppit)->active[MIN_ACTIVE] = true;
			}
			if ( (*power_ppit)->maxVoltage != UNKNOWN_VOLTAGE && ! (*power_ppit)->active[MAX_IGNORE] ) {
				(*power_ppit)->active[MAX_ACTIVE] = true;
			}
		}
	}
}

void CCvcDb::SetInitialMinMaxPower() {
	reportFile << "CVC: Calculating min/max voltages..." << endl;
	isFixedMinNet = isFixedMaxNet = false;
	minConnectionDependencyMap.clear();
	maxConnectionDependencyMap.clear();
	ResetMinMaxActiveStatus();
	SetTrivialMinMaxPower();
//	for (CPowerPtrList::iterator power_ppit = cvcParameters.cvcPowerPtrList.begin(); power_ppit != cvcParameters.cvcPowerPtrList.end(); power_ppit++) {
	CVirtualNet myMinMasterNet, myMaxMasterNet;
//	int myPrintCounter = 0;
	for ( netId_t net_it = 0; net_it < netCount; net_it++ ) {
/*
		if ( ++myPrintCounter >= 2  1000000  ) {
			cout << "\r" << "	Net " << net_it << std::flush;
			myPrintCounter = 0;
			usleep(1e6);
		}
*/
		if ( net_it != GetEquivalentNet(net_it) ) continue; // skip subordinate nets
		myMinMasterNet(minNet_v, net_it);
//		myMinMasterNet = CVirtualNet(minNet_v, net_it);
//		mySimMasterNet = CVirtualNet(simNet_v, GetEquivalentNet(net_it));
		myMaxMasterNet(maxNet_v, net_it);
//		myMaxMasterNet = CVirtualNet(maxNet_v, net_it);
		CPower * myPower_p = netVoltagePtr_v[myMinMasterNet.finalNetId];
//		if ( myPower_p && ! myPower_p->type[EXPECTED_ONLY_BIT] ) {
//		if ( myPower_p && myPower_p->active[MIN_ACTIVE] && ! ( isFixedSimNet && myPower_p->type[HIZ_BIT]) ) {
		if ( myPower_p && myPower_p->active[MIN_ACTIVE] ) {
//			netId_t myNetId = myPower_p->baseNetId;
			voltage_t myMinVoltage = ( isFixedSimNet && myPower_p->simVoltage != UNKNOWN_VOLTAGE ) ? myPower_p->simVoltage : myPower_p->minVoltage;
			if ( myMinVoltage != UNKNOWN_VOLTAGE ) {
				EnqueueAttachedDevicesByTerminal(minEventQueue, net_it, firstSource_v, nextSource_v, myMinVoltage);
				EnqueueAttachedDevicesByTerminal(minEventQueue, net_it, firstDrain_v, nextDrain_v, myMinVoltage);
			}
		}
		myPower_p = netVoltagePtr_v[myMaxMasterNet.finalNetId];
//		if ( myPower_p && ! myPower_p->type[EXPECTED_ONLY_BIT] ) {
//		if ( myPower_p && myPower_p->active[MAX_ACTIVE] && ! ( isFixedSimNet && myPower_p->type[HIZ_BIT]) ) {
		if ( myPower_p && myPower_p->active[MAX_ACTIVE] ) {
			voltage_t myMaxVoltage = ( isFixedSimNet && myPower_p->simVoltage != UNKNOWN_VOLTAGE ) ? myPower_p->simVoltage : myPower_p->maxVoltage;
			if ( myMaxVoltage != UNKNOWN_VOLTAGE ) {
				EnqueueAttachedDevicesByTerminal(maxEventQueue, net_it, firstSource_v, nextSource_v, myMaxVoltage);
				EnqueueAttachedDevicesByTerminal(maxEventQueue, net_it, firstDrain_v, nextDrain_v, myMaxVoltage);
			}
		}
	}
//	myPrintCounter = 0;
/*
	if ( minEventQueue.savedDelayQueue.size() == 0 || minEventQueue.savedMainQueue.size() == 0 ) {
		minEventQueue.BackupQueue();
	}
	if ( maxEventQueue.savedDelayQueue.size() == 0 || maxEventQueue.savedMainQueue.size() == 0 ) {
		maxEventQueue.BackupQueue();
	}
*/
	minEventQueue.queueStart = true;
	maxEventQueue.queueStart = true;
	while (minEventQueue.QueueSize() + maxEventQueue.QueueSize() > 0) {
/*
		if ( --myPrintCounter <= 0 ) {
			cout << "	Event Queue Size(min/max): " << minEventQueue.mainQueue.size() << "+" << minEventQueue.delayQueue.size();
			cout << "/" << maxEventQueue.mainQueue.size() << "+" << maxEventQueue.delayQueue.size() << endl;
//			myPrintCounter = 1000;
		}
*/
		if (gDebug_cvc) cout << "Event Queue Size(min/max): " << minEventQueue.mainQueue.size() << "+" << minEventQueue.delayQueue.size();
		if (gDebug_cvc) cout << "/" << maxEventQueue.mainQueue.size() << "+" << maxEventQueue.delayQueue.size() << endl;
//		if (debug_cvc) minEventQueue.Print("Min queue");
//		if (debug_cvc) maxEventQueue.Print("Max queue");

		// main queues first, then by size
		if ((minEventQueue.IsNextMainQueue() == maxEventQueue.IsNextMainQueue() && minEventQueue.QueueSize() > maxEventQueue.QueueSize())
				|| (minEventQueue.IsNextMainQueue() && ! maxEventQueue.IsNextMainQueue()) ) {
//			cout << "Min:" << minimumEventQueue.GetEvent() << endl;
			PropagateMinMaxVoltages(minEventQueue);
		} else {
//			cout << "Max:" << maximumEventQueue.GetEvent() << endl;
			PropagateMinMaxVoltages(maxEventQueue);
		}
	}
	// Reset min/max voltage conflict errors
	cvcCircuitList.PrintAndResetCircuitErrors(cvcParameters.cvcCircuitErrorLimit, errorFile, "! Power/Ground path through fuse", FUSE_MODELS);
	cvcCircuitList.PrintAndResetCircuitErrors(cvcParameters.cvcCircuitErrorLimit, errorFile, "! ... voltage already set");
//	errorFile << "! Finished" << endl << endl;
//	if ( ! isFixedSimNet ) cvcCircuitList.PrintAndResetCircuitErrors(cvcParameters.cvcCircuitErrorLimit, errorFile); // Bad LDD connections (first pass)
	CheckEstimateDependencies();
	reportFile << "CVC: Ignoring invalid calculations..." << endl;
	size_t myRemovedCount = 0;
	for ( netId_t net_it = 0; net_it < netCount; net_it++ ) {
		if ( netVoltagePtr_v[net_it] == NULL ) continue; // skips subordinate nets
		RemoveInvalidPower(net_it, myRemovedCount);
	}
	reportFile << "CVC:   Removed " << myRemovedCount << " calculations" << endl;
	int	myProgressCount = 0;
	reportFile << "Copying master nets"; cout.flush();
//	netId_t myEquivalentNet;
	for ( netId_t net_it = 0; net_it < netCount; net_it++ ) {
		if ( ++myProgressCount == 1000000 ) {
			cout << "."; cout.flush();
			myProgressCount = 0;
		}
//		if ( minNet_v[net_it].nextNetId != net_it && minNet_v[net_it].resistance == 0 ) {
//			minNet_v.Set(net_it, minNet_v[net_it].nextNetId, 0); // reset final resistance
//		}
//		if ( maxNet_v[net_it].nextNetId != net_it && maxNet_v[net_it].resistance == 0 ) {
//			maxNet_v.Set(net_it, maxNet_v[net_it].nextNetId, 0); // reset final resistance
//		}
//		myEquivalentNet = GetEquivalentNet(net_it);
//		if ( net_it != GetEquivalentNet(net_it) ) continue; // only fix equivalent master nets.
		// equivalent nets should never be accessed in min/max queues. values don't matter.
		minNet_v[net_it](minNet_v, net_it); // recalculate final values
		maxNet_v[net_it](maxNet_v, net_it); // recalculate final values
//		assert(fixedMinNet_v[net_it].nextNetId == minNet_v[net_it].finalNetId && fixedMinNet_v[net_it].resistance == minNet_v[net_it].finalResistance);
//		assert(fixedMaxNet_v[net_it].nextNetId == maxNet_v[net_it].finalNetId && fixedMaxNet_v[net_it].resistance == maxNet_v[net_it].finalResistance);
//		fixedMinNet_v[net_it].GetMasterNet(minNet_v, net_it);
//		fixedMaxNet_v[net_it].GetMasterNet(maxNet_v, net_it);
	}
	reportFile << endl;
	isFixedMinNet = isFixedMaxNet = true;
	reportFile << "CVC: Ignoring non-conducting devices..." << endl;
	// ignore devices with no leak paths
	// really slow. has to calculate #device*#terminal instead of #net
	size_t myIgnoreCount = 0;
	CFullConnection myConnections;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		if ( deviceStatus_v[device_it][SIM_INACTIVE] ) continue;
		MapDeviceSourceDrainNets(device_it, myConnections);
		bool myIsHiZDrain = ( myConnections.minDrainPower_p && myConnections.minDrainPower_p->type[HIZ_BIT] ) ||
				( myConnections.maxDrainPower_p && myConnections.maxDrainPower_p->type[HIZ_BIT] );
		bool myIsHiZSource = ( myConnections.minSourcePower_p && myConnections.minSourcePower_p->type[HIZ_BIT] ) ||
				( myConnections.maxSourcePower_p && myConnections.maxSourcePower_p->type[HIZ_BIT] );
		if ( (! myIsHiZSource && myConnections.minSourceVoltage == UNKNOWN_VOLTAGE && myConnections.maxSourceVoltage == UNKNOWN_VOLTAGE) ||
				(! myIsHiZDrain && myConnections.minDrainVoltage == UNKNOWN_VOLTAGE && myConnections.maxDrainVoltage == UNKNOWN_VOLTAGE) ) {
			if (gDebug_cvc) cout << "Ignoring device " << device_it << " Connected to unknown min/max voltage." << endl;
			IgnoreDevice(device_it);
			myIgnoreCount++;
		}
	}
	reportFile << "CVC:   Ignored " << myIgnoreCount << " devices" << endl;
}

void CCvcDb::IgnoreUnusedDevices() {
	CFullConnection myConnections;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		if ( deviceStatus_v[device_it][SIM_INACTIVE] ) continue;
		MapDeviceNets(device_it, myConnections);
		if ( myConnections.minSourceVoltage == UNKNOWN_VOLTAGE || myConnections.minDrainVoltage == UNKNOWN_VOLTAGE ||
				myConnections.maxSourceVoltage == UNKNOWN_VOLTAGE || myConnections.maxDrainVoltage == UNKNOWN_VOLTAGE ) {
			if (gDebug_cvc) cout << "Ignoring device " << device_it << " Connected to unknown min/max voltage." << endl;
			IgnoreDevice(device_it);
		}
	}
}

void CCvcDb::ResetMinMaxPower() {
	minEventQueue.ResetQueue(deviceCount);
	maxEventQueue.ResetQueue(deviceCount);
	for (deviceId_t device_it = 0; device_it < deviceCount; device_it++) {
		deviceStatus_v[device_it][MIN_INACTIVE] = deviceStatus_v[device_it][MAX_INACTIVE] = deviceStatus_v[device_it][SIM_INACTIVE];
	}
//	voltage_t myMinVoltage, myMaxVoltage;
	CPower * myVoltage_p;
	for (netId_t net_it = 0; net_it < netCount; net_it++) {
//		bool myCalculatedVoltageFlag = false;
		if ( isFixedSimNet ) { // after first sim pass
			if ( simNet_v[net_it].finalNetId == UNKNOWN_NET
					|| ! netVoltagePtr_v[simNet_v[net_it].finalNetId]
					|| netVoltagePtr_v[simNet_v[net_it].finalNetId]->simVoltage == UNKNOWN_VOLTAGE ) {
				minNet_v[net_it] = simNet_v[net_it];
				maxNet_v[net_it] = simNet_v[net_it];
			} else {
				if ( minNet_v[net_it].finalNetId == UNKNOWN_NET
						|| ! netVoltagePtr_v[minNet_v[net_it].finalNetId]
						|| netVoltagePtr_v[minNet_v[net_it].finalNetId]->minVoltage == UNKNOWN_VOLTAGE
						|| ( netVoltagePtr_v[simNet_v[net_it].finalNetId]->minVoltage != UNKNOWN_VOLTAGE
								&& netVoltagePtr_v[minNet_v[net_it].finalNetId]->minVoltage <= netVoltagePtr_v[simNet_v[net_it].finalNetId]->minVoltage ) ) {
					minNet_v[net_it] = simNet_v[net_it];
				} else {
					minNet_v.Set(net_it, net_it, 0, 0);
				}
				if ( maxNet_v[net_it].finalNetId == UNKNOWN_NET
					|| ! netVoltagePtr_v[maxNet_v[net_it].finalNetId]
					|| netVoltagePtr_v[maxNet_v[net_it].finalNetId]->maxVoltage == UNKNOWN_VOLTAGE
					|| ( netVoltagePtr_v[maxNet_v[net_it].finalNetId]->maxVoltage != UNKNOWN_VOLTAGE
							&& netVoltagePtr_v[maxNet_v[net_it].finalNetId]->maxVoltage >= netVoltagePtr_v[simNet_v[net_it].finalNetId]->maxVoltage ) ) {
					maxNet_v[net_it] = simNet_v[net_it];
				} else {
					maxNet_v.Set(net_it, net_it, 0, 0);
				}
			}
			if ( netVoltagePtr_v[net_it] &&
					simNet_v[net_it].finalNetId != UNKNOWN_NET &&
					netVoltagePtr_v[simNet_v[net_it].finalNetId]->simVoltage == UNKNOWN_VOLTAGE ) {
				// remove min/max calculations if not equal to sim
				if ( netVoltagePtr_v[net_it]->type[MIN_CALCULATED_BIT] ) {
					if ( netVoltagePtr_v[net_it] == leakVoltagePtr_v[net_it] ) {  // Save old copy before erasing.
						leakVoltagePtr_v[net_it] = new CPower(netVoltagePtr_v[net_it]);
//						*(leakVoltagePtr_v[net_it]) = *(netVoltagePtr_v[net_it]);
					}
					netVoltagePtr_v[net_it]->type[MIN_CALCULATED_BIT] = false;
					netVoltagePtr_v[net_it]->defaultMinNet = UNKNOWN_NET;
					netVoltagePtr_v[net_it]->minVoltage = UNKNOWN_VOLTAGE;
//					myCalculatedVoltageFlag = true;
				}
				if ( netVoltagePtr_v[net_it]->type[MAX_CALCULATED_BIT] ) {
					if ( netVoltagePtr_v[net_it] == leakVoltagePtr_v[net_it] ) {  // Save old copy before erasing.
						leakVoltagePtr_v[net_it] = new CPower(netVoltagePtr_v[net_it]);
//						*(leakVoltagePtr_v[net_it]) = *(netVoltagePtr_v[net_it]);
					}
					netVoltagePtr_v[net_it]->type[MAX_CALCULATED_BIT] = false;
					netVoltagePtr_v[net_it]->defaultMaxNet = UNKNOWN_NET;
					netVoltagePtr_v[net_it]->maxVoltage = UNKNOWN_VOLTAGE;
//					myCalculatedVoltageFlag = true;
				}
				if ( netVoltagePtr_v[net_it]->type == NO_TYPE && IsEmpty(netVoltagePtr_v[net_it]->powerSignal()) ) {
					if ( gDebug_cvc ) {
						cout << "DEBUG: Deleting net: " << net_it;
						netVoltagePtr_v[net_it]->Print(cout);
					}
					delete netVoltagePtr_v[net_it];
					netVoltagePtr_v[net_it] = NULL;
				}
			}
//			maxNet_v[net_it].nextNetId = minNet_v[net_it].nextNetId = fixedSimNet_v[net_it].nextNetId;
//			maxNet_v[net_it].resistance = minNet_v[net_it].resistance = fixedSimNet_v[net_it].resistance;
//			maxNet_v[net_it].finalNetId = minNet_v[net_it].finalNetId = fixedSimNet_v[net_it].finalNetId;
//			maxNet_v[net_it].finalResistance = minNet_v[net_it].finalResistance = fixedSimNet_v[net_it].finalResistance;
		} else { // first min max. reset from resistance propagation.
			if ( minNet_v[net_it].nextNetId == simNet_v[net_it].nextNetId ) {
				minNet_v[net_it].finalNetId = simNet_v[net_it].finalNetId;
				if ( minNet_v.IsTerminal(net_it) ) { // save resistances for calculated power
					minNet_v[net_it].resistance = minNet_v[net_it].finalResistance;
				}
				minNet_v[net_it].lastUpdate = 0;
			} else {
				minNet_v.Set(net_it, net_it, 0, 0);
//				RecalculateFinalResistance(minEventQueue, net_it);
//				minNet_v[net_it].nextNetId = net_it;
//				minNet_v[net_it].resistance = 0;
//				minNet_v[net_it].finalNetId = net_it;
//				minNet_v[net_it].finalResistance = 0;
			}
			if ( maxNet_v[net_it].nextNetId == simNet_v[net_it].nextNetId ) {
				maxNet_v[net_it].finalNetId = simNet_v[net_it].finalNetId;
				if ( maxNet_v.IsTerminal(net_it) ) { // save resistances for calculated power
					maxNet_v[net_it].resistance = maxNet_v[net_it].finalResistance;
				}
				maxNet_v[net_it].lastUpdate = 0;
			} else {
				maxNet_v.Set(net_it, net_it, 0, 0);
//				RecalculateFinalResistance(maxEventQueue, net_it);
//				maxNet_v[net_it].nextNetId = net_it;
//				maxNet_v[net_it].resistance = 0;
//				maxNet_v[net_it].finalNetId = net_it;
//				maxNet_v[net_it].finalResistance = 0;
			}
		}
/*
		maxNet_v[net_it].nextNetId = minNet_v[net_it].nextNetId = GetEquivalentNet(net_it);
		maxNet_v[net_it].resistance = minNet_v[net_it].resistance = 0;
*/
		myVoltage_p = netVoltagePtr_v[net_it];
		if ( myVoltage_p == NULL ) continue; // skips subordinate nets
		if ( isFixedSimNet && myVoltage_p->simVoltage != UNKNOWN_VOLTAGE ) {
			// default min/max voltages are not calculated. meaning there is no path to source voltage.
			string myDefinition = myVoltage_p->definition;
			if ( myVoltage_p->minVoltage == UNKNOWN_VOLTAGE ) {
				if ( leakVoltagePtr_v[net_it] && leakVoltagePtr_v[net_it]->minVoltage != UNKNOWN_VOLTAGE
						&& leakVoltagePtr_v[net_it]->minVoltage > myVoltage_p->simVoltage ) {
					myVoltage_p->minVoltage = leakVoltagePtr_v[net_it]->minVoltage;
					myVoltage_p->defaultMinNet = leakVoltagePtr_v[net_it]->defaultMinNet;
					myDefinition += " min=leak";
				} else {
					myVoltage_p->minVoltage = myVoltage_p->simVoltage;
					myVoltage_p->defaultMinNet = UNKNOWN_NET;
					myDefinition += " min=sim";
				}
				myVoltage_p->active[MIN_ACTIVE] = true;
//				myVoltage_p->baseMinId = myVoltage_p->baseSimId;
				myVoltage_p->type[MIN_CALCULATED_BIT] = true;
			}
			if ( myVoltage_p->maxVoltage == UNKNOWN_VOLTAGE ) {
				if ( leakVoltagePtr_v[net_it] && leakVoltagePtr_v[net_it]->maxVoltage != UNKNOWN_VOLTAGE
						&& leakVoltagePtr_v[net_it]->maxVoltage < myVoltage_p->simVoltage ) {
					myVoltage_p->maxVoltage = leakVoltagePtr_v[net_it]->maxVoltage;
					myVoltage_p->defaultMaxNet = UNKNOWN_NET;
					myDefinition += " max=leak";
				} else {
					myVoltage_p->maxVoltage = myVoltage_p->simVoltage;
					myVoltage_p->defaultMaxNet = UNKNOWN_NET;
					myDefinition += " max=sim";
				}
				myVoltage_p->active[MAX_ACTIVE] = true;
//				myVoltage_p->baseMaxId = myVoltage_p->baseSimId;
				myVoltage_p->type[MAX_CALCULATED_BIT] = true;
			}
			if ( myVoltage_p->active[MIN_ACTIVE] && myVoltage_p->minVoltage > myVoltage_p->simVoltage ) {
				reportFile << "WARNING: MIN > SIM " << myVoltage_p->minVoltage << " > " << myVoltage_p->simVoltage << " for " << NetName(net_it, PRINT_CIRCUIT_ON) << endl;
			}
			if ( myVoltage_p->active[MAX_ACTIVE] && myVoltage_p->maxVoltage < myVoltage_p->simVoltage ) {
				reportFile << "WARNING: MAX < SIM " << myVoltage_p->maxVoltage << " < " << myVoltage_p->simVoltage << " for " << NetName(net_it, PRINT_CIRCUIT_ON) << endl;
			}
			myVoltage_p->definition = CPower::powerDefinitionText.SetTextAddress((text_t)myDefinition.c_str());
/*
			if ( IsCalculatedVoltage_(myVoltage_p) ) {
				myVoltage_p->type[SIM_CALCULATED_BIT] = true;
			}
*/
/* unique calculated power
			if ( IsCalculatedVoltage_(netVoltagePtr_v[net_it]) ) {
				myMinVoltage = netVoltagePtr_v[net_it]->minVoltage;
				myMaxVoltage = netVoltagePtr_v[net_it]->maxVoltage;
				if ( myMinVoltage == UNKNOWN_VOLTAGE ) myMinVoltage = netVoltagePtr_v[net_it]->simVoltage;
				if ( myMaxVoltage == UNKNOWN_VOLTAGE ) myMaxVoltage = netVoltagePtr_v[net_it]->simVoltage;
				// memory leak
				netVoltagePtr_v[net_it] = cvcParameters.cvcPowerPtrList.FindCalculatedPower(myMinVoltage, netVoltagePtr_v[net_it]->simVoltage, myMaxVoltage, net_it, this);
				if ( debug_cvc ) cout << "DEBUG: Calculated power at net: " << net_it << " " << myMinVoltage << "/" << netVoltagePtr_v[net_it]->simVoltage << "/" << myMaxVoltage << endl;
			} else {
				if ( netVoltagePtr_v[net_it]->minVoltage == UNKNOWN_VOLTAGE ) netVoltagePtr_v[net_it]->minVoltage = netVoltagePtr_v[net_it]->simVoltage;
				if ( netVoltagePtr_v[net_it]->maxVoltage == UNKNOWN_VOLTAGE ) netVoltagePtr_v[net_it]->maxVoltage = netVoltagePtr_v[net_it]->simVoltage;
			}
*/
		} else if ( SimVoltage(net_it) == UNKNOWN_VOLTAGE && IsEmpty(myVoltage_p->powerSignal()) && IsCalculatedVoltage_(myVoltage_p) ) {
			if ( gDebug_cvc ) {
				cout << "DEBUG: Deleting net: " << net_it;
				myVoltage_p->Print(cout);
			}
			netVoltagePtr_v[net_it] = NULL;
//			delete myVoltage_p;
		}

	}
/*
	RestoreQueue(minEventQueue, minEventQueue, MIN_PENDING);
	RestoreQueue(maxEventQueue, maxEventQueue, MAX_PENDING);
*/
	SetInitialMinMaxPower();
}

void CCvcDb::SetSimPower(propagation_t thePropagationType) {
	reportFile << "CVC: Propagating Simulation voltages " << thePropagationType << "..." << endl;
	simEventQueue.ResetQueue(deviceCount);
	isFixedSimNet = false;
	CVirtualNet mySimMasterNet;
	for (netId_t net_it = 0; net_it < netCount; net_it++) {
		simNet_v[net_it].lastUpdate = 0;
		if ( net_it != GetEquivalentNet(net_it) ) continue; // skip subordinate nets
		// TODO: Change to GetMasterNet
		mySimMasterNet(simNet_v, net_it);
//		mySimMasterNet = CVirtualNet(simNet_v, net_it);
		if ( netVoltagePtr_v[mySimMasterNet.finalNetId] == NULL ) continue;
		CPower * myPower_p = netVoltagePtr_v[mySimMasterNet.finalNetId];
		if ( myPower_p->simVoltage == UNKNOWN_VOLTAGE && ! IsSCRCPower(myPower_p) ) continue;
		if ( thePropagationType == POWER_NETS_ONLY && ! IsPriorityPower_(myPower_p) ) continue;
		EnqueueAttachedDevicesByTerminal(simEventQueue, net_it, firstSource_v, nextSource_v, myPower_p->simVoltage);
		EnqueueAttachedDevicesByTerminal(simEventQueue, net_it, firstDrain_v, nextDrain_v, myPower_p->simVoltage);
		EnqueueAttachedDevicesByTerminal(simEventQueue, net_it, firstGate_v, nextGate_v, myPower_p->simVoltage);
	}
	simEventQueue.queueStart = true;
	while (simEventQueue.QueueSize() > 0) {
//		cout << "Event Queue Size(min/max): " << minimumEventQueue.QueueSize() << "/" << maximumEventQueue.QueueSize() << endl;
		PropagateSimVoltages(simEventQueue, thePropagationType);
	}
//	cvcCircuitList.PrintAndResetCircuitErrors(cvcParameters.cvcCircuitErrorLimit, errorFile);
	for ( netId_t net_it = 0; net_it < netCount; net_it++ ) {
//		if ( net_it != GetEquivalentNet(net_it) ) continue; // only fix master equivalent nets
//		if ( simNet_v[net_it].nextNetId != net_it && simNet_v[net_it].resistance == 0 ) {
//			simNet_v.Set(net_it, simNet_v[net_it].nextNetId, 0); // reset final resitance
//		}

//		fixedSimNet_v[net_it](simNet_v, net_it);
		simNet_v[net_it](simNet_v, net_it); // recalculate final values
//		assert(fixedSimNet_v[net_it].nextNetId == simNet_v[net_it].finalNetId && fixedSimNet_v[net_it].resistance == simNet_v[net_it].finalResistance);
//		fixedSimNet_v[net_it].GetMasterNet(simNet_v, net_it);
	}
	isFixedSimNet = true;
}

void CCvcDb::CheckConnections() {
//	resistance_t myResistance;
	static CVirtualNet myVirtualNet;
	unordered_map<netId_t, pair<deviceId_t, deviceId_t>> myBulkCount;
//	vector<deviceId_t> myBulkCount;
//	vector<deviceId_t> myBulkDevice;
//	ResetVector<vector<deviceId_t>>(myBulkCount, netCount, 0);
//	ResetVector<vector<deviceId_t>>(myBulkDevice, netCount, UNKNOWN_DEVICE);
	for (deviceId_t device_it = 0; device_it < deviceCount; device_it++) {
		if ( bulkNet_v[device_it] == UNKNOWN_NET ) continue;
//		myBulkCount[bulkNet_v[device_it]].first++;
		if ( myBulkCount[bulkNet_v[device_it]].first++ == 0 ) {
			myBulkCount[bulkNet_v[device_it]].second = device_it;
		}
	}
	for (netId_t net_it = 0; net_it < netCount; net_it++) {
//		if ( connectionCount_v[net_it].bulkCount > 0 && connectionCount_v[net_it].bulkCount >= connectionCount_v[net_it].sourceCount + connectionCount_v[net_it].drainCount ) {
		if ( myBulkCount.count(net_it) > 0 && myBulkCount[net_it].first >= connectionCount_v[net_it].sourceCount + connectionCount_v[net_it].drainCount ) {
			// skips subordinate nets because only equivalent masters have counts.
			// may have problems with moscaps, because one device has both source and drain connection.
			myVirtualNet(simNet_v, net_it);
			assert( myVirtualNet.finalNetId != UNKNOWN_NET );
			CPower * myPower_p = netVoltagePtr_v[myVirtualNet.finalNetId];
//			myResistance = SimResistance(net_it);
			if ( myVirtualNet.finalResistance > 1000000 && myVirtualNet.finalResistance != INFINITE_RESISTANCE ) {
				reportFile << "WARNING: high res bias (" << myBulkCount[net_it].first << ") " << NetName(net_it, PRINT_CIRCUIT_ON) << " r=" << myVirtualNet.finalResistance << endl;
			} else if ( ! myPower_p
					|| ( myPower_p->simVoltage == UNKNOWN_VOLTAGE
						&& ( myPower_p->minVoltage == UNKNOWN_VOLTAGE || myPower_p->maxVoltage == UNKNOWN_VOLTAGE ) ) ) {  // no error for missing bias if min/max defined.
				reportFile << "WARNING: missing bias (" << myBulkCount[net_it].first << ") " << NetName(net_it, PRINT_CIRCUIT_ON);
				reportFile << " at " << DeviceName(myBulkCount[net_it].second, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF) << endl;
			} else if ( IsCalculatedVoltage_(myPower_p) ) {
				reportFile << "WARNING: calculated bias (" << myBulkCount[net_it].first << ") " << NetName(net_it, PRINT_CIRCUIT_ON);
				reportFile << " at " << DeviceName(myBulkCount[net_it].second, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF)
						<< (myPower_p->type[MIN_CALCULATED_BIT] ? " 1|" : " 0|")
						<< (myPower_p->type[SIM_CALCULATED_BIT] ? "1|" : "0|")
						<< (myPower_p->type[MAX_CALCULATED_BIT] ? "1" : "0") <<	endl;
			}
		}
	}
}

netId_t CCvcDb::SetInverterInput(netId_t theNetId, netId_t theMaxNetId) {
	if ( inverterNet_v[theNetId] == UNKNOWN_NET ) {
		return(theNetId);
	} else if ( inverterNet_v[theNetId] == theMaxNetId ) { // prevent looping
		return(theMaxNetId);
	} else {
		netId_t myInputId = inverterNet_v[theNetId];
		inverterNet_v[theNetId] = SetInverterInput(myInputId, max(theMaxNetId, myInputId));
		highLow_v[theNetId] = ! highLow_v[myInputId];
		return(inverterNet_v[theNetId]);
	}
}

void CCvcDb::SetInverterHighLow() {
	for( netId_t net_it = 0; net_it < netCount; net_it++ ) {
		if ( inverterNet_v[net_it] != UNKNOWN_NET ) {
			netId_t myInputId = inverterNet_v[net_it];
			inverterNet_v[net_it] = SetInverterInput(myInputId, max(net_it, myInputId));
			highLow_v[net_it] = ! highLow_v[myInputId];
		}
	}
}
