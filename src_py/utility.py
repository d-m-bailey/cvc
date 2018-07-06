""" utility.py: check_cvc utility functions

    Functions:
      OpenFile: Open a possibly compressed file and return file structure.
      CompareErrors: Compare 2 error records and return "<", "=", ">", or "?".

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

import gzip

def OpenFile(theFileName, theMode="r"):
    """Open a possibly compressed file and return file structure.

    Errors need to be handled in calling routine.
    """
    if theFileName.endswith(".gz"):
        myFile = gzip.open(theFileName, mode=theMode + "t")
    else:
        myFile = open(theFileName, theMode)
    return myFile

def CompareErrors(theFirstItem, theSecondItem, thePrintFlag=False):
    """Compare 2 error records and return "<", "=", ">", or "?".

    If one value is undefined, the other is less.
    If both values are undefined, return "?".
    If the priorities are different, use priorities.
    If the priorities are the same, use the error data.
    """
    if thePrintFlag:
        print("first", theFirstItem)
        print("second", theSecondItem)
    if theFirstItem and not theSecondItem:
        return "<"
    elif not theFirstItem and theSecondItem:
        return ">"
    elif not theFirstItem and not theSecondItem:
        return "?"
    elif theFirstItem['priority'] < theSecondItem['priority']:
        return "<"
    elif theFirstItem['priority'] > theSecondItem['priority']:
        return ">"
    elif theFirstItem['keyData'] < theSecondItem['keyData']:
        return "<"
    elif theFirstItem['keyData'] > theSecondItem['keyData']:
        return ">"
    else:
        return "="  

#23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
