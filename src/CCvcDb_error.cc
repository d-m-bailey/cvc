/*
 * CCvcDb_error.cc
 *
 * Copyright 2014-2020 D. Mitch Bailey  cvc at shuharisystem dot com
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

#include "CCvcDb.hh"

#include "CCircuit.hh"
#include "CConnectionCount.hh"
#include "CModel.hh"
#include "CDevice.hh"
#include "CPower.hh"
#include "CInstance.hh"
#include "CEventQueue.hh"
#include "CVirtualNet.hh"
#include <stdio.h>

void CCvcDb::PrintFuseError(netId_t theTargetNetId, CConnection & theConnections) {
	if ( IncrementDeviceError(theConnections.deviceId, FUSE_ERROR) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
		CFullConnection myFullConnections;
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[theConnections.deviceId]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[theConnections.deviceId - myInstance_p->firstDeviceId];
		MapDeviceNets(myInstance_p, myDevice_p, myFullConnections);
		errorFile << "! Power/Ground path through fuse at " << NetName(theTargetNetId, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF);
		if ( deviceType_v[theConnections.deviceId] == FUSE_ON ) {
			errorFile << "; possible floating when cut" << endl;
		} else if ( deviceType_v[theConnections.deviceId] == FUSE_OFF ) {
			errorFile << "; possibly unusable" << endl;
		} else {
			errorFile << "; unknown fuse type" << endl;
		}
		PrintDeviceWithAllConnections(deviceParent_v[theConnections.deviceId], myFullConnections, errorFile);
		errorFile << endl;
	}
}

void CCvcDb::PrintMinVoltageConflict(netId_t theTargetNetId, CConnection & theMinConnections, voltage_t theExpectedVoltage, float theLeakCurrent) {
	if ( IncrementDeviceError(theMinConnections.deviceId, MIN_VOLTAGE_CONFLICT) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
		CFullConnection myFullConnections;
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[theMinConnections.deviceId]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[theMinConnections.deviceId - myInstance_p->firstDeviceId];
		MapDeviceNets(myInstance_p, myDevice_p, myFullConnections);
		errorFile << "! Min voltage already set for " << NetName(theTargetNetId, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF);
		errorFile << " at mos diode: expected/found " << theExpectedVoltage << "/" << theMinConnections.gateVoltage;
		errorFile << " estimated current " << AddSiSuffix(theLeakCurrent) << "A" << endl;
		PrintDeviceWithAllConnections(deviceParent_v[theMinConnections.deviceId], myFullConnections, errorFile);
		errorFile << endl;
	}
}

void CCvcDb::PrintMaxVoltageConflict(netId_t theTargetNetId, CConnection & theMaxConnections, voltage_t theExpectedVoltage, float theLeakCurrent) {
	if ( IncrementDeviceError(theMaxConnections.deviceId, MAX_VOLTAGE_CONFLICT) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
		CFullConnection myFullConnections;
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[theMaxConnections.deviceId]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[theMaxConnections.deviceId - myInstance_p->firstDeviceId];
		MapDeviceNets(myInstance_p, myDevice_p, myFullConnections);
		errorFile << "! Max voltage already set for " << NetName(theTargetNetId, PRINT_CIRCUIT_ON, PRINT_HIERARCHY_OFF);
		errorFile << " at mos diode: expected/found " << theExpectedVoltage << "/" << theMaxConnections.gateVoltage;
		errorFile << " estimated current " << AddSiSuffix(theLeakCurrent) << "A" << endl;
		PrintDeviceWithAllConnections(deviceParent_v[theMaxConnections.deviceId], myFullConnections, errorFile);
		errorFile << endl;
	}
}

void CCvcDb::FindVbgError(ogzstream & theErrorFile, voltage_t theParameter, CFullConnection & theConnections, instanceId_t theInstanceId, string theDisplayParameter) {
	theParameter += cvcParameters.cvcOvervoltageErrorThreshold;
	if ( ( theConnections.validMaxGate && theConnections.validMinBulk
				&& abs(theConnections.maxGateVoltage - theConnections.minBulkVoltage) > theParameter )
			|| ( theConnections.validMinGate && theConnections.validMaxBulk
				&& abs(theConnections.minGateVoltage - theConnections.maxBulkVoltage) > theParameter ) ) {
		PrintOverVoltageError(theErrorFile, theConnections, OVERVOLTAGE_VBG, "Overvoltage Error:Gate vs Bulk:" + theDisplayParameter, theInstanceId);
	} else if ( ! cvcParameters.cvcLeakOvervoltage ) {
		return ;
	} else if ( ( theConnections.validMaxGateLeak && theConnections.validMinBulkLeak
				&& abs(theConnections.maxGateLeakVoltage - theConnections.minBulkLeakVoltage) > theParameter )
			|| ( theConnections.validMinGateLeak && theConnections.validMaxBulkLeak
				&& abs(theConnections.minGateLeakVoltage - theConnections.maxBulkLeakVoltage) > theParameter )
			|| ( theConnections.validMaxGateLeak && ! theConnections.validMinBulkLeak
				&& abs(theConnections.maxGateLeakVoltage) > theParameter )
			|| ( theConnections.validMinGateLeak && ! theConnections.validMaxBulkLeak
				&& abs(theConnections.minGateLeakVoltage) > theParameter )
			|| ( ! theConnections.validMaxGateLeak && theConnections.validMinBulkLeak
				&& abs(theConnections.minBulkLeakVoltage) > theParameter )
			|| ( ! theConnections.validMinGate && theConnections.validMaxBulkLeak
				&& abs(theConnections.maxBulkLeakVoltage) > theParameter ) ) {
		PrintOverVoltageError(theErrorFile, theConnections, OVERVOLTAGE_VBG, "Overvoltage Error:Gate vs Bulk: (logic ok)" + theDisplayParameter, theInstanceId);
	}
}

void CCvcDb::FindVbsError(ogzstream & theErrorFile, voltage_t theParameter, CFullConnection & theConnections, instanceId_t theInstanceId, string theDisplayParameter) {
	theParameter += cvcParameters.cvcOvervoltageErrorThreshold;
	if ( ( theConnections.validMaxSource && theConnections.validMinBulk
				&& abs(theConnections.maxSourceVoltage - theConnections.minBulkVoltage) > theParameter )
			|| ( theConnections.validMinSource && theConnections.validMaxBulk
				&& abs(theConnections.minSourceVoltage - theConnections.maxBulkVoltage) > theParameter )
			|| ( theConnections.validMaxDrain && theConnections.validMinBulk
				&& abs(theConnections.maxDrainVoltage - theConnections.minBulkVoltage) > theParameter )
			|| ( theConnections.validMinDrain && theConnections.validMaxBulk
				&& abs(theConnections.minDrainVoltage - theConnections.maxBulkVoltage) > theParameter ) ) {
		PrintOverVoltageError(theErrorFile, theConnections, OVERVOLTAGE_VBS, "Overvoltage Error:Source/Drain vs Bulk:" + theDisplayParameter, theInstanceId);
	} else if ( ! cvcParameters.cvcLeakOvervoltage ) {
		return ;
	} else if ( ( theConnections.validMaxSourceLeak && theConnections.validMinBulkLeak
				&& abs(theConnections.maxSourceLeakVoltage - theConnections.minBulkLeakVoltage) > theParameter )
			|| ( theConnections.validMinSourceLeak && theConnections.validMaxBulkLeak
				&& abs(theConnections.minSourceLeakVoltage - theConnections.maxBulkLeakVoltage) > theParameter )
			|| ( theConnections.validMaxDrainLeak && theConnections.validMinBulkLeak
				&& abs(theConnections.maxDrainLeakVoltage - theConnections.minBulkLeakVoltage) > theParameter )
			|| ( theConnections.validMinDrainLeak && theConnections.validMaxBulkLeak
				&& abs(theConnections.minDrainLeakVoltage - theConnections.maxBulkLeakVoltage) > theParameter )
			|| ( theConnections.validMinBulkLeak && ! theConnections.validMaxSourceLeak && ! theConnections.validMaxDrainLeak
				&& abs(theConnections.minBulkLeakVoltage) > theParameter )
			|| ( theConnections.validMaxBulkLeak && ! theConnections.validMinSourceLeak && ! theConnections.validMinDrainLeak
				&& abs(theConnections.maxBulkLeakVoltage) > theParameter )
			|| ( theConnections.validMinSourceLeak && ! theConnections.validMaxBulkLeak
				&& abs(theConnections.minSourceLeakVoltage) > theParameter )
			|| ( theConnections.validMaxSourceLeak && ! theConnections.validMinBulkLeak
				&& abs(theConnections.maxSourceLeakVoltage) > theParameter )
			|| ( theConnections.validMinDrainLeak && ! theConnections.validMaxBulkLeak
				&& abs(theConnections.minDrainLeakVoltage) > theParameter )
			|| ( theConnections.validMaxDrainLeak && ! theConnections.validMinBulkLeak
				&& abs(theConnections.maxDrainLeakVoltage) > theParameter ) ) {
		PrintOverVoltageError(theErrorFile, theConnections, OVERVOLTAGE_VBS, "Overvoltage Error:Source/Drain vs Bulk: (logic ok)" + theDisplayParameter, theInstanceId);
	}
}

void CCvcDb::FindVdsError(ogzstream & theErrorFile, voltage_t theParameter, CFullConnection & theConnections, instanceId_t theInstanceId, string theDisplayParameter) {
	theParameter += cvcParameters.cvcOvervoltageErrorThreshold;
	if ( ( theConnections.validMinSource && theConnections.validMaxDrain
				&& abs(theConnections.minSourceVoltage - theConnections.maxDrainVoltage) > theParameter )
			|| ( theConnections.validMaxSource && theConnections.validMinDrain
				&& abs(theConnections.maxSourceVoltage - theConnections.minDrainVoltage) > theParameter ) ) {
		if ( theConnections.IsPumpCapacitor() &&
				abs(theConnections.minSourceVoltage - theConnections.minDrainVoltage) <= theParameter &&
				abs(theConnections.maxSourceVoltage - theConnections.maxDrainVoltage) <= theParameter ) {
			; // for pumping capacitors only check min-min/max-max differences
		} else {
			PrintOverVoltageError(theErrorFile, theConnections, OVERVOLTAGE_VDS, "Overvoltage Error:Source vs Drain:" + theDisplayParameter, theInstanceId);
		}
	} else if ( ! cvcParameters.cvcLeakOvervoltage ) {
		return ;
	} else if ( ( theConnections.validMinSourceLeak && theConnections.validMaxDrainLeak
				&& abs(theConnections.minSourceLeakVoltage - theConnections.maxDrainLeakVoltage) > theParameter )
			|| ( theConnections.validMaxSourceLeak && theConnections.validMinDrainLeak
				&& abs(theConnections.maxSourceLeakVoltage - theConnections.minDrainLeakVoltage) > theParameter )
			|| ( theConnections.validMinSourceLeak && ! theConnections.validMaxDrainLeak
				&& abs(theConnections.minSourceLeakVoltage) > theParameter )
			|| ( ! theConnections.validMinSourceLeak && theConnections.validMaxDrainLeak
				&& abs(theConnections.maxDrainLeakVoltage) > theParameter )
			|| ( theConnections.validMaxSourceLeak && ! theConnections.validMinDrainLeak
				&& abs(theConnections.maxSourceLeakVoltage) > theParameter )
			|| ( ! theConnections.validMaxSourceLeak && theConnections.validMinDrainLeak
				&& abs(theConnections.minDrainLeakVoltage) > theParameter )	) {
		PrintOverVoltageError(theErrorFile, theConnections, OVERVOLTAGE_VDS, "Overvoltage Error:Source vs Drain: (logic ok)" + theDisplayParameter, theInstanceId);
	}
}

void CCvcDb::FindVgsError(ogzstream & theErrorFile, voltage_t theParameter, CFullConnection & theConnections, instanceId_t theInstanceId, string theDisplayParameter) {
	theParameter += cvcParameters.cvcOvervoltageErrorThreshold;
	if ( ( theConnections.validMinGate && theConnections.validMaxSource
				&& abs(theConnections.minGateVoltage - theConnections.maxSourceVoltage) > theParameter )
			|| ( theConnections.validMaxGate && theConnections.validMinSource
				&& abs(theConnections.maxGateVoltage - theConnections.minSourceVoltage) > theParameter )
			|| ( theConnections.validMinGate && theConnections.validMaxDrain
				&& abs(theConnections.minGateVoltage - theConnections.maxDrainVoltage) > theParameter )
			|| ( theConnections.validMaxGate && theConnections.validMinDrain
				&& abs(theConnections.maxGateVoltage - theConnections.minDrainVoltage) > theParameter ) ) {
		PrintOverVoltageError(theErrorFile, theConnections, OVERVOLTAGE_VGS, "Overvoltage Error:Gate vs Source/Drain:" + theDisplayParameter, theInstanceId);
	} else if ( ! cvcParameters.cvcLeakOvervoltage ) {
		return ;
	} else if ( ( theConnections.validMinGateLeak && theConnections.validMaxSourceLeak
				&& abs(theConnections.minGateLeakVoltage - theConnections.maxSourceLeakVoltage) > theParameter )
			|| ( theConnections.validMaxGateLeak && theConnections.validMinSourceLeak
				&& abs(theConnections.maxGateLeakVoltage - theConnections.minSourceLeakVoltage) > theParameter )
			|| ( theConnections.validMinGateLeak && theConnections.validMaxDrainLeak
				&& abs(theConnections.minGateLeakVoltage - theConnections.maxDrainLeakVoltage) > theParameter )
			|| ( theConnections.validMaxGateLeak && theConnections.validMinDrainLeak
				&& abs(theConnections.maxGateLeakVoltage - theConnections.minDrainLeakVoltage) > theParameter )
			|| ( theConnections.validMinGateLeak && ! theConnections.validMaxSourceLeak && ! theConnections.validMaxDrainLeak
				&& abs(theConnections.minGateLeakVoltage) > theParameter )
			|| ( theConnections.validMaxGateLeak && ! theConnections.validMinSourceLeak && ! theConnections.validMinDrainLeak
				&& abs(theConnections.maxGateLeakVoltage) > theParameter )
			|| ( ! theConnections.validMinGateLeak && theConnections.validMaxSourceLeak
				&& abs(theConnections.maxSourceLeakVoltage) > theParameter )
			|| ( ! theConnections.validMinGateLeak && theConnections.validMaxDrainLeak
				&& abs(theConnections.maxDrainLeakVoltage) > theParameter )
			|| ( ! theConnections.validMaxGateLeak && theConnections.validMinSourceLeak
				&& abs(theConnections.minSourceLeakVoltage) > theParameter )
			|| ( ! theConnections.validMaxGateLeak && theConnections.validMinDrainLeak
				&& abs(theConnections.minDrainLeakVoltage) > theParameter )	) {
		PrintOverVoltageError(theErrorFile, theConnections, OVERVOLTAGE_VGS, "Overvoltage Error:Gate vs Source/Drain: (logic ok)" + theDisplayParameter, theInstanceId);
	}
}

void CCvcDb::FindModelError(ogzstream & theErrorFile, CModelCheck & theCheck, CFullConnection & theConnections, instanceId_t theInstanceId) {
	bool myError = false;
	if ( theCheck.parameter == "Vb" ) {
		if ( ! (theConnections.validMinBulk && theConnections.validMaxBulk) ) {
			myError = true;
		} else {
			if ( ! IsEmpty(theCheck.minExclusiveText) ) {
				myError |= theConnections.minBulkVoltage <= theCheck.minExclusiveVoltage;
			} else if ( ! IsEmpty(theCheck.minInclusiveText) ) {
				myError |= theConnections.minBulkVoltage < theCheck.minInclusiveVoltage;
			}
			if ( ! IsEmpty(theCheck.maxExclusiveText) ) {
				myError |= theConnections.maxBulkVoltage >= theCheck.maxExclusiveVoltage;
			} else if ( ! IsEmpty(theCheck.maxInclusiveText) ) {
				myError |= theConnections.maxBulkVoltage > theCheck.maxInclusiveVoltage;
			}
		}
	}
	if ( myError ) {
		PrintModelError(theErrorFile, theConnections, theCheck, theInstanceId);
	}
}

void CCvcDb::PrintOverVoltageError(ogzstream & theErrorFile, CFullConnection & theConnections, cvcError_t theErrorIndex, string theExplanation, instanceId_t theInstanceId) {
	if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(theConnections.deviceId, theErrorIndex) < cvcParameters.cvcCircuitErrorLimit ) {
		theErrorFile << theExplanation << endl;
		bool myLeakCheckFlag = ( theExplanation.find("logic ok") < string::npos );
		PrintDeviceWithAllConnections(theInstanceId, theConnections, theErrorFile, myLeakCheckFlag);
		theErrorFile << endl;
	}
}
 
void CCvcDb::PrintModelError(ogzstream & theErrorFile, CFullConnection & theConnections, CModelCheck & theCheck, instanceId_t theInstanceId) {
	if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(theConnections.deviceId, MODEL_CHECK) < cvcParameters.cvcCircuitErrorLimit ) {
		theErrorFile << "Model error: " << theCheck.check << endl;
		PrintDeviceWithAllConnections(theInstanceId, theConnections, theErrorFile, false);
		theErrorFile << endl;
	}
}

void CCvcDb::FindAllOverVoltageErrors() {
	CFullConnection myConnections;
	reportFile << "! Checking overvoltage errors" << endl << endl;
	string myVbgErrorFileName(tmpnam(NULL));
	ogzstream myVbgErrorFile(myVbgErrorFileName);
	myVbgErrorFile << "! Checking Vbg overvoltage errors" << endl << endl;
	string myVbsErrorFileName(tmpnam(NULL));
	ogzstream myVbsErrorFile(myVbsErrorFileName);
	myVbsErrorFile << "! Checking Vbs overvoltage errors" << endl << endl;
	string myVdsErrorFileName(tmpnam(NULL));
	ogzstream myVdsErrorFile(myVdsErrorFileName);
	myVdsErrorFile << "! Checking Vds overvoltage errors" << endl << endl;
	string myVgsErrorFileName(tmpnam(NULL));
	ogzstream myVgsErrorFile(myVgsErrorFileName);
	myVgsErrorFile << "! Checking Vgs overvoltage errors" << endl << endl;
	string myModelErrorFileName(tmpnam(NULL));
	ogzstream myModelErrorFile(myModelErrorFileName);
	myModelErrorFile << "! Checking Model errors" << endl << endl;

	for (CModelListMap::iterator keyModelListPair_pit = cvcParameters.cvcModelListMap.begin(); keyModelListPair_pit != cvcParameters.cvcModelListMap.end(); keyModelListPair_pit++) {
		for (CModelList::iterator model_pit = keyModelListPair_pit->second.begin(); model_pit != keyModelListPair_pit->second.end(); model_pit++) {
			if ( model_pit->maxVbg == UNKNOWN_VOLTAGE && model_pit->maxVbs == UNKNOWN_VOLTAGE
				&&  model_pit->maxVds == UNKNOWN_VOLTAGE && model_pit->maxVgs == UNKNOWN_VOLTAGE ) continue;
			CDevice * myDevice_p = model_pit->firstDevice_p;
			string myVbgDisplayParameter, myVbsDisplayParameter, myVdsDisplayParameter, myVgsDisplayParameter;
			if ( model_pit->maxVbg != UNKNOWN_VOLTAGE ) {
				myVbgDisplayParameter = " Vbg=" + PrintToleranceParameter(model_pit->maxVbgDefinition, model_pit->maxVbg, VOLTAGE_SCALE) + " " + model_pit->ConditionString();
			}
			if ( model_pit->maxVbs != UNKNOWN_VOLTAGE ) {
				myVbsDisplayParameter = " Vbs=" + PrintToleranceParameter(model_pit->maxVbsDefinition, model_pit->maxVbs, VOLTAGE_SCALE) + " " + model_pit->ConditionString();
			}
			if ( model_pit->maxVds != UNKNOWN_VOLTAGE ) {
				myVdsDisplayParameter = " Vds=" + PrintToleranceParameter(model_pit->maxVdsDefinition, model_pit->maxVds, VOLTAGE_SCALE) + " " + model_pit->ConditionString();
			}
			if ( model_pit->maxVgs != UNKNOWN_VOLTAGE ) {
				myVgsDisplayParameter = " Vgs=" + PrintToleranceParameter(model_pit->maxVgsDefinition, model_pit->maxVgs, VOLTAGE_SCALE) + " " + model_pit->ConditionString();
			}
			while (myDevice_p) {
				CCircuit * myParent_p = myDevice_p->parent_p;
				for (instanceId_t instance_it = 0; instance_it < myParent_p->instanceId_v.size(); instance_it++) {
					if  ( instancePtr_v[myParent_p->instanceId_v[instance_it]]->IsParallelInstance() ) continue;  // parallel/empty instances
					instanceId_t myInstanceId = myParent_p->instanceId_v[instance_it];
					CInstance * myInstance_p = instancePtr_v[myInstanceId];
					MapDeviceNets(myInstance_p, myDevice_p, myConnections);
					myConnections.SetMinMaxLeakVoltagesAndFlags(this);
					//string myErrorExplanation = "";
					if ( model_pit->maxVbg != UNKNOWN_VOLTAGE ) FindVbgError(myVbgErrorFile, model_pit->maxVbg, myConnections, myInstanceId, myVbgDisplayParameter);
					if ( model_pit->maxVbs != UNKNOWN_VOLTAGE ) FindVbsError(myVbsErrorFile, model_pit->maxVbs, myConnections, myInstanceId, myVbsDisplayParameter);
					if ( model_pit->maxVds != UNKNOWN_VOLTAGE ) FindVdsError(myVdsErrorFile, model_pit->maxVds, myConnections, myInstanceId, myVdsDisplayParameter);
					if ( model_pit->maxVgs != UNKNOWN_VOLTAGE ) FindVgsError(myVgsErrorFile, model_pit->maxVgs, myConnections, myInstanceId, myVgsDisplayParameter);
					for ( auto check_pit = model_pit->checkList.begin(); check_pit != model_pit->checkList.end(); check_pit++ ) {
						FindModelError(myModelErrorFile, *check_pit, myConnections, myInstanceId);
					}
				}
				myDevice_p = myDevice_p->nextDevice_p;
			}
		}
	}
	myVbgErrorFile.close();
	myVbsErrorFile.close();
	myVdsErrorFile.close();
	myVgsErrorFile.close();
	myModelErrorFile.close();
	AppendErrorFile(myVbgErrorFileName, "! Checking Vbg overvoltage errors", OVERVOLTAGE_VBG - OVERVOLTAGE_VBG);
	AppendErrorFile(myVbsErrorFileName, "! Checking Vbs overvoltage errors", OVERVOLTAGE_VBS - OVERVOLTAGE_VBG);
	AppendErrorFile(myVdsErrorFileName, "! Checking Vds overvoltage errors", OVERVOLTAGE_VDS - OVERVOLTAGE_VBG);
	AppendErrorFile(myVgsErrorFileName, "! Checking Vgs overvoltage errors", OVERVOLTAGE_VGS - OVERVOLTAGE_VBG);
	AppendErrorFile(myModelErrorFileName, "! Checking Model errors", MODEL_CHECK - OVERVOLTAGE_VBG);
}

void CCvcDb::AppendErrorFile(string theTempFileName, string theHeading, int theErrorSubIndex) {
	igzstream myTempFile(theTempFileName);
	errorFile << myTempFile.rdbuf();
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, theHeading, theErrorSubIndex);
	myTempFile.close();
	remove(theTempFileName.c_str());
}
 
void CCvcDb::FindNmosGateVsSourceErrors() {
	CFullConnection myConnections;
	reportFile << "! Checking nmos gate vs source errors: " << endl << endl;
	errorFile << "! Checking nmos gate vs source errors: " << endl << endl;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[device_it - myInstance_p->firstDeviceId];
		if ( ! IsNmos_(myDevice_p->model_p->type) ) continue;
		MapDeviceNets(myInstance_p, myDevice_p, myConnections);
		bool myVthFlag = false;
		bool myUnrelatedFlag = false;
		if ( ! myConnections.minGatePower_p ) continue;
		if ( myConnections.minGatePower_p->type[ANALOG_BIT] && ! cvcParameters.cvcAnalogGates ) continue;  // ignore analog gate errors
		if ( IsSCRCPower(myConnections.minGatePower_p) ) continue;  // ignore SCRC input (if not logically ok, should yield floating error)
		if ( myConnections.minGatePower_p->IsRelatedPower(myConnections.minSourcePower_p, netVoltagePtr_v, minNet_v, minNet_v, true, true)
				&& myConnections.minGatePower_p->IsRelatedPower(myConnections.minDrainPower_p, netVoltagePtr_v, minNet_v, minNet_v, true, true) ) {
			// if relatives (default), then checks are conditional
			voltage_t myGateSourceDifference = myConnections.minGateVoltage - min(myConnections.minSourceVoltage, myConnections.minSourceVoltage + myDevice_p->model_p->Vth);
			voltage_t myGateDrainDifference = myConnections.minGateVoltage - min(myConnections.minDrainVoltage, myConnections.minDrainVoltage + myDevice_p->model_p->Vth);
			voltage_t myMaxVoltageDifference = min(0, myDevice_p->model_p->Vth);
			if ( myConnections.CheckTerminalMinVoltages(GATE | SOURCE)
					&& myGateSourceDifference > myMaxVoltageDifference
					&& myConnections.gateId != myConnections.drainId ) {
				myMaxVoltageDifference = myGateSourceDifference;
			}
			if ( myConnections.CheckTerminalMinVoltages(GATE | DRAIN)
					&& myGateDrainDifference > myMaxVoltageDifference
					&& myConnections.gateId != myConnections.sourceId ) {
				myMaxVoltageDifference = myGateDrainDifference;
			}
			if ( myMaxVoltageDifference <= cvcParameters.cvcGateErrorThreshold
					|| ( cvcParameters.cvcMinVthGates && myMaxVoltageDifference < myDevice_p->model_p->Vth ) ) continue;  // no error
			// Skip gates that are always fully on
			if ( myConnections.CheckTerminalMaxVoltages(SOURCE) ) {
				if ( myConnections.CheckTerminalMaxVoltages(DRAIN) ) {
					if ( myConnections.minGateVoltage >= max(myConnections.maxSourceVoltage, myConnections.maxDrainVoltage) - cvcParameters.cvcGateErrorThreshold ) continue;
					if ( myConnections.sourceId == myConnections.drainId ) {  // capacitor check
						if ( IsPower_(netVoltagePtr_v[myConnections.gateId].full) && IsPower_(netVoltagePtr_v[myConnections.drainId].full) ) continue;  // ignore direct power capacitors
						if ( ! IsInputOrPower_(netVoltagePtr_v[myConnections.masterMinSourceNet.finalNetId].full)
								|| ! IsInputOrPower_(netVoltagePtr_v[myConnections.masterMinGateNet.finalNetId].full) ) continue;  // ignore capacitors connected to non-input/power nets
					}
				} else {
					if ( myConnections.minGateVoltage >= myConnections.maxSourceVoltage - cvcParameters.cvcGateErrorThreshold ) continue;
				}
			} else if ( myConnections.CheckTerminalMaxVoltages(DRAIN) ) {
				if ( myConnections.minGateVoltage >= myConnections.maxDrainVoltage - cvcParameters.cvcGateErrorThreshold ) continue;
			} else {
				continue;  // ignore devices with no max connections
			}
			myVthFlag = myConnections.minGatePower_p->type[MIN_CALCULATED_BIT]
				&& ( myConnections.minGateVoltage - myConnections.minSourceVoltage == myDevice_p->model_p->Vth
					|| myConnections.minGateVoltage - myConnections.minDrainVoltage == myDevice_p->model_p->Vth );
			if ( myVthFlag && ! cvcParameters.cvcVthGates ) continue;
		} else {
			myUnrelatedFlag = true;  // if not relatives, always an error
		}
		if ( IncrementDeviceError(myConnections.deviceId, NMOS_GATE_SOURCE) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
			if ( myUnrelatedFlag ) {
				errorFile << "Unrelated power error" << endl;
/*
			} else if ( myConnections.minGatePower_p->type[REFERENCE_BIT] ) {
				errorFile << "Gate reference signal" << endl;
*/
			} else if ( myVthFlag ) {
				errorFile << "Gate-source = Vth" << endl;
			}
			PrintDeviceWithAllConnections(deviceParent_v[device_it], myConnections, errorFile);
			errorFile << endl;
		}
	}
	CheckInverterIO(NMOS);
	CheckOppositeLogic(NMOS);
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking nmos gate vs source errors: ");
}
char GATE_LOGIC_CHECK[] = "GateVsSource logic check";

void CCvcDb::FindPmosGateVsSourceErrors() {
	CFullConnection myConnections;
	reportFile << "! Checking pmos gate vs source errors: " << endl << endl;
	errorFile << "! Checking pmos gate vs source errors: " << endl << endl;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[device_it - myInstance_p->firstDeviceId];
		if ( ! IsPmos_(myDevice_p->model_p->type) ) continue;
		MapDeviceNets(myInstance_p, myDevice_p, myConnections);
		bool myVthFlag = false;
		bool myUnrelatedFlag = false;
		if ( ! myConnections.maxGatePower_p ) continue;
		if ( myConnections.maxGatePower_p->type[ANALOG_BIT] && ! cvcParameters.cvcAnalogGates ) continue;  // ignore analog gate errors
		if ( IsSCRCPower(myConnections.maxGatePower_p) ) continue;  // ignore SCRC input (if not logically ok, should yield floating error)
		if ( myConnections.maxGatePower_p->IsRelatedPower(myConnections.maxSourcePower_p, netVoltagePtr_v, maxNet_v, maxNet_v, true, true)
				&& myConnections.maxGatePower_p->IsRelatedPower(myConnections.maxDrainPower_p, netVoltagePtr_v, maxNet_v, maxNet_v, true, true) ) {
			// if relatives (default), then checks are conditional
			voltage_t myGateSourceDifference = max(myConnections.maxSourceVoltage, myConnections.maxSourceVoltage + myDevice_p->model_p->Vth) - myConnections.maxGateVoltage;
			voltage_t myGateDrainDifference = max(myConnections.maxDrainVoltage, myConnections.maxDrainVoltage + myDevice_p->model_p->Vth) - myConnections.maxGateVoltage;
			voltage_t myMaxVoltageDifference = max(0, myDevice_p->model_p->Vth);
			if ( myConnections.CheckTerminalMaxVoltages(GATE | SOURCE) &&
					myGateSourceDifference > myMaxVoltageDifference  &&
					myConnections.gateId != myConnections.drainId ) {
				myMaxVoltageDifference = myGateSourceDifference;
			}
			if ( myConnections.CheckTerminalMaxVoltages(GATE | DRAIN) &&
					myGateDrainDifference > myMaxVoltageDifference  &&
					myConnections.gateId != myConnections.sourceId ) {
				myMaxVoltageDifference = myGateDrainDifference;
			}
			if ( myMaxVoltageDifference <= cvcParameters.cvcGateErrorThreshold
					|| ( cvcParameters.cvcMinVthGates && myMaxVoltageDifference < -myDevice_p->model_p->Vth ) ) continue;  // no error
			// Skip gates that are always fully on
			if ( myConnections.CheckTerminalMinVoltages(SOURCE) ) {
				if ( myConnections.CheckTerminalMinVoltages(DRAIN) ) {
					if ( myConnections.maxGateVoltage <= min(myConnections.minSourceVoltage, myConnections.minDrainVoltage) + cvcParameters.cvcGateErrorThreshold ) continue;
					if ( myConnections.sourceId == myConnections.drainId ) {  // capacitor check
						if ( IsPower_(netVoltagePtr_v[myConnections.gateId].full) && IsPower_(netVoltagePtr_v[myConnections.drainId].full) ) continue;  // ignore direct power capacitors
						if ( ! IsInputOrPower_(netVoltagePtr_v[myConnections.masterMinSourceNet.finalNetId].full)
								|| ! IsInputOrPower_(netVoltagePtr_v[myConnections.masterMinGateNet.finalNetId].full) ) continue;  // ignore capacitors connected to non-input/power nets
					}
				} else {
					if ( myConnections.maxGateVoltage <= myConnections.minSourceVoltage + cvcParameters.cvcGateErrorThreshold ) continue;
				}
			} else if ( myConnections.CheckTerminalMinVoltages(DRAIN) ) {
				if ( myConnections.maxGateVoltage <= myConnections.minDrainVoltage + cvcParameters.cvcGateErrorThreshold ) continue;
			} else {
				continue;  // ignore devices with no min connections
			}
			myVthFlag= myConnections.maxGatePower_p->type[MAX_CALCULATED_BIT]
				&& ( myConnections.maxGateVoltage - myConnections.maxSourceVoltage == myDevice_p->model_p->Vth
					|| myConnections.maxGateVoltage - myConnections.maxDrainVoltage == myDevice_p->model_p->Vth );
			if ( myVthFlag && ! cvcParameters.cvcVthGates ) continue;
		} else {
			myUnrelatedFlag = true; // if not relatives, always an error
		}
		if ( myConnections.maxGatePower_p->type[MAX_CALCULATED_BIT] && cvcParameters.cvcLogicDiodes && ! netVoltagePtr_v[myConnections.gateId].full ) {
			// If the source is a calculated value and the CVC_LOGIC_DIODE switch is on and there is no current definition,
			// there is no error if the gate logic value is low (WARNING: set to 0V is just a kludge)
			// check expected value later
			debugFile << "EXPECT low for GateVsSource: " << NetName(myConnections.gateId) << " at " << DeviceName(myConnections.deviceId, PRINT_CIRCUIT_ON) << endl;
			CPower * myPower_p = new CPower(myConnections.gateId);
			myPower_p->definition = CPower::powerDefinitionText.SetTextAddress(GATE_LOGIC_CHECK);
			myPower_p->extraData = new CExtraPowerData;
			myPower_p->minVoltage = UNKNOWN_VOLTAGE;
			myPower_p->simVoltage = UNKNOWN_VOLTAGE;
			myPower_p->maxVoltage = UNKNOWN_VOLTAGE;
			myPower_p->type[INPUT_BIT] = false;
			myPower_p->extraData->expectedSim = "0";
			cvcParameters.cvcExpectedLevelPtrList.push_back(myPower_p);
			continue;  // Don't flag error here
		}
		if ( IncrementDeviceError(myConnections.deviceId, PMOS_GATE_SOURCE) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
			if ( myUnrelatedFlag ) {
				errorFile << "Unrelated power error" << endl;
/*
			} else if ( myConnections.maxGatePower_p->type[REFERENCE_BIT] ) {
				errorFile << "Gate reference signal" << endl;
*/
			} else if ( myVthFlag ) {
				errorFile << "Gate-source = Vth" << endl;
			}
			PrintDeviceWithAllConnections(deviceParent_v[device_it], myConnections, errorFile);
			errorFile << endl;
		}
	}
	CheckInverterIO(PMOS);
	CheckOppositeLogic(PMOS);
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking pmos gate vs source errors: ");
}

void CCvcDb::FindNmosSourceVsBulkErrors() {
	CFullConnection myConnections;
	reportFile << "! Checking nmos source/drain vs bias errors: " << endl << endl;
	errorFile << "! Checking nmos source/drain vs bias errors: " << endl << endl;
	unordered_set<netId_t> myProblemNets;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[device_it - myInstance_p->firstDeviceId];
		if ( ! IsNmos_(myDevice_p->model_p->type) ) continue;
		MapDeviceNets(myInstance_p, myDevice_p, myConnections);
		if ( myConnections.sourceId == myConnections.drainId && myConnections.sourceId == myConnections.bulkId ) continue;  // ignore drain = source = bulk
		bool myErrorFlag = false;
		bool myUnrelatedFlag = false;
		bool mySourceError = false;
		bool myDrainError = false;
		if ( ! myConnections.minBulkPower_p
				|| (myConnections.minBulkPower_p->IsRelatedPower(myConnections.minSourcePower_p, netVoltagePtr_v, minNet_v, minNet_v, true, true)
					&& myConnections.minBulkPower_p->IsRelatedPower(myConnections.minDrainPower_p, netVoltagePtr_v, minNet_v, minNet_v, true, true)) ) {
			// if relatives (default), then checks are conditional
			if ( (myConnections.maxBulkVoltage == myConnections.minSourceVoltage || myConnections.minSourceVoltage == UNKNOWN_VOLTAGE)
					&& (myConnections.maxBulkVoltage == myConnections.minDrainVoltage || myConnections.minDrainVoltage == UNKNOWN_VOLTAGE) ) continue;
				// no error if max bulk = min source = min drain
			if ( myConnections.minBulkPower_p && myConnections.minBulkPower_p->type[HIZ_BIT] ) {
				if ( myConnections.sourceId == myConnections.bulkId && myConnections.drainId == myConnections.bulkId ) continue;
				if ( myConnections.minSourcePower_p && ! myConnections.minBulkPower_p->IsRelatedPower(myConnections.minSourcePower_p, netVoltagePtr_v, minNet_v, minNet_v, false) ) {
					myErrorFlag = true;
				} else if ( myConnections.simSourcePower_p && IsKnownVoltage_(myConnections.simSourcePower_p->simVoltage)
						&& ! myConnections.minBulkPower_p->IsRelatedPower(myConnections.simSourcePower_p, netVoltagePtr_v, minNet_v, simNet_v, false)
						&& myConnections.simSourceVoltage < myConnections.maxBulkVoltage ) {
					myErrorFlag = true;
				} else if ( myConnections.maxSourcePower_p && ! myConnections.minBulkPower_p->IsRelatedPower(myConnections.maxSourcePower_p, netVoltagePtr_v, minNet_v, maxNet_v, false) ) {
					myErrorFlag = true;
				} else if ( myConnections.minDrainPower_p && ! myConnections.minBulkPower_p->IsRelatedPower(myConnections.minDrainPower_p, netVoltagePtr_v, minNet_v, minNet_v, false) ) {
					myErrorFlag = true;
				} else if ( myConnections.simDrainPower_p && IsKnownVoltage_(myConnections.simDrainPower_p->simVoltage)
						&& ! myConnections.minBulkPower_p->IsRelatedPower(myConnections.simDrainPower_p, netVoltagePtr_v, minNet_v, simNet_v, false)
						&& myConnections.simDrainVoltage < myConnections.maxBulkVoltage ) {
					myErrorFlag = true;
				} else if ( myConnections.maxDrainPower_p && ! myConnections.minBulkPower_p->IsRelatedPower(myConnections.maxDrainPower_p, netVoltagePtr_v, minNet_v, maxNet_v, false) ) {
					myErrorFlag = true;
				}
			} else if ( myConnections.CheckTerminalMinVoltages(BULK) == false ) {
				if ( myConnections.minSourcePower_p || myConnections.simSourcePower_p || myConnections.maxSourcePower_p ||
						myConnections.minDrainPower_p || myConnections.simDrainPower_p || myConnections.maxDrainPower_p ) { // has some connection (all connections open -> no error)
					myErrorFlag = true;
				}
			} else if ( myConnections.CheckTerminalMinVoltages(BULK | SOURCE) &&
					( myConnections.minBulkVoltage - myConnections.minSourceVoltage > cvcParameters.cvcBiasErrorThreshold ||
						( myConnections.minSourceVoltage == myConnections.minBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.masterMinSourceNet.finalResistance < myConnections.masterMinBulkNet.finalResistance) ) ) {
				mySourceError = true;
			} else if (	myConnections.CheckTerminalMinVoltages(BULK | DRAIN) &&
					( myConnections.minBulkVoltage - myConnections.minDrainVoltage > cvcParameters.cvcBiasErrorThreshold ||
						( myConnections.minDrainVoltage == myConnections.minBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.masterMinDrainNet.finalResistance < myConnections.masterMinBulkNet.finalResistance) ) ) {
				myDrainError = true;
			} else if (	myConnections.CheckTerminalSimVoltages(BULK | SOURCE) &&
					( myConnections.simBulkVoltage - myConnections.simSourceVoltage > cvcParameters.cvcBiasErrorThreshold ||
						( myConnections.simSourceVoltage == myConnections.simBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.masterSimSourceNet.finalResistance < myConnections.masterSimBulkNet.finalResistance) ) ) {
				myErrorFlag = true;
			} else if (	myConnections.CheckTerminalSimVoltages(BULK | DRAIN) &&
					( myConnections.simBulkVoltage - myConnections.simDrainVoltage > cvcParameters.cvcBiasErrorThreshold ||
						( myConnections.simDrainVoltage == myConnections.simBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.masterSimDrainNet.finalResistance < myConnections.masterSimBulkNet.finalResistance) ) ) {
				myErrorFlag = true;
			} else if (	myConnections.CheckTerminalMaxVoltages(BULK | SOURCE) &&
					( ( myConnections.maxBulkVoltage - myConnections.maxSourceVoltage > cvcParameters.cvcBiasErrorThreshold &&
							myConnections.maxSourcePower_p->defaultMaxNet != myConnections.bulkId ) ||
						( myConnections.maxSourceVoltage == myConnections.maxBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.maxSourceVoltage != myConnections.minBulkVoltage && // no leak path
	//						myConnections.masterMinBulkNet.finalNetId != myConnections.masterMaxBulkNet.finalNetId &&
							myConnections.masterMaxSourceNet.finalResistance > myConnections.masterMaxBulkNet.finalResistance &&
							! myConnections.minBulkPower_p->type[HIZ_BIT] &&
							! PathContains(maxNet_v, myConnections.sourceId, myConnections.bulkId)) ) ) { // resistance check backwards in NMOS max (ignore connections through self)
				myErrorFlag = true;
	// Added 20140523
			} else if (	myConnections.CheckTerminalMaxVoltages(BULK | DRAIN) &&
					( ( myConnections.maxBulkVoltage - myConnections.maxDrainVoltage > cvcParameters.cvcBiasErrorThreshold &&
							myConnections.maxDrainPower_p->defaultMaxNet != myConnections.bulkId ) ||
						( myConnections.maxDrainVoltage == myConnections.maxBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.maxDrainVoltage != myConnections.minBulkVoltage && // no leak path
	//						myConnections.masterMinBulkNet.finalNetId != myConnections.masterMaxBulkNet.finalNetId &&
							myConnections.masterMaxDrainNet.finalResistance > myConnections.masterMaxBulkNet.finalResistance &&
							! myConnections.minBulkPower_p->type[HIZ_BIT] &&
							! PathContains(maxNet_v, myConnections.drainId, myConnections.bulkId)) ) ) { // resistance check backwards in NMOS max (ignore connections through self)
				myErrorFlag = true;
			}
		} else {
			myErrorFlag = true; // if not relatives, always an error
			myUnrelatedFlag = true;
		}
		if ( gSetup_cvc ) {
			if ( myDrainError ) {
				myProblemNets.insert(myConnections.drainId);
			} else if ( mySourceError ) {
				myProblemNets.insert(myConnections.sourceId);
			}
		} else if ( myErrorFlag || myDrainError || mySourceError ) {
			if ( IncrementDeviceError(myConnections.deviceId, NMOS_SOURCE_BULK) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
				if ( myUnrelatedFlag ) {
					errorFile << "Unrelated power error" << endl;
				}
				PrintDeviceWithAllConnections(deviceParent_v[device_it], myConnections, errorFile);
				errorFile << endl;
			}
		}
	}
	if ( gSetup_cvc ) {
		reportFile << endl << "CVC SETUP: nmos bias problems" << endl << endl;
		unordered_set<netId_t> myPrintedNets;
		unordered_set<netId_t> myParentNets;
		for ( auto net_pit = myProblemNets.begin(); net_pit != myProblemNets.end(); net_pit++ ) {
			netId_t myNextNet = *net_pit;
			while(myNextNet != minNet_v[myNextNet].nextNetId) {
				myNextNet = minNet_v[myNextNet].nextNetId;
				myParentNets.insert(myNextNet);
			}
		}
		for ( auto net_pit = myProblemNets.begin(); net_pit != myProblemNets.end(); net_pit++ ) {
			if ( myParentNets.count(*net_pit) ) continue;
			netId_t traceNet_it = *net_pit;
			reportFile << endl;
			PrintNetWithModelCounts(traceNet_it, SOURCE | DRAIN);
			while ( traceNet_it != minNet_v[traceNet_it].nextNetId ) {
				traceNet_it = minNet_v[traceNet_it].nextNetId;
				if ( traceNet_it == minNet_v[traceNet_it].nextNetId ) {
					reportFile << NetName(traceNet_it, PRINT_CIRCUIT_ON) << endl;
				} else {
					PrintNetWithModelCounts(traceNet_it, SOURCE | DRAIN);
					if ( myPrintedNets.count(traceNet_it) ) {
						reportFile << "**********" << endl;
						break;
					}
					myPrintedNets.insert(traceNet_it);
				}
			}
		}
	} else {
		cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking nmos source/drain vs bias errors: ");
	}
}

void CCvcDb::FindPmosSourceVsBulkErrors() {
	CFullConnection myConnections;
	reportFile << "! Checking pmos source/drain vs bias errors: " << endl << endl;
	errorFile << "! Checking pmos source/drain vs bias errors: " << endl << endl;
	unordered_set<netId_t> myProblemNets;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[device_it - myInstance_p->firstDeviceId];
		if ( ! IsPmos_(myDevice_p->model_p->type) ) continue;
		MapDeviceNets(myInstance_p, myDevice_p, myConnections);
		if ( myConnections.sourceId == myConnections.drainId && myConnections.sourceId == myConnections.bulkId ) continue;  // ignore drain = source = bulk
		bool myErrorFlag = false;
		bool myUnrelatedFlag = false;
		bool mySourceError = false;
		bool myDrainError = false;
		if ( ! myConnections.maxBulkPower_p
				|| (myConnections.maxBulkPower_p->IsRelatedPower(myConnections.maxSourcePower_p, netVoltagePtr_v, maxNet_v, maxNet_v, true, true)
					&& myConnections.maxBulkPower_p->IsRelatedPower(myConnections.maxDrainPower_p, netVoltagePtr_v, maxNet_v, maxNet_v, true, true)) ) {
			// if relatives (default), then checks are conditional
			if ( (myConnections.minBulkVoltage == myConnections.maxSourceVoltage || myConnections.maxSourceVoltage == UNKNOWN_VOLTAGE)
					&& (myConnections.minBulkVoltage == myConnections.maxDrainVoltage || myConnections.maxDrainVoltage == UNKNOWN_VOLTAGE) ) continue;
				 // no error if min bulk = max source = max drain
			if ( myConnections.maxBulkPower_p && myConnections.maxBulkPower_p->type[HIZ_BIT] ) {
				if ( myConnections.minSourcePower_p && ! myConnections.maxBulkPower_p->IsRelatedPower(myConnections.minSourcePower_p, netVoltagePtr_v, maxNet_v, minNet_v, false) ) {
					myErrorFlag = true;
				} else if ( myConnections.simSourcePower_p && IsKnownVoltage_(myConnections.simSourcePower_p->simVoltage)
						&& ! myConnections.maxBulkPower_p->IsRelatedPower(myConnections.simSourcePower_p, netVoltagePtr_v, maxNet_v, simNet_v, false)
						&& myConnections.simSourceVoltage > myConnections.minBulkVoltage ) {
					myErrorFlag = true;
				} else if ( myConnections.maxSourcePower_p && ! myConnections.maxBulkPower_p->IsRelatedPower(myConnections.maxSourcePower_p, netVoltagePtr_v, maxNet_v, maxNet_v, false) ) {
					myErrorFlag = true;
				} else if ( myConnections.minDrainPower_p && ! myConnections.maxBulkPower_p->IsRelatedPower(myConnections.minDrainPower_p, netVoltagePtr_v, maxNet_v, minNet_v, false) ) {
					myErrorFlag = true;
				} else if ( myConnections.simDrainPower_p && IsKnownVoltage_(myConnections.simDrainPower_p->simVoltage)
						&& ! myConnections.maxBulkPower_p->IsRelatedPower(myConnections.simDrainPower_p, netVoltagePtr_v, maxNet_v, simNet_v, false)
						&& myConnections.simDrainVoltage > myConnections.minBulkVoltage ) {
					myErrorFlag = true;
				} else if ( myConnections.maxDrainPower_p && ! myConnections.maxBulkPower_p->IsRelatedPower(myConnections.maxDrainPower_p, netVoltagePtr_v, maxNet_v, maxNet_v, false) ) {
					myErrorFlag = true;
				}
			} else if ( myConnections.CheckTerminalMaxVoltages(BULK) == false ) {
				if ( myConnections.minSourcePower_p || myConnections.simSourcePower_p || myConnections.maxSourcePower_p ||
						myConnections.minDrainPower_p || myConnections.simDrainPower_p || myConnections.maxDrainPower_p ) { // has some connection (all connections open -> no error)
					myErrorFlag = true;
				}
			} else if (	myConnections.CheckTerminalMinVoltages(BULK | SOURCE) &&
					( ( myConnections.minSourceVoltage - myConnections.minBulkVoltage > cvcParameters.cvcBiasErrorThreshold &&
							myConnections.minSourcePower_p->defaultMinNet != myConnections.bulkId ) ||
						( myConnections.minSourceVoltage == myConnections.minBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
	//						myConnections.masterMinBulkNet.finalNetId != myConnections.masterMaxBulkNet.finalNetId &&
							myConnections.minSourceVoltage != myConnections.maxBulkVoltage && // no leak path
							myConnections.masterMinSourceNet.finalResistance > myConnections.masterMinBulkNet.finalResistance &&
							! myConnections.maxBulkPower_p->type[HIZ_BIT] &&
							! PathContains(minNet_v, myConnections.sourceId, myConnections.bulkId)) ) ) { // resistance check backwards in PMOS min (ignore connections through self)
				myErrorFlag = true;
			} else if (	myConnections.CheckTerminalMinVoltages(BULK | DRAIN) &&
					( ( myConnections.minDrainVoltage - myConnections.minBulkVoltage > cvcParameters.cvcBiasErrorThreshold &&
							myConnections.minDrainPower_p->defaultMinNet != myConnections.bulkId ) ||
						( myConnections.minDrainVoltage == myConnections.minBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
	//						myConnections.masterMinBulkNet.finalNetId != myConnections.masterMaxBulkNet.finalNetId &&
							myConnections.minDrainVoltage != myConnections.maxBulkVoltage && // no leak path
							myConnections.masterMinDrainNet.finalResistance > myConnections.masterMinBulkNet.finalResistance &&
							! myConnections.maxBulkPower_p->type[HIZ_BIT] &&
							! PathContains(minNet_v, myConnections.drainId, myConnections.bulkId)) ) ) { // resistance check backwards in PMOS min (ignore connections through self)
				myErrorFlag = true;
			} else if (	myConnections.CheckTerminalSimVoltages(BULK | SOURCE) &&
					( myConnections.simSourceVoltage - myConnections.simBulkVoltage > cvcParameters.cvcBiasErrorThreshold ||
						( myConnections.simSourceVoltage == myConnections.simBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.masterSimSourceNet.finalResistance < myConnections.masterSimBulkNet.finalResistance) ) ) {
				myErrorFlag = true;
			} else if (	myConnections.CheckTerminalSimVoltages(BULK | DRAIN) &&
					( myConnections.simDrainVoltage - myConnections.simBulkVoltage > cvcParameters.cvcBiasErrorThreshold ||
						( myConnections.simDrainVoltage == myConnections.simBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.masterSimDrainNet.finalResistance < myConnections.masterSimBulkNet.finalResistance) ) ) {
				myErrorFlag = true;
			} else if (	myConnections.CheckTerminalMaxVoltages(BULK | SOURCE) &&
					( myConnections.maxSourceVoltage - myConnections.maxBulkVoltage > cvcParameters.cvcBiasErrorThreshold ||
						( myConnections.maxSourceVoltage == myConnections.maxBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.masterMaxSourceNet.finalResistance < myConnections.masterMaxBulkNet.finalResistance) ) ) {
				mySourceError = true;
			} else if (	myConnections.CheckTerminalMaxVoltages(BULK | DRAIN) &&
					( myConnections.maxDrainVoltage - myConnections.maxBulkVoltage > cvcParameters.cvcBiasErrorThreshold ||
						( myConnections.maxDrainVoltage == myConnections.maxBulkVoltage &&
							cvcParameters.cvcBiasErrorThreshold == 0 &&
							myConnections.masterMaxDrainNet.finalResistance < myConnections.masterMaxBulkNet.finalResistance) ) ) {
				myDrainError = true;
			}
		} else {
			myUnrelatedFlag = true;
			myErrorFlag = true; // if not relatives, always an error
		}
		if ( gSetup_cvc ) {
			if ( myDrainError ) {
				myProblemNets.insert(myConnections.drainId);
			} else if ( mySourceError ) {
				myProblemNets.insert(myConnections.sourceId);
			}
		} else if ( myErrorFlag || myDrainError || mySourceError ) {
			if ( IncrementDeviceError(myConnections.deviceId, PMOS_SOURCE_BULK) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
				if ( myUnrelatedFlag ) {
					errorFile << "Unrelated power error" << endl;
				}
				PrintDeviceWithAllConnections(deviceParent_v[device_it], myConnections, errorFile);
				errorFile << endl;
			}
		}
	}
	if ( gSetup_cvc ) {
		reportFile << endl << "CVC SETUP: pmos bias problems" << endl << endl;
		unordered_set<netId_t> myPrintedNets;
		unordered_set<netId_t> myParentNets;
		for ( auto net_pit = myProblemNets.begin(); net_pit != myProblemNets.end(); net_pit++ ) {
			netId_t myNextNet = *net_pit;
			while(myNextNet != minNet_v[myNextNet].nextNetId) {
				myNextNet = minNet_v[myNextNet].nextNetId;
				myParentNets.insert(myNextNet);
			}
		}
		for ( auto net_pit = myProblemNets.begin(); net_pit != myProblemNets.end(); net_pit++ ) {
			if ( myParentNets.count(*net_pit) ) continue;
			netId_t traceNet_it = *net_pit;
			reportFile << endl;
			PrintNetWithModelCounts(traceNet_it, SOURCE | DRAIN);
			while ( traceNet_it != minNet_v[traceNet_it].nextNetId ) {
				traceNet_it = minNet_v[traceNet_it].nextNetId;
				if ( traceNet_it == minNet_v[traceNet_it].nextNetId ) {
					reportFile << NetName(traceNet_it, PRINT_CIRCUIT_ON) << endl;
				} else {
					PrintNetWithModelCounts(traceNet_it, SOURCE | DRAIN);
					if ( myPrintedNets.count(traceNet_it) ) {
						reportFile << "**********" << endl;
						break;
					}
					myPrintedNets.insert(traceNet_it);
				}
			}
		}
	} else {
		cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking pmos source/drain vs bias errors: ");
	}
}

void CCvcDb::FindForwardBiasDiodes() {
	CFullConnection myConnections;
	CFullConnection myDiodeConnections;
	reportFile << "! Checking forward bias diode errors: " << endl << endl;
	errorFile << "! Checking forward bias diode errors: " << endl << endl;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[device_it - myInstance_p->firstDeviceId];
//		if ( myDevice_p->model_p->type != DIODE ) continue;
		if ( IsNmos_(deviceType_v[device_it]) || IsPmos_(deviceType_v[device_it]) ) continue;
		if ( myDevice_p->model_p->diodeList.empty() ) continue;
		MapDeviceNets(myInstance_p, myDevice_p, myConnections);
		bool myErrorFlag = false;
		for ( auto diode_pit = myDevice_p->model_p->diodeList.begin(); diode_pit != myDevice_p->model_p->diodeList.end(); diode_pit++ ) {
			if ( myErrorFlag ) continue;  // only one error per device
			SetDiodeConnections((*diode_pit), myConnections, myDiodeConnections);  // switch connections for diodes
			if ( GetEquivalentNet(myDiodeConnections.originalSourceId) == GetEquivalentNet(myDiodeConnections.originalDrainId) ) continue;
			bool myUnrelatedFlag = false;
			if ( myDiodeConnections.CheckTerminalMinMaxVoltages(SOURCE | DRAIN) ) {
				voltage_t mySourceVoltage = UNKNOWN_VOLTAGE, myDrainVoltage = UNKNOWN_VOLTAGE;
				if ( myDiodeConnections.minSourcePower_p == myDiodeConnections.minDrainPower_p ) {
					if ( myDiodeConnections.maxSourceVoltage == myDiodeConnections.maxDrainVoltage
							&& myDiodeConnections.minSourceVoltage == myDiodeConnections.maxDrainVoltage ) continue;
					if ( myDiodeConnections.masterMinSourceNet.finalResistance < myDiodeConnections.masterMinDrainNet.finalResistance ) {
						mySourceVoltage = myDiodeConnections.minSourceVoltage;
					} else if ( myDiodeConnections.masterMinSourceNet.finalResistance > myDiodeConnections.masterMinDrainNet.finalResistance ) {
						myDrainVoltage = myDiodeConnections.minDrainVoltage;
					}
				}
				if ( myDiodeConnections.maxSourcePower_p == myDiodeConnections.maxDrainPower_p ) {
					if ( myDiodeConnections.masterMaxDrainNet.finalResistance < myDiodeConnections.masterMaxSourceNet.finalResistance ) {
						if (myDrainVoltage == UNKNOWN_VOLTAGE) {
							myDrainVoltage = myDiodeConnections.maxDrainVoltage; // min drain overrides
						} else if ( PathCrosses(maxNet_v, myDiodeConnections.sourceId, minNet_v, myDiodeConnections.drainId) ) {
							continue; // no error if anode to power crosses cathode to ground path
						} else {
							logFile << "INFO: unexpected diode " << DeviceName(device_it, PRINT_CIRCUIT_ON) << endl << endl;
							PrintVirtualNet<CVirtualNetVector>(maxNet_v, myDiodeConnections.sourceId, "Max anode path", logFile);
							PrintVirtualNet<CVirtualNetVector>(minNet_v, myDiodeConnections.sourceId, "Min anode path", logFile);
							PrintVirtualNet<CVirtualNetVector>(maxNet_v, myDiodeConnections.drainId, "Max cathode path", logFile);
							PrintVirtualNet<CVirtualNetVector>(minNet_v, myDiodeConnections.drainId, "Min cathode path", logFile);
						}
					} else {
						if ( PathCrosses(minNet_v, myDiodeConnections.drainId, maxNet_v, myDiodeConnections.sourceId) ) {
							continue; // no error if anode to power crosses cathode to ground path
						} else if ( mySourceVoltage != UNKNOWN_VOLTAGE ) {
							logFile << "INFO: unexpected diode " << DeviceName(device_it, PRINT_CIRCUIT_ON) << endl << endl;
							PrintVirtualNet<CVirtualNetVector>(maxNet_v, myDiodeConnections.sourceId, "Max anode path", logFile);
							PrintVirtualNet<CVirtualNetVector>(minNet_v, myDiodeConnections.sourceId, "Min anode path", logFile);
							PrintVirtualNet<CVirtualNetVector>(maxNet_v, myDiodeConnections.drainId, "Max cathode path", logFile);
							PrintVirtualNet<CVirtualNetVector>(minNet_v, myDiodeConnections.drainId, "Min cathode path", logFile);
						}
						mySourceVoltage = myDiodeConnections.maxSourceVoltage; // max source overrides
					}
				}
				if ( mySourceVoltage == UNKNOWN_VOLTAGE ) {
					mySourceVoltage = myDiodeConnections.maxSourceVoltage;
				}
				if ( myDrainVoltage == UNKNOWN_VOLTAGE ) {
					myDrainVoltage = myDiodeConnections.minDrainVoltage;
				}
				if ( mySourceVoltage - myDrainVoltage > cvcParameters.cvcForwardErrorThreshold ) {
					if ( myDiodeConnections.maxSourcePower_p->type[HIZ_BIT] || myDiodeConnections.minDrainPower_p->type[HIZ_BIT] ) {
						// the following are errors for cutoff power. unknown annode, unknown cathode, source > max drain, max source not related to min drain
						if ( mySourceVoltage == UNKNOWN_VOLTAGE || myDiodeConnections.maxDrainVoltage == UNKNOWN_VOLTAGE || mySourceVoltage > myDiodeConnections.maxDrainVoltage ) {
								myErrorFlag = true;
						} else if ( ! myDiodeConnections.maxSourcePower_p->IsRelatedPower(myDiodeConnections.minDrainPower_p, netVoltagePtr_v, maxNet_v, minNet_v, true)) {
								myErrorFlag = true;
						}
					} else {
						myErrorFlag = true;
					}
				} else if ( ! myDiodeConnections.minSourcePower_p->IsRelatedPower(myDiodeConnections.minDrainPower_p, netVoltagePtr_v, minNet_v, minNet_v, true, true)
						|| ! myDiodeConnections.maxSourcePower_p->IsRelatedPower(myDiodeConnections.maxDrainPower_p, netVoltagePtr_v, maxNet_v, maxNet_v, true, true) ) {
					myErrorFlag = true;
					myUnrelatedFlag = true;
				}
			} else if ( myDiodeConnections.CheckTerminalMinMaxVoltages(SOURCE) && ! myDiodeConnections.CheckTerminalMinMaxVoltages(DRAIN) ) {
				if ( myDiodeConnections.maxSourceVoltage > 0 && ! myDiodeConnections.maxSourcePower_p->type[HIZ_BIT] ) {
					myErrorFlag = true;
				}
			} else if ( ! myDiodeConnections.CheckTerminalMinMaxVoltages(SOURCE) && myDiodeConnections.CheckTerminalMinMaxVoltages(DRAIN) ) {
				if ( myDiodeConnections.minDrainVoltage <= 0 && ! myDiodeConnections.minDrainPower_p->type[HIZ_BIT] ) {
					myErrorFlag = true;
				}
			}
			if ( myErrorFlag ) {
				if ( IncrementDeviceError(myConnections.deviceId, FORWARD_DIODE) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
					if ( myUnrelatedFlag ) {
						errorFile << "Unrelated power error" << endl;
					}
					PrintDeviceWithAllConnections(deviceParent_v[device_it], myConnections, errorFile);
					errorFile << endl;
				}
			}
		}
	}
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking forward bias diode errors: ");
}

void CCvcDb::FindNmosPossibleLeakErrors() {
	CFullConnection myConnections;
	reportFile << "! Checking nmos possible leak errors: " << endl << endl;
	errorFile << "! Checking nmos possible leak errors: " << endl << endl;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[device_it - myInstance_p->firstDeviceId];
		if ( ! IsNmos_(myDevice_p->model_p->type) ) continue;
		bool myErrorFlag = false;
		MapDeviceNets(myInstance_p, myDevice_p, myConnections);
		if ( IsKnownVoltage_(myConnections.simGateVoltage) ) continue;
		if ( myConnections.CheckTerminalMinVoltages(SOURCE | DRAIN) == true ) {
			voltage_t myMaxGateVoltage = MaxVoltage(myConnections.gateId);
			if ( myMaxGateVoltage == UNKNOWN_VOLTAGE ) {
				myMaxGateVoltage = MaxLeakVoltage(myConnections.gateId);
			}
			if ( myMaxGateVoltage != UNKNOWN_VOLTAGE
					&& myMaxGateVoltage <= myConnections.minSourceVoltage + myConnections.device_p->model_p->Vth
					&& myMaxGateVoltage <= myConnections.minDrainVoltage + myConnections.device_p->model_p->Vth ) {
				continue;  // always off
			}
		}
		if ( myConnections.CheckTerminalSimVoltages(SOURCE | DRAIN) == true ) {
			if ( myConnections.simSourceVoltage == myConnections.simDrainVoltage ) continue;
			if ( abs(myConnections.simSourceVoltage - myConnections.simDrainVoltage) <= cvcParameters.cvcLeakErrorThreshold ) continue;
			if ( myConnections.simSourcePower_p->type[HIZ_BIT] &&
					! myConnections.simSourcePower_p->IsRelatedPower(myConnections.simDrainPower_p, netVoltagePtr_v, simNet_v, simNet_v, false) ) {
				;  // possible leaks to cutoff power
			} else if ( myConnections.simDrainPower_p->type[HIZ_BIT] &&
					! myConnections.simDrainPower_p->IsRelatedPower(myConnections.simSourcePower_p, netVoltagePtr_v, simNet_v, simNet_v, false) ) {
				;  // possible leaks to cutoff power
			} else if ( myConnections.simSourcePower_p->IsInternalOverride() || myConnections.simDrainPower_p->IsInternalOverride() ) {
				continue;
			} else if ( myConnections.simSourcePower_p->type[SIM_CALCULATED_BIT] || myConnections.simDrainPower_p->type[SIM_CALCULATED_BIT] ) {
				if ( myConnections.EstimatedCurrent() <= cvcParameters.cvcLeakLimit ) continue;
			}
			myErrorFlag = true;
		} else if ( myConnections.EstimatedMinimumCurrent() > cvcParameters.cvcLeakLimit ) {
			myErrorFlag = true;
		}
		if ( myErrorFlag ) {
			if ( IncrementDeviceError(myConnections.deviceId, NMOS_POSSIBLE_LEAK) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
				PrintDeviceWithAllConnections(deviceParent_v[device_it], myConnections, errorFile);
				errorFile << endl;
			}
		}
	}
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking nmos possible leak errors: ");
}

void CCvcDb::FindPmosPossibleLeakErrors() {
	CFullConnection myConnections;
	reportFile << "! Checking pmos possible leak errors: " << endl << endl;
	errorFile << "! Checking pmos possible leak errors: " << endl << endl;
	for ( deviceId_t device_it = 0; device_it < deviceCount; device_it++ ) {
		CInstance * myInstance_p = instancePtr_v[deviceParent_v[device_it]];
		CCircuit * myParent_p = myInstance_p->master_p;
		CDevice * myDevice_p = myParent_p->devicePtr_v[device_it - myInstance_p->firstDeviceId];
		if ( ! IsPmos_(myDevice_p->model_p->type) ) continue;
		bool myErrorFlag = false;
		MapDeviceNets(myInstance_p, myDevice_p, myConnections);
		if ( IsKnownVoltage_(myConnections.simGateVoltage) ) continue;
		if ( myConnections.CheckTerminalMaxVoltages(SOURCE | DRAIN) == true ) {
			voltage_t myMinGateVoltage = MinVoltage(myConnections.gateId);
			if ( myMinGateVoltage == UNKNOWN_VOLTAGE ) {
				myMinGateVoltage = MinLeakVoltage(myConnections.gateId);
			}
			if ( myMinGateVoltage != UNKNOWN_VOLTAGE
					&& myMinGateVoltage >= myConnections.maxSourceVoltage + myConnections.device_p->model_p->Vth
					&& myMinGateVoltage >= myConnections.maxDrainVoltage + myConnections.device_p->model_p->Vth ) {
				continue;  // always off
			}
		}
		if ( myConnections.CheckTerminalSimVoltages(SOURCE | DRAIN) == true ) {
			if ( myConnections.simSourceVoltage == myConnections.simDrainVoltage ) continue;
			if ( abs(myConnections.simSourceVoltage - myConnections.simDrainVoltage) <= cvcParameters.cvcLeakErrorThreshold ) continue;
			if ( myConnections.simSourcePower_p->type[HIZ_BIT] &&
					! myConnections.simSourcePower_p->IsRelatedPower(myConnections.simDrainPower_p, netVoltagePtr_v, simNet_v, simNet_v, false) ) {
				;  // possible leaks to cutoff power
			} else if ( myConnections.simDrainPower_p->type[HIZ_BIT] &&
					! myConnections.simDrainPower_p->IsRelatedPower(myConnections.simSourcePower_p, netVoltagePtr_v, simNet_v, simNet_v, false) ) {
				;  // possible leaks to cutoff power
			} else if ( myConnections.simSourcePower_p->IsInternalOverride() || myConnections.simDrainPower_p->IsInternalOverride() ) {
				continue;
			} else if ( myConnections.simSourcePower_p->type[SIM_CALCULATED_BIT] || myConnections.simDrainPower_p->type[SIM_CALCULATED_BIT] ) {
				if ( myConnections.EstimatedCurrent() <= cvcParameters.cvcLeakLimit ) continue;
			}
			myErrorFlag = true;
		} else if ( myConnections.EstimatedMinimumCurrent() > cvcParameters.cvcLeakLimit ) {
			myErrorFlag = true;
		}
		if ( myErrorFlag ) {
			if ( IncrementDeviceError(myConnections.deviceId, PMOS_POSSIBLE_LEAK) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
				PrintDeviceWithAllConnections(deviceParent_v[device_it], myConnections, errorFile);
				errorFile << endl;
			}
		}
	}
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking pmos possible leak errors: ");
}

void CCvcDb::FindFloatingInputErrors() {
	CFullConnection myConnections;
	reportFile << "! Checking mos floating input errors:" << endl << endl;
	errorFile << "! Checking mos floating input errors:" << endl << endl;
	bool myFloatingFlag;
	for (netId_t net_it = 0; net_it < netCount; net_it++) {
		if ( connectionCount_v[net_it].gateCount > 0 ) { // skips subordinate nets. only equivalent master nets have counts
			if ( firstGate_v[net_it] == UNKNOWN_DEVICE ) continue;
			MapDeviceNets(firstGate_v[net_it], myConnections);
			if ( myConnections.simGateVoltage != UNKNOWN_VOLTAGE ) continue;  // skip known voltages
			myFloatingFlag = IsFloatingGate(myConnections);
			if ( myFloatingFlag || myConnections.IsPossibleHiZ(this) ) {
				for ( deviceId_t device_it = firstGate_v[net_it]; device_it != UNKNOWN_DEVICE; device_it = nextGate_v[device_it] ) {
					MapDeviceNets(device_it, myConnections);
					bool myHasLeakPath = HasLeakPath(myConnections);
					if ( myFloatingFlag ) {
						deviceStatus_v[device_it][SIM_INACTIVE] = true;  // ignore true floating input gates (not possible floating)
					}
					if ( cvcParameters.cvcIgnoreVthFloating && IsAlwaysOff(myConnections) ) continue;  // skips Hi-Z input that is never on
					if ( ! myHasLeakPath && cvcParameters.cvcIgnoreNoLeakFloating
							&& connectionCount_v[net_it].SourceDrainCount() == 0 ) continue;  // skip no leak floating
					if ( myHasLeakPath || connectionCount_v[net_it].SourceDrainCount() == 0 ) {  // physically floating gates too
						if ( IncrementDeviceError(myConnections.deviceId, HIZ_INPUT) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
							if ( ! myHasLeakPath ) errorFile << "* No leak path" << endl;
							if ( ! myFloatingFlag ) errorFile << "* Tri-state input" << endl;
							PrintDeviceWithAllConnections(deviceParent_v[device_it], myConnections, errorFile);
							errorFile << endl;
						}
					}
				}
			}
		}
	}
	for (netId_t net_it = 0; net_it < netCount; net_it++) {  // second pass to catch floating nets caused by floating nets
		if ( connectionCount_v[net_it].gateCount > 0 ) { // skips subordinate nets. only equivalent master nets have counts
			if ( firstGate_v[net_it] == UNKNOWN_DEVICE ) continue;
			if ( SimVoltage(net_it) != UNKNOWN_VOLTAGE || (netVoltagePtr_v[net_it].full && netVoltagePtr_v[net_it].full->type[INPUT_BIT]) ) continue;
			if ( HasActiveConnections(net_it) ) continue;
			MapDeviceNets(firstGate_v[net_it], myConnections);
			if ( IsFloatingGate(myConnections) ) continue;  // Already processed previously
			for ( deviceId_t device_it = firstGate_v[net_it]; device_it != UNKNOWN_DEVICE; device_it = nextGate_v[device_it] ) {
				MapDeviceNets(device_it, myConnections);
				bool myHasLeakPath = HasLeakPath(myConnections);
				if ( myHasLeakPath ) {
					if ( IncrementDeviceError(myConnections.deviceId, HIZ_INPUT) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
						errorFile << "* Secondary HI-Z error" << endl;
						PrintDeviceWithAllConnections(deviceParent_v[device_it], myConnections, errorFile);
						errorFile << endl;
					}
				}
			}
		}
	}
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking mos floating input errors:");
}

void CCvcDb::CheckExpectedValues() {
	CFullConnection myConnections;
	netId_t myNetId, mySimNetId, myMinNetId, myMaxNetId;
//	voltage_t mySimVoltage, myMinVoltage, myMaxVoltage;
	CVirtualNet mySimNet, myMinNet, myMaxNet;
	bool myExpectedValueFound;
	reportFile << "! Checking expected values:" << endl << endl;
	errorFile << "! Checking expected values:" << endl << endl;
	// Expected voltage errors do not respect error limit
	for (auto power_ppit = cvcParameters.cvcExpectedLevelPtrList.begin(); power_ppit != cvcParameters.cvcExpectedLevelPtrList.end(); power_ppit++) {
		size_t myLastErrorCount = errorCount[EXPECTED_VOLTAGE];
		myNetId = GetEquivalentNet((*power_ppit)->netId);
		if ( ! IsEmpty((*power_ppit)->expectedSim()) ) {
			mySimNet(simNet_v, myNetId);
			mySimNetId = mySimNet.finalNetId;
			myExpectedValueFound = false;
			myMinNet(minNet_v, myNetId);
			myMinNetId = myMinNet.finalNetId;
			myMaxNet(maxNet_v, myNetId);
			myMaxNetId = myMaxNet.finalNetId;
			if ( (*power_ppit)->expectedSim() == "open" ) {
				if ( ( mySimNetId == UNKNOWN_NET || ! netVoltagePtr_v[mySimNetId].full || netVoltagePtr_v[mySimNetId].full->simVoltage == UNKNOWN_VOLTAGE ) &&
						( myMinNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMinNetId].full || netVoltagePtr_v[myMinNetId].full->minVoltage == UNKNOWN_VOLTAGE || netVoltagePtr_v[myMinNetId].full->type[HIZ_BIT] ||
							myMaxNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMaxNetId].full || netVoltagePtr_v[myMaxNetId].full->maxVoltage == UNKNOWN_VOLTAGE || netVoltagePtr_v[myMaxNetId].full->type[HIZ_BIT]) ) { // open match
					myExpectedValueFound = true;
				}
			} else if ( mySimNetId != UNKNOWN_NET && netVoltagePtr_v[mySimNetId].full && netVoltagePtr_v[mySimNetId].full->simVoltage != UNKNOWN_VOLTAGE ) {
				if ( IsValidVoltage_((*power_ppit)->expectedSim())
						&& abs(String_to_Voltage((*power_ppit)->expectedSim()) - netVoltagePtr_v[mySimNetId].full->simVoltage) <= cvcParameters.cvcExpectedErrorThreshold ) { // voltage match
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedSim() == NetName(mySimNetId) ) { // name match
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedSim() == string(netVoltagePtr_v[mySimNetId].full->powerAlias()) ) { // alias match
					myExpectedValueFound = true;
				}
				if ( ! myExpectedValueFound ) {
					debugFile << "EXPECT: " << NetName(myNetId) << " " << NetName(mySimNetId) << "@" << netVoltagePtr_v[mySimNetId].full->simVoltage << "~>" << netVoltagePtr_v[mySimNetId].full->powerAlias() << " = " << (*power_ppit)->expectedSim() << endl;
				}
			}
			if ( ! myExpectedValueFound ) {
				errorCount[EXPECTED_VOLTAGE]++;
				errorFile << "Expected " << NetName(myNetId) << " = " << (*power_ppit)->expectedSim() << " but found ";
				if ( mySimNetId != UNKNOWN_NET && netVoltagePtr_v[mySimNetId].full && netVoltagePtr_v[mySimNetId].full->simVoltage != UNKNOWN_VOLTAGE ) {
					errorFile << NetName(mySimNetId) << "@" << PrintVoltage(netVoltagePtr_v[mySimNetId].full->simVoltage) << endl;
				} else if ( (*power_ppit)->expectedSim() == "open" ) {
					bool myPrintedReason = false;
					if ( myMinNetId != UNKNOWN_NET && netVoltagePtr_v[myMinNetId].full && netVoltagePtr_v[myMinNetId].full->minVoltage != UNKNOWN_VOLTAGE ) {
						errorFile << "(min)" << NetName(myMinNetId) << "@" << PrintVoltage(netVoltagePtr_v[myMinNetId].full->minVoltage) << " ";
						myPrintedReason = true;
					}
					if ( myMaxNetId != UNKNOWN_NET && netVoltagePtr_v[myMaxNetId].full && netVoltagePtr_v[myMaxNetId].full->maxVoltage != UNKNOWN_VOLTAGE ) {
						errorFile << "(max)" << NetName(myMaxNetId) << "@" << PrintVoltage(netVoltagePtr_v[myMaxNetId].full->maxVoltage);
						myPrintedReason = true;
					}
					if ( myPrintedReason ) {
						errorFile << endl;
					} else {
						errorFile << "unknown" << endl;
					}
				} else {
					errorFile << "unknown" << endl;
				}
			}
		}
		if ( ! IsEmpty((*power_ppit)->expectedMin()) ) {
			myMinNet(minNet_v, myNetId);
			myMinNetId = myMinNet.finalNetId;
			myExpectedValueFound = false;
			if ( (*power_ppit)->expectedMin() == "open" ) {
				// simVoltage in the following condition is correct. open voltages can have min/max voltages.
				if ( myMinNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMinNetId].full || netVoltagePtr_v[myMinNetId].full->simVoltage == UNKNOWN_VOLTAGE ) { // open match
					myExpectedValueFound = true;
				}
			} else if ( myMinNetId != UNKNOWN_NET && netVoltagePtr_v[myMinNetId].full ) {
				if ( String_to_Voltage((*power_ppit)->expectedMin()) <= netVoltagePtr_v[myMinNetId].full->minVoltage ) { // voltage match (anything higher than expected is ok)
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedMin() == NetName(myMinNetId) ) { // name match
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedMin() == string(netVoltagePtr_v[myMinNetId].full->powerAlias()) ) { // alias match
					myExpectedValueFound = true;
				}
			}
			if ( ! myExpectedValueFound ) {
				errorCount[EXPECTED_VOLTAGE]++;
				errorFile << "Expected minimum " << NetName(myNetId) << " = " << (*power_ppit)->expectedMin() << " but found ";
				if ( myMinNetId != UNKNOWN_NET && netVoltagePtr_v[myMinNetId].full && netVoltagePtr_v[myMinNetId].full->minVoltage != UNKNOWN_VOLTAGE ) {
					errorFile << NetName(myMinNetId) << "@" << PrintVoltage(netVoltagePtr_v[myMinNetId].full->minVoltage) << endl;
				} else {
					errorFile << "unknown" << endl;
				}
			}
		}
		if ( ! IsEmpty((*power_ppit)->expectedMax()) ) {
			myMaxNet(maxNet_v, myNetId);
			myMaxNetId = myMaxNet.finalNetId;
			myExpectedValueFound = false;
			if ( (*power_ppit)->expectedMax() == "open" ) {
				// simVoltage in the following condition is correct. open voltages can have min/max voltages.
				if ( myMaxNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMaxNetId].full || netVoltagePtr_v[myMaxNetId].full->simVoltage == UNKNOWN_VOLTAGE ) { // open match
					myExpectedValueFound = true;
				}
			} else if ( myMaxNetId != UNKNOWN_NET && netVoltagePtr_v[myMaxNetId].full ) {
				if ( String_to_Voltage((*power_ppit)->expectedMax()) >= netVoltagePtr_v[myMaxNetId].full->maxVoltage ) { // voltage match (anything lower than expected is ok)
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedMax() == NetName(myMaxNetId) ) { // name match
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedMax() == string(netVoltagePtr_v[myMaxNetId].full->powerAlias()) ) { // alias match
					myExpectedValueFound = true;
				}
			}
			if ( ! myExpectedValueFound ) {
				errorCount[EXPECTED_VOLTAGE]++;
				errorFile << "Expected maximum " << NetName(myNetId) << " = " << (*power_ppit)->expectedMax() << " but found ";
				if ( myMaxNetId != UNKNOWN_NET && netVoltagePtr_v[myMaxNetId].full && netVoltagePtr_v[myMaxNetId].full->maxVoltage != UNKNOWN_VOLTAGE ) {
					errorFile << NetName(myMaxNetId) << "@" << PrintVoltage(netVoltagePtr_v[myMaxNetId].full->maxVoltage) << endl;
				} else {
					errorFile << "unknown" << endl;
				}
			}
		}
		if ( myLastErrorCount != errorCount[EXPECTED_VOLTAGE] ) errorFile << endl;
	}
	errorFile << "! Finished" << endl << endl;
}

void CCvcDb::FindLDDErrors() {
	CFullConnection myConnections;
	for (CModelListMap::iterator keyModelListPair_pit = cvcParameters.cvcModelListMap.begin(); keyModelListPair_pit != cvcParameters.cvcModelListMap.end(); keyModelListPair_pit++) {
		for (CModelList::iterator model_pit = keyModelListPair_pit->second.begin(); model_pit != keyModelListPair_pit->second.end(); model_pit++) {
			if ( model_pit->type == LDDN || model_pit->type == LDDP ) {
				reportFile << "! Checking LDD errors for model: " << model_pit->definition << endl << endl;
				errorFile << "! Checking LDD errors for model: " << model_pit->definition << endl << endl;
				CDevice * myDevice_p = model_pit->firstDevice_p;
				while (myDevice_p) {
					CCircuit * myParent_p = myDevice_p->parent_p;
	//				deviceId_t myLocalDeviceId = myParent_p->localDeviceIdMap[myDevice_p->name];
					for (instanceId_t instance_it = 0; instance_it < myParent_p->instanceId_v.size(); instance_it++) {
						if  ( instancePtr_v[myParent_p->instanceId_v[instance_it]]->IsParallelInstance() ) continue;  // parallel/empty instances
						CInstance * myInstance_p = instancePtr_v[myParent_p->instanceId_v[instance_it]];
						MapDeviceNets(myInstance_p, myDevice_p, myConnections);
						if ( model_pit->type == LDDN ) {
							if ( ! myConnections.CheckTerminalMinMaxVoltages(SOURCE | DRAIN, true) ) continue;
							if ( myConnections.maxSourceVoltage <= myConnections.minDrainVoltage ) continue;
							if ( myConnections.minSourceVoltage > myConnections.minDrainVoltage ||
									( myConnections.minSourceVoltage == myConnections.minDrainVoltage &&
										myConnections.masterMinSourceNet.finalResistance > myConnections.masterMinDrainNet.finalResistance ) ||
									myConnections.maxSourceVoltage > myConnections.maxDrainVoltage ||
									( myConnections.maxSourceVoltage == myConnections.maxDrainVoltage &&
										myConnections.masterMaxSourceNet.finalResistance < myConnections.masterMaxDrainNet.finalResistance &&
										myConnections.minSourceVoltage != myConnections.maxDrainVoltage && // if min = max -> no leak regardless of resistance
//										myConnections.masterMaxSourceNet.finalNetId != myConnections.masterMaxDrainNet.finalNetId &&
										myConnections.masterMaxDrainNet.nextNetId != GetEquivalentNet(myConnections.sourceId)) ) { // NMOS max voltage resistance check is intentionally backwards
								if ( !( IsKnownVoltage_(myConnections.simGateVoltage) &&
										myConnections.simGateVoltage <= min(myConnections.minSourceVoltage, myConnections.minDrainVoltage) ) ) {
									if ( IncrementDeviceError(myConnections.deviceId, LDD_SOURCE) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
										PrintDeviceWithAllConnections(myParent_p->instanceId_v[instance_it], myConnections, errorFile);
										errorFile << endl;
									}
								}
							}
						} else {
							assert( model_pit->type == LDDP );
							if ( ! myConnections.CheckTerminalMinMaxVoltages(SOURCE | DRAIN, true) ) continue;
							if ( myConnections.minSourceVoltage >= myConnections.maxDrainVoltage ) continue;
							if ( myConnections.maxSourceVoltage < myConnections.maxDrainVoltage ||
									( myConnections.maxSourceVoltage == myConnections.maxDrainVoltage &&
										myConnections.masterMaxSourceNet.finalResistance > myConnections.masterMaxDrainNet.finalResistance ) ||
									myConnections.minSourceVoltage < myConnections.minDrainVoltage ||
									( myConnections.minSourceVoltage == myConnections.minDrainVoltage &&
										myConnections.masterMinSourceNet.finalResistance < myConnections.masterMinDrainNet.finalResistance &&
										myConnections.maxSourceVoltage != myConnections.minDrainVoltage && // if max = min -> no leak regardless of resistance
//										myConnections.masterMinSourceNet.finalNetId != myConnections.masterMinDrainNet.finalNetId &&
										myConnections.masterMinDrainNet.nextNetId != GetEquivalentNet(myConnections.sourceId)) ) { // PMOS max voltage resistance check is intentionally backwards
								if ( !( IsKnownVoltage_(myConnections.simGateVoltage) &&
										myConnections.simGateVoltage >= max(myConnections.maxSourceVoltage, myConnections.maxDrainVoltage) ) ) {
									if ( IncrementDeviceError(myConnections.deviceId, LDD_SOURCE) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
										PrintDeviceWithAllConnections(myParent_p->instanceId_v[instance_it], myConnections, errorFile);
										errorFile << endl;
									}
								}
							}
						}
					}
					myDevice_p = myDevice_p->nextDevice_p;
				}
			}
		}
	}
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking LDD errors for model: ");
}

void CCvcDb::CheckInverterIO(modelType_t theType) {
	// Checks nets loaded from cvcNetCheckFile
	for ( auto check_pit = inverterInputOutputCheckList.begin(); check_pit != inverterInputOutputCheckList.end(); check_pit++ ) {
		debugFile << "DEBUG: inverter input output check " << *check_pit << endl;
		set<netId_t> * myNetIdList = FindUniqueNetIds(*check_pit);
		CVirtualNet myMinInput;
		CVirtualNet myMaxInput;
		CVirtualNet myMinOutput;
		CVirtualNet myMaxOutput;
		for ( auto net_pit = myNetIdList->begin(); net_pit != myNetIdList->end(); net_pit++ ) {
			netId_t myInverterInput = FindInverterInput(*net_pit);
			if ( myInverterInput == UNKNOWN_NET ) {
				myInverterInput = inverterNet_v[*net_pit];
				reportFile << "INFO: Couldn't calculate inverter input for " << NetName(*net_pit, true) << endl;
			}
			if (myInverterInput == UNKNOWN_NET) {
				reportFile << "Warning: expected inverter input at " << NetName(*net_pit, true) << endl;
				continue;

			}
			myMinInput(minNet_v, myInverterInput);
			myMaxInput(maxNet_v, myInverterInput);
			myMinOutput(minNet_v, *net_pit);
			myMaxOutput(maxNet_v, *net_pit);
			if ( myMinInput.finalNetId == myMaxInput.finalNetId
					&& ( myMinInput.finalNetId == myMinOutput.finalNetId
						|| myMaxInput.finalNetId == myMaxOutput.finalNetId ) ) continue;  // input is tied to matching power.

			if ( ( myMinInput.finalNetId != myMinOutput.finalNetId && theType == NMOS )
					|| ( myMaxInput.finalNetId != myMaxOutput.finalNetId && theType == PMOS ) ) {
				deviceId_t myDevice = FindInverterDevice(myInverterInput, *net_pit, theType);
				if ( myDevice == UNKNOWN_DEVICE ) {
					reportFile << "Warning: could not find inverter device for " << NetName(*net_pit, true) << endl;
					continue;

				}
				if ( IncrementDeviceError(myDevice, (theType == NMOS ? NMOS_GATE_SOURCE : PMOS_GATE_SOURCE)) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
					CFullConnection myFullConnections;
					MapDeviceNets(myDevice, myFullConnections);
					errorFile << "* inverter input/output mismatch" << endl;
					PrintDeviceWithAllConnections(deviceParent_v[myDevice], myFullConnections, errorFile);
					errorFile << endl;
				}
			}
		}
	}
}

void CCvcDb::CheckOppositeLogic(modelType_t theType) {
	for ( auto check_pit = oppositeLogicList.begin(); check_pit != oppositeLogicList.end(); check_pit++ ) {
		debugFile << "DEBUG: opposite logic check " << get<0>(*check_pit) << " & " << get<1>(*check_pit) << endl;
		forward_list<netId_t> * myNetIdList = FindNetIds(get<0>(*check_pit));
		forward_list<netId_t> * myOppositeNetIdList = FindNetIds(get<1>(*check_pit));
		for ( auto net_pit = myNetIdList->begin(), opposite_pit = myOppositeNetIdList->begin(); net_pit != myNetIdList->end(); net_pit++, opposite_pit++ ) {
			CPower * myFirstPower_p = netVoltagePtr_v[GetEquivalentNet(*net_pit)].full;
			CPower * mySecondPower_p = netVoltagePtr_v[GetEquivalentNet(*opposite_pit)].full;
			if ( myFirstPower_p && mySecondPower_p && IsPower_(myFirstPower_p) && IsPower_(mySecondPower_p)
				&& myFirstPower_p->simVoltage != mySecondPower_p->simVoltage ) continue;  // ignore direct connections to different power

			unordered_set<netId_t> myInvertedNets;
			unordered_set<netId_t> mySameLogicNets;
			netId_t net_it = GetEquivalentNet(*net_pit);
			// make sets of same logic nets and opposite logic nets for first nets
			bool myInverted = false;
			logFile << "Checking " << NetName(net_it, true) << endl;
			while ( net_it != UNKNOWN_NET && mySameLogicNets.count(net_it) == 0 ) {
				if ( myInverted ) {
					myInvertedNets.insert(net_it);
					assert(myInvertedNets.count(inverterNet_v[net_it]) == 0);  // oscillators

				} else {
					mySameLogicNets.insert(net_it);
					assert(mySameLogicNets.count(inverterNet_v[net_it]) == 0);  // oscillators

				}
				myInverted = ! myInverted;
				net_it = inverterNet_v[net_it];
			}
			// check second net against first net to find opposite logic
			myInverted = true;
			net_it = GetEquivalentNet(*opposite_pit);
			while ( net_it != UNKNOWN_NET
					&& ( (myInverted && myInvertedNets.count(net_it) == 0)
						|| (! myInverted && mySameLogicNets.count(net_it) == 0) ) ) {
				net_it = inverterNet_v[net_it];
				myInverted = ! myInverted;
			}
			if ( net_it != UNKNOWN_NET ) continue;  // nets are opposite

			netId_t myErrorNet = (myFirstPower_p && IsPower_(myFirstPower_p)) ? GetEquivalentNet(*opposite_pit) : GetEquivalentNet(*net_pit);
			int myErrorCount = 0;
			for ( auto device_it = firstGate_v[myErrorNet]; device_it != UNKNOWN_DEVICE; device_it = nextGate_v[device_it]) {
				if ( sourceNet_v[device_it] == drainNet_v[device_it] ) continue;  // ignore inactive devices

				if ( theType == PMOS && ! IsPmos_(deviceType_v[device_it]) ) continue;  // ignore wrong types

				if ( theType == NMOS && ! IsNmos_(deviceType_v[device_it]) ) continue;  // ignore wrong types

				myErrorCount++;
				if ( IncrementDeviceError(device_it, HIZ_INPUT) < cvcParameters.cvcCircuitErrorLimit || cvcParameters.cvcCircuitErrorLimit == 0 ) {
					CFullConnection myFullConnections;
					MapDeviceNets(device_it, myFullConnections);
					errorFile << "* opposite logic required " << get<0>(*check_pit) << " & " << get<1>(*check_pit) << endl;
					PrintDeviceWithAllConnections(deviceParent_v[device_it], myFullConnections, errorFile);
					errorFile << endl;
				}
			}
			if ( myErrorCount == 0 ) {
				reportFile << "Warning: No errors printed for opposite logic check at " << get<0>(*check_pit) << " & " << get<1>(*check_pit);
				reportFile << " for net " << NetName(*net_pit, true) << endl;
			}
		}
	}
}
