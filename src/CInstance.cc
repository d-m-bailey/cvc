/*
 * CInstance.cc
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

#include "CInstance.hh"

#include "CCvcDb.hh"
#include "CCircuit.hh"

void CInstance::AssignTopGlobalIDs(CCvcDb * theCvcDb_p, CCircuit * theMaster_p) {

	theMaster_p->instanceId_v.push_back(0);
	theCvcDb_p->subcircuitCount = 1;
//	theCvcDb_p->subcircuitParent.push_back(0);
	firstSubcircuitId = 1;
	parentId = 0;
	master_p = theMaster_p;
	netId_t myLastNet = theMaster_p->localSignalIdMap.size();
	localToGlobalNetId_v.reserve(myLastNet);
	localToGlobalNetId_v.resize(myLastNet);
	theMaster_p->internalSignal_v.clear();
	theMaster_p->internalSignal_v.reserve(theMaster_p->localSignalIdMap.size());
	theMaster_p->internalSignal_v.resize(theMaster_p->localSignalIdMap.size());
	for (CTextNetIdMap::iterator textNetIdPair_pit = theMaster_p->localSignalIdMap.begin(); textNetIdPair_pit != theMaster_p->localSignalIdMap.end(); textNetIdPair_pit++) {
		theMaster_p->internalSignal_v[textNetIdPair_pit->second] = textNetIdPair_pit->first;
	}
	for (netId_t globalNet_it = 0; globalNet_it < myLastNet; globalNet_it++) {
		localToGlobalNetId_v[globalNet_it] = globalNet_it;
	}
	theCvcDb_p->netCount = myLastNet;
	theCvcDb_p->subcircuitCount += theMaster_p->subcircuitPtr_v.size();
	theCvcDb_p->deviceCount = theMaster_p->devicePtr_v.size();
	theCvcDb_p->netParent_v.resize(theCvcDb_p->netCount, 0);
//	theCvcDb_p->subcircuitParent.resize(theCvcDb_p->subcircuitCount, 0);
	theCvcDb_p->deviceParent_v.resize(theCvcDb_p->deviceCount, 0);

	instanceId_t myLastSubcircuitId = theMaster_p->subcircuitPtr_v.size();
	instanceId_t myNewInstanceId;
	for (instanceId_t subcircuit_it = 0; subcircuit_it < myLastSubcircuitId; subcircuit_it++) {
		if (theMaster_p->subcircuitPtr_v[subcircuit_it]->master_p->deviceCount > 0) {
			myNewInstanceId = firstSubcircuitId + subcircuit_it;
			theCvcDb_p->instancePtr_v[myNewInstanceId] = new CInstance;
			theCvcDb_p->instancePtr_v[myNewInstanceId]->AssignGlobalIDs(theCvcDb_p, myNewInstanceId, theMaster_p->subcircuitPtr_v[subcircuit_it], 0, this);
		}
	}
}

void CInstance::AssignGlobalIDs(CCvcDb * theCvcDb_p, const instanceId_t theInstanceId, const CDevice * theSubcircuit_p, const instanceId_t theParentId, const CInstance * theParent_p) {
	CCircuit * myMaster_p = theSubcircuit_p->master_p;
	if ( myMaster_p->instanceId_v.size() == 0 ) {
		myMaster_p->instanceId_v.reserve(myMaster_p->instanceCount);
	}
	myMaster_p->instanceId_v.push_back(theInstanceId);

 	firstSubcircuitId = theCvcDb_p->subcircuitCount;
 	firstNetId = theCvcDb_p->netCount;
 	firstDeviceId = theCvcDb_p->deviceCount;
	parentId = theParentId;
	master_p = myMaster_p;

	netId_t lastNet = myMaster_p->localSignalIdMap.size();
	localToGlobalNetId_v.reserve(lastNet);
	localToGlobalNetId_v.resize(lastNet);
	for (netId_t net_it = 0; net_it < myMaster_p->portCount; net_it++) {
		localToGlobalNetId_v[net_it] = theParent_p->localToGlobalNetId_v[theSubcircuit_p->signalId_v[net_it]];
	}
	for (netId_t net_it = myMaster_p->portCount; net_it < lastNet; net_it++) {
		localToGlobalNetId_v[net_it] = firstNetId + net_it - myMaster_p->portCount;
	}
	theCvcDb_p->netCount += myMaster_p->LocalNetCount();
	theCvcDb_p->subcircuitCount += master_p->subcircuitPtr_v.size();
	theCvcDb_p->deviceCount += myMaster_p->devicePtr_v.size();
	theCvcDb_p->netParent_v.resize(theCvcDb_p->netCount, theInstanceId);
//	theCvcDb_p->subcircuitParent.resize(theCvcDb_p->subcircuitCount, theParentId);
	theCvcDb_p->deviceParent_v.resize(theCvcDb_p->deviceCount, theInstanceId);

	instanceId_t myLastSubcircuitId = myMaster_p->subcircuitPtr_v.size();
	instanceId_t myNewInstanceId;
	for (instanceId_t subcircuit_it = 0;	subcircuit_it < myLastSubcircuitId; subcircuit_it++) {
		if (myMaster_p->subcircuitPtr_v[subcircuit_it]->master_p->deviceCount > 0) {
			myNewInstanceId = firstSubcircuitId + subcircuit_it;
			theCvcDb_p->instancePtr_v[myNewInstanceId] = new CInstance;
			theCvcDb_p->instancePtr_v[myNewInstanceId]->AssignGlobalIDs(theCvcDb_p, myNewInstanceId, myMaster_p->subcircuitPtr_v[subcircuit_it], theInstanceId, this);
		}
	}
}

void CInstance::Print (const instanceId_t theInstanceId, const string theIndentation) {
	cout << theIndentation << "Instance> " << theInstanceId << endl;
	string myIndentation = theIndentation + " ";
	cout << myIndentation << "First: subcircuit: " << firstSubcircuitId;
	cout << "  net: " << firstNetId;
	cout << "  device: " << firstDeviceId << endl;
	cout << myIndentation << "parent: " << parentId << "  master: " << master_p->name << endl;
	cout << myIndentation << "signal map:";
	for (netId_t net_it = 0; net_it < localToGlobalNetId_v.size(); net_it++) {
		cout << " " << net_it << ":" << localToGlobalNetId_v[net_it];
	}
	cout << endl;
}

