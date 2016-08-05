/*
 * cdlParser.yy
 *
 * Copyright 2014-2106 D. Mitch Bailey  d.mitch.bailey at gmail dot com
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

%skeleton "lalr1.cc"
%require "3.0"

%define parser_class_name {CCdlParser}

%define parse.assert

%code requires {
#include "Cvc.hh"

#include "CCircuit.hh"
#include "CDevice.hh"

class CCdlParserDriver;

#define YYDEBUG 1
}

// The parsing context.
%param { CCdlParserDriver& driver } { CCircuitPtrList& cdlCircuitList } { bool theCvcSOI }

%locations
%initial-action
{
// Initialize the initial location.
@$.initialize(&driver.filename);
};

%define parse.trace
%define parse.error verbose

%code
{
#include "CCdlParserDriver.hh"
}

%union { string * StringPtr; }
%union { char * charPtr; }

%token CDL_EOF 0 "END-OF-FILE"
%token SUBCKT "SUBCKT"
%token ENDS "ENDS"
%token <charPtr> SUBCIRCUIT 
%token <charPtr> BIPOLAR 
%token <charPtr> CAPACITOR 
%token <charPtr> DIODE 
%token <charPtr> MOSFET 
%token <charPtr> RESISTOR
%token <charPtr> INDUCTOR
%token <charPtr> STRING
%token EOL "EOL"

%union { void * VoidPtr; }
%type <VoidPtr> circuitList

%union { CCircuit * CircuitPtr; }
%type <CircuitPtr> circuit

%union { CTextList * charPtrListPtr; }
%type <charPtrListPtr> stringList
%type <charPtrListPtr> interface

%union { CDevicePtrList *DevicePtrListPtr; }
%type <DevicePtrListPtr> deviceList

%union { CDevice * DevicePtr; }
%type <DevicePtr> device
%type <DevicePtr> subcircuit
%type <DevicePtr> bipolar
%type <DevicePtr> capacitor
%type <DevicePtr> diode
%type <DevicePtr> inductor
%type <DevicePtr> mosfet
%type <DevicePtr> resistor

/* 
%printer { yyoutput << $$; } <*>;
%printer { yyoutput << "\"" << $$->c_str << "\""; } <StringPtr>;
%printer { yyoutput << "<>"; } <>;
*/

%%

%start circuitList;

circuitList[after]: 
	%empty {
		$after = NULL;
	};
|	circuitList[before] circuit {
		if ( $circuit != NULL ) {
	//		$circuit->Print();
			cdlCircuitList.push_back($circuit);
			cdlCircuitList.circuitNameMap[$circuit->name] = $circuit;
		}
	};

circuit:
	SUBCKT interface ENDS EOL {
/**/
		$circuit = new CCircuit();
		$circuit->name = $interface->front();
		$interface->pop_front();
		$circuit->AddPortSignalIds($interface);
/**/
		delete $interface;
//		$circuit = NULL;
	}
|	SUBCKT interface deviceList ENDS EOL {
		$circuit = new CCircuit();
		$circuit->name = $interface->front();
		$interface->pop_front();
		$circuit->AddPortSignalIds($interface);
		delete $interface;
//		$circuit->devices = $deviceList;
		$circuit->LoadDevices($deviceList);
//		delete $deviceList;
//		for (CDeviceList::iterator current = $circuit->devices->begin(); current != $circuit->devices->end(); current++ ) {
//			current->signalIndexVector = $circuit->SetSignalIndexes(current->signals);
//			delete current->signals;
//		}
		delete $deviceList;
	};

interface:
	stringList EOL;
	
stringList[after]:
	STRING {
		$after = new CTextList();
		$after->push_back($STRING);
	}
|	stringList[before] STRING {
		$before->push_back($STRING);
	};

deviceList[after]:
	device {
//		$after = new CDevicePtrList(1, *$device);
		$after = new CDevicePtrList();
		if ( $device != NULL ) {
			if ( $device->IsSubcircuit() ) {
				$after->subcircuitCount++;
			}
			$after->push_back($device);
		}
	}
|	deviceList[before] device {
		if ( $device != NULL ) {
			if ( $device->IsSubcircuit() ) {
				$before->subcircuitCount++;
			}
			$before->push_back($device);
		}
	};

device:
	subcircuit {}
|	bipolar {}
|	capacitor {}
|	diode {}
|	inductor {}
|	mosfet {}
|	resistor {};

subcircuit:
	SUBCIRCUIT stringList EOL {
//		printf("** NOERROR: found subckt: %s\n", $SUBCIRCUIT);
		$subcircuit = new CDevice();
		$subcircuit->name = $SUBCIRCUIT;
		char * myString = $stringList->back();
		$stringList->pop_back();
		CTextList * myParameterList = new CTextList;
		while( strchr(myString, '=') ) {
			myParameterList->push_front(myString);
			myString = $stringList->back();
			$stringList->pop_back();
		}	
		$subcircuit->masterName = myString;
		$subcircuit->signalList_p = $stringList;
		if ( ! myParameterList->empty() ) {
			myParameterList->push_front(myString);  // add box model name
			$subcircuit->parameters = cdlCircuitList.parameterText.SetTextAddress("X", myParameterList);
		}
	};

bipolar:
	BIPOLAR STRING[collector] STRING[base] STRING[emitter] stringList EOL {
/*
		printf("** ERROR: bipolar device not supported: %s\n", $BIPOLAR);
		cdlCircuitList.errorCount++;
		$bipolar = NULL;
*/
/**/
		$bipolar = new CDevice();
		$bipolar->name = $BIPOLAR;
		$bipolar->signalList_p = new CTextList();
		$bipolar->AppendSignal($collector);
		$bipolar->AppendSignal($base);
		$bipolar->AppendSignal($emitter);
		$bipolar->parameters = cdlCircuitList.parameterText.SetTextAddress("Q", $stringList);	
/**/	
};

capacitor:
	CAPACITOR STRING[plus] STRING[minus] stringList EOL {
		$capacitor = new CDevice();
		$capacitor->name = $CAPACITOR;
		$capacitor->signalList_p = new CTextList();
		$capacitor->AppendSignal($plus);
		$capacitor->AppendSignal($minus);
		$capacitor->AppendSignal($stringList->BiasNet(cdlCircuitList.cdlText));
/*
		if ( $stringList->BiasNet(cdlCircuitList.cdlText) ) {
			printf("** Warning: three terminal capacitor not supported: %s\n", $CAPACITOR);
			cdlCircuitList.warningCount++;
		}
*/
		$capacitor->parameters = cdlCircuitList.parameterText.SetTextAddress("C", $stringList);	
	};

diode:
	DIODE STRING[anode] STRING[cathode] stringList EOL {
		$diode = new CDevice();
		$diode->name = $DIODE;
		$diode->sourceDrainSet = true;
		$diode->sourceDrainSwapOk = false;
		$diode->signalList_p = new CTextList();
		$diode->AppendSignal($anode);
		$diode->AppendSignal($cathode);
		$diode->parameters = cdlCircuitList.parameterText.SetTextAddress("D", $stringList);		
	};

inductor:
	INDUCTOR STRING[plus] STRING[minus] stringList EOL {
		printf("** ERROR: inductor device not supported: %s\n", $INDUCTOR);
		cdlCircuitList.errorCount++;
		$inductor = NULL;
	/*
		$inductor = new CDevice();
		$inductor->name = $INDUCTOR;
		$inductor->AppendSignal($plus);
		$inductor->AppendSignal($minus);
		$inductor->parameters = cdlCircuitList.parameterText.SetTextAddress($stringList);	
	*/
	};

mosfet:
	MOSFET STRING[drain] STRING[gate] STRING[source] STRING[bulk] stringList EOL {
		$mosfet = new CDevice();
		$mosfet->name = $MOSFET;
		$mosfet->signalList_p = new CTextList();
		$mosfet->AppendSignal($drain);
		$mosfet->AppendSignal($gate);
		$mosfet->AppendSignal($source);
		if ( ! theCvcSOI ) {
			$mosfet->AppendSignal($bulk);
		}
		$mosfet->parameters = cdlCircuitList.parameterText.SetTextAddress("M", $stringList);	
	};

resistor:
	RESISTOR STRING[plus] STRING[minus] stringList EOL {
		$resistor = new CDevice();
		$resistor->name = $RESISTOR;
		$resistor->signalList_p = new CTextList();
		$resistor->AppendSignal($plus);
		$resistor->AppendSignal($minus);
		$resistor->AppendSignal($stringList->BiasNet(cdlCircuitList.cdlText));
/*
		if ( $stringList->BiasNet(cdlCircuitList.cdlText) ) {
			printf("** Warning: three terminal resistor not supported: %s\n", $RESISTOR);
			cdlCircuitList.warningCount++;
		}
*/
		$resistor->parameters = cdlCircuitList.parameterText.SetTextAddress("R", $stringList);	
	};

%%

void
yy::CCdlParser::error (const yy::location& l, const string& m)
{
	driver.error (l, m);
}
