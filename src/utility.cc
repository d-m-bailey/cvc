/*
 * utility.cc
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

#include "Cvc.hh"

#include <stdlib.h>
#include <stddef.h>
#include <locale.h>
#include <stdio.h>
#include <time.h>

void * xmalloc (size_t size) {
	register void *value = malloc (size);
	if (value == 0) {
		perror ("virtual memory exhausted");
		exit (1);
	}
	return value;
}

std::string PrintParameter(int parameter, int scale) {
	char formattedString[64];
	if ( parameter == UNKNOWN_VOLTAGE ) {
		return "????";
	} else {
		sprintf(formattedString, "%.3f", float(parameter) / scale);
		// remove trailing '0's
		if ( formattedString[strlen(formattedString)-1] == '0' ) formattedString[strlen(formattedString)-1] = '\0';
		if ( formattedString[strlen(formattedString)-1] == '0' ) formattedString[strlen(formattedString)-1] = '\0';
		return formattedString;
	}
}

std::string PrintToleranceParameter(std::string definition, int parameter, int scale) {
	string myOutput = definition;
	if ( parameter != UNKNOWN_VOLTAGE ) {
		string myConvertedParameter = PrintParameter(parameter, scale);
		if ( definition != myConvertedParameter ) myOutput += "->" + myConvertedParameter;
	}
	return(myOutput);
}

std::string AddSiSuffix(float theValue) {
	char myFormattedString[64];
	const string myBigSuffix = "KMGTPE";
	const string mySmallSuffix = "munpfa";
	int myIndex = 0;
	while ( abs(theValue) > 1000 && myIndex < 6 ) {
		myIndex++;
		theValue = theValue / 1000;
	}
	while ( abs(theValue) < 1 && myIndex > -6 ) {
		myIndex--;
		theValue = theValue * 1000;
	}
	if ( myIndex == -6 && abs(theValue) < 0.001 ) {
		myIndex = 0;
		theValue = 0.0;
	}
	if ( myIndex > 0 ) {
		sprintf(myFormattedString, "%.3g%c", theValue, myBigSuffix[myIndex-1]);
	} else if ( myIndex < 0 ) {
		sprintf(myFormattedString, "%.3g%c", theValue, mySmallSuffix[abs(myIndex)-1]);
	} else {
		sprintf(myFormattedString, "%.3g", theValue);
	}
	string myOutput = myFormattedString;
	return(myOutput);
}

std::string RemoveCellNames(std::string thePath) {
	size_t myCellNameStart, myCellNameEnd;
	string myReturnString;
	myCellNameStart = thePath.find_first_of("(");
	myReturnString = thePath.substr(0, myCellNameStart);
	while( myCellNameStart <= thePath.length() ) {
		myCellNameEnd = thePath.find_first_of(")", myCellNameStart);
		myCellNameStart = thePath.find_first_of("(", myCellNameEnd);
		myReturnString += thePath.substr(myCellNameEnd + 1, myCellNameStart - myCellNameEnd - 1);
	}
//	cout << "DEBUG: input:" << thePath << " output:" << myReturnString << endl;
	return(myReturnString);
}

char * CurrentTime() {
	time_t rawtime;
	struct tm * timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	return ( asctime (timeinfo) );
}

std::list<string> * ParseEquation(std::string theEquation, std::string theDelimiters) {
	std::list<std::string> * myTokenList_p = new(std::list<std::string>);
	size_t myTokenStart = 0, myTokenEnd;
//	cout << "Parsing " << theEquation << " =";
	while ( myTokenStart < theEquation.length() ) {
		if ( theDelimiters.find_first_of(theEquation[myTokenStart]) < theDelimiters.length() ) {
			myTokenList_p->push_back(theEquation.substr(myTokenStart, 1));
//			cout << " operator " << myTokenList_p->back();
			myTokenStart++;
		} else {
			myTokenEnd = theEquation.find_first_of(theDelimiters, myTokenStart);
			myTokenList_p->push_back(theEquation.substr(myTokenStart, myTokenEnd - myTokenStart));
//			cout << " operand " << myTokenList_p->back();
			myTokenStart = myTokenEnd;
		}
	}
	//	std::cout << endl << "Parsing result ";
/*
	std::cout << "Parsing result ";
	for ( auto token_pit = myTokenList_p->begin(); token_pit != myTokenList_p->end(); token_pit++ ) {
		std::cout << "'" << *token_pit << "' ";
	}
	std::cout << std::endl;
*/
	return(myTokenList_p);
}

std::list<string> * postfix(std::string theEquation) {
	// see http://csis.pace.edu/~wolf/CS122/infix-postfix.htm
	// < is the min operator. returns the lesser of 2 values
	// > is the max operator. returns the greater of 2 values
	std::string myOperators = "()+-/*<>";
	std::string myLowPrecedence = "+-";
	std::string myHighPrecedence = "/*<>";
	std::string myLastToken = "";
	std::list<std::string> * myParseList_p = ParseEquation(theEquation, myOperators);
	std::list<std::string> * myPostfixList_p = new(std::list<std::string>);
	std::list<std::string> myOperatorStack;
	for ( auto string_pit = myParseList_p->begin(); string_pit != myParseList_p->end(); string_pit++ ) {
		if ( myOperators.find(*string_pit) > myOperators.length() ) { // not an operator
			myPostfixList_p->push_back(*string_pit);
		} else if ( myOperatorStack.empty() || myOperatorStack.back() == "(" || *string_pit == "(" ) {
			if ( *string_pit == "-" && ( IsEmpty(myLastToken) || myLastToken == "(" ) ) {
				myPostfixList_p->push_back("0"); // for unary '-'
			}
			myOperatorStack.push_back(*string_pit);
		} else if ( *string_pit == ")" ) {
			while ( myOperatorStack.back() != "(" ) {
				myPostfixList_p->push_back(myOperatorStack.back());
				myOperatorStack.pop_back();
			}
			myOperatorStack.pop_back();
		} else if ( myHighPrecedence.find(*string_pit) < myHighPrecedence.length() && myLowPrecedence.find(myOperatorStack.back()) < myLowPrecedence.length() ) { // higher precedence
			myOperatorStack.push_back(*string_pit);
		} else if ( myLowPrecedence.find(*string_pit) < myLowPrecedence.length() && myHighPrecedence.find(myOperatorStack.back()) < myHighPrecedence.length() ) { // lower precedence
			while ( ! myOperatorStack.empty() && myHighPrecedence.find(myOperatorStack.back()) < myHighPrecedence.length() ) {
				myPostfixList_p->push_back(myOperatorStack.back());
				myOperatorStack.pop_back();
			}
			if ( ! myOperatorStack.empty() && myLowPrecedence.find(myOperatorStack.back()) < myLowPrecedence.length() ) {
				myPostfixList_p->push_back(myOperatorStack.back());
				myOperatorStack.pop_back();
			}
			myOperatorStack.push_back(*string_pit);
		} else { // same priority
			myPostfixList_p->push_back(myOperatorStack.back());
			myOperatorStack.pop_back();
			myOperatorStack.push_back(*string_pit);
		}
		myLastToken = *string_pit;
	}
	while ( ! myOperatorStack.empty() ) {
		myPostfixList_p->push_back(myOperatorStack.back());
		myOperatorStack.pop_back();
	}
	delete myParseList_p;
/*
	std::cout << "Postfix result ";
	for ( auto token_pit = myPostfixList_p->begin(); token_pit != myPostfixList_p->end(); token_pit++ ) {
		std::cout << "'" << *token_pit << "' ";
	}
	std::cout << std::endl;
*/
	return (myPostfixList_p);
}

std::string RegexErrorString(std::regex_constants::error_type theErrorCode) {
	if ( theErrorCode == std::regex_constants::error_collate ) return("error_collate");
	if ( theErrorCode == std::regex_constants::error_ctype ) return("error_ctype");
	if ( theErrorCode == std::regex_constants::error_escape ) return("error_escape");
	if ( theErrorCode == std::regex_constants::error_backref ) return("error_backref");
	if ( theErrorCode == std::regex_constants::error_brack ) return("error_brack");
	if ( theErrorCode == std::regex_constants::error_paren ) return("error_paren");
	if ( theErrorCode == std::regex_constants::error_brace ) return("error_brace");
	if ( theErrorCode == std::regex_constants::error_badbrace ) return("error_badbrace");
	if ( theErrorCode == std::regex_constants::error_range ) return("error_range");
	if ( theErrorCode == std::regex_constants::error_space ) return("error_space");
	if ( theErrorCode == std::regex_constants::error_badrepeat ) return("error_badrepeat");
	if ( theErrorCode == std::regex_constants::error_complexity ) return("error_complexity");
	if ( theErrorCode == std::regex_constants::error_stack ) return("error_stack");
	return("unknown regex error: " + to_string<int>(theErrorCode));
}

/**
 * \brief Converts shell globbing syntax to regex
 *
 * Replaces '*' -> '.*' and '?' -> '.'.\n
 * Combines with original filter to search for both simultaneously.
 */
std::string FuzzyFilter(std::string theFilter) {
	std::string myGlobFilter = theFilter;
	bool myUseGlob = false;
	for (size_t char_it = 0; char_it < myGlobFilter.length(); char_it++) {
		switch (myGlobFilter[char_it]) {
			case '*': {
				if ( char_it == 0 || myGlobFilter[char_it-1] != '.' ) {
					myGlobFilter.replace(char_it, 1, ".*");
					char_it++;
					myUseGlob = true;
				} break;
			}
			case '?': { myGlobFilter.replace(char_it, 1, "."); break; }
			default: break;
		}
	}
	try { // check original filter
		std::regex myTestFilter(theFilter);
	}
	catch (const std::regex_error& myError) { // if invalid, use only globbed filter
		return(myGlobFilter);
	}
//	std::cout << myGlobFilter << endl;
	if ( myUseGlob ) return(myGlobFilter);
	return("(" + myGlobFilter + ")|(" + theFilter + ")");
}

/**
 * \brief Returns true if the input string consists of only alphanumeric characters
 */
bool IsAlphanumeric(std::string theString) {
	for ( size_t char_it = 0; char_it < theString.length(); char_it++ ) {
		if ( ! isalnum(theString[char_it]) ) return false;
	}
	return true;
}
