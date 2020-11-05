/*
 * CInstance.hh
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

#ifndef CINSTANCE_HH_
#define CINSTANCE_HH_

#include "Cvc.hh"

#include <unordered_map>
#include "CCircuit.hh"

class CCvcDb;

class CInstance {
public:
	deviceId_t	firstDeviceId = 0;
	instanceId_t	firstSubcircuitId = 0;
	netId_t		firstNetId = 0;
	union {
		instanceId_t  parallelInstanceCount = 0;  // parallel (maybe) instances kept
		instanceId_t  parallelInstanceId;  // for parallel instances deleted
	};

	CNetIdVector	localToGlobalNetId_v;

	/* The CInstance structure also doubles as a hash.
	   Each master has a vector of instances.
	*/
	instanceId_t	nextHashedInstanceId = UNKNOWN_DEVICE;
 
	instanceId_t	parentId = 0;
	CCircuit * master_p = NULL;
	bool	isMasked = false;

	void AssignTopGlobalIDs(CCvcDb * theCvcDb_p, CCircuit * theMaster_p);
	void AssignGlobalIDs(CCvcDb * theCvcDb_p, const instanceId_t theInstanceId, CDevice * theSubcircuit_p, const instanceId_t theParentId,
		CInstance * theParent_p, bool isParallel);
	bool IsParallelInstance() { return (localToGlobalNetId_v.size() == 0); };

	void Print(const instanceId_t theInstanceId, const string theIndentation = "");
};

class CInstancePtrVector : public vector<CInstance *> {
public:
	~CInstancePtrVector();
	void Clear();
};

#endif /* CINSTANCE_HH_ */
