""" SummaryFile.py: Read and hold summary file data class.

    Copyright 2016, 2017 D. Mitch Bailey  cvc at shuharisystem dot com

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

from __future__ import division
from __future__ import print_function

import re
import os
from operator import itemgetter

import cvc_globals

from utility import OpenFile
    
class SummaryFile():
    """ Class SummaryFile: Class for check_cvc summary file handling.

    A summary file consists of lines in the format:
      #ALWAYS <- subsequent lines apply to all modes
      #IF mode1 [mode2 ...] <- subsequent lines apply all to only listed modes
      #IFNOT mode1 [mode2 ...] <- subsequent lines do not apply to listed modes
      #comment [reference] level:section *:details
      [reference] level:section *:details 
        where level is one of ERROR, Warning, Check, ignore and section is from cvc_globals
    
    Class variables:
      countRE: regular expression to get error counts
    Instance variables:
      summaryFileName: Name of the summary file.
      summaryLine: List of non-blank lines in the summary file.
    Methods:
      __init__: Create SummaryFile object from summary file data.
      ExtractSummary: Return a sorted summary list for a specific mode.
        See ResultFile.CreateDisplayList for list record details
    """
    countRE = re.compile("^(.*INFO: .* error count )([0-9]+)/([0-9]+)")
    
    def __init__(self, theFileName):
        """Create SummaryFile object from summaryFile

        Inputs: theFileName - possible compressed summary file name
        """
        mySummaryFile = OpenFile(theFileName)
        self.summaryFileName = os.path.abspath(theFileName)
        self.summaryLine = []
        for line_it in mySummaryFile:
            myLine = line_it.strip()
            if myLine:
            	self.summaryLine.append(myLine)
        mySummaryFile.close()

    def ExtractSummary(self, theModeName):
        """Return sorted summary list for specified mode. Sorted by error priority and error data.

        Inputs: theModeName - name of mode
        Returns: sorted list of summary lines applicable to the specified mode.

        See SummaryFile class docstring for summary formats.
        """
        myActiveModeList = []
        myInactiveModeList = []
        mySummaryList = []
        for summary_it in self.summaryLine:
            if summary_it.startswith("#ALWAYS"):
                myActiveModeList = []
                myInactiveModeList = []
                continue
            elif summary_it.startswith("#IF "):
                myActiveModeList = summary_it.split()[1:]
                myInactiveModeList = []
                continue
            elif summary_it.startswith("#IFNOT "):
                myActiveModeList = []
                myInactiveModeList = summary_it.split()[1:]
                continue
            if myActiveModeList and theModeName not in myActiveModeList:
                continue
            if myInactiveModeList and theModeName in myInactiveModeList:
                continue
            myTokens = []
            myData = summary_it
            myLevel = 'unknown'
            if summary_it.startswith("#"):  # comment lines
                myTokens = summary_it.split()  # (#comment [reference] level:section *:details)
                myReference = myTokens[0] + " "  # -> '#comment '
                myData = " ".join(myTokens[1:])  # -> [reference] level:section *:details
                myTokens = myData.split(":")  # ([reference] level:section *:details)
            else:            
                myTokens = summary_it.split(":")  # ([reference] level:section *:details)
                myReference = ""
            if myTokens[0].startswith("["):  # [reference] level
                mySection = myTokens[1].split()[0]  # (section *)
                myData = ":".join(myTokens[1:])  # -> section *:details
                myReference += myTokens[0]  # += [reference] level
                myCheckLevel = myReference.split()[-1]  # ([reference] level)
                if myCheckLevel not in cvc_globals.validLevels:
                    mySection = 'summary_error'
                elif myLevel == 'unknown':
                    myLevel = myCheckLevel
            else:
                mySection = 'summary_error'
            if mySection in cvc_globals.priorityMap:
                myData = myData.strip()
                myMatch = self.countRE.match(myData)
                if myMatch:
                    if myMatch.group(2) == myMatch.group(3):  # all instance have errors
                        myKeyData = myMatch.group(1) + "all"
                    else:
                        myKeyData = myMatch.group(1) + myMatch.group(2)
                else:
                    myKeyData = myData
                mySummaryList.append( 
                    {'priority': cvc_globals.priorityMap[mySection], 
                     'reference': myReference, 
                     'level': myLevel,
                     'keyData': myKeyData,
                     'data': myData})
            else:
                print("Unknown format ignored:", summary_it)
        return sorted(mySummaryList, key=itemgetter('priority', 'keyData'))

#23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
