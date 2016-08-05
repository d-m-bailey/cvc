/*
 * CDevice.hh
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

#ifndef CDEVICE_HH_
#define CDEVICE_HH_

#include "Cvc.hh"

#include "CModel.hh"

class CCircuit;

class CDevice {
public:
	text_t name;
	CTextList * signalList_p;
	CNetIdVector signalId_v;
	CCircuit * parent_p;

	// for subcircuits
	text_t masterName;
	CCircuit * master_p = NULL;
	// for devices
	CModel * model_p = NULL;
	text_t parameters = NULL;
	CDevice * nextDevice_p = NULL;
//	resistance_t	cvcResistance = INFINITE_RESISTANCE;
	bool	sourceDrainSet = false;
	bool 	sourceDrainSwapOk = true;

	inline void AppendSignal (text_t theNewSignal) {
		// skip null signals (missing bias)
		if ( theNewSignal ) signalList_p->push_back(theNewSignal);
	}
	inline bool IsSubcircuit() { return name[0] == 'X' && parameters == text_t(NULL); }
	inline bool IsBox() { return name[0] == 'X' && parameters != text_t(NULL); }
	inline bool IsMosfet() { return name[0] == 'M'; }
	inline bool IsCapacitor() { return name[0] == 'C'; }
	inline bool IsDiode() { return name[0] == 'D'; }
	inline bool IsResistor() { return name[0] == 'R'; }
	inline bool IsSwitch() { return name[0] == 'S'; }
	inline bool IsFuse() { return name[0] == 'F'; }

	void Print(CTextVector& theSignalName_v, const string theIndentation = "");
	void Print(CTextVector& theSignalName_v, CTextDeviceIdMap& theSubcircuitId,
			const string theIndentation = "");
};

class CDevicePtrList : public list<CDevice *> {
public:
//	CDeviceList();
//	CDeviceList(int count, CDevice new_instance);
	deviceId_t	subcircuitCount = 0;

	inline deviceId_t	DeviceCount() { return (size() - subcircuitCount); }
	inline deviceId_t	SubcircuitCount() { return subcircuitCount; }
	void Print(CTextVector& theSignalName_v, const string theIndentation = "",
			const string theHeading = "DeviceList>");
};

class CDevicePtrVector : public vector<CDevice *> {
public:

	void Print(CTextVector& theSignalName_v, const string theIndentation = "",
			const string theHeading = "DeviceList>");
	void Print(CTextVector& theSignalName_v, CTextDeviceIdMap& theSubcircuitId,
			const string theIndentation = "", const string theHeading = "DeviceList>");
};

#endif /* CDEVICE_HH_ */
