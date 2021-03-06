/*
 * CVirtualNet.cc
 *
 * Copyright 2014-2020 D. Mitch Bailey  cvc at shuharisystem dot com
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

#include "CVirtualNet.hh"
#include "mmappable_vector.h"

using namespace mmap_allocator_namespace;

extern long gVirtualNetUpdateCount;
extern long gVirtualNetAccessCount;


void CVirtualNet::operator= (CVirtualNet& theEqualNet) {
	nextNetId = theEqualNet.nextNetId;
	resistance = theEqualNet.resistance;
	finalNetId = theEqualNet.finalNetId;
	finalResistance = theEqualNet.finalResistance;
	backupNetId = theEqualNet.backupNetId;
}

CVirtualNet& CVirtualNet::operator() (CVirtualNetVector& theVirtualNet_v, netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) {
		nextNetId = finalNetId = UNKNOWN_NET;
		resistance = finalResistance = INFINITE_RESISTANCE;
	} else {
		gVirtualNetAccessCount++;
		nextNetId = theVirtualNet_v[theNetId].nextNetId;
		resistance = theVirtualNet_v[theNetId].resistance;
		if ( theVirtualNet_v.lastUpdate_v.size() > 0
				&& theVirtualNet_v.lastUpdate_v[theNetId] < theVirtualNet_v.lastUpdate ) {
			gVirtualNetUpdateCount++;
			int myLinkCount = 0;
			finalNetId = theNetId;
			finalResistance = 0;
			while ( finalNetId != theVirtualNet_v[finalNetId].nextNetId ) {
				AddResistance(finalResistance, theVirtualNet_v[finalNetId].resistance);
				finalNetId = theVirtualNet_v[finalNetId].nextNetId;
				myLinkCount++;
				if ( myLinkCount > 5000 ) {
					cout << "looping at net " << finalNetId << endl;
					assert ( myLinkCount < 5021 );
				}
			}
			AddResistance(finalResistance, theVirtualNet_v[finalNetId].resistance);
			theVirtualNet_v[theNetId].finalNetId = finalNetId;
			theVirtualNet_v[theNetId].finalResistance = finalResistance;
			theVirtualNet_v.lastUpdate_v[theNetId] = theVirtualNet_v.lastUpdate;
		} else {
			finalNetId = theVirtualNet_v[theNetId].finalNetId;
			finalResistance = theVirtualNet_v[theNetId].finalResistance;
		}
		assert(finalResistance < MAX_RESISTANCE);
	}
	return (*this);
}

CVirtualNetMappedVector::CVirtualNetMappedVector(mmap_allocator<CVirtualNet> theAllocator) :
	mmappable_vector(theAllocator) {
}

void CVirtualNetMappedVector::operator= (CVirtualNetVector& theSourceVector) {
	mmap_file(theSourceVector.size());
	assign(theSourceVector.begin(), theSourceVector.end());
	remmap_file_for_read();
}

void CVirtualNetVector::Set(netId_t theNetId, netId_t theNextNet, resistance_t theResistance, eventKey_t theTime) {
	if ( (*this)[theNextNet].nextNetId == theNetId && theNextNet != theNetId ) {
		cout << "DEBUG: The next net of " << theNetId << " is already set to " << theNextNet << endl;
		return;
	}
	(*this)[theNetId].nextNetId = theNextNet;
	(*this)[theNetId].resistance = theResistance;
	netId_t myFinalNetId;
	resistance_t myFinalResistance;
	if ( (*this)[theNetId].nextNetId == UNKNOWN_NET ) {
		myFinalNetId = UNKNOWN_NET;
		myFinalResistance = INFINITE_RESISTANCE;
	} else {
		int myLinkCount = 0;
		myFinalNetId = theNetId;
		myFinalResistance = 0;
		while ( myFinalNetId != (*this)[myFinalNetId].nextNetId ) {
			assert ((*this)[myFinalNetId].resistance >= 0);
			AddResistance(myFinalResistance, (*this)[myFinalNetId].resistance);
			myFinalNetId = (*this)[myFinalNetId].nextNetId;
			myLinkCount++;
			if ( myLinkCount > 5000 ) {
				cout << "looping at net " << myFinalNetId << endl;
				cout << "DEBUG: linking " << theNetId << " to " << theNextNet << endl;
				assert ( myLinkCount < 5021 );
			}
		}
		assert ((*this)[myFinalNetId].resistance >= 0);
		AddResistance(myFinalResistance, (*this)[myFinalNetId].resistance);
	}
	(*this)[theNetId].finalNetId = myFinalNetId;
	(*this)[theNetId].finalResistance = myFinalResistance;
	lastUpdate = theTime;
	(*this).lastUpdate_v[theNetId] = theTime;
}

void CVirtualNet::Copy(const CVirtualNet& theBase) {
	nextNetId = theBase.nextNetId;
	resistance = theBase.resistance;
	finalNetId = theBase.finalNetId;
	finalResistance = theBase.finalResistance;
}

void CVirtualNet::Print(ostream& theOutputFile) {
	theOutputFile << "Next net(R) " << nextNetId << "(" << resistance << ")";
	theOutputFile << " final net(R) " << finalNetId << "(" << finalResistance << ")" << endl;
}

void CVirtualNetVector::Print(string theTitle, string theIndentation) {
	cout << theIndentation << "VirtualNetVector" << theTitle << "> start " << size() << endl;
	for ( netId_t net_it = 0; net_it < size(); net_it++ ) {
		if ( net_it != (*this)[net_it].nextNetId ) {
			cout << theIndentation + " " << net_it << "->" << (*this)[net_it].nextNetId << "@" << (*this)[net_it].resistance << endl;
		}
	}
	cout << theIndentation << "VirtualNetVector" << theTitle << "> end" << endl;
}

void CVirtualNetVector::DebugVirtualNet(netId_t theNetId, string theTitle, ostream& theOutputFile) {
	theOutputFile << theTitle << endl;
	theOutputFile << theNetId << endl;
	while ( theNetId != (*this)[theNetId].nextNetId ) {
		theOutputFile << "->" << (*this)[theNetId].nextNetId << " r=" << (*this)[theNetId].resistance << endl;
		theNetId = (*this)[theNetId].nextNetId;
	}
	theOutputFile << "-> last r=" << (*this)[theNetId].resistance << endl;
	theOutputFile << endl;
}

void CVirtualNetVector::BackupVirtualNets() {
	for ( auto net_pit = begin(); net_pit != end(); net_pit++ ) {
		net_pit->backupNetId = net_pit->nextNetId;
	}
}
