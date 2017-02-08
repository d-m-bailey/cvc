#! /usr/bin/env python
""" check_cvc.py: Track analysis of CVC errors

    Note: requires kivy

    usage: check_cvc.py [-i] summaryFileName logFile1 [logFile2 ...]

    Options:
      -i: interactive mode facilitating error analysis
    Inputs:
      summaryFileName: file name of error details with references indicating result of analysis
      logFile*: file name of CVC log file for individual modes
    Output:
      list of error summaries by mode
      updated summary file (interactive mode only)
      
    Summary file entries are compared to entries in the logFiles and corresponding error files.
    There are 5 possible outcomes for each summary-error combination:
      1) Match: these are analyzed errors and maybe one of ERROR, Warning, Check, ignore.
      2) Unchecked: error with no corresponding summary.
      3) Comment: summary only entry that begins with comment character "#".
      4) Unmatched: non-comment summary entry with no corresponding error.
      5) Uncommitted: created when copying error analyses from one mode to another.
    Analysis is complete when there are no unchecked and no unmatched and no uncommitted errors.
 
    Globals:
      cvc_globals: Error recognition expressions and priorities
    Classes:
      SummaryFile: Read and hold summary file data.
      ResultFile: Read and process error file data.
      SummaryApp: Kivy GUI App.

    Copyright 2106, 2017 D. Mitch Bailey  d.mitch.bailey at gmail dot com

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

import sys
import os
import getopt

os.environ["KIVY_NO_ARGS"] = "1"
os.environ["KIVY_NO_CONSOLELOG"] = "1"

import cvc_globals

from SummaryFile import SummaryFile
from ResultFile import ResultFile
from SummaryApp import SummaryApp

def DisplayLicense():
    """Display GPLv3 reference."""
    print("check_cvc v1.0.10 Copyright (C) 2016, 2017  D. Mitch Bailey")
    print("This program comes with ABSOLUTELY NO WARRANTY.")
    print("This is free software licensed under GPLv3,")
    print("and you are welcome to redistribute it under certain conditions.")
    print("See http://www.gnu.org/licenses/ for details.\n")

def main(argv):
    """Manage the analysis of CVC error results of multiple modes.

    usage: check_cvc.py [-i] summaryFileName logFile1 [logFile2 ...]
      -i: interactive mode (otherwise just prints error summaries)
      summaryFileName: name of file containing analysis results (initially empty)
      logFile*: name of CVC result file (multiple modes possible)
    """
    DisplayLicense()
    options, arguments = getopt.getopt(argv, "i")
    if len(argv) < 2:
        print ("usage check_cvc [-i] summary_file log_file1 [log_file2} ...")
        return
    cvc_globals.InitializeErrors()
    mySummaryList = SummaryFile(arguments[0])
    myResultsList = []
    for logFileName_it in arguments[1:]:
        try:
            myResultsList.append(ResultFile(logFileName_it))
        except IOError:  # skip files that can't be opened
            pass
    print("")
    if myResultsList:
        if "-i" in [option[0] for option in options]:
            SummaryApp(mySummaryList, myResultsList).run()
        else:  # batch mode: only print error summary
            SummaryApp(mySummaryList, myResultsList).BatchRun()
 
if __name__ == '__main__':
    if len(sys.argv) == 1:
        main(["-i", "../cvc/summary_cvc",
              "../cvc/test_logs/160829_1200/CVC_TEST_diode.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_voltage.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_SHORT1.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_float1.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_float2.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_float3.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_float4.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_gate1.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_shadan.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_shadan1.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_pleak.cvc_20131001.log",
              "../cvc/test_logs/160829_1200/CVC_TEST_base.cvc_20131001.log"])
    else:
        main(sys.argv[1:])

#23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
