/*
 * CNormalValue.hh
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

#ifndef CNORMALVALUE_HH_
#define CNORMALVALUE_HH_

#include "Cvc.hh"

class CNormalValue {
public:
	long int	value = 0;
	int			scale = 0;

	CNormalValue(string stringValue);
	CNormalValue();

	inline bool operator==(CNormalValue theCompareValue) {return ( value == theCompareValue.value && scale == theCompareValue.scale );};
	bool operator<(CNormalValue theCompareValue);
	bool operator>(CNormalValue theCompareValue);

	static void Test(string theFirstValue, string theSecondValue);
	static void TestFunction();
	short int Compare(string stringValue);
	float RealValue();

	void Print();
	string Format();

};

typedef CNormalValue * CNormalValuePtr;

#endif /* CNORMALVALUE_HH_ */
