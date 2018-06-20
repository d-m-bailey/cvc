/*
 * Copyright (c) D. Mitch Bailey 2014.
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

#include "CCvcParameters.hh"

#include "wordexp.h"

/*
CCvcParameters::CCvcParameters() {
//	cvcDate = "20130731";
//	cvcTopBlock = "CVC_TEST";
//	cvcFileName = "CVC_TEST.cdl";
//	cvcReportTitle = "CVC TEST CIRCUIT";
//	cvcHtmlFilenameTemplate = cvcTopBlock + "_" + cvcDate + "_";
}

CCvcParameters::CCvcParameters(const int argc, const char * argv[]) {
//	cvcDate = "20130731";
//	cvcTopBlock = "CVC_TEST";
//	cvcFileName = "CVC_TEST.cdl";
//	cvcReportTitle = "CVC TEST CIRCUIT";
//	cvcHtmlFilenameTemplate = cvcTopBlock +	"_" + cvcDate + "_";
	if (argc > 1) {
//		cvcFileName.assign(argv[1]);
		LoadEnvironment(argv[1]);
//		cvcFileName.assign(getenv("CVC_NETLIST"));
	}
	if (argc > 2 && strcmp(argv[2], "--debug") == 0) debug_cvc = true;

}
*/

string CCvcParameters::CvcFileName() {
	return cvcNetlistFilename;
}

bool CCvcParameters::IsSameDatabase() {
	return(cvcLastTopBlock == cvcTopBlock && cvcLastNetlistFilename == cvcNetlistFilename && cvcLastSOI == cvcSOI);
}
void CCvcParameters::SaveDatabaseParameters() {
		cvcLastTopBlock = cvcTopBlock;
		cvcLastNetlistFilename = cvcNetlistFilename;
		cvcLastSOI = cvcSOI;
}

void CCvcParameters::ResetEnvironment() {
	//! Sets CVC environment variables to defaults.

//	cvcLastTopBlock = cvcTopBlock;
//	cvcLastNetlistFilename = cvcNetlistFilename;
//	cvcLastSOI = cvcSOI;
	/*!
	 * cvcLast* variables uniquely identify the last CDL file input.
	 * If these values are the same for the next run,
	 * the current database is used and the CDL file is NOT reread.
	 */
	cvcTopBlock = "";
	cvcNetlistFilename = "";
	cvcModelFilename = "";
	cvcPowerFilename = "";
	cvcFuseFilename = "";
	//! The cvcFuseFilename is an optional parameter used to override the type (fuse_on/fuse_off) of specific fuses.
	//! The fuse file can be created from the interactive menu command 'dumpfuse filename'.
	cvcReportFilename = "";
	cvcReportTitle = "";
	cvcCircuitErrorLimit = defaultErrorLimit;
	//! Default error limit for a given error at a given device in a given circuit is cvcCircuitErrorLimit (default 100).
	//! Once the error limit is exceeded, specific device information is NOT printed.
	cvcLeakLimit = defaultLeakLimit;
	//! Leak errors involving overridden or calculated voltages are flagged if the leak current is >= cvcLeakLimit (default 200uA).
	cvcSOI = defaultSOI;
	//! cvcSOI (Silicon-On-Insolator) is false for 4 terminal (D-G-S-B) mos.
	//! To ignore the bulk connection and corresponding errors, set cvcSOI = true;
	cvcSCRC = defaultSCRC;
	//! cvcSCRC (Sub-threshhold Current Reduction Circuit)
	//! When true, calculates expected SCRC levels after first propagation.
	cvcVthGates = defaultVthGates;
	//! When true, detects gate-source errors at Vth. Default is to ignore errors at exactly Vth.
	cvcLeakOvervoltage = defaultLeakOvervoltage;
	//! When true, detects worst case overvoltage errors. Default is to flag all errors including those not possible with current mode logic.
	cvcLogicDiodes = defaultLogicDiodes;
	//! When true, uses logic values, if known, for diode checks. Default is to ignore logic values.
	cvcShortErrorThreshold = defaultErrorThreshold;
	cvcBiasErrorThreshold = defaultErrorThreshold;
	cvcForwardErrorThreshold = defaultErrorThreshold;
	cvcGateErrorThreshold = defaultErrorThreshold;
	cvcLeakErrorThreshold = defaultErrorThreshold;
	//! Ignore errors with voltage difference less than the threshold. Default is 0, flag errors regardless of voltage difference.
}

void CCvcParameters::PrintEnvironment(ostream & theOutputFile) {
	theOutputFile << "Using the following parameters for CVC (Circuit Validation Check) from " << cvcParamterFilename << endl;
	theOutputFile << "CVC_TOP = '" << cvcTopBlock << "'" << endl;
	theOutputFile << "CVC_NETLIST = '" << cvcNetlistFilename << "'" << endl;
	theOutputFile << "CVC_MODE = '" << cvcMode << "'" << endl;
	theOutputFile << "CVC_MODEL_FILE = '" << cvcModelFilename << "'" << endl;
	theOutputFile << "CVC_POWER_FILE = '" << cvcPowerFilename << "'" << endl;
	theOutputFile << "CVC_FUSE_FILE = '" << cvcFuseFilename << "'" << endl;
	theOutputFile << "CVC_REPORT_FILE = '" << cvcReportFilename << "'" << endl;
	theOutputFile << "CVC_REPORT_TITLE = '" << cvcReportTitle << "'" << endl;
	theOutputFile << "CVC_CIRCUIT_ERROR_LIMIT = '" << cvcCircuitErrorLimit << "'" << endl;
	theOutputFile << "CVC_SEARCH_LIMIT = '" << cvcSearchLimit << "'" << endl;
	theOutputFile << "CVC_LEAK_LIMIT = '" << cvcLeakLimit << "'" << endl;
	theOutputFile << "CVC_SOI = '" << (( cvcSOI ) ? "true" : "false") << "'" << endl;
	theOutputFile << "CVC_SCRC = '" << (( cvcSCRC ) ? "true" : "false") << "'" << endl;
	theOutputFile << "CVC_VTH_GATES = '" << (( cvcVthGates ) ? "true" : "false") << "'" << endl;
	theOutputFile << "CVC_LEAK_OVERVOLTAGE = '" << (( cvcLeakOvervoltage ) ? "true" : "false") << "'" << endl;
	theOutputFile << "CVC_LOGIC_DIODES = '" << (( cvcLogicDiodes ) ? "true" : "false") << "'" << endl;
	theOutputFile << "CVC_SHORT_ERROR_THRESHOLD = '" << Voltage_to_float(cvcShortErrorThreshold) << "'" << endl;
	theOutputFile << "CVC_BIAS_ERROR_THRESHOLD = '" << Voltage_to_float(cvcBiasErrorThreshold) << "'" << endl;
	theOutputFile << "CVC_FORWARD_ERROR_THRESHOLD = '" << Voltage_to_float(cvcForwardErrorThreshold) << "'" << endl;
	theOutputFile << "CVC_GATE_ERROR_THRESHOLD = '" << Voltage_to_float(cvcGateErrorThreshold) << "'" << endl;
	theOutputFile << "CVC_LEAK?_ERROR_THRESHOLD = '" << Voltage_to_float(cvcLeakErrorThreshold) << "'" << endl;
	theOutputFile << "End of parameters" << endl << endl;
}

void CCvcParameters::PrintDefaultEnvironment() {
	string myDefaultCvcrcFilename = "default.cvcrc";
	ifstream myTemporaryCvcrc(myDefaultCvcrcFilename);
	if ( myTemporaryCvcrc.good() ) return; // file exists, do nothing
	ofstream myDefaultCvcrc(myDefaultCvcrcFilename);
	if ( myDefaultCvcrc.fail() ) return; // ignore errors

	cout << "Default cvcrc in " << myDefaultCvcrcFilename << endl;
	myDefaultCvcrc << "CVC_TOP = '" << cvcTopBlock << "'" << endl;
	myDefaultCvcrc << "CVC_NETLIST = '" << cvcNetlistFilename << "'" << endl;
	myDefaultCvcrc << "CVC_MODE = '" << cvcMode << "'" << endl;
	myDefaultCvcrc << "CVC_MODEL_FILE = '" << cvcModelFilename << "'" << endl;
	myDefaultCvcrc << "CVC_POWER_FILE = '" << cvcPowerFilename << "'" << endl;
	myDefaultCvcrc << "CVC_FUSE_FILE = '" << cvcFuseFilename << "'" << endl;
	myDefaultCvcrc << "CVC_REPORT_FILE = '" << cvcReportFilename << "'" << endl;
	myDefaultCvcrc << "CVC_REPORT_TITLE = '" << cvcReportTitle << "'" << endl;
	myDefaultCvcrc << "CVC_CIRCUIT_ERROR_LIMIT = '" << cvcCircuitErrorLimit << "'" << endl;
	myDefaultCvcrc << "CVC_CIRCUIT_SEARCH_LIMIT = '" << cvcSearchLimit << "'" << endl;
	myDefaultCvcrc << "CVC_LEAK_LIMIT = '" << cvcLeakLimit << "'" << endl;
	myDefaultCvcrc << "CVC_SOI = '" << (( cvcSOI ) ? "true" : "false") << "'" << endl;
	myDefaultCvcrc << "CVC_SCRC = '" << (( cvcSCRC ) ? "true" : "false") << "'" << endl;
	myDefaultCvcrc << "CVC_VTH_GATES = '" << (( cvcVthGates ) ? "true" : "false") << "'" << endl;
	myDefaultCvcrc << "CVC_LEAK_OVERVOLTAGE = '" << (( cvcLeakOvervoltage ) ? "true" : "false") << "'" << endl;
	myDefaultCvcrc << "CVC_LOGIC_DIODES = '" << (( cvcLogicDiodes ) ? "true" : "false") << "'" << endl;
	myDefaultCvcrc << "CVC_SHORT_ERROR_THRESHOLD = '" << Voltage_to_float(cvcShortErrorThreshold) << "'" << endl;
	myDefaultCvcrc << "CVC_BIAS_ERROR_THRESHOLD = '" << Voltage_to_float(cvcBiasErrorThreshold) << "'" << endl;
	myDefaultCvcrc << "CVC_FORWARD_ERROR_THRESHOLD = '" << Voltage_to_float(cvcForwardErrorThreshold) << "'" << endl;
	myDefaultCvcrc << "CVC_GATE_ERROR_THRESHOLD = '" << Voltage_to_float(cvcGateErrorThreshold) << "'" << endl;
	myDefaultCvcrc << "CVC_LEAK?_ERROR_THRESHOLD = '" << Voltage_to_float(cvcLeakErrorThreshold) << "'" << endl;
	myDefaultCvcrc.close();
}

void CCvcParameters::LoadEnvironment(const string theEnvironmentFilename, const string theReportPrefix) {
	//! Set CVC environment variables from cvcrc file.
	/*!
	 * CVC environment files can access and define shell environment variables.
	 */
	ifstream myEnvironmentFile(theEnvironmentFilename);
	if ( myEnvironmentFile.fail() ) {
//		cout << "ERROR: Could not open " << theEnvironmentFilename << endl;
		throw EFatalError("Could not open " + theEnvironmentFilename);
	}
	cvcParamterFilename = theEnvironmentFilename;
	string myTuple, myVariable, myValue;
//	wordexp_t myWordExpansion;
	char myBuffer[1024];

	while ( getline(myEnvironmentFile, myTuple) ) {
		if ( myTuple[0] == '#' ) continue; // skip comments
		if ( myTuple.find_first_not_of(" \t\n") > myTuple.length() ) continue; // skip blank lines
		if ( myTuple.find("=") > myTuple.length() ) {
			cout << "Invalid environment setting: " << myTuple << endl;
			throw EBadEnvironment();
		}
		myVariable = trim_(myTuple.substr(0, myTuple.find("=")));
		myValue = trim_(myTuple.substr(myTuple.find("=") + 1));
//		if ( wordexp(myValue.c_str(), &myWordExpansion, WRDE_APPEND | WRDE_NOCMD | WRDE_SHOWERR | WRDE_UNDEF) ) throw EBadEnvironment();
		string myEchoCommand = "echo " + myValue;
		FILE * myEcho = popen(myEchoCommand.c_str(), "r");
		fgets(myBuffer, 1024, myEcho);
		myBuffer[strlen(myBuffer) - 1] = '\0';
		setenv(myVariable.c_str(), myBuffer, 1);
//		cout << myVariable << " = '" << myBuffer << "'" << endl;
		pclose(myEcho);
		if ( myVariable == "CVC_TOP" ) {
			cvcTopBlock = myBuffer;
		} else if ( myVariable == "CVC_NETLIST" ) {
			cvcNetlistFilename = myBuffer;
		} else if ( myVariable == "CVC_MODE" ) {
			cvcMode = myBuffer;
		} else if ( myVariable == "CVC_MODEL_FILE" ) {
			cvcModelFilename = myBuffer;
		} else if ( myVariable == "CVC_POWER_FILE" ) {
			cvcPowerFilename = myBuffer;
		} else if ( myVariable == "CVC_FUSE_FILE" ) {
			cvcFuseFilename = myBuffer;
		} else if ( myVariable == "CVC_REPORT_FILE" ) {
			cvcReportFilename = myBuffer;
			if ( cvcReportFilename.find_first_of("\\/") < cvcReportFilename.length() ) {
				cvcReportDirectory = cvcReportFilename.substr(0, cvcReportFilename.find_last_of("\\/") + 1);
				cvcReportName = cvcReportFilename.substr(cvcReportFilename.find_last_of("\\/") + 1);
			} else {
				cvcReportDirectory = "";
				cvcReportName = cvcReportFilename;
			}
		} else if ( myVariable == "CVC_REPORT_TITLE" ) {
			cvcReportTitle = myBuffer;
		} else if ( myVariable == "CVC_CIRCUIT_ERROR_LIMIT" ) {
			cvcCircuitErrorLimit = from_string<deviceId_t>(myBuffer);
		} else if ( myVariable == "CVC_LEAK_LIMIT" ) {
			cvcLeakLimit = from_string<float>(myBuffer);
		} else if ( myVariable == "CVC_SEARCH_LIMIT" ) {
			cvcSearchLimit = from_string<int>(myBuffer);
		} else if ( myVariable == "CVC_SOI" ) {
			cvcSOI = strcasecmp(myBuffer, "true") == 0;
		} else if ( myVariable == "CVC_SCRC" ) {
			cvcSCRC = strcasecmp(myBuffer, "true") == 0;
		} else if ( myVariable == "CVC_VTH_GATES" ) {
			cvcVthGates = strcasecmp(myBuffer, "true") == 0;
		} else if ( myVariable == "CVC_LEAK_OVERVOLTAGE" ) {
			cvcLeakOvervoltage = strcasecmp(myBuffer, "true") == 0;
		} else if ( myVariable == "CVC_LOGIC_DIODES" ) {
			cvcLogicDiodes = strcasecmp(myBuffer, "true") == 0;
		} else if ( myVariable == "CVC_SHORT_ERROR_THRESHOLD" ) {
			cvcShortErrorThreshold = String_to_Voltage(string(myBuffer));
		} else if ( myVariable == "CVC_BIAS_ERROR_THRESHOLD" ) {
			cvcBiasErrorThreshold = String_to_Voltage(string(myBuffer));
		} else if ( myVariable == "CVC_FORWARD_ERROR_THRESHOLD" ) {
			cvcForwardErrorThreshold = String_to_Voltage(string(myBuffer));
		} else if ( myVariable == "CVC_GATE_ERROR_THRESHOLD" ) {
			cvcGateErrorThreshold = String_to_Voltage(string(myBuffer));
		} else if ( myVariable == "CVC_LEAK?_ERROR_THRESHOLD" ) {
			cvcLeakErrorThreshold = String_to_Voltage(string(myBuffer));
		}
	}
	if ( ! IsEmpty(theReportPrefix) ) {
		cvcReportName = theReportPrefix + "-" + cvcReportName;
	}
/*
	if ( gInteractive_cvc ) {
		cvcReportName = "i-" + cvcReportName;
//		size_t myBaseNameOffset = cvcReportFilename.find_last_of("/\\");
//		if ( myBaseNameOffset < cvcReportFilename.length() ) {
//			cvcReportFilename = cvcReportFilename.substr(0, myBaseNameOffset + 1) + "i-" + cvcReportFilename.substr(myBaseNameOffset + 1);
//		} else {
//			cvcReportFilename = "i-" + cvcReportFilename;
//		}
	}
*/
	cvcReportFilename = cvcReportDirectory + cvcReportName;
	cvcLockFile = cvcReportDirectory + "." + cvcReportName;
	myEnvironmentFile.close();
}

void CCvcParameters::AddTestModels() {
	cvcModelListMap.AddModel("NMOS N Vth=0.3 Vgs=1.2 Vds=1.2");
	cvcModelListMap.AddModel("PMOS P Vth=-0.4 Vgs=1.2 Vds=1.2");
	cvcModelListMap.AddModel("NMOS MN Vth=0.35 Vgs=5.0 Vds=5.0");
	cvcModelListMap.AddModel("PMOS MP Vth=-0.45 Vgs=5.0 Vds=5.0");
	cvcModelListMap.AddModel("CAPACITOR CAP Vds=1.2");
	cvcModelListMap.AddModel("RESISTOR RES");
	cvcModelListMap.AddModel("RESISTOR RESSW model=switch_on");
	cvcModelListMap.AddModel("RESISTOR RESWON model=switch_on");
	cvcModelListMap.AddModel("RESISTOR RESWOFF model=switch_off");
	cvcModelListMap.AddModel("RESISTOR FUSEON model=fuse_on");
	cvcModelListMap.AddModel("RESISTOR FUSEOFF model=fuse_off");
	cvcModelListMap.AddModel("CAPACITOR CAPSW model=switch_off");
	cvcModelListMap.AddModel("DIODE DIO");
}

returnCode_t CCvcParameters::LoadModels() {
	ifstream myModelFile(cvcModelFilename);
	cvcModelListMap.Clear();
	cvcModelListMap.filename = cvcModelFilename;
	if ( myModelFile.fail() ) {
		reportFile << "ERROR: Could not open " << cvcModelFilename << endl;
		return (FAIL);
//		throw EFatalError("Could not open " + cvcModelFilename);
//		exit(1);
	}
	string myInput, myVariable, myValue;
//	wordexp_t myWordExpansion;
//	char myBuffer[1024];

	reportFile << "CVC: Reading device model settings..." << endl;
	cvcModelListMap.hasError = false;
	while ( getline(myModelFile, myInput) ) {
//		myModelFile.getline(myBuffer, 1024);
//		myInput = myBuffer;
		if ( myInput[0] == '#' ) continue; // skip comments
		if ( myInput.find_first_not_of(" \t\n") > myInput.length() ) continue; // skip blank lines
		cvcModelListMap.AddModel(myInput);
	}
	myModelFile.close();
	if ( cvcModelListMap.hasError ) {
		reportFile << "Invalid model file" << endl;
		return(SKIP);
	} else {
		return(OK);
	}

}

/*
void CCvcParameters::AddTestPower() {

	cvcPowerPtrList.push_back(new CPower("VDD 1.1"));
	cvcPowerPtrList.push_back(new CPower("VSS -0.5"));
	cvcPowerPtrList.push_back(new CPower("IN1 min@0.5 sim@1.2 max@1.8"));
	cvcPowerPtrList.push_back(new CPower("IN2 min@0.5 max@1.2"));
	cvcPowerPtrList.push_back(new CPower("OUT expect@1.3"));

	cvcPowerPtrList.push_back(new CPower("VDD 1.2"));
	cvcPowerPtrList.push_back(new CPower("VSS 0.0"));
	cvcPowerPtrList.push_back(new CPower("VCC 3.3"));
	cvcPowerPtrList.push_back(new CPower("VBB -0.5"));
	cvcPowerPtrList.push_back(new CPower("INH min@0.0 sim@1.2 max@1.2"));
	cvcPowerPtrList.push_back(new CPower("INL min@0.0 sim@0.0 max@1.2"));
	cvcPowerPtrList.push_back(new CPower("INX min@0.0 max@1.2"));
	cvcPowerPtrList.push_back(new CPower("OUTH expect@1.2"));
	cvcPowerPtrList.push_back(new CPower("OUTL expect@0.0"));
}
*/

returnCode_t CCvcParameters::LoadPower() {
	ifstream myPowerFile(cvcPowerFilename);
	cvcPowerPtrList.Clear();
	cvcExpectedLevelPtrList.Clear();
	cvcPowerFamilyMap.clear();
	cvcPowerMacroPtrMap.clear();
	if ( myPowerFile.fail() ) {
		reportFile << "ERROR: Could not open " << cvcPowerFilename << endl;
		return (FAIL);
//		throw EFatalError("Could not open " + cvcPowerFilename);
//		exit(1);
	}
	string myInput, myVariable, myValue;
//	wordexp_t myWordExpansion;

	reportFile << "CVC: Reading power settings..." << endl;
	bool myPowerErrorFlag = false;
	bool myAutoMacroFlag = true;
	while ( getline(myPowerFile, myInput) ) {
		try {
			bool myIsMacro = ( myInput.substr(0, myInput.find_first_of(" \t", 0)) == "#define" );
			bool myIsInstance = ( myInput.substr(0, myInput.find_first_of(" \t", 0)) == "#instance" );
			if ( myInput == "#NO AUTO MACROS" ) myAutoMacroFlag = false;
			if ( myInput[0] == '#' && ! (myIsMacro || myIsInstance) ) continue; // skip comments
			if ( myInput.find_first_not_of(" \t\n") > myInput.length() ) continue; // skip blank lines
			myInput = trim_(myInput);
	/*  // handle bus signals in SetModePower
			size_t mySignalEnd = myInput.find_first_of(" \t");
			size_t myBusBegin = myInput.find_first_of("<");
			size_t myBusDelimiter = myInput.find_first_of(":");
			size_t myBusEnd = myInput.find_first_of(">");

			if ( myBusBegin < myBusDelimiter && myBusDelimiter < myBusEnd && myBusEnd < mySignalEnd ) {
	//			cout << "Adding bus power definition:" << myInput << endl;

				string myBaseSignal = myInput.substr(0, myBusBegin + 1);
				int myFirstBusIndex = from_string<int>(myInput.substr(myBusBegin + 1, myBusDelimiter - myBusBegin - 1));
				int myLastBusIndex = from_string<int>(myInput.substr(myBusDelimiter + 1, myBusEnd - myBusDelimiter - 1));
				for ( int myBusIndex = min(myFirstBusIndex, myLastBusIndex); myBusIndex <= max(myFirstBusIndex, myLastBusIndex); myBusIndex++ ) {
					string myPowerDefinition = myBaseSignal + to_string<int>(myBusIndex) + myInput.substr(myBusEnd);
					cvcPowerPtrList.push_back(new CPower(myPowerDefinition));
	//				cout << "  Added:" << myPowerDefinition << endl;
				}
			} else
	*/
//			cout << "Power definition: " << myInput << endl;
			string myMacroName = "";
			string myMacroDefinition;
			if ( myIsMacro ) {
				myMacroDefinition = myInput.substr(myInput.find_first_not_of(" \t\n", 7));
				if ( myMacroDefinition.substr(0, myMacroDefinition.find_first_of(" \t", 0)) == "family" ) {
					cvcPowerFamilyMap.AddFamily(myMacroDefinition);
				} else {
					myMacroName = myMacroDefinition.substr(0, myMacroDefinition.find_first_of(" \t", 0));
				}
			} else if ( myIsInstance ) {
				cvcInstancePowerPtrList.push_back(new CInstancePower(myInput));
			} else {
				CPower * myPowerPtr = new CPower(myInput, cvcPowerMacroPtrMap, cvcModelListMap);
				if ( ! (IsEmpty(myPowerPtr->expectedMin()) && IsEmpty(myPowerPtr->expectedSim()) && IsEmpty(myPowerPtr->expectedMax())) ) {
					cvcExpectedLevelPtrList.push_back(new CPower(myPowerPtr));  // duplicate CPower (not a bit-wise copy)
				}
				if (myPowerPtr->type != NO_TYPE || myPowerPtr->minVoltage != UNKNOWN_VOLTAGE || myPowerPtr->simVoltage != UNKNOWN_VOLTAGE || myPowerPtr->maxVoltage != UNKNOWN_VOLTAGE) {
					cvcPowerPtrList.push_back(new CPower(myPowerPtr));  // duplicate CPower (not a bit-wise copy)
				}
				if ( myAutoMacroFlag ) {
					myMacroName = string(myPowerPtr->powerSignal());
					if ( myMacroName[0] == '/' ) { // macros for top level nets that are not ports
						myMacroName = myMacroName.substr(1);
					}
					myMacroDefinition = myInput;
				} else {
					myMacroName = "?";
				}
				delete myPowerPtr;
			}
			if ( myMacroName.find_first_of("(<[}/*@+-") > myMacroName.length() && isalpha(myMacroName[0]) ) { // no special characters in macro names
				if ( cvcPowerMacroPtrMap.count(myMacroName) > 0 ) throw EPowerError("duplicate macro name: " + myMacroName);
				cvcPowerMacroPtrMap[myMacroName] = new CPower(myMacroDefinition, cvcPowerMacroPtrMap, cvcModelListMap);
			}
		}
		catch (const EPowerError& myException) {
			reportFile << myException.what() << endl;
			myPowerErrorFlag = true;
		}
	}
	for ( auto instance_ppit = cvcInstancePowerPtrList.begin(); instance_ppit != cvcInstancePowerPtrList.end(); instance_ppit++ ) {
		ifstream myInstancePowerFile((*instance_ppit)->powerFile);
		if ( myInstancePowerFile.fail() ) {
			reportFile << "ERROR: Could not open " << (*instance_ppit)->powerFile << endl;
			return (FAIL);
		}
		while ( getline(myInstancePowerFile, myInput) ) {
			if ( myInput[0] == '#' ) continue; // skip comments, macros, and instances (no instance in instance)
			if ( myInput.find_first_not_of(" \t\n") > myInput.length() ) continue; // skip blank lines
			myInput = trim_(myInput);
			(*instance_ppit)->powerList.push_back(myInput);
		}
		myInstancePowerFile.close();
	}
	cvcPowerPtrList.SetFamilies(cvcPowerFamilyMap);
	SetHiZPropagation();
	myPowerFile.close();
	if ( myPowerErrorFlag ) {
		reportFile << "Invalid power file: " << cvcPowerFilename << endl;
		return (SKIP);
	} else {
		return (OK);
	}
}

void CCvcParameters::SetHiZPropagation() {
	for ( auto power_ppit = cvcPowerPtrList.begin(); power_ppit != cvcPowerPtrList.end(); power_ppit++ ) {
		if ( (*power_ppit)->type[HIZ_BIT] && ! IsEmpty((*power_ppit)->family()) ) {
			if ( (*power_ppit)->RelativeVoltage(cvcPowerMacroPtrMap, MIN_POWER, cvcModelListMap) == (*power_ppit)->minVoltage ) {
				(*power_ppit)->active[MIN_IGNORE] = true;
			}
			if ( (*power_ppit)->RelativeVoltage(cvcPowerMacroPtrMap, MAX_POWER, cvcModelListMap) == (*power_ppit)->maxVoltage ) {
				(*power_ppit)->active[MAX_IGNORE] = true;
			}
		}
	}
}

void CCvcParameters::PrintPowerList(ostream & theLogFile, string theIndentation) {
	string myIndentation = theIndentation + " ";
	theLogFile << endl << theIndentation << "PowerList> filename " << cvcPowerFilename << endl;
	for (CPowerPtrList::iterator power_ppit = cvcPowerPtrList.begin(); power_ppit != cvcPowerPtrList.end(); power_ppit++) {
		(*power_ppit)->Print(theLogFile, myIndentation);
	}
	theLogFile << theIndentation << "PowerList> end" << endl << endl;
}
