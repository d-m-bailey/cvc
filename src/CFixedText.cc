/*
 * CFixedText.cc
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

#include "CFixedText.hh"

#include "obstack.h"


CFixedText::CFixedText() {
	const char myDummy[] = "";
	obstack_init(&fixedTextObstack);
	firstAddress = (text_t) obstack_alloc(&fixedTextObstack, 0);
	obstack_copy0(&(fixedTextObstack), (void *) myDummy, 0);  // first string is null string
	size = 1;
}

CFixedText::~CFixedText() {
	obstack_free(&fixedTextObstack, NULL);
	//obstack_init(&fixedTextObstack);
}

void CFixedText::Clear() {
	const char myDummy[] = "";
	obstack_free(&fixedTextObstack, firstAddress);
	fixedTextToAddressMap.clear();
	obstack_copy0(&(fixedTextObstack), (void *) myDummy, 0);  // first string is null string
	size = 1;
}

text_t CFixedText::SetTextAddress(const text_t theNewText){
	text_t myTextAddress;
	//static string myKeyText;
	string myKeyText;

	// TODO: try with count for speed (2 hash lookups vs throwing error)
	try {
		myKeyText = theNewText;
		myTextAddress = fixedTextToAddressMap.at(myKeyText);
	}
	catch (const out_of_range& oor_exception) {
		int myNewSize = int(myKeyText.length());
		myTextAddress = (text_t) obstack_copy0(&(fixedTextObstack), (void *) myKeyText.c_str(), myNewSize);
		size += myNewSize + 1;
		fixedTextToAddressMap[myKeyText] = myTextAddress;
	}
	return(myTextAddress);
}

text_t CFixedText::SetTextAddress(const string theType,	CTextList* theNewTextList) {
	text_t myTextAddress;
	//static string myKeyText;
	string myKeyText;
	myKeyText = theType;
	for (CTextList::iterator text_pit = theNewTextList->begin(); text_pit != theNewTextList->end(); text_pit++) {
		myKeyText += " ";
		myKeyText += *text_pit;
	}
	try {
		myTextAddress = fixedTextToAddressMap.at(myKeyText);
	}
	catch (const out_of_range& oor_exception) {
		int myNewSize = int(myKeyText.length());
		myTextAddress = (text_t) obstack_copy0(&fixedTextObstack, (void *) myKeyText.c_str(), myNewSize);
		size += myNewSize + 1;
		fixedTextToAddressMap[myKeyText] = myTextAddress;
	}
	return (myTextAddress);
}

text_t CFixedText::GetTextAddress(const text_t theNewText) {
//	text_t textAddress;
	static string myKeyText;

//	try {
//	send all exceptions to caller
		myKeyText = theNewText;
		return(fixedTextToAddressMap.at(myKeyText));
//	}
//	return(textAddress);
}

text_t CFixedText::GetTextAddress(const string theKeyText){
//	text_t textAddress;

//	try {
	//	send all exceptions to caller
	return(fixedTextToAddressMap.at(theKeyText));
//	}
}

text_t CTextList::BiasNet(CFixedText & theCdlText) {
	for ( auto text_it = begin(); text_it != end(); text_it++ ) {
		if ( strncmp(*text_it, "$SUB=", 4) == 0 ) {
			text_t myBiasNet = *text_it + 5;
			erase(text_it);
			return (theCdlText.SetTextAddress(myBiasNet));
		}
	}
	return (0);
}

