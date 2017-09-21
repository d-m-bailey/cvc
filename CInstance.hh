/*
 * CInstance.hh
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

#ifndef CINSTANCE_HH_
#define CINSTANCE_HH_

#include "Cvc.hh"

#include "CCircuit.hh"

class CCvcDb;

class CInstance {
public:
	deviceId_t	firstDeviceId = 0;
	instanceId_t	firstSubcircuitId = 0;
	netId_t		firstNetId = 0;

	CNetIdVector	localToGlobalNetId_v;

	instanceId_t	parentId = 0;
	CCircuit * master_p = NULL;

	void AssignTopGlobalIDs(CCvcDb * theCvcDb_p, CCircuit * theMaster_p);
	void AssignGlobalIDs(CCvcDb * theCvcDb_p, const instanceId_t theInstanceId, const CDevice * theSubcircuit_p, const instanceId_t theParentId, const CInstance * theParent_p);

	void Print(const instanceId_t theInstanceId, const string theIndentation = "");
};

#endif /* CINSTANCE_HH_ */
