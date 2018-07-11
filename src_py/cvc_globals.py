"""cvc_globals: Definitions of global lists for check_cvc

    The order of errors in errorList is the priority that the errors are displayed in.
    The priorityMap is generated from errorList.

    Functions:
      InitializeErrors: Initialize error data from cvc_globals.

    Copyright 2016-2018 D. Mitch Bailey  cvc at shuharisystem dot com

    This file is part of check_cvc.

    check_cvc is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    check_cvc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with check_cvc.  If not, see <http://www.gnu.org/licenses/>.

    You can download check_cvc from https://github.com/d-m-bailey/check_cvc.git
"""

import re

validLevels = ['ERROR', 'Warning', 'Check', 'ignore']
priorityMap = {}  # section: priority
errorList = [ \
    {'source': 'log', 'section': 'resistor',
     'searchText': "^INFO: resistor voltage calculation"},
    {'source': 'log', 'section': 'unsupported_device',
     'searchText': "^WARNING: unsupported devices in netlist"},
    {'source': 'log', 'section': 'min_voltage_ignore',
     'searchText': "^WARNING: min voltage .* ignored for "},
    {'source': 'log', 'section': 'max_voltage_ignore',
     'searchText': "^WARNING: max voltage .* ignored for "},
    {'source': 'log', 'section': 'unexpected_max_voltage',
     'searchText': "^WARNING: Max voltage already set for"},
    {'source': 'log', 'section': 'unexpected_min_voltage',
     'searchText': "^WARNING: Min voltage already set for"},
    {'source': 'log', 'section': 'unknown_max_gate',
     'searchText': "^WARNING: unknown max gate at"},
    {'source': 'log', 'section': 'unknown_min_gate',
     'searchText': "^WARNING: unknown min gate at"},
    {'source': 'log', 'section': 'fuse_only',
     'searchText': "^WARNING: MIN = MAX .* through FUSE"},
    {'source': 'log', 'section': 'resistor_error',
     'searchText': "^WARNING: Could not calculate explicit resistor voltage"},
    {'source': 'log', 'section': 'inverter_error',
     'searchText': "^WARNING: could not set .* ->inv-> "},
    {'source': 'log', 'section': 'port_min/max',
     'searchText': "^WARNING: Invalid Min/Max on top level port"},
    {'source': 'log', 'section': 'min_exceeds_sim',
     'searchText': "^WARNING: MIN > SIM"},
    {'source': 'log', 'section': 'sim_exceeds_max',
     'searchText': "^WARNING: MAX < SIM"},
    {'source': 'log', 'section': 'high_res_bias',
     'searchText': "^WARNING: high res bias"},
    {'source': 'log', 'section': 'missing_bias',
     'searchText': "^WARNING: missing bias"},
    {'source': 'log', 'section': 'missing_overvoltage',
     'searchText': "^WARNING: no overvoltage check for model"},
    {'source': 'log', 'section': 'possible_missing_input',
     'searchText': "^WARNING: possible missing input at net"},
    {'source': 'log', 'section': 'resistance_overflow',
     'searchText': "^WARNING: resistance "},
    {'source': 'log', 'section': 'unexpected_diode',
     'searchText': "^INFO: unexpected diode"},
    {'source': 'report', 'section': 'MOS_DIODE',
     'searchText': "^! ... voltage already"},
    {'source': 'report', 'section': 'SHORT',
     'searchText': "^! Short Detected"},
    {'source': 'report', 'section': 'BIAS',
     'searchText': "^! Checking .mos source/drain vs bias errors"},
    {'source': 'report', 'section': 'FORWARD',
     'searchText': "^! Checking forward bias diode errors"},
    {'source': 'report', 'section': 'FLOATING',
     'searchText': "^! Checking mos floating input errors"},
    {'source': 'report', 'section': 'GATE',
     'searchText': "^! Checking .mos gate vs source errors"},
    {'source': 'report', 'section': 'LEAK?',
     'searchText': "^! Checking .mos possible leak errors"},
    {'source': 'report', 'section': 'VALUE',
     'searchText': "^! Checking expected values"},
    {'source': 'report', 'section': 'LDD',
     'searchText': "^! Checking LDD errors for model"},
    {'source': 'report', 'section': 'OVERVOLTAGE_VBG',
     'searchText': "^! Checking Vbg overvoltage errors"},
    {'source': 'report', 'section': 'OVERVOLTAGE_VBS',
     'searchText': "^! Checking Vbs overvoltage errors"},
    {'source': 'report', 'section': 'OVERVOLTAGE_VDS',
     'searchText': "^! Checking Vds overvoltage errors"},
    {'source': 'report', 'section': 'OVERVOLTAGE_VGS',
     'searchText': "^! Checking Vgs overvoltage errors"},
    {'source': 'report', 'section': 'UNKNOWN',
     'searchText': "^! Finished"}
]

def InitializeErrors():
    """Initialize error data from cvc_globals.

    Modifies:
      errorList - adds 'priority' and 'regex'
      priorityMap - error section to priority map (priority is order in errorList)
    """
    myIndex = 0
    for myError in errorList:
        myError['priority'] = myIndex
        myError['regex'] = re.compile(myError['searchText'])
        priorityMap[myError['section']] = myIndex
        myIndex += 1

#23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
