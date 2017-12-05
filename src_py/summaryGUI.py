""" summaryGUI.py kivy widget class

    Classes:
      SummaryListItem: Selectable error list item.
      SummaryTabContent: Tabbed panel content for a single report.
      ReferenceModeButton: Button for selecting modes when copying references and comments.
      HistoryTextInput: Implements text input with history and clear.
      SummaryWidget: Root widget handles filters, error totals, error displays and buttons.

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

import sys
import os
import cvc_globals
import pprint
import re
import inspect
import copy
from functools import partial
import pdb
import time
import threading

from utility import CompareErrors

from kivy.app import App
from kivy.event import EventDispatcher
from kivy.uix.widget import Widget
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.uix.listview import ListView, ListItemButton
from kivy.uix.button import Button
from kivy.uix.popup import Popup
from kivy.uix.selectableview import SelectableView
from kivy.uix.spinner import Spinner
from kivy.uix.tabbedpanel import TabbedPanel, TabbedPanelHeader
from kivy.adapters.listadapter import ListAdapter
from kivy.graphics import Color
from kivy.core.window import Window
from kivy.core.clipboard import Clipboard
from kivy.clock import Clock
from kivy.event import EventDispatcher
from kivy.properties import StringProperty, ListProperty, DictProperty, NumericProperty
from kivy.properties import BooleanProperty
from kivy.factory import Factory
from kivy.config import Config

class SummaryListItem(SelectableView, BoxLayout):
    """Selectable error list item.

    Instance variables:
      errorText: One line of error display text including leading '!' or '*'.
      is_selected: True if selected.
      index: The number of this item in filtered list.
      parentView: The parent listview.
      type: The type of error ('checked', 'unchecked', 'uncommited', 'unmatched', 'comment').
      baseIndex: The number of this item in display list.
    Methods:
      __init__: Create instance from filteredList item.
      TouchError: Handle error touch. 1) Set filter. 2) Show detail. 3) Change selection.
      ChangeSelection: Toggle selected status.
      DragSelect: Select/deselect list item.
    """
    def __init__(self, **kwargs):
        """Create instance from filteredList item."""
        super(SummaryListItem, self).__init__()
        self.errorText = kwargs['errorText']
        self.is_selected = kwargs['is_selected']
        self.index = kwargs['index']
        self.parentView = kwargs['parentView']
        self.type = kwargs['type']
        self.baseIndex = kwargs['baseIndex']

    def TouchListItem(self, theTouch, theRoot, theIndex):
        """Handle error selection.

        Inputs:
          theTouch: Record containing touch data.
          theRoot: The top widget.
          theIndex: The index of the selected line.
        Control-touch: Set filter.
        Shift-touch: Show details.
        Touch-only: Toggle select and set selection type.
        Modifies:
          parentView.selectMode: Current mode (select/unselect) for DragSelect.
          parentView.selectType: Current selection type.
        """
        if 'button' in theTouch.profile and theTouch.button != 'left':
            return  # ignore middle, right clicks
        if 'control' in theRoot.modifiers:
            theRoot.SetCellFilter(theIndex)
        if 'shift' in theRoot.modifiers:
            theRoot.ViewDetails(theIndex)
        if not theRoot.modifiers:  # Change selection.
            theTouch.grab(self.parentView)
            myAdapter = self.parentView.adapter
            if self.is_selected:
                self.parentView.selectMode = 'deselect'
            else:
                self.parentView.selectMode = 'select'
                if len(myAdapter.selection) == 1 and self.type != self.parentView.selectType:
                    # unselect and reset selection type
                    myAdapter.handle_selection(myAdapter.selection[0])
                if not myAdapter.selection:
                    self.parentView.selectType = self.type
            self.DragSelect()

    def DragSelect(self):
        """Select/deselect list item.

        Select only lines of the same 'type' ('checked', 'unmatched', etc.).
        """
        myAdapter = self.parentView.adapter
        if (self.parentView.selectMode == 'select' and not self.is_selected
                and self.type == self.parentView.selectType):
            myAdapter.handle_selection(self.errorRef)
        elif self.parentView.selectMode == 'deselect' and self.is_selected:
            myAdapter.handle_selection(self.errorRef)
        myAdapter.dispatch('on_selection_change')


class SummaryTabContent(BoxLayout):
    """Tabbed panel content for a single report.

    Instance variables:
      report: Report data for this tab.
      root: The root widget.
      undoList: List of display lists to undo previous changes.
      redoList: List of display lists to redo changes.
      filteredList: List of filtered items from the display list of the report.
    Methods:
      __init__: Create tab content from a report.
      SetTabColor: Set tab color based on percentage complete.
      RedisplayErrors: Freeze screen and filter errors in background.
      FinishSelection: Reset buttons after finishing selection.
    """
    def __init__(self, theReport, theRootWidget, **kwargs):
        """Create tab content from a report.

        Inputs:
          theReport: Data from the log and error files for one mode.
          theRootWidget: The top widget.
        Sets:
          logFileName:
          summaryFileName:
        """
        self.report = theReport
        self.root = theRootWidget
        self.undoList = []
        self.redoList = []
        super(SummaryTabContent, self).__init__(**kwargs)
        self.ids.logFileName_id.text = theReport.logFileName
        self.ids.summaryFileName_id.text = theReport.summaryFileName

    def _Format(self, theIndex):
        """Return error display line formatted according to type, reference.

        Inputs:
          theIndex: Index of error to format.
        """
        myItem = self.report.displayList[theIndex]
        if myItem['reference']:
            myReference = myItem['reference']
            if myItem['level'] == 'ERROR':  # red
                myReference = myReference.replace('ERROR', '[color=#ff0000]ERROR[/color]')
            elif myItem['level'] == 'Warning':  # orange
                myReference = myReference.replace('Warning', '[color=#ff8800]Warning[/color]')
            elif myItem['level'] == 'Check':  # yellow
                myReference = myReference.replace('Check', '[color=#ffff00]Check[/color]')
            elif myItem['level'] == 'ignore':  # green
                myReference = myReference.replace('ignore', '[color=#00ff00]ignore[/color]')
            return myReference + ":" + myItem['data']
        return myItem['data'] + ""

    def _SetSpinner(self):
        """Set up section spinner.

        Modifies:
          sectionSpinner:
        If changing from old mode to new mode that does not have current section, set text to last
        section and set the counts to 0/0.
        """
        self.root.ids.setSectionFlag = False
        mySpinner = self.root.ids.sectionSpinner_id
        mySpinner.values = self.report.errorDetails
        if not self.root.sectionFilter or self.root.sectionFilter == "Total":
            mySpinner.text = mySpinner.values[-1]
        else:
            mySpinner.text = "%s     0/0" % self.root.sectionFilter
            for error_it in mySpinner.values:
                if error_it.startswith(self.root.sectionFilter):
                    mySpinner.text = error_it
                    break

    def SetTabColor(self, theTab):
        """Set tab color based on percentage complete.

        Inputs:
          theTab: The tab containing text to be colored.
        Color is set on linear scale from red 0% to yellow 99.9%.
        At 100%, the color is set to green,
        unless there are uncommited or unmatched errors, in which case color is blue.
        """
        if self.report.percentage == 1000:
            if self.report.errorCount['unmatched'] + self.report.errorCount['uncommitted'] > 0:
                (theTab.r, theTab.g, theTab.b) = (0, 0, 1)  # blue
            else:
                (theTab.r, theTab.g, theTab.b) = (0, 1, 0)  # green
        else:
            (theTab.r, theTab.g, theTab.b) = (1, self.report.percentage / 1000, 0)  # red - yellow

    def _SetTypeFilterCounts(self, theErrorList):
        """Set counts for type check box filters.

        Inputs:
          theErrorList: List of totals for each error type.
        Type check box filters have ids that start with "show_"
        """
        for id_it in self.root.ids:
            if id_it.startswith("show_") and id_it != 'show_none':
                myKey = id_it[5:]  # ex. "show_all" -> "all"
                self.root.ids[id_it].count = str(theErrorList[myKey])

    def RedisplayErrors(self):
        """Freeze screen and filter errors in background."""
        self.root.currentContent = self
        self.root.workingPopupRef.open()
        self.redisplayThread = threading.Thread(target=self._FilterErrors)
        self.redisplayThread.start()

    def _FilterErrors(self):
        """Filter display list according to check box, section, and text filters.

        Calls _FinishRedisplay when finished to unlock screen and refresh display data.
        Modifies:
          filteredList:
        """
        myTextFilter = self.root.filterTextRef.textRef.text
        try:
            myFilterRegex = re.compile(myTextFilter) if myTextFilter else None
        except:
            print("Invalid regular expression '%s'. trying as literal" % (myTextFilter))
            myFilterRegex = re.compile(re.escape(myTextFilter))
        self._SetSpinner()
        myFilterPriority = cvc_globals.priorityMap[self.root.sectionFilter] \
            if self.root.sectionFilter in cvc_globals.priorityMap else None
        myFilteredList = []
        myDisplayList = self.report.displayList
        myInitialCheck = False if self.root.checkValues else True
        myCheckValues = self.root.checkValues
        for index_it in range(len(myDisplayList)):
            myRecord = myDisplayList[index_it]
            if not myFilterPriority or myFilterPriority == myRecord['priority']:
                myDisplayText = self._Format(index_it)
                if not myTextFilter or myFilterRegex.search(myDisplayText):
                    if myRecord['type'] == 'checked':
                        myLevelFilter = "show_" + myRecord['level']
                    else:
                        myLevelFilter = "show_" + myRecord['type']
                    if myInitialCheck or myCheckValues[myLevelFilter]:
                        myNewRecord = myRecord.copy()
                        myNewRecord['baseIndex'] = index_it
                        myNewRecord['display_text'] = myDisplayText
                        myFilteredList.append(myNewRecord)
        self.filteredList = myFilteredList
        Clock.schedule_once(self._FinishRedisplay)

    def _FinishRedisplay(self, dt):
        """Redisplay counts and errors, and release screen."""
        myFilteredList = self.filteredList
        self.listRef.adapter.data = [
            {'errorText': myFilteredList[index]['display_text'],
             'is_selected': False,
             'type': myFilteredList[index]['type'],
             'baseIndex': myFilteredList[index]['baseIndex']}
            for index in range(len(myFilteredList))]
        self.SetTabColor(self.root.modePanelRef.current_tab)
        self.root.currentContent.report.CountErrors()
        self._SetTypeFilterCounts(self.root.currentContent.report.errorCount)
        self.root.ids.displayCount_id.text = str(len(myFilteredList))
        self.root.DeselectAll()  # resets button status
        self.root.workingPopupRef.dismiss()

    def FinishSelection(self, theTouch, theListView):
        """Reset buttons after finishing selection."""
        self.root.EnableButtons(theListView.adapter)
        theListView.selectMode = ""
        theTouch.ungrab(theListView)


class ReferenceModeButton(Button):
    """Button for selecting modes when copying references and comments.

    Instance variables:
      text: Button text.
      r: Red component of text color.
      g: Green component of text color.
      b: Blue component of text color.
      root: The root widget.
    Methods:
      __init__: Set button text and text color
      ToggleButton: Toggle mode selection and update popup buttons.
    Button text is the mode and text color indicates percentage complete.
    """
    def __init__(self, **kwargs):
        """Initialize button with text=mode and color indicating check status."""
        self.text = kwargs.pop('text')
        self.r = kwargs.pop('r')
        self.g = kwargs.pop('g')
        self.b = kwargs.pop('b')
        self.root = kwargs.pop('root')
        super(ReferenceModeButton, self).__init__(**kwargs)

    def ToggleButton(self):
        """Toggle mode selection and update popup buttons."""
        self.is_selected = False if self.is_selected else True
        self.root.EnableReferenceButtons()


class HistoryTextInput(BoxLayout):
    """Implements text input with history and clear.

    Instance variables:
      undoList: List of previous values.
      redoList: List of undone values.
      root: Root widget reference.
    Methods:
      Undo: Set text input to previous value.
      Redo: Set text input to previous undone value.
      Clear: Clear text input.
      Set: Set the text input.
    """
    def __init__(self, **kwargs):
        super(HistoryTextInput, self).__init__(**kwargs)

    def Undo(self, theTextRef, theRedisplayFlag):
        myAppendOk = len(self.redoList) == 0 or self.redoList[-1] != theTextRef.text
        if theTextRef.text and myAppendOk:
            self.redoList.append(theTextRef.text)
        theTextRef.text = self.undoList[-1]
        del self.undoList[-1]
        if theRedisplayFlag:
            self.root.currentContent.RedisplayErrors()

    def Redo(self, theTextRef, theRedisplayFlag):
        myAppendOk = len(self.undoList) == 0 or self.undoList[-1] != theTextRef.text
        if theTextRef.text and myAppendOk:
            self.undoList.append(theTextRef.text)
        theTextRef.text = self.redoList[-1]
        del self.redoList[-1]
        if theRedisplayFlag:
            self.root.currentContent.RedisplayErrors()

    def Clear(self, theTextRef, theRedisplayFlag):
        myAppendOk = len(self.undoList) == 0 or self.undoList[-1] != theTextRef.text
        if theTextRef.text and myAppendOk:
            self.undoList.append(theTextRef.text)
        theTextRef.text = ""
        if theRedisplayFlag:
            self.root.currentContent.RedisplayErrors()

    def Set(self, theHasFocusFlag, theTextRef, theRedisplayFlag):
        myAppendOk = len(self.undoList) == 0 or self.undoList[-1] != theTextRef.text
        if theHasFocusFlag and theTextRef.text and myAppendOk:  # save last text on focus
            self.undoList.append(theTextRef.text)
        if not theHasFocusFlag and theRedisplayFlag:
            self.root.currentContent.RedisplayErrors()


class SummaryWidget(Widget):
    """Root widget handles filters, error totals, error displays and buttons.

    Class variables:
      sectionFilter: Priority of section to filter on.
      checkValues: List of values of type filter check boxes.
      copyValues: List of vaules of copy filter check boxes.
      modifiers: List of currently pressed 'shift' or 'control' keys.
      currentContent: Shortcut to content of the current tab of the mode panel.
      setSectionFlag: Flag to prevent recursion when setting sectionFilter.
      changedFlag: Summary data has changed.
      autoSaveFlag: Summary data has changed and needs auto-save.
      referenceAction: How to handle reference data. 'replace' or 'ask' (-> 'replace' or 'keep').
      errorLevel: Saved error level for reference action 'ask'.
    Instance variables:
      app: The parent app.
      summary: The data from the summary file.
      timeStamp: Date of program start for auto-save file.
      autoSaveFileName: Name of the auto-save file.
    Methods:
      __init__: Initialize root widget with summary and report data.
      SetSectionFilter: Set section filter and redisplay.
      IncrementReference: Increment/decrement the last number in the reference text.
      SetFilters: Set check box values and assign to theValues list.
      ResetCheckboxes: Set checkboxes to values in theValues.
      UndoChanges: Undo one set of changes for the current content.
      RedoChanges: Redo one set of changes for the current content.
      AddReference: Add reference text and theLevel to displayList for selected unchecked lines,
        or for checked lines, replace theLevel while keeping or replacing reference.
      AddComment: Add comment to displayList for selected unmatched lines.
      ClearReference: Remove reference from displayList for selected checked, comment,
        uncommitted and unmatched lines.
      SetCellFilter: Set text filter to cell name in selected filtered display line.
      ViewDetails: View details for selected filtered display line.
      EnableButtons: Enable main screen buttons based on status.
      SetAllReferences: Set all references selected or unselected.
      EnableReferenceButtons: Enable copy buttons based on reference button status.
      SelectAll: Select all filtered display lines of the current type.
      DeselectAll: Unselect all selected filtered display lines.
      AddReferenceModes: Add a selectable mode button for each mode, except for the current mode.
      CopyReferences: Copy references from the current mode to selected modes based
        on copy filters.
      CommitReferences: Commit selected references copied from other modes.
      DeleteSummary: Delete selected filtered display lines.
      AutoSaveSummary: Auto-save the summary file to a temporary file name at regular intervals,
        if necessary.
      RemoveAutoSaveFile: Attempt to remove last auto-saved file.
      SaveSummaryAs: Save summary file as a new file, checking for overwrites.
      SaveSummary: Save the summary file.
      ExportSummaryAs: Export summary file as a CSV file, checking for overwrites.
      ExportSummary: Save the summary file as a CSV file.
      Quit: Check for summary changes before ending.
    """
    sectionFilter = None
    checkValues = {}
    copyValues = {}
    modifiers = {}
    currentContent = None
    setSectionFlag = False
#    changedFlag = BooleanProperty(False)   # set in summary.kv
    autoSaveFlag = False
    referenceAction = None
    errorLevel = None

    def __init__(self, theApp, theSummaryList, theReportList, **kwargs):
        """Initialize root widget with summary and report data.

        Watch the keyboard for shift/control down.
        """
        self.app = theApp
        self.summary = theSummaryList
        super(SummaryWidget, self).__init__(**kwargs)
        self._GetKeyboard()
        self.app.modeRows = (len(theReportList)-1) // 10 + 1
        for report_it in theReportList:
            myPanelHeader = TabbedPanelHeader()
            report_it.CreateDisplayList(theSummaryList)
            report_it.PrintTotals()
            myPanelHeader.text = report_it.modeName
            myPanelHeader.content = SummaryTabContent(report_it, self)
            myPanelHeader.content.SetTabColor(myPanelHeader)
            self.modePanelRef.add_widget(myPanelHeader)
        self.SetFilters('all', "show_", self.checkValues)
        self.SetFilters('all', "copy_", self.copyValues)
        self.modePanelRef.set_def_tab(self.modePanelRef.tab_list[-1])
        self.currentContent = self.modePanelRef.current_tab.content
        Clock.schedule_interval(self.AutoSaveSummary, 300)
        myLocalTime = time.localtime(time.time())
        self.timeStamp = "%d%02d%02d_%02d%02d%02d" % (myLocalTime[0:6])
        self.autoSaveFileName = self.summary.summaryFileName + "." + self.timeStamp
        Clock.create_trigger(partial(Config.set, 'kivy', 'log_level', 'info'))

    def _GetKeyboard(self):
        """Set up call backs for key events.

        Since we just monitor shift/control keys, no need to ever release keyboard.
        """
        myKeyboard = Window.request_keyboard(None, self)
        myKeyboard.bind(on_key_down=self._OnKeyboardDown)
        myKeyboard.bind(on_key_up=self._OnKeyboardUp)

    def _OnKeyboardDown(self, keyboard, keycode, text, modifiers):
        """Set modifiers for shift and control keys."""
        if keycode[1] in ['lctrl', 'rctrl']:
            self.modifiers['control'] = True
        elif keycode[1] in ['shift', 'rshift']:
            self.modifiers['shift'] = True

    def _OnKeyboardUp(self, keyboard, keycode):
        """Unset modifiers for shift and control keys."""
        if keycode[1] in ['lctrl', 'rctrl']:
            if 'control' in self.modifiers:
                del self.modifiers['control']
        elif keycode[1] in ['shift', 'rshift']:
            if 'shift' in self.modifiers:
                del self.modifiers['shift']

    def SetSectionFilter(self, theText):
        """Set section filter and redisplay.

        Input:
          theText: Text displayed in spinner. "section checked#/total#"
        """
        self.setSectionFlag = False
        self.sectionFilter = theText.split()[0] if theText else ""
        self.currentContent.RedisplayErrors()

    def IncrementReference(self, theIncrement):
        """Increment/decrement the last number in the reference text.

        Input:
          theIncrement: The value to add to last number in the reference.
        Modifies:
          referenceText
        ex. A002D5 -> A002D4, A002D5a -> A002D6a
        Retains same number of digits on decrement, may increase on increment.
        """
        myText = self.ids.referenceText_id.textRef.text
        myMatch = re.match("(.*?)(\d*)(\D*)$", myText)
        if myMatch.group(2):
            myDigits = len(myMatch.group(2))
            myNewIndex = max(0, int(myMatch.group(2)) + theIncrement)
            myNewReference = myMatch.group(1) + "%0*d" % (myDigits, myNewIndex)
            myNewReference += myMatch.group(3) if myMatch.group(3) else ""
            self.ids.referenceText_id.textRef.text = myNewReference

    def SetAllFilters(self, theValue, theType):
        """Set filter check boxes to true or false.

        Inputs:
          theValue: True or False.
          theType: The type of check boxes - "show_" or "copy_".
        Modifies:
          show_* or copy_* check boxes.
        """
        for check_it in self.ids:
            if check_it.startswith(theType):
                self.ids[check_it].checkRef.active = theValue
        self.ids[theType + 'none'].checkRef.active = not theValue

    def SetFilters(self, theId, theType, theValues):
        """Set check box values and assign to theValues list.

        all and none checks are recalculated from the other values.
        Inputs:
          theId: Id of the the box clicked.
          theType: The type of check boxes - "show_" or "copy_".
          theValues: Array of check box values.
        Modifies:
          show_* or copy_* check boxes.
        """
        myId = theType + theId if theId else None
        myNewState = not self.ids[myId].checkRef.active if theId else None
        if 'shift' in self.modifiers:  # shift-click to select theId and unselect all others.
            myNewState = True
            self.SetAllFilters(False, theType)
        elif 'control' in self.modifiers: # control-click to deselect theId and select all others.
            myNewState = False
            self.SetAllFilters(True, theType)
        if myNewState:
            if theId == 'all':
                self.SetAllFilters(True, theType)
            elif theId == 'none':
                self.SetAllFilters(False, theType)
        for check_it in self.ids:
            if check_it.startswith(theType):
                theValues[check_it] = self.ids[check_it].checkRef.active
        if theId:
            theValues[myId] = myNewState
        myAllCheck = True
        myNoneCheck = True
        for check_it in theValues:
            if check_it in [theType + 'all', theType + 'none']:
                pass
            else:
                myAllCheck = myAllCheck and theValues[check_it]
                myNoneCheck = myNoneCheck and not theValues[check_it]
        theValues[theType + 'all'] = myAllCheck
        theValues[theType + 'none'] = myNoneCheck
        if theType == 'show_':
            if self.currentContent:  # skip redisplay if not defined (initial redisplay)
                self.currentContent.RedisplayErrors()
            Clock.schedule_once(partial(self.ResetCheckboxes, self.checkValues))
        elif theType == 'copy_':
            Clock.schedule_once(partial(self.ResetCheckboxes, self.copyValues))

    def ResetCheckboxes(self, theValues, *largs):
        """Set checkboxes to values in theValues."""
        for check_it in theValues:
            self.ids[check_it].checkRef.active = theValues[check_it]

    def _AddUndo(self, theContent, theDisplayList):
        """Add current theDisplayList to undo list for theContent.

        push(undo), clear(redo).
        """
        theContent.undoList.append(copy.deepcopy(theDisplayList))
        theContent.redoList = []

    def UndoChanges(self):
        """Undo one set of changes for the current content.

        push(redo) -> pop(undo).
        """
        myContent = self.currentContent
        myContent.redoList.append(copy.deepcopy(myContent.report.displayList))
        myContent.report.displayList = myContent.undoList[-1]
        del myContent.undoList[-1]
        myContent.report.CountErrors()
        myContent.RedisplayErrors()
        self.changedFlag = self.autoSaveFlag = True

    def RedoChanges(self):
        """Redo one set of changes for the current content.

        push(undo) -> pop(redo).
        """
        myContent = self.currentContent
        myContent.undoList.append(copy.deepcopy(myContent.report.displayList))
        myContent.report.displayList = myContent.redoList[-1]
        del myContent.redoList[-1]
        myContent.report.CountErrors()
        myContent.RedisplayErrors()
        self.changedFlag = self.autoSaveFlag = True

    def AddReference(self, theLevel, theReferenceAction):
        """Add reference text and theLevel to displayList for selected unchecked lines,
          or for checked lines, replace theLevel while keeping or replacing reference.

        Surrounding '[', ']' added if not present.
        'type' is changed to 'checked'
        """
        if theReferenceAction == 'ask':
            self.errorLevel = theLevel
            self.replaceReferencePopupRef.open()
            return

        self.replaceReferencePopupRef.dismiss()
        myContent = self.currentContent
        myAdapter = myContent.listRef.adapter
        myDisplayList = myContent.report.displayList
        self._AddUndo(myContent, myDisplayList)
        myReference = self.ids.referenceText_id.textRef.text.strip()
        myEndOfReferenceRE = re.compile(r"\] .*")
        if not myReference.startswith("["):
            myReference = "[" + myReference + "]"
        for record_it in myAdapter.selection:
            myData = myAdapter.get_data_item(record_it.index)
            myIndex = myData['baseIndex']
            myDisplayList[myIndex]['level'] = theLevel
            myDisplayList[myIndex]['type'] = 'checked'
            if theReferenceAction == 'replace':
                myDisplayList[myIndex]['reference'] = myReference + " " + theLevel
            else:  # 'keep'
                myDisplayList[myIndex]['reference'] = \
                    myEndOfReferenceRE.sub("] " + theLevel, myDisplayList[myIndex]['reference'])
        myContent.report.CountErrors()
        myContent.RedisplayErrors()
        self.changedFlag = self.autoSaveFlag = True

    def AddComment(self):
        """Add comment to displayList for selected unmatched lines.

        Leading '#' added, if missing.
        'type' is changed to 'comment'.
        """
        myContent = self.currentContent
        myAdapter = myContent.listRef.adapter
        myDisplayList = myContent.report.displayList
        self._AddUndo(myContent, myDisplayList)
        myComment = self.ids.commentText_id.textRef.text.strip()
        if not myComment.startswith("#"):
            myComment = "#" + myComment
        for record_it in myAdapter.selection:
            myData = myAdapter.get_data_item(record_it.index)
            myIndex = myData['baseIndex']
            myDisplayList[myIndex]['level'] = 'unknown'
            myDisplayList[myIndex]['type'] = 'comment'
            myReference = myDisplayList[myIndex]['reference']
            if myReference.startswith("* "):  # unmatched lines without leading # have leading *
                myReference = myReference[2:]
            myDisplayList[myIndex]['reference'] = myComment + " " + myReference
        myContent.report.CountErrors()
        myContent.RedisplayErrors()
        self.changedFlag = self.autoSaveFlag = True

    def ClearReference(self):
        """Remove reference from displayList for selected checked, comment, uncommitted
        and unmatched lines.

        'comment' lines revert to 'unmatched'.
        'checked' and 'uncommitted' lines revert to 'unchecked'.
        """
        myContent = self.currentContent
        myAdapter = myContent.listRef.adapter
        myDisplayList = myContent.report.displayList
        self._AddUndo(myContent, myDisplayList)
        for record_it in myAdapter.selection:
            myData = myAdapter.get_data_item(record_it.index)
            myIndex = myData['baseIndex']
            myDisplayList[myIndex]['level'] = 'unknown'
            if (myDisplayList[myIndex]['type'] == 'comment'
                    or myDisplayList[myIndex]['type'] == 'unmatched'):
                myDisplayList[myIndex]['type'] = 'unmatched'
                myDisplayList[myIndex]['reference'] = "* "
            else:
                myDisplayList[myIndex]['type'] = 'unchecked'
                myDisplayList[myIndex]['reference'] = ""
        myContent.report.CountErrors()
        myContent.RedisplayErrors()
        self.changedFlag = self.autoSaveFlag = True

    def SetCellFilter(self, theErrorIndex):
        """Set text filter to cell name in selected filtered display line.

        Inputs:
          theErrorIndex: Index to filtered display list.
        Only works for lines containing SUBCKT (cellname).
        """
        myContent = self.currentContent
        myData = myContent.listRef.adapter.get_data_item(theErrorIndex)
        myMatch = re.search("[^:\[]*\[([^\]]*)", myData['errorText'])  #*[reference] *
        if myMatch and myMatch.group(1):
            if self.referenceTextRef.textRef.text:
                self.referenceTextRef.undoList.append(self.referenceTextRef.textRef.text)
            self.referenceTextRef.textRef.text = myMatch.group(1)
        myMatch = re.search("^#([^\[:]*) *", myData['errorText'])  ##comment *
        if myMatch and myMatch.group(1):
            if self.commentTextRef.textRef.text:
                self.commentTextRef.undoList.append(self.commentTextRef.textRef.text)
            self.commentTextRef.textRef.text = myMatch.group(1)
        myMatch = re.search("SUBCKT ([^\)]*\))", myData['errorText'])  # * INFO: SUBCKT (cellName)
        if myMatch and myMatch.group(1):
            if self.filterTextRef.textRef.text:
                self.filterTextRef.undoList.append(self.filterTextRef.textRef.text)
            self.filterTextRef.textRef.text = myMatch.group(1)
            if myData['type'] in ['unmatched', 'comment']:
                Clipboard.copy("")
            else:
                Clipboard.copy(myContent.report.GetFirstError(myData))
            myContent.RedisplayErrors()

    def ViewDetails(self, theErrorIndex):
        """View details for selected filtered display line.

        Inputs:
          theErrorIndex: Index to filtered display list.
        No effect for unmatched or comment lines.
        """
        myContent = self.currentContent
        myData = myContent.listRef.adapter.get_data_item(theErrorIndex)
        if myData['type'] in ['unmatched', 'comment']:
            return
        Clipboard.copy(myContent.report.GetFirstError(myData))
        self.ids.errorDetails_id.text = myContent.report.GetErrorDetails(myData)
        self.detailPopupRef.open()

    def _CountLines(self, theAdapter, theType):
        """Count the number of lines of a given type in the current filtered display."""
        myCount = 0
        for data_it in theAdapter.data:
            if data_it['type'] == theType:
                myCount += 1
        return myCount

    def EnableButtons(self, theAdapter):
        """Enable main screen buttons based on status.

        'Select All', 'Unselect All' enabled when something is selected.
          Select All shows the total displayed lines of the selected type.
          Unselect All shows the actual number of selected lines.
        'ERROR', 'Warning', 'Check', 'ignore' buttons enabled when 'unchecked' type selected,
          also enabled when 'checked' type selected.
        'Clear' button enabled for everything except, unchecked lines with comments.
        'Comment' button enabled form unmatched lines.
        'Delete' button enabled for unmatched and comment lines.
        'Commit' button enabled for uncommitted lines.
        'Undo' button enabled if undo stack is not empty and shows the size of the stack.
        'Redo' button enabled if redo stack is not empty and shows the size of the stack.
        'View' button enabled if textFilter is not empty.
        'Copy Checks' button enabled if more than one mode.
        """
        self.ids.errorButton.disabled = True
        self.ids.warningButton.disabled = True
        self.ids.checkButton.disabled = True
        self.ids.ignoreButton.disabled = True
        self.ids.commitButton.disabled = True
        self.ids.clearButton.disabled = True
        self.ids.commentButton.disabled = True
        self.ids.deleteButton.disabled = True
        self.ids.undoButton.disabled = True
        self.ids.redoButton.disabled = True
        self.ids.selectAllButton.disabled = True
        self.ids.selectAllButton.text = "Select All"
        self.ids.unselectAllButton.disabled = True
        self.ids.unselectAllButton.text = "Unselect All"
        self.ids.referenceButton.disabled = True
        self.ids.viewButton.disabled = True
        if theAdapter.selection:
            self.ids.selectAllButton.disabled = False
            myData = theAdapter.get_data_item(theAdapter.selection[0].index)
            myType = myData['type']  # get type from first item
            self.ids.selectAllButton.text = \
                "Select All (" + str(self._CountLines(theAdapter, myType))  + ")"
            self.ids.unselectAllButton.disabled = False
            self.ids.unselectAllButton.text = \
                "Unselect All (" + str(len(theAdapter.selection)) + ")"
            if myType == 'unchecked':
                self.ids.errorButton.disabled = False
                self.ids.warningButton.disabled = False
                self.ids.checkButton.disabled = False
                self.ids.ignoreButton.disabled = False
                self.referenceAction = 'replace'
                if myData['errorText'].startswith("!"):
                    self.ids.clearButton.disabled = False
            elif myType == 'unmatched':
                self.ids.commentButton.disabled = False
                self.ids.clearButton.disabled = False
                self.ids.deleteButton.disabled = False
            elif myType == 'uncommitted':
                self.ids.commitButton.disabled = False
                self.ids.clearButton.disabled = False
            elif myType == 'checked':
                self.ids.errorButton.disabled = False
                self.ids.warningButton.disabled = False
                self.ids.checkButton.disabled = False
                self.ids.ignoreButton.disabled = False
                self.referenceAction = 'ask'
                self.ids.clearButton.disabled = False
            elif myType == 'comment':
                self.ids.clearButton.disabled = False
                self.ids.deleteButton.disabled = False
        myContent = self.currentContent
        if myContent.undoList:
            self.ids.undoButton.text = "Undo(" + str(len(myContent.undoList)) + ")"
            self.ids.undoButton.disabled = False
        else:
            self.ids.undoButton.text = "Undo"
        if myContent.redoList:
            self.ids.redoButton.text = "Redo(" + str(len(myContent.redoList)) + ")"
            self.ids.redoButton.disabled = False
        else:
            self.ids.redoButton.text = "Redo"
        if self.filterTextRef.textRef.text:
            self.ids.viewButton.disabled = False
        if len(self.modePanelRef.tab_list) > 1:
            self.ids.referenceButton.disabled = False

    def SetAllReferences(self, theValue):
        """Set all references selected or unselected.

        Inputs:
          theValue: If true, select all, else unselect all.
        """
        for child_it in self.ids.referenceGrid_id.children:
            child_it.is_selected = theValue
        self.EnableReferenceButtons()

    def EnableReferenceButtons(self):
        """Enable copy buttons based on reference button status.

        'Unselect All' enabled when something is selected.
        'OK' enabled when something is type selected.
        """
        self.ids.unselectAllModesButton.disabled = True
        self.ids.copyReferencesButton.disabled = True
        myHasSelection = False
        for child_it in self.ids.referenceGrid_id.children:
            if child_it.is_selected:
                myHasSelection = True
        if myHasSelection:
            self.ids.unselectAllModesButton.disabled = False
            self.ids.copyReferencesButton.disabled = False

    def SelectAll(self):
        """Select all filtered display lines of the current type."""
        myType = self.currentContent.listRef.selectType
        myAdapter = self.currentContent.listRef.adapter
        for index in range(len(myAdapter.data)):
            myView = myAdapter.get_view(index)
            myError = myView.errorRef
            if not myError in myAdapter.selection and myError.parent.type == myType:
                myAdapter.handle_selection(myError, hold_dispatch=True)
        myAdapter.dispatch('on_selection_change')
        self.EnableButtons(myAdapter)

    def DeselectAll(self):
        """Unselect all selected filtered display lines."""
        myAdapter = self.currentContent.listRef.adapter
        for index in range(len(myAdapter.data)):
            myView = myAdapter.get_view(index)
            myError = myView.errorRef
            if myError in myAdapter.selection:
                myAdapter.handle_selection(myError, hold_dispatch=True)
        myAdapter.dispatch('on_selection_change')
        self.EnableButtons(myAdapter)

    def AddReferenceModes(self, theLayout):
        """Add a selectable mode button for each mode, except for the current mode.

        Inputs:
          theLayout: Layout to add mode buttons to.
        """
        theLayout.clear_widgets()
        myCurrentMode = self.modePanelRef.current_tab.text
        self.copyPopupRef.baseMode = myCurrentMode
        for mode_it in self.modePanelRef.tab_list[::-1]:
            # To get same order as main screen, reverse the order of the list.
            if mode_it.text == myCurrentMode:  # don't add the copy source.
                continue
            myButton = ReferenceModeButton(text=mode_it.text, root=self,
                                           r=mode_it.r, g=mode_it.g, b=mode_it.b)
            theLayout.add_widget(myButton)
        self.EnableReferenceButtons()

    def CopyReferences(self):
        """Copy references from the current mode to selected modes based on copy filters.

        If the source is checked and target is unchecked and copy level filter is set,
          copy reference and level and set type to uncommitted.
        If the source is comment and target is unmatched, copy type and reference.
        """
        mySelection = []
        for child_it in self.ids.referenceGrid_id.children:
            if child_it.is_selected:
                mySelection.append(child_it.text)
        mySourceDisplayList = self.currentContent.report.displayList
        for child_it in self.modePanelRef.tab_list:
            if child_it.content.report.modeName not in mySelection:
                continue

            myTargetDisplayList = child_it.content.report.displayList
            self._AddUndo(child_it.content, myTargetDisplayList)
            mySourceIndex = 0
            myTargetIndex = 0
            while (mySourceIndex < len(mySourceDisplayList)
                   and myTargetIndex < len(myTargetDisplayList)):
                mySourceError = mySourceDisplayList[mySourceIndex]
                myTargetError = myTargetDisplayList[myTargetIndex]
                myOrder = CompareErrors(mySourceError, myTargetError)
                if myOrder == "<":
                    mySourceIndex += 1
                    continue

                if myOrder == ">":
                    myTargetIndex += 1
                    continue

                if myTargetError['type'] == 'unchecked' and mySourceError['type'] == 'checked':
                    myUpdateOk = False
                    if mySourceError['level'] == 'ERROR' and self.copyValues['copy_ERROR']:
                        myUpdateOk = True
                    elif mySourceError['level'] == 'Warning' and self.copyValues['copy_Warning']:
                        myUpdateOk = True
                    elif mySourceError['level'] == 'Check' and self.copyValues['copy_Check']:
                        myUpdateOk = True
                    elif mySourceError['level'] == 'ignore' and self.copyValues['copy_ignore']:
                        myUpdateOk = True
                    if myUpdateOk:
                        myTargetError['type'] = 'uncommitted'
                        myTargetError['level'] = mySourceError['level']
                        myTargetError['reference'] = "? " + mySourceError['reference']
                elif (myTargetError['type'] == 'unmatched'
                      and mySourceError['type'] == 'comment'
                      and self.copyValues['copy_comment']):
                    myTargetError['type'] = 'comment'
                    myTargetError['level'] = 'unknown'
                    myTargetError['reference'] = mySourceError['reference']
                mySourceIndex += 1
                myTargetIndex += 1
            child_it.content.report.CountErrors()
        self.copyPopupRef.dismiss()
        self.changedFlag = self.autoSaveFlag = True

    def CommitReferences(self):
        """Commit selected references copied from other modes."""
        myContent = self.currentContent
        myAdapter = myContent.listRef.adapter
        myDisplayList = myContent.report.displayList
        self._AddUndo(myContent, myDisplayList)
        for record_it in myAdapter.selection:
            myData = myAdapter.get_data_item(record_it.index)
            myIndex = myData['baseIndex']
            myDisplayList[myIndex]['type'] = 'checked'
            # remove leading "? " on uncommitted lines
            myDisplayList[myIndex]['reference'] = myDisplayList[myIndex]['reference'][2:]
        myContent.report.CountErrors()
        myContent.RedisplayErrors()
        self.changedFlag = self.autoSaveFlag = True

    def DeleteSummary(self):
        """Delete selected filtered display lines.

        Only comment and unmatched lines are deletable.
        """
        myContent = self.currentContent
        myAdapter = myContent.listRef.adapter
        myDisplayList = myContent.report.displayList
        self._AddUndo(myContent, myDisplayList)
        myIndexList = []
        for record_it in myAdapter.selection[::-1]:
            myData = myAdapter.get_data_item(record_it.index)
            myIndexList.append(myData['baseIndex'])
        # Because deleting by index, must delete in reverse order.
        for index_it in sorted(myIndexList, reverse=True):
            del myDisplayList[index_it]
        myContent.report.CountErrors()
        myContent.RedisplayErrors()
        self.changedFlag = self.autoSaveFlag = True

    def AutoSaveSummary(self, theTime):
        """Auto-save the summary file to a temporary file name at regular intervals, if necessary.

        Saves and restores current selection state.
        Inputs:
          theTime: unused.
        """
        if self.autoSaveFlag:
            mySelection = self.currentContent.listRef.adapter.selection[:]
            self.autoSaveFileName = self.summary.summaryFileName + "." + self.timeStamp
            self.SaveSummary(theFileName=self.autoSaveFileName,
                             theAutoCommitFlag=False,
                             theSaveUnmatchedFlag=True,
                             theAutoSaveFlag=True)
            self.currentContent.listRef.adapter.selection = mySelection
            self.autoSaveFlag = False

    def SaveSummaryAs(self, theFileName, theAutoCommitFlag, theSaveUnmatchedFlag):
        """Save summary file as a new file, checking for overwrites.

        Inputs:
          theFileName: The name of the summary file.
          theAutoCommitFlag: If true, automatically commit uncommited references.
            (references copied from other modes)
          theSaveUnmatchedFlag: If true, save unmatched lines.
            (Non comment lines only in summary file.)
        """
        if os.path.exists(theFileName):
            self.confirmSavePopupRef.title = "Overwrite " + theFileName;
            self.confirmSavePopupRef.open()
        else:
            self.SaveSummary(theFileName, theAutoCommitFlag, theSaveUnmatchedFlag)

    def ExportSummaryAs(self, theFileName):
        """Export summary file as a CSV file, checking for overwrites.

        Inputs:
          theFileName: The name of the CSV file.
        """
        if os.path.exists(theFileName):
            self.confirmExportPopupRef.title = "Overwrite " + theFileName;
            self.confirmExportPopupRef.open()
        else:
            self.ExportSummary(theFileName)

    def _UpdateDisplayList(self, theContent, theAutoCommitFlag, theSaveUnmatchedFlag):
        """Update display list according to flags.

        Inputs:
          theContent: The content corresponding to display list mode.
          theAutoCommitFlag: If true, automatically commit uncommited references.
            (References copied from other modes.)
          theSaveUnmatchedFlag: If false, delete unmatched lines.
            (Non comment lines in summary file.)
        Update uncommitted, unmatched items.
        """
        myList = theContent.report.displayList
        for line_it in range(len(myList))[::-1]:
            # reversed because of possible delete
            myLine = myList[line_it]
            if myLine['type'] == 'uncommitted' and theAutoCommitFlag:
                myLine['reference'] = myLine['reference'][2:]  # remove "? "
                myLine['type'] = 'checked'
            elif myLine['type'] == 'unmatched' and theSaveUnmatchedFlag:
                # This is temporary. Must restore after save.
                myLine['reference'] = myLine['reference'][2:]  # remove "* "
            elif myLine['type'] == 'unmatched' and not theSaveUnmatchedFlag:
                del myList[line_it]

    def _InitializeSave(self):
        """Initialize variables for save function."""
        self._discrepancyFound = False
        self._lastAppliedModes = []
        self._trueLastOutputModes = []
        self._lastData = ""
        self._lastOutput = ""

    def _PrintSaveBanner(self, theFileName,
                             theAutoCommitFlag, theSaveUnmatchedFlag, theAutoSaveFlag):
        """Print banner with save file parameters."""
        myAutoCommitText = ", saving uncommitted checks " \
            if theAutoCommitFlag else ", ignoring uncommitted checks"
        mySaveUnmatchedText = ", saving unmatched checks " \
            if theSaveUnmatchedFlag else ", ignoring unmatched checks"
        print("Saving " + os.path.basename(theFileName) + myAutoCommitText + mySaveUnmatchedText)
        if not theAutoSaveFlag:
            print("Checking for summary discrepancies between modes...")

    def _PrintCSVHeader(self, theExportFile, theModeList):
        """Print header for CSV file."""
        theExportFile.write("Reference,Level,Type,Device")
        for mode_it in theModeList:
            theExportFile.write(",%s" % mode_it)
        theExportFile.write("\n")

    def _GetModes(self, theHeaderList):
        """Returns a list of all modes displayed."""
        myList = []
        for header_it in theHeaderList:
            myList.append(header_it.content.report.modeName)
        return myList

    def _GetDisplayLists(self, theAutoCommitFlag, theSaveUnmatchedFlag):
        """Return dict of all updated display lists. Pushes to undo stack, if necessary.

        Inputs:
          theAutoCommitFlag: If true, automatically commit uncommited references.
            (References copied from other modes.)
          theSaveUnmatchedFlag: If true, save unmatched lines.
            (Non comment lines in summary file.)
        Push undo stack if necessary.
        Update uncommitted, unmatched items.
        """
        myDisplayList = {}
        for header_it in self.modePanelRef.tab_list:
            myReport = header_it.content.report
            # Following 2 conditions may cause changes to summary file, so save in undo stacks.
            if myReport.errorCount['uncommitted'] > 0 and theAutoCommitFlag:
                self._AddUndo(header_it.content, myReport.displayList)
            elif myReport.errorCount['unmatched'] > 0 and not theSaveUnmatchedFlag:
                self._AddUndo(header_it.content, myReport.displayList)
            self._UpdateDisplayList(header_it.content,
                                    theAutoCommitFlag, theSaveUnmatchedFlag)
            myDisplayList[myReport.modeName] = myReport.displayList
        return myDisplayList

    def _Unprinted(self, theIndexes, theDisplayLists):
        """Return true if there are unprinted errors.

        Inputs:
          theIndexes: List of indexes into each displayList for each mode.
          theDisplayLists: List of displayLists for each mode.
        """
        for mode_it in theIndexes:
            if theIndexes[mode_it] < len(theDisplayLists[mode_it]):
                return True
        return False

    def _GetLeastItem(self, theItems):
        """Return the least error in the list of theItems.

        Inputs:
          theItems: List of errors, one for each mode.
        """
        myLeastItem = None
        for item_it in theItems:
            if CompareErrors(myLeastItem, theItems[item_it]) != "<":
                myLeastItem = theItems[item_it]
        return myLeastItem

    def _FindOutputModes(self, theOutputItem, theIndex, theItems, theModeList):
        """Return output modes for theOutputItem and update theIndex array.

        Inputs:
          theOutputItem: Data to output.
          theIndex: List of indices to display lists. Updated here.
          theItems: List of one item per mode.
          theModeList: List of modes.
        """
        myAppliedModes = []
        myOutputModes = []
        for mode_it in theModeList:  # combine modes for matching references
            if (theItems[mode_it]
                    and theOutputItem['reference'] == theItems[mode_it]['reference']
                    and theOutputItem['data'] == theItems[mode_it]['data']):
                myAppliedModes.append(mode_it)
                theIndex[mode_it] += 1
                if theItems[mode_it]['type'] in ['checked', 'comment', 'unmatched']:
                    # if not theSaveUnmatchedFlag, 'unmatched' records have been deleted.
                    myOutputModes.append(mode_it)
        return (myAppliedModes, myOutputModes)

    def _CheckDiscrepancies(self, theOutputItem, theOutput, theAppliedModes, theAutoSaveFlag):
        """Print discrepancies in summary between modes."""
        if theOutput.startswith("#"):  # ignore differences in comments
            return
        if (not theAutoSaveFlag
                and theOutputItem['data'] == self._lastData
                and theOutput != self._lastOutput):  # same error, but different summary
            self._discrepancyFound = True
            print("modes %s %s\nmodes %s %s\n\n" % (str(self._lastAppliedModes), self._lastOutput,
                                                    str(theAppliedModes), theOutput))
        self._lastData = theOutputItem['data']
        self._lastOutput = theOutput
        self._lastAppliedModes = theAppliedModes

    def _OutputSummary(self, theOutput, theOutputModes, theModeList, theSummaryFile):
        """Output one summary line, possibly with mode list heading."""
        if theOutputModes != self._trueLastOutputModes:
            if len(theOutputModes) == len(theModeList):
                theSummaryFile.write("\n#ALWAYS\n")
            else:
                theSummaryFile.write("\n#IFNOT %s\n" % " ".join(
                    sorted(set(theModeList).difference(theOutputModes))))
            self._trueLastOutputModes = theOutputModes
        theSummaryFile.write("%s\n" % theOutput)

    def _ExportCSV(self, theExportMatch, theOutputModes, theModeList, theExportFile):
        """Export one CSV summary line."""
        # \1 reference, \2 level, \3 type, \4 device, (\5 count)
#        print(theExportMatch.groups()[0:3])
        theExportFile.write("%s,%s,%s,%s" % (theExportMatch.groups()[0:4]))
        if len(theExportMatch.groups()) == 5:
            myCount = theExportMatch.group(5)
        else:
            myCount = "*"
        for mode_it in theModeList:
            if mode_it in theOutputModes:
                theExportFile.write(",'%s" % myCount)
            else:
                theExportFile.write(",")
        theExportFile.write("\n")

    def _FinalizeSave(self, theFileName, theSaveUnmatchedFlag, theAutoSaveFlag):
        """Finish save by recaculating totals and setting variables."""
        if not theAutoSaveFlag:
            if not self._discrepancyFound:
                print("no discrepancies found.")
            self.summary.summaryFileName = theFileName
            self.currentContent.RedisplayErrors()
            self.changedFlag = False
        for header_it in self.modePanelRef.tab_list:
            header_it.content.report.CountErrors()
            header_it.content.ids.summaryFileName_id.text = self.summary.summaryFileName
            if theSaveUnmatchedFlag:  # leading "* " have been removed
                for display_it in header_it.content.report.displayList:
                    if display_it['type'] == 'unmatched':  # re-add leading "* " for unmatched
                        display_it['reference'] = "* " + display_it['reference']
        self.autoSaveFlag = False
        self.savePopupRef.dismiss()
        self.confirmSavePopupRef.dismiss()

    def RemoveAutoSaveFile(self):
        """Attempt to remove last auto-saved file.

        Ignore failed attempts.
        """
        try:
            os.remove(self.autoSaveFileName)
        except:
            pass

    def SaveSummary(self, theFileName,
                    theAutoCommitFlag, theSaveUnmatchedFlag, theAutoSaveFlag=False):
        """Save the summary file.

        Inputs:
          theFileName: The name of the summary file.
          theAutoCommitFlag: If true, automatically commit uncommited references.
            (references copied from other modes)
          theSaveUnmatchedFlag: If true, save unmatched lines.
            (Non comment lines only in summary file.)
          theAutoSaveFlag: If true, this is an auto-save.
        Display lists from all modes are grouped before output.
        """
        self.RemoveAutoSaveFile()
        try:
            mySummaryFile = open(theFileName, "w")
        except Exception as inst:
            print(type(inst), inst.args)     # the exception instance & arguments stored in .args
            print('could not open ' + theFileName)
            return

        self._PrintSaveBanner(theFileName,
                              theAutoCommitFlag, theSaveUnmatchedFlag, theAutoSaveFlag)
        self._InitializeSave()
        myModeList = sorted(self._GetModes(self.modePanelRef.tab_list))
        myDisplayLists = self._GetDisplayLists(theAutoCommitFlag, theSaveUnmatchedFlag)
        myIndex = {mode_it: 0 for mode_it in myModeList}
        while self._Unprinted(myIndex, myDisplayLists):
            myItems = {}
            for mode_it in myModeList:  # one item from each mode
                myItems[mode_it] = myDisplayLists[mode_it][myIndex[mode_it]] \
                                   if myIndex[mode_it] < len(myDisplayLists[mode_it]) else {}
            myOutputItem = self._GetLeastItem(myItems)
            (myAppliedModes, myOutputModes) = self._FindOutputModes(myOutputItem, myIndex,
                                                                    myItems, myModeList)
            myOutput = "%s:%s" % (myOutputItem['reference'], myOutputItem['data'])
            self._CheckDiscrepancies(myOutputItem, myOutput, myAppliedModes, theAutoSaveFlag)
            if myOutputModes:
                self._OutputSummary(myOutput, myOutputModes, myModeList, mySummaryFile)
        mySummaryFile.close()
        self._FinalizeSave(theFileName, theSaveUnmatchedFlag, theAutoSaveFlag)

    def ExportSummary(self, theFileName):
        """Export the summary file as CSV file.

        Inputs:
          theFileName: The name of the CSV file.
        Display lists from all modes are grouped before output.
        """
        try:
            myExportFile = open(theFileName, "w")
        except Exception as inst:
            print(type(inst), inst.args)     # the exception instance & arguments stored in .args
            print('could not open ' + theFileName)
            return

        print("Exporting " + os.path.basename(theFileName))
        self._InitializeSave()
        myModeList = sorted(self._GetModes(self.modePanelRef.tab_list))
        self._PrintCSVHeader(myExportFile, myModeList)
        myDisplayLists = self._GetDisplayLists(theAutoCommitFlag=False, theSaveUnmatchedFlag=False)
        myIndex = {mode_it: 0 for mode_it in myModeList}
        myLogRE = re.compile("^(\[.*?\]) (.*?),(.*?) .*(\(.*?\)/[^\s\(]*) ")
        # \1 reference, \2 level, \3 type, \4 device
        myErrorRE = re.compile("^(\[.*?\]) (.*?),(.*?) .*? SUBCKT (\S*) error count (\d*/\d*)")
        # \1 reference, \2 level, \3 type, \4 device, \5 count
        while self._Unprinted(myIndex, myDisplayLists):
            myItems = {}
            for mode_it in myModeList:  # one item from each mode
                myItems[mode_it] = myDisplayLists[mode_it][myIndex[mode_it]] \
                                   if myIndex[mode_it] < len(myDisplayLists[mode_it]) else {}
            myOutputItem = self._GetLeastItem(myItems)
            (myAppliedModes, myOutputModes) = self._FindOutputModes(myOutputItem, myIndex,
                                                                    myItems, myModeList)
            myOutput = "%s,%s" % (myOutputItem['reference'], myOutputItem['data'])
            if myOutputModes and not myOutput.startswith("#"):
                myExportMatch = myErrorRE.search(myOutput)
                if not myExportMatch:
                    myExportMatch = myLogRE.search(myOutput)
                if myExportMatch:
                    self._ExportCSV(myExportMatch, myOutputModes, myModeList, myExportFile)
                else:
                    print("Could not create CSV data for: " + myOutput)
        myExportFile.close()
        self.exportPopupRef.dismiss()
        self.confirmExportPopupRef.dismiss()

    def Quit(self):
        """Check for summary changes before ending."""
        if self.changedFlag:
            self.quitPopupRef.open()
        else:
            Window.close()

#23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
