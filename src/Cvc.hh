/*
 * Cvc.hh
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

#ifndef CVC_H_
#define CVC_H_

#define CVC_VERSION "1.1.5"

extern bool gDebug_cvc;
extern bool gSetup_cvc;
extern bool gInterrupted;
extern bool gInteractive_cvc;

// valid voltage globals
extern char vv_sign;
extern int vv_integer, vv_fraction;
extern char vv_suffix[], vv_trailer[];

#include "CvcTypes.hh"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <array>
#include <bitset>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <exception>
#include <iostream>
#include <fstream>
#include <list>
#include <forward_list>
#include <map>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iomanip>
#include <set>
#include <regex>

#include "sys/resource.h"

using namespace std;

typedef char * text_t;

#include "utility.h"
#include "CvcMaps.hh"
#include "CCvcExceptions.hh"
// #include "SFHash.hh"

// basic classes

template <class T>
void ResetVector (T& theVector, uintmax_t theSize) {
	theVector.clear();
	theVector.reserve(theSize);
	theVector.resize(theSize);
}

template <class T>
void ResetVector (T& theVector, uintmax_t theSize, uintmax_t theDefault) {
	theVector.clear();
	theVector.reserve(theSize);
	theVector.resize(theSize, theDefault);
}

template <class T>
void ResetVector (T& theVector, uintmax_t theSize, modelType_t theDefault) {
	theVector.clear();
	theVector.reserve(theSize);
	theVector.resize(theSize, theDefault);
}

template <class T>
void ResetPointerVector (T& theVector, size_t theSize) {
	theVector.clear();
	theVector.reserve(theSize);
	theVector.resize(theSize, NULL);
}

template<typename KEY_T, typename TARGET_T>
void DumpStatistics(unordered_map<KEY_T, TARGET_T> & theMap, string theTitle, ostream& theOutputFile) {
	theOutputFile << "Hash dump:" << theTitle << endl;
	unsigned myBucketCount = theMap.bucket_count();
	unsigned myHashSize = theMap.size();
	theOutputFile << "Contains " << myBucketCount << " buckets, " << myHashSize << " elements" << endl;
	unsigned myElementCount = 0;
	unsigned myMaxBucketSize = 0;
	vector<int> myBucket_v;
	myBucket_v.resize(50, 0);
	for ( unsigned bucket_it = 0; bucket_it < myBucketCount; bucket_it++ ) {
		myElementCount = theMap.bucket_size(bucket_it);
		if ( myElementCount > myMaxBucketSize ) myMaxBucketSize = myElementCount;
		if ( myElementCount > 49 ) myElementCount = 49;
		myBucket_v[myElementCount] += 1;
	}
	int myDepthCount = 0;
	for ( unsigned count_it = 0; count_it <= (unsigned) min((int) myMaxBucketSize, 49); count_it++ ) {
		theOutputFile << "Element count " << count_it << ", " << myBucket_v[count_it] << endl;
		myDepthCount += count_it * myBucket_v[count_it] * count_it;
	}
	int myLastPrecision = cout.precision();
	theOutputFile.precision(2);
	theOutputFile << fixed << "Unused hash: " << ((float) myBucket_v[0] / myBucketCount) << ", average depth " << ((float) myDepthCount / myHashSize) << endl;
	theOutputFile.precision(myLastPrecision);
	//	cout << "Max element count " << myMaxBucketSize << endl;
}

class CTextVector : public vector<text_t> {
public:
};

class CFixedText;

class CTextList : public list<text_t> {
public:
	text_t BiasNet(CFixedText & theCdlText);
};

#define DEFAULT_LOAD_FACTOR	2.0

class CTextResistanceMap : public unordered_map<text_t, resistance_t> {
public:
	CTextResistanceMap(float theLoadFactor = DEFAULT_LOAD_FACTOR) {max_load_factor(theLoadFactor);}
};

class CTextNetIdMap : public unordered_map<text_t, netId_t> {
public:
	CTextNetIdMap(float theLoadFactor = DEFAULT_LOAD_FACTOR) {max_load_factor(theLoadFactor);}
};

class CTextDeviceIdMap : public unordered_map<text_t, deviceId_t> {
public:
	CTextDeviceIdMap(float theLoadFactor = DEFAULT_LOAD_FACTOR) {max_load_factor(theLoadFactor);}
};

class CTextInstanceIdMap : public unordered_map<text_t, instanceId_t> {
public:
	CTextInstanceIdMap(float theLoadFactor = DEFAULT_LOAD_FACTOR) {max_load_factor(theLoadFactor);}
};

class CStringTextMap : public unordered_map<string, text_t> {
public:
	CStringTextMap(float theLoadFactor = DEFAULT_LOAD_FACTOR) {max_load_factor(theLoadFactor);}
};

class CNetIdVector : public vector<netId_t> {
public:
};

class CDeviceIdVector : public vector<deviceId_t> {
public:
};

class CInstanceIdVector : public vector<instanceId_t> {
public:
};

class CNetIdSet : public unordered_set<netId_t> {
public:
};

typedef bitset<8> CStatus;
extern CStatus PMOS_ONLY, NMOS_ONLY, NMOS_PMOS, NO_TYPE; //, MIN_CHECK_BITS, MAX_CHECK_BITS;

class CStatusVector : public vector<CStatus> {
public:
};

#define IGNORE_WARNINGS	false
#define PRINT_WARNINGS	true

#define toupper_(theString) for (size_t i=0; i<=theString.length(); i++) { theString[i]=toupper(theString[i]); }

class CNetList {

};

class CHierList {

};

class CStringList {

};

template<typename T1, typename T2>
T1 min(T1 a, T2 b) {return ((a < (T1)b) ? a : (T1) b);}
template<typename T1, typename T2>
T1 max(T1 a, T2 b) {return ((a > (T1)b) ? a : (T1) b);}

enum returnCode_t {UNKNOWN_RETURN_CODE=0, OK, SKIP, FAIL};

enum stage_t {STAGE_START = 1, STAGE_LINK, STAGE_RESISTANCE, STAGE_FIRST_MINMAX, STAGE_FIRST_SIM, STAGE_SECOND_SIM, STAGE_COMPLETE};

#endif /* CVC_H_ */
