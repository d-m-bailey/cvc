/*
 * CCvcExceptions.hh
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

#ifndef CCVCEXCEPTIONS_HH_
#define CCVCEXCEPTIONS_HH_

#include "Cvc.hh"

class EEquivalenceError : public exception {
public:
	const char* what() const noexcept { return "Could not short nets connected to different power"; }
};

class EPowerDefinition : public exception {
public:
	const char* what() const noexcept { return "Power definition problem"; }
};


class EUnknownModel : public exception {
public:
	const char* what() const noexcept { return "Unknown model"; }
};

class EModelError : public exception {
public:
	string errorMessage = "";
	EModelError();
	EModelError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { string myMessage = "Model error:" + errorMessage; return myMessage.c_str(); }
};

class EDuplicateInstance : public exception {
public:
	const char* what() const noexcept { return "Duplicate instance"; }
};

class EDatabaseError : public exception {
public:
	string errorMessage = "";
	EDatabaseError();
	EDatabaseError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { string myMessage = "Database error:" + errorMessage; return myMessage.c_str(); }
};

class EFatalError : public exception {
public:
	string errorMessage = "";
	EFatalError();
	EFatalError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { string myMessage = "Fatal error:" + errorMessage; return myMessage.c_str(); }
};

class EBadEnvironment : public exception {
public:
	const char* what() const noexcept { return "Bad environment"; }
};

class EQueueError : public exception {
public:
	string errorMessage = "";
	EQueueError();
	EQueueError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { string myMessage = "Queue error:" + errorMessage; return myMessage.c_str(); }
};

class EPowerError : public exception {
public:
	string errorMessage = "";
	EPowerError();
	EPowerError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { string myMessage = "Power error:" + errorMessage; return myMessage.c_str(); }
};

class EResistanceError : public exception {
public:
	string errorMessage = "";
	EResistanceError();
	EResistanceError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { string myMessage = "Resistance error:" + errorMessage; return myMessage.c_str(); }
};

class EShortFileError : public exception {
public:
	string errorMessage = "";
	const char* what() const noexcept { return "Error reading short file"; }
};

#endif /* CCVCEXCEPTIONS_HH_ */
