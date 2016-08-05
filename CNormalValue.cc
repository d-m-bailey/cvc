/*
 * CNormalValue.cc
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

#include "CNormalValue.hh"

CNormalValue::CNormalValue(string theStringValue) {
	string myStringValue = trim_(theStringValue);
//	trim_(myStringValue);
	size_t myUnitOffset = myStringValue.find_first_not_of("-+0123456789.eE");
	if ( myUnitOffset > myStringValue.length() ) myUnitOffset = myStringValue.length();
	double	myRealValue = from_string<double>(myStringValue.substr(0, myUnitOffset));

//	cout << "DEBUG: CNormalValue " << myRealValue << " " << myStringValue[myUnitOffset] << endl;
	switch ( myStringValue[myUnitOffset] ) {
		case 'G': {	myRealValue *= pow(10, 9); break; }
		case 'M': {	myRealValue *= pow(10, 6); break; }
		case 'K': {	myRealValue *= pow(10, 3); break; }
		case 'm': {	myRealValue *= pow(10, -3); break; }
		case 'u': {	myRealValue *= pow(10, -6); break; }
		case 'n': {	myRealValue *= pow(10, -9); break; }
		case 'p': {	myRealValue *= pow(10, -12); break; }
		case 'f': {	myRealValue *= pow(10, -15); break; }
		case '\0': break;
		default: {
//			cout << "ERROR: unknown suffix on parameter: " << myStringValue << endl;
			throw EFatalError("unknown suffix on parameter: " + myStringValue);
//			exit(1);
		}
	}
	while ( abs(myRealValue * pow(10, - scale)) < 1e6 && scale > -21 ) {
		scale -= 3;
	}
	value = (long int)(round(myRealValue * pow(10, -scale) + 0.1));
}

CNormalValue::CNormalValue() {
	value = 0;
	scale = 0;
}

bool CNormalValue::operator<(CNormalValue theCompareValue) {
	if ( *this == theCompareValue ) return false;
	if ( value < 0 && theCompareValue.value >= 0 ) return true;
	if ( value >= 0 && theCompareValue.value < 0 ) return false;
	if ( value >= 0 ) {
		if ( scale < theCompareValue.scale ) return true;
		if ( scale > theCompareValue.scale ) return false;
		return ( value < theCompareValue.value );
	} else {
		if ( scale > theCompareValue.scale ) return true;
		if ( scale < theCompareValue.scale ) return false;
		return ( value < theCompareValue.value );
	}
}

bool CNormalValue::operator>(CNormalValue theCompareValue) {
	return ( (*this == theCompareValue) ? false : ! (*this < theCompareValue) );
}

float CNormalValue::RealValue() {
	return ( (float) value / pow(10, -scale) );
}

short int CNormalValue::Compare(string theStringValue) {
	CNormalValue myCompareValue(theStringValue);

	if ( value > 0 ) {
		if ( myCompareValue.value <= 0 ) {
			return (1);
		} else if ( scale > myCompareValue.scale ) {
			return (1);
		} else if ( scale < myCompareValue.scale ) {
			return (-1);
		} else if ( value > myCompareValue.value ) {
			return (1);
		} else if ( value < myCompareValue.value ) {
			return (-1);
		}
	} else if ( value < 0 ) {
		if ( myCompareValue.value >= 0 ) {
			return (-1);
		} else if ( scale > myCompareValue.scale ) {
			return (-1);
		} else if ( scale < myCompareValue.scale ) {
			return (1);
		} else if ( value > myCompareValue.value ) {
			return (1);
		} else if ( value < myCompareValue.value ) {
			return (-1);
		}
	} else { // value == 0
		if ( myCompareValue.value < 0 ) {
			return (-1);
		} else if ( myCompareValue.value > 0 ) {
			return (1);
		}
	}
	return (0);

}

string CNormalValue::Format() {
	// value = 0, scale = ?; output "0"
	// value = 1000000, scale = -6; output "1"
	// value = 800000, scale = -9; output "800e-6"
	if ( value == 0 ) {
		return("0");
	} else {
		int myScale = 0;
		while ( value == int(value / pow(10, myScale + 3)) * pow(10, myScale + 3) ) {
			myScale += 3;
		}
		return to_string<long>(value / pow(10, myScale)) + (( scale + myScale == 0 ) ? "" : ("e" + to_string<int> (myScale + scale)));
	}
}

void CNormalValue::Print() {
	cout << Format();
}

void CNormalValue::Test(string theFirstString, string theSecondString) {
	CNormalValue myFirstValue(theFirstString);
	CNormalValue mySecondValue(theSecondString);
	cout << "Test " << theFirstString << " vs " << theSecondString << endl;
	myFirstValue.Print();
	if ( myFirstValue < mySecondValue ) cout << " < ";
	if ( myFirstValue == mySecondValue ) cout << " = ";
	if ( myFirstValue > mySecondValue ) cout << " > ";
	mySecondValue.Print();
	cout << endl;
 }

void CNormalValue::TestFunction() {
	Test("0", "0");
	Test("-1", "0");
	Test("0", "1");
	Test("1", "0");
	Test("1", "2");
	Test("-1", "-4.5");
	Test("-0.3", "-3");
	Test("0.3", "-3");
	Test("3", "-0.3");
	Test("10u", "10m");
	Test("1.4e-6", "1.4u");
	Test("27K", "-27K");
	Test("68M", "3m");
	Test("6f", "0");

}

