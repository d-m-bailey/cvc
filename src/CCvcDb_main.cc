/*
 * CCvcDb_main.cc
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

#include "Cvc.hh"
#include "CCdlParserDriver.hh"
#include "CCvcParameters.hh"
#include "CCircuit.hh"
#include "CCvcDb.hh"

#include "CDevice.hh"
#include "resource.hh"

/// \file
/// CVC main loop

/// \name interactive globals
///@{
bool gInteractive_cvc = false; //!< interactive mode
int gContinueCount = 0; //!< number of stages to run before stopping; always stop after final stage
///@}

rusage lastSnapshot; //!< resource usage tracking

// debugging variables for virtual net updates
long gVirtualNetAccessCount = 0;
long gVirtualNetUpdateCount = 0;

/**
 * \brief Main Loop: Verify circuits using settings in each verification resource file.
 *
 * Verification resource files typically have a .cvcrc suffix.
 */
void CCvcDb::VerifyCircuitForAllModes(int argc, const char * argv[]) {
	CCdlParserDriver cvcParserDriver;

	cvcParameters.PrintDefaultEnvironment();
	for ( ; cvcArgIndex < argc; cvcArgIndex++ ) { // loop through all cvcrc files on command line
/// Setup
		gContinueCount = 0;
		if ( ! cvcParameters.cvcPowerPtrList.empty() ) Cleanup();
		RemoveLock();
		detectErrorFlag = true;
		cout << "CVC: Circuit Validation Check  Version " << CVC_VERSION << endl;
		cvcParameters.ResetEnvironment();
		cvcParameters.LoadEnvironment(argv[cvcArgIndex], reportPrefix);
		if ( ! LockReport(gInteractive_cvc) ) continue;
		SetOutputFiles(cvcParameters.cvcReportFilename);
		logFile << "CVC: Circuit Validation Check  Version " << CVC_VERSION << endl;
		reportFile << "CVC: Start: " << CurrentTime() << endl;
		TakeSnapshot(&lastSnapshot);
		cvcParameters.PrintEnvironment(reportFile);
///	Read model and power settings.
		modelFileStatus = cvcParameters.LoadModels();
		powerFileStatus = cvcParameters.LoadPower();
		if ( modelFileStatus != OK || powerFileStatus != OK ) { // Catch syntax problems before reading netlist
			if ( ! gInteractive_cvc ) {
				reportFile << "ERROR: skipped due to problems in model/power files" << endl;
				continue;
			}
		}

/// Read netlist
		if ( cvcParameters.IsSameDatabase() ) {
			reportFile << "CVC: Reusing " << cvcParameters.cvcTopBlock << " of "
					<< cvcParameters.cvcNetlistFilename << endl;
/*
			if ( isDeviceModelSet ) {
				ResetMosFuse();
			}
*/
		} else {
			reportFile << "CVC: Parsing netlist " << cvcParameters.cvcNetlistFilename << endl;
			cvcCircuitList.Clear();
			instancePtr_v.Clear();
			if (cvcParserDriver.parse (cvcParameters.cvcNetlistFilename, cvcCircuitList,
					cvcParameters.cvcSOI ) != 0 ) {
				throw EFatalError("Could not parse " + cvcParameters.cvcNetlistFilename);
			}
			if (cvcCircuitList.errorCount > 0 || cvcCircuitList.warningCount > 0) {
				reportFile << "WARNING: unsupported devices in netlist" << endl;
			}
			cvcParameters.SaveDatabaseParameters();
			reportFile << "Cdl fixed data size " << cvcCircuitList.cdlText.Size() << endl;
			reportFile << PrintProgress(&lastSnapshot, "CDL ") << endl;
			CountObjectsAndLinkSubcircuits();
			AssignGlobalIDs();
			reportFile << PrintProgress(&lastSnapshot, "DB ") << endl;
		}
		reportFile << "CVC: " << topCircuit_p->subcircuitCount << " instances, "
				<< topCircuit_p->netCount << " nets, " << topCircuit_p->deviceCount
				<< " devices." << endl;

/// Stage 1) Set power and model
		if ( powerFileStatus == OK ) {
			cout << "Setting power for mode..." << endl;
			powerFileStatus = SetModePower();
		}
		if ( modelFileStatus == OK ) {
			cout << "Setting models..." << endl;
			modelFileStatus = SetDeviceModels();
		}
		if ( modelFileStatus == OK ) {
			fuseFileStatus = CheckFuses();
		} else {
			fuseFileStatus = FAIL;
		}
		if ( gInteractive_cvc && --gContinueCount < 1
				&& InteractiveCvc(STAGE_START) == SKIP ) continue;
		if ( modelFileStatus != OK || powerFileStatus != OK || fuseFileStatus != OK ) {
			reportFile << "ERROR: skipped due to problems in model/power/fuse files" << endl;
			continue;
		}

/// Stage 2) Create database
		ResetMinSimMaxAndQueues();
		SetEquivalentNets();
		if ( SetInstancePower() != OK || SetExpectedPower() != OK ) {
			reportFile << "ERROR: skipped due to problems in power files" << endl;
			continue;
		}
		cvcParameters.cvcModelListMap.Print(logFile);
		PrintPowerList(logFile, "Power List");
		cvcParameters.cvcPowerPtrList.SetPowerLimits(maxPower, minPower);
		LinkDevices();
		OverrideFuses();
		reportFile << PrintProgress(&lastSnapshot, "EQUIV ") << endl;
		reportFile << "Power nets " << CPower::powerCount << endl;
//		DumpStatistics(parameterModelPtrMap, "parameter->model map", logFile);
		DumpStatistics(parameterResistanceMap, "parameter->resistance map", logFile);
		DumpStatistics(cvcCircuitList.circuitNameMap, "text->circuit map", logFile);
		DumpStatistics(cvcCircuitList.cdlText.fixedTextToAddressMap, "string->text map", logFile);
		if ( gInteractive_cvc && --gContinueCount < 1 && InteractiveCvc(STAGE_LINK) == SKIP ) {
			continue;
		}

/// Stage 3) Calculate voltages across resistors
/// - Calculated resistance
		ShortNonConductingResistors();
		SetResistorVoltagesByPower();
		reportFile << PrintProgress(&lastSnapshot, "RES ") << endl;
		reportFile << "Power nets " << CPower::powerCount << endl;
		if ( gInteractive_cvc && --gContinueCount < 1
				&& InteractiveCvc(STAGE_RESISTANCE) == SKIP ) continue;

/// Stage 4) First min/max propagation\n
/// - unexpected min/max values\n
/// - forward bias diode errors\n
/// - NMOS source-bulk errors\n
/// - NMOS gate-source errors\n
/// - PMOS source-bulk errors\n
/// - PMOS gate-source errors\n
		ResetMinMaxPower();
		reportFile << PrintProgress(&lastSnapshot, "MIN/MAX1 ") << endl;
		reportFile << "Power nets " << CPower::powerCount << endl;
		if ( detectErrorFlag ) {
			//FindForwardBiasDiodes();
			if ( ! cvcParameters.cvcSOI ) {
				FindNmosSourceVsBulkErrors();
			}
			FindNmosGateVsSourceErrors();
			if ( ! cvcParameters.cvcSOI ) {
				FindPmosSourceVsBulkErrors();
			}
			FindPmosGateVsSourceErrors();
			reportFile << PrintProgress(&lastSnapshot, "ERROR ") << endl;
		}
		if ( gInteractive_cvc && --gContinueCount < 1
				&& InteractiveCvc(STAGE_FIRST_MINMAX) == SKIP ) continue;

/// Stage 5) First sim propagation\n
/// - missing bulk connection check
		SaveMinMaxLeakVoltages();
		SetSimPower(POWER_NETS_ONLY);
		cvcCircuitList.PrintAndResetCircuitErrors(cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Logic shorts 1");
		reportFile << PrintProgress(&lastSnapshot, "SIM1 ") << endl;
		reportFile << "Power nets " << CPower::powerCount << endl;
		if ( ! cvcParameters.cvcSOI ) {
			CheckConnections();
		}
		if ( gInteractive_cvc && --gContinueCount < 1
				&& InteractiveCvc(STAGE_FIRST_SIM) == SKIP ) continue;
		SaveInitialVoltages();
		if ( gDebug_cvc ) {
			PrintAllVirtualNets<CVirtualNetMappedVector>(
					minLeakNet_v, simNet_v, maxLeakNet_v, "(1)");
		}

/// Stage 6) Second sim propagation\n
/// - LDD connection errors
		if ( cvcParameters.cvcSCRC ) {
			SetSCRCPower();
		}
		SetSimPower(ALL_NETS_AND_FUSE);
		reportFile << PrintProgress(&lastSnapshot, "SIM2 ") << endl;
		reportFile << "Power nets " << CPower::powerCount << endl;
		while ( SetLatchPower() ) {
			SetSimPower(ALL_NETS_AND_FUSE);
			reportFile << PrintProgress(&lastSnapshot, "LATCH ") << endl;
		}
		cvcCircuitList.PrintAndResetCircuitErrors(cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Logic shorts 2");
		if ( detectErrorFlag ) {
			FindLDDErrors();
			FindForwardBiasDiodes();
		}
		if ( gInteractive_cvc && --gContinueCount < 1
				&& InteractiveCvc(STAGE_SECOND_SIM) == SKIP ) continue;

/// Stage 7) Second min/max propagation\n
/// - overvoltage errors\n
/// - NMOS possible leak errors\n
/// - PMOS possible leak errors\n
/// - floating gate errors\n
/// - expected value errors
		ResetMinMaxPower();
		SetInverterHighLow();
		reportFile << PrintProgress(&lastSnapshot, "MIN/MAX2 ") << endl;
		reportFile << "Power nets " << CPower::powerCount << endl;
		if ( detectErrorFlag ) {
			FindOverVoltageErrors("Vbg", OVERVOLTAGE_VBG);
			FindOverVoltageErrors("Vbs", OVERVOLTAGE_VBS);
			FindOverVoltageErrors("Vds", OVERVOLTAGE_VDS);
			FindOverVoltageErrors("Vgs", OVERVOLTAGE_VGS);
			FindNmosPossibleLeakErrors();
			FindPmosPossibleLeakErrors();
			FindFloatingInputErrors();
			CheckExpectedValues();
		}
		PrintErrorTotals();
//		PrintShortedNets(cvcParameters.cvcReportBaseFilename + ".shorts.gz");
		reportFile << PrintProgress(&lastSnapshot, "Total ") << endl;
		if ( gDebug_cvc ) {
			PrintAllVirtualNets<CVirtualNetVector>(minNet_v, simNet_v, maxNet_v, "(3)");
			cvcCircuitList.Print("", "CVC Full Circuit List");
			cvcParameters.cvcModelListMap.DebugPrint();
			Print("", "CVC Database");
			PrintFlatCdl();
		}
		reportFile << "Virtual net update/access " << gVirtualNetUpdateCount << "/"
				<< gVirtualNetAccessCount << endl;
		reportFile << "CVC: Log output to " << cvcParameters.cvcReportFilename << endl;
		reportFile << "CVC: End: " << CurrentTime() << endl;
		errorFile.close();
		if ( gInteractive_cvc ) InteractiveCvc(STAGE_COMPLETE);

/// Clean-up
		logFile.close();
		debugFile.close();
	}
	Cleanup();
	RemoveLock();
}



