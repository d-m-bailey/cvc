/*
 * CCvcDb_print.cc
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

void CCvcDb::SetOutputFiles(string theReportFilename) {
	ifstream myTestFile;
	string myBackupFilename, myBaseFilename;
	string myCompressedFilename, myBaseCompressedFilename;
	string mySystemCommand;
	myBaseFilename = theReportFilename.substr(0, theReportFilename.find(".log"));
	cvcParameters.cvcReportBaseFilename = myBaseFilename;
	int myFileSuffix = 0;
	myBackupFilename = theReportFilename;
	myCompressedFilename = myBackupFilename + ".gz";
	while ( ( myTestFile.open(myBackupFilename), myTestFile.good() ) || ( myTestFile.open(myCompressedFilename), myTestFile.good() ) ) {
		myTestFile.close();
		myBackupFilename = theReportFilename + "." + to_string<int>(++myFileSuffix);
		myCompressedFilename = myBackupFilename + ".gz";
	}
	if ( myFileSuffix > 0 ) {
		cout << "INFO: Moving " << theReportFilename << " to " << myCompressedFilename << endl;
		assert(rename(theReportFilename.c_str(), myBackupFilename.c_str()) == 0);
		mySystemCommand = "gzip -f " + myBackupFilename;
		assert(system(mySystemCommand.c_str()) == 0);

		myBaseFilename = cvcParameters.cvcReportBaseFilename + ".error";
		myBaseCompressedFilename = myBaseFilename + ".gz";
		myBackupFilename = myBaseFilename + "." + to_string<int>(myFileSuffix);
		myCompressedFilename = myBackupFilename + ".gz";
		if ( ( myTestFile.open(myBackupFilename), myTestFile.good() ) || ( myTestFile.open(myCompressedFilename), myTestFile.good() ) ) {
			myTestFile.close();
			cout << "INFO: Removing " << myBackupFilename << endl;
			assert((remove(myBackupFilename.c_str()) == 0) || (remove(myCompressedFilename.c_str()) == 0));
		}
		if ( myTestFile.open(myBaseFilename), myTestFile.good() ) {
			myTestFile.close();
			cout << "INFO: Moving " << myBaseFilename << " to " << myCompressedFilename << endl;
			assert(rename(myBaseFilename.c_str(), myBackupFilename.c_str()) == 0);
			mySystemCommand = "gzip -f " + myBackupFilename;
			assert(system(mySystemCommand.c_str()) == 0);
		} else if ( myTestFile.open(myBaseCompressedFilename), myTestFile.good() ) {
			myTestFile.close();
			cout << "INFO: Moving " << myBaseCompressedFilename << " to " << myCompressedFilename << endl;
			assert(rename(myBaseCompressedFilename.c_str(), myCompressedFilename.c_str()) == 0);
		}

		myBaseFilename = cvcParameters.cvcReportBaseFilename + ".shorts";
		myBaseCompressedFilename = myBaseFilename + ".gz";
		myBackupFilename = myBaseFilename + "." + to_string<int>(myFileSuffix);
		myCompressedFilename = myBackupFilename + ".gz";
		if ( ( myTestFile.open(myBackupFilename), myTestFile.good() ) || ( myTestFile.open(myCompressedFilename), myTestFile.good() ) ) {
			myTestFile.close();
			cout << "INFO: Removing " << myBackupFilename << endl;
			assert((remove(myBackupFilename.c_str()) == 0) || (remove(myCompressedFilename.c_str()) == 0));
		}
		if ( myTestFile.open(myBaseFilename), myTestFile.good() ) {
			myTestFile.close();
			cout << "INFO: Moving " << myBaseFilename << " to " << myCompressedFilename << endl;
			assert(rename(myBaseFilename.c_str(), myBackupFilename.c_str()) == 0);
			mySystemCommand = "gzip -f " + myBackupFilename;
			assert(system(mySystemCommand.c_str()) == 0);
		} else if ( myTestFile.open(myBaseCompressedFilename), myTestFile.good() ) {
			myTestFile.close();
			cout << "INFO: Moving " << myBaseCompressedFilename << " to " << myCompressedFilename << endl;
			assert(rename(myBaseCompressedFilename.c_str(), myCompressedFilename.c_str()) == 0);
		}
	} else {
		myBaseFilename = cvcParameters.cvcReportBaseFilename + ".error";
		myBaseCompressedFilename = myBaseFilename + ".gz";
		if ( myTestFile.open(myBaseFilename), myTestFile.good() ) {
			myTestFile.close();
			cout << "INFO: Removing " << myBaseFilename << endl;
			remove(myBaseFilename.c_str());
		}
		if ( myTestFile.open(myBaseCompressedFilename), myTestFile.good() ) {
			myTestFile.close();
			cout << "INFO: Removing " << myBaseCompressedFilename << endl;
			remove(myBaseCompressedFilename.c_str());
		}
		myBaseFilename = cvcParameters.cvcReportBaseFilename + ".shorts";
		myBaseCompressedFilename = myBaseFilename + ".gz";
		if ( myTestFile.open(myBaseFilename), myTestFile.good() ) {
			myTestFile.close();
			cout << "INFO: Removing " << myBaseFilename << endl;
			remove(myBaseFilename.c_str());
		}
		if ( myTestFile.open(myBaseCompressedFilename), myTestFile.good() ) {
			myTestFile.close();
			cout << "INFO: Removing " << myBaseCompressedFilename << endl;
			remove(myBaseCompressedFilename.c_str());
		}
	}
	cout << "DEBUG: report file '" << theReportFilename << "'" << endl;
	logFile.open(theReportFilename);
	if ( ! logFile.good() ) {
		throw EFatalError("Could not open " + theReportFilename);
	}
	errorFile.open(cvcParameters.cvcReportBaseFilename + ".error.gz");
	if ( ! errorFile.good() ) {
		throw EFatalError("Could not open " + cvcParameters.cvcReportBaseFilename + ".error.gz");
	}
	debugFile.open(cvcParameters.cvcReportBaseFilename + ".debug.gz");
	if ( ! debugFile.good() ) {
		throw EFatalError("Could not open " + cvcParameters.cvcReportBaseFilename + ".debug.gz");
	}

	reportFile << "CVC: Log output to " << theReportFilename << endl;
	reportFile << "CVC: Error output to " << cvcParameters.cvcReportBaseFilename << ".error.gz" << endl;
	reportFile << "CVC: Debug output to " << cvcParameters.cvcReportBaseFilename << ".debug.gz" << endl;
//	reportFile << "CVC: Short output to " << cvcParameters.cvcReportBaseFilename << ".shorts.gz" << endl;
}

string CCvcDb::NetAlias(netId_t theNetId, bool thePrintCircuitFlag) {
	string myAlias = "";
	CPower * myPower_p = netVoltagePtr_v[theNetId];
	if ( myPower_p && ! IsEmpty(myPower_p->powerAlias()) && myPower_p->powerAlias() != myPower_p->powerSignal() ) myAlias = "~>" + string(myPower_p->powerAlias());
	return (thePrintCircuitFlag) ? myAlias : "";
}

string CCvcDb::NetName(CPower * thePowerPtr, bool thePrintCircuitFlag, bool thePrintHierarchyFlag) {
	netId_t myNetId;
	if ( thePowerPtr->netId != UNKNOWN_NET ) {
		myNetId = thePowerPtr->netId;
	} else {
		return "unknown";
	}
	instanceId_t myParentId = netParent_v[myNetId];
	if ( myParentId == 0 && myNetId < instancePtr_v[myParentId]->master_p->portCount ) {
		return instancePtr_v[myParentId]->master_p->internalSignal_v[myNetId] + NetAlias(myNetId, thePrintCircuitFlag);
	} else {
		netId_t myNetOffset = myNetId - instancePtr_v[myParentId]->firstNetId;
		return(HierarchyName(myParentId, thePrintCircuitFlag, thePrintHierarchyFlag) + "/" + instancePtr_v[myParentId]->master_p->internalSignal_v[myNetOffset]) + NetAlias(myNetId, thePrintCircuitFlag);
	}
}

string CCvcDb::NetName(const netId_t theNetId, bool thePrintCircuitFlag, bool thePrintHierarchyFlag) {
	if ( theNetId == UNKNOWN_NET ) return "unknown";
	instanceId_t myParentId = netParent_v[theNetId];
	if ( myParentId == 0 && theNetId < instancePtr_v[myParentId]->master_p->portCount ) {
		return instancePtr_v[myParentId]->master_p->internalSignal_v[theNetId] + NetAlias(theNetId, thePrintCircuitFlag);
	} else {
		netId_t myNetOffset = theNetId - instancePtr_v[myParentId]->firstNetId;
		return(HierarchyName(myParentId, thePrintCircuitFlag, thePrintHierarchyFlag) + "/" + instancePtr_v[myParentId]->master_p->internalSignal_v[myNetOffset]) + NetAlias(theNetId, thePrintCircuitFlag);
	}
}

string CCvcDb::HierarchyName(const instanceId_t theInstanceId, bool thePrintCircuitFlag, bool thePrintHierarchyFlag) {
	if ( theInstanceId == 0 ) {
		// top circuit
		return "";
	} else {
		instanceId_t myParentId = instancePtr_v[theInstanceId]->parentId;
		instanceId_t mySubcircuitOffset = theInstanceId - instancePtr_v[myParentId]->firstSubcircuitId;
		string myCircuitName = "";
		if ( thePrintCircuitFlag ) {
			myCircuitName = "(" + string(instancePtr_v[myParentId]->master_p->subcircuitPtr_v[mySubcircuitOffset]->masterName) + ")";
			if ( ! thePrintHierarchyFlag ) return ( myCircuitName );
		}
		return(HierarchyName(myParentId, thePrintCircuitFlag, thePrintHierarchyFlag) + "/" + instancePtr_v[myParentId]->master_p->subcircuitPtr_v[mySubcircuitOffset]->name + myCircuitName);
	}
}

string CCvcDb::DeviceName(const deviceId_t theDeviceId, bool thePrintCircuitFlag, bool thePrintHierarchyFlag) {
	instanceId_t myParentId = deviceParent_v[theDeviceId];
	deviceId_t myDeviceOffset = theDeviceId - instancePtr_v[myParentId]->firstDeviceId;
	return(HierarchyName(myParentId, thePrintCircuitFlag, thePrintHierarchyFlag) + "/" + instancePtr_v[myParentId]->master_p->devicePtr_v[myDeviceOffset]->name);
}

string CCvcDb::DeviceName(string theName, const instanceId_t theParentId, bool thePrintCircuitFlag, bool thePrintHierarchyFlag) {
	return(HierarchyName(theParentId, thePrintCircuitFlag, thePrintHierarchyFlag) + "/" + theName);
}

void CCvcDb::PrintEquivalentNets(string theIndentation) {
	string myIndentation = theIndentation + " ";
	cout << theIndentation << "EquivalentNets> start" << endl;
	for (netId_t net_it = 0; net_it < netCount; net_it++) {
		if ( net_it != equivalentNet_v[net_it] ) cout << myIndentation << net_it << "(" << equivalentNet_v[net_it] << ")" << endl;
	}
	cout << theIndentation << "EquivalentNets> end" << endl;
}

void CCvcDb::PrintInverterNets(string theIndentation) {
	string myIndentation = theIndentation + " ";
	cout << theIndentation << "InverterNets> start" << endl;
	for (netId_t net_it = 0; net_it < netCount; net_it++) {
		if ( inverterNet_v[net_it] != UNKNOWN_NET ) cout << myIndentation << "in " << inverterNet_v[net_it] << ((highLow_v[net_it]) ? " ->" : " =>") << " out " << net_it << endl;
	}
	cout << theIndentation << "InverterNets> end" << endl;
}

void CCvcDb::PrintFlatCdl() {
	PrintNewCdlLine(string(".SUBCKT"));
	PrintCdlLine(topCircuit_p->name);
	for (netId_t net_it = 0; net_it < topCircuit_p->portCount; net_it++) {
		PrintCdlLine(topCircuit_p->internalSignal_v[net_it]);
	}

	CCircuit * myMaster_p;
	for (instanceId_t instance_it = 0; instance_it < instancePtr_v.size(); instance_it++) {
		if ( instancePtr_v[instance_it]->IsParallelInstance() ) continue;
		myMaster_p = instancePtr_v[instance_it]->master_p;
		for (deviceId_t device_it = 0; device_it < myMaster_p->devicePtr_v.size(); device_it++) {
			PrintNewCdlLine(myMaster_p->devicePtr_v[device_it]->parameters[0] + DeviceName(instancePtr_v[instance_it]->firstDeviceId + device_it, PRINT_CIRCUIT_OFF));
			for (netId_t net_it = 0; net_it < myMaster_p->devicePtr_v[device_it]->signalId_v.size(); net_it++) {
				PrintCdlLine(NetName(instancePtr_v[instance_it]->localToGlobalNetId_v[myMaster_p->devicePtr_v[device_it]->signalId_v[net_it]]));
			}
			PrintCdlLine(myMaster_p->devicePtr_v[device_it]->parameters + 2);  // skip first 2 characters of parameter string
			if ( instancePtr_v[instance_it]->parallelInstanceCount > 1 ) {
				PrintCdlLine("M=" + to_string<instanceId_t> (instancePtr_v[instance_it]->parallelInstanceCount));
			}
			PrintCdlLine(string(""));
		}
	}
	PrintNewCdlLine(string(".ENDS"));
	PrintNewCdlLine(string(""));
}

void CCvcDb::PrintHierarchicalCdl(CCircuit *theCircuit, unordered_set<text_t> & thePrintedList, ostream & theCdlFile) {
	for (instanceId_t instance_it = 0; instance_it < theCircuit->subcircuitPtr_v.size(); instance_it++) {
		CCircuit * myMaster_p = theCircuit->subcircuitPtr_v[instance_it]->master_p;
		if (thePrintedList.count(myMaster_p->name) == 0) {
			thePrintedList.insert(myMaster_p->name);
			PrintHierarchicalCdl(myMaster_p, thePrintedList, theCdlFile);
		}
	}

	PrintNewCdlLine(string(".SUBCKT"), theCdlFile);
	PrintCdlLine(theCircuit->name, theCdlFile);
	vector<text_t> mySignals_v;
	mySignals_v.reserve(theCircuit->localSignalIdMap.size());
	for (auto signal_net_pair_pit = theCircuit->localSignalIdMap.begin(); signal_net_pair_pit != theCircuit->localSignalIdMap.end(); signal_net_pair_pit++) {
		mySignals_v[signal_net_pair_pit->second] = signal_net_pair_pit->first;
	}
	for (netId_t net_it = 0; net_it < theCircuit->portCount; net_it++) {
		PrintCdlLine(mySignals_v[net_it], theCdlFile);
	}
	for (deviceId_t device_it = 0; device_it < theCircuit->devicePtr_v.size(); device_it++) {
		PrintNewCdlLine(theCircuit->devicePtr_v[device_it]->name, theCdlFile);
		string myDeviceType = theCircuit->devicePtr_v[device_it]->model_p->baseType;
		if ( myDeviceType == "R" || myDeviceType == "C" ) {
			for (netId_t net_it = 0; net_it < 2; net_it++) {
				PrintCdlLine(mySignals_v[theCircuit->devicePtr_v[device_it]->signalId_v[net_it]], theCdlFile);
			}
		} else {
			for (netId_t net_it = 0; net_it < theCircuit->devicePtr_v[device_it]->signalId_v.size(); net_it++) {
				PrintCdlLine(mySignals_v[theCircuit->devicePtr_v[device_it]->signalId_v[net_it]], theCdlFile);
			}
		}
		PrintCdlLine(theCircuit->devicePtr_v[device_it]->parameters + 2, theCdlFile);  // The first 2 characters of the parameters are device type code. ignore them.
/*
		if ( theCircuit->devicePtr_v[device_it]->model_p->type == NRESISTOR || theCircuit->devicePtr_v[device_it]->model_p->type == PRESISTOR ) {
			PrintCdlLine(string("$SUB=") + mySignals_v[theCircuit->devicePtr_v[device_it]->signalId_v[2]], theCdlFile);
		}
*/
		if ( (myDeviceType == "R" || myDeviceType == "C") && theCircuit->devicePtr_v[device_it]->signalId_v.size() == 3 ) {
			PrintCdlLine(string("$SUB=") + mySignals_v[theCircuit->devicePtr_v[device_it]->signalId_v[2]], theCdlFile);
		}
		PrintCdlLine(string(""), theCdlFile);
	}
	for (instanceId_t instance_it = 0; instance_it < theCircuit->subcircuitPtr_v.size(); instance_it++) {
		PrintNewCdlLine(theCircuit->subcircuitPtr_v[instance_it]->name, theCdlFile);
		for (netId_t net_it = 0; net_it < theCircuit->subcircuitPtr_v[instance_it]->signalId_v.size(); net_it++) {
			PrintCdlLine(mySignals_v[theCircuit->subcircuitPtr_v[instance_it]->signalId_v[net_it]], theCdlFile);
		}
		PrintCdlLine(theCircuit->subcircuitPtr_v[instance_it]->masterName, theCdlFile);
		PrintCdlLine(string(""), theCdlFile);
	}
	PrintNewCdlLine(string(".ENDS"), theCdlFile);
	PrintNewCdlLine(string(" "), theCdlFile);
}

void CCvcDb::PrintCdlLine(const string theData, ostream & theOutput, const unsigned int theMaxLength) {
	if (lineLength + theData.length() > theMaxLength) {
		theOutput << endl << "+";
		lineLength = 1;
	}
	theOutput << " " << theData;
	lineLength += theData.length() + 1;
}

void CCvcDb::PrintNewCdlLine(const string theData, ostream & theOutput) {
	if ( lineLength > 0) {
		theOutput << endl;
	}
	theOutput << theData;
	lineLength = theData.length();
}

void CCvcDb::PrintCdlLine(const char * theData, ostream & theOutput, const unsigned int theMaxLength) {
	if (lineLength + strlen(theData) > theMaxLength) {
		theOutput << endl << "+";
		lineLength = 1;
	}
	theOutput << " " << theData;
	lineLength += strlen(theData);
}

void CCvcDb::PrintNewCdlLine(const text_t theData, ostream & theOutput) {
	if ( lineLength > 0) {
		theOutput << endl;
	}
	theOutput << theData;
	lineLength = strlen(theData);
}

void CCvcDb::PrintNewCdlLine(const char theData, ostream & theOutput) {
	if ( lineLength > 0) {
		theOutput << endl;
	}
	theOutput << theData;
	lineLength = 1;
}

void CCvcDb::PrintSourceDrainConnections(CStatus& theConnectionStatus, string theIndentation) {
	cout << theIndentation << "SourceConnectionsTo:";
	if ( theConnectionStatus[NMOS] ) cout << " NMOS";
	if ( theConnectionStatus[PMOS] ) cout << " PMOS";
	if ( theConnectionStatus[RESISTOR] ) cout << " RESISTOR";
	if ( theConnectionStatus[FUSE_ON] ) cout << " FUSE_ON";
	if ( theConnectionStatus[FUSE_OFF] ) cout << " FUSE_OFF";
	cout << endl;
}

void CCvcDb::PrintConnections(deviceId_t theDeviceCount, deviceId_t theDeviceId, CDeviceIdVector& theNextDevice_v, string theIndentation, string theHeading) {
	cout << theIndentation << theHeading << "(" << theDeviceCount << ")>";
	while (theDeviceId != UNKNOWN_DEVICE ) {
		cout << " " << theDeviceId;
		theDeviceId = theNextDevice_v[theDeviceId];
	}
	cout << endl;
}

void CCvcDb::PrintBulkConnections(netId_t theNetId, string theIndentation, string theHeading) {
	// debug function. not optimized.
	deviceId_t myDeviceCount = 0;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		if ( bulkNet_v[device_it] == theNetId ) {
			myDeviceCount++;
		}
	}
	cout << theIndentation << theHeading << "(" << myDeviceCount << ")>";
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		if ( bulkNet_v[device_it] == theNetId ) {
			cout << " " << device_it;
		}
	}
	cout << endl;
}

string CCvcDb::StatusString(const CStatus& theStatus) {
	string myStatus = "";
	for (size_t bit_it = 0; bit_it < theStatus.size(); bit_it++) {
		myStatus += (theStatus[bit_it]) ? "1" : "0";
	}
	return myStatus;
}

void CCvcDb::PrintPowerList(ostream & theOutputFile, string theHeading, string theIndentation) {
	theOutputFile << theIndentation << theHeading << "> filename " << cvcParameters.cvcPowerFilename << endl;
	string myRealPowerName;
	text_t myLastPowerSignal = NULL;
	for (CPowerPtrList::iterator power_ppit = cvcParameters.cvcPowerPtrList.begin(); power_ppit != cvcParameters.cvcPowerPtrList.end(); power_ppit++) {
		if ( IsEmpty((*power_ppit)->powerSignal()) ) continue; // skip resistor definitions
		myRealPowerName = NetName((*power_ppit)->netId);
		if ( myLastPowerSignal != (*power_ppit)->powerSignal() && myRealPowerName != string((*power_ppit)->powerSignal()) ) {
			string myAlias = "";
			if ( ! IsEmpty((*power_ppit)->powerAlias()) && string((*power_ppit)->powerSignal()) != (*power_ppit)->powerAlias() ) {
				myAlias = ALIAS_DELIMITER + string((*power_ppit)->powerAlias());
			}
			theOutputFile << (*power_ppit)->powerSignal() << myAlias << (*power_ppit)->definition << endl;
		}
		myLastPowerSignal = (*power_ppit)->powerSignal();
		(*power_ppit)->Print(theOutputFile, theIndentation + " ", myRealPowerName);
	}
	myLastPowerSignal = NULL;
	theOutputFile << theIndentation << "> expected values" << endl;
	for (auto power_ppit = cvcParameters.cvcExpectedLevelPtrList.begin(); power_ppit != cvcParameters.cvcExpectedLevelPtrList.end(); power_ppit++) {
		myRealPowerName = NetName((*power_ppit)->netId);
		if ( myLastPowerSignal != (*power_ppit)->powerSignal() && myRealPowerName != string((*power_ppit)->powerSignal()) ) {
			string myAlias = "";
			if ( ! IsEmpty((*power_ppit)->powerAlias()) && (*power_ppit)->powerSignal() != (*power_ppit)->powerAlias() ) {
				myAlias = ALIAS_DELIMITER + string((*power_ppit)->powerAlias());
			}
			theOutputFile << (*power_ppit)->powerSignal() << myAlias << (*power_ppit)->definition << endl;
		}
		myLastPowerSignal = (*power_ppit)->powerSignal();
		(*power_ppit)->Print(theOutputFile, theIndentation + " ", myRealPowerName);
	}
	theOutputFile << theIndentation << "> macros" << endl;
	for (auto powerMap_pit = cvcParameters.cvcPowerMacroPtrMap.begin(); powerMap_pit != cvcParameters.cvcPowerMacroPtrMap.end(); powerMap_pit++) {
		netId_t myNetId = FindNet(0, powerMap_pit->first, false);
//		if (myNetId == UNKNOWN_NET || ! netVoltagePtr_v[myNetId]) { // power definition does not exist, therefore this is macro
		if (myNetId == UNKNOWN_NET) { // power definition does not exist, therefore this is macro
			theOutputFile << theIndentation << " #define";
			powerMap_pit->second->Print(theOutputFile, theIndentation + " ", powerMap_pit->first);
		}
	}
	theOutputFile << theIndentation << theHeading << "> end" << endl << endl;
}

void CCvcDb::Print(const string theIndentation, const string theHeading) {
	string myIndentation = theIndentation + " ";

	cout << endl << theIndentation << theHeading << " start" << endl;
	cout << myIndentation << "Counts> Devices: " << deviceCount;
	cout << "  Subcircuits: " << subcircuitCount;
	cout << "  Nets: " << netCount << endl;
	cout << myIndentation << "InstanceList> start" << endl;
	for (instanceId_t instance_it = 0; instance_it < subcircuitCount; instance_it++) {
		instancePtr_v[instance_it]->Print(instance_it, myIndentation + " ");
	}
	cout << myIndentation << "InstanceList> end" << endl << endl;
	cout << myIndentation << "NetList> start" << endl;
	vector<deviceId_t> myBulkCount;
	ResetVector<vector<deviceId_t>>(myBulkCount, netCount, 0);
	for (deviceId_t device_it = 0; device_it < deviceCount; device_it++) {
		if ( bulkNet_v[device_it] == UNKNOWN_DEVICE ) continue;
		myBulkCount[bulkNet_v[device_it]]++;
	}
	for (netId_t net_it = 0; net_it < netCount; net_it++) {
		cout << myIndentation + " Net " << net_it << ":" << NetName(net_it) << endl;
		if ( netVoltagePtr_v[net_it] ) {
			netVoltagePtr_v[net_it]->Print(cout, myIndentation + "  ");
//			if ( IsCalculatedVoltage_(netVoltagePtr_v[net_it]) ) {
//				netVoltagePtr_v[net_it]->Print(cout, myIndentation + "  =>");
//			} else {
//				netVoltagePtr_v[net_it]->Print(cout, myIndentation + "  ");
//			}
		}
		if ( firstSource_v[net_it] != UNKNOWN_DEVICE ) PrintConnections(connectionCount_v[net_it].sourceCount, firstSource_v[net_it], nextSource_v, myIndentation + "  ", "SourceConnections");
		if ( firstDrain_v[net_it] != UNKNOWN_DEVICE ) PrintConnections(connectionCount_v[net_it].drainCount, firstDrain_v[net_it], nextDrain_v, myIndentation + "  ", "DrainConnections");
		if ( firstGate_v[net_it] != UNKNOWN_DEVICE ) PrintConnections(connectionCount_v[net_it].gateCount, firstGate_v[net_it], nextGate_v, myIndentation + "  ", "GateConnections");
		if ( myBulkCount[net_it] > 0 ) PrintBulkConnections(net_it, myIndentation + "  ", "BulkConnections");
//		if ( firstBulk_v[net_it] != UNKNOWN_DEVICE ) PrintConnections(connectionCount_v[net_it].bulkCount, firstBulk_v[net_it], nextBulk_v, myIndentation + "  ", "BulkConnections");
		if ( connectionCount_v[net_it].sourceCount + connectionCount_v[net_it].drainCount > 0 ) {
			PrintSourceDrainConnections(connectionCount_v[net_it].sourceDrainType, myIndentation + "  ");
		}
	}
	cout << myIndentation << "NetList> end" << endl << endl;
	cout << myIndentation << "DeviceList> start" << endl;
	for (deviceId_t device_it = 0; device_it < deviceCount; device_it++) {
		cout << myIndentation + " Device " << device_it << ":";
		cout << gModelTypeMap[deviceType_v[device_it]] << " ";
		cout << DeviceName(device_it, PRINT_CIRCUIT_ON) << "@" << parameterResistanceMap[DeviceParameters(device_it)];
		cout << " Status: " << StatusString(deviceStatus_v[device_it]) << endl;
	}
	cout << myIndentation << "DeviceList> end" << endl << endl;
	PrintPowerList(cout, "PowerList", myIndentation);
/*
	cout << myIndentation << "PowerList> start" << endl;
	string myRealPowerName;
	for (CPowerPtrList::iterator power_ppit = cvcParameters.cvcPowerPtrList.begin(); power_ppit != cvcParameters.cvcPowerPtrList.end(); power_ppit++) {
		myRealPowerName = NetName((*power_ppit)->baseNetId);
		(*power_ppit)->Print(myIndentation + " ", myRealPowerName);
	}
	cout << myIndentation << "PowerList> end" << endl << endl;
*/

	PrintEquivalentNets(myIndentation);
	PrintInverterNets(myIndentation);

	cout << theIndentation << theHeading << " end" << endl << endl;
}

void CCvcDb::PrintVirtualNets(CVirtualNetVector& theVirtualNet_v, string theTitle, string theIndentation) {
	cout << theIndentation << "VirtualNetVector" << theTitle << "> start " << theVirtualNet_v.size() << endl;
	netId_t myEquivalentNet;
	for ( netId_t net_it = 0; net_it < theVirtualNet_v.size(); net_it++ ) {
		myEquivalentNet = equivalentNet_v[theVirtualNet_v[net_it].nextNetId];
		if ( netVoltagePtr_v[myEquivalentNet] && netVoltagePtr_v[myEquivalentNet]->netId != UNKNOWN_NET ) {
			myEquivalentNet = netVoltagePtr_v[myEquivalentNet]->netId;
		}
		cout << theIndentation + "  ";
		if ( net_it == myEquivalentNet ) {
			if ( netVoltagePtr_v[net_it] ) {
				cout << net_it << "=>";
				if ( netVoltagePtr_v[net_it]->minVoltage != UNKNOWN_VOLTAGE ) {
					cout << netVoltagePtr_v[net_it]->minVoltage;
				}
				cout << "/";
				if ( netVoltagePtr_v[net_it]->simVoltage != UNKNOWN_VOLTAGE ) {
					cout << netVoltagePtr_v[net_it]->simVoltage;
				}
				cout << "/";
				if ( netVoltagePtr_v[net_it]->maxVoltage != UNKNOWN_VOLTAGE ) {
					cout << netVoltagePtr_v[net_it]->maxVoltage;
				}
				cout << endl;
			} else {
				cout << net_it << "??" << endl;
			}
		} else {
			cout << net_it << "->" << myEquivalentNet << "@" << theVirtualNet_v[net_it].resistance << endl;
		}
	}
	cout << theIndentation << "VirtualNetVector" << theTitle << "> end" << endl;
}

void CCvcDb::PrintDeviceWithAllConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile, bool theIncludeLeakVoltage) {
	int myMFactor = CalculateMFactor(theParentId);
	theErrorFile << DeviceName(theConnections.device_p->name, theParentId, PRINT_CIRCUIT_ON) << " " << theConnections.device_p->parameters;
	if ( myMFactor > 1 ) theErrorFile << " {m=" << myMFactor << "}";
	theErrorFile << " (r=" << parameterResistanceMap[theConnections.device_p->parameters] << ")" << endl;
	switch ( theConnections.device_p->model_p->type ) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			PrintAllTerminalConnections(GATE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintAllTerminalConnections(SOURCE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintAllTerminalConnections(DRAIN, theConnections, theErrorFile, theIncludeLeakVoltage);
			if ( ! cvcParameters.cvcSOI ) PrintAllTerminalConnections(BULK, theConnections, theErrorFile, theIncludeLeakVoltage);
		break; }
		case RESISTOR: case CAPACITOR: case DIODE: case FUSE_OFF: case FUSE_ON: case SWITCH_ON: case SWITCH_OFF: {
			PrintAllTerminalConnections(SOURCE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintAllTerminalConnections(DRAIN, theConnections, theErrorFile, theIncludeLeakVoltage);
			if ( theConnections.originalBulkId != UNKNOWN_NET) {
				PrintAllTerminalConnections(BULK, theConnections, theErrorFile, theIncludeLeakVoltage);
			}
		break; }
		case BIPOLAR: {
			PrintAllTerminalConnections(SOURCE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintAllTerminalConnections(GATE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintAllTerminalConnections(DRAIN, theConnections, theErrorFile, theIncludeLeakVoltage);
		break; }
		default: {
			throw EDatabaseError("Invalid device type: " + theConnections.device_p->model_p->type);
		}
	}
}

/*
void CCvcDb::PrintDeviceWithMinMaxConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile, bool theIncludeLeakVoltage) {
	errorFile << DeviceName(theConnections.device_p->name, theParentId, PRINT_CIRCUIT_ON) << " " << theConnections.device_p->parameters;
	errorFile << " (r=" << parameterResistanceMap[theConnections.device_p->parameters] << ")" << endl;
	switch ( theConnections.device_p->model_p->type ) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			PrintMinMaxTerminalConnections(GATE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintMinMaxTerminalConnections(SOURCE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintMinMaxTerminalConnections(DRAIN, theConnections, theErrorFile, theIncludeLeakVoltage);
			if ( ! cvcParameters.cvcSOI ) PrintMinMaxTerminalConnections(BULK, theConnections, theErrorFile, theIncludeLeakVoltage);
		break; }
		case RESISTOR: case CAPACITOR: case DIODE: case FUSE_OFF: case FUSE_ON: case SWITCH_ON: case SWITCH_OFF: {
			PrintMinMaxTerminalConnections(SOURCE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintMinMaxTerminalConnections(DRAIN, theConnections, theErrorFile, theIncludeLeakVoltage);
			if ( theConnections.originalBulkId != UNKNOWN_NET) {
				PrintMinMaxTerminalConnections(BULK, theConnections, theErrorFile);
			}
		break; }
		case BIPOLAR: {
			PrintMinMaxTerminalConnections(SOURCE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintMinMaxTerminalConnections(GATE, theConnections, theErrorFile, theIncludeLeakVoltage);
			PrintMinMaxTerminalConnections(DRAIN, theConnections, theErrorFile, theIncludeLeakVoltage);
		break; }
		default: {
			throw EDatabaseError("Invalid device type: " + theConnections.device_p->model_p->type);
		}
	}
}

void CCvcDb::PrintDeviceWithMinSimConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile) {
	errorFile << DeviceName(theConnections.device_p->name, theParentId, PRINT_CIRCUIT_ON) << " " << theConnections.device_p->parameters;
	errorFile << " (r=" << parameterResistanceMap[theConnections.device_p->parameters] << ")" << endl;
	switch ( theConnections.device_p->model_p->type ) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			PrintMinSimTerminalConnections(GATE, theConnections, theErrorFile);
			PrintMinSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMinSimTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( ! cvcParameters.cvcSOI ) PrintMinSimTerminalConnections(BULK, theConnections, theErrorFile);
		break; }
		case RESISTOR: case CAPACITOR: case DIODE: case FUSE_OFF: case FUSE_ON: case SWITCH_ON: case SWITCH_OFF: {
			PrintMinSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMinSimTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( theConnections.originalBulkId != UNKNOWN_NET) {
				PrintMinSimTerminalConnections(BULK, theConnections, theErrorFile);
			}
		break; }
		case BIPOLAR: {
			PrintMinSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMinSimTerminalConnections(GATE, theConnections, theErrorFile);
			PrintMinSimTerminalConnections(DRAIN, theConnections, theErrorFile);
		break; }
		default: {
			throw EDatabaseError("Invalid device type: " + theConnections.device_p->model_p->type);
		}
	}
}

void CCvcDb::PrintDeviceWithSimMaxConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile) {
	errorFile << DeviceName(theConnections.device_p->name, theParentId, PRINT_CIRCUIT_ON) << " " << theConnections.device_p->parameters;
	errorFile << " (r=" << parameterResistanceMap[theConnections.device_p->parameters] << ")" << endl;
	switch ( theConnections.device_p->model_p->type ) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			PrintSimMaxTerminalConnections(GATE, theConnections, theErrorFile);
			PrintSimMaxTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimMaxTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( ! cvcParameters.cvcSOI ) PrintSimMaxTerminalConnections(BULK, theConnections, theErrorFile);
		break; }
		case RESISTOR: case CAPACITOR: case DIODE: case FUSE_OFF: case FUSE_ON: case SWITCH_ON: case SWITCH_OFF: {
			PrintSimMaxTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimMaxTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( theConnections.originalBulkId != UNKNOWN_NET) {
				PrintSimMaxTerminalConnections(BULK, theConnections, theErrorFile);
			}
		break; }
		case BIPOLAR: {
			PrintSimMaxTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimMaxTerminalConnections(GATE, theConnections, theErrorFile);
			PrintSimMaxTerminalConnections(DRAIN, theConnections, theErrorFile);
		break; }
		default: {
			throw EDatabaseError("Invalid device type: " + theConnections.device_p->model_p->type);
		}
	}
}

void CCvcDb::PrintDeviceWithMaxConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile) {
	errorFile << DeviceName(theConnections.device_p->name, theParentId, PRINT_CIRCUIT_ON) << " " << theConnections.device_p->parameters;
	errorFile << " (r=" << parameterResistanceMap[theConnections.device_p->parameters] << ")" << endl;
	switch ( theConnections.device_p->model_p->type ) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			PrintMaxTerminalConnections(GATE, theConnections, theErrorFile);
			PrintMaxTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMaxTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( ! cvcParameters.cvcSOI ) PrintMaxTerminalConnections(BULK, theConnections, theErrorFile);
		break; }
		case RESISTOR: case CAPACITOR: case DIODE: case FUSE_OFF: case FUSE_ON: case SWITCH_ON: case SWITCH_OFF: {
			PrintMaxTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMaxTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( theConnections.originalBulkId != UNKNOWN_NET) {
				PrintMaxTerminalConnections(BULK, theConnections, theErrorFile);
			}
		break; }
		case BIPOLAR: {
			PrintMaxTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMaxTerminalConnections(GATE, theConnections, theErrorFile);
			PrintMaxTerminalConnections(DRAIN, theConnections, theErrorFile);
		break; }
		default: {
			throw EDatabaseError("Invalid device type: " + theConnections.device_p->model_p->type);
		}
	}
}

void CCvcDb::PrintDeviceWithMinConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile) {
	errorFile << DeviceName(theConnections.device_p->name, theParentId, PRINT_CIRCUIT_ON) << " " << theConnections.device_p->parameters;
	errorFile << " (r=" << parameterResistanceMap[theConnections.device_p->parameters] << ")" << endl;
	switch ( theConnections.device_p->model_p->type ) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			PrintMinTerminalConnections(GATE, theConnections, theErrorFile);
			PrintMinTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMinTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( ! cvcParameters.cvcSOI ) PrintMinTerminalConnections(BULK, theConnections, theErrorFile);
		break; }
		case RESISTOR: case CAPACITOR: case DIODE: case FUSE_OFF: case FUSE_ON: case SWITCH_ON: case SWITCH_OFF: {
			PrintMinTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMinTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( theConnections.originalBulkId != UNKNOWN_NET) {
				PrintMinTerminalConnections(BULK, theConnections, theErrorFile);
			}
		break; }
		case BIPOLAR: {
			PrintMinTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMinTerminalConnections(GATE, theConnections, theErrorFile);
			PrintMinTerminalConnections(DRAIN, theConnections, theErrorFile);
		break; }
		default: {
			throw EDatabaseError("Invalid device type: " + theConnections.device_p->model_p->type);
		}
	}
}
*/

void CCvcDb::PrintDeviceWithSimConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile) {
	int myMFactor = CalculateMFactor(theParentId);
	theErrorFile << DeviceName(theConnections.device_p->name, theParentId, PRINT_CIRCUIT_ON) << " " << theConnections.device_p->parameters;
	if ( myMFactor > 1 ) theErrorFile << " {m=" << myMFactor << "}";
	theErrorFile << " (r=" << parameterResistanceMap[theConnections.device_p->parameters] << ")" << endl;
	switch ( theConnections.device_p->model_p->type ) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			PrintSimTerminalConnections(GATE, theConnections, theErrorFile);
			PrintSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( ! cvcParameters.cvcSOI ) PrintSimTerminalConnections(BULK, theConnections, theErrorFile);
		break; }
		case RESISTOR: case CAPACITOR: case DIODE: case FUSE_OFF: case FUSE_ON: case SWITCH_ON: case SWITCH_OFF: {
			PrintSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( theConnections.originalBulkId != UNKNOWN_NET) {
				PrintSimTerminalConnections(BULK, theConnections, theErrorFile);
			}
		break; }
		case BIPOLAR: {
			PrintSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimTerminalConnections(GATE, theConnections, theErrorFile);
			PrintSimTerminalConnections(DRAIN, theConnections, theErrorFile);
		break; }
		default: {
			throw EDatabaseError("Invalid device type: " + theConnections.device_p->model_p->type);
		}
	}
}

/*
void CCvcDb::PrintDeviceWithMinGateAndSimConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile) {
	errorFile << DeviceName(theConnections.device_p->name, theParentId, PRINT_CIRCUIT_ON) << " " << theConnections.device_p->parameters;
	errorFile << " (r=" << parameterResistanceMap[theConnections.device_p->parameters] << ")" << endl;
	switch ( theConnections.device_p->model_p->type ) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			PrintMinTerminalConnections(GATE, theConnections, theErrorFile);
			PrintSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( ! cvcParameters.cvcSOI ) PrintSimTerminalConnections(BULK, theConnections, theErrorFile);
		break; }
		case RESISTOR: case CAPACITOR: case DIODE: case FUSE_OFF: case FUSE_ON: case SWITCH_ON: case SWITCH_OFF: {
			PrintSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( theConnections.originalBulkId != UNKNOWN_NET) {
				PrintSimTerminalConnections(BULK, theConnections, theErrorFile);
			}
		break; }
		case BIPOLAR: {
			PrintSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMinTerminalConnections(GATE, theConnections, theErrorFile);
			PrintSimTerminalConnections(DRAIN, theConnections, theErrorFile);
		break; }
		default: {
			throw EDatabaseError("Invalid device type: " + theConnections.device_p->model_p->type);
		}
	}
}

void CCvcDb::PrintDeviceWithMaxGateAndSimConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile) {
	errorFile << DeviceName(theConnections.device_p->name, theParentId, PRINT_CIRCUIT_ON) << " " << theConnections.device_p->parameters;
	errorFile << " (r=" << parameterResistanceMap[theConnections.device_p->parameters] << ")" << endl;
	switch ( theConnections.device_p->model_p->type ) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			PrintMaxTerminalConnections(GATE, theConnections, theErrorFile);
			PrintSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( ! cvcParameters.cvcSOI ) PrintSimTerminalConnections(BULK, theConnections, theErrorFile);
		break; }
		case RESISTOR: case CAPACITOR: case DIODE: case FUSE_OFF: case FUSE_ON: case SWITCH_ON: case SWITCH_OFF: {
			PrintSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintSimTerminalConnections(DRAIN, theConnections, theErrorFile);
			if ( theConnections.originalBulkId != UNKNOWN_NET) {
				PrintSimTerminalConnections(BULK, theConnections, theErrorFile);
			}
		break; }
		case BIPOLAR: {
			PrintSimTerminalConnections(SOURCE, theConnections, theErrorFile);
			PrintMaxTerminalConnections(GATE, theConnections, theErrorFile);
			PrintSimTerminalConnections(DRAIN, theConnections, theErrorFile);
		break; }
		default: {
			throw EDatabaseError("Invalid device type: " + theConnections.device_p->model_p->type);
		}
	}
}
*/

string CCvcDb::PrintVoltage(voltage_t theVoltage) {
	if ( theVoltage == UNKNOWN_VOLTAGE ) {
		return "???";
	} else {
		return to_string<voltage_t>(theVoltage);
	}
}

string CCvcDb::PrintVoltage(voltage_t theVoltage, CPower * thePower_p) {
	if ( theVoltage == UNKNOWN_VOLTAGE ) {
		if ( thePower_p && thePower_p->type[HIZ_BIT] ) {
			return "open";
		} else {
			return "???";
		}
	} else {
		return to_string<voltage_t>(theVoltage);
	}
}

void CCvcDb::PrintAllTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile, bool theIncludeLeakVoltage) {
	netId_t myNetId, myMinNetId, mySimNetId, myMaxNetId;
	string myMinVoltageString, mySimVoltageString, myMaxVoltageString;
	string myMinLeakVoltageString = "", myMaxLeakVoltageString = "";
	string myMinPowerDelimiter, mySimPowerDelimiter, myMaxPowerDelimiter;
	resistance_t myMinResistance;
	resistance_t mySimResistance;
	resistance_t myMaxResistance;
	switch (theTerminal) {
		case GATE: {
			theErrorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "B: " : "G: ");
			myNetId = theConnections.originalGateId;
			myMinNetId = theConnections.masterMinGateNet.finalNetId;
			myMinResistance = theConnections.masterMinGateNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minGateVoltage, theConnections.minGatePower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minGatePower_p, MIN_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.minGateVoltage != theConnections.minGateLeakVoltage ) {
				myMinLeakVoltageString = PrintVoltage(theConnections.minGateLeakVoltage, (CPower *) NULL);;
			}
			mySimNetId = theConnections.masterSimGateNet.finalNetId;
			mySimResistance = theConnections.masterSimGateNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simGateVoltage, theConnections.simGatePower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simGatePower_p, SIM_CALCULATED_BIT);
			myMaxNetId = theConnections.masterMaxGateNet.finalNetId;
			myMaxResistance = theConnections.masterMaxGateNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxGateVoltage, theConnections.maxGatePower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxGatePower_p, MAX_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.maxGateVoltage != theConnections.maxGateLeakVoltage ) {
				myMaxLeakVoltageString = PrintVoltage(theConnections.maxGateLeakVoltage, (CPower *) NULL);;
			}
		break; }
		case SOURCE: {
			theErrorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "C: " : "S: ");
			myNetId = theConnections.originalSourceId;
			myMinNetId = theConnections.masterMinSourceNet.finalNetId;
			myMinResistance = theConnections.masterMinSourceNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minSourceVoltage, theConnections.minSourcePower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minSourcePower_p, MIN_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.minSourceVoltage != theConnections.minSourceLeakVoltage ) {
				myMinLeakVoltageString = PrintVoltage(theConnections.minSourceLeakVoltage, (CPower *) NULL);;
			}
			mySimNetId = theConnections.masterSimSourceNet.finalNetId;
			mySimResistance = theConnections.masterSimSourceNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simSourceVoltage, theConnections.simSourcePower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simSourcePower_p, SIM_CALCULATED_BIT);
			myMaxNetId = theConnections.masterMaxSourceNet.finalNetId;
			myMaxResistance = theConnections.masterMaxSourceNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxSourceVoltage, theConnections.maxSourcePower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxSourcePower_p, MAX_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.maxSourceVoltage != theConnections.maxSourceLeakVoltage ) {
				myMaxLeakVoltageString = PrintVoltage(theConnections.maxSourceLeakVoltage, (CPower *) NULL);;
			}
		break; }
		case DRAIN: {
			theErrorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "E: " : "D: ");
			myNetId = theConnections.originalDrainId;
			myMinNetId = theConnections.masterMinDrainNet.finalNetId;
			myMinResistance = theConnections.masterMinDrainNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minDrainVoltage, theConnections.minDrainPower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minDrainPower_p, MIN_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.minDrainVoltage != theConnections.minDrainLeakVoltage ) {
				myMinLeakVoltageString = PrintVoltage(theConnections.minDrainLeakVoltage, (CPower *) NULL);;
			}
			mySimNetId = theConnections.masterSimDrainNet.finalNetId;
			mySimResistance = theConnections.masterSimDrainNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simDrainVoltage, theConnections.simDrainPower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simDrainPower_p, SIM_CALCULATED_BIT);
			myMaxNetId = theConnections.masterMaxDrainNet.finalNetId;
			myMaxResistance = theConnections.masterMaxDrainNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxDrainVoltage, theConnections.maxDrainPower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxDrainPower_p, MAX_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.maxDrainVoltage != theConnections.maxDrainLeakVoltage ) {
				myMaxLeakVoltageString = PrintVoltage(theConnections.maxDrainLeakVoltage, (CPower *) NULL);;
			}
		break; }
		case BULK: {
			theErrorFile << "B: ";
			myNetId = theConnections.originalBulkId;
			myMinNetId = theConnections.masterMinBulkNet.finalNetId;
			myMinResistance = theConnections.masterMinBulkNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minBulkVoltage, theConnections.minBulkPower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minBulkPower_p, MIN_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.minBulkVoltage != theConnections.minBulkLeakVoltage ) {
				myMinLeakVoltageString = PrintVoltage(theConnections.minBulkLeakVoltage, (CPower *) NULL);;
			}
			mySimNetId = theConnections.masterSimBulkNet.finalNetId;
			mySimResistance = theConnections.masterSimBulkNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simBulkVoltage, theConnections.simBulkPower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simBulkPower_p, SIM_CALCULATED_BIT);
			myMaxNetId = theConnections.masterMaxBulkNet.finalNetId;
			myMaxResistance = theConnections.masterMaxBulkNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxBulkVoltage, theConnections.maxBulkPower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxBulkPower_p, MAX_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.maxBulkVoltage != theConnections.maxBulkLeakVoltage ) {
				myMaxLeakVoltageString = PrintVoltage(theConnections.maxBulkLeakVoltage, (CPower *) NULL);;
			}
		break; }
		default: { throw EDatabaseError("Invalid terminal type: " + theTerminal); }
	}
	theErrorFile << NetName(myNetId) << endl;
	theErrorFile << " Min: " << NetName(myMinNetId) << NetVoltageSuffix(myMinPowerDelimiter, myMinVoltageString, myMinResistance, myMinLeakVoltageString) << endl;
	theErrorFile << " Sim: " << NetName(mySimNetId) << NetVoltageSuffix(mySimPowerDelimiter, mySimVoltageString, mySimResistance) << endl;
	theErrorFile << " Max: " << NetName(myMaxNetId) << NetVoltageSuffix(myMaxPowerDelimiter, myMaxVoltageString, myMaxResistance, myMaxLeakVoltageString) << endl;
}

/*
void CCvcDb::PrintMinMaxTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile, bool theIncludeLeakVoltage) {
	netId_t myNetId, myMinNetId, myMaxNetId;
	string myMinVoltageString, myMaxVoltageString;
	string myMinLeakVoltageString = "", myMaxLeakVoltageString = "";
	string myMinPowerDelimiter, myMaxPowerDelimiter;
	resistance_t myMinResistance;
	resistance_t myMaxResistance;
	switch (theTerminal) {
		case GATE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "B: " : "G: ");
			myNetId = theConnections.originalGateId;
			myMinNetId = theConnections.masterMinGateNet.finalNetId;
			myMinResistance = theConnections.masterMinGateNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minGateVoltage, theConnections.minGatePower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minGatePower_p, MIN_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.minGateVoltage != theConnections.minGateLeakVoltage ) {
				myMinLeakVoltageString = PrintVoltage(theConnections.minGateLeakVoltage, (CPower *) NULL);;
			}
			myMaxNetId = theConnections.masterMaxGateNet.finalNetId;
			myMaxResistance = theConnections.masterMaxGateNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxGateVoltage, theConnections.maxGatePower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxGatePower_p, MAX_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.maxGateVoltage != theConnections.maxGateLeakVoltage ) {
				myMaxLeakVoltageString = PrintVoltage(theConnections.maxGateLeakVoltage, (CPower *) NULL);;
			}
		break; }
		case SOURCE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "C: " : "S: ");
			myNetId = theConnections.originalSourceId;
			myMinNetId = theConnections.masterMinSourceNet.finalNetId;
			myMinResistance = theConnections.masterMinSourceNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minSourceVoltage, theConnections.minSourcePower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minSourcePower_p, MIN_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.minSourceVoltage != theConnections.minSourceLeakVoltage ) {
				myMinLeakVoltageString = PrintVoltage(theConnections.minSourceLeakVoltage, (CPower *) NULL);;
			}
			myMaxNetId = theConnections.masterMaxSourceNet.finalNetId;
			myMaxResistance = theConnections.masterMaxSourceNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxSourceVoltage, theConnections.maxSourcePower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxSourcePower_p, MAX_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.maxSourceVoltage != theConnections.maxSourceLeakVoltage ) {
				myMaxLeakVoltageString = PrintVoltage(theConnections.maxSourceLeakVoltage, (CPower *) NULL);;
			}
		break; }
		case DRAIN: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "E: " : "D: ");
			myNetId = theConnections.originalDrainId;
			myMinNetId = theConnections.masterMinDrainNet.finalNetId;
			myMinResistance = theConnections.masterMinDrainNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minDrainVoltage, theConnections.minDrainPower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minDrainPower_p, MIN_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.minDrainVoltage != theConnections.minDrainLeakVoltage ) {
				myMinLeakVoltageString = PrintVoltage(theConnections.minDrainLeakVoltage, (CPower *) NULL);;
			}
			myMaxNetId = theConnections.masterMaxDrainNet.finalNetId;
			myMaxResistance = theConnections.masterMaxDrainNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxDrainVoltage, theConnections.maxDrainPower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxDrainPower_p, MAX_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.maxDrainVoltage != theConnections.maxDrainLeakVoltage ) {
				myMaxLeakVoltageString = PrintVoltage(theConnections.maxDrainLeakVoltage, (CPower *) NULL);;
			}
		break; }
		case BULK: {
			errorFile << "B: ";
			myNetId = theConnections.originalBulkId;
			myMinNetId = theConnections.masterMinBulkNet.finalNetId;
			myMinResistance = theConnections.masterMinBulkNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minBulkVoltage, theConnections.minBulkPower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minBulkPower_p, MIN_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.minBulkVoltage != theConnections.minBulkLeakVoltage ) {
				myMinLeakVoltageString = PrintVoltage(theConnections.minBulkLeakVoltage, (CPower *) NULL);;
			}
			myMaxNetId = theConnections.masterMaxBulkNet.finalNetId;
			myMaxResistance = theConnections.masterMaxBulkNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxBulkVoltage, theConnections.maxBulkPower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxBulkPower_p, MAX_CALCULATED_BIT);
			if ( theIncludeLeakVoltage && theConnections.maxBulkVoltage != theConnections.maxBulkLeakVoltage ) {
				myMaxLeakVoltageString = PrintVoltage(theConnections.maxBulkLeakVoltage, (CPower *) NULL);;
			}
		break; }
		default: { throw EDatabaseError("Invalid terminal type: " + theTerminal); }
	}
	errorFile << NetName(myNetId) << endl;
	errorFile << " Min: " << NetName(myMinNetId) << NetVoltageSuffix(myMinPowerDelimiter, myMinVoltageString, myMinResistance, myMinLeakVoltageString) << endl;
	errorFile << " Max: " << NetName(myMaxNetId) << NetVoltageSuffix(myMaxPowerDelimiter, myMaxVoltageString, myMaxResistance, myMaxLeakVoltageString) << endl;
}

void CCvcDb::PrintMinSimTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile) {
	netId_t myNetId, myMinNetId, mySimNetId;
	string myMinVoltageString, mySimVoltageString;
	string myMinPowerDelimiter, mySimPowerDelimiter;
	resistance_t myMinResistance;
	resistance_t mySimResistance;
	switch (theTerminal) {
		case GATE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "B: " : "G: ");
			myNetId = theConnections.originalGateId;
			myMinNetId = theConnections.masterMinGateNet.finalNetId;
			myMinResistance = theConnections.masterMinGateNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minGateVoltage, theConnections.minGatePower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minGatePower_p, MIN_CALCULATED_BIT);
			mySimNetId = theConnections.masterSimGateNet.finalNetId;
			mySimResistance = theConnections.masterSimGateNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simGateVoltage, theConnections.simGatePower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simGatePower_p, SIM_CALCULATED_BIT);
		break; }
		case SOURCE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "C: " : "S: ");
			myNetId = theConnections.originalSourceId;
			myMinNetId = theConnections.masterMinSourceNet.finalNetId;
			myMinResistance = theConnections.masterMinSourceNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minSourceVoltage, theConnections.minSourcePower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minSourcePower_p, MIN_CALCULATED_BIT);
			mySimNetId = theConnections.masterSimSourceNet.finalNetId;
			mySimResistance = theConnections.masterSimSourceNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simSourceVoltage, theConnections.simSourcePower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simSourcePower_p, SIM_CALCULATED_BIT);
		break; }
		case DRAIN: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "E: " : "D: ");
			myNetId = theConnections.originalDrainId;
			myMinNetId = theConnections.masterMinDrainNet.finalNetId;
			myMinResistance = theConnections.masterMinDrainNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minDrainVoltage, theConnections.minDrainPower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minDrainPower_p, MIN_CALCULATED_BIT);
			mySimNetId = theConnections.masterSimDrainNet.finalNetId;
			mySimResistance = theConnections.masterSimDrainNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simDrainVoltage, theConnections.simDrainPower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simDrainPower_p, SIM_CALCULATED_BIT);
		break; }
		case BULK: {
			errorFile << "B: ";
			myNetId = theConnections.originalBulkId;
			myMinNetId = theConnections.masterMinBulkNet.finalNetId;
			myMinResistance = theConnections.masterMinBulkNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minBulkVoltage, theConnections.minBulkPower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minBulkPower_p, MIN_CALCULATED_BIT);
			mySimNetId = theConnections.masterSimBulkNet.finalNetId;
			mySimResistance = theConnections.masterSimBulkNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simBulkVoltage, theConnections.simBulkPower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simBulkPower_p, SIM_CALCULATED_BIT);
		break; }
		default: { throw EDatabaseError("Invalid terminal type: " + theTerminal); }
	}
	errorFile << NetName(myNetId) << endl;
	errorFile << " Min: " << NetName(myMinNetId) << NetVoltageSuffix(myMinPowerDelimiter, myMinVoltageString, myMinResistance) << endl;
	errorFile << " Sim: " << NetName(mySimNetId) << NetVoltageSuffix(mySimPowerDelimiter, mySimVoltageString, mySimResistance) << endl;
}

void CCvcDb::PrintSimMaxTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile) {
	netId_t myNetId, mySimNetId, myMaxNetId;
	string mySimVoltageString, myMaxVoltageString;
	string mySimPowerDelimiter, myMaxPowerDelimiter;
	resistance_t mySimResistance;
	resistance_t myMaxResistance;
	switch (theTerminal) {
		case GATE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "B: " : "G: ");
			myNetId = theConnections.originalGateId;
			mySimNetId = theConnections.masterSimGateNet.finalNetId;
			mySimResistance = theConnections.masterSimGateNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simGateVoltage, theConnections.simGatePower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simGatePower_p, SIM_CALCULATED_BIT);
			myMaxNetId = theConnections.masterMaxGateNet.finalNetId;
			myMaxResistance = theConnections.masterMaxGateNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxGateVoltage, theConnections.maxGatePower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxGatePower_p, MAX_CALCULATED_BIT);
		break; }
		case SOURCE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "C: " : "S: ");
			myNetId = theConnections.originalSourceId;
			mySimNetId = theConnections.masterSimSourceNet.finalNetId;
			mySimResistance = theConnections.masterSimSourceNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simSourceVoltage, theConnections.simSourcePower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simSourcePower_p, SIM_CALCULATED_BIT);
			myMaxNetId = theConnections.masterMaxSourceNet.finalNetId;
			myMaxResistance = theConnections.masterMaxSourceNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxSourceVoltage, theConnections.maxSourcePower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxSourcePower_p, MAX_CALCULATED_BIT);
		break; }
		case DRAIN: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "E: " : "D: ");
			myNetId = theConnections.originalDrainId;
			mySimNetId = theConnections.masterSimDrainNet.finalNetId;
			mySimResistance = theConnections.masterSimDrainNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simDrainVoltage, theConnections.simDrainPower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simDrainPower_p, SIM_CALCULATED_BIT);
			myMaxNetId = theConnections.masterMaxDrainNet.finalNetId;
			myMaxResistance = theConnections.masterMaxDrainNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxDrainVoltage, theConnections.maxDrainPower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxDrainPower_p, MAX_CALCULATED_BIT);
		break; }
		case BULK: {
			errorFile << "B: ";
			myNetId = theConnections.originalBulkId;
			mySimNetId = theConnections.masterSimBulkNet.finalNetId;
			mySimResistance = theConnections.masterSimBulkNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simBulkVoltage, theConnections.simBulkPower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simBulkPower_p, SIM_CALCULATED_BIT);
			myMaxNetId = theConnections.masterMaxBulkNet.finalNetId;
			myMaxResistance = theConnections.masterMaxBulkNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxBulkVoltage, theConnections.maxBulkPower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxBulkPower_p, MAX_CALCULATED_BIT);
		break; }
		default: { throw EDatabaseError("Invalid terminal type: " + theTerminal); }
	}
	errorFile << NetName(myNetId) << endl;
	errorFile << " Sim: " << NetName(mySimNetId) << NetVoltageSuffix(mySimPowerDelimiter, mySimVoltageString, mySimResistance) << endl;
	errorFile << " Max: " << NetName(myMaxNetId) << NetVoltageSuffix(myMaxPowerDelimiter, myMaxVoltageString, myMaxResistance) << endl;
}

void CCvcDb::PrintMinTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile) {
	netId_t myNetId, myMinNetId;
	string myMinVoltageString;
	string myMinPowerDelimiter;
	resistance_t myMinResistance;
	switch (theTerminal) {
		case GATE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "B: " : "G: ");
			myNetId = theConnections.originalGateId;
			myMinNetId = theConnections.masterMinGateNet.finalNetId;
			myMinResistance = theConnections.masterMinGateNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minGateVoltage, theConnections.minGatePower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minGatePower_p, MIN_CALCULATED_BIT);
		break; }
		case SOURCE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "C: " : "S: ");
			myNetId = theConnections.originalSourceId;
			myMinNetId = theConnections.masterMinSourceNet.finalNetId;
			myMinResistance = theConnections.masterMinSourceNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minSourceVoltage, theConnections.minSourcePower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minSourcePower_p, MIN_CALCULATED_BIT);
		break; }
		case DRAIN: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "E: " : "D: ");
			myNetId = theConnections.originalDrainId;
			myMinNetId = theConnections.masterMinDrainNet.finalNetId;
			myMinResistance = theConnections.masterMinDrainNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minDrainVoltage, theConnections.minDrainPower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minDrainPower_p, MIN_CALCULATED_BIT);
		break; }
		case BULK: {
			errorFile << "B: ";
			myNetId = theConnections.originalBulkId;
			myMinNetId = theConnections.masterMinBulkNet.finalNetId;
			myMinResistance = theConnections.masterMinBulkNet.finalResistance;
			myMinVoltageString = PrintVoltage(theConnections.minBulkVoltage, theConnections.minBulkPower_p);
			myMinPowerDelimiter = PowerDelimiter_(theConnections.minBulkPower_p, MIN_CALCULATED_BIT);
		break; }
		default: { throw EDatabaseError("Invalid terminal type: " + theTerminal); }
	}
	errorFile << NetName(myNetId) << endl;
	errorFile << " Min: " << NetName(myMinNetId) << NetVoltageSuffix(myMinPowerDelimiter, myMinVoltageString, myMinResistance) << endl;
}

void CCvcDb::PrintMaxTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile) {
	netId_t myNetId, myMaxNetId;
	string myMaxVoltageString;
	string myMaxPowerDelimiter;
	resistance_t myMaxResistance;
	switch (theTerminal) {
		case GATE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "B: " : "G: ");
			myNetId = theConnections.originalGateId;
			myMaxNetId = theConnections.masterMaxGateNet.finalNetId;
			myMaxResistance = theConnections.masterMaxGateNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxGateVoltage, theConnections.maxGatePower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxGatePower_p, MAX_CALCULATED_BIT);
		break; }
		case SOURCE: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "C: " : "S: ");
			myNetId = theConnections.originalSourceId;
			myMaxNetId = theConnections.masterMaxSourceNet.finalNetId;
			myMaxResistance = theConnections.masterMaxSourceNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxSourceVoltage, theConnections.maxSourcePower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxSourcePower_p, MAX_CALCULATED_BIT);
		break; }
		case DRAIN: {
			errorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "E: " : "D: ");
			myNetId = theConnections.originalDrainId;
			myMaxNetId = theConnections.masterMaxDrainNet.finalNetId;
			myMaxResistance = theConnections.masterMaxDrainNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxDrainVoltage, theConnections.maxDrainPower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxDrainPower_p, MAX_CALCULATED_BIT);
		break; }
		case BULK: {
			errorFile << "B: ";
			myNetId = theConnections.originalBulkId;
			myMaxNetId = theConnections.masterMaxBulkNet.finalNetId;
			myMaxResistance = theConnections.masterMaxBulkNet.finalResistance;
			myMaxVoltageString = PrintVoltage(theConnections.maxBulkVoltage, theConnections.maxBulkPower_p);
			myMaxPowerDelimiter = PowerDelimiter_(theConnections.maxBulkPower_p, MAX_CALCULATED_BIT);
		break; }
		default: { throw EDatabaseError("Invalid terminal type: " + theTerminal); }
	}
	errorFile << NetName(myNetId) << endl;
	errorFile << " Max: " << NetName(myMaxNetId) << NetVoltageSuffix(myMaxPowerDelimiter, myMaxVoltageString, myMaxResistance) << endl;
}
*/

void CCvcDb::PrintSimTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile) {
	netId_t myNetId, mySimNetId;
	string mySimVoltageString;
	string mySimPowerDelimiter;
	resistance_t mySimResistance;
	switch (theTerminal) {
		case GATE: {
			theErrorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "B: " : "G: ");
			myNetId = theConnections.originalGateId;
			mySimNetId = theConnections.masterSimGateNet.finalNetId;
			mySimResistance = theConnections.masterSimGateNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simGateVoltage, theConnections.simGatePower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simGatePower_p, SIM_CALCULATED_BIT);
		break; }
		case SOURCE: {
			theErrorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "C: " : "S: ");
			myNetId = theConnections.originalSourceId;
			mySimNetId = theConnections.masterSimSourceNet.finalNetId;
			mySimResistance = theConnections.masterSimSourceNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simSourceVoltage, theConnections.simSourcePower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simSourcePower_p, SIM_CALCULATED_BIT);
		break; }
		case DRAIN: {
			theErrorFile << (theConnections.device_p->model_p->type == BIPOLAR ? "E: " : "D: ");
			myNetId = theConnections.originalDrainId;
			mySimNetId = theConnections.masterSimDrainNet.finalNetId;
			mySimResistance = theConnections.masterSimDrainNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simDrainVoltage, theConnections.simDrainPower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simDrainPower_p, SIM_CALCULATED_BIT);
		break; }
		case BULK: {
			theErrorFile << "B: ";
			myNetId = theConnections.originalBulkId;
			mySimNetId = theConnections.masterSimBulkNet.finalNetId;
			mySimResistance = theConnections.masterSimBulkNet.finalResistance;
			mySimVoltageString = PrintVoltage(theConnections.simBulkVoltage, theConnections.simBulkPower_p);
			mySimPowerDelimiter = PowerDelimiter_(theConnections.simBulkPower_p, SIM_CALCULATED_BIT);
		break; }
		default: { throw EDatabaseError("Invalid terminal type: " + theTerminal); }
	}
	theErrorFile << NetName(myNetId) << endl;
	theErrorFile << " Sim: " << NetName(mySimNetId) << NetVoltageSuffix(mySimPowerDelimiter, mySimVoltageString, mySimResistance) << endl;
}

void CCvcDb::PrintErrorTotals() {
	reportFile << "CVC: Error Counts" << endl;
	reportFile << "CVC: Fuse Problems:         " << errorCount[FUSE_ERROR] << endl;
	reportFile << "CVC: Min Voltage Conflicts: " << errorCount[MIN_VOLTAGE_CONFLICT] << endl;
	reportFile << "CVC: Max Voltage Conflicts: " << errorCount[MAX_VOLTAGE_CONFLICT] << endl;
	reportFile << "CVC: Leaks:                 " << errorCount[LEAK] << endl;
	if ( detectErrorFlag ) {
		reportFile << "CVC: LDD drain->source:     " << errorCount[LDD_SOURCE] << endl;
		reportFile << "CVC: HI-Z Inputs:           " << errorCount[HIZ_INPUT] << endl;
		reportFile << "CVC: Forward Bias Diodes:   " << errorCount[FORWARD_DIODE] << endl;
		if ( ! cvcParameters.cvcSOI ) reportFile << "CVC: NMOS Source vs Bulk:   " << errorCount[NMOS_SOURCE_BULK] << endl;
		reportFile << "CVC: NMOS Gate vs Source:   " << errorCount[NMOS_GATE_SOURCE] << endl;
		reportFile << "CVC: NMOS Possible Leaks:   " << errorCount[NMOS_POSSIBLE_LEAK] << endl;
		if ( ! cvcParameters.cvcSOI ) reportFile << "CVC: PMOS Source vs Bulk:   " << errorCount[PMOS_SOURCE_BULK] << endl;
		reportFile << "CVC: PMOS Gate vs Source:   " << errorCount[PMOS_GATE_SOURCE] << endl;
		reportFile << "CVC: PMOS Possible Leaks:   " << errorCount[PMOS_POSSIBLE_LEAK] << endl;
		reportFile << "CVC: Overvoltage-VBG:       " << errorCount[OVERVOLTAGE_VBG] << endl;
		reportFile << "CVC: Overvoltage-VBS:       " << errorCount[OVERVOLTAGE_VBS] << endl;
		reportFile << "CVC: Overvoltage-VDS:       " << errorCount[OVERVOLTAGE_VDS] << endl;
		reportFile << "CVC: Overvoltage-VGS:       " << errorCount[OVERVOLTAGE_VGS] << endl;
		reportFile << "CVC: Unexpected voltage :   " << errorCount[EXPECTED_VOLTAGE] << endl;
	} else {
		reportFile << "WARNING: Error detection incomplete" << endl;
	}
	size_t myErrorTotal = 0;
	for ( auto myIndex = 0; myIndex < ERROR_TYPE_COUNT; myIndex++ ) {
		myErrorTotal += errorCount[myIndex];
	}
	reportFile << "CVC: Total:                 " << myErrorTotal << endl;
}

//void CCvcDb::OpenErrorFile(string theErrorFileName) {
//	cout << "CVC: Errors output to " << theErrorFileName << endl;
//	errorFile.open(theErrorFileName);
//}

/*
void CCvcDb::PrintShortedNets(string theShortFileName) {
	cout << "CVC: Printing shorted nets to " << theShortFileName << " ..." << endl;
	ogzstream	shortFile(theShortFileName);
//	instanceId_t myLastParent = 0;
//	CInstance * myParent_p;
	netId_t mySimNet;
	resistance_t myResistance;
//	bool myPrintResistance;
	string myNetPrefix;
	for (netId_t net_it = 0; net_it < netCount; net_it++) {
		//changed to include sim paths
		mySimNet = simNet_v[GetEquivalentNet(net_it)].nextNetId;
		myResistance = simNet_v[GetEquivalentNet(net_it)].resistance;
*/
		/*
		mySimNet = GetEquivalentNet(net_it);
		myResistance = 0;
		while ( mySimNet != simNet_v[mySimNet].nextNetId ) {
			myResistance += simNet_v[mySimNet].resistance;
			mySimNet = simNet_v[mySimNet].nextNetId;
		}
		*/
/*
		if ( net_it == mySimNet && ! netVoltagePtr_v[net_it] ) continue;
		if ( net_it != mySimNet ) shortFile << net_it << "->";
		shortFile << mySimNet;
		short_v[net_it].first = mySimNet;
		if ( netVoltagePtr_v[mySimNet] && netVoltagePtr_v[mySimNet]->simVoltage != UNKNOWN_VOLTAGE ) {
			if ( netVoltagePtr_v[mySimNet]->type[SIM_CALCULATED_BIT] ) {
				short_v[net_it].second = "=";
				if ( net_it < instancePtr_v[0]->master_p->portCount ) {
					logFile << "WARNING: possible missing input at net " << NetName(net_it);
					logFile << NetVoltageSuffix(PowerDelimiter_(netVoltagePtr_v[mySimNet], SIM_CALCULATED_BIT),
							PrintVoltage(netVoltagePtr_v[mySimNet]->simVoltage, netVoltagePtr_v[mySimNet]),
							simNet_v[mySimNet].finalResistance) << endl;
				}
			} else {
				short_v[net_it].second = "@";
			}
			short_v[net_it].second += PrintParameter(netVoltagePtr_v[mySimNet]->simVoltage, VOLTAGE_SCALE);
			if ( net_it != mySimNet ) short_v[net_it].second += " r=" + to_string<resistance_t>(myResistance);
			shortFile << short_v[net_it].second;
		} else {
			short_v[net_it].second = "";
		}
		shortFile << endl;
*/
/*
// short file format
0@1200		<- power definition
1@0
2			<- defined voltage, but no simulation value
3
4
6@-500
7@0
8@1200
9@3300
14->13		<- switch short
15->16
17->18
20->8@1200 r=875	<- propagated voltage and resistance
21=210				<- calculated voltage
*/


		/*
		if ( netParent_v[net_it] != myLastParent ) {
			shortFile << HierarchyName(netParent_v[net_it], PRINT_CIRCUIT_ON) << endl;
			myLastParent = netParent_v[net_it];
		}
		myResistance = 0;
		while ( mySimNet != simNet_v[mySimNet].nextNetId ) {
			myResistance += simNet_v[mySimNet].resistance;
			mySimNet = simNet_v[mySimNet].nextNetId;
		}
		myParent_p = instancePtr_v[myLastParent];
		shortFile << myParent_p->master_p->internalSignal_v[net_it - myParent_p->firstNetId];
		myPrintResistance = true;
		myMissingInput = false;
		if ( net_it == mySimNet ) {
			myPrintResistance = false;
			shortFile << " => ";
		} else {
			myPrintResistance = true;
			if ( myLastParent == netParent_v[mySimNet] && myLastParent != 0 ) {
				myNetPrefix = "~/";
			} else if ( netParent_v[mySimNet] == 0 ) {
				myNetPrefix = "";
			} else {
				myNetPrefix = "../";
			}
			myParent_p = instancePtr_v[netParent_v[mySimNet]];
			shortFile << " -> " << myNetPrefix << myParent_p->master_p->internalSignal_v[mySimNet - myParent_p->firstNetId];
		}
		if ( netVoltagePtr_v[mySimNet] == NULL || netVoltagePtr_v[mySimNet]->simVoltage == UNKNOWN_VOLTAGE ) {
			shortFile << " ??";
		} else {
			if ( netVoltagePtr_v[mySimNet]->type[SIM_CALCULATED_BIT] ) {
				shortFile << "=";
				if ( net_it < instancePtr_v[0]->master_p->portCount ) {
					myMissingInput = true;
				}
			} else {
				shortFile << "@";
			}
			shortFile << PrintVoltage(netVoltagePtr_v[mySimNet]->simVoltage);
		}
		if ( myPrintResistance ) {
			shortFile << " r=" << myResistance;
		}
		if ( myMissingInput ) {
			shortFile << " WARNING: possible missing input";
		}
		shortFile << endl;
*/
/*
	}
	shortFile.close();
	isValidShortData = true;
}
*/

string CCvcDb::NetVoltageSuffix(string theDelimiter, string theVoltage, resistance_t theResistance, string theLeakVoltage) {
	if ( theVoltage == "???" && IsEmpty(theLeakVoltage) ) return ("");
	if ( ! IsEmpty(theLeakVoltage) ) theLeakVoltage = "(" + theLeakVoltage + ")";
	return ( theDelimiter + theVoltage + theLeakVoltage + " r=" + to_string<resistance_t>(theResistance));
}

void CCvcDb::PrintResistorOverflow(netId_t theNet, ofstream& theOutputFile) {
	theOutputFile << "WARNING: resistance exceeded 1G ohm at " << NetName(theNet, PRINT_CIRCUIT_ON) << endl;
}

void CCvcDb::PrintClassSizes() {
	cout << "CBaseVirtualNet " << sizeof(class CBaseVirtualNet) << endl;
//	cout << "CBaseVirtualNetVector " << sizeof(class CBaseVirtualNetVector) << endl;
//	cout << "CCdlParserDriver " << sizeof(class CCdlParserDriver) << endl;
	cout << "CCdlText " << sizeof(class CCdlText) << endl;
	cout << "CCircuit " << sizeof(class CCircuit) << endl;
	cout << "CCircuitPtrList " << sizeof(class CCircuitPtrList) << endl;
	cout << "CCondition " << sizeof(class CCondition) << endl;
	cout << "CConditionPtrList " << sizeof(class CConditionPtrList) << endl;
	cout << "CConnection " << sizeof(class CConnection) << endl;
	cout << "CConnectionCount " << sizeof(class CConnectionCount) << endl;
	cout << "CConnectionCountVector " << sizeof(class CConnectionCountVector) << endl;
	cout << "CCvcDb " << sizeof(class CCvcDb) << endl;
	cout << "CCvcParameters " << sizeof(class CCvcParameters) << endl;
	cout << "CDependencyMap " << sizeof(class CDependencyMap) << endl;
	cout << "CDevice " << sizeof(class CDevice) << endl;
	cout << "CDeviceCount " << sizeof(class CDeviceCount) << endl;
	cout << "CDeviceIdVector " << sizeof(class CDeviceIdVector) << endl;
	cout << "CDevicePtrList " << sizeof(class CDevicePtrList) << endl;
	cout << "CDevicePtrVector " << sizeof(class CDevicePtrVector) << endl;
	cout << "CEventList " << sizeof(class CEventList) << endl;
	cout << "CEventQueue " << sizeof(class CEventQueue) << endl;
	cout << "CEventSubQueue " << sizeof(class CEventSubQueue) << endl;
	cout << "CFixedText " << sizeof(class CFixedText) << endl;
	cout << "CFullConnection " << sizeof(class CFullConnection) << endl;
	cout << "CHierList " << sizeof(class CHierList) << endl;
	cout << "CInstance " << sizeof(class CInstance) << endl;
	cout << "CInstanceIdVector " << sizeof(class CInstanceIdVector) << endl;
	cout << "CInstancePtrVector " << sizeof(class CInstancePtrVector) << endl;
	cout << "CLeakList " << sizeof(class CLeakList) << endl;
	cout << "CLeakMap " << sizeof(class CLeakMap) << endl;
	cout << "CModel " << sizeof(class CModel) << endl;
	cout << "CModeList " << sizeof(class CModeList) << endl;
	cout << "CModelList " << sizeof(class CModelList) << endl;
	cout << "CModelListMap " << sizeof(class CModelListMap) << endl;
	cout << "CNetIdVector " << sizeof(class CNetIdVector) << endl;
	cout << "CNetList " << sizeof(class CNetList) << endl;
	cout << "CNetMap " << sizeof(class CNetMap) << endl;
	cout << "CNormalValue " << sizeof(class CNormalValue) << endl;
	cout << "CParameterMap " << sizeof(class CParameterMap) << endl;
	cout << "CPower " << sizeof(class CPower) << endl;
	cout << "CPowerFamilyMap " << sizeof(class CPowerFamilyMap) << endl;
	cout << "CPowerPtrList " << sizeof(class CPowerPtrList) << endl;
	cout << "CPowerPtrMap " << sizeof(class CPowerPtrMap) << endl;
	cout << "CPowerPtrVector " << sizeof(class CPowerPtrVector) << endl;
	cout << "CSet " << sizeof(class CSet) << endl;
	cout << "CShortVector " << sizeof(class CShortVector) << endl;
	cout << "CStatusVector " << sizeof(class CStatusVector) << endl;
	cout << "CStringList " << sizeof(class CStringList) << endl;
	cout << "CStringTextMap " << sizeof(class CStringTextMap) << endl;
	cout << "CTextCircuitPtrMap " << sizeof(class CTextCircuitPtrMap) << endl;
	cout << "CTextDeviceIdMap " << sizeof(class CTextDeviceIdMap) << endl;
	cout << "CTextInstanceIdMap " << sizeof(class CTextInstanceIdMap) << endl;
	cout << "CTextList " << sizeof(class CTextList) << endl;
	cout << "CTextModelPtrMap " << sizeof(class CTextModelPtrMap) << endl;
	cout << "CTextNetIdMap " << sizeof(class CTextNetIdMap) << endl;
	cout << "CTextResistanceMap " << sizeof(class CTextResistanceMap) << endl;
	cout << "CTextVector " << sizeof(class CTextVector) << endl;
//	cout << "CVirtualLeakNet " << sizeof(class CVirtualLeakNet) << endl;
//	cout << "CVirtualLeakNetVector " << sizeof(class CVirtualLeakNetVector) << endl;
	cout << "CVirtualNet " << sizeof(class CVirtualNet) << endl;
	cout << "CVirtualNetMappedVector " << sizeof(class CVirtualNetMappedVector) << endl;
	cout << "CVirtualNetVector " << sizeof(class CVirtualNetVector) << endl;
}

void CCvcDb::PrintNetWithModelCounts(netId_t theNetId, int theTerminals) {
	map<string, deviceId_t> myDeviceCount;
	for (CModelListMap::iterator keyModelListPair_pit = cvcParameters.cvcModelListMap.begin(); keyModelListPair_pit != cvcParameters.cvcModelListMap.end(); keyModelListPair_pit++) {
		for (CModelList::iterator model_pit = keyModelListPair_pit->second.begin(); model_pit != keyModelListPair_pit->second.end(); model_pit++) {
			myDeviceCount[model_pit->name] = 0;
		}
	}
	if ( theTerminals & GATE ) {
		for ( deviceId_t device_it = firstGate_v[theNetId]; device_it != UNKNOWN_NET; device_it = nextGate_v[device_it]) {
			CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
			deviceId_t myDeviceOffset = device_it - myInstance_p->firstDeviceId;
			myDeviceCount[myInstance_p->master_p->devicePtr_v[myDeviceOffset]->model_p->name]++;
		}
	}
	if ( theTerminals & SOURCE ) {
		for ( deviceId_t device_it = firstSource_v[theNetId]; device_it != UNKNOWN_NET; device_it = nextSource_v[device_it]) {
			CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
			deviceId_t myDeviceOffset = device_it - myInstance_p->firstDeviceId;
			myDeviceCount[myInstance_p->master_p->devicePtr_v[myDeviceOffset]->model_p->name]++;
		}
	}
	if ( theTerminals & DRAIN ) {
		for ( deviceId_t device_it = firstDrain_v[theNetId]; device_it != UNKNOWN_NET; device_it = nextDrain_v[device_it]) {
			CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
			deviceId_t myDeviceOffset = device_it - myInstance_p->firstDeviceId;
			myDeviceCount[myInstance_p->master_p->devicePtr_v[myDeviceOffset]->model_p->name]++;
		}
	}
	reportFile << NetName(theNetId, PRINT_CIRCUIT_ON);
	for ( auto count_pit = myDeviceCount.begin(); count_pit != myDeviceCount.end(); count_pit++ ) {
		if ( count_pit->second > 0 ) {
			reportFile << " " << count_pit->first << "(" << count_pit->second << ")";
		}
	}
	reportFile << endl;
}

void CCvcDb::PrintBackupNet(CVirtualNetVector& theVirtualNet_v, netId_t theNetId, string theTitle, ostream& theOutputFile) {
	theOutputFile << theTitle << endl;
	theOutputFile << NetName(theNetId) << endl;
	netId_t myNetId = GetEquivalentNet(theNetId);
	if ( myNetId != theNetId ) cout << "=>" << NetName(myNetId) << endl;
	while ( myNetId != theVirtualNet_v[myNetId].backupNetId ) {
		theOutputFile << "->" << NetName(theVirtualNet_v[myNetId].backupNetId) << " r=" << theVirtualNet_v[myNetId].backupResistance << endl;
		myNetId = theVirtualNet_v[myNetId].backupNetId;
	}
	if ( leakVoltagePtr_v[myNetId] ) leakVoltagePtr_v[myNetId]->Print(theOutputFile);
	theOutputFile << endl;
}

