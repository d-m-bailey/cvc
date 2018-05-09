/*
 * CEventQueue.hh
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

#ifndef CEVENTQUEUE_HH_
#define CEVENTQUEUE_HH_

#include "Cvc.hh"

#include "CVirtualNet.hh"
#include "CPower.hh"
#include "CConnection.hh"

enum queuePosition_t {QUEUE_HIZ = -3, SKIP_QUEUE, MOS_DIODE, MAIN_BACK, DELAY_FRONT, DELAY_BACK};

#define DefaultQueuePosition_(flag, queue) (((flag) || queue.queueType == SIM_QUEUE) ? MAIN_BACK : DELAY_FRONT)

/*
class CEventList : public deque<deviceId_t> {
public:
};
*/
class CEventList : public pair<deviceId_t, deviceId_t> {
public:
	vector<deviceId_t>& queueArray;
	size_t eventListSize = 0;

	CEventList(vector<deviceId_t>& theQueueArray) : queueArray(theQueueArray) { first = second = UNKNOWN_DEVICE; }
	void push_back(deviceId_t theDevice);
	void push_front(deviceId_t theDevice);
	deviceId_t pop_front();
	deviceId_t front();
	inline size_t size() {return eventListSize;};
	bool empty();
};

class CLeakList : public forward_list<deviceId_t> {
public:
	int listSize = 0;
	void push_front(deviceId_t theDeviceId) {
		listSize++;
		forward_list<deviceId_t>::push_front(theDeviceId);
	}
	void Print(string theIndentation = "");
};

class CLeakMap : public map<string, CLeakList> {
public:
	CPowerPtrList * powerPtrList_p = NULL;

	string PrintLeakKey(string theKey);
	void Print(string theIndentation = "");
};

class CEventSubQueue : public map<eventKey_t, CEventList> {
public:
	vector<deviceId_t>& queueArray;

	CEventSubQueue(vector<deviceId_t>& theQueueArray) : queueArray(theQueueArray) {}
	CEventList& operator[] (eventKey_t theEventKey) { return ( ( *((this->insert(make_pair(theEventKey, CEventList(queueArray)))).first)).second ); }
	eventKey_t QueueTime(eventQueue_t theQueueType);
};

class CEventQueue {
public:
	vector<deviceId_t> queueArray;

	const eventQueue_t	queueType;
	const deviceStatus_t		inactiveBit;
	const deviceStatus_t		pendingBit;
	CVirtualNetVector& virtualNet_v;
	CPowerPtrVector& netVoltage_v;
	CLeakMap  leakMap;

	CEventSubQueue	mainQueue;
	CEventSubQueue	delayQueue;
//	CEventSubQueue	savedMainQueue;
//	CEventSubQueue	savedDelayQueue;

	bool	queueStart = false;

	long	enqueueCount = 0;
	long	dequeueCount = 0;
	long	requeueCount = 0;
//	long	alreadyProcessed = 0;
	int		printCounter = 1000000;

	CEventQueue(eventQueue_t theQueueType, deviceStatus_t theInactiveBit, deviceStatus_t thePendingBit, CVirtualNetVector& theVirtualNet_v, CPowerPtrVector& theNetVoltage_v) :
		queueType(theQueueType), inactiveBit(theInactiveBit), pendingBit(thePendingBit), virtualNet_v(theVirtualNet_v), netVoltage_v(theNetVoltage_v), mainQueue(queueArray), delayQueue(queueArray) {}
	void ResetQueue(deviceId_t theDeviceCount);
	void AddEvent(eventKey_t theEventKey, deviceId_t theDeviceId, queuePosition_t theQueuePosition);
	void BackupQueue();
	// void RestoreQueue();
	deviceId_t GetMainEvent();
	deviceId_t GetDelayEvent();
	bool IsNextMainQueue();

	deviceId_t GetEvent();
	eventKey_t QueueTime();
	inline deviceId_t QueueSize() { return (enqueueCount - dequeueCount); };
	bool Later(eventKey_t theEventKey);
	bool Later(eventKey_t theFirstKey, eventKey_t theSecondKey);

	void AddLeak(deviceId_t theDevice, CConnection& theConnections);
	void Print(string theIndentation = "");
	void PrintStatus(int theNextPrintCount = 1000000);
};


#endif /* CEVENTQUEUE_HH_ */
