/*
 * CCondition.cc
 *
 * Copyright 2014-2106 D. Mitch Bailey  cvc at shuharisystem dot com
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

#include "CCondition.hh"

CCondition::CCondition(string theParameter, string theRelation, string theValue) {
	parameter = theParameter;
	relation = gRelationStringMap[theRelation];
	stringValue = theValue;
	CNormalValue myNumericValue(theValue);
	numericValue = myNumericValue;
}

bool CCondition::CheckCondition(CNormalValue & theCheckValue) {
	switch (relation) {
			case equals: return (( theCheckValue == numericValue ) ? true : false);
			case lessThan: return (( theCheckValue < numericValue ) ? true : false);
			case greaterThan: return (( theCheckValue > numericValue ) ? true : false);
			case notLessThan: return (( ! ( theCheckValue < numericValue )) ? true : false);
			case notGreaterThan: return (( ! ( theCheckValue > numericValue )) ? true : false);
	default: return false;
	}
}

string CCondition::PrintRelation() {
	try {
		return gRelationMap.at(relation);
	}
	catch (const out_of_range& oor_exception) {
		return "??";
	}
}

string CCondition::PrintValue() {
	return to_string<long int>(numericValue.value) + "@" + to_string<int>(numericValue.scale);
}

void	CCondition::Print(ostream & theLogFile, string theIndentation) {
	if ( theIndentation == "same-line" ) {
		theLogFile << " " << parameter << PrintRelation() << numericValue.Format();
	} else {
		theLogFile << theIndentation << "Condition: " << parameter << PrintRelation() << numericValue.Format() << endl;
	}
}

