/*
 * CvcMaps.hh
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

#ifndef CVCMAPS_HH_
#define CVCMAPS_HH_

#include "Cvc.hh"

extern unordered_map<string, relation_t> gRelationStringMap;

extern map<relation_t, string> gRelationMap;

extern unordered_map<string, modelType_t> gModelTypeStringMap;

extern map<modelType_t, string> gModelTypeMap;

extern map<modelType_t, string> gBaseModelTypePrefixMap;

extern map<eventQueue_t, string> gEventQueueTypeMap;

#define String_to_Voltage(value) (IsValidVoltage_(value) ? round(from_string<float>(value) * VOLTAGE_SCALE + 0.1) : UNKNOWN_VOLTAGE)
#define Voltage_to_float(value) (float(value) / VOLTAGE_SCALE)

#endif /* CVCMAPS_HH_ */
