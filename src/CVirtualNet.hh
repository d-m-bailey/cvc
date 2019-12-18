/*
 * CVirtualNet.hh
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

#ifndef CVIRTUALNET_HH_
#define CVIRTUALNET_HH_

#include "Cvc.hh"

#include "CPower.hh"
#include "mmappable_vector.h"

using namespace mmap_allocator_namespace;

class CVirtualNet;
class CVirtualNetVector;

class CBaseVirtualNet {
public:
	netId_t	nextNetId = UNKNOWN_NET;
	resistance_t resistance = 0;

	void operator= (CVirtualNet& theEqualNet);
};

/*
class CBaseVirtualNetVector : public vector<CBaseVirtualNet> {
public:
	CBaseVirtualNetVector& operator() (CVirtualNetVector& theFullNetVector);
};

class CVirtualLeakNet : public CBaseVirtualNet {
public:
	netId_t	finalNetId = UNKNOWN_NET;

	void operator= (CVirtualNet& theEqualNet);
};

class CVirtualLeakNetVector : public vector<CVirtualLeakNet> {
public:
	CVirtualLeakNetVector& operator() (CVirtualNetVector& theFullNetVector);
};
*/

class CVirtualNetMappedVector : public mmappable_vector<CVirtualNet> {
public:
	CVirtualNetMappedVector(mmap_allocator<CVirtualNet> theAllocator);
	void operator= (CVirtualNetVector& theNetVector);
};

class CVirtualNet {
public:
	netId_t	nextNetId = UNKNOWN_NET;
	resistance_t resistance = 0;
	netId_t	finalNetId = UNKNOWN_NET;
	resistance_t finalResistance = 0;
	netId_t	backupNetId = UNKNOWN_NET;
	resistance_t backupResistance = 0;
//	eventKey_t lastUpdate = 0;

	void operator= (CVirtualNet& theEqualNet);
	inline bool operator== (CVirtualNet& theTestNet) { return (nextNetId == theTestNet.nextNetId && resistance == theTestNet.resistance); };
//	void operator+= (CVirtualNet theAddNet);
	CVirtualNet& operator() (CVirtualNetVector& theVirtualNet_v, netId_t theNetId);
//	CVirtualNet();
//	CVirtualNet(CVirtualNetVector& theVirtualNet_v, netId_t theNetId);
//	void GetMasterNet(CVirtualNetVector& theVirtualNet_v, netId_t theNetId);
	void Copy(const CVirtualNet& theBase);
	void Print(ostream& theOutputFile);
};

class CVirtualNetVector : public vector<CVirtualNet> {
public:
	eventKey_t lastUpdate;
	vector<eventKey_t> lastUpdate_v;
	powerType_t calculatedBit;

	CVirtualNetVector(powerType_t theCalculatedBit) : calculatedBit(theCalculatedBit) {};

//	CVirtualNet * MasterNet(netId_t theNetId);
	inline void resize (size_type n) { vector<CVirtualNet>::resize(n); lastUpdate_v.resize(n, 0); };
//	inline void resize (size_type n, const value_type& val) { vector<CVirtualNet>::resize(n, val); lastUpdate_v.resize(n, 0); };;
	inline void reserve (size_type n) { vector<CVirtualNet>::reserve(n); lastUpdate_v.reserve(n); };
	inline void shrink_to_fit() { vector<CVirtualNet>::shrink_to_fit(); lastUpdate_v.shrink_to_fit(); };
	inline bool IsTerminal(netId_t theNetId) {	return ( theNetId == (*this)[theNetId].nextNetId ); }
	void Print(string theTitle = "", string theIndentation = "");
	void Print(CNetIdVector& theEquivalentNet_v, string theTitle = "", string theIndentation = "");
//	void SetFinalNet(netId_t theNetId);
	void Set(netId_t theNetId, netId_t theNextNet, resistance_t theResistance, eventKey_t theTime);
	void DebugVirtualNet(netId_t theNetId, string theTitle = "", ostream& theOutputFile = cout);
	void BackupVirtualNets();
	inline void InitializeUpdateArray() { lastUpdate_v.resize(size(), 0); };
	inline void ClearUpdateArray() { lastUpdate_v.clear(); };

};

#endif /* CVIRTUALNET_HH_ */
