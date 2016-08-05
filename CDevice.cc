/*
 * CDevice.cc
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

#include "CDevice.hh"

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

void CDevice::Print (CTextVector & theSignalName_v, CTextDeviceIdMap & theSubcircuitIdMap, const string theIndentation) {
	string myIndentation = theIndentation + " ";
	cout << theIndentation << "Instance Name: " << name << ":" << theSubcircuitIdMap[name] << endl;
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

void CDevicePtrVector::Print(CTextVector& theSignalName_v,	const string theIndentation, const string myHeading) {
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

void CDevicePtrVector::Print(CTextVector& theSignalName_v,	CTextDeviceIdMap& theSubcircuitId, const string theIndentation, const string myHeading) {
	if (empty()) {
		cout << theIndentation << myHeading << " empty" << endl;
	} else {
		cout << theIndentation << myHeading << " start" << endl;
		string myIndentation = theIndentation + " ";
		for (CDevicePtrVector::iterator device_ppit = begin(); device_ppit != end(); ++device_ppit) {
			(*device_ppit)->Print(theSignalName_v, theSubcircuitId, myIndentation);
		}
		cout << theIndentation << myHeading << " end" << endl;
	}
}



