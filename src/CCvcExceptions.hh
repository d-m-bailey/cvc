/*
 * CCvcExceptions.hh
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

class EInvalidTerminal : public exception {
public:
	static string displayMessage;
	string errorMessage;
	EInvalidTerminal(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { displayMessage = "Invalid terminal: " + errorMessage; return displayMessage.c_str(); }
};

class EModelError : public exception {
public:
	static string displayMessage;
	string errorMessage;
	EModelError();
	EModelError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { displayMessage = "Model error: " + errorMessage; return displayMessage.c_str(); }
};

class EDuplicateInstance : public exception {
public:
	const char* what() const noexcept { return "Duplicate instance"; }
};

class EDatabaseError : public exception {
public:
	static string displayMessage;
	string errorMessage;
	EDatabaseError();
	EDatabaseError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { displayMessage = "Database error:" + errorMessage; return displayMessage.c_str(); }
};

class EFatalError : public exception {
public:
	static string displayMessage;
	string errorMessage;
	EFatalError();
	EFatalError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { displayMessage = "Fatal error:" + errorMessage; return displayMessage.c_str(); }
};

class EBadEnvironment : public exception {
public:
	const char* what() const noexcept { return "Bad environment"; }
};

class EQueueError : public exception {
public:
	static string displayMessage;
	string errorMessage;
	EQueueError();
	EQueueError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { displayMessage = "Queue error:" + errorMessage; return displayMessage.c_str(); }
};

class EPowerError : public exception {
public:
	static string displayMessage;
	string errorMessage;
	EPowerError();
	EPowerError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { displayMessage = "Power error:" + errorMessage; return displayMessage.c_str(); }
};

class EResistanceError : public exception {
public:
	static string displayMessage;
	string errorMessage;
	EResistanceError();
	EResistanceError(const string theErrorMessage) { errorMessage = theErrorMessage; };
	const char* what() const noexcept { displayMessage = "Resistance error:" + errorMessage; return displayMessage.c_str(); }
};

class EShortFileError : public exception {
public:
	const char* what() const noexcept { return "Error reading short file"; }
};

#endif /* CCVCEXCEPTIONS_HH_ */
