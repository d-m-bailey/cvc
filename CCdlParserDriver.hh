/*
 * CCdlParserDriver.hh
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

#ifndef CCdlParserDriver_HH

#define CCdlParserDriver_HH

#include "Cvc.hh"
#include "location.hh"

#include "CCircuit.hh"

namespace yy {
	typedef location location_type;
	class CCdlParser;
}

// Conducting the whole scanning and parsing of Cdl File.
class CCdlParserDriver
{
public:
	CCdlParserDriver ();
	virtual ~CCdlParserDriver ();

	int result;

	// Handling the scanner.
	void scan_begin ();
	void scan_end ();
	bool trace_scanning;

	// Run the parser on file F.
	// Return 0 on success.
	int parse (const string& theCdlFileName, CCircuitPtrList& theCircuitPtrList, bool theCvcSOI);

	// The name of the file being parsed.
	// Used later to pass the file name to the location tracker.
	string filename;

	// Whether parser traces should be generated.
	bool trace_parsing;


	// Error handling.
	void error (const yy::location_type& theLocation, const string& theMessage);
	void error (const string& theMessage);
};

#include "cdlParser.hh"

// Tell Flex the lexer's prototype ...
# define YY_DECL \
 yy::CCdlParser::token_type yylex (yy::CCdlParser::semantic_type *yylval, yy::CCdlParser::location_type *yylloc, CCdlParserDriver& driver, CCircuitPtrList& cdlCircuitList, bool theCvcSOI)
YY_DECL;


#endif // ! CCdlParserDriver_HH
