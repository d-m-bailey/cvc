/*
 * CvcMaps.cc
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

#include "CvcMaps.hh"

unordered_map<string, relation_t> gRelationStringMap({
	{"=", equals},
	{"<", lessThan},
	{">", greaterThan},
	{"=>", notLessThan},
	{">=", notLessThan},
	{"=<", notGreaterThan},
	{"<=", notGreaterThan},
});

map<relation_t, string> gRelationMap({
	{equals, "="},
	{lessThan, "<"},
	{greaterThan, ">"},
	{notLessThan, ">="},
	{notGreaterThan, "<="},
});

unordered_map<string, modelType_t> gModelTypeStringMap({
	{"M", MOSFET},
	{"MN", NMOS},	{"LDDN", LDDN},
	{"MP", PMOS},	{"LDDP", LDDP},
	{"R", RESISTOR},	{"RESISTOR", RESISTOR},
	{"C", CAPACITOR},	{"CAPACITOR", CAPACITOR},
	{"D", DIODE},	{"DIODE", DIODE},
	{"switch_on", SWITCH_ON},
	{"switch_off", SWITCH_OFF},
	{"fuse_on", FUSE_ON},
	{"fuse_off", FUSE_OFF},
	{"Q", BIPOLAR}, {"BIPOLAR", BIPOLAR},
	{"BOX", BOX}
});

map<modelType_t, string> gModelTypeMap({
	{MOSFET, "mosfet"},
	{NMOS, "nmos"},
	{PMOS, "pmos"},
	{LDDN, "lddn"},
	{LDDP, "lddp"},
	{RESISTOR, "resistor"},
	{CAPACITOR, "capacitor"},
	{DIODE, "diode"},
	{SWITCH_ON, "switch_on"},
	{SWITCH_OFF, "switch_off"},
	{FUSE_ON, "fuse_on"},
	{FUSE_OFF, "fuse_off"},
	{BIPOLAR, "bipolar"},
	{BOX, "box"}
});

map<modelType_t, string> gBaseModelTypePrefixMap({
	{MOSFET, "M"},
	{NMOS, "M"},
	{PMOS, "M"},
	{LDDN, "M"},
	{LDDP, "M"},
	{RESISTOR, "R"},
	{CAPACITOR, "C"},
	{DIODE, "D"},
	{SWITCH_ON, ""},
	{SWITCH_OFF, ""},
	{FUSE_ON, ""},
	{FUSE_OFF, ""},
	{BIPOLAR, "Q"},
	{BOX, "X"}
});

map<eventQueue_t, string> gEventQueueTypeMap({
	{MAX_QUEUE, "MaximumQueue"},
	{MIN_QUEUE, "MinimumQueue"},
	{SIM_QUEUE, "SimulationQueue"},
});



