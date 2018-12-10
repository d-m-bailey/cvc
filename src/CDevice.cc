/*
 * CDevice.cc
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

#include "CDevice.hh"

#include "CCvcDb.hh"
#include "CCircuit.hh"

CDevice::~CDevice() {
	delete signalList_p;
}

// Print primitive devices
void CDevice::Print (CTextVector& theSignalName_v, const string theIndentation) {
	string myIndentation = theIndentation + " ";
	cout << theIndentation << "Device Name: " << name << endl;
	if ( model_p == NULL ) {
		cout << myIndentation << "Model: unknown" << endl;
	} else {
		model_p->Print(cout, PRINT_DEVICE_LIST_OFF, myIndentation);
	}
	cout << myIndentation << "Signal List(" << signalId_v.size() << "):";
	cout << ((sourceDrainSet) ? "+" : "-") << ((sourceDrainSwapOk) ? "+" : "-");
	for (netId_t net_it = 0; net_it < signalId_v.size(); ++net_it) {
		cout << " " << theSignalName_v[signalId_v[net_it]] << ":" << signalId_v[net_it];
	}
	cout << endl << myIndentation << "Parameter List: " << parameters << endl;
}

// Print Subcircuit Devices

void CDevice::Print (CTextVector & theSignalName_v, deviceId_t theId, const string theIndentation) {
	string myIndentation = theIndentation + " ";
	cout << theIndentation << "Instance Name: " << name << ":" << theId << endl;
	cout << myIndentation << "Master Name: " << masterName;
	if ( master_p == NULL ) {
		cout << " ?" << endl;
	} else {
		cout << " ok" << endl;
	}
	cout << myIndentation << "Signal List(" << signalId_v.size() << "):";
	for (netId_t net_it = 0; net_it < signalId_v.size(); ++net_it) {
		cout << " " << theSignalName_v[signalId_v[net_it]] << ":" << signalId_v[net_it];
	}
	cout << endl;
}

void CDevicePtrList::Print (CTextVector& theSignalName_v, const string theIndentation, const string myHeading) {
	cout << theIndentation << myHeading << " start" << endl;
	string myIndentation = theIndentation + " ";
	for (CDevicePtrList::iterator device_ppit = begin(); device_ppit != end(); ++device_ppit) {
		(*device_ppit)->Print(theSignalName_v, myIndentation);
	}
	cout << theIndentation << myHeading << " end" << endl;
}

CDevicePtrVector::~CDevicePtrVector() {
	for ( auto device_ppit = begin(); device_ppit != end(); device_ppit++ ) {
		delete (*device_ppit);
	}
	resize(0);
}

void CDevicePtrVector::Print(CTextVector& theSignalName_v, const string theIndentation, const string myHeading) {
	if (empty()) {
		cout << theIndentation << myHeading << " empty" << endl;
	} else {
		cout << theIndentation << myHeading << " start" << endl;
		string myIndentation = theIndentation + " ";
		for (CDevicePtrVector::iterator device_ppit = begin(); device_ppit != end(); ++device_ppit) {
			(*device_ppit)->Print(theSignalName_v, myIndentation);
		}
		cout << theIndentation << myHeading << " end" << endl;
	}
}

void CDevicePtrVector::Print(CTextVector& theSignalName_v, CCircuit * theParent_p, const string theIndentation, const string myHeading) {
	if (empty()) {
		cout << theIndentation << myHeading << " empty" << endl;
	} else {
		cout << theIndentation << myHeading << " start" << endl;
		string myIndentation = theIndentation + " ";
		for (CDevicePtrVector::iterator device_ppit = begin(); device_ppit != end(); ++device_ppit) {
			(*device_ppit)->Print(theSignalName_v, (*device_ppit)->offset, myIndentation);
		}
		cout << theIndentation << myHeading << " end" << endl;
	}
}

#define A_PRIME 0xcc9e2d51

instanceId_t CDevice::MakePortHash(CNetIdVector & theLocalToGlobalNetId_v) {
	instanceId_t myHash;
	for ( netId_t port_it = 0; port_it < master_p->portCount; port_it++ ) {
		myHash = myHash << 16 ^ theLocalToGlobalNetId_v[signalId_v[port_it]] ^ myHash;
		myHash *= A_PRIME;
	}
	return ( myHash % master_p->instanceCount );
}

extern int gHashCollisionCount;
extern int gMaxHashLength;

instanceId_t CDevice::FindParallelInstance(CCvcDb * theCvcDb_p, instanceId_t theInstanceId, CNetIdVector & theLocalToGlobalNetId_v) {
	// Executed once and only once for each instance
	instanceId_t myKey = MakePortHash(theLocalToGlobalNetId_v);
	assert(myKey != UNKNOWN_DEVICE && myKey < master_p->instanceCount );
	if ( master_p->instanceHashId_v[myKey] == UNKNOWN_DEVICE ) {
		master_p->instanceHashId_v[myKey] = theInstanceId;
	} else {
		instanceId_t myCheckInstanceId = master_p->instanceHashId_v[myKey];
		CInstance * myInstance_p;
		int myHashLength = 0;
		do {
			myInstance_p = theCvcDb_p->instancePtr_v[myCheckInstanceId];
			if ( master_p != myInstance_p->master_p ) {
				cout << "DEBUG: master mismatch at instance " << theInstanceId << " and " << myCheckInstanceId << endl;
				cout << "  " << master_p->name << " != " << myInstance_p->master_p->name << endl;
			}
			for ( netId_t port_it = 0; port_it <= master_p->portCount; port_it++ ) {
				if ( port_it == master_p->portCount ) {  // all ports match
					return (myCheckInstanceId);
				} else {
					if (theLocalToGlobalNetId_v[signalId_v[port_it]] != myInstance_p->localToGlobalNetId_v[port_it]) break;
				}
			}
			myHashLength++;
			gHashCollisionCount++;
			myCheckInstanceId = myInstance_p->nextHashedInstanceId;
		} while ( myCheckInstanceId != UNKNOWN_DEVICE );
		if ( gMaxHashLength < myHashLength ) gMaxHashLength = myHashLength;
		myInstance_p->nextHashedInstanceId = theInstanceId;
	}
	return (theInstanceId);
}
