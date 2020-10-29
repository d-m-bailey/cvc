/*
 * CEventQueue.cc
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

#include "CEventQueue.hh"

void CEventQueue::ResetQueue(deviceId_t theDeviceCount) {
	queueArray.clear();
	queueArray.resize(theDeviceCount, UNKNOWN_DEVICE);
	enqueueCount = dequeueCount = requeueCount = 0;
	queueStart = false;
	virtualNet_v.lastUpdate = 0;
}

bool CEventQueue::IsNextMainQueue() {
	if ( mainQueue.empty() ) return false;
	if ( delayQueue.empty() ) return true;
	// for SIM_QUEUE, mainQueue has priority always. for MIN/MAX_QUEUE, mainQueue has priority if not later key
	return ( queueType == SIM_QUEUE || ( mainQueue.begin()->first <= delayQueue.begin()->first ) );
}

void CEventQueue::AddEvent(eventKey_t theEventKey, deviceId_t theDeviceIndex, queuePosition_t theQueuePosition) {
	// theDelay = 1 : front of delay queue
	// theDelay = 2 : back of delay queue
	// theDelay = 0 : back of main queue
	// theDelay = -1 : front of main queue (mos diode)
	// theDelay = -2 : skip (shouldn't be here)
	// theDelay = -3 : front of main queue (HiZ)
	if ( queueType == MAX_QUEUE ) theEventKey = - theEventKey;
	switch (theQueuePosition) {
		case QUEUE_HIZ:
		case MOS_DIODE: { mainQueue[theEventKey].push_front(theDeviceIndex); break; }
		case MAIN_BACK: { mainQueue[theEventKey].push_back(theDeviceIndex); break; }
		case DELAY_FRONT: { delayQueue[theEventKey].push_front(theDeviceIndex); break; }
		case DELAY_BACK: { delayQueue[theEventKey].push_back(theDeviceIndex); break; }
		default: { throw EFatalError("invalid queue delay " + to_string<int>((int) theQueuePosition)); }
	}
	enqueueCount++;
	if ( --printCounter <= 0 ) PrintStatus();
	if (gDebug_cvc) cout << "Adding to queue(" << gEventQueueTypeMap[queueType] << ") device: " << theDeviceIndex << "@" << theEventKey << "+" << theQueuePosition << endl;
  }

deviceId_t CEventQueue::GetMainEvent() {
	deviceId_t myDeviceIndex = mainQueue.begin()->second.pop_front(); //mainQueue.begin()->second.front();
	if ( mainQueue.begin()->second.empty() ) {
		mainQueue.erase(mainQueue.begin());
	}
	dequeueCount++;
	if ( --printCounter <= 0 ) PrintStatus();
	return myDeviceIndex;
}

deviceId_t CEventQueue::GetDelayEvent() {
	deviceId_t myDeviceIndex = delayQueue.begin()->second.pop_front(); //delayQueue.begin()->second.front();
	if ( delayQueue.begin()->second.empty() ) {
		delayQueue.erase(delayQueue.begin());
	}
	dequeueCount++;
	if ( --printCounter <= 0 ) PrintStatus();
	return myDeviceIndex;
}

deviceId_t CEventQueue::GetEvent() {
	if ( IsNextMainQueue() ) {
		return GetMainEvent();
	} else {
		return GetDelayEvent();
	}
}

void CEventQueue::AddLeak(deviceId_t theDevice, CConnection& theConnections) {
	string	myKey;
	if ( theConnections.sourcePower_p->powerId < theConnections.drainPower_p->powerId ) {
		myKey = to_string<int>(theConnections.sourcePower_p->powerId) + " " + to_string<int>(theConnections.drainPower_p->powerId);
	} else {
		myKey = to_string<int>(theConnections.drainPower_p->powerId) + " " + to_string<int>(theConnections.sourcePower_p->powerId);
	}
	leakMap[myKey].push_front(theDevice);
	if (gDebug_cvc) cout << "leak key " << myKey << " map size " << leakMap.size() << " list size " << leakMap[myKey].listSize << endl;
}

void CEventQueue::Print(string theIndentation) {
	string myIndentation = theIndentation + " ";
	cout << theIndentation << "EventQueue(" << gEventQueueTypeMap[queueType] << ")> start" << endl;
	cout << myIndentation << "Counts (enqueue/dequeue/requeue) " << enqueueCount << "/" << dequeueCount << "/" << requeueCount << endl;
	cout << myIndentation << "Main Queue>" << endl;
	for (CEventSubQueue::iterator eventPair_pit = mainQueue.begin(); eventPair_pit != mainQueue.end(); eventPair_pit++) {
		cout << myIndentation << "Time: " << eventPair_pit->first << " (" << eventPair_pit->second.size() << "):";
		for (deviceId_t device_it = eventPair_pit->second.first, myLastDevice = UNKNOWN_DEVICE;
				device_it != UNKNOWN_DEVICE && device_it != myLastDevice;
				myLastDevice = device_it, device_it = queueArray[device_it]) {
			cout  << " " << device_it;
		}
		cout << endl;
	}
	cout << myIndentation << "Delay Queue>" << endl;
	for (CEventSubQueue::iterator eventPair_pit = delayQueue.begin(); eventPair_pit != delayQueue.end(); eventPair_pit++) {
		cout << myIndentation << "Time: " << eventPair_pit->first << " (" << eventPair_pit->second.size() << "):";
		for (deviceId_t device_it = eventPair_pit->second.first, myLastDevice = UNKNOWN_DEVICE;
				device_it != UNKNOWN_DEVICE && device_it != myLastDevice;
				myLastDevice = device_it, device_it = queueArray[device_it]) {
			cout  << " " << device_it;
		}
		cout << endl;
	}
	leakMap.Print(myIndentation);
	cout << theIndentation << "EventQueue> end" << endl;
}

CEventList& CEventSubQueue::operator[] (eventKey_t theEventKey) {
	CEventSubQueue::iterator myItem_it = find(theEventKey);
	if ( myItem_it == end() ) {
		pair<CEventSubQueue::iterator, bool> myResult;
		myResult = insert(make_pair(theEventKey, CEventList(queueArray)));
		return (myResult.first->second);
	} else {
		return (myItem_it->second);
	}
}

eventKey_t CEventSubQueue::QueueTime(eventQueue_t theQueueType) {
	if ( size() == 0 ) return MAX_EVENT_TIME;
	return ( begin()->first );
}

eventKey_t CEventQueue::QueueTime() {
	if ( (! queueStart) || (delayQueue.empty() && mainQueue.empty()) ) return 0;
	if ( queueType == MAX_QUEUE ) {
		return (- min(mainQueue.QueueTime(queueType), delayQueue.QueueTime(queueType)));
	} else {
		return (min(mainQueue.QueueTime(queueType), delayQueue.QueueTime(queueType)));
	}
}

bool CEventQueue::Later(eventKey_t theEventKey) {
	if ( queueType == MAX_QUEUE ) {
		return ( theEventKey < QueueTime() );
	} else {
		return ( theEventKey > QueueTime() );
	}
}

bool CEventQueue::Later(eventKey_t theFirstKey, eventKey_t theSecondKey) {
	if ( queueType == MAX_QUEUE ) {
		return ( theFirstKey < theSecondKey );
	} else {
		return ( theFirstKey > theSecondKey );
	}
}

void CLeakList::Print(string theIndentation) {
	cout << theIndentation << "LeakList>";
	for (CLeakList::iterator device_pit = begin(); device_pit != end(); device_pit++) {
		cout  << " " << *device_pit;
	}
	cout << endl;
}

string CLeakMap::PrintLeakKey(string theKey) {
	unsigned int myFirstKey = from_string<unsigned int>(theKey.substr(0, theKey.find(" ")));
	unsigned int mySecondKey = from_string<unsigned int>(theKey.substr(theKey.find(" ")));
	string myFirstPowerName = "", mySecondPowerName = "";
	for (CPowerPtrList::iterator power_ppit = powerPtrList_p->begin(); power_ppit != powerPtrList_p->end(); power_ppit++) {
		if ( (*power_ppit)->powerId == myFirstKey ) myFirstPowerName = string((*power_ppit)->powerSignal());
		if ( (*power_ppit)->powerId == mySecondKey ) mySecondPowerName = string((*power_ppit)->powerSignal());
	}
	return (myFirstPowerName + ":" + mySecondPowerName);
}

void CLeakMap::Print(string theIndentation) {
	string myIndentation = theIndentation + " ";
	cout << theIndentation << "LeakMap(" << size() << ")> start" << endl;
	for (CLeakMap::iterator leakPair_pit = begin(); leakPair_pit != end(); leakPair_pit++) {
		cout << myIndentation << "Key: " << PrintLeakKey(leakPair_pit->first) << " :(" << leakPair_pit->second.listSize << ") ";
		leakPair_pit->second.Print("");
	}
	cout << theIndentation << "LeakMap> end" << endl;
}

void CEventQueue::PrintStatus(int theNextPrintCount) {
	cout << gEventQueueTypeMap[queueType] << " Counts (size/enqueue/requeue) " << enqueueCount - dequeueCount<< "/" <<  enqueueCount << "/" << requeueCount << "\r" << std::flush;
	printCounter = theNextPrintCount;
}

void CEventList::push_back(deviceId_t theDevice) {
	if (queueArray[theDevice] != UNKNOWN_DEVICE) throw EQueueError(to_string<deviceId_t> (theDevice));
	queueArray[theDevice] = theDevice;
	if ( first == UNKNOWN_DEVICE ) {
		assert(second == UNKNOWN_DEVICE);
		first = theDevice;
	} else {
		assert(second != UNKNOWN_DEVICE);
		queueArray[second] = theDevice;
	}
	eventListSize++;
	second = theDevice;
}

void CEventList::push_front(deviceId_t theDevice) {
	if (queueArray[theDevice] != UNKNOWN_DEVICE) throw EQueueError(to_string<deviceId_t> (theDevice));
	if ( first == UNKNOWN_DEVICE ) {
		assert(second == UNKNOWN_DEVICE);
		second = theDevice;
		queueArray[theDevice] = theDevice;
	} else {
		assert(second != UNKNOWN_DEVICE);
		queueArray[theDevice] = first;
	}
	eventListSize++;
	first = theDevice;
}

deviceId_t CEventList::pop_front() {
	if (first == UNKNOWN_DEVICE) throw EQueueError("empty queue");
	deviceId_t myDevice;
	myDevice = first;
	if ( first == queueArray[first] ) { // last element in queue
		first = second = UNKNOWN_DEVICE;
	} else {
		first = queueArray[first];
	}
	queueArray[myDevice] = UNKNOWN_DEVICE;
	eventListSize--;
	return(myDevice);
}

inline deviceId_t CEventList::front() {
	return(first);
}

bool CEventList::empty() {
	return(first == UNKNOWN_DEVICE);
}

