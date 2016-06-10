#! /usr/bin/env python
""" expand_cells.py: Create a list of cells to expand or ignore from CDL netlist for calibre SVS

    Copyright 2106 D. Mitch Bailey

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

from __future__ import division

import sys
import getopt
import gzip
import re
import os
import copy

def ReadCellOverrides(theOverrideCellFileName):
    """Return a dictionary of cell override settings.

    Sample lines:
    EXPAND cell1 <- cell1 will be expanded (flattened) unconditionally (LVS EXCLUDE HCELL)
    KEEP cell2   <- cell2 will remain in the final hierarchy unless parameterized
    IGNORE cell3 <- contents of cell3 will be removed from the final netlist (LVS BOX CELL)
    # comment

    Other formats result in fatal errors.
    Duplicate settings that don't match also result in fatal errors.
    """
    myCellOverrides = {}
    myOverrideFilterRE = re.compile('^(EXPAND|KEEP|IGNORE)\s+(\S+)$')
    myCommentRE = re.compile('^#')
    myOverrideError = False
    myCellOverrideFile = OpenFile(theOverrideCellFileName)
    for cell_it in myCellOverrideFile:
        myLine = cell_it.strip()
        if not myLine: continue  # Skip blank lines
        myMatch = myOverrideFilterRE.match(myLine)
        if myMatch:
            myOverride = myMatch.group(1)
            myCell = myMatch.group(2)
            if myCell in myCellOverrides and myCellOverrides[myCell] != myOverride:
                print("ERROR: conflicting override " + myLine 
                      + " != " + myCellOverrides[myCell] + " " + myCell)
                myOverrideError = True
            else:
                myCellOverrides[myCell] = myOverride
        elif not myCommentRE.search(myLine):
            print("ERROR: invalid format " + myLine)
            myOverrideError = True
    if myOverrideError: raise
    return myCellOverrides

def OpenFile(theFileName):
    """Open a file (possibly compressed gz) and return file"""
    try:
        if theFileName.endswith(".gz"):
            myFile = gzip.open(theFileName, mode="rt")
        else:
            myFile = open(theFileName)
    except IOError as myErrorDetail:
        print("ERROR: Could not open " + theFileName + " " + str(myErrorDetail.args))
        raise IOError
    return myFile

gNetlist = {}
gBoxlist = {}

def ReadNetlist(theCDLFileName, theCellOverrideList):
    """Read a CDL netlist and return as a list with continuation lines merged"""
    mySubcktStartRE = re.compile("^\.[sS][uU][bB][cC][kK][tT]\s+(\S+)")
    myCDLFile = OpenFile(theCDLFileName)
    myContinuationRE = re.compile("^\+")
    myCommentRE = re.compile("^\*")
    myBlankRE = re.compile("^\s*$")
    myLine = ""
    myLongLines = []
    for line_it in myCDLFile:
        if myCommentRE.search(line_it): continue  #ignore comments
        if myBlankRE.match(line_it): continue  #ignore blank lines
        line_it.replace(" /", " ")  # remove optional subcircuit delimiter 
        if myContinuationRE.search(line_it):
            myLine += re.sub("^\+", " ", line_it)
        else:
            if myLine:
                myMatch = mySubcktStartRE.search(myLine)
                if myMatch:
                    myName = myMatch.group(1)
                    gNetlist[myName] = {
                        'instances': {}, 'mos_models': {},
                        'resistor_count': 0, 'other_count': 0, 'checked': False
                    }
                    if myName in theCellOverrideList and theCellOverrideList[myName] = 'IGNORE':
                        gNetlist[mySubcircuitName]['small'] = True
                myLongLines.append(myLine)
            myLine = line_it
    if myLine != myLongLines[-1]:
        myLongLines.append(myLine)
    return myLongLines

def AnalyzeNetlist(theSubcircuits, theCellOverrideList):
    """Count instances, mos models, resistor in each subckt and return hierarchy dictionary."""
    mySubcktStartRE = re.compile("^\.[sS][uU][bB][cC][kK][tT]\s+(\S+)")
    mySubcktEndRE = re.compile("^\.[eE][nN][dD][sS]")
    myInstanceRE = re.compile("^[xX]")
    myMosRE = re.compile("^[mM]\S*\s+(\S+)\s+\S+\s+(\S+)\s+\S+\s+(\S+)")
    myResistorRE = re.compile("^[rR]\S*\s+(\S+)\s+(\S+)")
    myParameterRE = re.compile("([^=\s]+=[^=\s]+)|(\$\S+)")
    myCircuit = None
    for line_it in theSubcircuits:
        myMatch = mySubcktStartRE.search(line_it)
        if myMatch:
            mySubcircuitName = myMatch.group(1)
            #myNetlist[mySubcircuitName] = {
                #'instances': {}, 'mos_models': {}, 'resistor_count': 0, 'checked': False
            #}
            myCircuit = gNetlist[mySubcircuitName]
            continue

        if myInstanceRE.search(line_it):
            myInstance = ""
            for word_it in line_it.split():
                myWord = word_it.strip()
                if myParameterRE.match(myWord): 
                    if not re.match("\$PINS", myWord):
                        #if myInstance not in myNetlist:
                            #myNetlist[myInstance] = {
                            	#'instances': {}, 'mos_models': {},
                                #'resistor_count': 0, 'checked': False
                            #}
                        gNetlist[myInstance]['small'] = True  # Expand instances with parameters.
                    break  # Stop at first parameter.
                else:
                    myInstance = myWord  # Last word before first parameter
            if not small in gNetlist[myInstance]:
                if myInstance not in myCircuit['instances']:
                    myCircuit['instances'][myInstance] = 0
                myCircuit['instances'][myInstance] += 1
            continue

        myMatch = myMosRE.search(line_it)
        if myMatch and myMatch.group(1) != myMatch.group(2):  # Skip S=D MOS.
            myMosModel = myMatch.group(3)
            if myMosModel not in myCircuit['mos_models']:
                myCircuit['mos_models'][myMosModel] = 0 
            myCircuit['mos_models'][myMosModel] += 1 
            continue

        myMatch = myResistorRE.search(line_it)
        if myMatch and myMatch.group(1) != myMatch.group(2):  # Skip S=D resistor.
            myCircuit['resistor_count'] += 1
            continue

        if mySubcktEndRE.search(line_it): 
            myCircuit = None
        else:
            myCircuit['other_count'] += 1

def PrintSmallCells(theCellOverrideList, theTopCell):
    """Print list of cells to expand into parent.

    Following cells are expanded
         cellname contains ICV_ or $$
      or override file forced EXPAND
      or 1 instance and no mos or resistor
      or no instances and 1 mos type and < 5 transistors
      or no instances and no mos
    unless override file forced KEEP and no parameters
    """
    myCircuit = gNetlist[theTopCell]
#    if myCircuit['checked'] == True: return  # Already processed
    myCircuit['checked'] = True
    for instance_it in myCircuit['instances'].keys():
        if not gNetlist[instance_it]['checked']:
            PrintSmallCells(theCellOverrideList, instance_it)  # Recursive call
        if instance_it in gBoxlist:
            gBoxlist[instance_it] += 1
        if 'small' in gNetlist[instance_it]:
            # Smash the small cells into the parent cells
            for subinstance_it in gNetlist[instance_it]['instances']:
                if subinstance_it not in myCircuit['instances']:
                    myCircuit['instances'][subinstance_it] = 0
                myCircuit['instances'][subinstance_it] += (
                    gNetlist[instance_it]['instances'][subinstance_it] 
                    * myCircuit['instances'][instance_it]
                )
            for mos_it in gNetlist[instance_it]['mos_models']:
                if mos_it not in myCircuit['mos_models']:
                    myCircuit['mos_models'][mos_it] = 0
                myCircuit['mos_models'][mos_it] += (
                    gNetlist[instance_it]['mos_models'][mos_it] 
                    * myCircuit['instances'][instance_it]
                )
            myCircuit['resistor_count'] += gNetlist[instance_it]['resistor_count']
            del gNetlist[theTopCell]['instances'][instance_it]

    mySmashFlag = True
    myInstanceCount = 0
    for instance_it in myCircuit['instances']:
        myInstanceCount += myCircuit['instances'][instance_it]
    if myInstanceCount > 1:
        mySmashFlag = False  # Keep circuits with >= 2 instance
    if len(myCircuit['mos_models']) > 1:
        mySmashFlag = False  # Keep circuits with >= 2 mos models
    myTotalMosCount = 0
    for mos_it in myCircuit['mos_models']:
        myTotalMosCount += myCircuit['mos_models'][mos_it]
    if myTotalMosCount > 4:
        mySmashFlag = False  # Keep circuits with >= 5 mos devices
    if myInstanceCount == 1 and (myCircuit['resistor_count'] + myTotalMosCount) > 0:
        mySmashFlag = False  # Keep circuits with 1 instance and a mos or resistor
    if re.search("ICV_", theTopCell) or re.search("\$\$", theTopCell):
        mySmashFlag = True  # Smash circuits with ICV_ or $$ in cell name
    if not (myCircuit[instances] or myCircuit[mos_models]
            or myCircuit['resistor_count'] or myCircuit['other_count']):
        gBoxlist[theTopCell] = 0
    if theTopCell in theCellOverrideList and theCellOverrideList[theTopCell] == 'KEEP':
        mySmashFlag = False
    if mySmashFlag or (theTopCell in theCellOverrideList 
                       and theCellOverrideList[theTopCell] == 'EXPAND'):
        myCircuit['small'] = True
        print('LVS EXCLUDE HCELL ' + theTopCell)
#    print("cell %s, instance count %d, mos model count %d, total mos count %d, resistor count %d"
#          %  (theTopCell, myInstanceCount, len(myCircuit['mos_models']), 
#              myTotalMosCount, myCircuit['resistor_count']))

def PrintIgnoreCells(theCellOverrideList):
    """Print cells that were ignored in override file."""
    for cell_it in theCellOverrideList:
        if theCellOverrideList[cell_it] == 'IGNORE':
            print('LVS BOX ' + cell_it)
            print('LVS FILTER ' + cell_it + " OPEN")

def PrintBoxCellUsage():
    """Print box cell usage as comment."""
    for cell_it in sorted(gBoxlist):
        print("// box " + cell_it + ": " + str(gBoxlist[cell_it]))

def main(argv):
    """Print list of cells to expand during Calibre SVS

    usage: expand_cells.py overrideCellFile topCell cdlFile[.gz]
    """
    options, arguments = getopt.getopt(argv, "?")
    if len(argv) != 3:
        print("usage: expand_cells.py overrideCellFile topCell cdlFile[.gz]")
        raise "Usage Error"
    myCellOverrideList = ReadCellOverrides(argv[0])
    mySubcircuits = ReadNetlist(argv[2], myCellOverrideList)
    AnalyzeNetlist(mySubcircuits, myCellOverrideList)
    PrintIgnoreCells(myCellOverrideList)
    PrintSmallCells(myCellOverrideList, argv[1])
    PrintBoxCellUsage()

if __name__ == '__main__':
    main(sys.argv[1:])

#23456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789
