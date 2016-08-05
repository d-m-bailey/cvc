/*
 * CCondition.hh
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

#ifndef CCONDITION_HH_
#define CCONDITION_HH_

#include "Cvc.hh"

#include "CNormalValue.hh"

class CCondition {
public:
	string parameter;
	relation_t	relation;
	string	stringValue;
	CNormalValue 	numericValue;

	CCondition(string theParameter, string theRelation, string theValue);
	bool	CheckCondition(CNormalValue & theCheckValue);
	long int	ConvertValue(string theStringValue);

	string 	PrintRelation();
	string	PrintValue();
	void	Print(ostream & theLogFile = cout, string theIndentation = "");
};

class CConditionPtrList : public list<CCondition *> {
public:

};


#endif /* CCONDITION_HH_ */
