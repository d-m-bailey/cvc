/*
 * CConnectionCount.cc
 *
 * Copyright 2014-2107 D. Mitch Bailey  d.mitch.bailey at gmail dot com
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

#include "CConnectionCount.hh"

#include "CCvcDb.hh"

CDeviceCount::CDeviceCount(netId_t theNetId, CCvcDb * theCvcDb_p) {
// count devices attached to theNet
	for ( auto device_it = theCvcDb_p->firstSource_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb_p->nextSource_v[device_it] ) {
		switch( theCvcDb_p->deviceType_v[device_it] ) {
		case NMOS:
		case LDDN: { nmosCount++; break; }
		case PMOS:
		case LDDP: { pmosCount++; break; }
		case RESISTOR: { resistorCount++; break; }
		case CAPACITOR: { capacitorCount++; break; }
		case DIODE: { diodeCount++; break; }
		default: break;
		}
	}
	for ( auto device_it = theCvcDb_p->firstDrain_v[theNetId]; device_it != UNKNOWN_DEVICE; device_it = theCvcDb_p->nextDrain_v[device_it] ) {
		switch( theCvcDb_p->deviceType_v[device_it] ) {
		case NMOS:
		case LDDN: { nmosCount++; break; }
		case PMOS:
		case LDDP: { pmosCount++; break; }
		case RESISTOR: { resistorCount++; break; }
		case CAPACITOR: { capacitorCount++; break; }
		case DIODE: { diodeCount++; break; }
		default: break;
		}
	}
}
