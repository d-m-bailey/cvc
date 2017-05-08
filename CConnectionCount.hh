/*
 * CConnectionCount.hh
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

#ifndef CCONNECTIONCOUNT_HH_
#define CCONNECTIONCOUNT_HH_

#include "Cvc.hh"

class CConnectionCount {
public:
	deviceId_t sourceCount = 0;
	deviceId_t drainCount = 0;
	deviceId_t gateCount = 0;
	deviceId_t bulkCount = 0;
	CStatus sourceDrainType = bitset<8>(0);

	inline deviceId_t SourceDrainCount() { return sourceCount + drainCount; };
};

class CConnectionCountVector : public vector<CConnectionCount> {
public:
};

#include "CCvcDb.hh"

class CDeviceCount {
public:
	deviceId_t resistorCount = 0;
	deviceId_t nmosCount = 0;
	deviceId_t pmosCount = 0;
	deviceId_t capacitorCount = 0;
	deviceId_t diodeCount = 0;

	CDeviceCount(netId_t theNetId, CCvcDb * theCvcDb);
};

#endif /* CCONNECTIONCOUNT_HH_ */
