/*
 * CCdlParserDriver.cc
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

#include "CCdlParserDriver.hh"
#include "cdlParser.hh"


#include "CFixedText.hh"

void yyset_in(FILE * in_str);


CCdlParserDriver::CCdlParserDriver() :
		trace_scanning(false), trace_parsing(false) {
	result = 0;
}

CCdlParserDriver::~CCdlParserDriver() {
}

int CCdlParserDriver::parse(const string &theCdlFilename, CCircuitPtrList& theCircuitPtrList, bool theCvcSOI) {
	filename = theCdlFilename;
	ifstream myTestFile;
//	scan_begin();
	FILE * myCdlFile;
	if ( filename.substr(filename.length()-3, 3) == ".gz" && (myTestFile.open(filename), myTestFile.good()) ) {
		myTestFile.close();
		string myZcat = "zcat " + filename;
		myCdlFile = popen(myZcat.c_str(), "r");
	} else {
		myCdlFile = fopen(const_cast<const text_t>(filename.c_str()), "r");
	}
	int myParser_result;
	if (myCdlFile == NULL) {
		cout << "Could not open file " << filename << "\n";
		myParser_result = 1;
	} else {
		yyset_in(myCdlFile);
		yy::CCdlParser parser(*this, theCircuitPtrList, theCvcSOI);
		cout << endl;  // Clear progress output
		parser.set_debug_level(trace_parsing);
		myParser_result = parser.parse();
//	scan_end();
	}
	return myParser_result;
}

void CCdlParserDriver::error(const yy::location_type& theLocation, const string& theMessage) {
	cerr << theLocation << ": " << theMessage << endl;
}

void CCdlParserDriver::error(const string& theMessage) {
	cerr << theMessage << endl;
}
