/*
 * CvcTypes.hh
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

#ifndef CVCTYPES_HH_
#define CVCTYPES_HH_

#include <stdint-gcc.h>
#include <string>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <malloc.h>
#include <unordered_map>

#ifndef INT64_MAX
#define INT64_MAX INT32_MAX
#endif

typedef uint32_t netId_t;
typedef uint32_t deviceId_t;
typedef uint32_t instanceId_t;

typedef int32_t	voltage_t;
typedef uint32_t resistance_t;
typedef int32_t eventKey_t;

// Max resistance is 1G. Anything over that truncates to 1G with a warning.
#define INFINITE_RESISTANCE (UINT32_MAX)
#define MAX_RESISTANCE ((UINT32_MAX >> 2) - 1)

#define VOLTAGE_SCALE 1000
#define MAX_VOLTAGE (INT32_MAX - 1)
#define MIN_VOLTAGE (INT32_MIN + 1)

#define MIN_EVENT_TIME (INT32_MIN)
#define MAX_EVENT_TIME (INT32_MAX)

#define MAX_NET (UINT32_MAX - 1)
#define MAX_DEVICE (UINT32_MAX - 1)
#define MAX_SUBCIRCUIT (UINT32_MAX - 1)

#define UNKNOWN_NET UINT32_MAX
#define UNKNOWN_SUBCIRCUIT UINT32_MAX
#define UNKNOWN_DEVICE UINT32_MAX
#define UNKNOWN_INSTANCE UINT32_MAX

#define UNKNOWN_VOLTAGE (INT32_MAX)

#define DEFAULT_MOS_RESISTANCE "L/W*7000"
#define DEFAULT_RESISTANCE "R"
#define DEFAULT_UNKNOWN_RESISTANCE 1100100

#define HIERARCHY_DELIMITER	"/"
#define ALIAS_DELIMITER "~>"

#define IsMos_(theDeviceType) ((theDeviceType) == NMOS || (theDeviceType) == PMOS || (theDeviceType) == LDDN || (theDeviceType) == LDDP)
#define IsNmos_(theDeviceType) ((theDeviceType) == NMOS || (theDeviceType) == LDDN)
#define IsPmos_(theDeviceType) ((theDeviceType) == PMOS || (theDeviceType) == LDDP)

#define IsLesserVoltage_(theFirstVoltage, theSecondVoltage) (theFirstVoltage != UNKNOWN_VOLTAGE && theSecondVoltage != UNKNOWN_VOLTAGE && theFirstVoltage < theSecondVoltage)
#define IsLesserOrEqualVoltage_(theFirstVoltage, theSecondVoltage) (theFirstVoltage != UNKNOWN_VOLTAGE && theSecondVoltage != UNKNOWN_VOLTAGE && theFirstVoltage <= theSecondVoltage)
#define IsEqualVoltage_(theFirstVoltage, theSecondVoltage) (theFirstVoltage != UNKNOWN_VOLTAGE && theSecondVoltage != UNKNOWN_VOLTAGE && theFirstVoltage == theSecondVoltage)
#define IsGreaterVoltage_(theFirstVoltage, theSecondVoltage) (theFirstVoltage != UNKNOWN_VOLTAGE && theSecondVoltage != UNKNOWN_VOLTAGE && theFirstVoltage > theSecondVoltage)
#define IsGreaterOrEqualVoltage_(theFirstVoltage, theSecondVoltage) (theFirstVoltage != UNKNOWN_VOLTAGE && theSecondVoltage != UNKNOWN_VOLTAGE && theFirstVoltage >= theSecondVoltage)

template <typename T> T from_string(std::string const & s) {
    std::stringstream ss(s);
    T result;
    static const std::unordered_map<std::string, float> SISuffix = {
       {"a", 1e-18},
       {"f", 1e-15},
       {"p", 1e-12},
       {"n", 1e-9},
       {"u", 1e-6},
       {"m", 1e-3},
       {"K", 1e3},
       {"M", 1e6},
       {"G", 1e9},
       {"T", 1e12},
       {"P", 1e15},
       {"E", 1e18}
    };
    std::string mySuffix = "";
    ss >> result;    // TODO handle errors
    ss >> mySuffix;
    if ( mySuffix != "" ) {
    	result = result * SISuffix.at(mySuffix);
    }
    return result;
}

template <typename T> std::string to_string(T theValue) {
	std::ostringstream convert;
	convert << theValue;
	return convert.str();
}

enum relation_t { equals, lessThan, greaterThan, notLessThan, notGreaterThan };

// also used as bit offset in source connection status. do not use any offsets after fuse
enum modelType_t { NMOS = 0, PMOS, RESISTOR, CAPACITOR, DIODE, BIPOLAR, FUSE_ON, FUSE_OFF,
	SWITCH_ON, SWITCH_OFF, MOSFET, LDDN, LDDP, BOX, UNKNOWN };

enum eventQueue_t { SIM_QUEUE, MAX_QUEUE, MIN_QUEUE };

enum queueStatus_t { ENQUEUE, DEQUEUE };

enum deviceStatus_t { MIN_INACTIVE = 0, MIN_PENDING, MAX_INACTIVE, MAX_PENDING, SIM_INACTIVE, SIM_PENDING };

enum netStatus_t { ANALOG = 0, MIN_POWER, SIM_POWER, MAX_POWER, NEEDS_MIN_CHECK, NEEDS_MAX_CHECK, NEEDS_MIN_CONNECTION, NEEDS_MAX_CONNECTION };
// NEEDS_MIN/MAX_CHECK for min/max prop through pmos/nmos
// NEEDS_MIN/MAX_CONNECTION for pmos/nmos connected as diode

enum shortDirection_t { SOURCE_TO_MASTER_DRAIN, DRAIN_TO_MASTER_SOURCE };

enum terminal_t { GATE = 1, SOURCE = 2, GS, DRAIN = 4, GD, SD, GSD, BULK = 8, GB, SB, GSB, DB, GDB, SDB, GSDB };

enum propagation_t { POWER_NETS_ONLY = 1, ALL_NETS_NO_FUSE, ALL_NETS_AND_FUSE };

enum cvcError_t { LEAK = 0, HIZ_INPUT, FORWARD_DIODE, NMOS_SOURCE_BULK, NMOS_GATE_SOURCE, NMOS_POSSIBLE_LEAK,
	PMOS_SOURCE_BULK, PMOS_GATE_SOURCE, PMOS_POSSIBLE_LEAK, OVERVOLTAGE_VBG, OVERVOLTAGE_VBS, OVERVOLTAGE_VDS,
	OVERVOLTAGE_VGS, EXPECTED_VOLTAGE, LDD_SOURCE, MIN_VOLTAGE_CONFLICT, MAX_VOLTAGE_CONFLICT, ERROR_TYPE_COUNT };

// Flag Constants

#define PRINT_CIRCUIT_ON	true
#define PRINT_CIRCUIT_OFF	false
#define PRINT_HIERARCHY_ON	true
#define PRINT_HIERARCHY_OFF	false

#define PRINT_DEVICE_LIST_ON	true
#define PRINT_DEVICE_LIST_OFF	false

#define ZERO_DELAY	0

#define RESISTOR_ONLY 0x04

// teebuf from http://wordaligned.org/articles/cpp-streambufs

#include <streambuf>

class teebuf: public std::streambuf
{
public:
    // Construct a streambuf which tees output to both input
    // streambufs.
    teebuf(std::streambuf * sb1, std::streambuf * sb2)
        : sb1(sb1)
        , sb2(sb2)
    {
    }
private:
    // This tee buffer has no buffer. So every character "overflows"
    // and can be put directly into the teed buffers.
    virtual int overflow(int c)
    {
        if (c == traits_type::eof())
        {
            return !traits_type::eof();
        }
        else
        {
            int const r1 = sb1->sputc(c);
            int const r2 = sb2->sputc(c);
            return r1 == traits_type::eof() || r2 == traits_type::eof() ? traits_type::eof() : c;
        }
    }

    // Sync both teed buffers.
    virtual int sync()
    {
        int const r1 = sb1->pubsync();
        int const r2 = sb2->pubsync();
        return r1 == 0 && r2 == 0 ? 0 : -1;
    }
private:
    std::streambuf * sb1;
    std::streambuf * sb2;
};

class teestream : public std::ostream
{
public:
    // Construct an ostream which tees output to the supplied
    // ostreams.
    teestream(std::ostream & o1, std::ostream & o2) : std::ostream(&tbuf), tbuf(o1.rdbuf(), o2.rdbuf()) {};
private:
    teebuf tbuf;
};

// end teebuf

#endif /* CVCTYPES_HH_ */
