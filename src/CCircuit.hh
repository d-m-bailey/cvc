/*
 * CCircuit.hh
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

#ifndef CCIRCUIT_HH_
#define CCIRCUIT_HH_

#include "Cvc.hh"

class CCvcDb;
#include "CDevice.hh"
#include "CInstance.hh"
#include "CFixedText.hh"
#include "gzstream.h"

class CCircuit {
	// subcircuit instance/device name to deviceID map
	static text_t lastDeviceMap;
	static CTextDeviceIdMap localDeviceIdMap;
	static text_t lastSubcircuitMap;
	static CTextDeviceIdMap localSubcircuitIdMap;
public:
	deviceId_t errorLimit = UNKNOWN_DEVICE;
	text_t name;
	// local signal to local netID Map
	CTextNetIdMap localSignalIdMap;
	// temporary list to convert to vector
	CTextList	internalSignalList;
	// local netID to signal name map
	CTextVector	internalSignal_v;
	CDevicePtrVector	devicePtr_v;
	CDevicePtrVector	subcircuitPtr_v;
	vector<array<deviceId_t, 4>>	deviceErrorCount_v;
	vector<array<deviceId_t, 4>>    devicePrintCount_v;
	CInstanceIdVector instanceId_v;
	CInstanceIdVector instanceHashId_v;

	netId_t	portCount = 0;
	// total items for this circuit and all subcircuits
	netId_t	netCount = 0;
	deviceId_t	deviceCount = 0;
	// the number of children
	instanceId_t	subcircuitCount = 0;
	// the number of instantiations
	instanceId_t	instanceCount = 0;
	string	checksum = "";

//	deviceId_t		errorCount = 0;
//	deviceId_t		warningCount = 0;
	bool	linked = false;

	inline netId_t	LocalNetCount() { return ( localSignalIdMap.size() - portCount); }

	void AddPortSignalIds(CTextList * thePortList_p);
	void SetSignalIds(CTextList * theSignalList_p, CNetIdVector & theSignalId_v);
	deviceId_t GetLocalDeviceId(text_t theName);
	deviceId_t GetLocalSubcircuitId(text_t theName);
	void LoadDevices(CDevicePtrList * theDeviceList_p);

	void CountObjectsAndLinkSubcircuits(unordered_map<text_t, CCircuit *> & theCircuitNameMap);
	void CountInstantiations();

	void Print(const string theIndentation = "");

	void AllocateInstances(CCvcDb * theCvcDb_p, instanceId_t theFirstInstanceId);
};

class CTextCircuitPtrMap : public unordered_map<text_t, CCircuit *> {
public:
	CTextCircuitPtrMap(float theLoadFactor = DEFAULT_LOAD_FACTOR) {max_load_factor(theLoadFactor);}
};

static set<modelType_t> emptyModelList;

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
	void SetChecksum(const string theSearchCircuit, const string theChecksum);
	void PrintAndResetCircuitErrors(CCvcDb * theCvcDb_p, deviceId_t theErrorLimit, ofstream & theLogFile, ogzstream & theErrorFile,
		string theSummaryHeading = "", int theSubErrorIndex = 0, set<modelType_t> & theModelList = emptyModelList);
};


#endif /* CCIRCUIT_HH_ */
