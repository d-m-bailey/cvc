/*
 * CCvcDb_init.cc
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
#include <csignal>
#include <sys/stat.h>
#include <regex>
#include "mmappable_vector.h"

extern CCvcDb * gCvcDb;
extern int gContinueCount;

char SCRC_FORCED_TEXT[] = "SCRC forced";
char SCRC_EXPECTED_TEXT[] = "SCRC expected";

void interrupt_handler(int signum) {
	if ( gInteractive_cvc ) {
		if ( gContinueCount > 0 ) {
			cout << endl << "Stopping at next stage. Use Control-\\ to abort." << endl << endl;
			gContinueCount = 0;
		} else {
			cout << endl << "Control-C disabled in interactive mode. Use Control-\\ to abort." << endl << endl;
		}
	} else {
		cout << endl << "Switching to interactive mode at next stage. Use Control-\\ to abort." << endl << endl;
		gInteractive_cvc = true;
	}
	gInterrupted = true;
}

void cleanup_handler(int signum) {
	gCvcDb->RemoveLock();
	signal(signum, SIG_DFL);
	raise(signum);
}

CCvcDb::CCvcDb(int argc, const char * argv[]) :
		cvcParameters(reportFile),
		minNet_v(MIN_CALCULATED_BIT),
		simNet_v(SIM_CALCULATED_BIT),
		maxNet_v(MAX_CALCULATED_BIT),
		initialSimNet_v(mmap_allocator<CVirtualNet>("", READ_WRITE_SHARED, 0, MAP_WHOLE_FILE | ALLOW_REMAP | BYPASS_FILE_POOL)),
		maxLeakNet_v(mmap_allocator<CVirtualNet>("", READ_WRITE_SHARED, 0, MAP_WHOLE_FILE | ALLOW_REMAP | BYPASS_FILE_POOL)),
		minLeakNet_v(mmap_allocator<CVirtualNet>("", READ_WRITE_SHARED, 0, MAP_WHOLE_FILE | ALLOW_REMAP | BYPASS_FILE_POOL)),
		maxEventQueue(MAX_QUEUE, MAX_INACTIVE, MAX_PENDING, maxNet_v, netVoltagePtr_v),
		minEventQueue(MIN_QUEUE, MIN_INACTIVE, MIN_PENDING, minNet_v, netVoltagePtr_v),
		simEventQueue(SIM_QUEUE, SIM_INACTIVE, SIM_PENDING, simNet_v, netVoltagePtr_v),
		reportFile(cout, logFile) {
	cvcArgCount = argc;
	reportPrefix = "";
	while ( cvcArgIndex < argc && argv[cvcArgIndex][0] == '-' ) {
		if ( strcmp(argv[cvcArgIndex], "--debug") == 0 ) {
			gDebug_cvc = true;
		} else if ( strcmp(argv[cvcArgIndex], "-i") == 0 || strcmp(argv[cvcArgIndex], "--interactive") == 0 ) {
			gInteractive_cvc = true;
		} else if ( strcmp(argv[cvcArgIndex], "-p") == 0 || strcmp(argv[cvcArgIndex], "--prefix") == 0 ) {
			cvcArgIndex++;
			reportPrefix = argv[cvcArgIndex];
			if ( ! IsAlphanumeric(reportPrefix) ) throw EFatalError("invalid prefix " + reportPrefix);
		} else if ( strcmp(argv[cvcArgIndex], "-v") == 0 || strcmp(argv[cvcArgIndex], "--version") == 0 ) {
			cout << "CVC: Circuit Validation Check  Version " << CVC_VERSION << endl;
		} else {
			cout << "WARNING: unrecognized option " << argv[cvcArgIndex] << endl;
		}
		cvcArgIndex++;
	}
	/*
	if ( gInteractive_cvc ) {
//		cout << endl << "Control-C disabled in interactive mode. Use Control-\\ to interrupt." << endl << endl;
		signal(SIGINT, interrupt_handler);
	}
	*/
	signal(SIGINT, interrupt_handler);
	signal(SIGABRT, cleanup_handler);
	signal(SIGFPE, cleanup_handler);
	signal(SIGILL, cleanup_handler);
	signal(SIGSEGV, cleanup_handler);
	signal(SIGTERM, cleanup_handler);
	signal(SIGQUIT, cleanup_handler);

//	if (argc > 1) {
//		cvcParameters.LoadEnvironment(argv[1]);

//		cvcParameters.cvcFileName.assign(argv[1]);
//	}
//	if (argc > 2 && strcmp(argv[2], "--debug") == 0 ) gDebug_cvc = true;
}

CCvcDb::~CCvcDb() {
	cvcCircuitList.Clear();
	instancePtr_v.Clear();
}

void CCvcDb::CountObjectsAndLinkSubcircuits() {
	reportFile << "CVC: Counting and linking..." << endl;
	isDeviceModelSet = false;
	try {
		topCircuit_p = cvcCircuitList.FindCircuit(cvcParameters.cvcTopBlock);
	}
	catch (const out_of_range& oor_exception) {
		throw EFatalError("could not find subcircuit: " + cvcParameters.cvcTopBlock);
	}
	topCircuit_p->CountObjectsAndLinkSubcircuits(cvcCircuitList.circuitNameMap);
	topCircuit_p->netCount += topCircuit_p->portCount;
	topCircuit_p->subcircuitCount++;
	topCircuit_p->instanceCount++;
	if ( topCircuit_p->netCount > MAX_NET || topCircuit_p->subcircuitCount > MAX_SUBCIRCUIT || topCircuit_p->deviceCount > MAX_DEVICE ) {
//		reportFile << "** ERROR: data size exceeds 4G items" << endl;
		throw EFatalError("data size exceeds 4G items");
//		exit(1);
	}
}

void CCvcDb::AssignGlobalIDs() {
	reportFile << "CVC: Assigning IDs ..." << endl;
	deviceCount = 0;
	subcircuitCount = 0;
	netCount = 0;
//	instances[0] = new CInstance(topCircuit, )
	netParent_v.clear();
	netParent_v.reserve(topCircuit_p->netCount);
//	subcircuitParent.reserve(topCircuit->subcircuitCount);
	deviceParent_v.clear();
	deviceParent_v.reserve(topCircuit_p->deviceCount);

	instancePtr_v.clear();
	instancePtr_v.reserve(topCircuit_p->subcircuitCount);
	instancePtr_v.resize(topCircuit_p->subcircuitCount, NULL);
	instancePtr_v[0] = new CInstance;
	instancePtr_v[0]->AssignTopGlobalIDs(this, topCircuit_p);
}

void CCvcDb::ResetMinSimMaxAndQueues() {
	isFixedEquivalentNet = false;
//	cout << "NetCount " << netCount << endl;
	ResetVector<CVirtualNetVector>(maxNet_v, netCount);
	ResetVector<CVirtualNetVector>(minNet_v, netCount);
	ResetVector<CVirtualNetVector>(simNet_v, netCount);
//	ResetVector<CVirtualNetVector>(fixedMaxNet_v, netCount);
//	ResetVector<CVirtualNetVector>(fixedMinNet_v, netCount);
//	ResetVector<CVirtualNetVector>(fixedSimNet_v, netCount);
//	ResetVector<CShortVector>(short_v, netCount);
	for (netId_t net_it = 0; net_it < netCount; net_it++) { // process all nets. before equivalency processing
		minNet_v.Set(net_it, net_it, 0, 0);
		simNet_v.Set(net_it, net_it, 0, 0);
		maxNet_v.Set(net_it, net_it, 0, 0);
//		maxNet_v[net_it].nextNetId = minNet_v[net_it].nextNetId = simNet_v[net_it].nextNetId = net_it;
//		maxNet_v[net_it].resistance = minNet_v[net_it].resistance = simNet_v[net_it].resistance = 0;
//		maxNet_v[net_it].finalNetId = minNet_v[net_it].finalNetId = simNet_v[net_it].finalNetId = net_it;
//		maxNet_v[net_it].finalResistance = minNet_v[net_it].finalResistance = simNet_v[net_it].finalResistance = 0;
//		short_v[net_it].first = UNKNOWN_NET;
//		short_v[net_it].second = "";
	}

	ResetVector<CStatusVector>(deviceStatus_v, deviceCount, 0);
//	ResetVector<CStatusVector>(netStatus_v, netCount, 0);
	minEventQueue.leakMap.powerPtrList_p = &cvcParameters.cvcPowerPtrList;
	maxEventQueue.leakMap.powerPtrList_p = &cvcParameters.cvcPowerPtrList;
	simEventQueue.leakMap.powerPtrList_p = &cvcParameters.cvcPowerPtrList;
	minEventQueue.ResetQueue(deviceCount);
	maxEventQueue.ResetQueue(deviceCount);
//	simEventQueue.ResetQueue(deviceCount);
//	cvcParameters.cvcPowerPtrList.Clear();

	isFixedMinNet = isFixedSimNet = isFixedMaxNet = false;
	leakVoltageSet = false;
	isValidShortData = false;

	for ( size_t error_it = 0; error_it < ERROR_TYPE_COUNT; error_it++ ) {
		errorCount[error_it] = 0;
	}

/*
	maximumEventQueue.queueType = MAX_QUEUE;
	maximumEventQueue.inactiveBit = MAX_INACTIVE;
	maximumEventQueue.pendingBit = MAX_PENDING;
	minimumEventQueue.queueType = MIN_QUEUE;
	minimumEventQueue.inactiveBit = MIN_INACTIVE;
	minimumEventQueue.pendingBit = MIN_PENDING;
	simulationEventQueue.queueType = SIM_QUEUE;
	simulationEventQueue.inactiveBit = SIM_INACTIVE;
	simulationEventQueue.pendingBit = SIM_PENDING;
*/
}

CPower * CCvcDb::SetMasterPower(netId_t theFirstNetId, netId_t theSecondNetId, bool & theSamePowerFlag) {
	theSamePowerFlag = false;
	if ( netVoltagePtr_v[theFirstNetId] == NULL ) return netVoltagePtr_v[theSecondNetId];
	if ( netVoltagePtr_v[theSecondNetId] == NULL ) return netVoltagePtr_v[theFirstNetId];
	if ( netVoltagePtr_v[theFirstNetId]->IsSamePower(netVoltagePtr_v[theSecondNetId]) ) {
		theSamePowerFlag = true;
		return NULL;
	}
	if ( netVoltagePtr_v[theFirstNetId]->IsValidSubset(netVoltagePtr_v[theSecondNetId], cvcParameters.cvcShortErrorThreshold) ) {
		theSamePowerFlag = true;
		return netVoltagePtr_v[theFirstNetId];
	}
	if ( netVoltagePtr_v[theSecondNetId]->IsValidSubset(netVoltagePtr_v[theFirstNetId], cvcParameters.cvcShortErrorThreshold) ) {
		theSamePowerFlag = true;
		return netVoltagePtr_v[theSecondNetId];
	}
	throw EEquivalenceError();
}

// unused
netId_t CCvcDb::MasterPowerNet(netId_t theFirstNetId, netId_t theSecondNetId) {
	if ( netVoltagePtr_v[theFirstNetId] == NULL ) return theSecondNetId;
	if ( netVoltagePtr_v[theSecondNetId] == NULL ) return theFirstNetId;
	throw EEquivalenceError();
}

// merges 2 equivalency lists
void CCvcDb::MakeEquivalentNets(CNetMap & theNetMap, netId_t theFirstNetId, netId_t theSecondNetId, deviceId_t theDeviceId) {
	netId_t myLesserNetId, myGreaterNetId;

	theFirstNetId = GetLeastEquivalentNet(theFirstNetId);
	theSecondNetId = GetLeastEquivalentNet(theSecondNetId);
	if ( theFirstNetId > theSecondNetId ) {
		myGreaterNetId = theFirstNetId;
		myLesserNetId = theSecondNetId;
	} else if ( theFirstNetId < theSecondNetId ) {
		myLesserNetId = theFirstNetId;
		myGreaterNetId = theSecondNetId;
	} else {
		return; // already equivalent
	}
	bool mySamePowerFlag;
	CPower * myMasterPower_p = SetMasterPower(theFirstNetId, theSecondNetId, mySamePowerFlag);
	netId_t myMinorNetId = ( netVoltagePtr_v[myGreaterNetId] == myMasterPower_p ) ? myLesserNetId : myGreaterNetId;
	if ( mySamePowerFlag ) { // skip to short same definition
		if ( myMasterPower_p ) {  // ignore near subset
			reportFile << endl << "INFO: Power definition ignored at " << DeviceName(theDeviceId, PRINT_CIRCUIT_ON) << endl;
			reportFile << " net " << NetName(GetEquivalentNet(myMinorNetId)) << " (" << netVoltagePtr_v[myMinorNetId]->definition << ")" << endl;
			reportFile << " shorted to " << NetName(myMasterPower_p->netId) << " (" << netVoltagePtr_v[myMasterPower_p->netId]->definition << ")" << endl;
		} else {
			reportFile << endl << "INFO: Short between same power definitions ignored at " << DeviceName(theDeviceId, PRINT_CIRCUIT_ON);
			reportFile << " net " << NetName(GetEquivalentNet(theFirstNetId), PRINT_CIRCUIT_ON) << " <-> " << NetName(GetEquivalentNet(theSecondNetId), PRINT_CIRCUIT_ON) << endl;
			return;
		}
	}
	if ( myMasterPower_p ) {
//		netId_t myMinorNetId = ( netVoltagePtr_v[myGreaterNetId] == myMasterPower_p ) ? myLesserNetId : myGreaterNetId;
		netVoltagePtr_v[myMinorNetId] = myMasterPower_p;
		for ( auto net_pit = theNetMap[myMinorNetId].begin(); net_pit != theNetMap[myMinorNetId].end(); net_pit++ ) {
			netVoltagePtr_v[*net_pit] = myMasterPower_p;
		}
	}
	theNetMap[myLesserNetId].push_front(myGreaterNetId);
	if ( theNetMap.count(myGreaterNetId) > 0 ) {
		theNetMap[myLesserNetId].splice_after(theNetMap[myLesserNetId].before_begin(), theNetMap[myGreaterNetId]);
//		for ( auto net_pit = theNetMap[myGreaterNetId].begin(); net_pit != theNetMap[myGreaterNetId].end(); net_pit++ ) {
//			theNetMap[myLesserNetId].push_front(*net_pit);
//		}
	}
	equivalentNet_v[myGreaterNetId] = myLesserNetId;
//	netVoltagePtr_v[myLesserNetId] = myMasterPower_p;
//	for ( auto net_pit = theNetMap[myLesserNetId].begin(); net_pit != theNetMap[myLesserNetId].end(); net_pit++ ) {
//		equivalentNet_v[*net_pit] = myLesserNetId;
//		netVoltagePtr_v[*net_pit] = myMasterPower_p;
//	}
}

/*
 * // merges 2 equivalency lists
void CCvcDb::MakeEquivalentNets(netId_t theFirstNetId, netId_t theSecondNetId, deviceId_t theDeviceId) {
	netId_t myLesserNetId, myGreaterNetId;

	theFirstNetId = GetGreatestEquivalentNet(theFirstNetId);
	theSecondNetId = GetGreatestEquivalentNet(theSecondNetId);
	if ( theFirstNetId > theSecondNetId ) {
		myGreaterNetId = theFirstNetId;
		myLesserNetId = theSecondNetId;
	} else if ( theFirstNetId < theSecondNetId ) {
		myLesserNetId = theFirstNetId;
		myGreaterNetId = theSecondNetId;
	} else {
		return; // already equivalent
	}
	bool mySamePowerFlag;
	CPower * myMasterPower_p = SetMasterPower(theFirstNetId, theSecondNetId, mySamePowerFlag);
	if ( mySamePowerFlag ) { // skip to short same definition
		reportFile << endl << "INFO: Short between same power definitions ignored at " << DeviceName(theDeviceId, PRINT_CIRCUIT_ON);
		reportFile << " net " << NetName(GetEquivalentNet(theFirstNetId), PRINT_CIRCUIT_ON) << " <-> " << NetName(GetEquivalentNet(theSecondNetId), PRINT_CIRCUIT_ON) << endl;
		return;
	}
//	netId_t myOldMasterPowerNet = MasterPowerNet(GetLeastEquivalentNet(theFirstNetId), GetLeastEquivalentNet(theSecondNetId));
	bool	myLesserFinishedFlag = false, myGreaterFinishedFlag = false;
	theFirstNetId = equivalentNet_v[myGreaterNetId];
	theSecondNetId = myLesserNetId;
	netId_t myCurrentNetId = myGreaterNetId;
	while ( ! myLesserFinishedFlag ) {
		if ( equivalentNet_v[myCurrentNetId] == myGreaterNetId ) myGreaterFinishedFlag = true; // first net loops
		if ( myGreaterFinishedFlag || theSecondNetId > theFirstNetId ) {
			equivalentNet_v[myCurrentNetId] = theSecondNetId;
			theSecondNetId = equivalentNet_v[theSecondNetId];
		} else {
			equivalentNet_v[myCurrentNetId] = theFirstNetId;
			theFirstNetId = equivalentNet_v[theFirstNetId];
		}
		netVoltagePtr_v[myCurrentNetId] = myMasterPower_p;
		myCurrentNetId = equivalentNet_v[myCurrentNetId];
		if ( equivalentNet_v[myCurrentNetId] == myLesserNetId ) myLesserFinishedFlag = true; // second net loops
	}
	netVoltagePtr_v[myCurrentNetId] = myMasterPower_p;
	if ( myGreaterFinishedFlag ) {
		equivalentNet_v[myCurrentNetId] = myGreaterNetId;
	} else {
		equivalentNet_v[myCurrentNetId] = theFirstNetId;
		while ( equivalentNet_v[myCurrentNetId] != myGreaterNetId ) {
			netVoltagePtr_v[myCurrentNetId] = myMasterPower_p;
			myCurrentNetId = equivalentNet_v[myCurrentNetId];
		}
		netVoltagePtr_v[myCurrentNetId] = myMasterPower_p;
	}
//	netId_t myNewMasterPowerNet = GetLeastEquivalentNet(theFirstNetId);
//	if ( netVoltagePtr_v[myOldMasterPowerNet] && myOldMasterPowerNet != myNewMasterPowerNet ) {
//		netVoltagePtr_v[myNewMasterPowerNet] = netVoltagePtr_v[myOldMasterPowerNet];
//		netVoltagePtr_v[myOldMasterPowerNet] = NULL;
//	}
}
*/

void CCvcDb::SetEquivalentNets() {
	reportFile << "CVC: Shorting switches..." << endl;
	isFixedEquivalentNet = false;
	ResetVector<CNetIdVector>(equivalentNet_v, netCount);
	for (netId_t net_it = 0; net_it < netCount; net_it++) {
		equivalentNet_v[net_it] = net_it;
	}
	CNetMap myEquivalentNetMap;
	for (CModelListMap::iterator keyModelListPair_pit = cvcParameters.cvcModelListMap.begin(); keyModelListPair_pit != cvcParameters.cvcModelListMap.end(); keyModelListPair_pit++) {
		for (CModelList::iterator model_pit = keyModelListPair_pit->second.begin(); model_pit != keyModelListPair_pit->second.end(); model_pit++) {
			if ( model_pit->type == SWITCH_ON ) {
				cout << " model " << model_pit->name << "..."; cout.flush();
				deviceId_t myPrintCount = 10000;
				deviceId_t myDeviceCount = 0;
				CDevice * myDevice_p = model_pit->firstDevice_p;
				while (myDevice_p) {
					CCircuit * myParent_p = myDevice_p->parent_p;
//					deviceId_t myLocalDeviceId = myParent_p->localDeviceIdMap[myDevice_p->name];
					deviceId_t myLocalDeviceId = myDevice_p->offset;
					assert(myDevice_p->signalId_v.size() == 2);
					for (instanceId_t instance_it = 0; instance_it < myParent_p->instanceId_v.size(); instance_it++) {
						if ( --myPrintCount <= 0 ) {
							cout << "."; cout.flush();
							myPrintCount = 10000;
						}
						CInstance * myInstance_p = instancePtr_v[myParent_p->instanceId_v[instance_it]];
						deviceId_t myDeviceId = myInstance_p->firstDeviceId + myLocalDeviceId;
						try {
							// short source and drain
							MakeEquivalentNets(myEquivalentNetMap,
									myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[0]],
									myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[1]],
									myDeviceId);
						}
						catch (const EEquivalenceError& myException) {
							CFullConnection myConnections;
							myConnections.originalSourceId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[0]];
							myConnections.originalDrainId = myInstance_p->localToGlobalNetId_v[myDevice_p->signalId_v[1]];
							myConnections.masterSimSourceNet.finalNetId = GetEquivalentNet(myConnections.originalSourceId);
							myConnections.masterSimDrainNet.finalNetId = GetEquivalentNet(myConnections.originalDrainId);
							myConnections.simSourceVoltage = SimVoltage(myConnections.masterSimSourceNet.finalNetId);
							myConnections.simDrainVoltage = SimVoltage(myConnections.masterSimDrainNet.finalNetId);
							myConnections.simSourcePower_p = netVoltagePtr_v[myConnections.masterSimSourceNet.finalNetId];
							myConnections.simDrainPower_p = netVoltagePtr_v[myConnections.masterSimDrainNet.finalNetId];
							myConnections.deviceId = myDeviceId;
							myConnections.device_p = myDevice_p;
							errorCount[LEAK]++;
							if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(myConnections.deviceId) < cvcParameters.cvcCircuitErrorLimit ) {
								errorFile << "! Short Detected: " << endl;
								PrintDeviceWithSimConnections(myParent_p->instanceId_v[instance_it], myConnections, errorFile);
								errorFile << endl;
							}
						}
						IgnoreDevice(myInstance_p->firstDeviceId + myLocalDeviceId);
						myDeviceCount++;
					}
					myDevice_p = myDevice_p->nextDevice_p;
				}
				cout << endl;
				reportFile << "	Shorted " << myDeviceCount << " " << model_pit->name << endl;
			}
		}
	}
	cvcCircuitList.PrintAndResetCircuitErrors(cvcParameters.cvcCircuitErrorLimit, errorFile, "! Switch shorts");
//	PrintEquivalentNets("before fix");
	// following iterator must be int! netId_t results in infinite loop
	for (int net_it = equivalentNet_v.size() - 1; net_it >= 0; net_it--) {
		equivalentNet_v[net_it] = GetEquivalentNet(net_it);
		if ( netVoltagePtr_v[net_it] && netId_t(net_it) != netVoltagePtr_v[net_it]->netId ) netVoltagePtr_v[net_it] = NULL;
	}
	myEquivalentNetMap.clear();
	isFixedEquivalentNet = true;
}

void CCvcDb::LinkDevices() {
	reportFile << "CVC: Linking devices..." << endl;
	ResetVector<CDeviceIdVector>(firstSource_v, netCount, UNKNOWN_DEVICE);
	ResetVector<CDeviceIdVector>(firstGate_v, netCount, UNKNOWN_DEVICE);
	ResetVector<CDeviceIdVector>(firstDrain_v, netCount, UNKNOWN_DEVICE);
//	ResetVector<CDeviceIdVector>(firstBulk_v, netCount, UNKNOWN_DEVICE);
	ResetVector<CDeviceIdVector>(nextSource_v, deviceCount, UNKNOWN_DEVICE);
	ResetVector<CDeviceIdVector>(nextGate_v, deviceCount, UNKNOWN_DEVICE);
	ResetVector<CDeviceIdVector>(nextDrain_v, deviceCount, UNKNOWN_DEVICE);
//	ResetVector<CDeviceIdVector>(nextBulk_v, deviceCount, UNKNOWN_DEVICE);
	ResetVector<CNetIdVector>(sourceNet_v, deviceCount, UNKNOWN_NET);
	ResetVector<CNetIdVector>(drainNet_v, deviceCount, UNKNOWN_NET);
	ResetVector<CNetIdVector>(gateNet_v, deviceCount, UNKNOWN_NET);
	ResetVector<CNetIdVector>(bulkNet_v, deviceCount, UNKNOWN_NET);
	ResetVector<CNetIdVector>(inverterNet_v, netCount, UNKNOWN_NET);
	ResetVector<vector<bool>>(highLow_v, netCount);
	ResetVector<vector<modelType_t>>(deviceType_v, deviceCount, UNKNOWN);
	ResetVector<CConnectionCountVector>(connectionCount_v, netCount);
	deviceId_t myDeviceCount = 0;
	instanceId_t myInstanceCount = 0;
	deviceId_t myPrintCount = 0;
	register netId_t mySourceNet, myDrainNet, myGateNet; //, myBulkNet;
	for (CCircuitPtrList::iterator circuit_ppit = cvcCircuitList.begin(); circuit_ppit != cvcCircuitList.end(); circuit_ppit++) {
		CCircuit * myCircuit_p = *circuit_ppit;
		if ( myCircuit_p->linked ) {
			for (instanceId_t instance_it = 0; instance_it < myCircuit_p->instanceId_v.size(); instance_it++) {
				myInstanceCount++;
				CInstance * myInstance_p = instancePtr_v[myCircuit_p->instanceId_v[instance_it]];
				for (deviceId_t device_it = 0; device_it < myCircuit_p->devicePtr_v.size(); device_it++) {
					myDeviceCount++;
					CDevice * myDevice_p = myCircuit_p->devicePtr_v[device_it];
					deviceId_t myDeviceId = myInstance_p->firstDeviceId + device_it;
					deviceType_v[myDeviceId] = myDevice_p->model_p->type;
//					cout << "linking device #" << myDeviceId << "from instance " << aInstanceId_v[instance_i] << endl;
					SetDeviceNets(myInstance_p, myDevice_p, sourceNet_v[myDeviceId], gateNet_v[myDeviceId], drainNet_v[myDeviceId], bulkNet_v[myDeviceId]);
					if ( ++myPrintCount >= 1000000 ) {
						cout << "	Average device/instance: " << myDeviceCount << "/" << myInstanceCount << "=" << myDeviceCount / myInstanceCount << "\r" << std::flush;
						myPrintCount = 0;
					}
					if ( sourceNet_v[myDeviceId] == drainNet_v[myDeviceId] ) {
						IgnoreDevice(myDeviceId);
						continue;
					}
					switch (deviceType_v[myDeviceId]) {
						case NMOS: case PMOS: case LDDN: case LDDP: {
							myGateNet = gateNet_v[myDeviceId];
							if ( firstGate_v[myGateNet] != UNKNOWN_DEVICE ) nextGate_v[myDeviceId] = firstGate_v[myGateNet];
							firstGate_v[myGateNet] = myDeviceId;
							connectionCount_v[myGateNet].gateCount++;
/*
							myBulkNet = bulkNet_v[myDeviceId];
							if ( myBulkNet != UNKNOWN_NET ) {
								if ( firstBulk_v[myBulkNet] != UNKNOWN_DEVICE ) nextBulk_v[myDeviceId] = firstBulk_v[myBulkNet];
								firstBulk_v[myBulkNet] = myDeviceId;
								connectionCount_v[myBulkNet].bulkCount++;
							}
*/
						}
						//continues
						case FUSE_ON: case FUSE_OFF: case RESISTOR: {
							modelType_t myDeviceType = deviceType_v[myDeviceId];
							if ( IsNmos_(myDeviceType) ) myDeviceType = NMOS; // LDDN -> NMOS
							if ( IsPmos_(myDeviceType) ) myDeviceType = PMOS; // LDDP -> PMOS
							mySourceNet = sourceNet_v[myDeviceId];
							if ( firstSource_v[mySourceNet] != UNKNOWN_DEVICE ) nextSource_v[myDeviceId] = firstSource_v[mySourceNet];
							firstSource_v[mySourceNet] = myDeviceId;
							connectionCount_v[mySourceNet].sourceCount++;
							connectionCount_v[mySourceNet].sourceDrainType[myDeviceType] = true;
							myDrainNet = drainNet_v[myDeviceId];
//							if ( myDrainNet != mySourceNet ) { // avoid double counting mos caps and inactive res
							assert(myDrainNet != mySourceNet);  // previously ignored
							if ( firstDrain_v[myDrainNet] != UNKNOWN_DEVICE ) nextDrain_v[myDeviceId] = firstDrain_v[myDrainNet];
							firstDrain_v[myDrainNet] = myDeviceId;
							connectionCount_v[myDrainNet].drainCount++;
							connectionCount_v[myDrainNet].sourceDrainType[myDeviceType] = true;
//							}
							break; }
						case SWITCH_ON: {
//							MakeEquivalentNets(mySourceId, myDrainId);
							break; }
						case BIPOLAR:	case DIODE:	case SWITCH_OFF: case CAPACITOR: {
							IgnoreDevice(myDeviceId);
							break; }
						default: {
//							cout << "ERROR: Unknown model type in LinkDevices" << endl;
							myDevice_p->model_p->Print();
							throw EUnknownModel();
						}
					}
				}
			}
		}
	}
	cout << endl;
}

returnCode_t CCvcDb::SetDeviceModels() {
	set<string> myErrorModelSet;
	reportFile << "CVC: Setting models ..." << endl;
//	parameterModelPtrMap.clear();
//	parameterModelPtrMap.reserve(cvcCircuitList.parameterText.Entries());
	parameterResistanceMap.clear();
	parameterResistanceMap.reserve(cvcCircuitList.parameterText.Entries());
	bool myModelError = false;
	for (CCircuitPtrList::iterator circuit_ppit = cvcCircuitList.begin(); circuit_ppit != cvcCircuitList.end(); circuit_ppit++) {
		CCircuit * myCircuit_p = *circuit_ppit;
		if ( myCircuit_p->linked ) {
			for (deviceId_t device_it = 0; device_it < myCircuit_p->devicePtr_v.size(); device_it++) {
				CDevice * myDevice_p = myCircuit_p->devicePtr_v[device_it];
//				try {
//					myDevice_p->model_p = parameterModelPtrMap.at(myDevice_p->parameters);
//				}
//				catch (const out_of_range& oor_exception) {
					myDevice_p->model_p = cvcParameters.cvcModelListMap.FindModel(myCircuit_p->name, myDevice_p->parameters, parameterResistanceMap, logFile);
//					parameterModelPtrMap[myDevice_p->parameters] = myDevice_p->model_p;
//				}
				if ( myDevice_p->model_p == NULL ) {
					reportFile << "ERROR: No model match " << (*circuit_ppit)->name << "/" << myDevice_p->name;
					reportFile << " " << myDevice_p->parameters << endl;
					string	myParameterString = trim_(string(myDevice_p->parameters));
					myErrorModelSet.insert(myParameterString.substr(0, myParameterString.find(" ", 2)));
					myModelError = true;
				} else if ( ! myDevice_p->model_p->validModel ) {
					reportFile << "ERROR: Invalid model definition for " << (*circuit_ppit)->name << "/" << myDevice_p->name;
					reportFile << " " << myDevice_p->parameters << endl;
					string	myParameterString = trim_(string(myDevice_p->parameters));
					myErrorModelSet.insert(myParameterString.substr(0, myParameterString.find(" ", 2)));
					myModelError = true;
				} else {
					if ( myDevice_p->model_p->firstDevice_p != NULL ) {
						myDevice_p->nextDevice_p = myDevice_p->model_p->firstDevice_p;
					}
					myDevice_p->model_p->firstDevice_p = myDevice_p;
					if ( myDevice_p->model_p->baseType == "M" && ( myDevice_p->model_p->type == FUSE_ON || myDevice_p->model_p->type == FUSE_OFF ) ) {
						if ( myDevice_p->signalId_v[0] != myDevice_p->signalId_v[2] ) {
							reportFile << "ERROR: mosfet used as fuse must have source=drain: " << (*circuit_ppit)->name << "/" << myDevice_p->name << endl;
							myModelError = true;
						} else if ( cvcParameters.cvcSOI && myDevice_p->signalId_v[0] != myDevice_p->signalId_v[3] ) {
							reportFile << "ERROR: mosfet used as fuse must have source=drain=bulk: " << (*circuit_ppit)->name << "/" << myDevice_p->name << endl;
							myModelError = true;
/*
						} else {
							myDevice_p->signalId_v[0] = myDevice_p->signalId_v[1]; // make drain = gate for mosfets used as fuse.
*/
						}
					}
				}
			}
		}
	}
	isDeviceModelSet = true;
	if ( myModelError ) {
		if ( ! myErrorModelSet.empty() ) {
			reportFile << "Missing models" << endl;
			for ( auto model_pit = myErrorModelSet.begin(); model_pit != myErrorModelSet.end(); model_pit++) {
				reportFile << *model_pit << endl;
			}
		}
		reportFile << "ERROR: Model file problem" << endl;
		return(FAIL);
	} else {
		return (cvcParameters.cvcModelListMap.SetVoltageTolerances(reportFile, cvcParameters.cvcPowerMacroPtrMap));
	}
}

/*
void CCvcDb::ResetMosFuse() {
	for (CModelListMap::iterator keyModelListPair_pit = cvcParameters.cvcModelListMap.begin(); keyModelListPair_pit != cvcParameters.cvcModelListMap.end(); keyModelListPair_pit++) {
		for (CModelList::iterator model_pit = keyModelListPair_pit->second.begin(); model_pit != keyModelListPair_pit->second.end(); model_pit++) {
			if ( model_pit->baseType == "M" && ( model_pit->type == FUSE_ON || model_pit->type == FUSE_OFF ) ) {
				for (CDevice * device_pit = model_pit->firstDevice_p; device_pit != NULL; device_pit = device_pit->nextDevice_p) {
					device_pit->signalId_v[0] = device_pit->signalId_v[2]; // reset source = drain for mos used as fuse
				}
			}
		}
	}
}
*/

void CCvcDb::OverrideFuses() {
	if ( IsEmpty(cvcParameters.cvcFuseFilename) ) return;
	ifstream myFuseFile(cvcParameters.cvcFuseFilename);
	if ( myFuseFile.fail() ) {
//		cout << "ABORT: Could not open fuse file: " << cvcParameters.cvcFuseFilename << endl;
		throw EFatalError("Could not open fuse file: " + cvcParameters.cvcFuseFilename);
//		exit(1);
	}
	string myFuseName, myFuseStatus;
	bool myFuseError = false;
	deviceId_t myDeviceId;
	string myInputLine;
	while ( getline(myFuseFile, myInputLine) ) {
		if ( myInputLine.length() == 0 ) continue;
		if ( myInputLine.substr(0, 1) == "#" ) continue;
		myInputLine = trim_(myInputLine);
		stringstream myFuseStringStream(myInputLine);
		myFuseStringStream >> myFuseName >> myFuseStatus;
		if ( myFuseStringStream.fail() ) {
			myFuseError = true;
			continue;
		}
		myDeviceId = FindDevice(0, myFuseName);
		if ( myFuseStatus != "fuse_on" && myFuseStatus != "fuse_off" ) {
			myFuseError = true;
		}
		if ( myDeviceId == UNKNOWN_DEVICE ) {
			myFuseError = true;
		} else {
			CInstance * myInstance_p = instancePtr_v[deviceParent_v[myDeviceId]];
			CCircuit * myCircuit_p = myInstance_p->master_p;
			CModel * myModel_p = myCircuit_p->devicePtr_v[myDeviceId - myInstance_p->firstDeviceId]->model_p;
			if ( myModel_p->type != FUSE_ON && myModel_p->type != FUSE_OFF ) {
				myFuseError = true;
			}
			if ( myFuseError == false ) {
				if ( myModel_p->type == FUSE_ON && myFuseStatus == "fuse_off" ) {
					deviceType_v[myDeviceId] = FUSE_OFF;
					logFile << "INFO: fuse override ON->OFF " << myFuseName << endl;
				} else if ( myModel_p->type == FUSE_OFF && myFuseStatus == "fuse_on" ) {
					deviceType_v[myDeviceId] = FUSE_ON;
					logFile << "INFO: fuse override OFF->ON " << myFuseName << endl;
				}
			}
		}
	}
	myFuseFile.close();
	if ( myFuseError ) {
		logFile << "ABORT: unexpected fuse error" << endl;
		throw EFatalError("unexpected fuse error");
//		exit(1);
	}
	return;
}

/*
void CCvcDb::ShortNonconductingResistor(netId_t theNetId) {
	deviceId_t myDeviceId = UNKNOWN_DEVICE;
	if ( firstSource_v[net_it] != UNKNOWN_DEVICE ) {
		myDeviceId = firstSource_v[net_it];
		if ( firstDrain_v[net_it] != UNKNOWN_DEVICE ) {
			throw EDatabaseError();
		}
	} else if ( firstDrain_v[net_it] != UNKNOWN_DEVICE ) {
		myDeviceId = firstDrain_v[net_it];
	} else {
		throw EDatabaseError();
	}
}
*/

deviceId_t CCvcDb::FindUniqueSourceDrainConnectedDevice(netId_t theNetId) {
	deviceId_t myResultDeviceId = UNKNOWN_DEVICE;
	for ( deviceId_t device_it = firstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
		if ( deviceType_v[device_it] == RESISTOR && ! deviceStatus_v[device_it][SIM_INACTIVE] ) {
			if ( myResultDeviceId != UNKNOWN_DEVICE ) throw EDatabaseError("FindUniqueSourceDrainConnectedDevice: " + NetName(theNetId));
			myResultDeviceId = device_it;
		}
	}
	for ( deviceId_t device_it = firstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
		if ( deviceType_v[device_it] == RESISTOR && ! deviceStatus_v[device_it][SIM_INACTIVE] ) {
			if ( myResultDeviceId != UNKNOWN_DEVICE ) throw EDatabaseError("FindUniqueSourceDrainConnectedDevice: " + NetName(theNetId));
			myResultDeviceId = device_it;
		}
	}
	return myResultDeviceId;
}

void CCvcDb::MergeConnectionListByTerminals(netId_t theFromNet, netId_t theToNet, deviceId_t theIgnoreDeviceId,
		CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v, CNetIdVector& theTerminal_v) {
	deviceId_t myNextDevice;
	for ( deviceId_t device_it = theFirstDevice_v[theFromNet]; device_it != UNKNOWN_DEVICE; device_it = myNextDevice) {
		myNextDevice = theNextDevice_v[device_it];
//		if ( device_it == theIgnoreDeviceId ) {
//			theNextDevice_v[device_it] = UNKNOWN_DEVICE;
//		} else {
			theNextDevice_v[device_it] = theFirstDevice_v[theToNet];
			theFirstDevice_v[theToNet] = device_it;
//		}
		theTerminal_v[device_it] = theToNet;
	}
	theFirstDevice_v[theFromNet] = UNKNOWN_DEVICE;
}

void CCvcDb::DumpConnectionList(string theHeading, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v) {
	deviceId_t myDeviceId;
	cout << theHeading << endl;
	for ( netId_t net_it = 0; net_it < theFirstDevice_v.size(); net_it++) {
		if ( theFirstDevice_v[net_it] == UNKNOWN_DEVICE ) continue;
		cout << "  " << net_it << ":" << NetName(net_it) << " => ";
		myDeviceId = theFirstDevice_v[net_it];
		while ( myDeviceId != UNKNOWN_DEVICE ) {
			cout << myDeviceId << ":" << DeviceName(myDeviceId) << " -> ";
			myDeviceId = theNextDevice_v[myDeviceId];
		}
		cout << "end" << endl;
	}
}

void CCvcDb::DumpConnectionLists(string theHeading) {
	cout << "Connection list dump " << theHeading << endl;
	DumpConnectionList(" Gate connections", firstGate_v, nextGate_v);
	DumpConnectionList(" Source connections", firstSource_v, nextSource_v);
	DumpConnectionList(" Drain connections", firstDrain_v, nextDrain_v);
//	DumpConnectionList(" Bulk connections", firstBulk_v, nextBulk_v);
	cout << "Connection list dump end" << endl;
}

deviceId_t CCvcDb::RecountConnections(netId_t theNetId, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v) {
	deviceId_t myCount = 0;
	for ( deviceId_t device_it = theFirstDevice_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theNextDevice_v[device_it] ) {
		myCount++;
	}
	return myCount;
}

void CCvcDb::MergeConnectionLists(netId_t theFromNet, netId_t theToNet, deviceId_t theIgnoreDeviceId) {
	static int myCallCount = 0;
	if ( myCallCount == 0 ) {
//		DumpConnectionLists(to_string<int>(myCallCount++));
	}
	MergeConnectionListByTerminals(theFromNet, theToNet, theIgnoreDeviceId, firstGate_v, nextGate_v, gateNet_v);
	MergeConnectionListByTerminals(theFromNet, theToNet, theIgnoreDeviceId, firstSource_v, nextSource_v, sourceNet_v);
	MergeConnectionListByTerminals(theFromNet, theToNet, theIgnoreDeviceId, firstDrain_v, nextDrain_v, drainNet_v);
//	MergeConnectionListByTerminals(theFromNet, theToNet, theIgnoreDeviceId, firstBulk_v, nextBulk_v, bulkNet_v);
	// Source/drain counts handled in calling routine
	connectionCount_v[theFromNet].gateCount = RecountConnections(theFromNet, firstGate_v, nextGate_v);
//	connectionCount_v[theFromNet].sourceCount = RecountConnections(theFromNet, firstSource_v, nextSource_v);
//	connectionCount_v[theFromNet].drainCount = RecountConnections(theFromNet, firstDrain_v, nextDrain_v);
//	connectionCount_v[theFromNet].bulkCount = RecountConnections(theFromNet, firstBulk_v, nextBulk_v);
	connectionCount_v[theToNet].gateCount = RecountConnections(theToNet, firstGate_v, nextGate_v);
//	connectionCount_v[theToNet].sourceCount = RecountConnections(theToNet, firstSource_v, nextSource_v);
//	connectionCount_v[theToNet].drainCount = RecountConnections(theToNet, firstDrain_v, nextDrain_v);
//	connectionCount_v[theToNet].bulkCount = RecountConnections(theToNet, firstBulk_v, nextBulk_v);
	connectionCount_v[theToNet].sourceDrainType |= connectionCount_v[theFromNet].sourceDrainType;
//	DumpConnectionLists(to_string<int>(myCallCount++));
}

void CCvcDb::ShortNonConductingResistor(deviceId_t theDeviceId, netId_t theFirstNet, netId_t theSecondNet, shortDirection_t theDirection) {
/*
	minNet_v.Set(theFirstNet, theSecondNet, 0);
	simNet_v.Set(theFirstNet, theSecondNet, 0);
	maxNet_v.Set(theFirstNet, theSecondNet, 0);
*/
//	maxNet_v[theFirstNet].nextNetId = theSecondNet;
//	minNet_v[theFirstNet].nextNetId = theSecondNet;
//	simNet_v[theFirstNet].nextNetId = theSecondNet;
	CCircuit * myParent_p = instancePtr_v[deviceParent_v[theDeviceId]]->master_p;
	deviceId_t myLocalDeviceId = theDeviceId - instancePtr_v[deviceParent_v[theDeviceId]]->firstDeviceId;
	CDevice * myDevice_p = myParent_p->devicePtr_v[myLocalDeviceId];
//	maxNet_v[theFirstNet].resistance = 0;
//	minNet_v[theFirstNet].resistance = 0;
//	simNet_v[theFirstNet].resistance = 0;
//	maxNet_v.SetFinalNet(theFirstNet);
//	minNet_v.SetFinalNet(theFirstNet);
//	simNet_v.SetFinalNet(theFirstNet);
	deviceStatus_v[theDeviceId][MAX_INACTIVE] = true;
	deviceStatus_v[theDeviceId][MIN_INACTIVE] = true;
	deviceStatus_v[theDeviceId][SIM_INACTIVE] = true;
	myDevice_p->sourceDrainSet = true;

	if ( theDirection == SOURCE_TO_MASTER_DRAIN ) {
		connectionCount_v[theFirstNet].sourceCount--;
		connectionCount_v[theSecondNet].drainCount--;
//		SwapSourceDrain(myDevice_p);
//		myDevice_p->sourceDrainSwapOk = false;
	} else if ( theDirection == DRAIN_TO_MASTER_SOURCE ){
		connectionCount_v[theFirstNet].drainCount--;
		connectionCount_v[theSecondNet].sourceCount--;
	}

	minNet_v.Set(theFirstNet, theSecondNet, 1, ++minNet_v.lastUpdate);
	simNet_v.Set(theFirstNet, theSecondNet, 1, ++simNet_v.lastUpdate);
	maxNet_v.Set(theFirstNet, theSecondNet, 1, ++maxNet_v.lastUpdate);
	MergeConnectionLists(theFirstNet, theSecondNet, theDeviceId);
	assert(connectionCount_v[theFirstNet].SourceDrainCount() == 0);
	// propagate
	if ( connectionCount_v[theSecondNet].SourceDrainCount() == 1 && netVoltagePtr_v[theSecondNet] == NULL ) {
		deviceId_t myConnectedDeviceId = FindUniqueSourceDrainConnectedDevice(theSecondNet);
		if ( myConnectedDeviceId != UNKNOWN_DEVICE ) {
			if ( sourceNet_v[myConnectedDeviceId] == theSecondNet ) {
				ShortNonConductingResistor(myConnectedDeviceId, sourceNet_v[myConnectedDeviceId], drainNet_v[myConnectedDeviceId], SOURCE_TO_MASTER_DRAIN);
			} else if ( drainNet_v[myConnectedDeviceId] == theSecondNet ) {
				ShortNonConductingResistor(myConnectedDeviceId, drainNet_v[myConnectedDeviceId], sourceNet_v[myConnectedDeviceId], DRAIN_TO_MASTER_SOURCE);
			} else throw EDatabaseError("ShortNonConductingResistor: " + DeviceName(myConnectedDeviceId));

		}
	}
//	RecalculateFinalResistance(minEventQueue, theFirstNet);
//	RecalculateFinalResistance(simEventQueue, theFirstNet);
//	RecalculateFinalResistance(maxEventQueue, theFirstNet);
}

void CCvcDb::ShortNonConductingResistors() {
	reportFile << "CVC: Shorting non conducting resistors..." << endl;
//  TODO: process by model?
	for (deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		if ( deviceType_v[device_it] == RESISTOR && ! deviceStatus_v[device_it][SIM_INACTIVE] ) {
//			if ( sourceNet_v[device_it] < topCircuit_p->portCount || drainNet_v[device_it] < topCircuit_p->portCount ) continue; // skip top ports
			if ( sourceNet_v[device_it] >= topCircuit_p->portCount  // don't consider input nets
					&& connectionCount_v[sourceNet_v[device_it]].SourceDrainCount() == 1  // only one leak path
					&& netVoltagePtr_v[sourceNet_v[device_it]] == NULL ) {  // not defined
				ShortNonConductingResistor(device_it, sourceNet_v[device_it], drainNet_v[device_it], SOURCE_TO_MASTER_DRAIN);
			} else if ( drainNet_v[device_it] >= topCircuit_p->portCount  // don't consider ports
					&& connectionCount_v[drainNet_v[device_it]].SourceDrainCount() == 1  // only one leak path
					&& netVoltagePtr_v[drainNet_v[device_it]] == NULL ) {  // not defined
				ShortNonConductingResistor(device_it, drainNet_v[device_it], sourceNet_v[device_it], DRAIN_TO_MASTER_SOURCE);
			} else {
				continue;
			}
		}
	}
}

forward_list<instanceId_t> CCvcDb::FindInstanceIds(string theHierarchy, instanceId_t theParent) {
	list<string> * myHierarchyList_p = SplitHierarchy(theHierarchy);
	forward_list<instanceId_t> mySearchInstanceIdList;
//	size_t myHierarchyOffset = thePowerSignal.find(HIERARCHY_DELIMITER, 0);
//	size_t myStringBegin = 0;
//	size_t myStringEnd;
	string	myInstanceName = "";
	string	myUnmatchedInstance = "";
//	instanceId_t mySearchInstanceId = UNKNOWN_SUBCIRCUIT;
	try {
		for ( auto hierarchy_pit = myHierarchyList_p->begin(); hierarchy_pit != myHierarchyList_p->end(); hierarchy_pit++ ) {
			if ( (*hierarchy_pit).empty() ) {
				mySearchInstanceIdList.push_front(0);
			} else if ( hierarchy_pit->substr(0,2) == "*(" && hierarchy_pit->substr(hierarchy_pit->size() - 1, 1) == ")" ) { // circuit search
				if ( ! IsEmpty(myUnmatchedInstance) ) throw out_of_range("invalid hierarchy: " + myUnmatchedInstance + HIERARCHY_DELIMITER + *hierarchy_pit ); // no circuit searches with pending hierarchy
				string myCellName = theHierarchy.substr(2, hierarchy_pit->size() - 3);
				regex mySearchPattern(FuzzyFilter(myCellName));
				bool myFoundMatch = false;
				if ( mySearchInstanceIdList.empty() ) { // global circuit search
					try { // exact match
						text_t myCellText = cvcCircuitList.cdlText.GetTextAddress(myCellName);
						CCircuit * myCircuit = cvcCircuitList.circuitNameMap.at(myCellText);
						myFoundMatch = true;
						for ( auto instance_pit = myCircuit->instanceId_v.begin(); instance_pit != myCircuit->instanceId_v.end(); instance_pit++ ) {
							mySearchInstanceIdList.push_front(*instance_pit);
						}
					}
					catch (const out_of_range& oor_exception) { // check for regex match
						for ( auto circuit_pit = cvcCircuitList.begin(); circuit_pit != cvcCircuitList.end(); circuit_pit++ ) {
							if ( regex_match((*circuit_pit)->name, mySearchPattern) ) {
								for ( auto instance_pit = (*circuit_pit)->instanceId_v.begin(); instance_pit != (*circuit_pit)->instanceId_v.end(); instance_pit++ ) {
									mySearchInstanceIdList.push_front(*instance_pit);
									myFoundMatch = true;
								}
							}
						}
					}
				} else { // local circuit search
					forward_list<instanceId_t> myNewSearchList;
					for (auto instanceId_pit = mySearchInstanceIdList.begin(); instanceId_pit != mySearchInstanceIdList.end(); instanceId_pit++) {
						instanceId_t myParentsFirstSubcircuitId = instancePtr_v[*instanceId_pit]->firstSubcircuitId;
						CCircuit * myCircuit = instancePtr_v[*instanceId_pit]->master_p;
						for ( size_t subcircuit_it = 0; subcircuit_it < myCircuit->subcircuitPtr_v.size(); subcircuit_it++ ) {
							try { // exact match
								text_t myCellText = cvcCircuitList.cdlText.GetTextAddress(myCellName);
								if ( myCircuit->subcircuitPtr_v[subcircuit_it]->masterName == myCellText ) {
									myNewSearchList.push_front(myParentsFirstSubcircuitId + subcircuit_it);
									myFoundMatch = true;
								}
							}
							catch (const out_of_range& oor_exception) { // check for regex match
								if ( regex_match(myCircuit->subcircuitPtr_v[subcircuit_it]->masterName, mySearchPattern) ) {
									myNewSearchList.push_front(myParentsFirstSubcircuitId + subcircuit_it);
									myFoundMatch = true;
								}
							}
						}
					}
					mySearchInstanceIdList.clear();
					mySearchInstanceIdList = myNewSearchList;
				}
				if ( ! myFoundMatch ) throw out_of_range("invalid hierarchy: missing circuit " + myCellName);
			} else { // instance search
				if ( mySearchInstanceIdList.empty() ) throw out_of_range("invalid hierarchy: " + *hierarchy_pit); // no relative path searches
				if ( ! IsEmpty(myUnmatchedInstance) ) {
					myInstanceName = myUnmatchedInstance + HIERARCHY_DELIMITER + *hierarchy_pit;
				} else {
					myInstanceName = *hierarchy_pit;
				}
				regex mySearchPattern(FuzzyFilter(myInstanceName));
				forward_list<instanceId_t> myNewSearchList;
				bool myFoundMatch = false;
				for (auto instanceId_pit = mySearchInstanceIdList.begin(); instanceId_pit != mySearchInstanceIdList.end(); instanceId_pit++) {
					instanceId_t myParentsFirstSubcircuitId = instancePtr_v[*instanceId_pit]->firstSubcircuitId;
					CCircuit * myCircuit = instancePtr_v[*instanceId_pit]->master_p;
					for ( auto instance_pit = myCircuit->subcircuitPtr_v.begin(); instance_pit != myCircuit->subcircuitPtr_v.end(); instance_pit++ ) {
						try { // exact match
							text_t myInstanceText = cvcCircuitList.cdlText.GetTextAddress(myInstanceName);
							if ( (*instance_pit)->name == myInstanceText ) {
								myNewSearchList.push_front(myParentsFirstSubcircuitId + (*instance_pit)->offset);
								myFoundMatch = true;
							}
						}
						catch (const out_of_range& oor_exception) { // check for regex match
							if ( regex_match((*instance_pit)->name, mySearchPattern) ) {
								myNewSearchList.push_front(myParentsFirstSubcircuitId + (*instance_pit)->offset);
								myFoundMatch = true;
							}
						}
					}
				}
				if ( myFoundMatch ) {
					myUnmatchedInstance = "";
					mySearchInstanceIdList.clear();
					mySearchInstanceIdList = myNewSearchList;
				} else {
					myUnmatchedInstance = myInstanceName;
				}
			}
		}
		if ( theParent != 0 ) {
			// only use instances in parent
			forward_list<instanceId_t> myNewSearchList;
			for (auto instanceId_pit = mySearchInstanceIdList.begin(); instanceId_pit != mySearchInstanceIdList.end(); instanceId_pit++) {
				if ( IsSubcircuitOf(*instanceId_pit, theParent) ) {
					myNewSearchList.push_front(*instanceId_pit);
				}
			}
			mySearchInstanceIdList = myNewSearchList;
		}
	}
	catch (const out_of_range& oor_exception) {
		reportFile << "ERROR: could not expand instance " << theHierarchy << " " << oor_exception.what() << endl;
		mySearchInstanceIdList.clear();
	}
	catch (const regex_error& myError) {
		reportFile << "regex_error: " << RegexErrorString(myError.code()) << endl;
		mySearchInstanceIdList.clear();
	}
	if ( myUnmatchedInstance != "" ) {  // not a complete match
		mySearchInstanceIdList.clear();
	}
	return mySearchInstanceIdList;
}

set<netId_t> * CCvcDb::FindNetIds(string thePowerSignal, instanceId_t theParent) {
	list<string> * myHierarchyList_p = SplitHierarchy(thePowerSignal);
	set<netId_t> * myNetIdSet_p = new set<netId_t>;
	forward_list<instanceId_t> mySearchInstanceIdList;
//	size_t myHierarchyOffset = thePowerSignal.find(HIERARCHY_DELIMITER, 0);
//	size_t myStringBegin = 0;
//	size_t myStringEnd;
	string	myInstanceName = "";
	string	mySignalName = "";
	string	myUnmatchedInstance = "";
//	instanceId_t mySearchInstanceId = UNKNOWN_SUBCIRCUIT;
	size_t mySearchEnd = thePowerSignal.length();
	do {
		mySearchEnd = thePowerSignal.find_last_of(HIERARCHY_DELIMITER, mySearchEnd-1);
		if (mySearchEnd > thePowerSignal.length()) {
			mySearchEnd = 0;
			myInstanceName = "";
			mySignalName = thePowerSignal;
		} else {
			myInstanceName = thePowerSignal.substr(0, mySearchEnd);
			mySignalName = thePowerSignal.substr(mySearchEnd+1);
		}
		mySearchInstanceIdList = FindInstanceIds(myInstanceName, theParent);
//		cout << "DEBUG: " << thePowerSignal << " split into " << myInstanceName << " and " << mySignalName << endl;
	} while( mySearchInstanceIdList.empty() && myInstanceName != "" );
	try {
//		cout << "DEBUG: instance list " << mySearchInstanceIdList.front() << endl;
		list<string> * myNetNameList_p = ExpandBusNet(mySignalName);
		bool myCheckTopPort = false;
		if ( mySignalName == thePowerSignal ) { // top port
			myCheckTopPort = true;
//			mySearchInstanceIdList.push_front(0);
		}
		bool myFoundNetMatch = false;
		for ( auto myNetName_pit = myNetNameList_p->begin(); myNetName_pit != myNetNameList_p->end(); myNetName_pit++ ) {
			string myNetName = *myNetName_pit;
			if ( ! myUnmatchedInstance.empty() ) {
				myNetName = myUnmatchedInstance + HIERARCHY_DELIMITER + myNetName;
			}
			string myFuzzyFilter = FuzzyFilter(myNetName);
			regex mySearchPattern(myFuzzyFilter);
			netId_t myNetId;
			bool myExactMatch = true;
			bool myFuzzySearch = (myFuzzyFilter.find_first_of("^$.*+?()[]{}|\\") < myFuzzyFilter.npos);
			text_t mySignalText;
			try {
				mySignalText = cvcCircuitList.cdlText.GetTextAddress(myNetName);
			}
			catch (const out_of_range& oor_exception) {
				myExactMatch = false;
			}
			for (auto instanceId_pit = mySearchInstanceIdList.begin(); instanceId_pit != mySearchInstanceIdList.end(); instanceId_pit++) {
//				bool myCheckTopPort = ( *instanceId_pit == 0 );
				CTextNetIdMap * mySignalIdMap_p = &(instancePtr_v[*instanceId_pit]->master_p->localSignalIdMap);
				for ( auto signalIdPair_pit = mySignalIdMap_p->begin(); signalIdPair_pit != mySignalIdMap_p->end(); signalIdPair_pit++ ) {
					if ( myExactMatch ) { // exact match
						if ( signalIdPair_pit->first == mySignalText ) {
							myNetId = instancePtr_v[*instanceId_pit]->localToGlobalNetId_v[signalIdPair_pit->second];
//							if ( myCheckTopPort && *instanceId_pit == 0 && myNetId >= topCircuit_p->portCount ) throw out_of_range("requires leading '/'"); // top signals that are not ports posing as ports
//							if ( ! myCheckTopPort && *instanceId_pit == 0 && myNetId < topCircuit_p->portCount ) throw out_of_range("leading '/' not needed"); // top signals that should be ports
							if ( myCheckTopPort && *instanceId_pit == 0 && myNetId >= topCircuit_p->portCount ) continue; // top signals that are not ports posing as ports
							if ( ! myCheckTopPort && *instanceId_pit == 0 && myNetId < topCircuit_p->portCount ) continue; // top signals that should be ports
							myNetIdSet_p->insert(myNetId);
							myFoundNetMatch = true;
						}
					} else if ( myFuzzySearch ) {
						if ( regex_match(signalIdPair_pit->first, mySearchPattern) ) {
							myNetId = instancePtr_v[*instanceId_pit]->localToGlobalNetId_v[signalIdPair_pit->second];
//							if ( myCheckTopPort && *instanceId_pit == 0 && myNetId >= topCircuit_p->portCount ) throw out_of_range("requires leading '/'"); // top signals that are not ports posing as ports
//							if ( ! myCheckTopPort && *instanceId_pit == 0 && myNetId < topCircuit_p->portCount ) throw out_of_range("leading '/' not needed"); // top signals that should be ports
							if ( myCheckTopPort && *instanceId_pit == 0 && myNetId >= topCircuit_p->portCount ) continue; // top signals that are not ports posing as ports
							if ( ! myCheckTopPort && *instanceId_pit == 0 && myNetId < topCircuit_p->portCount ) continue; // top signals that should be ports
							myNetIdSet_p->insert(myNetId);
							myFoundNetMatch = true;
						}
					}
	//				myNetIdSet_p->insert(instancePtr_v[*instanceId_pit]->localToGlobalNetId_v[myLocalNetId]);
				}
			}
//			if ( myCheckTopPort ) {
//				for ( auto net_pit = myNetIdSet_p->begin(); net_pit != myNetIdSet_p->end(); net_pit++ ) {
//					if ( *net_pit >= topCircuit_p->portCount ) throw out_of_range("invalid signal"); // top signals that are not ports without leading hierarchy delimiter
//				}
//			}
			if ( ! myFoundNetMatch ) throw out_of_range("signal " + myNetName + " not found");
		}
		delete myNetNameList_p;
		if ( ! myFoundNetMatch ) throw out_of_range("signal not found");
	}
	catch (const out_of_range& oor_exception) {
		reportFile << "ERROR: could not expand signal " << thePowerSignal << " " << oor_exception.what() << endl;
		myNetIdSet_p->clear();
	}
	catch (const regex_error& myError) {
		reportFile << "regex_error: " << RegexErrorString(myError.code()) << endl;
		myNetIdSet_p->clear();
	}
	delete myHierarchyList_p;
	return myNetIdSet_p;
}

returnCode_t CCvcDb::SetModePower() {
	bool myPowerError = false;
	ResetPointerVector<CPowerPtrVector>(netVoltagePtr_v, netCount);
	ResetVector<CStatusVector>(netStatus_v, netCount, 0);
//	for (CPowerPtrList::iterator power_ppit = cvcParameters.cvcPowerPtrList.begin(); power_ppit != cvcParameters.cvcPowerPtrList.end(); power_ppit++) {
	CPowerPtrList::iterator power_ppit = cvcParameters.cvcPowerPtrList.begin();
	// Normal power definitions
	while( power_ppit != cvcParameters.cvcPowerPtrList.end() ) {
		CPower * myPower_p = *power_ppit;
		set<netId_t> * myNetIdList = FindNetIds(string(myPower_p->powerSignal())); // expands buses and hierarchy
		for (auto netId_pit = myNetIdList->begin(); netId_pit != myNetIdList->end(); netId_pit++) {
			string myExpandedNetName = NetName(*netId_pit);
			if ( string(myPower_p->powerSignal()) != myExpandedNetName ) {
//				logFile << "INFO: Expanded " << myPower_p->powerSignal << " -> " << myExpandedNetName << endl;
			}
 			if (netVoltagePtr_v[*netId_pit] && myPower_p->definition != netVoltagePtr_v[*netId_pit]->definition ) {
 				bool myPowerConflict = false;
 				if ( myPower_p->expectedMin() != "" ) {
 					if ( netVoltagePtr_v[*netId_pit]->expectedMin() == "" ) {
 						logFile << "INFO: Override expectedMin for " << NetName(*netId_pit) << " from " << myPower_p->definition << endl;
 						netVoltagePtr_v[*netId_pit]->expectedMin() = myPower_p->expectedMin();
 					} else if ( netVoltagePtr_v[*netId_pit]->expectedMin() != myPower_p->expectedMin() ) {
 						myPowerConflict = true;
 					}
 				}
 				if ( myPower_p->expectedSim() != "" ) {
 					if ( netVoltagePtr_v[*netId_pit]->expectedSim() == "" ) {
 						logFile << "INFO: Override expectedSim for " << NetName(*netId_pit) << " from " << myPower_p->definition << endl;
 						netVoltagePtr_v[*netId_pit]->expectedSim() = myPower_p->expectedSim();
 					} else if ( netVoltagePtr_v[*netId_pit]->expectedSim() != myPower_p->expectedSim() ) {
 						myPowerConflict = true;
 					}
 				}
 				if ( myPower_p->expectedMax() != "" ) {
 					if ( netVoltagePtr_v[*netId_pit]->expectedMax() == "" ) {
 						logFile << "INFO: Override expectedMax for " << NetName(*netId_pit) << " from " << myPower_p->definition << endl;
 						netVoltagePtr_v[*netId_pit]->expectedMax() = myPower_p->expectedMax();
 					} else if ( netVoltagePtr_v[*netId_pit]->expectedMax() != myPower_p->expectedMax() ) {
 						myPowerConflict = true;
 					}
 				}
 				if ( myPower_p->minVoltage != UNKNOWN_VOLTAGE ) {
 					if ( netVoltagePtr_v[*netId_pit]->minVoltage == UNKNOWN_VOLTAGE ) {
 						logFile << "INFO: Override minVoltage for " << NetName(*netId_pit) << " from " << myPower_p->definition << endl;
 						netVoltagePtr_v[*netId_pit]->minVoltage = myPower_p->minVoltage;
 					} else if ( netVoltagePtr_v[*netId_pit]->minVoltage != myPower_p->minVoltage ) {
 						myPowerConflict = true;
 					}
 				}
 				if ( myPower_p->simVoltage != UNKNOWN_VOLTAGE ) {
 					if ( netVoltagePtr_v[*netId_pit]->simVoltage == UNKNOWN_VOLTAGE ) {
 						logFile << "INFO: Override simVoltage for " << NetName(*netId_pit) << " from " << myPower_p->definition << endl;
 						netVoltagePtr_v[*netId_pit]->simVoltage = myPower_p->simVoltage;
 					} else if ( netVoltagePtr_v[*netId_pit]->simVoltage != myPower_p->simVoltage ) {
 						myPowerConflict = true;
 					}
 				}
 				if ( myPower_p->maxVoltage != UNKNOWN_VOLTAGE ) {
 					if ( netVoltagePtr_v[*netId_pit]->maxVoltage == UNKNOWN_VOLTAGE ) {
 						logFile << "INFO: Override maxVoltage for " << NetName(*netId_pit) << " from " << myPower_p->definition << endl;
 						netVoltagePtr_v[*netId_pit]->maxVoltage = myPower_p->maxVoltage;
 					} else if ( netVoltagePtr_v[*netId_pit]->maxVoltage != myPower_p->maxVoltage ) {
 						myPowerConflict = true;
 					}
 				}
 				if ( myPower_p->type != NO_TYPE || myPower_p->family() != "" ) {  // can't override power types or families
 					myPowerConflict = true;
 				}
 				if ( myPowerConflict ) {
 					reportFile << "ERROR: Duplicate power definition " << NetName(*netId_pit) << ": \"";
 					reportFile << netVoltagePtr_v[*netId_pit]->powerSignal() << "(" << netVoltagePtr_v[*netId_pit]->definition;
 					reportFile << ")\" != \"" << myPower_p->powerSignal() << "(" << myPower_p->definition << ")\"" << endl;
 					myPowerError = true;
 				}
 			}
			if ( netId_pit != myNetIdList->begin() ) {
				myPower_p = new CPower(myPower_p, *netId_pit);
				cvcParameters.cvcPowerPtrList.insert(++power_ppit, myPower_p); // insert after current power_ppit (before next node)
				power_ppit--; // set power_ppit to node just inserted
			} else { // add net number to existing power definitions
				myPower_p->netId = *netId_pit;
			}
			if ( ! netVoltagePtr_v[*netId_pit] ) {
				netVoltagePtr_v[*netId_pit] = myPower_p;
			}
			if ( myPower_p->minVoltage != UNKNOWN_VOLTAGE ) netStatus_v[*netId_pit][MIN_POWER] = true;
			if ( myPower_p->maxVoltage != UNKNOWN_VOLTAGE ) netStatus_v[*netId_pit][MAX_POWER] = true;
			if ( myPower_p->simVoltage != UNKNOWN_VOLTAGE ) netStatus_v[*netId_pit][SIM_POWER] = true;
		}
		if ( myNetIdList->empty() ) {
			reportFile << "ERROR: Could not find net " << myPower_p->powerSignal() << endl;
			myPowerError = true;
		}
		power_ppit++;
		delete myNetIdList;
	}
	if ( myPowerError ) {
		reportFile << "Power definition error" << endl;
		return(SKIP);
	} else {
		return(OK);
	}
}

returnCode_t CCvcDb::SetInstancePower() {
	// Instance power definitions
	reportFile << "Setting instance power..." << endl;
	bool myPowerError = false;
	// Instance power definitions
	for ( auto instance_ppit = cvcParameters.cvcInstancePowerPtrList.begin(); instance_ppit != cvcParameters.cvcInstancePowerPtrList.end(); instance_ppit++ ) {
		forward_list<instanceId_t> myInstanceIdList = FindInstanceIds((*instance_ppit)->instanceName);  // expands buses and hierarchy
		for ( auto instanceId_pit = myInstanceIdList.begin(); instanceId_pit != myInstanceIdList.end(); instanceId_pit++ ) {
			CPowerPtrMap myLocalMacroPtrMap;
			for ( auto power_pit = (*instance_ppit)->powerList.begin(); power_pit != (*instance_ppit)->powerList.end(); power_pit++ ) {
				CPower * myPower_p = new CPower(*power_pit, myLocalMacroPtrMap, cvcParameters.cvcModelListMap);
				string myOriginalPower = myPower_p->powerSignal();
				string myNewPower;
				if ( myOriginalPower.substr(0, 1) == "/" ) {
					myNewPower = HierarchyName(*instanceId_pit) + myPower_p->powerSignal();
					myPower_p->extraData->powerSignal = CPower::powerDefinitionText.SetTextAddress((text_t)myNewPower.c_str());
				} else if ( myOriginalPower.substr(0, 2) != "*(" ) {
					myNewPower = HierarchyName(*instanceId_pit) + HIERARCHY_DELIMITER + myPower_p->powerSignal();
					myPower_p->extraData->powerSignal = CPower::powerDefinitionText.SetTextAddress((text_t)myNewPower.c_str());
				}
				set<netId_t> * myNetIdList_p = FindNetIds(string(myPower_p->powerSignal()), *instanceId_pit);
				for ( auto net_pit = myNetIdList_p->begin(); net_pit != myNetIdList_p->end(); net_pit++ ) {
					netId_t myNetId = GetEquivalentNet(*net_pit);
					if ( net_pit != myNetIdList_p->begin() ) {
						myPower_p = new CPower(myPower_p, myNetId);
					} else { // add net number to existing power definitions
						myPower_p->netId = myNetId;
					}
					if ( myPower_p->type[POWER_BIT] && isalpha(myOriginalPower[0]) ) {
						if ( netVoltagePtr_v[myNetId] && netVoltagePtr_v[myNetId]->type[POWER_BIT] ) {  // regular power definitions become macros
							myLocalMacroPtrMap[myOriginalPower] = netVoltagePtr_v[myNetId];
						} else {  // top instance power nets must be already defined
							reportFile << "ERROR: Instance power signal " << myPower_p->powerSignal() << " not defined in parent " << NetName(myNetId) << endl;
							myPowerError = true;
						}
					} else if ( myPower_p->type[INPUT_BIT] ) {
						if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
						if ( myPower_p->minVoltage != UNKNOWN_VOLTAGE ) myPower_p->extraData->expectedMin = to_string<float>(float(myPower_p->minVoltage) / VOLTAGE_SCALE);
						if ( myPower_p->simVoltage != UNKNOWN_VOLTAGE ) myPower_p->extraData->expectedSim = to_string<float>(float(myPower_p->simVoltage) / VOLTAGE_SCALE);
						if ( myPower_p->maxVoltage != UNKNOWN_VOLTAGE ) myPower_p->extraData->expectedMax = to_string<float>(float(myPower_p->maxVoltage) / VOLTAGE_SCALE);
						if ( ! (IsEmpty(myPower_p->expectedMin()) && IsEmpty(myPower_p->expectedSim()) && IsEmpty(myPower_p->expectedMax())) ) {
							myPower_p->minVoltage = UNKNOWN_VOLTAGE;
							myPower_p->simVoltage = UNKNOWN_VOLTAGE;
							myPower_p->maxVoltage = UNKNOWN_VOLTAGE;
							myPower_p->type[INPUT_BIT] = false;
							myPower_p->definition = CPower::powerDefinitionText.SetTextAddress((text_t)myPower_p->StandardDefinition().c_str());
							cvcParameters.cvcExpectedLevelPtrList.push_back(myPower_p);
						}
					} else {
						if ( netVoltagePtr_v[myNetId] ) {
							reportFile << "Warning: Duplicate power definition " << NetName(myNetId);
							reportFile << " and " << myPower_p->powerSignal() << " of " << HierarchyName(*instanceId_pit) << " ignored" << endl;
						} else {
							cvcParameters.cvcPowerPtrList.push_back(myPower_p);
							netVoltagePtr_v[myNetId] = myPower_p;
						}
					}
				}
			}
		}
	}
//	cout << "DEBUG: finished instance power" << endl; cout.flush();
	if ( myPowerError ) {
		reportFile << "Power definition error" << endl;
		return(SKIP);
	} else {
		return(OK);
	}
}

returnCode_t CCvcDb::SetExpectedPower() {
	// Expected voltage definitions
	unordered_map<netId_t, text_t> myExpectedLevelDefinitionMap;
	unordered_map<netId_t, string> myExpectedLevelSignalMap;
	CPowerPtrList::iterator power_ppit = cvcParameters.cvcExpectedLevelPtrList.begin();
	bool myPowerError = false;
	while( power_ppit != cvcParameters.cvcExpectedLevelPtrList.end() ) {
		CPower * myPower_p = *power_ppit;
		set<netId_t> * myNetIdList;
		if ( myPower_p->netId == UNKNOWN_NET ) {
			myNetIdList = FindNetIds(string(myPower_p->powerSignal())); // expands buses and hierarchy
		} else {
			myNetIdList = new set<netId_t>;
			myNetIdList->insert(myPower_p->netId);
		}
		for (auto netId_pit = myNetIdList->begin(); netId_pit != myNetIdList->end(); netId_pit++) {
			string myExpandedNetName = NetName(*netId_pit);
			if ( myPower_p->powerSignal() != myExpandedNetName ) {
//				logFile << "INFO: Expanded " << myPower_p->powerSignal << " -> " << myExpandedNetName << endl;
			}
			if (netVoltagePtr_v[*netId_pit] && myPower_p->definition != netVoltagePtr_v[*netId_pit]->definition ) {
				voltage_t mySimVoltage = netVoltagePtr_v[*netId_pit]->simVoltage;
				if ( myPower_p->expectedSim() == ""
						|| ( IsValidVoltage_(myPower_p->expectedSim()) && mySimVoltage == String_to_Voltage(myPower_p->expectedSim()) ) ) {
					reportFile << "Warning: ignoring power check for " << NetName(*netId_pit) << ": \"";
					reportFile << myPower_p->powerSignal() << "(" << myPower_p->definition << ")\" already set as ";
					reportFile << netVoltagePtr_v[*netId_pit]->powerSignal() << "(" << netVoltagePtr_v[*netId_pit]->definition << ")\"" << endl;
				} else {
					reportFile << "ERROR: Duplicate power definition " << NetName(*netId_pit) << ": \"";
					reportFile << netVoltagePtr_v[*netId_pit]->powerSignal() << "(" << netVoltagePtr_v[*netId_pit]->definition;
					reportFile << ")\" != \"" << myPower_p->powerSignal() << "(" << myPower_p->definition << ")\"" << endl;
					myPowerError = true;
				}
				continue;
			}
			if ( netId_pit != myNetIdList->begin() ) {
				myPower_p = new CPower(myPower_p, *netId_pit);
				cvcParameters.cvcExpectedLevelPtrList.insert(++power_ppit, myPower_p); // insert after current power_ppit (before next node)
				power_ppit--; // set power_ppit to node just inserted
			} else { // add net number to existing power definitions
				myPower_p->netId = *netId_pit;
			}
		}
		if ( myNetIdList->empty() ) {
			reportFile << "ERROR: Could not find net " << myPower_p->powerSignal() << endl;
			myPowerError = true;
		}
		power_ppit++;
		delete myNetIdList;
	}
	if ( myPowerError ) {
		reportFile << "Power definition error" << endl;
		return(SKIP);
	} else {
		return(OK);
	}
}

bool CCvcDb::LockReport(bool theInteractiveFlag) {
	if ( mkdir(cvcParameters.cvcLockFile.c_str(), 0700) != 0 ) {
		cout << "ERROR: Could not create lock file " << cvcParameters.cvcLockFile << endl;
		cout << "report file may be in use by another program." << endl;
		cout << "If no other program is running, remove the lock file and try again." << endl;
		if ( theInteractiveFlag ) {
			cout << "Type \"rerun\" to try again, anything else to skip" << endl;
			string myResponse;
			getline(cin, myResponse);
			if ( myResponse == "rerun" ) {
				cvcArgIndex--;
			}
		}
		lockFile = "";
		return false;
	} else {
		lockFile = cvcParameters.cvcLockFile;
		return true;
	}
}

void CCvcDb::RemoveLock() {
	if ( ! IsEmpty(lockFile) ) {
		int myStatus;
//		string myCommand = "rmdir " + lockFile;
//		cout << myCommand << endl;
//		system(myCommand.c_str());
		myStatus = rmdir(lockFile.c_str());
		if (myStatus) {
			cout << "Remove lock file return value " << myStatus << " error " << errno << endl;
		}
	}
	lockFile = "";
}

void CCvcDb::SetSCRCPower() {
	reportFile << "Setting SCRC power mos nets..." << endl;
	size_t mySCRCSignalCount = 0;
	size_t mySCRCIgnoreCount = 0;
	// Set expected levels for SCRC power mos.
	for ( auto power_ppit = cvcParameters.cvcPowerPtrList.begin(); power_ppit != cvcParameters.cvcPowerPtrList.end(); power_ppit++ ) {
		//if ( IsSCRCPower(*power_ppit) ) {
		if ( (*power_ppit)->type[HIZ_BIT] ) {
			// Check for power switches.
			netId_t myNetId = (*power_ppit)->netId;
			(void) SetSCRCGatePower(myNetId, firstDrain_v, nextDrain_v, sourceNet_v, mySCRCSignalCount, mySCRCIgnoreCount, true);
			(void) SetSCRCGatePower(myNetId, firstSource_v, nextSource_v, drainNet_v, mySCRCSignalCount, mySCRCIgnoreCount, true);
//			for ( auto device_it = firstDrain_v[myNetId]; device_it != UNKNOWN_DEVICE; device_it = nextDrain_v[device_it] ) {
//				if ( ! IsMos_(deviceType_v[device_it]) ) continue;  // Only process mosfets.
//				CPower * mySourcePower_p = netVoltagePtr_v[sourceNet_v[device_it]];
//				if ( mySourcePower_p && mySourcePower_p->type[POWER_BIT] ) {  // Mosfet bridges power nets.
////					bool myExpectHighInput;
////					if ( IsNmos_(deviceType_v[device_it]) ) {
////						myExpectHighInput = false;
////					} else if ( IsPmos_(deviceType_v[device_it]) ) {
////						myExpectHighInput = true;
////					}
//					SetSCRCParentPower(gateNet_v[device_it], IsPmos_(deviceType_v[device_it]), mySCRCSignalCount, mySCRCIgnoreCount);
//
////					netId_t myTargetNet = myNetId;
////					while ( inverterNet_v[myTargetNet] != UNKNOWN_NET ) {
////						myTargetNet = inverterNet_v[myTargetNet];
////						myExpectHighInput = ! myExpectHighInput;
////					}
//				}
//			}
//			for ( auto device_it = firstSource_v[myNetId]; device_it != UNKNOWN_DEVICE; device_it = nextSource_v[device_it] ) {
//				if ( ! IsMos_(deviceType_v[device_it]) ) continue;  // Only process mosfets.
//				CPower * myDrainPower_p = netVoltagePtr_v[drainNet_v[device_it]];
//				if ( myDrainPower_p && myDrainPower_p->type[POWER_BIT] ) {  // Mosfet bridges power nets.
//					SetSCRCParentPower(gateNet_v[device_it], IsPmos_(deviceType_v[device_it]), mySCRCSignalCount, mySCRCIgnoreCount);
//				}
//			}
		}
	}
	reportFile << "Set " << mySCRCSignalCount << " power mos signals." << " Ignored " << mySCRCIgnoreCount << " signals." << endl;
	mySCRCSignalCount = 0;
	mySCRCIgnoreCount = 0;
	reportFile << "Setting SCRC internal nets..." << endl;
	// Set expected levels for SCRC logic.
	for ( size_t net_it = 0; net_it < netCount; net_it++ ) {
		if ( IsSCRCLogicNet(net_it) ) {
			size_t myAttemptCount = 0;
			myAttemptCount += SetSCRCGatePower(net_it, firstDrain_v, nextDrain_v, sourceNet_v, mySCRCSignalCount, mySCRCIgnoreCount, false);
			myAttemptCount += SetSCRCGatePower(net_it, firstSource_v, nextSource_v, drainNet_v, mySCRCSignalCount, mySCRCIgnoreCount, false);
			if ( myAttemptCount == 0 ) {  // Could not set any gate nets, so set net directly
				voltage_t myExpectedVoltage = IsSCRCPower(netVoltagePtr_v[minNet_v[net_it].finalNetId]) ?
						netVoltagePtr_v[maxNet_v[net_it].finalNetId]->maxVoltage : netVoltagePtr_v[minNet_v[net_it].finalNetId]->minVoltage;
				logFile << "Forcing net " << NetName(net_it) << " to " << PrintVoltage(myExpectedVoltage) << endl;
				netVoltagePtr_v[net_it] = new CPower(net_it, myExpectedVoltage, true);
				netVoltagePtr_v[net_it]->extraData->powerSignal = CPower::powerDefinitionText.SetTextAddress(SCRC_FORCED_TEXT);
				cvcParameters.cvcPowerPtrList.push_back(netVoltagePtr_v[net_it]);
				mySCRCSignalCount++;
			}
//			if ( inverterNet_v[net_it] != UNKNOWN_NET ) {
//
//				bool myExpectHighLevel = ( netVoltagePtr_v[minNet_v[net_it].finalNetId]->type[HIZ_BIT] );
//				SetSCRCParentPower(net_it, myExpectHighLevel, mySCRCSignalCount, mySCRCIgnoreCount);
////				netId_t myParentNet = inverterNet_v[net_it];
////				while ( inverterNet_v[myParentNet] != UNKNOWN_NET ) {
////					myParentNet = inverterNet_v[myParentNet];
////					myExpectHighInput = ! myExpectHighInput;
////				}
////				voltage_t myHighVoltage = netVoltagePtr_v[maxNet_v[myParentNet].finalNetId]->simVoltage;
////				voltage_t myLowVoltage = netVoltagePtr_v[minNet_v[myParentNet].finalNetId]->simVoltage;
////				voltage_t myExpectedVoltage = myExpectHighInput ? myHighVoltage : myLowVoltage;
////				if ( myExpectedVoltage == UNKNOWN_VOLTAGE ) {
////					logFile << NetName(myParentNet) << " not set to SCRC voltage." << endl;
////					mySCRCIgnoreCount++;
////				} else if ( netVoltagePtr_v[myParentNet] == NULL ) {
////					logFile << "Setting net " << NetName(myParentNet) << " to " << PrintVoltage(myExpectedVoltage) << endl;
////					netVoltagePtr_v[myParentNet] = new CPower(myParentNet, (myExpectHighInput ? myHighVoltage : myLowVoltage));
////					cvcParameters.cvcPowerPtrList.push_back(netVoltagePtr_v[myParentNet]);
////					mySCRCSignalCount++;
////				} else if ( netVoltagePtr_v[myParentNet]->simVoltage != myExpectedVoltage ) {
////					logFile << NetName(net_it) << ": voltage not set for " << NetName(myParentNet) << " expected " << PrintVoltage(myExpectedVoltage);
////					logFile << " found " << PrintVoltage(netVoltagePtr_v[myParentNet]->simVoltage) << endl;
////				}
//			}
		}
	}
	reportFile << "Set " << mySCRCSignalCount << " inverter signals." << " Ignored " << mySCRCIgnoreCount << " signals." << endl;
}

size_t CCvcDb::SetSCRCGatePower(netId_t theNetId, CDeviceIdVector & theFirstSource_v, CDeviceIdVector & theNextSource_v, CNetIdVector & theDrain_v,
		size_t & theSCRCSignalCount, size_t & theSCRCIgnoreCount, bool theNoCheckFlag) {
	CPower * mySourcePower_p = netVoltagePtr_v[theNetId];
	size_t myAttemptCount = 0;
	for ( auto device_it = theFirstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theNextSource_v[device_it] ) {
		if ( ! IsMos_(deviceType_v[device_it]) ) continue;  // Only process mosfets.
		CPower * myDrainPower_p = netVoltagePtr_v[theDrain_v[device_it]];
		if ( ! myDrainPower_p ) continue;  // ignore non-power nets
		if ( theNoCheckFlag && mySourcePower_p->IsRelatedPower(myDrainPower_p, netVoltagePtr_v, simNet_v, simNet_v, false) ) continue;  // ignore relatives
		if ( myDrainPower_p->type[POWER_BIT] && (theNoCheckFlag || IsSCRCPower(myDrainPower_p)) ) {  // Mosfet bridges power nets.
			SetSCRCParentPower(theNetId, device_it, IsPmos_(deviceType_v[device_it]), theSCRCSignalCount, theSCRCIgnoreCount);
			myAttemptCount++;
		}
	}
	return (myAttemptCount);
}

void CCvcDb::SetSCRCParentPower(netId_t theNetId, deviceId_t theDeviceId, bool theExpectedHighInput, size_t & theSCRCSignalCount, size_t & theSCRCIgnoreCount) {
	netId_t myParentNet = gateNet_v[theDeviceId];
	while ( inverterNet_v[myParentNet] != UNKNOWN_NET && ! (netVoltagePtr_v[myParentNet] && netVoltagePtr_v[myParentNet]->simVoltage != UNKNOWN_VOLTAGE) ) {
		myParentNet = inverterNet_v[myParentNet];
		theExpectedHighInput = ! theExpectedHighInput;
	}
	CPower * myHighPower_p = netVoltagePtr_v[maxNet_v[myParentNet].finalNetId];
	CPower * myLowPower_p = netVoltagePtr_v[minNet_v[myParentNet].finalNetId];
	CPower * myExpectedPower_p = theExpectedHighInput ? myHighPower_p : myLowPower_p;
	voltage_t myExpectedVoltage = UNKNOWN_VOLTAGE;
	if ( myExpectedPower_p ) {
		myExpectedVoltage = theExpectedHighInput ? myExpectedPower_p->maxVoltage : myExpectedPower_p->minVoltage;
	}
	if ( netVoltagePtr_v[myParentNet] && netVoltagePtr_v[myParentNet]->simVoltage != UNKNOWN_VOLTAGE ) {
		if ( myExpectedPower_p && myExpectedVoltage != netVoltagePtr_v[myParentNet]->simVoltage ) {
			logFile << NetName(theNetId) << ": voltage already set for " << NetName(myParentNet) << " expected " << (theExpectedHighInput ? "high" : "low");
			logFile << " found " << netVoltagePtr_v[myParentNet]->simVoltage << endl;
		}
		theSCRCIgnoreCount++;
		return;  // return if voltage already set
	}
	if ( ! myExpectedPower_p ) {
		logFile << NetName(theNetId) << ": voltage not set for " << NetName(myParentNet) << " expected " << (theExpectedHighInput ? "high" : "low");
		logFile << " found non-power" << endl;
		theSCRCIgnoreCount++;
	} else {
		assert(myExpectedPower_p);
		if ( myExpectedVoltage == UNKNOWN_VOLTAGE || myExpectedPower_p->type[HIZ_BIT] ) {
			logFile << NetName(theNetId) << ": voltage not set for " << NetName(myParentNet) << " expected " << (theExpectedHighInput ? "high" : "low");
			logFile << " found " << (myExpectedVoltage == UNKNOWN_VOLTAGE ? "???" : "Hi-Z") << endl;
			theSCRCIgnoreCount++;
		} else if ( netVoltagePtr_v[myParentNet] == NULL ) {
			logFile << "Setting net " << NetName(myParentNet) << " to " << PrintVoltage(myExpectedVoltage) << " for " << NetName(gateNet_v[theDeviceId]) << endl;
			netVoltagePtr_v[myParentNet] = new CPower(myParentNet, myExpectedVoltage, true);
			netVoltagePtr_v[myParentNet]->extraData->powerSignal = CPower::powerDefinitionText.SetTextAddress(SCRC_EXPECTED_TEXT);
			cvcParameters.cvcPowerPtrList.push_back(netVoltagePtr_v[myParentNet]);
			theSCRCSignalCount++;
		} else if ( netVoltagePtr_v[myParentNet]->simVoltage != myExpectedVoltage ) {
			logFile << "ERROR: " << NetName(theNetId) << ": voltage not set for " << NetName(myParentNet) << " expected " << PrintVoltage(myExpectedVoltage);
			logFile << " found " << PrintVoltage(netVoltagePtr_v[myParentNet]->simVoltage) << endl;
			theSCRCIgnoreCount++;
		}
	}
}

bool CCvcDb::IsSCRCLogicNet(netId_t theNetId) {
	CPower * myMinPower = netVoltagePtr_v[minNet_v[theNetId].finalNetId];
	CPower * myMaxPower = netVoltagePtr_v[maxNet_v[theNetId].finalNetId];
	if ( ! myMinPower || ! myMaxPower ) return false;
	if ( myMinPower->type[HIZ_BIT] == myMaxPower->type[HIZ_BIT] ) return false;
	if ( myMinPower->type[MIN_CALCULATED_BIT] || myMaxPower->type[MAX_CALCULATED_BIT] ) return false;
	if ( connectionCount_v[theNetId].sourceDrainType != NMOS_PMOS ) return false;
	if ( IsSCRCPower(myMinPower) || IsSCRCPower(myMaxPower) ) return true;
	return false;
}

bool CCvcDb::IsSCRCPower(CPower * thePower_p) {
	// TODO: change to macro.
	if ( ! thePower_p ) return false;
	if ( ! thePower_p->type[HIZ_BIT] ) return false;
	if ( thePower_p->minVoltage != thePower_p->maxVoltage ) return false;
	return true;
}

#define MAX_LATCH_DEVICE_COUNT 6

bool CCvcDb::SetLatchPower() {
	int myLatchCount = 0;
	for (unsigned int net_it = 0; net_it < simNet_v.size(); net_it++) {
		if ( simNet_v[net_it].finalNetId != net_it ) continue;  // already assigned
		if ( netVoltagePtr_v[net_it] && netVoltagePtr_v[net_it]->simVoltage != UNKNOWN_VOLTAGE ) continue;  // already defined
		if ( connectionCount_v[net_it].sourceDrainType != NMOS_PMOS ) continue;  // not an output net
		if ( connectionCount_v[net_it].SourceDrainCount() > MAX_LATCH_DEVICE_COUNT ) continue;  // only simple connections
		int myNmosCount = 0;
		int myPmosCount = 0;
		netId_t myMinNet = minNet_v[net_it].finalNetId;
		voltage_t myMinVoltage = UNKNOWN_VOLTAGE;
		if ( myMinNet != UNKNOWN_NET && netVoltagePtr_v[myMinNet] && netVoltagePtr_v[myMinNet]->type[POWER_BIT] ) {
			myMinVoltage = netVoltagePtr_v[myMinNet]->minVoltage;
//		} else {
//			myMinNet = UNKNOWN_NET;
		}
		netId_t myMaxNet = maxNet_v[net_it].finalNetId;
		voltage_t myMaxVoltage = UNKNOWN_VOLTAGE;
		if ( myMaxNet != UNKNOWN_NET && netVoltagePtr_v[myMaxNet] && netVoltagePtr_v[myMaxNet]->type[POWER_BIT] ) {
			myMaxVoltage = netVoltagePtr_v[myMaxNet]->maxVoltage;
//		} else {
//			myMaxNet = UNKNOWN_NET;
		}
		if ( myMinVoltage == UNKNOWN_VOLTAGE && myMaxVoltage == UNKNOWN_VOLTAGE ) continue;  // skip unknown min/max output
		if ( myMinVoltage == UNKNOWN_VOLTAGE ) myMinVoltage = myMaxVoltage;
		if ( myMaxVoltage == UNKNOWN_VOLTAGE ) myMaxVoltage = myMinVoltage;
		mosData_t myNmosData_v[MAX_LATCH_DEVICE_COUNT];
		mosData_t myPmosData_v[MAX_LATCH_DEVICE_COUNT];
		FindLatchDevices(net_it, myNmosData_v, myPmosData_v, myNmosCount, myPmosCount, myMinVoltage, myMaxVoltage,
			firstDrain_v, nextDrain_v, sourceNet_v);
		FindLatchDevices(net_it, myNmosData_v, myPmosData_v, myNmosCount, myPmosCount, myMinVoltage, myMaxVoltage,
			firstSource_v, nextSource_v, drainNet_v);
		voltage_t myNmosVoltage = UNKNOWN_VOLTAGE;
		voltage_t myPmosVoltage = UNKNOWN_VOLTAGE;
		string myNmosPowerName;
		text_t myNmosPowerAlias;
		string myPmosPowerName;
		text_t myPmosPowerAlias;
		deviceId_t mySampleNmos = UNKNOWN_DEVICE;
//		deviceId_t mySamplePmos = UNKNOWN_DEVICE;
		for ( int mos_it = 0; mos_it < myNmosCount-1; mos_it++ ) {
			if ( ! netVoltagePtr_v[myNmosData_v[mos_it].source] || netVoltagePtr_v[myNmosData_v[mos_it].source]->simVoltage == UNKNOWN_VOLTAGE ) continue;  // skip non power
			voltage_t myVoltage = netVoltagePtr_v[myNmosData_v[mos_it].source]->simVoltage;
			for ( int nextMos_it = mos_it+1; nextMos_it < myNmosCount; nextMos_it++ ) {
				if ( ! netVoltagePtr_v[myNmosData_v[nextMos_it].source] || netVoltagePtr_v[myNmosData_v[nextMos_it].source]->simVoltage == UNKNOWN_VOLTAGE ) continue;  // skip non power
				voltage_t myNextVoltage = netVoltagePtr_v[myNmosData_v[nextMos_it].source]->simVoltage;
				if ( myVoltage == myNextVoltage && IsOppositeLogic(myNmosData_v[mos_it].gate, myNmosData_v[nextMos_it].gate) ) {  // same source, opposite gate
					myNmosVoltage = myVoltage;
					myNmosPowerName = NetName(myNmosData_v[mos_it].source);
					myNmosPowerAlias = netVoltagePtr_v[myNmosData_v[mos_it].source]->powerAlias();
					mySampleNmos = myNmosData_v[mos_it].id;
				}
			}
		}
		for ( int mos_it = 0; mos_it < myPmosCount-1; mos_it++ ) {
			if ( ! netVoltagePtr_v[myPmosData_v[mos_it].source] || netVoltagePtr_v[myPmosData_v[mos_it].source]->simVoltage == UNKNOWN_VOLTAGE ) continue;  // skip non power
			voltage_t myVoltage = netVoltagePtr_v[myPmosData_v[mos_it].source]->simVoltage;
			for ( int nextMos_it = mos_it+1; nextMos_it < myPmosCount; nextMos_it++ ) {
				if ( ! netVoltagePtr_v[myPmosData_v[nextMos_it].source] || netVoltagePtr_v[myPmosData_v[nextMos_it].source]->simVoltage == UNKNOWN_VOLTAGE ) continue;  // skip non power
				voltage_t myNextVoltage = netVoltagePtr_v[myPmosData_v[nextMos_it].source]->simVoltage;
				if ( myVoltage == myNextVoltage && IsOppositeLogic(myPmosData_v[mos_it].gate, myPmosData_v[nextMos_it].gate) ) {  // same source, opposite gate
					myPmosVoltage = myVoltage;
					myPmosPowerName = NetName(myPmosData_v[mos_it].source);
					myPmosPowerAlias = netVoltagePtr_v[myPmosData_v[mos_it].source]->powerAlias();
//					mySamplePmos = myPmosData_v[mos_it].id;
				}
			}
		}
		if ( myNmosVoltage != UNKNOWN_VOLTAGE && myPmosVoltage != UNKNOWN_VOLTAGE && myNmosVoltage != myPmosVoltage ) {
			errorCount[LEAK]++;
			if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(mySampleNmos) < cvcParameters.cvcCircuitErrorLimit ) {
				CFullConnection myConnections;
				MapDeviceNets(mySampleNmos, myConnections);
				errorFile << "! Short Detected: " << PrintVoltage(myNmosVoltage) << " to " << PrintVoltage(myPmosVoltage) << " at n/pmux" << endl;
				PrintDeviceWithAllConnections(deviceParent_v[mySampleNmos], myConnections, errorFile);
				errorFile << endl;
			}
			continue;  // don't propagate non-matching power
		}
		if ( myNmosVoltage != UNKNOWN_VOLTAGE ) {
			if ( netVoltagePtr_v[net_it] ) {  // just add sim voltage
				assert(netVoltagePtr_v[net_it]->simVoltage == UNKNOWN_VOLTAGE);
				netVoltagePtr_v[net_it]->simVoltage = myNmosVoltage;
			} else {
				netVoltagePtr_v[net_it] = new CPower(net_it, myNmosVoltage, true);
				netVoltagePtr_v[net_it]->extraData->powerSignal = myNmosPowerAlias;
				netVoltagePtr_v[net_it]->extraData->powerAlias = myNmosPowerAlias;
				cvcParameters.cvcPowerPtrList.push_back(netVoltagePtr_v[net_it]);
			}
			myLatchCount++;
			logFile << "Added latch for " << NetName(net_it) << endl;
		} else if ( myPmosVoltage != UNKNOWN_VOLTAGE ) {
			if ( netVoltagePtr_v[net_it] ) {
				assert(netVoltagePtr_v[net_it]->simVoltage == UNKNOWN_VOLTAGE);
				netVoltagePtr_v[net_it]->simVoltage = myPmosVoltage;
			} else {
				netVoltagePtr_v[net_it] = new CPower(net_it, myPmosVoltage, true);
				netVoltagePtr_v[net_it]->extraData->powerSignal = myPmosPowerAlias;
				netVoltagePtr_v[net_it]->extraData->powerAlias = myPmosPowerAlias;
				cvcParameters.cvcPowerPtrList.push_back(netVoltagePtr_v[net_it]);
			}
			myLatchCount++;
			logFile << "Added latch for " << NetName(net_it) << endl;
		}
	}
	reportFile << "Added " << myLatchCount << " latch voltages" << endl;
	return (myLatchCount > 0);
}

void CCvcDb::FindLatchDevices(netId_t theNetId, mosData_t theNmosData_v[], mosData_t thePmosData_v[], int & theNmosCount, int & thePmosCount,
	voltage_t theMinVoltage, voltage_t theMaxVoltage,
	CDeviceIdVector & theFirstDrain_v, CDeviceIdVector & theNextDrain_v, CNetIdVector & theSourceNet_v) {
	for ( deviceId_t device_it = theFirstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theNextDrain_v[device_it] ) {
		if ( theSourceNet_v[device_it] == gateNet_v[device_it] && netVoltagePtr_v[theSourceNet_v[device_it]] ) continue; // skip ESD mos
		netId_t mySource = simNet_v[theSourceNet_v[device_it]].finalNetId;
		netId_t myGate = simNet_v[gateNet_v[device_it]].finalNetId;
		voltage_t myGateVoltage = UNKNOWN_VOLTAGE;
		if ( netVoltagePtr_v[myGate] && ! netVoltagePtr_v[myGate]->type[SIM_CALCULATED_BIT] ) {
			myGateVoltage = netVoltagePtr_v[myGate]->simVoltage;
		}
		if ( IsNmos_(deviceType_v[device_it]) ) {
			if ( myGateVoltage == UNKNOWN_VOLTAGE ) {
				theNmosData_v[theNmosCount].source = mySource;
				theNmosData_v[theNmosCount].gate = myGate;
				theNmosData_v[theNmosCount].id = device_it;
				theNmosCount++;
			} else if ( myGateVoltage >= theMaxVoltage ) {  // device is on (assumes maxVoltage turns on nmos)
				deviceId_t myNextDevice = GetSeriesConnectedDevice(device_it, mySource);
				if ( myNextDevice != UNKNOWN_DEVICE ) {
					// this section uses the instance sourceNet_v
					if ( simNet_v[sourceNet_v[myNextDevice]].finalNetId == mySource ) {
						theNmosData_v[theNmosCount].source = simNet_v[drainNet_v[myNextDevice]].finalNetId;
					} else {
						theNmosData_v[theNmosCount].source = simNet_v[sourceNet_v[myNextDevice]].finalNetId;
					}
					theNmosData_v[theNmosCount].gate = simNet_v[gateNet_v[myNextDevice]].finalNetId;
					theNmosData_v[theNmosCount].id = myNextDevice;
					theNmosCount++;
				}
			}
		}
		if ( IsPmos_(deviceType_v[device_it]) ) {
			if ( myGateVoltage == UNKNOWN_VOLTAGE ) {
				thePmosData_v[thePmosCount].source = mySource;
				thePmosData_v[thePmosCount].gate = myGate;
				thePmosData_v[thePmosCount].id = device_it;
				thePmosCount++;
			} else if ( myGateVoltage <= theMinVoltage ) {  // device is on (assumes minVoltage turns on pmos)
				deviceId_t myNextDevice = GetSeriesConnectedDevice(device_it, mySource);
				if ( myNextDevice != UNKNOWN_DEVICE ) {
					// this section uses the instance sourceNet_v
					if ( simNet_v[sourceNet_v[myNextDevice]].finalNetId == mySource ) {
						thePmosData_v[thePmosCount].source = simNet_v[drainNet_v[myNextDevice]].finalNetId;
					} else {
						thePmosData_v[thePmosCount].source = simNet_v[sourceNet_v[myNextDevice]].finalNetId;
					}
					thePmosData_v[thePmosCount].gate = simNet_v[gateNet_v[myNextDevice]].finalNetId;
					thePmosData_v[thePmosCount].id = myNextDevice;
					thePmosCount++;
				}
			}
		}
	}
}

bool CCvcDb::IsOppositeLogic(netId_t theFirstNet, netId_t theSecondNet) {
	return (inverterNet_v[theFirstNet] == theSecondNet || theFirstNet == inverterNet_v[theSecondNet]);
}

