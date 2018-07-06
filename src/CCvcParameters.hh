/*
 * CCvcParameters.hh
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

#ifndef CCVCPARAMETERS_HH_
#define CCVCPARAMETERS_HH_

#include "Cvc.hh"

#include "CModel.hh"
#include "CPower.hh"

#include <cstring>

class CModeList {

};

class CCvcParameters {
public:
	const deviceId_t defaultErrorLimit = 100;
	const float	defaultLeakLimit = 200e-6;
	const int defaultSearchLimit = 100;
	const bool defaultSOI = false;
	const bool defaultSCRC = false;
	const bool defaultVthGates = false;
	const bool defaultMinVthGates = false;
	const bool defaultLeakOvervoltage = true;
	const bool defaultLogicDiodes = false;
	const voltage_t defaultErrorThreshold = 0;

	string	cvcReportTitle;

	string	cvcParamterFilename = "";
	string	cvcTopBlock = "";
	string	cvcNetlistFilename = "";
	string	cvcMode = "";
	string	cvcModelFilename;
	string	cvcPowerFilename;
	string	cvcFuseFilename = "";
	string	cvcReportFilename;
	string	cvcReportDirectory;
	string	cvcReportName;
	string	cvcLockFile;
	string	cvcReportBaseFilename;
	CModelListMap	cvcModelListMap;
	CPowerPtrList	cvcPowerPtrList;
	CPowerPtrList	cvcExpectedLevelPtrList;
	CPowerPtrMap	cvcPowerMacroPtrMap;
	CInstancePowerPtrList	cvcInstancePowerPtrList;
	CPowerFamilyMap cvcPowerFamilyMap;
	deviceId_t	cvcCircuitErrorLimit = defaultErrorLimit;
	float	cvcLeakLimit = defaultLeakLimit;
	size_t		cvcSearchLimit = defaultSearchLimit;
	bool	cvcSOI = defaultSOI;
	bool	cvcSCRC = defaultSCRC;
	bool	cvcVthGates = defaultVthGates;
	bool    cvcMinVthGates = defaultMinVthGates;
	bool	cvcLeakOvervoltage = defaultLeakOvervoltage;
	bool	cvcLogicDiodes = defaultLogicDiodes;
	voltage_t	cvcShortErrorThreshold = defaultErrorThreshold;
	voltage_t	cvcBiasErrorThreshold = defaultErrorThreshold;
	voltage_t	cvcForwardErrorThreshold = defaultErrorThreshold;
	voltage_t	cvcGateErrorThreshold = defaultErrorThreshold;
	voltage_t	cvcLeakErrorThreshold = defaultErrorThreshold;
	voltage_t	cvcExpectedErrorThreshold = defaultErrorThreshold;

	string	cvcLastTopBlock;
	string	cvcLastNetlistFilename;
	bool	cvcLastSOI;

	teestream& reportFile;

	CCvcParameters(teestream& theReportFile) : reportFile(theReportFile) {};
//	CCvcParameters(int argc, const char * argv[]);

	string CvcFileName();
	bool	IsSameDatabase();
	void	SaveDatabaseParameters();
	void	ResetEnvironment();
	void	PrintEnvironment(ostream & theOutputFile = cout);
	void	PrintDefaultEnvironment();
	void	LoadEnvironment(const string theEnvironmentFilename, const string theReportPrefix);
	returnCode_t	LoadModels();
	returnCode_t	LoadPower();
	void	AddTestModels();
	void	AddTestPower();
	void	SetHiZPropagation();
	void	PrintPowerList(ostream & theLogFile, string theIndentation = "");
};

#endif /* CCVCPARAMETERS_HH_ */
