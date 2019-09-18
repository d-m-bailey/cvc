/*
 * CCvcDb_error.cc
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

/*
void CCvcDb::FindOverVoltageErrors(string theCheck, int theErrorIndex) {
	CFullConnection myConnections;
	reportFile << "! Checking " << theCheck << " overvoltage errors" << endl << endl;
	errorFile << "! Checking " << theCheck << " overvoltage errors" << endl << endl;
	for (CModelListMap::iterator keyModelListPair_pit = cvcParameters.cvcModelListMap.begin(); keyModelListPair_pit != cvcParameters.cvcModelListMap.end(); keyModelListPair_pit++) {
		for (CModelList::iterator model_pit = keyModelListPair_pit->second.begin(); model_pit != keyModelListPair_pit->second.end(); model_pit++) {
			if ( theErrorIndex == OVERVOLTAGE_VBG && model_pit->maxVbg == UNKNOWN_VOLTAGE) continue;
			if ( theErrorIndex == OVERVOLTAGE_VBS && model_pit->maxVbs == UNKNOWN_VOLTAGE) continue;
			if ( theErrorIndex == OVERVOLTAGE_VDS && model_pit->maxVds == UNKNOWN_VOLTAGE) continue;
			if ( theErrorIndex == OVERVOLTAGE_VGS && model_pit->maxVgs == UNKNOWN_VOLTAGE) continue;
			CDevice * myDevice_p = model_pit->firstDevice_p;
			string myDisplayParameter;
			if ( theErrorIndex == OVERVOLTAGE_VBG ) {
				myDisplayParameter = " Vbg=" + PrintToleranceParameter(model_pit->maxVbgDefinition, model_pit->maxVbg, VOLTAGE_SCALE);
			} else if ( theErrorIndex == OVERVOLTAGE_VBS ) {
				myDisplayParameter = " Vbs=" + PrintToleranceParameter(model_pit->maxVbsDefinition, model_pit->maxVbs, VOLTAGE_SCALE);
			} else if ( theErrorIndex == OVERVOLTAGE_VDS ) {
				myDisplayParameter = " Vds=" + PrintToleranceParameter(model_pit->maxVdsDefinition, model_pit->maxVds, VOLTAGE_SCALE);
			} else if ( theErrorIndex == OVERVOLTAGE_VGS ) {
				myDisplayParameter = " Vgs=" + PrintToleranceParameter(model_pit->maxVgsDefinition, model_pit->maxVgs, VOLTAGE_SCALE);
			}
			myDisplayParameter += " " + model_pit->ConditionString();
			while (myDevice_p) {
				CCircuit * myParent_p = myDevice_p->parent_p;
				for (instanceId_t instance_it = 0; instance_it < myParent_p->instanceId_v.size(); instance_it++) {
					if  ( instancePtr_v[myParent_p->instanceId_v[instance_it]]->IsParallelInstance() ) continue;  // parallel/empty instances
					CInstance * myInstance_p = instancePtr_v[myParent_p->instanceId_v[instance_it]];
					MapDeviceNets(myInstance_p, myDevice_p, myConnections);
					myConnections.SetMinMaxLeakVoltagesAndFlags(this);
					string myErrorExplanation = "";
					if ( theErrorIndex == OVERVOLTAGE_VBG ) {
						myErrorExplanation = FindVbgError(model_pit->maxVbg, myConnections);
					} else if ( theErrorIndex == OVERVOLTAGE_VBS ) {
						myErrorExplanation = FindVbsError(model_pit->maxVbs, myConnections);
					} else if ( theErrorIndex == OVERVOLTAGE_VDS ) {
						myErrorExplanation = FindVdsError(model_pit->maxVds, myConnections);
					} else if ( theErrorIndex == OVERVOLTAGE_VGS ) {
						myErrorExplanation = FindVgsError(model_pit->maxVgs, myConnections);
					}
					if ( model_pit->maxVbg != UNKNOWN_VOLTAGE ) {
					}
					if ( ! IsEmpty(myErrorExplanation) ) {
//						errorCount[theErrorIndex]++;
						if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(myConnections.deviceId, theErrorIndex) < cvcParameters.cvcCircuitErrorLimit ) {
							errorFile << myErrorExplanation + myDisplayParameter << endl;;
							bool myLeakCheckFlag = ( myErrorExplanation.find("logic ok") < string::npos );
							PrintDeviceWithAllConnections(myParent_p->instanceId_v[instance_it], myConnections, errorFile, myLeakCheckFlag);
							errorFile << endl;
						}
					}
				}
				myDevice_p = myDevice_p->nextDevice_p;
			}
		}
	}
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking " + theCheck + " overvoltage errors", theErrorIndex - OVERVOLTAGE_VBG);
//	  errorFile << "! Finished" << endl << endl; << !
}
*/
 
void CCvcDb::PrintOverVoltageError(ogzstream & theErrorFile, CFullConnection & theConnections, cvcError_t theErrorIndex, string theExplanation, instanceId_t theInstanceId) {
	if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(theConnections.deviceId, theErrorIndex) < cvcParameters.cvcCircuitErrorLimit ) {
		theErrorFile << theExplanation << endl;
		bool myLeakCheckFlag = ( theExplanation.find("logic ok") < string::npos );
		PrintDeviceWithAllConnections(theInstanceId, theConnections, theErrorFile, myLeakCheckFlag);
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

/*					if ( ! IsEmpty(myErrorExplanation) ) {
//						  errorCount[theErrorIndex]++;
						if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(myConnections.deviceId, theErrorIndex) < cvcParameters.cvcCircuitErrorLimit ) {
							errorFile << myErrorExplanation + myDisplayParameter << endl;;
							bool myLeakCheckFlag = ( myErrorExplanation.find("logic ok") < string::npos );
							PrintDeviceWithAllConnections(myParent_p->instanceId_v[instance_it], myConnections, errorFile, myLeakCheckFlag);
							errorFile << endl;
						}
					}
*/
				}
				myDevice_p = myDevice_p->nextDevice_p;
			}
		}
	}
	myVbgErrorFile.close();
	myVbsErrorFile.close();
	myVdsErrorFile.close();
	myVgsErrorFile.close();
	AppendErrorFile(myVbgErrorFileName, "! Checking Vbg overvoltage errors", OVERVOLTAGE_VBG - OVERVOLTAGE_VBG);
	AppendErrorFile(myVbsErrorFileName, "! Checking Vbs overvoltage errors", OVERVOLTAGE_VBS - OVERVOLTAGE_VBG);
	AppendErrorFile(myVdsErrorFileName, "! Checking Vds overvoltage errors", OVERVOLTAGE_VDS - OVERVOLTAGE_VBG);
	AppendErrorFile(myVgsErrorFileName, "! Checking Vgs overvoltage errors", OVERVOLTAGE_VGS - OVERVOLTAGE_VBG);
//	  cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking " + theCheck + " overvoltage errors", theErrorIndex - OVERVOLTAGE_VBG);
//	errorFile << "! Finished" << endl << endl;
}

void CCvcDb::AppendErrorFile(string theTempFileName, string theHeading, int theErrorSubIndex) {
	igzstream myTempFile(theTempFileName);
	errorFile << myTempFile.rdbuf();
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, theHeading, theErrorSubIndex);
	myTempFile.close();
	remove(theTempFileName.c_str());
}
 
//void CCvcDb::FindOverVoltageErrors() {
//	CFullConnection myConnections;
//	for (CModelListMap::iterator keyModelListPair_pit = cvcParameters.cvcModelListMap.begin(); keyModelListPair_pit != cvcParameters.cvcModelListMap.end(); keyModelListPair_pit++) {
//		for (CModelList::iterator model_pit = keyModelListPair_pit->second.begin(); model_pit != keyModelListPair_pit->second.end(); model_pit++) {
//			if ( model_pit->maxVbg == UNKNOWN_VOLTAGE && model_pit->maxVgs == UNKNOWN_VOLTAGE && model_pit->maxVbs == UNKNOWN_VOLTAGE && model_pit->maxVds == UNKNOWN_VOLTAGE ) {
//				if ( model_pit->type != SWITCH_OFF && model_pit->type != SWITCH_ON && model_pit->firstDevice_p ) {
//					reportFile << "WARNING: no overvoltage check for model: " << model_pit->definition << endl << endl;
//				}
//				continue;
//			}
//			reportFile << "! Checking overvoltage errors for model: " << model_pit->definition << endl << endl;
//			errorFile << "! Checking overvoltage errors for model: " << model_pit->definition << endl << endl;
//			CDevice * myDevice_p = model_pit->firstDevice_p;
//			while (myDevice_p) {
//				CCircuit * myParent_p = myDevice_p->parent_p;
////				deviceId_t myLocalDeviceId = myParent_p->localDeviceIdMap[myDevice_p->name];
//				for (instanceId_t instance_it = 0; instance_it < myParent_p->instanceId_v.size(); instance_it++) {
//					CInstance * myInstance_p = instancePtr_v[myParent_p->instanceId_v[instance_it]];
//					MapDeviceNets(myInstance_p, myDevice_p, myConnections);
////					myConnections.SetUnknownVoltage();
//					myConnections.SetMinMaxLeakVoltages(this);
//					bool myErrorFlag = false;
//					bool myLeakCheckFlag = false;
//					string myErrorExplanation;
//					if ( model_pit->maxVbg != UNKNOWN_VOLTAGE ) {
//						if ( ( myConnections.CheckTerminalMinMaxVoltages(GATE | BULK, false) &&
//									( abs(myConnections.maxGateVoltage - myConnections.minBulkVoltage) > model_pit->maxVbg ||
//											abs(myConnections.minGateVoltage - myConnections.maxBulkVoltage) > model_pit->maxVbg ) ) ||
//								( myConnections.CheckTerminalMinMaxVoltages(GATE, false) && ! myConnections.CheckTerminalMinMaxVoltages(BULK, false) &&
//										max(abs(myConnections.minGateVoltage), abs(myConnections.maxGateVoltage)) > model_pit->maxVbg ) ||
//								( ! myConnections.CheckTerminalMinMaxVoltages(GATE, false) && myConnections.CheckTerminalMinMaxVoltages(BULK, false) &&
//										max(abs(myConnections.minBulkVoltage), abs(myConnections.maxBulkVoltage)) > model_pit->maxVbg ) ) {
//							myErrorExplanation += "Overvoltage Error:Gate vs Bulk:\n";
//							myErrorFlag = true;
//						} else if ( ( myConnections.CheckTerminalMinMaxLeakVoltages(GATE | BULK) &&
//									( abs(myConnections.maxGateLeakVoltage - myConnections.minBulkLeakVoltage) > model_pit->maxVbg ||
//											abs(myConnections.minGateLeakVoltage - myConnections.maxBulkLeakVoltage) > model_pit->maxVbg ) ) ||
//								( myConnections.CheckTerminalMinMaxLeakVoltages(GATE) && ! myConnections.CheckTerminalMinMaxLeakVoltages(BULK) &&
//										max(abs(myConnections.minGateLeakVoltage), abs(myConnections.maxGateLeakVoltage)) > model_pit->maxVbg ) ||
//								( ! myConnections.CheckTerminalMinMaxLeakVoltages(GATE) && myConnections.CheckTerminalMinMaxLeakVoltages(BULK) &&
//										max(abs(myConnections.minBulkLeakVoltage), abs(myConnections.maxBulkLeakVoltage)) > model_pit->maxVbg ) ) {
//							myErrorExplanation += "Overvoltage Error:Gate vs Bulk: (logic ok)\n";
//							myErrorFlag = true;
//							myLeakCheckFlag = true;
//						}
//					}
//					if ( model_pit->maxVgs != UNKNOWN_VOLTAGE ) {
//						if ( ( myConnections.CheckTerminalMinMaxVoltages(GATE | SOURCE, false) &&
//									( abs(myConnections.maxGateVoltage - myConnections.minSourceVoltage) > model_pit->maxVgs ||
//										abs(myConnections.minGateVoltage - myConnections.maxSourceVoltage) > model_pit->maxVgs ) ) ||
//								( myConnections.CheckTerminalMinMaxVoltages(GATE | DRAIN, false) &&
//									( abs(myConnections.maxGateVoltage - myConnections.minDrainVoltage) > model_pit->maxVgs ||
//										abs(myConnections.minGateVoltage - myConnections.maxDrainVoltage) > model_pit->maxVgs ) ) ||
//								( myConnections.CheckTerminalMinMaxVoltages(GATE, false) && ! myConnections.CheckTerminalMinMaxVoltages(SOURCE | DRAIN, false) &&
//										max(abs(myConnections.minGateVoltage), abs(myConnections.maxGateVoltage)) > model_pit->maxVgs ) ||
//								( ! myConnections.CheckTerminalMinMaxVoltages(GATE, false) && myConnections.CheckTerminalMinMaxVoltages(SOURCE, false) &&
//										max(abs(myConnections.minSourceVoltage), abs(myConnections.maxSourceVoltage)) > model_pit->maxVgs ) ||
//								( ! myConnections.CheckTerminalMinMaxVoltages(GATE, false) && myConnections.CheckTerminalMinMaxVoltages(DRAIN, false) &&
//										max(abs(myConnections.minDrainVoltage), abs(myConnections.maxDrainVoltage)) > model_pit->maxVgs ) ) {
//							myErrorExplanation += "Overvoltage Error:Gate vs Source/Drain:\n";
//							myErrorFlag = true;
//						} else if ( ( myConnections.CheckTerminalMinMaxLeakVoltages(GATE | SOURCE) &&
//									( abs(myConnections.maxGateLeakVoltage - myConnections.minSourceLeakVoltage) > model_pit->maxVgs ||
//										abs(myConnections.minGateLeakVoltage - myConnections.maxSourceLeakVoltage) > model_pit->maxVgs ) ) ||
//								( myConnections.CheckTerminalMinMaxLeakVoltages(GATE | DRAIN) &&
//									( abs(myConnections.maxGateLeakVoltage - myConnections.minDrainLeakVoltage) > model_pit->maxVgs ||
//										abs(myConnections.minGateLeakVoltage - myConnections.maxDrainLeakVoltage) > model_pit->maxVgs ) ) ||
//								( myConnections.CheckTerminalMinMaxLeakVoltages(GATE) && ! myConnections.CheckTerminalMinMaxLeakVoltages(SOURCE | DRAIN) &&
//										max(abs(myConnections.minGateLeakVoltage), abs(myConnections.maxGateLeakVoltage)) > model_pit->maxVgs ) ||
//								( ! myConnections.CheckTerminalMinMaxLeakVoltages(GATE) && myConnections.CheckTerminalMinMaxLeakVoltages(SOURCE) &&
//										max(abs(myConnections.minSourceLeakVoltage), abs(myConnections.maxSourceLeakVoltage)) > model_pit->maxVgs ) ||
//								( ! myConnections.CheckTerminalMinMaxLeakVoltages(GATE) && myConnections.CheckTerminalMinMaxLeakVoltages(DRAIN) &&
//										max(abs(myConnections.minDrainLeakVoltage), abs(myConnections.maxDrainLeakVoltage)) > model_pit->maxVgs ) ) {
//							myErrorExplanation += "Overvoltage Error:Gate vs Source/Drain: (logic ok)\n";
//							myErrorFlag = true;
//							myLeakCheckFlag = true;
//						}
//					}
//					if ( model_pit->maxVbs != UNKNOWN_VOLTAGE ) {
//						if ( ( myConnections.CheckTerminalMinMaxVoltages(SOURCE | BULK, false) &&
//									(abs(myConnections.maxBulkVoltage - myConnections.minSourceVoltage) > model_pit->maxVbs ||
//										abs(myConnections.minBulkVoltage - myConnections.maxSourceVoltage) > model_pit->maxVbs) ) ||
//								( myConnections.CheckTerminalMinMaxVoltages(DRAIN | BULK, false) &&
//									(abs(myConnections.maxBulkVoltage - myConnections.minDrainVoltage) > model_pit->maxVbs ||
//										abs(myConnections.minBulkVoltage - myConnections.maxDrainVoltage) > model_pit->maxVbs ) ) ||
//								( myConnections.CheckTerminalMinMaxVoltages(BULK, false) && ! myConnections.CheckTerminalMinMaxVoltages(SOURCE | DRAIN, false) &&
//									max(abs(myConnections.minBulkVoltage), abs(myConnections.maxBulkVoltage)) > model_pit->maxVbs ) ||
//								( ! myConnections.CheckTerminalMinMaxVoltages(BULK, false) && myConnections.CheckTerminalMinMaxVoltages(SOURCE, false) &&
//									max(abs(myConnections.minSourceVoltage), abs(myConnections.maxSourceVoltage)) > model_pit->maxVbs ) ||
//								( ! myConnections.CheckTerminalMinMaxVoltages(BULK, false) && myConnections.CheckTerminalMinMaxVoltages(DRAIN, false) &&
//									max(abs(myConnections.minDrainVoltage), abs(myConnections.maxDrainVoltage)) > model_pit->maxVbs ) )	{
//							myErrorExplanation += "Overvoltage Error:Source/Drain vs Bulk:\n";
//							myErrorFlag = true;
//						} else if ( ( myConnections.CheckTerminalMinMaxLeakVoltages(SOURCE | BULK) &&
//									(abs(myConnections.maxBulkLeakVoltage - myConnections.minSourceLeakVoltage) > model_pit->maxVbs ||
//										abs(myConnections.minBulkLeakVoltage - myConnections.maxSourceLeakVoltage) > model_pit->maxVbs) ) ||
//								( myConnections.CheckTerminalMinMaxLeakVoltages(DRAIN | BULK) &&
//									(abs(myConnections.maxBulkLeakVoltage - myConnections.minDrainLeakVoltage) > model_pit->maxVbs ||
//										abs(myConnections.minBulkLeakVoltage - myConnections.maxDrainLeakVoltage) > model_pit->maxVbs ) ) ||
//								( myConnections.CheckTerminalMinMaxLeakVoltages(BULK) && ! myConnections.CheckTerminalMinMaxLeakVoltages(SOURCE | DRAIN) &&
//									max(abs(myConnections.minBulkLeakVoltage), abs(myConnections.maxBulkLeakVoltage)) > model_pit->maxVbs ) ||
//								( ! myConnections.CheckTerminalMinMaxLeakVoltages(BULK) && myConnections.CheckTerminalMinMaxLeakVoltages(SOURCE) &&
//									max(abs(myConnections.minSourceLeakVoltage), abs(myConnections.maxSourceLeakVoltage)) > model_pit->maxVbs ) ||
//								( ! myConnections.CheckTerminalMinMaxLeakVoltages(BULK) && myConnections.CheckTerminalMinMaxLeakVoltages(DRAIN) &&
//									max(abs(myConnections.minDrainLeakVoltage), abs(myConnections.maxDrainLeakVoltage)) > model_pit->maxVbs ) )	{
//							myErrorExplanation += "Overvoltage Error:Source/Drain vs Bulk: (logic ok)\n";
//							myErrorFlag = true;
//							myLeakCheckFlag = true;
//						}
//					}
//					if ( model_pit->maxVds != UNKNOWN_VOLTAGE ) {
//						if ( ( myConnections.CheckTerminalMinMaxVoltages(SOURCE | DRAIN, false) &&
//									(abs(myConnections.maxSourceVoltage - myConnections.minDrainVoltage) > model_pit->maxVds ||
//										abs(myConnections.minSourceVoltage - myConnections.maxDrainVoltage) > model_pit->maxVds ) ) ||
//								( myConnections.CheckTerminalMinMaxVoltages(SOURCE, false) && ! myConnections.CheckTerminalMinMaxVoltages(DRAIN, false) &&
//									max(abs(myConnections.minSourceVoltage), abs(myConnections.maxSourceVoltage)) > model_pit->maxVds ) ||
//								( ! myConnections.CheckTerminalMinMaxVoltages(SOURCE, false) && myConnections.CheckTerminalMinMaxVoltages(DRAIN, false) &&
//									max(abs(myConnections.minDrainVoltage), abs(myConnections.maxDrainVoltage)) > model_pit->maxVds ) ) {
//							if ( myConnections.IsPumpCapacitor() &&
//									abs(myConnections.minSourceVoltage - myConnections.minDrainVoltage) <= model_pit->maxVds &&
//									abs(myConnections.maxSourceVoltage - myConnections.maxDrainVoltage)	<= model_pit->maxVds ) {
//								; // for pumping capacitors only check min-min/max-max differences
//							} else {
//								myErrorExplanation += "Overvoltage Error:Source vs Drain:\n";
//								myErrorFlag = true;
//							}
//						} else if ( ( myConnections.CheckTerminalMinMaxLeakVoltages(SOURCE | DRAIN) &&
//									(abs(myConnections.maxSourceLeakVoltage - myConnections.minDrainLeakVoltage) > model_pit->maxVds ||
//										abs(myConnections.minSourceLeakVoltage - myConnections.maxDrainLeakVoltage) > model_pit->maxVds ) ) ||
//								( myConnections.CheckTerminalMinMaxLeakVoltages(SOURCE) && ! myConnections.CheckTerminalMinMaxLeakVoltages(DRAIN) &&
//									max(abs(myConnections.minSourceLeakVoltage), abs(myConnections.maxSourceLeakVoltage)) > model_pit->maxVds ) ||
//								( ! myConnections.CheckTerminalMinMaxLeakVoltages(SOURCE) && myConnections.CheckTerminalMinMaxLeakVoltages(DRAIN) &&
//									max(abs(myConnections.minDrainLeakVoltage), abs(myConnections.maxDrainLeakVoltage)) > model_pit->maxVds ) ) {
//							myErrorExplanation += "Overvoltage Error:Source vs Drain: (logic ok)\n";
//							myErrorFlag = true;
//							myLeakCheckFlag = true;
//						}
//					}
//					if ( myErrorFlag ) {
//						errorCount[OVERVOLTAGE]++;
//						if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(myConnections.deviceId) < cvcParameters.cvcCircuitErrorLimit ) {
//							errorFile << myErrorExplanation;
//							PrintDeviceWithMinMaxConnections(myParent_p->instanceId_v[instance_it], myConnections, errorFile, myLeakCheckFlag);
//							errorFile << endl;
//						}
//					}
//				}
//				myDevice_p = myDevice_p->nextDevice_p;
//			}
//		}
//	}
//	cvcCircuitList.PrintAndResetCircuitErrors(cvcParameters.cvcCircuitErrorLimit, errorFile);
//	errorFile << "! Finished" << endl << endl;
//}

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
						if ( IsPower_(netVoltagePtr_v[myConnections.gateId]) && IsPower_(netVoltagePtr_v[myConnections.drainId]) ) continue;  // ignore direct power capacitors
						if ( ! IsInputOrPower_(netVoltagePtr_v[myConnections.masterMinSourceNet.finalNetId])
								|| ! IsInputOrPower_(netVoltagePtr_v[myConnections.masterMinGateNet.finalNetId]) ) continue;  // ignore capacitors connected to non-input/power nets
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
	cvcCircuitList.PrintAndResetCircuitErrors(this, cvcParameters.cvcCircuitErrorLimit, logFile, errorFile, "! Checking nmos gate vs source errors: ");
}

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
						if ( IsPower_(netVoltagePtr_v[myConnections.gateId]) && IsPower_(netVoltagePtr_v[myConnections.drainId]) ) continue;  // ignore direct power capacitors
						if ( ! IsInputOrPower_(netVoltagePtr_v[myConnections.masterMinSourceNet.finalNetId])
								|| ! IsInputOrPower_(netVoltagePtr_v[myConnections.masterMinGateNet.finalNetId]) ) continue;  // ignore capacitors connected to non-input/power nets
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
//					if ( cvcParameters.cvcLogicDiodes && myDiodeConnections.simSourceVoltage != UNKNOWN_VOLTAGE ) {
//						mySourceVoltage = myDiodeConnections.simSourceVoltage;
//					} else {
						mySourceVoltage = myDiodeConnections.maxSourceVoltage;
//					}
				}
				if ( myDrainVoltage == UNKNOWN_VOLTAGE ) {
//					if ( cvcParameters.cvcLogicDiodes && myDiodeConnections.simDrainVoltage != UNKNOWN_VOLTAGE ) {
//						myDrainVoltage = myDiodeConnections.simDrainVoltage;
//					} else {
						myDrainVoltage = myDiodeConnections.minDrainVoltage;
//					}
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
//						CCircuit * myParent_p = instancePtr_v[deviceParent_v[device_it]]->master_p;
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
			if ( SimVoltage(net_it) != UNKNOWN_VOLTAGE || (netVoltagePtr_v[net_it] && netVoltagePtr_v[net_it]->type[INPUT_BIT]) ) continue;
			if ( HasActiveConnections(net_it) ) continue;
			MapDeviceNets(firstGate_v[net_it], myConnections);
			if ( IsFloatingGate(myConnections) ) continue;  // Already processed previously
			for ( deviceId_t device_it = firstGate_v[net_it]; device_it != UNKNOWN_DEVICE; device_it = nextGate_v[device_it] ) {
				MapDeviceNets(device_it, myConnections);
				bool myHasLeakPath = HasLeakPath(myConnections);
				if ( myHasLeakPath ) {
	//						CCircuit * myParent_p = instancePtr_v[deviceParent_v[device_it]]->master_p;
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
//	errorFile << "! Finished" << endl << endl;
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
				if ( ( mySimNetId == UNKNOWN_NET || ! netVoltagePtr_v[mySimNetId] || netVoltagePtr_v[mySimNetId]->simVoltage == UNKNOWN_VOLTAGE ) &&
						( myMinNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMinNetId] || netVoltagePtr_v[myMinNetId]->minVoltage == UNKNOWN_VOLTAGE || netVoltagePtr_v[myMinNetId]->type[HIZ_BIT] ||
							myMaxNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMaxNetId] || netVoltagePtr_v[myMaxNetId]->maxVoltage == UNKNOWN_VOLTAGE || netVoltagePtr_v[myMaxNetId]->type[HIZ_BIT]) ) { // open match
					myExpectedValueFound = true;
				}
			} else if ( mySimNetId != UNKNOWN_NET && netVoltagePtr_v[mySimNetId] && netVoltagePtr_v[mySimNetId]->simVoltage != UNKNOWN_VOLTAGE ) {
				if ( IsValidVoltage_((*power_ppit)->expectedSim())
						&& abs(String_to_Voltage((*power_ppit)->expectedSim()) - netVoltagePtr_v[mySimNetId]->simVoltage) <= cvcParameters.cvcExpectedErrorThreshold ) { // voltage match
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedSim() == NetName(mySimNetId) ) { // name match
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedSim() == string(netVoltagePtr_v[mySimNetId]->powerAlias()) ) { // alias match
					myExpectedValueFound = true;
				}
			}
			if ( ! myExpectedValueFound ) {
				errorCount[EXPECTED_VOLTAGE]++;
				errorFile << "Expected " << NetName(myNetId) << " = " << (*power_ppit)->expectedSim() << " but found ";
				if ( mySimNetId != UNKNOWN_NET && netVoltagePtr_v[mySimNetId] && netVoltagePtr_v[mySimNetId]->simVoltage != UNKNOWN_VOLTAGE ) {
					errorFile << NetName(mySimNetId) << "@" << PrintVoltage(netVoltagePtr_v[mySimNetId]->simVoltage) << endl;
				} else if ( (*power_ppit)->expectedSim() == "open" ) {
					bool myPrintedReason = false;
					if ( myMinNetId != UNKNOWN_NET && netVoltagePtr_v[myMinNetId] && netVoltagePtr_v[myMinNetId]->minVoltage != UNKNOWN_VOLTAGE ) {
						errorFile << "(min)" << NetName(myMinNetId) << "@" << PrintVoltage(netVoltagePtr_v[myMinNetId]->minVoltage) << " ";
						myPrintedReason = true;
					}
					if ( myMaxNetId != UNKNOWN_NET && netVoltagePtr_v[myMaxNetId] && netVoltagePtr_v[myMaxNetId]->maxVoltage != UNKNOWN_VOLTAGE ) {
						errorFile << "(max)" << NetName(myMaxNetId) << "@" << PrintVoltage(netVoltagePtr_v[myMaxNetId]->maxVoltage);
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
				if ( myMinNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMinNetId] || netVoltagePtr_v[myMinNetId]->simVoltage == UNKNOWN_VOLTAGE ) { // open match
					myExpectedValueFound = true;
				}
			} else if ( myMinNetId != UNKNOWN_NET && netVoltagePtr_v[myMinNetId] ) {
				if ( String_to_Voltage((*power_ppit)->expectedMin()) <= netVoltagePtr_v[myMinNetId]->minVoltage ) { // voltage match (anything higher than expected is ok)
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedMin() == NetName(myMinNetId) ) { // name match
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedMin() == string(netVoltagePtr_v[myMinNetId]->powerAlias()) ) { // alias match
					myExpectedValueFound = true;
				}
			}
			if ( ! myExpectedValueFound ) {
				errorCount[EXPECTED_VOLTAGE]++;
				errorFile << "Expected minimum " << NetName(myNetId) << " = " << (*power_ppit)->expectedMin() << " but found ";
				if ( myMinNetId != UNKNOWN_NET && netVoltagePtr_v[myMinNetId] && netVoltagePtr_v[myMinNetId]->minVoltage != UNKNOWN_VOLTAGE ) {
					errorFile << NetName(myMinNetId) << "@" << PrintVoltage(netVoltagePtr_v[myMinNetId]->minVoltage) << endl;
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
				if ( myMaxNetId == UNKNOWN_NET || ! netVoltagePtr_v[myMaxNetId] || netVoltagePtr_v[myMaxNetId]->simVoltage == UNKNOWN_VOLTAGE ) { // open match
					myExpectedValueFound = true;
				}
			} else if ( myMaxNetId != UNKNOWN_NET && netVoltagePtr_v[myMaxNetId] ) {
				if ( String_to_Voltage((*power_ppit)->expectedMax()) >= netVoltagePtr_v[myMaxNetId]->maxVoltage ) { // voltage match (anything lower than expected is ok)
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedMax() == NetName(myMaxNetId) ) { // name match
					myExpectedValueFound = true;
				} else if ( (*power_ppit)->expectedMax() == string(netVoltagePtr_v[myMaxNetId]->powerAlias()) ) { // alias match
					myExpectedValueFound = true;
				}
			}
			if ( ! myExpectedValueFound ) {
				errorCount[EXPECTED_VOLTAGE]++;
				errorFile << "Expected maximum " << NetName(myNetId) << " = " << (*power_ppit)->expectedMax() << " but found ";
				if ( myMaxNetId != UNKNOWN_NET && netVoltagePtr_v[myMaxNetId] && netVoltagePtr_v[myMaxNetId]->maxVoltage != UNKNOWN_VOLTAGE ) {
					errorFile << NetName(myMaxNetId) << "@" << PrintVoltage(netVoltagePtr_v[myMaxNetId]->maxVoltage) << endl;
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

/*
	void CCvcDb::ReportBadLddConnection(CEventQueue & theEventQueue, deviceId_t theDeviceId) {
		static CFullConnection myConnections;

		MapDeviceNets(theDeviceId, myConnections);
		errorCount[LDD_SOURCE]++;
		if ( cvcParameters.cvcCircuitErrorLimit == 0 || IncrementDeviceError(theDeviceId) < cvcParameters.cvcCircuitErrorLimit ) {
			errorFile << "! LDD MOS drain->source connection: ";
			if ( theEventQueue.queueType == MIN_QUEUE ) {
				errorFile << PrintVoltage(myConnections.minDrainVoltage) << "->" << PrintVoltage(myConnections.minSourceVoltage) << endl;
				PrintDeviceWithMinConnections(deviceParent_v[theDeviceId], myConnections, errorFile);
			} else if ( theEventQueue.queueType == MAX_QUEUE ) {
				errorFile << PrintVoltage(myConnections.maxDrainVoltage) << "->" << PrintVoltage(myConnections.maxSourceVoltage) << endl;
				PrintDeviceWithMaxConnections(deviceParent_v[theDeviceId], myConnections, errorFile);
			}
			errorFile << endl;
		}
	}
*/


