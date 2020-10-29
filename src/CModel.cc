/*
 * CModel.cc
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

#include "CModel.hh"

#include "CCondition.hh"
#include "CParameterMap.hh"
#include "CDevice.hh"
#include "CCircuit.hh"
#include "CPower.hh"
#include <regex>

CModel::CModel(string theParameterString) {
// parameter string example
// NMOS nch Vth=0.500 Vgs=0.3 Vds=0.5 Vbs=0.4 condition=(L<0.4u w>=1.2u)
// RESISTOR fuse Vds=0.4 R=1000 condition=(r<10) model=switch_on
// BOX res1 model=resistor diode=(1-3,2-3)
	string myParameterName;
	string myParameterValue;
	size_t	myEqualIndex;
	size_t	myStringBegin = theParameterString.find_first_not_of(" \t");
	size_t	myStringEnd = theParameterString.find_first_of(" \t", myStringBegin);
	definition = theParameterString.substr(myStringBegin, myStringEnd - myStringBegin);
	type = gModelTypeStringMap.at(theParameterString.substr(myStringBegin, myStringEnd - myStringBegin));
	baseType = gBaseModelTypePrefixMap.at(type);
	if ( IsMos_(type) ) {
		resistanceDefinition = DEFAULT_MOS_RESISTANCE;
	} else if ( type == RESISTOR ) {
		resistanceDefinition = DEFAULT_RESISTANCE;
	}
	myStringBegin = theParameterString.find_first_not_of(" \t", myStringEnd);
	myStringEnd = theParameterString.find_first_of(" \t", myStringBegin);
	name = theParameterString.substr(myStringBegin, myStringEnd - myStringBegin);
	definition = definition + " " + name;

	myStringBegin = theParameterString.find_first_not_of(" \t", myStringEnd);
	while ( myStringBegin < theParameterString.length() ) {
		myStringEnd = theParameterString.find_first_of(" \t", myStringBegin);
		definition = definition + " " + theParameterString.substr(myStringBegin, myStringEnd - myStringBegin);

		myEqualIndex = theParameterString.find("=", myStringBegin);
		myParameterName = theParameterString.substr(myStringBegin, myEqualIndex - myStringBegin);
		myParameterValue = theParameterString.substr(myEqualIndex + 1, myStringEnd - (myEqualIndex + 1));
		if (myParameterName == "Vth") {
			vthDefinition = myParameterValue; // String_to_Voltage(myParameterValue);
		} else if (myParameterName == "Vgs") {
			maxVgsDefinition = myParameterValue; // String_to_Voltage(myParameterValue);
		} else if (myParameterName == "Vds") {
			maxVdsDefinition = myParameterValue; // String_to_Voltage(myParameterValue);
		} else if (myParameterName == "Vbs") {
			maxVbsDefinition = myParameterValue; // String_to_Voltage(myParameterValue);
		} else if (myParameterName == "Vbg") {
			maxVbgDefinition = myParameterValue; // String_to_Voltage(myParameterValue);
		} else if (myParameterName == "R") {
			resistanceDefinition = myParameterValue;
		} else if (myParameterName == "model") {
			if ( baseType != "M" && baseType != "R" && baseType != "C" && baseType != "X" ) throw EModelError("basetype " + baseType + "cannot be overriden as " + myParameterValue);
			if ( baseType == "M" && ( myParameterValue != "fuse_on" && myParameterValue != "fuse_off" ) ) throw EModelError("mosfet cannot be overriden as " + myParameterValue);
			if ( baseType != "X" && myParameterValue != "switch_on" && myParameterValue != "switch_off" && myParameterValue != "fuse_on" && myParameterValue != "fuse_off" ) throw EModelError("invalid override " + myParameterValue);
			type = gModelTypeStringMap.at(myParameterValue);
		} else if (myParameterName == "condition") {
			CreateConditions(myParameterValue);
		} else if (myParameterName == "diode") {
			SetDiodes(myParameterValue);
		} else {
			throw EModelError("unknown parameter '" + theParameterString.substr(myStringBegin, myStringEnd - myStringBegin) + "' in " + definition);
		}
		myStringBegin = theParameterString.find_first_not_of(" \t", myStringEnd);
	}
	if ( diodeList.empty() ) {
		if ( IsNmos_(type) ) {
			diodeList.push_back(make_pair(4,1));
			diodeList.push_back(make_pair(4,3));
		} else if ( IsPmos_(type) ) {
			diodeList.push_back(make_pair(1,4));
			diodeList.push_back(make_pair(3,4));
		} else if ( type == DIODE ) {
			diodeList.push_back(make_pair(1,2));
		}
	}
}

bool CModel::ParameterMatch(CParameterMap& theParameterMap, text_t theCellName) {
	CNormalValue myCheckValue;

	for (CConditionPtrList::iterator condition_ppit = conditionPtrList.begin(); condition_ppit != conditionPtrList.end(); condition_ppit++) {
		try {
			myCheckValue = theParameterMap.at((*condition_ppit)->parameter);
			if ( ! (*condition_ppit)->CheckCondition(myCheckValue) ) return (false);
		}
		catch (const out_of_range& oor_exception) {
			throw EFatalError("missing parameter " + (*condition_ppit)->parameter + " in " + name);
		}
	}
	if ( cellFilterRegex_p ) {
		if ( ! regex_match(theCellName, *cellFilterRegex_p) ) return false;
	}
	return (true);
}

void CModel::CreateConditions (string theConditionString) {
// condition string example
// condition=(L<0.4u w>=1.2u)
// condition=(r<10)
	string myConditionName;
	string myConditionRelation;
	string myConditionValue;
	size_t	myRelationIndex;
	size_t  myValueIndex;
	size_t	myStringBegin = theConditionString.find_first_not_of("( \t");
	size_t	myStringEnd = theConditionString.find_first_of(", )", myStringBegin);
	while ( myStringEnd < theConditionString.length() ) {
		myRelationIndex = theConditionString.find_first_of("<=>", myStringBegin);
		myValueIndex = theConditionString.find_first_not_of("<=>", myRelationIndex);
		myConditionName = trim_(theConditionString.substr(myStringBegin, myRelationIndex - myStringBegin));
		myConditionRelation = trim_(theConditionString.substr(myRelationIndex, myValueIndex - myRelationIndex));
		myConditionValue = trim_(theConditionString.substr(myValueIndex, myStringEnd - myValueIndex));
		toupper_(myConditionName);
		if ( myConditionName == "CELL" && myConditionRelation == "=" ) {
			cellFilter = myConditionValue;
			cellFilterRegex_p = new regex(FuzzyFilter(myConditionValue));
		} else {
			conditionPtrList.push_back(new CCondition(myConditionName, myConditionRelation, myConditionValue));
		}
		myStringBegin = theConditionString.find_first_not_of("), \t", myStringEnd);
		myStringEnd = theConditionString.find_first_of(", )", myStringBegin);
	}
}

void CModel::SetDiodes (string theDiodeString) {
// diode string example
// diode=(1-3,2-3)
// diode=(3-2)
	if ( ! diodeList.empty() ) {
		diodeList.clear();
	}
	int myAnode;
	int myCathode;
	size_t	myDelimterIndex;
	size_t	myStringBegin = theDiodeString.find_first_not_of("(, \t");
	size_t	myStringEnd = theDiodeString.find_first_of("), \t", myStringBegin);
	while ( myStringEnd < theDiodeString.length() ) {
		myDelimterIndex = theDiodeString.find_first_of("-", myStringBegin);
		myAnode = from_string<int>(trim_(theDiodeString.substr(myStringBegin, myDelimterIndex - myStringBegin)));
		myCathode = from_string<int>(trim_(theDiodeString.substr(myDelimterIndex+1, myStringEnd - myDelimterIndex - 1)));
		diodeList.push_back(make_pair(myAnode, myCathode));
		myStringBegin = theDiodeString.find_first_not_of("), \t", myStringEnd);
		myStringEnd = theDiodeString.find_first_of("), \t", myStringBegin);
	}
}

void CModel::Clear() {
	while ( ! conditionPtrList.empty() ) {
		delete conditionPtrList.front();
		conditionPtrList.pop_front();
	}
}

size_t CModel::ModelCount() {
	size_t myModelCount = 0;
	for (CDevice * myDevice_p = firstDevice_p; myDevice_p; myDevice_p = myDevice_p->nextDevice_p) {
		myModelCount += myDevice_p->parent_p->instanceCount;
	}
	return myModelCount;
}

void CModel::Print(ostream & theLogFile, bool thePrintDeviceListFlag, string theIndentation) {
	string myIndentation = theIndentation + " ";
	theLogFile << theIndentation << "Model> "<< setw(10) << left << name;
	theLogFile << right << setw(10) << ModelCount();
	theLogFile << " " << baseType << "->" << setw(10) << left << gModelTypeMap[type];
	theLogFile << " Parameters>";
	switch (type) {
		case NMOS: case PMOS: case LDDN: case LDDP: {
			theLogFile << " Vth=" << PrintToleranceParameter(vthDefinition, Vth, VOLTAGE_SCALE);
			if ( ! IsEmpty(maxVdsDefinition) ) theLogFile << " Vds=" << PrintToleranceParameter(maxVdsDefinition, maxVds, VOLTAGE_SCALE);
			if ( ! IsEmpty(maxVgsDefinition) ) theLogFile << " Vgs=" << PrintToleranceParameter(maxVgsDefinition, maxVgs, VOLTAGE_SCALE);
			if ( ! IsEmpty(maxVbgDefinition) ) theLogFile << " Vbg=" << PrintToleranceParameter(maxVbgDefinition, maxVbg, VOLTAGE_SCALE);
			if ( ! IsEmpty(maxVbsDefinition) ) theLogFile << " Vbs=" << PrintToleranceParameter(maxVbsDefinition, maxVbs, VOLTAGE_SCALE);
			theLogFile << " R=" << resistanceDefinition;
			break; }
		case RESISTOR: {
			if ( ! IsEmpty(maxVdsDefinition) ) theLogFile << " Vds=" << PrintToleranceParameter(maxVdsDefinition, maxVds, VOLTAGE_SCALE);
			if ( ! IsEmpty(resistanceDefinition) ) theLogFile << " R=" << resistanceDefinition;
			break; }
		case FUSE_ON:
		case FUSE_OFF:
		case CAPACITOR:
		case DIODE: {
			if ( ! IsEmpty(maxVdsDefinition) ) theLogFile << " Vds=" << PrintToleranceParameter(maxVdsDefinition, maxVds, VOLTAGE_SCALE);
			break; }
		case BIPOLAR:
		case SWITCH_ON:
		case SWITCH_OFF: {
			break; }
		default: {
			theLogFile << " Unknown type:";
		}
	}
	if ( ! conditionPtrList.empty() ) {
		theLogFile << " Conditions>";
		for (CConditionPtrList::iterator condition_ppit = conditionPtrList.begin(); condition_ppit != conditionPtrList.end(); condition_ppit++) {
			(*condition_ppit)->Print(theLogFile, "same-line");
		}
	}
	if ( cellFilter != "" ) {
		theLogFile << " Cell filter>" << cellFilter;
	}
	if ( ! diodeList.empty() ) {
		theLogFile << " Diodes>";
		for ( auto diode_pit = diodeList.begin(); diode_pit != diodeList.end(); diode_pit++ ) {
			theLogFile << " " << diode_pit->first << "-" << diode_pit->second;
		}
	}
	theLogFile << endl;
	if ( firstDevice_p && thePrintDeviceListFlag ) {
		theLogFile << myIndentation << "DeviceList> start" << endl;
		for ( CDevice * myDevice_p = firstDevice_p; myDevice_p != NULL; myDevice_p = myDevice_p->nextDevice_p ) {
			theLogFile << myIndentation << " " << myDevice_p->parent_p->name << "/" << myDevice_p->name << endl;
		}
		theLogFile << myIndentation << "DeviceList> end" << endl;
	}
}

string CModel::ConditionString() {
	stringstream myConditionString;
	if ( ! conditionPtrList.empty() ) {
		myConditionString << " Conditions>";
		for (CConditionPtrList::iterator condition_ppit = conditionPtrList.begin(); condition_ppit != conditionPtrList.end(); condition_ppit++) {
			(*condition_ppit)->Print(myConditionString, "same-line");
		}
	}
	if ( cellFilter != "" ) {
		myConditionString << " Cell filter>" << cellFilter;
	}
	return(myConditionString.str());
}

void CModelListMap::Clear() {
	while ( ! empty() ) {
		CModelList& myModelList = begin()->second;
		while ( ! myModelList.empty() ) {
			myModelList.front().Clear();
			myModelList.pop_front();
		}
		erase(begin());
	}
}

void CModelListMap::AddModel(string theParameterString) {
	try {
		CModel	myNewModel(theParameterString);
		string myModelKey = myNewModel.baseType + " " + myNewModel.name;
		try {
			this->at(myModelKey).push_back(myNewModel);
			if ( (*this)[myModelKey].vthDefinition != myNewModel.vthDefinition ) {
				cout << "Vth mismatch for " << myNewModel.name << " ignored. ";
				cout << (*this)[myModelKey].vthDefinition << "!=" << myNewModel.vthDefinition << endl;
			}
		}
		catch (const out_of_range& oor_exception) {
			(*this)[myModelKey].push_back(myNewModel);
			(*this)[myModelKey].vthDefinition = myNewModel.vthDefinition;
		}
	}
	catch (EModelError & myError) {
		cout << myError.what() << endl;
		cout << "Invalid model format: " << theParameterString << endl;
		hasError = true;
	}
	catch (...) {
		cout << "Invalid model format: " << theParameterString << endl;
		hasError = true;
	}
}

CModel * CModelListMap::FindModel(text_t theCellName, text_t theParameterText, CTextResistanceMap& theParameterResistanceMap, ostream& theLogFile) {
	string	myParameterString = trim_(string(theParameterText));
	string	myModelKey = myParameterString.substr(0, myParameterString.find(" ", 2));
	try {
		CParameterMap myParameterMap;
		if ( myParameterString.length() > myModelKey.length() ) {
			// if there are parameters besides the model name
			myParameterMap.CreateParameterMap(myParameterString.substr(myModelKey.length() + 1));
		}
		CModelList::iterator myLastModel = this->at(myModelKey).end();
		for (CModelList::iterator model_pit = this->at(myModelKey).begin(); model_pit != myLastModel; model_pit++) {
			if ( model_pit->ParameterMatch(myParameterMap, theCellName) ) {
				switch (model_pit->type) {
					case NMOS: case PMOS: case LDDN: case LDDP: {
						theParameterResistanceMap[theParameterText] = myParameterMap.CalculateResistance(model_pit->resistanceDefinition);
						if ( theParameterResistanceMap[theParameterText] == MAX_RESISTANCE ) {
							theLogFile << "WARNING: resistance for " << theParameterText << " exceeds maximum" << endl;
						}
						break; }
					case RESISTOR: {
						theParameterResistanceMap[theParameterText] = myParameterMap.CalculateResistance(model_pit->resistanceDefinition);
						if ( theParameterResistanceMap[theParameterText] == MAX_RESISTANCE ) {
							theLogFile << "WARNING: resistance for " << theParameterText << " exceeds maximum" << endl;
						}
						break; }
					default: theParameterResistanceMap[theParameterText] = 1;
				}

				return &(*model_pit);
			}
		}
		return (NULL);
	}
	catch (const out_of_range& oor_exception) {
		return (NULL);
	}
}

void CModelListMap::Print(ostream & theLogFile, string theIndentation) {
	string myIndentation = theIndentation + " ";
	theLogFile << endl << theIndentation << "ModelList> filename " << filename << endl;
	for (CModelListMap::iterator modelList_pit = begin(); modelList_pit != end(); modelList_pit++) {
		for (CModelList::iterator model_pit = modelList_pit->second.begin(); model_pit != modelList_pit->second.end(); model_pit++) {
			model_pit->Print(theLogFile, PRINT_DEVICE_LIST_OFF, myIndentation);
		}
	}
	theLogFile << theIndentation << "ModelList> end" << endl << endl;
}

void CModelListMap::DebugPrint(string theIndentation) {
	string myIndentation = theIndentation + " ";
	cout << theIndentation << "ModelList> start" << endl;
	for (CModelListMap::iterator modelList_pit = begin(); modelList_pit != end(); modelList_pit++) {
		for (CModelList::iterator model_pit = modelList_pit->second.begin(); model_pit != modelList_pit->second.end(); model_pit++) {
			model_pit->Print(cout, PRINT_DEVICE_LIST_ON, myIndentation);
		}
	}
	cout << theIndentation << "ModelList> end" << endl << endl;
}

#define PERMIT_UNDEFINED true
returnCode_t CModelListMap::SetVoltageTolerances(teestream & theReportFile, CPowerPtrMap & thePowerMacroPtrMap) {
	theReportFile << "Setting model tolerances..." << endl;
	bool myToleranceErrorFlag = false;
	for (CModelListMap::iterator modelList_pit = begin(); modelList_pit != end(); modelList_pit++) {
		for (CModelList::iterator model_pit = modelList_pit->second.begin(); model_pit != modelList_pit->second.end(); model_pit++) {
			try {
				if ( ! IsEmpty(model_pit->vthDefinition) ) {
					model_pit->Vth = thePowerMacroPtrMap.CalculateVoltage(model_pit->vthDefinition, SIM_POWER, (*this), PERMIT_UNDEFINED);
					if ( model_pit->Vth == UNKNOWN_VOLTAGE ) model_pit->validModel = false;
				}
				if ( ! IsEmpty(model_pit->maxVbgDefinition) ) {
					model_pit->maxVbg = thePowerMacroPtrMap.CalculateVoltage(model_pit->maxVbgDefinition, SIM_POWER, (*this), PERMIT_UNDEFINED);
					if ( model_pit->maxVbg == UNKNOWN_VOLTAGE ) model_pit->validModel = false;
				}
				if ( ! IsEmpty(model_pit->maxVbsDefinition) ) {
					model_pit->maxVbs = thePowerMacroPtrMap.CalculateVoltage(model_pit->maxVbsDefinition, SIM_POWER, (*this), PERMIT_UNDEFINED);
					if ( model_pit->maxVbs == UNKNOWN_VOLTAGE ) model_pit->validModel = false;
				}
				if ( ! IsEmpty(model_pit->maxVdsDefinition) ) {
					model_pit->maxVds = thePowerMacroPtrMap.CalculateVoltage(model_pit->maxVdsDefinition, SIM_POWER, (*this), PERMIT_UNDEFINED);
					if ( model_pit->maxVds == UNKNOWN_VOLTAGE ) model_pit->validModel = false;
				}
				if ( ! IsEmpty(model_pit->maxVgsDefinition) ) {
					model_pit->maxVgs = thePowerMacroPtrMap.CalculateVoltage(model_pit->maxVgsDefinition, SIM_POWER, (*this), PERMIT_UNDEFINED);
					if ( model_pit->maxVgs == UNKNOWN_VOLTAGE ) model_pit->validModel = false;
				}
			}
			catch (EPowerError & myException) {
				theReportFile << "ERROR: Model tolerance " << myException.what() << endl;
				myToleranceErrorFlag = true;
			}
		}
	}
	return (myToleranceErrorFlag) ? FAIL : OK;
}


