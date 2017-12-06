/*
 * CSet.cc
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

#include "CSet.hh"

bool CSet::Intersects(CSet & theOtherSet) {
	 CSet *mySmallSet_p, *myLargeSet_p;
	 if ( size() > theOtherSet.size() ) {
		 mySmallSet_p = &theOtherSet;
		 myLargeSet_p = this;
	 } else {
		 mySmallSet_p = this;
		 myLargeSet_p = &theOtherSet;
	 }
	 for ( auto myItem = mySmallSet_p->begin(); myItem != mySmallSet_p->end(); myItem++ ) {
		 if ( myLargeSet_p->count(*myItem) > 0 ) return true;
	 }
	 return false;
};



