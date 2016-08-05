/*
 * cvc.cc
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
char vv_trailer[2];
///@}

bool gDebug_cvc = false;

HIST_ENTRY **gHistoryList; //!< readline history

/// \name global status constants
///@{
CStatus PMOS_ONLY, NMOS_ONLY, NO_TYPE;
///@}

CCvcDb	* gCvcDb; //!< CVC global database

/**
 * usage:
 * cvc [-v|`--version`] [-i|`--interactive`] [-p|`--prefix` prefix] <mode1.cvcrc> [<mode2.cvcrc>] ...\n
 * -v : print cvc program version\n
 * -p "prefix" : add "prefix-" to all file names\n 
 * -i : interactive mode\n
 * <mode1.cvcrc> [<mode2.cvcrc>] ... : list of verification setting files.
 */
int main(int argc, const char * argv[]) {

	// Set global status constants
	PMOS_ONLY[PMOS] = true;
	NMOS_ONLY[NMOS] = true;
	NO_TYPE = 0;

try {
	using_history();
	gCvcDb = new CCvcDb(argc, argv);
	gCvcDb->VerifyCircuitForAllModes(argc, argv);
}
catch (EFatalError& e) {
	gCvcDb->RemoveLock();
	cout << e.what() << endl;
}
catch (exception& e) {
	gCvcDb->RemoveLock();
	cout << "unexpected error: " << e.what() << endl;
}
}

