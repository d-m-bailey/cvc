/*
 * CVirtualNet.cc
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

#include "CVirtualNet.hh"

extern long gVirtualNetUpdateCount;
extern long gVirtualNetAccessCount;

/*
void CVirtualNet::operator+= (CVirtualNet theAddNet) {
	nextNetId = theAddNet.nextNetId;
	assert( resistance >= 0 && theAddNet.resistance >= 0 );
	resistance += theAddNet.resistance;
}
*/

void CBaseVirtualNet::operator= (CVirtualNet& theEqualNet) {
	nextNetId = theEqualNet.nextNetId;
	resistance = theEqualNet.resistance;
}

void CVirtualLeakNet::operator= (CVirtualNet& theEqualNet) {
	nextNetId = theEqualNet.nextNetId;
	resistance = theEqualNet.resistance;
	finalNetId = theEqualNet.finalNetId;
}

void CVirtualNet::operator= (CVirtualNet& theEqualNet) {
	nextNetId = theEqualNet.nextNetId;
	resistance = theEqualNet.resistance;
	finalNetId = theEqualNet.finalNetId;
	finalResistance = theEqualNet.finalResistance;
	lastUpdate = theEqualNet.lastUpdate;
}

CVirtualNet& CVirtualNet::operator() (CVirtualNetVector& theVirtualNet_v, netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) {
		nextNetId = finalNetId = UNKNOWN_NET;
		resistance = finalResistance = INFINITE_RESISTANCE;
	} else {
		gVirtualNetAccessCount++;
		nextNetId = theVirtualNet_v[theNetId].nextNetId;
		resistance = theVirtualNet_v[theNetId].resistance;
		if ( theVirtualNet_v[theNetId].lastUpdate < theVirtualNet_v.lastUpdate ) {
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
//			if ( nextNetId != theVirtualNet_v[theNetId].finalNetId || resistance != theVirtualNet_v[theNetId].finalResistance ) {
//				cout << "virtual/final mismatch " << theNetId << " -> " << nextNetId << "!=" << theVirtualNet_v[theNetId].finalNetId;
//				cout << ", " << resistance << "!=" << theVirtualNet_v[theNetId].finalResistance << endl;
//				theVirtualNet_v.DebugVirtualNet(theNetId);
//			}
//			assert(nextNetId == theVirtualNet_v[theNetId].finalNetId);
//			assert(resistance == theVirtualNet_v[theNetId].finalResistance);
			lastUpdate = theVirtualNet_v[theNetId].lastUpdate = theVirtualNet_v.lastUpdate;
		} else {
			finalNetId = theVirtualNet_v[theNetId].finalNetId;
			finalResistance = theVirtualNet_v[theNetId].finalResistance;
			lastUpdate = theVirtualNet_v[theNetId].lastUpdate;
		}
		assert(finalResistance < MAX_RESISTANCE);
	}
//	finalNetId = nextNetId;
//	finalResistance = resistance;
	return (*this);
}


/*
CVirtualNet::CVirtualNet() {
}

CVirtualNet::CVirtualNet(CVirtualNetVector& theVirtualNet_v, netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) {
		nextNetId = UNKNOWN_NET;
		resistance = INFINITE_RESISTANCE;
	} else {
		int myLinkCount = 0;
		nextNetId = theNetId;
		resistance = 0;
		while ( nextNetId != theVirtualNet_v[nextNetId].nextNetId ) {
			(*this) += theVirtualNet_v[nextNetId];
			myLinkCount++;
			if ( myLinkCount > 1000 ) {
				cout << "looping at net " << theNetId << endl;
				assert ( myLinkCount < 1021 );
			}
		}
		resistance += theVirtualNet_v[nextNetId].resistance;
	}
}
*/

CBaseVirtualNetVector& CBaseVirtualNetVector::operator() (CVirtualNetVector& theFullNetVector) {
	this->reserve(theFullNetVector.size());
	for( netId_t net_it = 0; net_it < theFullNetVector.size(); net_it++) {
		(*this)[net_it] = theFullNetVector[net_it];
	}
	return(*this);
}

CVirtualLeakNetVector& CVirtualLeakNetVector::operator() (CVirtualNetVector& theFullNetVector) {
	this->reserve(theFullNetVector.size());
	for( netId_t net_it = 0; net_it < theFullNetVector.size(); net_it++) {
		(*this)[net_it] = theFullNetVector[net_it];
	}
	return(*this);
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
//		myFinalResistance += (*this)[myFinalNetId].resistance;
		AddResistance(myFinalResistance, (*this)[myFinalNetId].resistance);
/*
		nextNetId = theVirtualNet_v[theNetId].nextNetId;
		resistance = theVirtualNet_v[theNetId].resistance;
		while ( nextNetId != theVirtualNet_v[nextNetId].nextNetId ) {
			(*this) += theVirtualNet_v[nextNetId];
		}
*/
	}
	(*this)[theNetId].finalNetId = myFinalNetId;
	(*this)[theNetId].finalResistance = myFinalResistance;
	lastUpdate = theTime;
	(*this)[theNetId].lastUpdate = theTime;
//	cout << "Setting virtual net " << theNetId << " -> " << theNextNet << "@" << theResistance << " final -> " << myFinalNetId << "@" << myFinalResistance << endl;
}

/*void CVirtualNetVector::SetFinalNet(netId_t theNetId) {
	netId_t myFinalNetId;
	resistance_t myFinalResistance;
	if ( (*this)[theNetId].nextNetId == UNKNOWN_NET ) {
		myFinalNetId = UNKNOWN_NET;
		myFinalResistance = INFINITE_RESISTANCE;
	} else {
		int myLinkCount = 0;
		myFinalNetId = (*this)[theNetId].nextNetId;
		myFinalResistance = 0;
		while ( myFinalNetId != (*this)[myFinalNetId].nextNetId ) {
			myFinalNetId = (*this)[myFinalNetId].nextNetId;
			assert ((*this)[myFinalNetId].resistance >= 0);
			myFinalResistance += (*this)[myFinalNetId].resistance;
			myLinkCount++;
			if ( myLinkCount > 1000 ) {
				cout << "looping at net " << myFinalNetId << endl;
				assert ( myLinkCount < 1021 );
			}
		}
		assert ((*this)[myFinalNetId].resistance >= 0);
		myFinalResistance += (*this)[myFinalNetId].resistance;

//		nextNetId = theVirtualNet_v[theNetId].nextNetId;
//		resistance = theVirtualNet_v[theNetId].resistance;
//		while ( nextNetId != theVirtualNet_v[nextNetId].nextNetId ) {
//			(*this) += theVirtualNet_v[nextNetId];
//		}

	}
	(*this)[theNetId].finalNetId = myFinalNetId;
	(*this)[theNetId].finalResistance = myFinalResistance;
}*/

/*
void CVirtualNet::GetMasterNet(CVirtualNetVector& theVirtualNet_v, netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) {
		nextNetId = UNKNOWN_NET;
		resistance = INFINITE_RESISTANCE;
	} else {
		int myLinkCount = 0;
		nextNetId = theNetId;
		resistance = 0;
		while ( nextNetId != theVirtualNet_v[nextNetId].nextNetId ) {
			(*this) += theVirtualNet_v[nextNetId];
			myLinkCount++;
			if ( myLinkCount > 1000 ) {
				cout << "looping at net " << theNetId << endl;
				assert ( myLinkCount < 1021 );
			}
		}
		assert (theVirtualNet_v[nextNetId].resistance >= 0);
		resistance += theVirtualNet_v[nextNetId].resistance;
//
//		nextNetId = theVirtualNet_v[theNetId].nextNetId;
//		resistance = theVirtualNet_v[theNetId].resistance;
//		while ( nextNetId != theVirtualNet_v[nextNetId].nextNetId ) {
//			(*this) += theVirtualNet_v[nextNetId];
//		}
//
	}
	finalNetId = nextNetId;
	finalResistance = resistance;
}
*/

/*
CVirtualNet * CVirtualNetVector::MasterNet(netId_t theNetId) {
	CVirtualNet * myNewNet = new CVirtualNet;
	myNewNet->nextNetId = theNetId;
	while ( myNewNet->nextNetId != (*this)[myNewNet->nextNetId].nextNetId ) {
		*myNewNet += (*this)[theNetId];
	}
	return myNewNet;
}

*/
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
//	netId_t myNetId = GetEquivalentNet(theNetId);
//	if ( myNetId != theNetId ) cout << "=>" << NetName(myNetId) << endl;
	while ( theNetId != (*this)[theNetId].nextNetId ) {
		theOutputFile << "->" << (*this)[theNetId].nextNetId << " r=" << (*this)[theNetId].resistance << endl;
		theNetId = (*this)[theNetId].nextNetId;
	}
	theOutputFile << "-> last r=" << (*this)[theNetId].resistance << endl;
//	if ( netVoltagePtr_v[theNetId] ) netVoltagePtr_v[theNetId]->Print(theOutputFile);
	theOutputFile << endl;
}


