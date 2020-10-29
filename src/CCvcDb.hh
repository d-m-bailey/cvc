/*
 * CCvcDb.hh
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

#ifndef CCVCDB_HH_
#define CCVCDB_HH_

#include "Cvc.hh"

class CCvcDb;
#include "CInstance.hh"
#include "CCircuit.hh"
#include "CModel.hh"
#include "CPower.hh"
#include "CEventQueue.hh"
#include "CVirtualNet.hh"
#include "CCvcParameters.hh"
#include "CConnectionCount.hh"
#include "CConnection.hh"
#include "CDependencyMap.hh"
#include "gzstream.h"

extern char RESISTOR_TEXT[];
extern CNetIdSet EmptySet;

class CShortVector : public vector<pair<netId_t, string> > {
public:
};

class CNetMap : public unordered_map<netId_t, forward_list<netId_t>> {
public:
	CNetMap(float theLoadFactor = DEFAULT_LOAD_FACTOR) {max_load_factor(theLoadFactor);}
};

class CCvcDb {
public:
	int	cvcArgIndex = 1;
	int	cvcArgCount;

	bool detectErrorFlag;  //!< skip error processing if false

	CCvcParameters	cvcParameters;
	CCircuitPtrList	cvcCircuitList;

	CCircuit *	topCircuit_p;
	//CShortVector	short_v;

	CInstancePtrVector instancePtr_v;

	// parent instance.	 Use offset from first in instance to find name, etc.
	// [*] = instance
	CInstanceIdVector	netParent_v;
//	CInstanceIdVector	subcircuitParent;
	CInstanceIdVector	deviceParent_v;

	// [device] = device
	CDeviceIdVector	nextSource_v;
	CDeviceIdVector	nextGate_v;
	CDeviceIdVector	nextDrain_v;
//	CDeviceIdVector	nextBulk_v;

	// [net] = device
	CDeviceIdVector	firstSource_v;
	CDeviceIdVector	firstGate_v;
	CDeviceIdVector	firstDrain_v;
//	CDeviceIdVector	firstBulk_v;

	// [device] = net
	CNetIdVector	sourceNet_v;
	CNetIdVector	gateNet_v;
	CNetIdVector	drainNet_v;
	CNetIdVector	bulkNet_v;

	vector<modelType_t> deviceType_v;

	// [device] = status
	CStatusVector	deviceStatus_v;
	// [net] = status
	CStatusVector	netStatus_v;

	// [net] = virtualNet
	CVirtualNetVector	minNet_v;
	CVirtualNetVector	simNet_v;
	CVirtualNetVector	maxNet_v;
//	CVirtualNetVector	initialMaxNet_v;
//	CVirtualNetVector	initialMinNet_v;
//	CVirtualNetMappedVector	initialSimNet_v;
//	CVirtualNetVector	logicMaxNet_v;
//	CVirtualNetVector	logicMinNet_v;
//	CVirtualNetVector	logicSimNet_v;
//	CVirtualNetMappedVector	maxLeakNet_v;
//	CVirtualNetMappedVector	minLeakNet_v;
//	CVirtualNetVector	fixedMaxNet_v;
//	CVirtualNetVector	fixedMinNet_v;
//	CVirtualNetVector	fixedSimNet_v;

	bool isDeviceModelSet = false;
	bool isFixedMinNet, isFixedSimNet, isFixedMaxNet;
	bool leakVoltageSet;
	bool isValidShortData;
	returnCode_t modelFileStatus, powerFileStatus, fuseFileStatus;

	// [net] = CConnection
	CConnectionCountVector	connectionCount_v;

	// [net] = CPower
	CPowerPtrVector	netVoltagePtr_v;
	CPowerPtrVector	leakVoltagePtr_v;
	CPowerPtrVector	initialVoltagePtr_v;
//	CPowerPtrVector	logicVoltagePtr_v;

	bool	isFixedEquivalentNet;
	CNetIdVector	equivalentNet_v;
	CNetIdVector	inverterNet_v; // inverterNet_v[1] = 2 means 2 -|>o- 1
	vector<bool>	highLow_v;

	// [powerIndex] = CPower
//	CPowerPtrVector powerPtr_v;

	// [net] = resistance
//	vector<resistance_t>	maximumNetResistance;
//	vector<resistance_t>	minimumNetResistance;
//	vector<resistance_t>	simulationNetResistance;

	CEventQueue	maxEventQueue;
	CEventQueue minEventQueue;
	CEventQueue simEventQueue;

	unordered_set<deviceId_t> mosDiodeSet;
	unordered_set<string> unknownGateSet;

//	vector<voltage_t>	voltage;

	uintmax_t	deviceCount;
	uintmax_t	subcircuitCount;
	uintmax_t	netCount;

	unsigned int	lineLength = 0;

//	CTextModelPtrMap parameterModelPtrMap;
	CTextResistanceMap	parameterResistanceMap;

	voltage_t	minPower = MAX_VOLTAGE;
	voltage_t	maxPower = MIN_VOLTAGE;

	size_t	errorCount[ERROR_TYPE_COUNT];

	CDependencyMap minConnectionDependencyMap;
	CDependencyMap maxConnectionDependencyMap;

	map<netId_t, string> calculatedResistanceInfo_v;

	unordered_map<string, deviceId_t> cellErrorCountMap;

	forward_list<string> inverterInputOutputCheckList;  // list of nets to check for matched input/output
	forward_list<pair<string, string>> oppositeLogicList;  // list of nets to check for opposite logic

	ogzstream errorFile;
	ofstream logFile;
	teestream reportFile;  // simultaneous output to stdout and logFile
	ogzstream debugFile;

	string lockFile;
	string reportPrefix;

	typedef struct mos_data {
		netId_t gate;
		netId_t source;
		deviceId_t id;
	} mosData_t;

	// CCvcDb_main.cc
	/// Main Loop: Verify circuits using settings in each verification resource file.
	void VerifyCircuitForAllModes(int argc, const char * argv[]);

	// CCvcDb-init.cc
	CCvcDb(int argc, const char * argv[]);
	~CCvcDb();
	void CountObjectsAndLinkSubcircuits();

	void AssignGlobalIDs();
	void ResetMinSimMaxAndQueues();
	CPower * SetMasterPower(netId_t theFirstNet, netId_t theSecondNet, bool & theSamePowerFlag);
	netId_t MasterPowerNet(netId_t theFirstNetId, netId_t theSecondNetId);
	void MakeEquivalentNets(CNetMap & theNetMap, netId_t theFirstNetId, netId_t theSecondNetId, deviceId_t theDeviceId);
	void SetEquivalentNets();
	void LinkDevices();
	returnCode_t SetDeviceModels();
	void DumpConnectionList(string theHeading, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v);

	void DumpConnectionLists(string theHeading);

	void MergeConnectionListByTerminals(netId_t theFromNet, netId_t theToNet, deviceId_t theIgnoreDeviceId,
			CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v, CNetIdVector& theTerminal_v);
	deviceId_t RecountConnections(netId_t theNetId, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v);
	void MergeConnectionLists(netId_t theFromNet, netId_t theToNet, deviceId_t theIgnoreDeviceId);
//	void ResetMosFuse();
	void OverrideFuses();

	deviceId_t FindUniqueSourceDrainConnectedDevice(netId_t theNetId);
	void ShortNonConductingResistor(deviceId_t theDeviceId, netId_t theFirstNet, netId_t theSecondNet, shortDirection_t theDirection);
	void ShortNonConductingResistors();
	void SetResistorVoltagesForMosSwitches();
	forward_list<instanceId_t> FindInstanceIds(string theHierarchy, instanceId_t theParent = 0);
	set<netId_t> * FindUniqueNetIds(string thePowerSignal, instanceId_t theParent = 0);
	forward_list<netId_t> * FindNetIds(string thePowerSignal, instanceId_t theParent = 0);
	returnCode_t SetModePower();
	returnCode_t SetInstancePower();
	returnCode_t SetExpectedPower();
	bool LockReport(bool theInteractiveFlag);
	void RemoveLock();
	void SetSCRCPower();
	size_t SetSCRCGatePower(netId_t theNetId, CDeviceIdVector & theFirstSource_v, CDeviceIdVector & theNextSource_v, CNetIdVector & theDrain_v,
			size_t & theSCRCSignalCount, size_t & theSCRCIgnoreCount, bool theNoCheckFlag);
	void SetSCRCParentPower(netId_t theNetId, deviceId_t theDeviceId, bool theExpectedHighInput, size_t & theSCRCSignalCount, size_t & theSCRCIgnoreCount);
	bool IsSCRCLogicNet(netId_t theNetId);
	bool IsSCRCPower(CPower * thePower_p);
	bool SetLatchPower(int thePassCount, vector<bool> & theIgnoreNet_v, CNetIdSet & theNewNetSet);
	void FindLatchDevices(netId_t theNetId, mosData_t theNmosData_v[], mosData_t thePmosData_v[], int & theNmosCount, int & thePmosCount,
		voltage_t theMinVoltage, voltage_t theMaxVoltage,
		CDeviceIdVector & theFirstDrain_v, CDeviceIdVector & theNextDrain_v, CNetIdVector & theSourceNet_v);
	bool IsOppositeLogic(netId_t theFirstNet, netId_t theSecondNet);
	void PrintInputNetsWithMinMaxSuggestions(netId_t theNetId);
	void PrintNetSuggestions();
	returnCode_t	LoadCellErrorLimits();
	void LoadCellChecksums();
	void LoadNetChecks();

	// error
	void PrintFuseError(netId_t theTargetNetId, CConnection & theConnections);
	void PrintMinVoltageConflict(netId_t theTargetNetId, CConnection & theMinConnections, voltage_t theExpectedVoltage, float theLeakCurrent);
	void PrintMaxVoltageConflict(netId_t theTargetNetId, CConnection & theMaxConnections, voltage_t theExpectedVoltage, float theLeakCurrent);
	//string FindVbgError(voltage_t theParameter, CFullConnection & theConnections);
	//string FindVbsError(voltage_t theParameter, CFullConnection & theConnections);
	//string FindVdsError(voltage_t theParameter, CFullConnection & theConnections);
	//string FindVgsError(voltage_t theParameter, CFullConnection & theConnections);
	void FindVbgError(ogzstream & theErrorFile, voltage_t theParameter, CFullConnection & theConnections, instanceId_t theInstance_p, string theDisplayParameter);
	void FindVbsError(ogzstream & theErrorFile, voltage_t theParameter, CFullConnection & theConnections, instanceId_t theInstance_p, string theDisplayParameter);
	void FindVdsError(ogzstream & theErrorFile, voltage_t theParameter, CFullConnection & theConnections, instanceId_t theInstance_p, string theDisplayParameter);
	void FindVgsError(ogzstream & theErrorFile, voltage_t theParameter, CFullConnection & theConnections, instanceId_t theInstance_p, string theDisplayParameter);
	void PrintOverVoltageError(ogzstream & theErrorFile, CFullConnection & theConnections, cvcError_t theErrorIndex, string theExplanation, instanceId_t theInstance_p);
	void FindAllOverVoltageErrors();
	//void FindOverVoltageErrors(string theCheck, int theErrorIndex);
	void AppendErrorFile(string theTempFileName, string theHeading, int theErrorSubIndex);
	void FindNmosGateVsSourceErrors();
	void FindPmosGateVsSourceErrors();
	void FindNmosSourceVsBulkErrors();
	void FindPmosSourceVsBulkErrors();
	void FindForwardBiasDiodes();
	void FindNmosPossibleLeakErrors();
	void FindPmosPossibleLeakErrors();
	void FindFloatingInputErrors();
	void CheckExpectedValues();
	void FindLDDErrors();
	void CheckInverterIO(modelType_t theType);
	void CheckOppositeLogic(modelType_t theType);

	//
//	void ReportBadLddConnection(CEventQueue & theEventQueue, deviceId_t theDeviceId);
	void ReportSimShort(deviceId_t theDeviceId, voltage_t theMainVoltage, voltage_t theShortVoltage, string theCalculation);
	void ReportShort(deviceId_t theDeviceId);
	bool VoltageConflict(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections);
	bool BulkError(eventQueue_t theQueueType, CDevice * theDevice_p, netId_t theBulkId, voltage_t theVoltage);
	bool LastNmosConnection(deviceStatus_t thePendingBit, netId_t theNetId);
	bool LastPmosConnection(deviceStatus_t thePendingBit, netId_t theNetId);
	bool IsIrrelevant(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, voltage_t theVoltage, queueStatus_t theStatus);
	string AdjustMaxPmosKey(CConnection& theConnections, netId_t theDrainId, voltage_t theMaxSource, eventKey_t& theEventKey, queuePosition_t& theQueuePosition);
	string AdjustMaxNmosKey(CConnection& theConnections, voltage_t theSimGate, voltage_t theMaxSource, eventKey_t& theEventKey, queuePosition_t& theQueuePosition,
			bool theWarningFlag);
	string AdjustMinNmosKey(CConnection& theConnections, netId_t theDrainId, voltage_t theMinSource, eventKey_t& theEventKey, queuePosition_t& theQueuePosition);
	string AdjustMinPmosKey(CConnection& theConnections, voltage_t theSimGate, voltage_t theMinSource, eventKey_t& theEventKey, queuePosition_t& theQueuePosition,
			bool theWarningFlag);
	string AdjustKey(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, eventKey_t& theEventKey, queuePosition_t& theQueuePosition,
			shortDirection_t theDirection, bool theWarningFlag);
	string AdjustSimVoltage(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, voltage_t& theVoltage, shortDirection_t theDirection,
			propagation_t thePropagationType);
//	bool NeedsSwap(eventQueue_t theQueueType, modelType_t theModelType, shortDirection_t theDirection);
//	void SwapSourceDrain(CDevice * theDevice_p);
//	void CheckSourceDrain(eventQueue_t theQueueType, CConnection& theConnections, shortDirection_t theDirection);
	bool TopologicallyOffMos(eventQueue_t theQueueType, modelType_t theModelType, CConnection& theConnections);

	bool IsOffMos(eventQueue_t theQueueType, deviceId_t theDeviceId, CConnection& theConnections, voltage_t theVoltage);
	void EnqueueAttachedDevicesByTerminal(CEventQueue& theEventQueue, netId_t theNetId, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v, eventKey_t theEventKey);
	void EnqueueAttachedDevices(CEventQueue& theEventQueue, netId_t theNetId, eventKey_t theEventKey);
	void PropagateMinMaxVoltages(CEventQueue& theEventQueue);
	bool CheckEstimateDependency(CDependencyMap& theDependencyMap, size_t theEstimateType, list<netId_t>& theDependencyList);
	void CheckEstimateDependencies();
	void SetTrivialMinMaxPower();
	void ResetMinMaxActiveStatus();
	void SetInitialMinMaxPower();
	void ShiftVirtualNets(CEventQueue& theEventQueue, netId_t theNetId, CVirtualNet& theLastVirtualNet, resistance_t theNewResistance, resistance_t theOldResistance);
	void RecalculateFinalResistance(CEventQueue& theEventQueue, netId_t theNewNetId, bool theRecursingFlag = false);


	void EnqueueAttachedResistorsByTerminal(CEventQueue& theEventQueue, netId_t theNetId, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v, eventKey_t theEventKey, queuePosition_t theQueuePosition);
	void EnqueueAttachedResistors(CEventQueue& theEventQueue, netId_t theNetId, eventKey_t theEventKey, queuePosition_t theQueuePosition);
	bool CheckConnectionReroute(CEventQueue& theEventQueue, CConnection& theConnections, shortDirection_t theDirection);
	bool IsPriorityDevice(CEventQueue& theEventQueue, modelType_t theModel);
	void AlreadyShorted(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections);
	void ShortNets(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, shortDirection_t theDirection);
	void ShortNets(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, shortDirection_t theDirection, voltage_t theVoltage, string theCalculation);
	void ShortSimNets(CEventQueue& theEventQueue, deviceId_t theDeviceId, CConnection& theConnections, shortDirection_t theDirection, voltage_t theVoltage, string theCalculation);
	void PropagateResistorVoltages(CEventQueue& theEventQueue);
	void PropagateConnectionType(CVirtualNetVector& theVirtualNet_v, CStatus theSourceDrainType, netId_t theNetId);
	void PropagateSimVoltages(CEventQueue& theEventQueue, propagation_t thePropagationType);
	void CalculateResistorVoltage(netId_t theNetId, voltage_t theMinVoltage, resistance_t theMinResistance,
			voltage_t theMaxVoltage, resistance_t theMaxResistance );
	void PropagateResistorCalculations(netId_t theNetId, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v);
	void CalculateResistorVoltages();
	void SetResistorVoltagesByPower();
	void ResetMinMaxPower();
	void SetAnalogNets();
	void PropagateAnalogNetType(netId_t theNetId, int theGateCount);
	void PropagateAnalogNetTypeByTerminal(netId_t theNetId, CDeviceIdVector& theFirstDevice_v, CDeviceIdVector& theNextDevice_v, int theGateCount);
	void IgnoreUnusedDevices();
	void SetSimPower(propagation_t thePropagationType, CNetIdSet & theNewNetSet = EmptySet);

	void SetInverterHighLow(netId_t theNetId, netId_t theMaxNetId);
	netId_t SetInverterInput(netId_t theNetId, netId_t theMaxNetId);
	void SetInverters();

	// CCvcDb-utility
	voltage_t MinVoltage(netId_t theNetId, bool theSkipHiZFlag = false);
	voltage_t MinSimVoltage(netId_t theNetId);
	resistance_t MinResistance(netId_t theNetId);
	voltage_t MinLeakVoltage(netId_t theNetId);
	voltage_t SimVoltage(netId_t theNetId);
	bool IsAlwaysOnCandidate(deviceId_t theDeviceId, shortDirection_t theDirection);
	resistance_t SimResistance(netId_t theNetId);
	voltage_t MaxVoltage(netId_t theNetId, bool theSkipHiZFlag = false);
	voltage_t MaxSimVoltage(netId_t theNetId);
	resistance_t MaxResistance(netId_t theNetId);
	voltage_t MaxLeakVoltage(netId_t theNetId);
	netId_t GetGreatestEquivalentNet(netId_t theNetId);
	netId_t GetLeastEquivalentNet(netId_t theNetId);
	netId_t GetEquivalentNet(netId_t theNetId);
	list<string> * SplitHierarchy(string theFullPath);
	void SaveMinMaxLeakVoltages();
	void SaveInitialVoltages();
//	void SaveLogicVoltages();
	list<string> * ExpandBusNet(string theBusName);

	text_t DeviceParameters(const deviceId_t theDeviceId);

	void SetDeviceNets(CInstance * theInstance_p, CDevice * theDevice_p, netId_t& theSourceId, netId_t& theGateId, netId_t& theDrainId, netId_t& theBulkId);
	void SetDeviceNets(deviceId_t theDeviceId, CDevice * theDevice_p, netId_t& theSourceId, netId_t& theGateId, netId_t& theDrainId, netId_t& theBulkId);
	void MapDeviceNets(deviceId_t theDeviceId, CEventQueue& theEventQueue, CConnection& theConnections);
	void MapDeviceNets(deviceId_t theDeviceId, CFullConnection& theConnections);
	void MapDeviceSourceDrainNets(deviceId_t theDeviceId, CFullConnection& theConnections);
	void MapDeviceNets(CInstance * theInstance_p, CDevice * theDevice_p, CFullConnection& theConnections);

	void IgnoreDevice(deviceId_t theDeviceId);
	bool EqualMasterNets(CVirtualNetVector& theVirtualNet_v, netId_t theFirstNetId, netId_t theSecondNetId);
	bool GateEqualsDrain(CConnection& theConnections);
	inline bool IsFloatingGate(CFullConnection& myConnections) { return (myConnections.minGateVoltage == UNKNOWN_VOLTAGE || myConnections.minGatePower_p->type[HIZ_BIT]
		|| myConnections.maxGateVoltage == UNKNOWN_VOLTAGE || myConnections.maxGatePower_p->type[HIZ_BIT]); };
	inline bool IsVerifiedPower(netId_t theNetId) {return ! (netStatus_v[theNetId][NEEDS_MIN_CHECK] || netStatus_v[theNetId][NEEDS_MIN_CONNECTION]
		|| netStatus_v[theNetId][NEEDS_MAX_CHECK] || netStatus_v[theNetId][NEEDS_MAX_CONNECTION]); }
	voltage_t DefaultMinVoltage(CPower * thePower_p);
	voltage_t DefaultMaxVoltage(CPower * thePower_p);
	bool HasLeakPath(CFullConnection& theConnections);
	void RestoreQueue(CEventQueue& theBaseEventQueue, CEventQueue& theSavedEventQueue, deviceStatus_t theStatusBit);
	void CheckConnections();
	bool PathContains(CVirtualNetVector& theSearchVector, netId_t theSearchNet, netId_t theTargetNet);
	bool PathCrosses(CVirtualNetVector& theSearchVector, netId_t theSearchNet, CVirtualNetVector& theTargetVector, netId_t theTargetNet);
	bool HasActiveConnection(netId_t theNet);
	size_t IncrementDeviceError(deviceId_t theDeviceId, int theErrorIndex);
	eventKey_t SimKey(eventKey_t theCurrentKey, resistance_t theIncrement);
	bool IsDerivedFromFloating(CVirtualNetVector& theVirtualNet_v, netId_t theNetId);
	bool HasActiveConnections(netId_t theNetId);
	size_t InstanceDepth(instanceId_t theInstanceId);
	bool IsSubcircuitOf(instanceId_t theInstanceId, instanceId_t theParentId);
	void RemoveInvalidPower(netId_t theNetId, size_t & theRemovedCount);
	calculationType_t GetCalculationType(CPower * thePower_p, eventQueue_t theQueueType);
	deviceId_t GetSeriesConnectedDevice(deviceId_t theDeviceId, netId_t theNetId);
	void Cleanup();
	deviceId_t CountBulkConnections(netId_t theNetId);
	bool IsAnalogNet(netId_t theNetId);
	bool IsAlwaysOff(CFullConnection& theConnections);
	bool IsOneConnectionNet(netId_t theNetId);
	void SetDiodeConnections(pair<int, int> diode_pit, CFullConnection & myConnections, CFullConnection & myDiodeConnections);
	int CalculateMFactor(instanceId_t theInstanceId);
	deviceId_t GetAttachedDevice(netId_t theNetId, modelType_t theType, terminal_t theTerminal);
	deviceId_t FindInverterDevice(netId_t theInputNet, netId_t theOutputNet, modelType_t theType);
	returnCode_t FindUniqueMosInputs(netId_t theOutputNet, netId_t theGroundNet, netId_t thePowerNet,
		CDeviceIdVector &theFirst_v, CDeviceIdVector &theNext_v, CNetIdVector &theSourceNet_v, CNetIdVector &theDrainNet_v,
		netId_t &theNmosInput, netId_t &thePmosInput);
	deviceId_t FindInverterInput(netId_t theOutputNet);
	bool IsOnGate(deviceId_t theDevice, CPower * thePower_p);
	netId_t OppositeNet(deviceId_t theDevice, netId_t theNet);
	deviceId_t GetNextInSeries(deviceId_t theDevice, netId_t theNet);
	bool IsInstanceNet(netId_t theNetId, instanceId_t theInstance);
	instanceId_t FindNetInstance(netId_t theNetId, instanceId_t theInstance);
	bool IsInternalNet(netId_t theNetId, instanceId_t theInstance);
	text_t GetLocalNetName(instanceId_t theInstance, netId_t theNet);

	// CCvcDb-print
	void SetOutputFiles(string theReportFile);
	string NetAlias(netId_t theNetId, bool thePrintCircuitFlag);
	string NetName(CPower * thePowerPtr, bool thePrintCircuitFlag, bool thePrintHierarchyFlag = PRINT_HIERARCHY_ON);
	string NetName(const netId_t theNetId, bool thePrintCircuitFlag = PRINT_CIRCUIT_OFF, bool thePrintHierarchyFlag = PRINT_HIERARCHY_ON);
	string HierarchyName(const instanceId_t theInstanceId, bool thePrintCircuitFlag = PRINT_CIRCUIT_OFF, bool thePrintHierarchyFlag = PRINT_HIERARCHY_ON);
	string DeviceName(const deviceId_t theDeviceId, bool thePrintCircuitFlag = PRINT_CIRCUIT_OFF, bool thePrintHierarchyFlag = PRINT_HIERARCHY_ON);
	string DeviceName(string theName, const instanceId_t theParentId, bool thePrintCircuitFlag = PRINT_CIRCUIT_OFF, bool thePrintHierarchyFlag = PRINT_HIERARCHY_ON);

	void PrintEquivalentNets(string theIndentation);
	void PrintInverterNets(string theIndentation);
	void PrintFlatCdl();
	void PrintHierarchicalCdl(CCircuit *theCircuit, unordered_set<text_t> & thePrintedList, ostream & theCdlFile);
	void PrintNewCdlLine(const string theData, ostream & theOutput = cout);
	void PrintNewCdlLine(const text_t theData, ostream & theOutput = cout);
	void PrintNewCdlLine(const char theData, ostream & theOutput = cout);
	void PrintSourceDrainConnections(CStatus& theConnectionStatus, string theIndentation);
	void PrintConnections(deviceId_t theDeviceCount, deviceId_t theDeviceId, CDeviceIdVector& theNextDeviceId_v, string theIndentation = "", string theHeading = "Connections>");
	void PrintBulkConnections(netId_t theNetId, string theIndentation, string theHeading);

	void PrintCdlLine(const string theData, ostream & theOutput = cout, const unsigned int theMaxLength = 80);
	void PrintCdlLine(const char * theData, ostream & theOutput = cout, const unsigned int theMaxLength = 80);
	string StatusString(const CStatus& theStatus);

	void Print(const string theIndentation = "", const string theHeading = "CVC Database");
	void PrintPowerList(ostream & theOutputStream, string theHeading = "", string theIndentation = "");
	template <class TVirtualNetVector> void PrintVirtualNet(TVirtualNetVector& theVirtualNet_v, netId_t theNetId, string theTitle, ostream & theOutputFile);
	void PrintVirtualNets(CVirtualNetVector& theVirtualNet_v, string theTitle = "", string theIndentation = "");
	template <class TVirtualNetVector> void PrintAllVirtualNets(TVirtualNetVector& theMinNet_v, CVirtualNetVector& theSimNet_v, TVirtualNetVector& theMaxNet_v, string theTitle = "", string theIndentation = "", bool theUseLeak = false);
	string PrintVoltage(voltage_t theVoltage);
	string PrintVoltage(voltage_t theVoltage, CPower * thePower_p);

	void PrintDeviceWithAllConnections(instanceId_t theInstanceId, CFullConnection& theConnections, ogzstream& theErrorFile, bool theIncludeLeakVoltage = false);
//	void PrintDeviceWithMinMaxConnections(instanceId_t theInstanceId, CFullConnection& theConnections, ogzstream& theErrorFile, bool theIncludeLeakVoltage = false);
//	void PrintDeviceWithMinSimConnections(instanceId_t theInstanceId, CFullConnection& theConnections, ogzstream& theErrorFile);
//	void PrintDeviceWithSimMaxConnections(instanceId_t theInstanceId, CFullConnection& theConnections, ogzstream& theErrorFile);
//	void PrintDeviceWithMinConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile);
//	void PrintDeviceWithMaxConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile);
	void PrintDeviceWithSimConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile);
//	void PrintDeviceWithMinGateAndSimConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile);
//	void PrintDeviceWithMaxGateAndSimConnections(instanceId_t theParentId, CFullConnection& theConnections, ogzstream& theErrorFile);

	void PrintAllTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile, bool theIncludeLeakVoltage = false);
	//void PrintMinMaxTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile, bool theIncludeLeakVoltage = false);
	//void PrintMinSimTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile);
	//void PrintSimMaxTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile);
	//void PrintMinTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile);
	//void PrintMaxTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile);
	void PrintSimTerminalConnections(terminal_t theTerminal, CFullConnection& theConnections, ogzstream& theErrorFile);
	void PrintErrorTotals();
	//void PrintShortedNets(string theShortFileName);
	string NetVoltageSuffix(string theDelimiter, string theVoltage, resistance_t theResistance, string theLeakVoltage = "");
	void PrintResistorOverflow(netId_t theNet, ofstream& theOutputFile);
	void PrintClassSizes();
	void PrintNetWithModelCounts(netId_t theNetId, int theTerminals);
	void PrintBackupNet(CVirtualNetVector& theVirtualNet_v, netId_t theNetId, string theTitle, ostream& theOutputFile);
	void PrintLargeCircuits();

	// CCvcDb-interactive
	void FindInstances(string theSubcircuit, bool thePrintCircuitFlag);
	void FindNets(string theName, instanceId_t theInstanceId, bool thePrintCircuitFlag);
	void ShowNets(size_t & theNetCount, regex & theSearchPattern, instanceId_t theInstanceId, bool thePrintCircuitFlag);
	CCircuit * FindSubcircuit(string theSubcircuit);
	void PrintSubcircuitCdl(string theSubcircuit);
	instanceId_t FindHierarchy(instanceId_t theCurrentInstanceId, string theHierarchy, bool theAllowPartialMatch = false, bool thePrintUnmatchFlag = true);
	string ShortString(netId_t theNetId, bool thePrintSubcircuitNameFlag);
	string LeakShortString(netId_t theNetId, bool thePrintSubcircuitNameFlag);
	void PrintParallelInstance(instanceId_t theInstanceId, bool thePrintSubcircuitNameFlag);
	void PrintNets(instanceId_t theCurrentInstanceId, string theFilter, bool thePrintSubcircuitNameFlag, bool theIsValidPowerFlag);
	void PrintDevices(instanceId_t theCurrentInstanceId, string theFilter, bool thePrintSubcircuitNameFlag, bool theIsValidModelFlag);
	void PrintInstances(instanceId_t theCurrentInstanceId, string theFilter, bool thePrintSubcircuitNameFlag);
	//void ReadShorts(string theShortFileName);
	netId_t FindNet(instanceId_t theCurrentInstanceId, string theNetName, bool theDisplayErrorFlag = true);
	deviceId_t FindDevice(instanceId_t theCurrentInstanceId, string theDeviceName);
	returnCode_t InteractiveCvc(int theCurrentStage);
	void DumpFuses(string theFileName);
	void DumpAnalogNets(string theFileName, bool thePrintCircuitFlag);
	void DumpUnknownLogicalPorts(instanceId_t theCurrentInstanceId, string theFilter, string theFileName, bool thePrintCircuitFlag);
	void DumpUnknownLogicalNets(string theFileName, bool thePrintCircuitFlag);
	void DumpLevelShifters(string theFileName, bool thePrintCircuitFlag);
	returnCode_t CheckFuses();
	void CreateDebugCvcrcFile(ofstream & theOutputFile, instanceId_t theInstanceId, string theMode, int theCurrentStage);
	void PrintInstancePowerFile(instanceId_t theInstanceId, string thePowerFileName, int theCurrentStage);

};

template <class TVirtualNetVector>
void CCvcDb::PrintVirtualNet(TVirtualNetVector& theVirtualNet_v, netId_t theNetId, string theTitle, ostream& theOutputFile) {
	theOutputFile << theTitle << endl;
	theOutputFile << NetName(theNetId) << endl;
	netId_t myNetId = GetEquivalentNet(theNetId);
	if ( myNetId != theNetId ) cout << "=>" << NetName(myNetId) << endl;
	while ( myNetId != theVirtualNet_v[myNetId].nextNetId ) {
		theOutputFile << "->" << NetName(theVirtualNet_v[myNetId].nextNetId) << " r=" << theVirtualNet_v[myNetId].resistance << endl;
		myNetId = theVirtualNet_v[myNetId].nextNetId;
	}
	if ( netVoltagePtr_v[myNetId].full ) netVoltagePtr_v[myNetId].full->Print(theOutputFile);
	theOutputFile << endl;
}

template <class TVirtualNetVector>
void CCvcDb::PrintAllVirtualNets(TVirtualNetVector& theMinNet_v, CVirtualNetVector& theSimNet_v, TVirtualNetVector& theMaxNet_v, string theTitle, string theIndentation, bool theUseLeak) {
	cout << theIndentation << "VirtualNetVector" << theTitle << "> start " << netCount << endl;
	netId_t myEquivalentNet, myFinalNet;
	for ( netId_t net_it = 0; net_it < netCount; net_it++ ) {
		cout << net_it << ": ";
//		myEquivalentNet = equivalentNet_v[theMinNet_v[net_it].nextNetId];
		myEquivalentNet = GetEquivalentNet(net_it);
		myFinalNet = theMinNet_v[myEquivalentNet].nextNetId;
/*
		if ( netVoltagePtr_v[myEquivalentNet] && netVoltagePtr_v[myEquivalentNet]->baseNetId != UNKNOWN_NET ) {
			myEquivalentNet = netVoltagePtr_v[myEquivalentNet]->baseNetId;
		}
*/
		if ( net_it == myFinalNet ) {
			if ( netVoltagePtr_v[net_it].full && netVoltagePtr_v[net_it].full->minVoltage != UNKNOWN_VOLTAGE ) {
				cout << netVoltagePtr_v[net_it].full->minVoltage;
				if ( theUseLeak && MinLeakVoltage(net_it) != netVoltagePtr_v[net_it].full->minVoltage ) {
					cout << "(" << MinLeakVoltage(net_it) << ")";
				}
			} else {
				cout << "??";
				if ( theUseLeak && MinLeakVoltage(net_it) != UNKNOWN_VOLTAGE ) {
					cout << "(" << MinLeakVoltage(net_it) << ")";
				}
			}
		} else {
			cout << myFinalNet << "@" << theMinNet_v[myEquivalentNet].resistance;
			if ( theUseLeak && MinLeakVoltage(myEquivalentNet) != MinVoltage(myEquivalentNet) ) {
				cout << "(" << MinLeakVoltage(myEquivalentNet) << ")";
			}
		}
		cout << "/";
//		myEquivalentNet = equivalentNet_v[theSimNet_v[net_it].nextNetId];
//		myEquivalentNet = GetEquivalentNet(theSimNet_v[net_it].nextNetId);
		myFinalNet = theSimNet_v[myEquivalentNet].nextNetId;
/*
		if ( netVoltagePtr_v[myEquivalentNet] && netVoltagePtr_v[myEquivalentNet]->baseNetId != UNKNOWN_NET ) {
			myEquivalentNet = netVoltagePtr_v[myEquivalentNet]->baseNetId;
		}
*/
		if ( net_it == myFinalNet ) {
			if ( netVoltagePtr_v[net_it].full && netVoltagePtr_v[net_it].full->simVoltage != UNKNOWN_VOLTAGE ) {
				cout << netVoltagePtr_v[net_it].full->simVoltage;
			} else {
				cout << "??";
			}
		} else {
			cout << myFinalNet << "@" << theSimNet_v[myEquivalentNet].resistance;
		}
		cout << "/";
//		myEquivalentNet = equivalentNet_v[theMaxNet_v[net_it].nextNetId];
//		myEquivalentNet = GetEquivalentNet(theMaxNet_v[net_it].nextNetId);
		myFinalNet = theMaxNet_v[myEquivalentNet].nextNetId;
/*
		if ( netVoltagePtr_v[myEquivalentNet] && netVoltagePtr_v[myEquivalentNet]->baseNetId != UNKNOWN_NET ) {
			myEquivalentNet = netVoltagePtr_v[myEquivalentNet]->baseNetId;
		}
*/
		if ( net_it == myFinalNet ) {
			if ( netVoltagePtr_v[net_it].full && netVoltagePtr_v[net_it].full->maxVoltage != UNKNOWN_VOLTAGE ) {
				cout << netVoltagePtr_v[net_it].full->maxVoltage;
				if ( theUseLeak && MaxLeakVoltage(net_it) != netVoltagePtr_v[net_it].full->maxVoltage ) {
					cout << "(" << MaxLeakVoltage(net_it) << ")";
				}
			} else {
				cout << "??";
				if ( theUseLeak && MaxLeakVoltage(net_it) != UNKNOWN_VOLTAGE ) {
					cout << "(" << MaxLeakVoltage(net_it) << ")";
				}
			}
		} else {
			cout << myFinalNet << "@" << theMaxNet_v[myEquivalentNet].resistance;
			if ( theUseLeak && MaxLeakVoltage(myEquivalentNet) != MaxVoltage(myEquivalentNet) ) {
				cout << "(" << MaxLeakVoltage(myEquivalentNet) << ")";
			}
		}
		cout << endl;
	}
	cout << theIndentation << "VirtualNetVector" << theTitle << "> end" << endl;
}

#define CheckResistorOverflow_(theResistance, theId, theLog) if ( theResistance == MAX_RESISTANCE ) PrintResistorOverflow(theId, theLog)

#define RERUN_NG 0
#define RERUN_OK 1

#define SetConnections_(theConnections, theDeviceId) (\
		theConnections.sourceId = sourceNet_v[theDeviceId],\
		theConnections.gateId = gateNet_v[theDeviceId],\
		theConnections.drainId = drainNet_v[theDeviceId],\
		theConnections.bulkId = bulkNet_v[theDeviceId]\
		)

#define ExceedsLeakLimit_(theLeakCurrent) (rint(((theLeakCurrent) - cvcParameters.cvcLeakLimit) * 1e9) / 1e9 > 0)

#endif /* CCVCDB_HH_ */
