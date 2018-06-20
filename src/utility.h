/*
 * utility.h
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

#ifndef UTILITY_H_
#define UTILITY_H_

#include "Cvc.hh"

void * xmalloc(size_t size);
std::string PrintParameter(int parameter, int scale);
std::string PrintToleranceParameter(std::string definition, int parameter, int scale);
std::string AddSiSuffix(float theValue);
std::string RemoveCellNames(std::string thePath);

template< typename T >
std::string int_to_hex( T i )
{
  std::stringstream stream;
  stream << "0x"
	<< std::setfill ('0') << std::setw(sizeof(T)*2)
	<< std::hex << i;
  return stream.str();
}

#define trim_(string) (string.substr(0, string.find_last_not_of(" \t\n") + 1).substr(string.find_first_not_of(" \t\n")))
//#define IsValidVoltage_(string) (regex_search(string, regex("^[-+]?[0-9]+\\.?[0-9]*$", regex_constants::awk)))
#define IsValidVoltage_(string) (\
		sscanf((string).c_str(), "%[-+]%d.%d%1s", &vv_sign, &vv_integer, &vv_fraction, vv_trailer) == 3 \
		|| sscanf((string).c_str(), "%[-+]%d%1s", &vv_sign, &vv_integer, vv_trailer) == 2 \
		|| sscanf((string).c_str(), "%d.%d%1s", &vv_integer, &vv_fraction, vv_trailer) == 2 \
		|| sscanf((string).c_str(), "%d%1s", &vv_integer, vv_trailer) == 1 \
		|| sscanf((string).c_str(), "%[-+]%d.%d%1[afpnumKMGTPE]%1s", &vv_sign, &vv_integer, &vv_fraction, vv_suffix, vv_trailer) == 4 \
		|| sscanf((string).c_str(), "%[-+]%d%1[afpnumKMGTPE]%1s", &vv_sign, &vv_integer, vv_suffix, vv_trailer) == 3 \
		|| sscanf((string).c_str(), "%d.%d%1[afpnumKMGTPE]%1s", &vv_integer, &vv_fraction, vv_suffix, vv_trailer) == 3 \
		|| sscanf((string).c_str(), "%d%1[afpnumKMGTPE]%1s", &vv_integer, vv_suffix, vv_trailer) == 2 \
)

char * CurrentTime();

std::list<std::string> * ParseEquation(std::string theEquation, std::string theDelimiters);
std::list<std::string> * postfix(std::string theEquation);

inline void AddResistance(resistance_t & theBase, resistance_t theIncrement) {
	theBase = ( (long(theBase) + long(theIncrement)) > long(MAX_RESISTANCE) ) ? MAX_RESISTANCE : theBase + theIncrement;
};

inline bool gTextCompare(char* i, char* j) {return(strcmp(i, j) < 0);};

std::string RegexErrorString(std::regex_constants::error_type theErrorCode);

std::string FuzzyFilter(std::string theFilter);
bool IsAlphanumeric(std::string theString);
inline bool IsEmpty(char * theText) {return (theText[0] == '\0' );};
inline bool IsEmpty(std::string theString) {return (theString.empty());};

#endif /* UTILITY_H_ */
