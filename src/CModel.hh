/*
 * CModel.hh
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

#ifndef CMODEL_HH_
#define CMODEL_HH_

#include "Cvc.hh"

class CDevice;
class CPowerPtrMap;

#include "CNormalValue.hh"
#include "CCondition.hh"
#include "CParameterMap.hh"
#include <regex>

class CModelCheck {
public:
	string check = "";
	string parameter = "";
	bool isVoltage = false;
	string minInclusiveText = "";
	string minExclusiveText = "";
	string maxInclusiveText = "";
	string maxExclusiveText = "";
	float minInclusiveValue = 0.;
	float minExclusiveValue = 0.;
	float maxInclusiveValue = 0.;
	float maxExclusiveValue = 0.;
	voltage_t minInclusiveVoltage = 0.;
	voltage_t minExclusiveVoltage = 0.;
	voltage_t maxInclusiveVoltage = 0.;
	voltage_t maxExclusiveVoltage = 0.;

	CModelCheck(string theCheck, string theParameter, string theInclusiveMin, string theExclusiveMin, string theInclusiveMax, string theExclusiveMax );
	void SetValue(string theParameter, voltage_t &theVoltage, float &theValue);

};

class CModel {
public:
	string	name;

	string	maxVdsDefinition = "";
	string	maxVgsDefinition = "";
	string	maxVbsDefinition = "";
	string	maxVbgDefinition = "";

	string	vthDefinition = "";

	voltage_t	maxVds = UNKNOWN_VOLTAGE;
	voltage_t	maxVgs = UNKNOWN_VOLTAGE;
	voltage_t	maxVbs = UNKNOWN_VOLTAGE;
	voltage_t	maxVbg = UNKNOWN_VOLTAGE;

	voltage_t	Vth = UNKNOWN_VOLTAGE;

	string	resistanceDefinition;

	string	baseType = "";

	bool isLDD = false;
	bool validModel = true;

	modelType_t type = UNKNOWN;

	CConditionPtrList	conditionPtrList;
	string cellFilter = "";
	regex * cellFilterRegex_p = NULL;
	list <pair<int, int>>	diodeList;

	CDevice *	firstDevice_p = NULL;

	string	definition;

	forward_list<CModelCheck> checkList;

	CModel(string theParameterString);
	void Clear();
	size_t ModelCount();
	bool ParameterMatch(CParameterMap& theParameterMap,text_t theCellName);
	void CreateConditions(string theConditionString);
	void SetDiodes (string theDiodeString);

	void Print(ostream & theLogFile = cout, bool thePrintDeviceListFlag = false, string theIndentation = "");
	string ConditionString();
};

class CModelList : public list<CModel> {
public:
	int		Vth = UNKNOWN_VOLTAGE;
	string	vthDefinition;
};

class CTextModelPtrMap : public unordered_map<text_t, CModel *> {
public:
	CTextModelPtrMap(float theLoadFactor = DEFAULT_LOAD_FACTOR) {max_load_factor(theLoadFactor);}
};

class CModelListMap : public map<string, CModelList> {
public:
	bool hasError;
	string filename;

	void Clear();
	void AddModel(string theParameterString);
	CModel * FindModel(text_t theCellName, text_t theParameterText, CTextResistanceMap & theParameterResistanceMap, ostream& theLogFile);
	CModelList * FindModelList(string theModelName);
	void Print(ostream & theLogFile, string theIndentation = "");
	void DebugPrint(string theIndentation = "");
	returnCode_t SetVoltageTolerances(teestream & theReportFile, CPowerPtrMap & thePowerMacroPtrMap);
};

#endif /* CMODEL_HH_ */
