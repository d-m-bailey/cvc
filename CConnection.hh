/*
 * CConnection.hh
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

#ifndef CCONNECTION_HH_
#define CCONNECTION_HH_

#include "Cvc.hh"

#include "CVirtualNet.hh"
#include "CPower.hh"
#include "CDevice.hh"

class CConnection {
public:
	netId_t sourceId = UNKNOWN_NET;
	netId_t gateId = UNKNOWN_NET;
	netId_t drainId = UNKNOWN_NET;
	netId_t bulkId = UNKNOWN_NET;
	CVirtualNet masterSourceNet;
	CVirtualNet masterDrainNet;
	CVirtualNet masterGateNet;
	CVirtualNet masterBulkNet;
	voltage_t sourceVoltage = UNKNOWN_VOLTAGE;
	voltage_t drainVoltage = UNKNOWN_VOLTAGE;
	voltage_t gateVoltage = UNKNOWN_VOLTAGE;
	voltage_t bulkVoltage = UNKNOWN_VOLTAGE;
	CDevice * device_p = NULL;
	deviceId_t deviceId = UNKNOWN_DEVICE;
	CPower * sourcePower_p = NULL;
	CPower * gatePower_p = NULL;
	CPower * drainPower_p = NULL;
	resistance_t resistance = INFINITE_RESISTANCE;

	inline bool IsUnknownSourceVoltage() { return ( sourceVoltage == UNKNOWN_VOLTAGE && ! (sourcePower_p && sourcePower_p->type[HIZ_BIT]) ); }
	inline bool IsUnknownGateVoltage() { return ( gateVoltage == UNKNOWN_VOLTAGE && ! (gatePower_p && gatePower_p->type[HIZ_BIT]) ); }
	inline bool IsUnknownDrainVoltage() { return ( drainVoltage == UNKNOWN_VOLTAGE && ! (drainPower_p && drainPower_p->type[HIZ_BIT]) ); }
//	inline bool IsUnknownBulkVoltage() { return ( bulkVoltage == UNKNOWN_VOLTAGE && ! (bulkPower_p && bulkPower_p->type[HIZ_BIT]) ); }


};

void AddConnectedDevices(netId_t theNetId, list<deviceId_t>& myPmosToCheck,	list<deviceId_t>& myNmosToCheck,
		list<deviceId_t>& myResistorToCheck, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v, vector<modelType_t>& theDeviceType_v );

class CFullConnection {
public:
	netId_t originalSourceId = UNKNOWN_NET;
	netId_t originalGateId = UNKNOWN_NET;
	netId_t originalDrainId = UNKNOWN_NET;
	netId_t originalBulkId = UNKNOWN_NET;
	netId_t sourceId = UNKNOWN_NET;
	netId_t gateId = UNKNOWN_NET;
	netId_t drainId = UNKNOWN_NET;
	netId_t bulkId = UNKNOWN_NET;
	CVirtualNet masterMinSourceNet;
	CVirtualNet masterMinDrainNet;
	CVirtualNet masterMinGateNet;
	CVirtualNet masterMinBulkNet;
	voltage_t minSourceVoltage = UNKNOWN_VOLTAGE;
	voltage_t minDrainVoltage = UNKNOWN_VOLTAGE;
	voltage_t minGateVoltage = UNKNOWN_VOLTAGE;
	voltage_t minBulkVoltage = UNKNOWN_VOLTAGE;
	CPower * minSourcePower_p = NULL;
	CPower * minDrainPower_p = NULL;
	CPower * minGatePower_p = NULL;
	CPower * minBulkPower_p = NULL;
	CVirtualNet masterMaxSourceNet;
	CVirtualNet masterMaxDrainNet;
	CVirtualNet masterMaxGateNet;
	CVirtualNet masterMaxBulkNet;
	voltage_t maxSourceVoltage = UNKNOWN_VOLTAGE;
	voltage_t maxDrainVoltage = UNKNOWN_VOLTAGE;
	voltage_t maxGateVoltage = UNKNOWN_VOLTAGE;
	voltage_t maxBulkVoltage = UNKNOWN_VOLTAGE;
	CPower * maxSourcePower_p = NULL;
	CPower * maxDrainPower_p = NULL;
	CPower * maxGatePower_p = NULL;
	CPower * maxBulkPower_p = NULL;
	CVirtualNet masterSimSourceNet;
	CVirtualNet masterSimDrainNet;
	CVirtualNet masterSimGateNet;
	CVirtualNet masterSimBulkNet;
	voltage_t simSourceVoltage = UNKNOWN_VOLTAGE;
	voltage_t simDrainVoltage = UNKNOWN_VOLTAGE;
	voltage_t simGateVoltage = UNKNOWN_VOLTAGE;
	voltage_t simBulkVoltage = UNKNOWN_VOLTAGE;
	CPower * simSourcePower_p = NULL;
	CPower * simDrainPower_p = NULL;
	CPower * simGatePower_p = NULL;
	CPower * simBulkPower_p = NULL;

	voltage_t minSourceLeakVoltage = UNKNOWN_VOLTAGE;
	voltage_t minDrainLeakVoltage = UNKNOWN_VOLTAGE;
	voltage_t minGateLeakVoltage = UNKNOWN_VOLTAGE;
	voltage_t minBulkLeakVoltage = UNKNOWN_VOLTAGE;
	voltage_t maxSourceLeakVoltage = UNKNOWN_VOLTAGE;
	voltage_t maxDrainLeakVoltage = UNKNOWN_VOLTAGE;
	voltage_t maxGateLeakVoltage = UNKNOWN_VOLTAGE;
	voltage_t maxBulkLeakVoltage = UNKNOWN_VOLTAGE;

	CDevice * device_p = NULL;
	deviceId_t deviceId = UNKNOWN_DEVICE;
	resistance_t resistance = INFINITE_RESISTANCE;

	float EstimatedCurrent();
	float EstimatedCurrent(bool theVthFlag);
	float EstimatedMinimumCurrent();
	bool CheckTerminalMinMaxVoltages(int theCheckTerminals, bool theCheckHiZFlag = true);
	bool CheckTerminalMinMaxLeakVoltages(int theCheckTerminals);
	bool CheckTerminalMinVoltages(int theCheckTerminals);
	bool CheckTerminalMaxVoltages(int theCheckTerminals);
	bool CheckTerminalSimVoltages(int theCheckTerminals);
	void SetUnknownVoltageToSim();
	void SetUnknownVoltage();
	void SetMinMaxLeakVoltages(CCvcDb * theCvcDb);
	bool IsPossibleHiZ(CCvcDb * theCvcDb);
	bool IsPumpCapacitor();

};
#endif /* CCONNECTION_HH_ */
