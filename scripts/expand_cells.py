#! /usr/bin/env python
""" expand_cells.py: Create a list of cells to expand or ignore from CDL netlist for calibre SVS

    Copyright 2106-2018 D. Mitch Bailey  cvc at shuharisystem dot com

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
if sys.hexversion < 0x02060000:
    sys.exit("Python 2.6 or newer is required to run this program.")

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
    MOSFET cell4 <- cell4 will be treated as a mosfet
    RESISTOR cell5 <- cell4 will be treated as a resistor
    # comment

    Other formats result in fatal errors.
    Duplicate settings that don't match also result in fatal errors.
    """
    myCellOverrides = {}
    myOverrideFilterRE = re.compile('^(EXPAND|KEEP|IGNORE|MOSFET|RESISTOR)\s+(\S+)')
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
#               print("ERROR: conflicting override " + myLine
#                     + " != " + myCellOverrides[myCell] + " " + myCell)
                print >> sys.stderr, "ERROR: conflicting override " + myLine + " != " + myCellOverrides[myCell] + " " + myCell
                myOverrideError = True
            else:
                myCellOverrides[myCell] = myOverride
        elif not myCommentRE.search(myLine):
            print >> sys.stderr, "ERROR: invalid format " + myLine
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
        print >> sys.stderr, "ERROR: Could not open " + theFileName + " " + str(myErrorDetail.args)
        raise IOError
    return myFile

gNetlist = {}
gBoxList = {}
gParameterList = {}

def ReadNetlist(theCDLFileName, theCellOverrideList):
    """Read a CDL netlist and return as a list with continuation lines merged"""
    mySubcktStartRE = re.compile("^\.[sS][uU][bB][cC][kK][tT]\s+(\S+)")
    myCDLFile = OpenFile(theCDLFileName)
    myLine = ""
    myLongLines = []
    for line_it in myCDLFile:
        if line_it.startswith("*"): continue  #ignore comments
        if not line_it.strip(): continue  #ignore blank lines
        if line_it.startswith(".INCLUDE"):
            myWordList = line_it.split()
            myIncludeFile = myWordList[1]
            """
            To read INCLUDE "$dir/rules.antenna" statement
                1. Filename enclosed by quotation marks (")
                2. Using env variable ($...)
            """
            myIncludeFile = re.sub(r'"', '', myIncludeFile)     #1
            if re.match(r'\$[^/]+', myIncludeFile):
                myIncludeFile = os.path.expandvars(myIncludeFile)   #2
            print >> sys.stderr, "Reading from " + myIncludeFile
            myLongLines += ReadNetlist(myIncludeFile, theCellOverrideList)
        elif line_it.startswith("+"):
            myLine += " " + line_it[1:]
        else:
            if myLine:
                if myLine.startswith("."):
                    myMatch = mySubcktStartRE.search(myLine)
                    if myMatch:
                        myName = myMatch.group(1)
                        gNetlist[myName] = {
                            'instances': {}, 'mos_models': {},
                            'resistor_count': 0, 'other_count': 0, 'checked': False
                        }
                        if (myName in theCellOverrideList
                                and theCellOverrideList[myName] == 'IGNORE'):
                            gNetlist[myName]['small'] = True
                myLongLines.append(myLine.replace(" /", " "))
            myLine = line_it
    if myLine != myLongLines[-1]:
        myLongLines.append(myLine.replace(" /", " "))
    return myLongLines

def CountResistor(theCircuit, theLine, theRE):
    myMatch = theRE.search(theLine)
    if myMatch and myMatch.group(1) != myMatch.group(2):  # Skip S=D resistor.
        theCircuit['resistor_count'] += 1
    else:
        theCircuit['other_count'] += 1

def CountMosfet(theCircuit, theLine, theRE):
    myMatch = theRE.search(theLine)
    if myMatch and myMatch.group(1) != myMatch.group(2):  # Skip S=D MOS.
        myMosModel = myMatch.group(3)
        if myMosModel not in theCircuit['mos_models']:
            theCircuit['mos_models'][myMosModel] = 0
        theCircuit['mos_models'][myMosModel] += 1
    else:
        theCircuit['other_count'] += 1

def AnalyzeNetlist(theSubcircuits, theCellOverrideList):
    """Count instances, mos models, resistor in each subckt and return hierarchy dictionary."""
    mySubcktStartRE = re.compile("^\.[sS][uU][bB][cC][kK][tT]\s+(\S+)")
    mySubcktEndRE = re.compile("^\.[eE][nN][dD][sS]")
    myPinRE = re.compile("\$PINS")
    myMosRE = re.compile("^[mMxX]\S*\s+(\S+)\s+\S+\s+(\S+)\s+\S+\s+(?:/\s+){,1}(\S+)")
    myResistorRE = re.compile("^[rRxX]\S*\s+(\S+)\s+(?:/\s+){,1}(\S+)")
    myParameterRE = re.compile("([^=\s]+=[^=\s]+)|(\$\S+)")
    myCircuit = None
    for line_it in theSubcircuits:
        if line_it.startswith("X") or line_it.startswith("x"):
            myInstance = ""
            myWordList = line_it.split()
            if myParameterRE.match(myWordList[-1]):
                for word_it in myWordList:
    #                myWord = word_it.strip()
                    if myParameterRE.match(word_it):
                        if not myPinRE.match(word_it):
                            #myInstance = myInstance.replace("/", "")
                            try:
                                gNetlist[myInstance]['small'] = True  # Expand parameterized instances.
                            except KeyError:
                                print >> sys.stderr, "Missing subcircuit definition for " + myInstance
                            if not myInstance in gParameterList:
                                gParameterList[myInstance] = 0
                            gParameterList[myInstance] += 1
                        break  # Stop at first parameter.
                    else:
                        myInstance = word_it  # Last word before first parameter
            else:
                myInstance = myWordList[-1]
            #myInstance = myInstance.replace("/", "")
            if myInstance in theCellOverrideList and theCellOverrideList[myInstance] == 'RESISTOR':
                CountResistor(myCircuit, line_it, myResistorRE)
            elif myInstance in theCellOverrideList and theCellOverrideList[myInstance] == 'MOSFET':
                CountMosfet(myCircuit, line_it, myMosRE)
            elif myInstance not in myCircuit['instances']:
                myCircuit['instances'][myInstance] = 1
            else:
                myCircuit['instances'][myInstance] += 1
        elif line_it.startswith("R") or line_it.startswith("r"):
            CountResistor(myCircuit, line_it, myResistorRE)
        elif line_it.startswith("M") or line_it.startswith("m"):
            CountMosfet(myCircuit, line_it, myMosRE)
        elif line_it.startswith("."):
            myMatch = mySubcktStartRE.search(line_it)
            if myMatch:
                mySubcircuitName = myMatch.group(1)
                myCircuit = gNetlist[mySubcircuitName]
                myDefinition = line_it
            elif mySubcktEndRE.search(line_it):
                if not (myCircuit['instances'] or myCircuit['mos_models']
                        or myCircuit['resistor_count'] or myCircuit['other_count']):
                    gBoxList[mySubcircuitName] = myDefinition.strip()
                myCircuit = None
        elif myCircuit:
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
    myCircuit['checked'] = True
    for instance_it in list(myCircuit['instances'].keys()):
        if instance_it not in gNetlist:
            print >> sys.stderr, "missing subcircuit definition for " + instance_it
            continue
        if not gNetlist[instance_it]['checked']:
            PrintSmallCells(theCellOverrideList, instance_it)  # Recursive call
        if ('small' in gNetlist[instance_it]
                and not (instance_it in gBoxList and instance_it in gParameterList)):
            # Smash the small cells into the parent cells (keep parameterized boxes)
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
                myCircuit['mos_models'][mos_it] += (gNetlist[instance_it]['mos_models'][mos_it]
                                                    * myCircuit['instances'][instance_it])
            myCircuit['resistor_count'] += (gNetlist[instance_it]['resistor_count']
                                            * myCircuit['instances'][instance_it])
            myCircuit['other_count'] += (gNetlist[instance_it]['other_count']
                                            * myCircuit['instances'][instance_it])
            del gNetlist[theTopCell]['instances'][instance_it]

    mySmashFlag = True
    myInstanceCount = 0
    for instance_it in myCircuit['instances']:
        if instance_it not in gNetlist:
            print >> sys.stderr, "missing subcircuit definition for " + instance_it
            continue
        if not 'small' in gNetlist[instance_it]:  # Don't count small instances
            myInstanceCount += myCircuit['instances'][instance_it]
    if myInstanceCount > 1:
        mySmashFlag = False  # Keep circuits with >= 2 instance
    if len(myCircuit['mos_models']) > 1:
        mySmashFlag = False  # Keep circuits with >= 2 mos models
    myTotalMosCount = 0
    for mos_it in myCircuit['mos_models']:
        myTotalMosCount += myCircuit['mos_models'][mos_it]
#    if myTotalMosCount > 4:
#        mySmashFlag = False  # Keep circuits with >= 5 mos devices
    if myInstanceCount == 1 and (myCircuit['resistor_count'] + myTotalMosCount) > 0:
        mySmashFlag = False  # Keep circuits with 1 instance and a mos or resistor
    if re.search("ICV_", theTopCell) or re.search("\$\$", theTopCell):
        mySmashFlag = True  # Smash circuits with ICV_ or $$ in cell name
    if theTopCell in gBoxList and theTopCell in gParameterList:
        mySmashFlag = False
    if theTopCell in theCellOverrideList:  # Override calculation
        if theCellOverrideList[theTopCell] == 'KEEP':
            mySmashFlag = False
        elif theCellOverrideList[theTopCell] == 'EXPAND':
            mySmashFlag = True
    if mySmashFlag:
        myCircuit['small'] = True
        print("LVS EXCLUDE HCELL %s // X %d, MM %d, M %d, R %d, ? %d" % (
            theTopCell, myInstanceCount, len(myCircuit['mos_models']),
            myTotalMosCount, myCircuit['resistor_count'], myCircuit['other_count']
        ))
        
def PrintIgnoreCells(theCellOverrideList):
    """Print cells that were ignored in override file."""
    for cell_it in theCellOverrideList:
        if theCellOverrideList[cell_it] == 'IGNORE':
            print('LVS BOX ' + cell_it)
            print('LVS FILTER ' + cell_it + " OPEN")

def PrintBoxCellUsage():
    """Print box cell usage as comment."""
    for cell_it in sorted(gBoxList):
        if cell_it in gParameterList:
            print("// box " + cell_it + ": " + str(gParameterList[cell_it]))
            print("//" + str(gBoxList[cell_it]))

def main(argv):
    """Print list of cells to expand during Calibre SVS

    usage: expand_cells.py overrideCellFile topCell cdlFile[.gz]
    """
    options, arguments = getopt.getopt(argv, "?")
    if len(argv) != 3:
        print >> sys.stderr, "usage: expand_cells.py overrideCellFile topCell cdlFile[.gz]"
        return
    myCellOverrideList = ReadCellOverrides(argv[0])
    mySubcircuits = ReadNetlist(argv[2], myCellOverrideList)
    AnalyzeNetlist(mySubcircuits, myCellOverrideList)
    PrintIgnoreCells(myCellOverrideList)
    PrintSmallCells(myCellOverrideList, argv[1])
    PrintBoxCellUsage()

if __name__ == '__main__':
    main(sys.argv[1:])

#23456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789
