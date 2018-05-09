/*
 * CParameterMap.cc
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

#include "CParameterMap.hh"

void CParameterMap::CreateParameterMap(string theParameterString) {
	string myParameterName;
	string myParameterValue;
	size_t	myEqualIndex;
	size_t	myStringBegin = 0;
	size_t	myStringEnd;
	if (theParameterString.length() > 0) {
		do {
			myStringEnd = theParameterString.find(" ", myStringBegin);
			myEqualIndex = theParameterString.find("=", myStringBegin);
			myParameterName = theParameterString.substr(myStringBegin, myEqualIndex - myStringBegin);
			if ( myParameterName[0] == '$' ) myParameterName = myParameterName.substr(1);
/*
			for ( int myIndex = 0; myIndex < myParameterName.length(); myIndex++ ) {
				myParameterName[myIndex] = toupper(myParameterName[myIndex]);
			}
*/
			toupper_(myParameterName);
			myParameterValue = theParameterString.substr(myEqualIndex + 1, myStringEnd - (myEqualIndex + 1));
			(*this)[myParameterName] = myParameterValue;
			myStringBegin = myStringEnd + 1;
		} while (myStringEnd < theParameterString.length());
	}
}

void CParameterMap::Print(string theIndentation, string theHeading) {
	string myIndentation = theIndentation + " ";
	cout << theIndentation << theHeading << endl;
	cout << myIndentation << "Parameters:";
	for (CParameterMap::iterator parameterPair_pit = begin(); parameterPair_pit != end(); parameterPair_pit++) {
		cout << " " << parameterPair_pit->first << ":" << parameterPair_pit->second;
	}
	cout << endl;
}

resistance_t CParameterMap::CalculateResistance(string theEquation) {
	list<string> * myTokenList_p;
	// exceptions cause memory leaks
//	cout << "calculating " << theEquation << endl;
	if ( IsValidVoltage_(theEquation) ) {
		myTokenList_p = new(list<string>);
		myTokenList_p->push_back(theEquation);
	} else {
		myTokenList_p = postfix(theEquation);
	}
	list<double> myResistanceStack;
	double	myResistance;
	string	myOperators = "+-/*<>";
	for ( auto token_pit = myTokenList_p->begin(); token_pit != myTokenList_p->end(); token_pit++ ) {
		if ( myOperators.find(*token_pit) < myOperators.length() ) {
			if ( myResistanceStack.size() < 2 ) throw EPowerError("invalid equation: " + theEquation);
			myResistance = myResistanceStack.back();
			myResistanceStack.pop_back();
			if ( *token_pit == "+" ) {
				myResistanceStack.back() += myResistance;
			} else if ( *token_pit == "-" ) {
				myResistanceStack.back() -= myResistance;
			} else if ( *token_pit == "*" ) {
				myResistanceStack.back() *= myResistance;
			} else if ( *token_pit == "/" ) {
				myResistanceStack.back() /= myResistance;
			} else if ( *token_pit == "<" ) {
				myResistanceStack.back() = min(myResistanceStack.back(), myResistance);
			} else if ( *token_pit == ">" ) {
				myResistanceStack.back() = max(myResistanceStack.back(), myResistance);
			} else throw EPowerError("invalid operator: " + *token_pit);
		} else if ( isalpha((*token_pit)[0]) ) { // parameter name
			string myParameter = *token_pit;
			toupper_(myParameter);
			if ( this->count(myParameter) > 0 ) { // parameter definition exists
				CNormalValue myParameterValue((*this)[myParameter]);
				myResistanceStack.push_back(myParameterValue.RealValue());
			} else {
				throw EResistanceError("missing parameter: " + *token_pit + " in " + theEquation);
			}
		} else if ( IsValidVoltage_(*token_pit) ) {
			myResistanceStack.push_back(from_string<float>(*token_pit));
		} else {
			throw EResistanceError("invalid resistance calculation token: " + *token_pit + " in " + theEquation);
		}
	}
	if ( myResistanceStack.size() != 1 ) EResistanceError("invalid equation: " + theEquation);
	delete myTokenList_p;
//	cout << "Final voltage " << myResistanceStack.front() << endl;
	if ( myResistanceStack.front() >= MAX_RESISTANCE ) {
		return ( MAX_RESISTANCE );
	} else {
		return ( round(myResistanceStack.front()) );
	}
}
