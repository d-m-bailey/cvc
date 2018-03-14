/*
 * CPower.hh
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

#ifndef CPOWER_HH_
#define CPOWER_HH_

#include "Cvc.hh"
#include "CSet.hh"
#include "CModel.hh"

class CCvcDb;
class CEventQueue;
class CVirtualNetVector;

// status type bits
enum powerType_t { POWER_BIT=0, INPUT_BIT, HIZ_BIT, RESISTOR_BIT, REFERENCE_BIT, MIN_CALCULATED_BIT, SIM_CALCULATED_BIT, MAX_CALCULATED_BIT };
enum activeType_t { MIN_ACTIVE=0, MAX_ACTIVE, MIN_IGNORE, MAX_IGNORE };

enum calculationType_t { UNKNOWN_CALCULATION=0, NO_CALCULATION, UP_CALCULATION, DOWN_CALCULATION, RESISTOR_CALCULATION, ESTIMATED_CALCULATION };

#define IsExternalPower_(power_p) ((power_p)->type[POWER_BIT] || (power_p)->type[INPUT_BIT])
#define IsPriorityPower_(power_p) ((power_p)->type[POWER_BIT] || (power_p)->type[INPUT_BIT] || (power_p)->type[RESISTOR_BIT])

class CPowerFamilyMap : public unordered_map<string, unordered_set<string>> {
public:
	void AddFamily(string thePowerString);

};

class CPowerPtrMap; // forward definition
class CPowerPtrVector; // forward definition

class CPower {
public:
	static netId_t powerCount;

	netId_t powerId; // unique for each power net. used in leak detection
	string	powerSignal; // power name from power file (possibly bus or cell level net)
	string	powerAlias; // name used to represent this power definition (/VSS -> VSS, /X1/VDDA -> VDDA)
	CStatus	type;
	CStatus active;
	voltage_t	minVoltage = UNKNOWN_VOLTAGE;
	voltage_t	simVoltage = UNKNOWN_VOLTAGE;
	voltage_t	maxVoltage = UNKNOWN_VOLTAGE;
	netId_t netId = UNKNOWN_NET; // netId for this power definition
	// default nets are the nets that are used if the calculated voltage is invalid
	// the base from which voltages have been calculated
	// not used for min/max defaults to sim values
	// no default sim for resistance calculations
	netId_t	defaultMinNet = UNKNOWN_NET;
	netId_t	defaultSimNet = UNKNOWN_NET;
	netId_t	defaultMaxNet = UNKNOWN_NET;
	string	expectedSim = "";
	string	expectedMin = "";
	string	expectedMax = "";
	voltage_t pullDownVoltage = UNKNOWN_VOLTAGE;
	voltage_t pullUpVoltage = UNKNOWN_VOLTAGE;
	// base of calculations for calculated nets
//	netId_t	baseMinId = UNKNOWN_NET;
//	netId_t	baseSimId = UNKNOWN_NET;
//	netId_t	baseMaxId = UNKNOWN_NET;
	string	family = "";
	string	definition = "";
	calculationType_t minCalculationType = UNKNOWN_CALCULATION;
	calculationType_t simCalculationType = UNKNOWN_CALCULATION;
	calculationType_t maxCalculationType = UNKNOWN_CALCULATION;
	bool relativeFriendly = true;
	CSet	relativeSet;
	bool	printed = false;
	bool    flagAllShorts = false;

	CPower();
	CPower(string thePowerString, CPowerPtrMap & thePowerMacroPtrMap, CModelListMap & theModelListMap);
	CPower(CPower * thePower_p, netId_t theNetId);
	CPower(netId_t theNetId);
	CPower(netId_t theNetId, voltage_t theSimVoltage);
//	CPower(netId_t theNetId, string theNetName, voltage_t theNewVoltage, netId_t theMinNet, netId_t theMaxNet, string theCalculation);
//	CPower(voltage_t theMinVoltage, voltage_t theSimVoltage, voltage_t theMaxVoltage);
	CPower(netId_t theNetId, voltage_t theMinVoltage, voltage_t theSimVoltage, voltage_t theMaxVoltage, netId_t theDefaultMinNet, netId_t theDefaultSimNet, netId_t theDefaultMaxNet);

	CPower * GetBasePower(CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v);
//	CPower * GetMinBasePower(CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v);
//	CPower * GetSimBasePower(CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v);
//	CPower * GetMaxBasePower(CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v);
	void SetPowerAlias(string thePowerString, size_t theAliasStart);
	bool IsSamePower(CPower * theMatchPower);
	bool IsRelative(CPower * theTestPower_p, bool theDefault);
	bool IsRelatedPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v, CVirtualNetVector & theTestNet_v, bool theDefault);

//	bool IsRelatedMinPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v);
//	bool IsRelatedSimPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v);
//	bool IsRelatedMaxPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v);
//	bool RelatedPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theMinNet_v, CVirtualNetVector & theSimNet_v, CVirtualNetVector & theMaxNet_v);
//
//	bool RelatedPower(CPower * theTestPower_p);
	inline bool IsInternalOverride() {return( ! (type[INPUT_BIT] || type[POWER_BIT]) ); };

	void Print(ostream & theLogFile, string theIndentation = "", string theRealPowerName = "");
	string PowerDefinition();
	string StandardDefinition();
	voltage_t RelativeVoltage(CPowerPtrMap & thePowerMacroPtrMap, netStatus_t theType, CModelListMap & theModelListMap);
};

#define PowerDelimiter_(power_p, BIT) ((power_p == NULL || power_p->type[BIT]) ? "=" : "@")
#define IsDefinedVoltage_(power_p) (power_p && netStatus_v[power_p->baseNetId] && \
		( netStatus_v[power_p->baseNetId][MIN_POWER] \
		    || netStatus_v[power_p->baseNetId][SIM_POWER] \
		    || netStatus_v[power_p->baseNetId][MAX_POWER] \
     		|| power_p->type[HIZ_BIT] ) )
#define IsCalculatedVoltage_(power_p) (power_p && ( power_p->type[MIN_CALCULATED_BIT] || power_p->type[SIM_CALCULATED_BIT] || power_p->type[MAX_CALCULATED_BIT] ) )
#define IsKnownVoltage_(theVoltage) (theVoltage != UNKNOWN_VOLTAGE)

class CPowerPtrMap : public unordered_map<string, CPower *> {
public:
	string CalculateExpectedValue(string theEquation, netStatus_t theType, CModelListMap & theModelListMap);
	voltage_t CalculateVoltage(string theEquation, netStatus_t theType, CModelListMap & theModelListMap, bool thePermitUndefinedFlag = false);
};

class CPowerPtrList : public list<CPower *> {
public:
	CPowerPtrMap	calculatedPowerPtrMap;

	void Clear();
	void SetPowerLimits(voltage_t& theMaxPower, voltage_t& theMinPower);
//	CPower * FindCalculatedPower(CEventQueue& theEventQueue, CPower * theCurrentPower_p, voltage_t theShortVoltage, string theNetName);
//	CPower * FindCalculatedPower(CEventQueue& theEventQueue, CPower * theCurrentPower_p, voltage_t theShortVoltage, netId_t theNetId, netId_t theDefaultNetId, CCvcDb * theCvcDb);
//	CPower * FindCalculatedPower(voltage_t theMinVoltage, voltage_t theSimVoltage, voltage_t theMaxVoltage, netId_t theNetId, CCvcDb * theCvcDb);
	void SetFamilies(CPowerFamilyMap & thePowerFamilyMap);
};

class CPowerPtrVector : public vector<CPower *> {
public:
	voltage_t MaxVoltage(netId_t theNetId);
	voltage_t MinVoltage(netId_t theNetId);
	voltage_t SimVoltage(netId_t theNetId);
	void CalculatePower(CEventQueue& theEventQueue, voltage_t theShortVoltage, netId_t theNetId, netId_t theDefaultNetId, CCvcDb * theCvdDb_p, string theCalculation);
};

class CInstancePower {
public:
	string	instanceName;
	string	powerFile;
	list<string>	powerList;

	CInstancePower(string theDefinition);
};

class CInstancePowerPtrList : public list<CInstancePower *> {
public:

};

#define IsUnknownMinVoltage_(power_p) ( ! power_p || power_p->minVoltage == UNKNOWN_VOLTAGE || power_p->active[MIN_ACTIVE] == false )
#define IsUnknownMaxVoltage_(power_p) ( ! power_p || power_p->maxVoltage == UNKNOWN_VOLTAGE || power_p->active[MAX_ACTIVE] == false )

#endif /* CPOWER_HH_ */
