/*
 * CCircuit.hh
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

#ifndef CCIRCUIT_HH_
#define CCIRCUIT_HH_

#include "Cvc.hh"

#include "CDevice.hh"
#include "CFixedText.hh"
#include "gzstream.h"

class CCircuit {
public:
	text_t name;
	netId_t	portCount = 0;
	// local signal to local netID Map
	CTextNetIdMap localSignalIdMap;
	// subcircuit instance/device name to deviceID map
	CTextDeviceIdMap localDeviceIdMap;
	//Å@temporary list to convert to vector
	CTextList 	internalSignalList;
	// local netID to signal name map
	CTextVector 	internalSignal_v;
	CDevicePtrVector	devicePtr_v;
	CDevicePtrVector	subcircuitPtr_v;
	vector<size_t>		deviceErrorCount_v;

	// total items for this circuit and all subcircuits
	uintmax_t	netCount = 0;
	uintmax_t	deviceCount = 0;
	// the number of children
	uintmax_t	subcircuitCount = 0;
	// the number of instantiations
	uintmax_t	instanceCount = 0;

	CInstanceIdVector instanceId_v;
	bool	linked = false;

	deviceId_t		errorCount = 0;
	deviceId_t		warningCount = 0;

	inline netId_t	LocalNetCount() { return ( localSignalIdMap.size() - portCount); }

	void AddPortSignalIds(CTextList * thePortList_p);
	void SetSignalIds(CTextList * theSignalList_p, CNetIdVector & theSignalId_v);
	void LoadDevices(CDevicePtrList * theDeviceList_p);

	void CountObjectsAndLinkSubcircuits(unordered_map<text_t, CCircuit *> & theCircuitNameMap);
	void CountInstantiations();

	void Print(const string theIndentation = "");

};

class CTextCircuitPtrMap : public unordered_map<text_t, CCircuit *> {
public:

};

class CCircuitPtrList : public list<CCircuit *> {
private:
public:
	CFixedText	cdlText;
	CTextCircuitPtrMap  circuitNameMap;
	CFixedText	parameterText;
	int			errorCount = 0;
	int			warningCount = 0;
//	CCircuitPtrList();
//	CCircuitPtrList(int count, CDevice new_instance);
	void Clear();

	void Print(const string theIndentation = "", const string theHeading = "CircuitList>");
//	void CreateDatabase(const string theTopBlockName);
	CCircuit * FindCircuit(const string theSearchCircuit);
	void PrintAndResetCircuitErrors(deviceId_t theErrorLimit, ogzstream & theErrorFile);
};


#endif /* CCIRCUIT_HH_ */
