/*
 * CPower.cc
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

#include "CPower.hh"

#include "CEventQueue.hh"
#include "CCvcDb.hh"

netId_t CPower::powerCount = 0;
CFixedText CPower::powerDefinitionText;

CExtraPowerData::CExtraPowerData() {
	powerSignal = CPower::powerDefinitionText.BlankTextAddress();
	powerAlias = CPower::powerDefinitionText.BlankTextAddress();
}

CPower::CPower() {
	powerId = powerCount++;
	definition = CPower::powerDefinitionText.BlankTextAddress();
}

CPower::CPower(CPower * thePower_p) {
	// Similar to byte-wise copy, but duplicates extraData;
	*this = *thePower_p;
	if ( extraData ) {
		extraData = new CExtraPowerData(*(thePower_p->extraData));
	}
}

CPower::CPower(CPower * thePower_p, netId_t theNetId) {
	// copies power for expanding power short cuts *(cell)/net
	// use before family expansion
	powerId = powerCount++;
	minVoltage = thePower_p->minVoltage;
	maxVoltage = thePower_p->maxVoltage;
	simVoltage = thePower_p->simVoltage;
	if ( thePower_p->extraData ) {
		extraData = new CExtraPowerData;
		extraData->powerSignal = thePower_p->powerSignal();
		extraData->powerAlias = thePower_p->powerAlias();
		extraData->expectedSim = thePower_p->expectedSim();
		extraData->expectedMin = thePower_p->expectedMin();
		extraData->expectedMax = thePower_p->expectedMax();
		extraData->family = thePower_p->family();
		extraData->implicitFamily = thePower_p->implicitFamily();
		extraData->relativeSet = thePower_p->extraData->relativeSet;
	}
	relativeFriendly = thePower_p->relativeFriendly;
	type = thePower_p->type;
	definition = thePower_p->definition;
	active = thePower_p->active;
	netId = theNetId;
	minCalculationType = thePower_p->minCalculationType;
	simCalculationType = thePower_p->simCalculationType;
	maxCalculationType = thePower_p->maxCalculationType;
}

CPower::CPower(string thePowerString, CPowerPtrMap & thePowerMacroPtrMap, CModelListMap & theModelListMap) {
	// for power definitions
	// Valid power definitions
	// # ...	<- comment
	// VSS 0.0	<- power definition min=sim=max=0.0
	// VDD 1.2	<- power definition min=sim=max=1.2
	// VBB -0.5	<- power definition min=sim=max=-0.5
	// A1 min@0.0 sim@0.6 max@1.2 <- input/output/internal signal min/sim/max.
	//									missing values are calculated.
	// O1 min@0.0 expect@1.2 max@1.2 <- input/output/internal signal min/max.
	//									sim value is calculated and checked against expect.
	//									missing min/max values are calculated.
	// VDDX min@0.0 max@1.2 open permit@VSS	<- cutoff power definition (floating), unknown sim definition, path to VSS ok
	// VSSX min@0.0 max@1.2 open permit@VDD	<- cutoff power definition (floating), unknown sim definition, path to VDD ok
	// VDDZ min@1.2 max@1.2 open	<- SCRC power definition, no paths permitted
	// VSSZ min@0.0 max@0.0 open	<- SCRC power definition, no paths permitted
	// active usage: only use min/max power marked as active
	//   sim values imply defined min/max active
	//   open, input also imply defined min/max active
	//   other min/max overrides must have path to lower/higher value to become active
	//   reset before min/max calculations for unknown sim values.
	// powerAlias: all top level power nets have alias for power families VSS->VSS  /VDDA->VDDA
	//   explicit overrides permitted /X7/VDD~>VDD7
	// TODO: add power families
	// VPP 2.6 permit@vdd			<- power definition with explicit permitted connections
	// VNWL -0.2 prohibit@VSS		<- power definition with explicit prohibited connections
	// family vdd VDD,VSS,...		<- family definition
	// sanity checks
	// TODO: power definitions determine default families
	thePowerString = trim_(thePowerString);
	string myParameterName;
	string myParameterValue;
	size_t	myAtIndex;
	size_t	myStringBegin = thePowerString.find_first_not_of(" \t");
	size_t	myStringEnd = thePowerString.find_first_of(" \t", myStringBegin);
	type = 0;
	powerId = powerCount++;
	extraData = new CExtraPowerData;
	size_t myAliasBegin = thePowerString.find(ALIAS_DELIMITER);
	extraData->powerSignal = CPower::powerDefinitionText.SetTextAddress((text_t)(thePowerString.substr(myStringBegin, min(myStringEnd, myAliasBegin))).c_str());
//	definition = thePowerString.substr(thePowerString.find_first_not_of(" \t", myStringEnd));
	string myDefinition = "";
	string myImplicitFamily = "";
	while (myStringEnd < thePowerString.length()) {
		myStringBegin = thePowerString.find_first_not_of(" \t", myStringEnd);
		myStringEnd = thePowerString.find_first_of(" \t", myStringBegin);
		myAtIndex = thePowerString.find("@", myStringBegin);
		if ( myAtIndex < myStringEnd ) {
			myParameterName = thePowerString.substr(myStringBegin, myAtIndex - myStringBegin);
			myParameterValue = thePowerString.substr(myAtIndex + 1, myStringEnd - (myAtIndex + 1));
//			cout << "parameter '" << myParameterName << "' = '" << myParameterValue << "'" << endl;
			if ( myParameterName == "min" ) {
				minVoltage = thePowerMacroPtrMap.CalculateVoltage(myParameterValue, MIN_POWER, theModelListMap);
				myImplicitFamily += thePowerMacroPtrMap.implicitFamily;
				minCalculationType = NO_CALCULATION;
			}
			if ( myParameterName == "max" ) {
				maxVoltage = thePowerMacroPtrMap.CalculateVoltage(myParameterValue, MAX_POWER, theModelListMap);
				myImplicitFamily += thePowerMacroPtrMap.implicitFamily;
				maxCalculationType = NO_CALCULATION;
			}
			if ( myParameterName == "sim" ) {
				simVoltage = thePowerMacroPtrMap.CalculateVoltage(myParameterValue, SIM_POWER, theModelListMap);
				myImplicitFamily += thePowerMacroPtrMap.implicitFamily;
				simCalculationType = NO_CALCULATION;
			}
			if ( myParameterName == "expectMin" ) extraData->expectedMin = thePowerMacroPtrMap.CalculateExpectedValue(myParameterValue, MIN_POWER, theModelListMap);
			if ( myParameterName == "expectMax" ) extraData->expectedMax = thePowerMacroPtrMap.CalculateExpectedValue(myParameterValue, MAX_POWER, theModelListMap);
			if ( myParameterName == "expectSim" ) extraData->expectedSim = thePowerMacroPtrMap.CalculateExpectedValue(myParameterValue, SIM_POWER, theModelListMap);
			if ( myParameterName == "permit" || myParameterName == "prohibit" ) {
				if ( thePowerMacroPtrMap.count(myParameterValue) && ! IsEmpty(thePowerMacroPtrMap[myParameterValue]->powerAlias()) ) {
					extraData->family = thePowerMacroPtrMap[myParameterValue]->powerAlias();
				} else {
					extraData->family = myParameterValue;
				}
				relativeFriendly = ( myParameterName == "permit" ) ? true : false;
			}
			myDefinition += " " + myParameterName + "@" + myParameterValue;
		} else {
			myParameterName = thePowerString.substr(myStringBegin, myStringEnd - myStringBegin);
			myDefinition += " " + myParameterName;
//			cout << "parameter '" << myParameterName << "'" << endl;
			if ( myParameterName == "input" ) {
				type[INPUT_BIT] = true;
			} else if ( myParameterName == "power" ) {
				type[POWER_BIT] = true;
				if ( IsEmpty(powerAlias()) ) {
					SetPowerAlias(thePowerString, myAliasBegin);
				}
			} else if ( myParameterName == "resistor" ) {
				type[RESISTOR_BIT] = true;
			} else if ( myParameterName == "reference" ) {
				type[REFERENCE_BIT] = true;
			} else if ( myParameterName == "open" ) {
//				type[FIXED_BIT] = true;
				type[HIZ_BIT] = true;
				simVoltage = UNKNOWN_VOLTAGE;
			} else if ( IsValidVoltage_(myParameterName) ) { // voltage
//				type[FIXED_BIT] = true;
				simVoltage = String_to_Voltage(myParameterName);
				maxVoltage = simVoltage;
				minVoltage = simVoltage;
			} else if ( thePowerMacroPtrMap.count(myParameterName) > 0 ) { // macro
//				type = thePowerMacroPtrMap[myParameterName]->type;
				minVoltage = thePowerMacroPtrMap[myParameterName]->minVoltage;
				maxVoltage = thePowerMacroPtrMap[myParameterName]->maxVoltage;
				simVoltage = thePowerMacroPtrMap[myParameterName]->simVoltage;
				myImplicitFamily += "," + myParameterName;
				if ( ! IsEmpty(thePowerMacroPtrMap[myParameterName]->family()) ) {
					myImplicitFamily += "," + thePowerMacroPtrMap[myParameterName]->family();
				}
				if ( thePowerMacroPtrMap[myParameterName]->extraData ) {
					if (! extraData) extraData = new CExtraPowerData;
					extraData->expectedMin = thePowerMacroPtrMap[myParameterName]->expectedMin();
					extraData->expectedMax = thePowerMacroPtrMap[myParameterName]->expectedMax();
					extraData->expectedSim = thePowerMacroPtrMap[myParameterName]->expectedSim();
					extraData->family = thePowerMacroPtrMap[myParameterName]->family();
				}
				relativeFriendly = thePowerMacroPtrMap[myParameterName]->relativeFriendly;
				type[HIZ_BIT] = thePowerMacroPtrMap[myParameterName]->type[HIZ_BIT];
				extraData->powerAlias = CPower::powerDefinitionText.SetTextAddress((text_t)myParameterName.c_str());
			} else { // formula
//				type[FIXED_BIT] = true;
				simVoltage = thePowerMacroPtrMap.CalculateVoltage(myParameterName, SIM_POWER, theModelListMap);
				maxVoltage = thePowerMacroPtrMap.CalculateVoltage(myParameterName, MAX_POWER, theModelListMap);
				minVoltage = thePowerMacroPtrMap.CalculateVoltage(myParameterName, MIN_POWER, theModelListMap);
				myImplicitFamily += thePowerMacroPtrMap.implicitFamily;
			}
		}
	}
//	definition = StandardDefinition();
//	if ( ! type[FIXED_BIT] && minVoltage == UNKNOWN_VOLTAGE && maxVoltage == UNKNOWN_VOLTAGE && simVoltage == UNKNOWN_VOLTAGE &&
//			( expectedSim != "" || expectedMin != "" || expectedMax != "" )) {
//		type[EXPECTED_ONLY_BIT] = true;
//	}
	definition = CPower::powerDefinitionText.SetTextAddress((text_t)myDefinition.c_str());
	if ( type[HIZ_BIT] ) {
		if ( IsEmpty(extraData->family) ) extraData->family = "cvc-noleak";
	}
	if ( myImplicitFamily != "" ) {
		extraData->implicitFamily = myImplicitFamily.substr(1);  // ,net1,net2 -> net1,net2
//		cout << "DEBUG: " << extraData->powerSignal << " implicit family \"" << extraData->implicitFamily << "\"" << endl;
	}
}

CPower::CPower(netId_t theNetId) {
	// generic Power pointer for resistor propagation and calculated nets
	powerId = powerCount++;
//	powerSignal = "theNetName"; // TODO: Check literal
//	simVoltage = theNewVoltage;
//	maxVoltage = theNewVoltage;
//	minVoltage = theNewVoltage;
	netId = theNetId;
//	type[RESISTOR_BIT] = true;
//	type[MIN_CALCULATED_BIT] = true;
//	type[SIM_CALCULATED_BIT] = true;
//	type[MAX_CALCULATED_BIT] = true;
	definition = CPower::powerDefinitionText.BlankTextAddress();
}

CPower::CPower(netId_t theNetId, voltage_t theSimVoltage, bool theCreateExtraData) {
	// for set sim level to thePowerNetId
	powerId = powerCount++;
//	powerSignal = "theNetName"; // TODO: Check literal
	simVoltage = theSimVoltage;
//	maxVoltage = theNewVoltage;
//	minVoltage = theNewVoltage;
	netId = theNetId;
//	type[RESISTOR_BIT] = true;
//	type[MIN_CALCULATED_BIT] = true;
//	type[SIM_CALCULATED_BIT] = true;
//	type[MAX_CALCULATED_BIT] = true;
	flagAllShorts = true;
	simCalculationType = ESTIMATED_CALCULATION;
	definition = CPower::powerDefinitionText.BlankTextAddress();
	if ( theCreateExtraData ) extraData = new CExtraPowerData;
}

CPower::CPower(netId_t theNetId, voltage_t theMinVoltage, voltage_t theSimVoltage, voltage_t theMaxVoltage, netId_t theDefaultMinNet, netId_t theDefaultSimNet, netId_t theDefaultMaxNet) {
	powerId = powerCount++;
//	powerSignal = "";
	simVoltage = theSimVoltage;
	maxVoltage = theMaxVoltage;
	minVoltage = theMinVoltage;
	defaultMinNet = theDefaultMinNet;
	defaultSimNet = theDefaultSimNet;
	defaultMaxNet = theDefaultMaxNet;
	netId = theNetId;
	definition = CPower::powerDefinitionText.BlankTextAddress();
	// type set in calling routine
//	type[MIN_CALCULATED_BIT] = true;
//	type[SIM_CALCULATED_BIT] = true;
//	type[MAX_CALCULATED_BIT] = true;
}

CPower::~CPower() {
	if ( extraData ) delete extraData;
}

void CPower::SetPowerAlias(string thePowerString, size_t theAliasStart) {
	size_t myLastHierarchy = string(powerSignal()).find_last_of(HIERARCHY_DELIMITER);
	string myPowerAlias = "";
	if ( theAliasStart < thePowerString.length() ) {
		size_t myAliasEnd = thePowerString.find_first_of(" \t", theAliasStart);
		myPowerAlias = thePowerString.substr(theAliasStart + 2, myAliasEnd - theAliasStart - 2);
	} else if ( myLastHierarchy < strlen(powerSignal()) ) {
		if ( myLastHierarchy == 0 ) { // only set default powerAlias for top signals
			myPowerAlias = string(powerSignal()).substr(myLastHierarchy + 1);
		}
	} else {
		myPowerAlias = string(powerSignal());
	}
	size_t myBusStart = myPowerAlias.find_first_of("{[(<");
	if ( myBusStart < myPowerAlias.length() ) {
		myPowerAlias = myPowerAlias.substr(0, myBusStart);
	}
	extraData->powerAlias = CPower::powerDefinitionText.SetTextAddress((text_t)myPowerAlias.c_str());
}

bool CPower::IsSamePower(CPower * theMatchPower) {
	return ( type == theMatchPower->type &&
			minVoltage == theMatchPower->minVoltage &&
			simVoltage == theMatchPower->simVoltage &&
			maxVoltage == theMatchPower->maxVoltage &&
			expectedMin() == theMatchPower->expectedMin() &&
			expectedSim() == theMatchPower->expectedSim() &&
			expectedMax() == theMatchPower->expectedMax() &&
			family() == theMatchPower->family() &&
			relativeFriendly == theMatchPower->relativeFriendly);
}

bool CPower::IsValidSubset(CPower * theMatchPower, voltage_t theThreshold) {
	return ( theMatchPower->type == NO_TYPE &&
			abs(minVoltage - theMatchPower->minVoltage) <= theThreshold &&
			abs(simVoltage - theMatchPower->simVoltage) <= theThreshold &&
			abs(maxVoltage - theMatchPower->maxVoltage) <= theThreshold &&
			theMatchPower->expectedMin() == "" &&
			theMatchPower->expectedSim() == "" &&
			theMatchPower->expectedMax() == "" &&
			theMatchPower->family() == "");
}

void CPowerFamilyMap::AddFamily(string thePowerString) {
	// Valid power family definitions
	// family vdd VDD,VSS,...		<- family definition
	// sanity checks
	string myFamilyName;
	string myRelativeName;
	size_t	myStringBegin = thePowerString.find_first_not_of(" \t");
	size_t	myStringEnd = thePowerString.find_first_of(" \t", myStringBegin);
	assert(thePowerString.substr(myStringBegin, myStringEnd - myStringBegin) == "family");
	myStringBegin = thePowerString.find_first_not_of(" \t", myStringEnd);
	myStringEnd = thePowerString.find_first_of(" \t", myStringBegin);
	myFamilyName = thePowerString.substr(myStringBegin, myStringEnd - myStringBegin);
	string myDefinition = thePowerString.substr(myStringEnd);
	if ( this->count(myFamilyName) ) {
		if ( definition != myDefinition ) {
			cout << "DEBUG: duplicate family definition " << myFamilyName << " original " << definition << " new " << myDefinition << endl;
			assert(definition == myDefinition);
		}
		return;
	}
	while (myStringEnd < thePowerString.length()) {
		myStringBegin = thePowerString.find_first_not_of(", \t", myStringEnd);
		myStringEnd = thePowerString.find_first_of(",", myStringBegin);
		if ( myStringBegin < myStringEnd ) {
			myRelativeName = thePowerString.substr(myStringBegin, myStringEnd - myStringBegin);
			(*this)[myFamilyName].insert(CPower::powerDefinitionText.SetTextAddress((text_t)myRelativeName.c_str()));
		}
	}
}

CPower * CPower::GetBasePower(CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v) {
	CPower * myPower_p = this;
	netId_t myNetId = netId;
	netId_t myDefaultNet = UNKNOWN_NET;
	while ( myPower_p && ( myPower_p->type[theNet_v.calculatedBit] || myNetId != theNet_v[myNetId].nextNetId ) ) {
		switch ( theNet_v.calculatedBit ) {
		case MIN_CALCULATED_BIT: { myDefaultNet = myPower_p->defaultMinNet; break; }
		case SIM_CALCULATED_BIT: { myDefaultNet = myPower_p->defaultSimNet; break; }
		case MAX_CALCULATED_BIT: { myDefaultNet = myPower_p->defaultMaxNet; break; }
		default: break;
		}
		if ( myPower_p->type[theNet_v.calculatedBit] ) {
			if ( myDefaultNet != UNKNOWN_NET ) {
				myNetId = myDefaultNet;
			} else if ( myNetId == theNet_v[myNetId].nextNetId ) {  // no default net at master net
				break;
			}
		}
		while ( myNetId != theNet_v[myNetId].nextNetId ) {
			myNetId = theNet_v[myNetId].nextNetId;
		}
		myPower_p = theNetVoltagePtr_v[myNetId];
	}
	return(myPower_p);
}

/*
CPower * CPower::GetMinBasePower(CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v) {
	Cpower * myPower_p = this;
	netId_t myNetId = netId;
	while ( myPower_p && ( myPower_p->type[MIN_CALCULATED_BIT] || myNetId != theNet_v[myNetId].nextNetId ) ) {
		if ( myPower_p->type[MIN_CALCULATED_BIT] && myPower_p->defaultMinNet != UNKNOWN_NET ) myNetId = myPower_p->defaultMinNet;
		while ( myNetId != theNet_v[myNetId].nextNetId ) {
			myNetId = theNet_v[myNetId].nextNetId;
		}
		myPower_p = theNetVoltagePtr_v[myNetId];
	}
	return(myPower_p);
}

CPower * CPower::GetSimBasePower(CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v) {
	Cpower * myPower_p = this;
	netId_t myNetId = netId;
	while ( myPower_p && ( myPower_p->type[SIM_CALCULATED_BIT] || myNetId != theNet_v[myNetId].nextNetId ) ) {
		if ( myPower_p->type[SIM_CALCULATED_BIT] && myPower_p->defaultSimNet != UNKNOWN_NET ) myNetId = myPower_p->defaultSimNet;
		while ( myNetId != theNet_v[myNetId].nextNetId ) {
			myNetId = theNet_v[myNetId].nextNetId;
		}
		myPower_p = theNetVoltagePtr_v[myNetId];
	}
	return(myPower_p);
}

CPower * CPower::GetMaxBasePower(CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v) {
	Cpower * myPower_p = this;
	netId_t myNetId = netId;
	while ( myPower_p && ( myPower_p->type[MAX_CALCULATED_BIT] || myNetId != theNet_v[myNetId].nextNetId ) ) {
		if ( myPower_p->type[MAX_CALCULATED_BIT] && myPower_p->defaultMaxNet != UNKNOWN_NET ) myNetId = myPower_p->defaultMaxNet;
		while ( myNetId != theNet_v[myNetId].nextNetId ) {
			myNetId = theNet_v[myNetId].nextNetId;
		}
		myPower_p = theNetVoltagePtr_v[myNetId];
	}
	return(myPower_p);
}

*/
bool CPower::IsRelative(CPower * theTestPower_p, bool theDefault, bool theIsHiZRelative) {
	if ( ! theTestPower_p ) return theDefault;
	if ( theIsHiZRelative && (type[HIZ_BIT] || theTestPower_p->type[HIZ_BIT]) ) return true;  // HiZ is always family for certain checks

	CSet emptySet;
	CSet & myRelativeSet = (extraData) ? extraData->relativeSet : emptySet;
	CSet & myTestRelativeSet = (theTestPower_p->extraData) ? theTestPower_p->extraData->relativeSet : emptySet;
	if ( myRelativeSet.empty() && myTestRelativeSet.empty() ) return theDefault;
	bool myFriend = false, myEnemy = false, myTestFriend = false, myTestEnemy = false;
	bool myHasFriends = false, myHasEnemies = false, myTestHasFriends = false, myTestHasEnemies = false;
	// relative sets are for explicit and implicit connections. explicit overrides implicit. only check if explicit definition exists. ie implicit, test->implicit -> no check

	if ( ! IsEmpty(family()) ) {
		bool myRelation = ( myRelativeSet.count(theTestPower_p->powerSignal())
				|| myRelativeSet.count(theTestPower_p->powerAlias())
				|| ( ! myTestRelativeSet.empty()
						&& myRelativeSet.Intersects(myTestRelativeSet) ) );
		if ( relativeFriendly ) {
			myHasFriends = true;
			myFriend = myRelation;
		} else {
			myHasEnemies = true;
			myEnemy = myRelation;
		}
	}
	if ( ! IsEmpty(theTestPower_p->family()) ) {
		bool myRelation = ( myTestRelativeSet.count(powerSignal())
				|| myTestRelativeSet.count(powerAlias())
				|| ( ! myRelativeSet.empty()
						&& myTestRelativeSet.Intersects(myRelativeSet) ) );
		if ( theTestPower_p->relativeFriendly ) {
			myTestHasFriends = true;
			myTestFriend = myRelation;
		} else {
			myTestHasEnemies = true;
			myTestEnemy = myRelation;
		}
	}
	if ( myHasFriends && myTestHasFriends ) return ( myFriend || myTestFriend );
	if ( myHasFriends && myTestHasEnemies ) {
		if ( myTestEnemy ) return false;
		if ( myFriend ) return true;
		return theDefault;
	}
	if ( myHasEnemies && myTestHasFriends ) {
		if ( myEnemy ) return false;
		if ( myTestFriend ) return true;
		return theDefault;
	}
	if ( myHasEnemies && myTestHasEnemies ) return ! ( myEnemy || myTestEnemy );
	if ( myHasFriends ) return myFriend;
	if ( myHasEnemies ) return ! myEnemy;
	if ( myTestHasFriends ) return myTestFriend;
	/*if ( myTestHasEnemies )*/ return ! myTestEnemy;
}

bool CPower::IsRelatedPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v, CVirtualNetVector & theTestNet_v,
		bool theDefault, bool theIsHiZRelative) {
	if ( ! theTestPower_p ) return theDefault;
	CPower * myPower_p = GetBasePower(theNetVoltagePtr_v, theNet_v);
	CPower * myTestPower_p = theTestPower_p->GetBasePower(theNetVoltagePtr_v, theTestNet_v);
	assert( myPower_p && myTestPower_p );
	if ( myPower_p == myTestPower_p ) return true;
	if ( ! IsEmpty(myPower_p->powerAlias())
			&& (myPower_p->powerAlias() == myTestPower_p->powerAlias() || myPower_p->powerAlias() == myTestPower_p->powerSignal()) ) return true;
	if ( ! IsEmpty(myTestPower_p->powerAlias()) && myTestPower_p->powerAlias() == myPower_p->powerSignal()) return true;
	return myPower_p->IsRelative(myTestPower_p, theDefault, theIsHiZRelative);
}

/*
bool CPower::IsRelatedMinPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v) {
	CPower * myPower_p = GetMinBasePower(theNetVoltagePtr_v, theNet_v);
	CPower * myMinTestPower_p = theTestPower_p->GetMinBasePower(theNetVoltagePtr_v, theNet_v);
	if ( myPower_p && myTestPower_p ) {
		if ( myMinPower_p == myMinTestPower_p ) return true;
		return myMinPower_p->IsRelative(myMinTestPower_p, false);
	}
	return false;
}

bool CPower::IsRelatedSimPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v) {
	CPower * myPower_p = GetSimBasePower(theNetVoltagePtr_v, theNet_v);
	CPower * myTestPower_p = theTestPower_p->GetSimBasePower(theNetVoltagePtr_v, theNet_v);
	if ( myMinPower_p && myTestPower_p ) {
		if ( myPower_p == myTestPower_p ) return true;
		return myPower_p->IsRelative(myTestPower_p, false);
	}
	return false;
}

bool CPower::IsRelatedMaxPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v) {
	CPower * myPower_p = GetMaxBasePower(theNetVoltagePtr_v, theNet_v);
	CPower * myTestPower_p = theTestPower_p->GetMaxBasePower(theNetVoltagePtr_v, theNet_v);
	if ( myPower_p && myTestPower_p ) {
		if ( myPower_p == myTestPower_p ) return true;
		return myPower_p->IsRelative(myTestPower_p, false);
	}
	return false;
}

bool CPower::RelatedPower(CPower * theTestPower_p, CPowerPtrVector & theNetVoltagePtr_v, CVirtualNetVector & theNet_v, CVirtualNetVector & theTestNet_v) {
	if ( this == theTestPower_p ) return true;
	// only calculated nets have default*Net (derived net) settings. Power is related if base net = default net.
	bool myMinRelated = false, mySimRelated = false, myMaxRelated = false;
	bool myCheckedPower = false;
	CPower * myMinPower_p = GetMinBasePower(theNetVoltagePtr_v, theMinNet_v);
	CPower * myMinTestPower_p = theTestPower_p->GetMinBasePower(theNetVoltagePtr_v, theMinNet_v);
	CPower * mySimPower_p = GetSimBasePower(theNetVoltagePtr_v, theSimNet_v);
	CPower * mySimTestPower_p = theTestPower_p->GetSimBasePower(theNetVoltagePtr_v, theSimNet_v);
	CPower * myMaxPower_p = GetMaxBasePower(theNetVoltagePtr_v, theMaxNet_v);
	CPower * myMaxTestPower_p = theTestPower_p->GetMaxBasePower(theNetVoltagePtr_v, theMaxNet_v);
	if ( myMinPower_p && myMinTestPower_p ) {
		myCheckedPower = true;
		if ( myMinPower_p == myMinTestPower_p ) return true;
		myMinRelated = myMinPower_p->IsRelative(myMinTestPower_p, true);
	}
	if ( mySimPower_p && mySimTestPower_p ) {
		myCheckedPower = true;
		if ( mySimPower_p == mySimTestPower ) return true;
		mySimRelated = mySimPower_p->IsRelative(mySimTestPower_p, true);
	}
	if ( myMaxPower_p && myMaxTestPower_p ) {
		myCheckedPower = true;
		if ( myMaxPower_p == myMaxTestPower ) return true;
		myMaxRelated = myMaxPower_p->IsRelative(myMaxTestPower_p, true);
	}
	if ( myCheckedPower ) {
		return myMinRelated || mySimRelated || myMaxRelated;
	} else {
		return true; //
	}
/ *
	while ( myMinPower_p->type[MIN_CALCULATED_BIT] ) {
		myNetId = myMinPower_p->defaultMinNet;
		while ( myNetId != theMinNet_v[myNetId].nextNetId ) {
			myNetId = theMinNet_v[myNetId].nextNetId;
		}
		myMinPower_p = theNetVoltagePtr_v[myNetId];
	}
	if ( defaultMinNet != UNKNOWN_NET ) {
		assert(defaultMinNet != UNKNOWN_NET);
		myIsRelatedCalculation &= ((baseMinId == theTestPower_p->netId) || (baseMinId == theTestPower_p->baseMinId) || (defaultMinNet == theTestPower_p->defaultMinNet));
	}
	if ( theTestPower_p->baseMinId != UNKNOWN_NET ) {
		assert(theTestPower_p->defaultMinNet != UNKNOWN_NET);
		myIsRelatedCalculation &= ((theTestPower_p->baseMinId == netId) || (theTestPower_p->baseMinId == baseMinId) || (theTestPower_p->defaultMinNet == defaultMinNet));
	}
	if ( baseSimId != UNKNOWN_NET ) {
		assert(defaultSimNet != UNKNOWN_NET);
		myIsRelatedCalculation &= ((baseSimId == theTestPower_p->netId) || (baseSimId == theTestPower_p->baseSimId) || (defaultSimNet == theTestPower_p->defaultSimNet));
	}
	if ( theTestPower_p->baseSimId != UNKNOWN_NET ) {
		assert(theTestPower_p->defaultSimNet != UNKNOWN_NET);
		myIsRelatedCalculation &= ((theTestPower_p->baseSimId == netId) || (theTestPower_p->baseSimId == baseSimId) || (theTestPower_p->defaultSimNet == defaultSimNet));
	}
	if ( baseMaxId != UNKNOWN_NET ) {
		assert(defaultMaxNet != UNKNOWN_NET);
		myIsRelatedCalculation &= ((baseMaxId == theTestPower_p->netId) || (baseMaxId == theTestPower_p->baseSimId) || (defaultMaxNet == theTestPower_p->defaultMaxNet));
	}
	if ( theTestPower_p->baseMaxId != UNKNOWN_NET ) {
		assert(theTestPower_p->defaultMaxNet != UNKNOWN_NET);
		myIsRelatedCalculation &= ((theTestPower_p->baseMaxId == netId) || (theTestPower_p->baseMaxId == baseMaxId) || (theTestPower_p->defaultMaxNet == defaultMaxNet));
	}
	if ( ! myIsRelatedCalculation ) return false;
	if ( family != "" ) {
		if ( relativeFriendly ) {
			if ( relativeSet.count(theTestPower_p->powerSignal) == 0 ) return false;
		} else {
			if ( relativeSet.count(theTestPower_p->powerSignal) > 0 ) return false;
		}
	}
	if ( theTestPower_p->family != "" ) {
		if ( theTestPower_p->relativeFriendly ) {
			if ( theTestPower_p->relativeSet.count(powerSignal) == 0 ) return false;
		} else {
			if ( theTestPower_p->relativeSet.count(powerSignal) > 0 ) return false;
		}
	}
	return ( true );
* /

}
*/

void CPower::Print(ostream & theLogFile, string theIndentation, string theRealPowerName) {
	if ( IsEmpty(theRealPowerName) || theRealPowerName == string(powerSignal()) ) {
		string myAlias = ( IsEmpty(powerAlias()) || powerSignal() == powerAlias() ) ? "" : ALIAS_DELIMITER + string(powerAlias());
		theLogFile << theIndentation << powerSignal() << myAlias;
	} else { // expanded net names
		theLogFile << theIndentation << "->" << theRealPowerName;
	}
	string myDefinition = StandardDefinition();
/*
	if ( minVoltage == simVoltage && simVoltage == maxVoltage && type[FIXED_BIT] ) {
		myDefinition << " " << PrintParameter(minVoltage, VOLTAGE_SCALE);
	} else {
		if ( minVoltage != UNKNOWN_VOLTAGE ) myDefinition << " min@" << PrintParameter(minVoltage, VOLTAGE_SCALE);
		if ( simVoltage != UNKNOWN_VOLTAGE ) myDefinition << " sim@" << PrintParameter(simVoltage, VOLTAGE_SCALE);
		if ( maxVoltage != UNKNOWN_VOLTAGE ) myDefinition << " max@" << PrintParameter(maxVoltage, VOLTAGE_SCALE);
	}
	if ( expectedMin != "" ) myDefinition << " expectMin@" << expectedMin;
	if ( expectedSim != "" ) myDefinition << " expectSim@" << expectedSim;
	if ( expectedMax != "" ) myDefinition << " expectMax@" << expectedMax;
	if ( type[HIZ_BIT] ) myDefinition << " open";
	if ( type[INPUT_BIT] ) myDefinition << " input";
	if ( type[RESISTOR_BIT] ) myDefinition << " resistor";
//	if ( family != "" ) myDefinition << " Family:" << family << "(" << relativeSet.size() << " relatives)";
	if ( family != "" ) myDefinition << (relativeFriendly ? " permit@" : " prohibit@") << family;
*/
//	if ( ! definition.empty() ) {
		theLogFile << definition; // for defined voltages
//	}
	if ( ! IsEmpty(myDefinition) && myDefinition != string(definition) ) {
		theLogFile << (IsCalculatedVoltage_(this) ? " =>" : " ->") << myDefinition;
	}
	if ( extraData && ! extraData->relativeSet.empty() ) {
		theLogFile << " family(";
		for ( auto relative_it = extraData->relativeSet.begin(); relative_it != extraData->relativeSet.end(); relative_it++ ) {
			theLogFile << *relative_it << ";";
		}
		theLogFile << ")";
	}
	theLogFile << endl;
}

string CPower::StandardDefinition() {
	stringstream myDefinition;
	if ( simVoltage != UNKNOWN_VOLTAGE && minVoltage == simVoltage && simVoltage == maxVoltage && type[POWER_BIT] ) {
		myDefinition << " " << PrintParameter(minVoltage, VOLTAGE_SCALE);
	} else {
		if ( minVoltage != UNKNOWN_VOLTAGE ) myDefinition << " min" << (type[MIN_CALCULATED_BIT] ? "=" : "@") << PrintParameter(minVoltage, VOLTAGE_SCALE);
		if ( simVoltage != UNKNOWN_VOLTAGE ) myDefinition << " sim" << (type[SIM_CALCULATED_BIT] ? "=" : "@") << PrintParameter(simVoltage, VOLTAGE_SCALE);
		if ( maxVoltage != UNKNOWN_VOLTAGE ) myDefinition << " max" << (type[MAX_CALCULATED_BIT] ? "=" : "@") << PrintParameter(maxVoltage, VOLTAGE_SCALE);
	}
	if ( ! IsEmpty(expectedMin()) ) myDefinition << " expectMin@" << expectedMin();
	if ( ! IsEmpty(expectedSim()) ) myDefinition << " expectSim@" << expectedSim();
	if ( ! IsEmpty(expectedMax()) ) myDefinition << " expectMax@" << expectedMax();
	if ( type[HIZ_BIT] ) myDefinition << " open";
	if ( type[INPUT_BIT] ) myDefinition << " input";
	if ( type[POWER_BIT] ) myDefinition << " power";
	if ( type[RESISTOR_BIT] ) myDefinition << " resistor";
//	if ( family != "" ) myDefinition << " Family:" << family << "(" << relativeSet.size() << " relatives)";
	if ( ! IsEmpty(family()) ) myDefinition << (relativeFriendly ? " permit@" : " prohibit@") << family();
	return(myDefinition.str());
}

voltage_t CPowerPtrVector::MaxVoltage(netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) return UNKNOWN_VOLTAGE;
	if ( (*this)[theNetId] == NULL ) return UNKNOWN_VOLTAGE;
	return (*this)[theNetId]->maxVoltage;
}

voltage_t CPowerPtrVector::MinVoltage(netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) return UNKNOWN_VOLTAGE;
	if ( (*this)[theNetId] == NULL ) return UNKNOWN_VOLTAGE;
	return (*this)[theNetId]->minVoltage;
}

voltage_t CPowerPtrVector::SimVoltage(netId_t theNetId) {
	if ( theNetId == UNKNOWN_NET ) return UNKNOWN_VOLTAGE;
	if ( (*this)[theNetId] == NULL ) return UNKNOWN_VOLTAGE;
	return (*this)[theNetId]->simVoltage;
}

void CPowerPtrList::Clear() {
//	calculatedPowerPtrMap.clear();

	while ( ! empty() ) {
		delete front();
		pop_front();
	}
}

void CPowerPtrList::Clear(CPowerPtrVector & theLeakVoltage_v, CPowerPtrVector & theNetVoltage_v, netId_t theNetCount) {
	while ( ! empty() ) {
		netId_t myNetId = front()->netId;
		if ( myNetId != UNKNOWN_NET && theNetCount > 0 ) {
			if ( theLeakVoltage_v.size() > myNetId && theLeakVoltage_v[myNetId] && theLeakVoltage_v[myNetId] == front() ) {
				theLeakVoltage_v[myNetId] = NULL;
			}
			if ( theNetVoltage_v.size() > myNetId && theNetVoltage_v[myNetId] && theNetVoltage_v[myNetId] == front() ) {
				theNetVoltage_v[myNetId] = NULL;
			}
		}
		delete front();
		pop_front();
	}
	CPower::powerDefinitionText.Clear();
}

void CPowerPtrMap::Clear() {
	while ( ! empty() ) {
		delete begin()->second;
		erase(begin());
	}
}

void CPowerPtrList::SetPowerLimits(voltage_t& theMaxPower, voltage_t& theMinPower) {
	theMaxPower = MIN_VOLTAGE;
	theMinPower = MAX_VOLTAGE;
	for ( CPowerPtrList::iterator power_ppit = begin(); power_ppit != end(); power_ppit++ ) {
		theMinPower = min(theMinPower, (*power_ppit)->minVoltage);
		theMaxPower = max(theMaxPower, (*power_ppit)->maxVoltage);
	}
}

/*
CPower * CPowerPtrList::FindCalculatedPower(CEventQueue& theEventQueue, CPower * theCurrentPower_p, voltage_t theShortVoltage, netId_t theNetId, netId_t theDefaultNetId, CCvcDb * theCvcDb_p) {
	voltage_t myMinVoltage = UNKNOWN_VOLTAGE, mySimVoltage = UNKNOWN_VOLTAGE, myMaxVoltage = UNKNOWN_VOLTAGE;
	netId_t myDefaultMinNet = UNKNOWN_NET, myDefaultSimNet = UNKNOWN_NET, myDefaultMaxNet = UNKNOWN_NET;
	CPower * myPower_p;
	CStatus myStatus = 0;
	if ( theCurrentPower_p ) {
		myMinVoltage = theCurrentPower_p->minVoltage;
		myMaxVoltage = theCurrentPower_p->maxVoltage;
		mySimVoltage = theCurrentPower_p->simVoltage;
		myStatus = theCurrentPower_p->type;
	}
	if ( theEventQueue.queueType == MIN_QUEUE ) {
		if ( myMinVoltage != UNKNOWN_VOLTAGE ) throw EDatabaseError("FindCalculatedPower: Min " + to_string<voltage_t>(myMinVoltage));
		myMinVoltage = theShortVoltage;
		myDefaultMinNet = theDefaultNetId;
		myStatus[MIN_CALCULATED_BIT] = true;
	}
	if ( theEventQueue.queueType == MAX_QUEUE ) {
		if ( myMaxVoltage != UNKNOWN_VOLTAGE ) throw EDatabaseError("FindCalculatedPower: Max " + to_string<voltage_t>(myMaxVoltage));
		myMaxVoltage = theShortVoltage;
		myDefaultMaxNet = theDefaultNetId;
		myStatus[MAX_CALCULATED_BIT] = true;
	}
	if ( theEventQueue.queueType == SIM_QUEUE ) {
		if ( mySimVoltage != UNKNOWN_VOLTAGE ) throw EDatabaseError("FindCalculatedPower: Sim " + to_string<voltage_t>(mySimVoltage));
		mySimVoltage = theShortVoltage;
		myDefaultSimNet = theDefaultNetId;
		myStatus[SIM_CALCULATED_BIT] = true;
	}
	string myKeyString = int_to_hex<voltage_t>(myMinVoltage) + int_to_hex<voltage_t>(mySimVoltage) + int_to_hex<voltage_t>(myMaxVoltage);
	try {
		if (myMinVoltage != UNKNOWN_VOLTAGE && mySimVoltage != UNKNOWN_VOLTAGE && myMinVoltage > mySimVoltage) {
			cout << "WARNING: MIN > SIM " << myMinVoltage << " > " << mySimVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
		}
		if (myMaxVoltage != UNKNOWN_VOLTAGE && mySimVoltage != UNKNOWN_VOLTAGE && myMaxVoltage < mySimVoltage) {
			cout << "WARNING: MAX < SIM " << myMaxVoltage << " < " << mySimVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
		}
		myPower_p = calculatedPowerPtrMap.at(myKeyString);
	}
	catch (const out_of_range& oor_exception) {
		if ( debug_cvc ) cout << "DEBUG: creating power: " << myKeyString << " " << myMinVoltage << "/" << mySimVoltage << "/" << myMaxVoltage << endl;
		myPower_p = new CPower(myMinVoltage, mySimVoltage, myMaxVoltage, myDefaultMinNet, myDefaultSimNet, myDefaultMaxNet);
		myPower_p->type = myStatus;
		calculatedPowerPtrMap[myKeyString] = myPower_p;
	}
	return(myPower_p);
}

CPower * CPowerPtrList::FindCalculatedPower(voltage_t theMinVoltage, voltage_t theSimVoltage, voltage_t theMaxVoltage, netId_t theNetId, CCvcDb * theCvcDb_p) {
	CPower * myPower_p;
	string myKeyString = int_to_hex<voltage_t>(theMinVoltage) + int_to_hex<voltage_t>(theSimVoltage) + int_to_hex<voltage_t>(theMaxVoltage);
	try {
		if (theMinVoltage != UNKNOWN_VOLTAGE && theSimVoltage != UNKNOWN_VOLTAGE && theMinVoltage > theSimVoltage) {
			cout << "WARNING: MIN > SIM " << theMinVoltage << " > " << theSimVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
		}
		if (theMaxVoltage != UNKNOWN_VOLTAGE && theSimVoltage != UNKNOWN_VOLTAGE && theMaxVoltage < theSimVoltage) {
			cout << "WARNING: MAX < SIM " << theMaxVoltage << " < " << theSimVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
		}
		myPower_p = calculatedPowerPtrMap.at(myKeyString);
	}
	catch (const out_of_range& oor_exception) {
		if ( debug_cvc ) cout << "DEBUG: creating power: " << myKeyString << endl;
		myPower_p = new CPower(theMinVoltage, theSimVoltage, theMaxVoltage);
		myPower_p->type[MIN_CALCULATED_BIT] = true;
		myPower_p->type[SIM_CALCULATED_BIT] = true;
		myPower_p->type[MAX_CALCULATED_BIT] = true;
		calculatedPowerPtrMap[myKeyString] = myPower_p;
	}
	return(myPower_p);
}
*/

/*
// Changed 2014/7/23 to always create new Power node
CPower * CPowerPtrList::FindCalculatedPower(CEventQueue& theEventQueue, CPower * theCurrentPower_p, voltage_t theShortVoltage, netId_t theNetId, netId_t theDefaultNetId, CCvcDb * theCvcDb_p) {
	voltage_t myMinVoltage = UNKNOWN_VOLTAGE, mySimVoltage = UNKNOWN_VOLTAGE, myMaxVoltage = UNKNOWN_VOLTAGE;
	netId_t myDefaultMinNet = UNKNOWN_NET, myDefaultSimNet = UNKNOWN_NET, myDefaultMaxNet = UNKNOWN_NET;
	CPower * myPower_p;
	CStatus myStatus = 0, myActiveStatus = 0;
	if ( theCurrentPower_p ) {
		myMinVoltage = theCurrentPower_p->minVoltage;
		myMaxVoltage = theCurrentPower_p->maxVoltage;
		mySimVoltage = theCurrentPower_p->simVoltage;
		myDefaultMinNet = theCurrentPower_p->defaultMinNet;
		myDefaultMaxNet = theCurrentPower_p->defaultMaxNet;
		myDefaultSimNet = theCurrentPower_p->defaultSimNet;
		myStatus = theCurrentPower_p->type;
		myActiveStatus = theCurrentPower_p->active;
	} else {
		myStatus[CALCULATED_BIT] = true;
	}
	if ( theCvcDb_p->netVoltagePtr_v[theDefaultNetId] && theCvcDb_p->netVoltagePtr_v[theDefaultNetId]->type[HIZ_BIT] ) {
		myStatus[HIZ_BIT] = true;
	}
	if ( theEventQueue.queueType == MIN_QUEUE ) {
		if ( myMinVoltage != UNKNOWN_VOLTAGE ) throw EDatabaseError("FindCalculatedPower: Min " + to_string<voltage_t>(myMinVoltage));
		myMinVoltage = theShortVoltage;
		myDefaultMinNet = theDefaultNetId;
		myStatus[MIN_CALCULATED_BIT] = true;
		myActiveStatus[MIN_ACTIVE] = true;
	}
	if ( theEventQueue.queueType == MAX_QUEUE ) {
		if ( myMaxVoltage != UNKNOWN_VOLTAGE ) throw EDatabaseError("FindCalculatedPower: Max " + to_string<voltage_t>(myMaxVoltage));
		myMaxVoltage = theShortVoltage;
		myDefaultMaxNet = theDefaultNetId;
		myStatus[MAX_CALCULATED_BIT] = true;
		myActiveStatus[MAX_ACTIVE] = true;
	}
	if ( theEventQueue.queueType == SIM_QUEUE ) {
		if ( mySimVoltage != UNKNOWN_VOLTAGE ) throw EDatabaseError("FindCalculatedPower: Sim " + to_string<voltage_t>(mySimVoltage));
		mySimVoltage = theShortVoltage;
		myDefaultSimNet = theDefaultNetId;
		myStatus[SIM_CALCULATED_BIT] = true;
	}
//	string myKeyString = int_to_hex<voltage_t>(myMinVoltage) + int_to_hex<voltage_t>(mySimVoltage) + int_to_hex<voltage_t>(myMaxVoltage);
//	try {
		if (myMinVoltage != UNKNOWN_VOLTAGE && mySimVoltage != UNKNOWN_VOLTAGE && myMinVoltage > mySimVoltage) {
			cout << "WARNING: MIN > SIM " << myMinVoltage << " > " << mySimVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
		}
		if (myMaxVoltage != UNKNOWN_VOLTAGE && mySimVoltage != UNKNOWN_VOLTAGE && myMaxVoltage < mySimVoltage) {
			cout << "WARNING: MAX < SIM " << myMaxVoltage << " < " << mySimVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
		}
//		myPower_p = calculatedPowerPtrMap.at(myKeyString);
//	}
//	catch (const out_of_range& oor_exception) {
//		if ( debug_cvc ) cout << "DEBUG: creating power: " << myKeyString << " " << myMinVoltage << "/" << mySimVoltage << "/" << myMaxVoltage << endl;
		myPower_p = new CPower(myMinVoltage, mySimVoltage, myMaxVoltage, myDefaultMinNet, myDefaultSimNet, myDefaultMaxNet);
		myPower_p->type = myStatus;
		myPower_p->active = myActiveStatus;
//		calculatedPowerPtrMap[myKeyString] = myPower_p;
//	}
	return(myPower_p);
}
*/

void CPowerPtrVector::CalculatePower(CEventQueue& theEventQueue, voltage_t theShortVoltage, netId_t theNetId, netId_t theDefaultNetId, CCvcDb * theCvcDb_p, string theCalculation) {
	if ( (*this)[theNetId] == NULL ) {
		(*this)[theNetId] = new CPower(theNetId);
//		(*this)[theNetId]->type[CALCULATED_BIT] = true;
	}
	CPower * myPower_p = (*this)[theNetId];
//	voltage_t myMinVoltage = UNKNOWN_VOLTAGE, mySimVoltage = UNKNOWN_VOLTAGE, myMaxVoltage = UNKNOWN_VOLTAGE;
//	netId_t myDefaultMinNet = UNKNOWN_NET, myDefaultSimNet = UNKNOWN_NET, myDefaultMaxNet = UNKNOWN_NET;
//	CStatus myStatus = 0, myActiveStatus = 0;
/*
	if ( theCurrentPower_p ) {
		myMinVoltage = theCurrentPower_p->minVoltage;
		myMaxVoltage = theCurrentPower_p->maxVoltage;
		mySimVoltage = theCurrentPower_p->simVoltage;
		myDefaultMinNet = theCurrentPower_p->defaultMinNet;
		myDefaultMaxNet = theCurrentPower_p->defaultMaxNet;
		myDefaultSimNet = theCurrentPower_p->defaultSimNet;
		myStatus = theCurrentPower_p->type;
		myActiveStatus = theCurrentPower_p->active;
	} else {
		myStatus[CALCULATED_BIT] = true;
	}
*/
	if ( (*this)[theDefaultNetId] ) {
		if ( (*this)[theDefaultNetId]->type[HIZ_BIT] ) {
			myPower_p->type[HIZ_BIT] = true;
		}
		if ( (*this)[theDefaultNetId]->extraData ) {
			if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
			myPower_p->extraData->family = (*this)[theDefaultNetId]->family();
			myPower_p->extraData->implicitFamily = (*this)[theDefaultNetId]->implicitFamily();
			myPower_p->extraData->relativeSet = (*this)[theDefaultNetId]->extraData->relativeSet;
		}
		myPower_p->relativeFriendly = (*this)[theDefaultNetId]->relativeFriendly;
//		myPower_p->defaultMinNet = theDefaultNetId;
//		myPower_p->type[MIN_CALCULATED_BIT] = true;
//		myPower_p->defaultMaxNet = theDefaultNetId;
//		myPower_p->type[MAX_CALCULATED_BIT] = true;
//		myPower_p->defaultSimNet = theDefaultNetId;
//		myPower_p->type[SIM_CALCULATED_BIT] = true;
	}
	if ( theEventQueue.queueType == MIN_QUEUE ) {
		if ( myPower_p->minVoltage != UNKNOWN_VOLTAGE ) throw EDatabaseError("FindCalculatedPower: Min " + to_string<voltage_t>(myPower_p->minVoltage));
		myPower_p->minVoltage = theShortVoltage;
		myPower_p->defaultMinNet = theDefaultNetId;
		myPower_p->type[MIN_CALCULATED_BIT] = true;
		myPower_p->active[MIN_ACTIVE] = true;
		if ( (*this)[theDefaultNetId] && (*this)[theDefaultNetId]->minVoltage != UNKNOWN_VOLTAGE ) {
			if ( theShortVoltage < (*this)[theDefaultNetId]->minVoltage ) {
				myPower_p->minCalculationType = DOWN_CALCULATION;
			} else if ( theShortVoltage > (*this)[theDefaultNetId]->minVoltage ) {
				myPower_p->minCalculationType = UP_CALCULATION;
			}
		}
//		if ( myPower_p->powerAlias == "" ) {
//			CPower * myDefaultPower_p = myPower_p->GetBasePower(theCvcDb_p->netVoltagePtr_v, theCvcDb_p->minNet_v);
//			if ( myDefaultPower_p ) {
//				if (gDebug_cvc) cout << "Setting alias for " << theCvcDb_p->NetName(theNetId) << " = " << myDefaultPower_p->powerAlias << endl;
//				myPower_p->powerAlias = myDefaultPower_p->powerAlias;
//			}
//		}
	}
	if ( theEventQueue.queueType == MAX_QUEUE ) {
		if ( myPower_p->maxVoltage != UNKNOWN_VOLTAGE ) throw EDatabaseError("FindCalculatedPower: Max " + to_string<voltage_t>(myPower_p->maxVoltage));
		myPower_p->maxVoltage = theShortVoltage;
		myPower_p->defaultMaxNet = theDefaultNetId;
		myPower_p->type[MAX_CALCULATED_BIT] = true;
		myPower_p->active[MAX_ACTIVE] = true;
		if ( (*this)[theDefaultNetId] && (*this)[theDefaultNetId]->maxVoltage != UNKNOWN_VOLTAGE ) {
			if ( theShortVoltage < (*this)[theDefaultNetId]->maxVoltage ) {
				myPower_p->maxCalculationType = DOWN_CALCULATION;
			} else if ( theShortVoltage > (*this)[theDefaultNetId]->maxVoltage ) {
				myPower_p->maxCalculationType = UP_CALCULATION;
			}
		}
//		if ( myPower_p->powerAlias == "" ) {
//			CPower * myDefaultPower_p = myPower_p->GetBasePower(theCvcDb_p->netVoltagePtr_v, theCvcDb_p->maxNet_v);
//			if ( myDefaultPower_p ) {
//				if (gDebug_cvc) cout << "Setting alias for " << theCvcDb_p->NetName(theNetId) << " = " << myDefaultPower_p->powerAlias << endl;
//				myPower_p->powerAlias = myDefaultPower_p->powerAlias;
//			}
//		}
	}
	if ( theEventQueue.queueType == SIM_QUEUE ) {
		if ( myPower_p->simVoltage != UNKNOWN_VOLTAGE ) throw EDatabaseError("FindCalculatedPower: Sim " + to_string<voltage_t>(myPower_p->simVoltage));
		myPower_p->simVoltage = theShortVoltage;
		myPower_p->defaultSimNet = theDefaultNetId;
		myPower_p->type[SIM_CALCULATED_BIT] = true;
		if ( myPower_p->powerAlias()[0] == '\0' ) {
			CPower * myDefaultPower_p;
			if ( myPower_p->minVoltage == myPower_p->simVoltage ) {
				myDefaultPower_p = myPower_p->GetBasePower(theCvcDb_p->netVoltagePtr_v, theCvcDb_p->minNet_v);
			} else if ( myPower_p->maxVoltage == myPower_p->simVoltage ) {
				myDefaultPower_p = myPower_p->GetBasePower(theCvcDb_p->netVoltagePtr_v, theCvcDb_p->maxNet_v);
			} else {
				myDefaultPower_p = myPower_p->GetBasePower(theCvcDb_p->netVoltagePtr_v, theCvcDb_p->simNet_v);
			}
			if ( myDefaultPower_p && ! IsEmpty(myDefaultPower_p->powerAlias()) ) {
				if (gDebug_cvc) cout << "Setting alias for " << theCvcDb_p->NetName(theNetId) << " = " << myDefaultPower_p->powerAlias() << endl;
				if ( ! myPower_p->extraData ) myPower_p->extraData = new CExtraPowerData;
				myPower_p->extraData->powerAlias = myDefaultPower_p->powerAlias();
			}
		}
		if ( (*this)[theDefaultNetId] && (*this)[theDefaultNetId]->simVoltage != UNKNOWN_VOLTAGE ) {
			if ( theShortVoltage < (*this)[theDefaultNetId]->simVoltage ) {
				myPower_p->simCalculationType = DOWN_CALCULATION;
			} else if ( theShortVoltage > (*this)[theDefaultNetId]->simVoltage ) {
				myPower_p->simCalculationType = UP_CALCULATION;
			}
		}

	}
	if (myPower_p->minVoltage != UNKNOWN_VOLTAGE && myPower_p->simVoltage != UNKNOWN_VOLTAGE && myPower_p->minVoltage > myPower_p->simVoltage) {
		cout << "WARNING: MIN > SIM " << myPower_p->minVoltage << " > " << myPower_p->simVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
	}
	if (myPower_p->maxVoltage != UNKNOWN_VOLTAGE && myPower_p->simVoltage != UNKNOWN_VOLTAGE && myPower_p->maxVoltage < myPower_p->simVoltage) {
		cout << "WARNING: MAX < SIM " << myPower_p->maxVoltage << " < " << myPower_p->simVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
	}
	if ( myPower_p->definition[0] == '\0' ) {
		myPower_p->definition = CPower::powerDefinitionText.SetTextAddress((text_t)theCalculation.c_str());
	}
	if (gDebug_cvc) cout << "INFO: Calculated voltage at net " << myPower_p->netId << " default Min/Sim/Max: " << myPower_p->defaultMinNet << "/"
			<< myPower_p->defaultSimNet << "/" << myPower_p->defaultMaxNet << endl;
//	myPower_p = new CPower(myMinVoltage, mySimVoltage, myMaxVoltage, myDefaultMinNet, myDefaultSimNet, myDefaultMaxNet);
//	myPower_p->type = myStatus;
//	myPower_p->active = myActiveStatus;
//	return(myPower_p);
}

/*
CPower * CPowerPtrList::FindCalculatedPower(voltage_t theMinVoltage, voltage_t theSimVoltage, voltage_t theMaxVoltage, netId_t theNetId, CCvcDb * theCvcDb_p) {
	CPower * myPower_p;
//	string myKeyString = int_to_hex<voltage_t>(theMinVoltage) + int_to_hex<voltage_t>(theSimVoltage) + int_to_hex<voltage_t>(theMaxVoltage);
//	try {
		if (theMinVoltage != UNKNOWN_VOLTAGE && theSimVoltage != UNKNOWN_VOLTAGE && theMinVoltage > theSimVoltage) {
			cout << "WARNING: MIN > SIM " << theMinVoltage << " > " << theSimVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
		}
		if (theMaxVoltage != UNKNOWN_VOLTAGE && theSimVoltage != UNKNOWN_VOLTAGE && theMaxVoltage < theSimVoltage) {
			cout << "WARNING: MAX < SIM " << theMaxVoltage << " < " << theSimVoltage << " for " << theCvcDb_p->NetName(theNetId, PRINT_CIRCUIT_ON) << endl;
		}
//		myPower_p = calculatedPowerPtrMap.at(myKeyString);
//	}
//	catch (const out_of_range& oor_exception) {
//		if ( debug_cvc ) cout << "DEBUG: creating power: " << myKeyString << endl;
		myPower_p = new CPower(theMinVoltage, theSimVoltage, theMaxVoltage);
		myPower_p->type[MIN_CALCULATED_BIT] = true;
		myPower_p->type[SIM_CALCULATED_BIT] = true;
		myPower_p->type[MAX_CALCULATED_BIT] = true;
//		calculatedPowerPtrMap[myKeyString] = myPower_p;
//	}
	return(myPower_p);
}
*/

void CPowerPtrList::SetFamilies(CPowerFamilyMap & thePowerFamilyMap) {
	string myFamilyName;
	string myRelativeName;
	string myFamilyList;
	string myImplicitFamilyList;
	size_t	myStringBegin;
	size_t	myStringEnd;
	for ( CPowerPtrList::iterator powerPtr_ppit = begin(); powerPtr_ppit != end(); powerPtr_ppit++ ) {
		myFamilyList = (*powerPtr_ppit)->family();
		myImplicitFamilyList = (*powerPtr_ppit)->implicitFamily();
		if ( ! IsEmpty(myFamilyList) ) {
			myStringBegin = myFamilyList.find_first_not_of(",", 0);
			myStringEnd = myFamilyList.find_first_of(",", myStringBegin);
			if ( ! (*powerPtr_ppit)->extraData ) (*powerPtr_ppit)->extraData = new CExtraPowerData;
			while (myStringBegin < myFamilyList.length()) {
				myRelativeName = myFamilyList.substr(myStringBegin, myStringEnd - myStringBegin);
				if ( thePowerFamilyMap.count(myRelativeName) > 0 ) {
					(*powerPtr_ppit)->extraData->relativeSet.insert(thePowerFamilyMap[myRelativeName].begin(), thePowerFamilyMap[myRelativeName].end());
				} else {
					(*powerPtr_ppit)->extraData->relativeSet.insert(CPower::powerDefinitionText.SetTextAddress((text_t)myRelativeName.c_str()));
				}
				myStringBegin = myFamilyList.find_first_not_of(",", myStringEnd);
				myStringEnd = myFamilyList.find_first_of(",", myStringBegin);
			}
		// explicit family list overrides implicit list
		} else if ( ! IsEmpty(myImplicitFamilyList) ) {
			myStringBegin = myImplicitFamilyList.find_first_not_of(",", 0);
			myStringEnd = myImplicitFamilyList.find_first_of(",", myStringBegin);
			if ( ! (*powerPtr_ppit)->extraData ) (*powerPtr_ppit)->extraData = new CExtraPowerData;
			while (myStringBegin < myImplicitFamilyList.length()) {
				myRelativeName = myImplicitFamilyList.substr(myStringBegin, myStringEnd - myStringBegin);
				if ( thePowerFamilyMap.count(myRelativeName) > 0 ) {
					(*powerPtr_ppit)->extraData->relativeSet.insert(thePowerFamilyMap[myRelativeName].begin(), thePowerFamilyMap[myRelativeName].end());
				} else {
					(*powerPtr_ppit)->extraData->relativeSet.insert(CPower::powerDefinitionText.SetTextAddress((text_t)myRelativeName.c_str()));
				}
				myStringBegin = myImplicitFamilyList.find_first_not_of(",", myStringEnd);
				myStringEnd = myImplicitFamilyList.find_first_of(",", myStringBegin);
			}
		}
	}
}

string CPower::PowerDefinition() {
	string myPowerDefinition = "";
	if ( minVoltage != UNKNOWN_VOLTAGE ) myPowerDefinition += " min" + ( ((type[MIN_CALCULATED_BIT]) ? "=" : "@") + to_string<voltage_t>(minVoltage));
	if ( simVoltage != UNKNOWN_VOLTAGE ) myPowerDefinition += " sim" + ( ((type[SIM_CALCULATED_BIT]) ? "=" : "@") + to_string<voltage_t>(simVoltage));
	if ( maxVoltage != UNKNOWN_VOLTAGE ) myPowerDefinition += " max" + ( ((type[MAX_CALCULATED_BIT]) ? "=" : "@") + to_string<voltage_t>(maxVoltage));
	if ( ! IsEmpty(expectedMin()) ) myPowerDefinition += " min?" + expectedMin();
	if ( ! IsEmpty(expectedSim()) ) myPowerDefinition += " sim?" + expectedSim();
	if ( ! IsEmpty(expectedMax()) ) myPowerDefinition += " max?" + expectedMax();
	if ( type[HIZ_BIT] ) myPowerDefinition += " open";
	if ( type[INPUT_BIT] ) myPowerDefinition += " input";
	if ( type[RESISTOR_BIT] ) myPowerDefinition += " resistor";
//	if ( definition != "" ) myPowerDefinition += " '" + definition + "'";
	if ( ! IsEmpty(family()) ) myPowerDefinition += " '" + family() + ((relativeFriendly) ? " allowed'" : " prohibited'");
	return myPowerDefinition;
}

string CPowerPtrMap::CalculateExpectedValue(string theEquation, netStatus_t theType, CModelListMap & theModelListMap) {
	if ( theEquation == "open" ) return theEquation;
	if ( this->count(theEquation) > 0 ) return theEquation;
	if ( IsValidVoltage_(theEquation) ) return theEquation;
	voltage_t myVoltage = CalculateVoltage(theEquation, theType, theModelListMap);
	return(to_string<float>(float(myVoltage) / VOLTAGE_SCALE));
}

#define UNKNOWN_TOKEN (float(MAX_VOLTAGE) * 2)
voltage_t CPowerPtrMap::CalculateVoltage(string theEquation, netStatus_t theType, CModelListMap & theModelListMap, bool thePermitUndefinedFlag, bool theResetImplicitFlag) {
	if ( theResetImplicitFlag) implicitFamily = "";
	list<string> * myTokenList_p;
	// exceptions cause memory leaks
//	cout << "calculating " << theEquation << endl;
	if ( IsValidVoltage_(theEquation) ) {
		myTokenList_p = new(list<string>);
		myTokenList_p->push_back(theEquation);
	} else {
		myTokenList_p = postfix(theEquation);
	}
	list<float> myVoltageStack;
	float	myVoltage;
	string	myOperators = "+-/*<>";
	for ( auto token_pit = myTokenList_p->begin(); token_pit != myTokenList_p->end(); token_pit++ ) {
		if ( myOperators.find(*token_pit) < myOperators.length() ) {
			if ( myVoltageStack.size() < 2 ) throw EPowerError("invalid equation: " + theEquation);
			myVoltage = myVoltageStack.back();
			myVoltageStack.pop_back();
			if ( myVoltage == UNKNOWN_TOKEN ) {
				if ( ( *token_pit == "<" ) || ( *token_pit == ">" ) ) {  // min/max keep value on stack
					;
				} else {  // arithmetic with invalid values gives invalid value
					myVoltageStack.back() = UNKNOWN_TOKEN;
				}
			} else if ( myVoltageStack.back() == UNKNOWN_TOKEN ) {
				if ( ( *token_pit == "<" ) || ( *token_pit == ">" ) ) {  // min/max replace with last voltage
					myVoltageStack.back() = myVoltage;
				}
			} else if ( *token_pit == "+" ) {
				myVoltageStack.back() += myVoltage;
			} else if ( *token_pit == "-" ) {
				myVoltageStack.back() -= myVoltage;
			} else if ( *token_pit == "*" ) {
				myVoltageStack.back() *= myVoltage;
			} else if ( *token_pit == "/" ) {
				myVoltageStack.back() /= myVoltage;
			} else if ( *token_pit == "<" ) {
				myVoltageStack.back() = min(myVoltageStack.back(), myVoltage);
			} else if ( *token_pit == ">" ) {
				myVoltageStack.back() = max(myVoltageStack.back(), myVoltage);
			} else throw EPowerError("invalid operator: " + *token_pit);
		} else if ( isalpha((*token_pit)[0]) ) { // power name
			if ( this->count(*token_pit) > 0 ) { // power definition exists
				if ( theType == MIN_POWER ) {
					if ( (*this)[*token_pit]->minVoltage == UNKNOWN_VOLTAGE ) {
						myVoltageStack.push_back(UNKNOWN_TOKEN);
					} else {
						myVoltageStack.push_back(float((*this)[*token_pit]->minVoltage) / VOLTAGE_SCALE);
					}
				}
				if ( theType == SIM_POWER ) {
					if ( (*this)[*token_pit]->simVoltage == UNKNOWN_VOLTAGE ) {
						myVoltageStack.push_back(UNKNOWN_TOKEN);
					} else {
						myVoltageStack.push_back(float((*this)[*token_pit]->simVoltage) / VOLTAGE_SCALE);
					}
				}
				if ( theType == MAX_POWER ) {
					if ( (*this)[*token_pit]->maxVoltage == UNKNOWN_VOLTAGE ) {
						myVoltageStack.push_back(UNKNOWN_TOKEN);
					} else {
						myVoltageStack.push_back(float((*this)[*token_pit]->maxVoltage) / VOLTAGE_SCALE);
					}
				}
				implicitFamily += "," + *token_pit;
			} else if ( (*token_pit).substr(0,4) == "Vth[" && (*token_pit).back() == ']' ){
				string myModelName = "M " + (*token_pit).substr(4, (*token_pit).length() - 5);
				if ( theModelListMap.count(myModelName) > 0 ) {
					if ( theModelListMap[myModelName].Vth == UNKNOWN_VOLTAGE ) {
						theModelListMap[myModelName].Vth = CalculateVoltage(theModelListMap[myModelName].vthDefinition, SIM_POWER, theModelListMap, false, false);
						// recursive call fails if contains undefined macros. does not reset implicit power list
					}
					myVoltageStack.push_back(float(theModelListMap[myModelName].Vth) / VOLTAGE_SCALE);
				} else {
					throw EModelError("power definition error: " + theEquation + " unknown Vth for " + myModelName.substr(2));
				}
			} else if ( thePermitUndefinedFlag ){
				myVoltageStack.push_back(UNKNOWN_TOKEN);
			} else {
				throw EPowerError("undefined macro: " + *token_pit);
			}
		} else if ( IsValidVoltage_(*token_pit) ) {
			myVoltageStack.push_back(from_string<float>(*token_pit));
		} else {
			throw EPowerError("invalid power calculation token: " + *token_pit);
		}
	}
	delete myTokenList_p;
	if ( myVoltageStack.size() != 1 ) throw EPowerError("invalid equation: " + theEquation);
	if ( myVoltageStack.front() > float(MAX_VOLTAGE) ) {
		cout << "Warning: equation contains undefined tokens: " << theEquation << endl;
		return(UNKNOWN_VOLTAGE);
	}
//	cout << "Final voltage " << myVoltageStack.front() << endl;
	return ( round(myVoltageStack.front() * VOLTAGE_SCALE + 0.1) );
}

voltage_t CPower::RelativeVoltage(CPowerPtrMap & thePowerMacroPtrMap, netStatus_t theType, CModelListMap & theModelListMap) {
	if ( ! extraData || IsEmpty(extraData->family) ) return UNKNOWN_VOLTAGE;
	try {
		return thePowerMacroPtrMap.CalculateVoltage(string(*(extraData->relativeSet.begin())), theType, theModelListMap);
	}
	catch (...) {
		return UNKNOWN_VOLTAGE;
	}
}

CInstancePower::CInstancePower(string theDefinition) {
	theDefinition = trim_(theDefinition);
	//size_t	myAtIndex;
	size_t	myStringEnd = theDefinition.find_first_of(" \t");  // skip #instance
	size_t	myStringBegin = theDefinition.find_first_not_of(" \t", myStringEnd);
	myStringEnd = theDefinition.find_first_of(" \t", myStringBegin);
	instanceName = theDefinition.substr(myStringBegin, myStringEnd - myStringBegin);
	myStringBegin = theDefinition.find_first_not_of(" \t", myStringEnd);
	myStringEnd = theDefinition.find_first_of(" \t", myStringBegin);
	powerFile = theDefinition.substr(myStringBegin, myStringEnd - myStringBegin);
}
