/*
 * cvc.cc
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

/// \file
/// CVC main routine

#include "Cvc.hh"
#include "CCvcDb.hh"

#include <readline/readline.h>
#include <readline/history.h>

/// \name vv_* : valid voltage globals
///@{
char vv_sign;
int vv_integer, vv_fraction;
char vv_suffix[2], vv_trailer[2];
///@}

bool gDebug_cvc = false;
bool gInterrupted = false;  //!< for detecting interrupts

HIST_ENTRY **gHistoryList; //!< readline history

/// \name global status constants
///@{
CStatus PMOS_ONLY, NMOS_ONLY, NMOS_PMOS, NO_TYPE;
///@}

CCvcDb	* gCvcDb; //!< CVC global database

/**
 * usage:
 * cvc [-v|`--version`] [-i|`--interactive`] [-p|`--prefix` <I>prefix</I>]
 * <I>mode1.cvcrc</I> [<I>mode2.cvcrc</I> ...]\n
 * -v : print cvc program version\n
 * -p "prefix" : add "prefix-" to all file names\n 
 * -i : interactive mode\n
 * <mode1.cvcrc> [<mode2.cvcrc>] ... : list of verification setting files.
 */
int main(int argc, const char * argv[]) {

	// Set global status constants
	PMOS_ONLY[PMOS] = true;
	NMOS_ONLY[NMOS] = true;
	NMOS_PMOS[NMOS] = true;
	NMOS_PMOS[PMOS] = true;
	NO_TYPE = 0;

try {
	using_history();
	gCvcDb = new CCvcDb(argc, argv);
	if ( gDebug_cvc ) gCvcDb->PrintClassSizes();
	gCvcDb->VerifyCircuitForAllModes(argc, argv);
}
catch (EFatalError& e) {
	// Handle known errors.
	gCvcDb->RemoveLock();
	cout << e.what() << endl;
}
catch (exception& e) {
	// Handle unknown errors.
	gCvcDb->RemoveLock();
	cout << "unexpected error: " << e.what() << endl;
}
	delete gCvcDb;
}

/// \file Coding guidelines:
/// Default: Python coding conventions: PEP8
/// Max line length: 100 characters
/// Naming conventions:
///   Note: These conventions are intentionally different from library conventions.
///     This helps to distinguish program functions and variables from those in libraries.
///   Constants: ALL_CAPS
///   Functions: CamelCase with leading upper case letter. Function names should begin with a verb.
///   Macros used as functions: CamelCase with trailing '_'. e.g. SetParameters_
///   Classes: CamelCase with leading upper case letter. Class names should begin with a noun.
///   Variables: camelCase with leading lower case letter.
///     globals: leading 'g'. e.g. gCount
///     function parameters: leading 'the'. e.g. theCount
///     local: leading 'my'. e.g. myCount (exception for iterators)
///     iterators: trailing '_it'. e.g. element_it (or '_pit' for pointer iterators)
///     everything else is class variable.
///     pointers: trailing '_p'. e.g. cirtuit_p
///     vectors: trailing '_v'. e.g. circuit_v
/// Indents: one tab for each level
///   try-catch not indented
///   switch and case statements at same level.
///

//23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
