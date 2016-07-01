""" ResultFile.py: Read and process error file data class

    Copyright 2106 D. Mitch Bailey  d.mitch.bailey at gmail dot com

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

from utility import CompareErrors, OpenFile
    
class ResultFile():
    """ Class ResultFile: Class for check_cvc result file handling.

    Result data is extracted from 2 files. The log file and the error file. 
    The error file name is specified in the log file.
    
    Class variables:
      modeRE: regular expression to get mode name
      TopRE: regular expression to get top cell name
      summaryRE: regular expression to identify error detail lines
      logFileRE: regular expression to get base log file name
      errorFileRE: regular expression to get error file name
    Instance variables:
      topCell: Name of the top ciruit.
      modeName: Name of the CVC mode.
      logFileName: Name of the CVC log file.
      errorFileName: Name of the CVC error file.
      summaryFileName: Name of summary file.
      errorList: Sorted list of errors extracted from log and error files for a single mode.
        See CreateDisplayList for list record details
      displayList: List of display lines that are a result of matching summary list to error list.
        See CreateDisplayList for list record details
      errorCount: List of error totals by type.
      checkCount: List of matched errors by type.
      percentage: Percentage * 10 of errors checked. 
      errorDetails: Slice of error file detailing a specific error.
    Methods:
      __init__: Create ResultFile object from log and error file data.
      ExtractLogErrors: Extract errors from log file.
      ExtractReportErrors: Extract errors from error file and sort the result.
      GetErrorDetails: Return error details extracted from error file.
      CountErrors: Tally errors according to type.
      CreateDisplayList: Create a list of references and details to display.
      PrintTotals: Print summary of errors.
    """
    modeRE = re.compile("^CVC_MODE = '(.*)'")
    topRE = re.compile("^CVC_TOP = '(.*)'")
    summaryRE = re.compile("^(INFO|WARNING)")
    logFileRE = re.compile("^CVC: Log output to (.*)")
    errorFileRE = re.compile("^CVC: Error output to (.*)")
    
    def __init__(self, theLogFileName):
        """Create ResultFile object from log file and error file.

        Inputs:
          theLogFileName: Name of the log file.
        """
        self.modeName = ""
        self.errorFileName = ""
        self._baseLogFileName = ""
        self.topCell = ""
        self.errorList = []
        self.displayList = []
        self._errorData = []

        try:
            myLogFile = OpenFile(theLogFileName)
        except IOError:
            print ("ERROR: Could not open " + theLogFileName)
            raise IOError
        self.ExtractLogErrors(myLogFile)
        myLogFile.close()

        myRelativePath = self._CalculateErrorPath(theLogFileName,
                                                  self._baseLogFileName,
                                                  os.path.basename(self.errorFileName))
        try:
            myErrorFile = OpenFile(myRelativePath)
        except:
            print ("ERROR: Could not open "  + myRelativePath)
            raise IOError    
        self.ExtractReportErrors(myErrorFile)
        myErrorFile.close()

    def _CalculateErrorPath(self, theCurrentLogFileName, theBaseLogFileName, theErrorFileName):
        """Calculate error path name using difference between original and current file name."""
        myDirectory = os.path.dirname(theCurrentLogFileName)
        myCurrentBaseName = os.path.basename(theCurrentLogFileName)
        print("current: " + myCurrentBaseName + ", base: " +  theBaseLogFileName)
        if myCurrentBaseName != theBaseLogFileName:
            mySuffix = myCurrentBaseName.replace(theBaseLogFileName, "")
        else:
            mySuffix = ".gz"
        return(os.path.normpath(os.path.join(myDirectory,
                                             theErrorFileName.replace(".gz", "") + mySuffix)))

    def ExtractLogErrors(self, theFile):
        """Extract errors from log file.

        Inputs: theFile: the log file
        Modifies:
          logFileName: the path of the log file
          errorFileName: the error file listed in the log file
          modeName: the mode name listed in the log file
          topCell: the top circuit listed in the log file
          errorList: adds errors from lines matching log file error definitions in cvc_globals
        """
        self.logFileName = os.path.abspath(theFile.name)
        try:
            for line_it in theFile:
                if not self._baseLogFileName:
                    myMatch = self.logFileRE.search(line_it)  # "^CVC: Log output to (.*)"
                    if myMatch:
                        self._baseLogFileName = os.path.basename(myMatch.group(1))
                if not self.errorFileName:
                    myMatch = self.errorFileRE.search(line_it)  # "^CVC: Error output to (.*)"
                    if myMatch:
                        self.errorFileName = myMatch.group(1)
                if not self.modeName:
                    myMatch = self.modeRE.search(line_it)  # "^CVC_MODE = '(.*)'"
                    if myMatch:
                        self.modeName = myMatch.group(1)
                        print("Reading mode " + self.modeName + " errors from \n\t" + theFile.name)
                if not self.topCell:
                    myMatch = self.topRE.search(line_it)  # "^CVC_TOP = '(.*)'"
                    if myMatch:
                        self.topCell = myMatch.group(1)
                for error_it in cvc_globals.errorList:
                    if error_it['source'] == 'log' and error_it['regex'].search(line_it):
                        self.errorList.append(
                            {'priority': error_it['priority'],
                             'section': error_it['section'],
                             'data': error_it['section'] + " " + line_it.strip()})
                        break
        except IOError:
            print("ERROR: Problem reading " + self.logFileName)
            raise IOError

    def ExtractReportErrors(self, theFile):
        """Extract errors from error file and sort the result.

        Inputs:
          theFile: the error file
        Modifies:
          errorFileName: the absolute path of the error file actually opened
          errorList: add errors from line matching report file error definitions in cvc_globals
            and sorts the result on error priority and data
        """
        self.errorFileName = os.path.abspath(theFile.name)
        print(" and \t" + theFile.name)
        mySection = None
        myPriority = None
        try:
            for line_it in theFile:
                self._errorData.append(line_it)
                for error_it in cvc_globals.errorList:
                    if error_it['source'] == 'report' and error_it['regex'].search(line_it):
                        mySection = error_it['section']
                        myPriority = cvc_globals.priorityMap[mySection]
                        break
                if self.summaryRE.search(line_it):  # "^(INFO|WARNING)"
                    self.errorList.append({'priority': myPriority,
                                           'section': mySection,
                                           'data': mySection + " " + line_it.strip()})
        except IOError:
            print("ERROR: Problem reading " + self.errorFileName)
            raise IOError
        self.errorList = sorted(self.errorList, key=itemgetter('priority', 'data'))
                
    def GetErrorDetails(self, theError):
        """Return error details extracted from error file.

        Inputs:
          theError: selected error record 
        Returns: list of lines from error file detailing the error matching theError.
          If the number of errors is equal to the number of subcircuits, only extract details 
          for 1 error (a sample). If the number of errors is not equal to the number of 
          subcircuits, extract details for up to 10 errors.
        """
        myMatch = re.search(
           "(?:.*:)?(.*) INFO.*SUBCKT ([^\)]*\))([^ ]*) error count ([0-9]+)/([0-9]+)", 
           theError['errorText']
        )  # parse the error text string to get
           # (1) section, (2) top cell name, (3) device name, (4) error count, (5) cell count
        if not myMatch:
            return theError['errorText']  # display the error text as is
        myErrorRecord = cvc_globals.errorList[cvc_globals.priorityMap[myMatch.group(1)]]
        mySectionRE = re.compile(myErrorRecord['searchText'])
        myDeviceRE = None
        myDeviceName = myMatch.group(3).split("(")[0]  # (/name(model))
        if "(" + self.topCell + ")" == myMatch.group(2):
            # top cell errors do not have cell name in detailed error list
            myDeviceRE = re.compile("^" + myDeviceName + " ")
        else:
            myDeviceRE = re.compile("^/.*" + re.escape(myMatch.group(2)) + myDeviceName + " ")
        myBlankRE = re.compile("^\s*$")
        mySectionEndRE = re.compile("^! Finished")
        mySampleOnlyFlag = myMatch.group(4) == myMatch.group(5)  # error count == cell count
        mySaveLeadingFlag = True  # Save lines after blank lines before devices ("at Vth", etc.)
        myCorrectSectionFlag = False
        mySaveLineFlag = False  # Save lines for output
        myOutput = ""
        mySection = ""
        myLeadingLines = ""
        myErrorCount = 0
        for line_it in self._errorData:
            if mySectionRE.search(line_it):  # Use lastest heading for short errors.
                myCorrectSectionFlag = True
                mySection = line_it
            elif myCorrectSectionFlag:
                if myDeviceRE.search(line_it):
                    myErrorCount += 1
                    myOutput += mySection
                    myOutput += myLeadingLines
                    mySaveLeadingFlag = False
                    mySaveLineFlag = True
                if mySaveLineFlag:
                    myOutput += line_it
                if myBlankRE.match(line_it):
                    if myOutput and (mySampleOnlyFlag or myErrorCount > 9):
                        break  # only output first error
                    mySaveLeadingFlag = True
                    myLeadingLines = ""
                    mySaveLineFlag = False
                elif mySaveLeadingFlag:
                    myLeadingLines += line_it if line_it != mySection else ""  # no double lines
            if mySectionEndRE.search(line_it):
                myCorrectSectionFlag = False
                if myOutput:
                    break  # relavent errors only exist in one section
        return myOutput
                
    def CountErrors(self):
        """Tally errors according to type.

        Totals for errors by section, including checked and unchecked error totals.
        Modifies:
          errorCount: list of error counts for error types, error levels, and error sections
          checkCount: list of checked error counts for error sections
          errorDetails: list of "sectionName checkCount/errorCount" for each section and total
            with percentage of errors checked
          percentage: 10 * checkedCount / errorCount
        """
        self.errorCount = {}
        self.checkCount = {}
        self.errorDetails = []
        for error_it in ['ERROR', 'Warning', 'Check', 'ignore',
                         'unchecked', 'uncommitted', 'unmatched', 'comment']:
            self.errorCount[error_it] = 0
        self.errorCount['all'] = len(self.displayList)
        for line_it in self.displayList:
            mySection = ""
            myType = line_it['type']
            if myType in ['uncommitted', 'checked', 'unchecked']:
                mySection = cvc_globals.errorList[line_it['priority']]['section']
                if myType == 'checked':
                    myType = line_it['level']
            elif myType not in ['comment', 'unmatched']:
                print("Warning: unknown type", line_it)
            self.errorCount[myType] += 1
            if mySection and not mySection in self.errorCount:
                self.errorCount[mySection] = 0
                self.checkCount[mySection] = 0
            if line_it['type'] in ['uncommitted', 'unchecked']:
                self.errorCount[mySection] += 1
            elif line_it['type'] == 'checked':
                self.checkCount[mySection] += 1             
                self.errorCount[mySection] += 1
        myTotalErrors = 0
        myCheckedErrors = 0
        for error_it in cvc_globals.errorList:
            mySection = error_it['section']
            if mySection in self.errorCount:
                self.errorDetails.append("%s %5d/%d" % (mySection, self.checkCount[mySection],
                                                        self.errorCount[mySection]))
                myTotalErrors += self.errorCount[mySection]
                myCheckedErrors += self.checkCount[mySection]
        self.percentage = int(myCheckedErrors/myTotalErrors * 1000) if myTotalErrors != 0 else 0
        self.errorDetails.append("%s %5d/%d  %.1f%%" % ("Total", myCheckedErrors,
                                                        myTotalErrors, self.percentage / 10.0))

    def CreateDisplayList(self, theSummaryList):
        """Create a list of references and details to display.

        Sorted summary and error list are compared and corresponding display lines are created.
        Error counts are retotaled.
        A display record consists on the following fields:
          'priority': display priority
          'reference': from the matching summary entry
          'data': the display line text
          'level': for checked entries, 'ERROR' -> ... -> 'ignore'. for other entries, 'unknown'
          'type': see below
        There are 5 types of display lines:
          'comment'   : #comment [reference] level:section *:detail
          'unmatched' : * [reference] level:section *:detail
          'unchecked' : section *:detail
          'checked'   : [reference] level:section *:detail
          'uncommited': ! [reference] level: section *:detail

        Inputs: theSummaryList: List of references associated with errors.
        Modifies: displayList: List of formatted lines for possible display.
        """
        self.summaryFileName = theSummaryList.summaryFileName
        mySortedSummary = theSummaryList.ExtractSummary(self.modeName)
        myErrorIndex = 0
        mySummaryIndex = 0
        while myErrorIndex < len(self.errorList) or mySummaryIndex < len(mySortedSummary):
            mySummaryItem = mySortedSummary[mySummaryIndex] \
                            if mySummaryIndex < len(mySortedSummary) else {}
            myErrorItem = self.errorList[myErrorIndex] \
                          if myErrorIndex < len(self.errorList) else {}
            myOrder = CompareErrors(mySummaryItem, myErrorItem)
            myRecord = {'level': 'unknown'}
            if myOrder == "<":  # only in summary list
                myRecord['priority'] = mySummaryItem['priority']
                myRecord['reference'] = mySummaryItem['reference']
                myRecord['data'] = mySummaryItem['data']
                myRecord['type'] = 'comment'
                # For comments and unmatched, level data from summary item is ignored
                if not myRecord['reference'].startswith("#"):  # summary is not a comment <- ng
                    myRecord['reference'] = "* " + myRecord['reference']
                    myRecord['type'] = 'unmatched'
                mySummaryIndex += 1
            elif myOrder == ">":  # only in error list
                myRecord['priority'] = myErrorItem['priority']
                myRecord['reference'] = ''
                myRecord['data'] = myErrorItem['data']
                myRecord['type'] = 'unchecked'
                myErrorIndex += 1
            else:  # summary list and error list match
                myRecord['priority'] = myErrorItem['priority']
                myRecord['reference'] = mySummaryItem['reference']
                myRecord['data'] = myErrorItem['data']
                myRecord['level'] = mySummaryItem['level']
                myRecord['type'] = 'checked'
                if myRecord['reference'].startswith('#'):  # summary is a comment <- ng
                    myRecord['reference'] = "! " + myRecord['reference']
                    myRecord['type'] = 'unchecked'
                mySummaryIndex += 1
                myErrorIndex += 1
            self.displayList.append(myRecord)
        self.CountErrors()

    def PrintTotals(self):
        """Print summary of errors for each section and total errors."""
        print("Error totals for mode " + self.modeName)
        for error_it in self.errorDetails:
            print("    " + error_it)
            
#23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
