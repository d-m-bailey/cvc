""" SummaryApp.py: Kivy App class for check_cvc.

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

try:  # kivy installed
    from kivy.config import Config
    from kivy.app import App
    from kivy.properties import NumericProperty
    from summaryGUI import SummaryWidget

    class SummaryApp(App):
        """Kivy App class for check_cvc.

        Class variables:
          modeRows: Number of rows in tabbed panel.
          modeColumns: Number of columns in tabbed panel.
        Instance variables:
          summaryList: raw summary list for all modes
          reportList: list of reports; one for each log/error file combination
          root; the top widget, SummaryWidget
        Methods:
          __init__: Create instance with summary data and list of report data.
          build: Create top widget and start event loop.
          EnableReferenceButtons: Stub to root method.
          SetCheckFilters: Stub to root method for display check box filters.
          SetCopyFilters: Stub to root method for copy check box filters.
        """
        modeRows = NumericProperty(1)
        modeColumns = NumericProperty(10)
        
        def __init__(self, theSummaryList, theReportList, **kwargs):
            """Create instance with summary data and list of report data.

            Inputs:
              theSummaryList: raw summary list for all modes
              theReportList: list of reports: one for each log/error file combination
            """
#            Config.set('kivy', 'log_level', 'critical')
            self.summaryList = theSummaryList
            self.reportList = theReportList
            super(SummaryApp, self).__init__(**kwargs)
        
        def build(self):
            """Create top widget and start event loop."""
            return SummaryWidget(self, self.summaryList, self.reportList)

        def EnableReferenceButtons(self):
            """Stub to root method."""
            self.root.EnableReferenceButtons()

        def SetCheckFilters(self, theId):
            """Stub to root method for display check box filters."""
            self.root.SetFilters(theId, 'show_', self.root.checkValues)

        def SetCopyFilters(self, theId):
            """Stub to root method for copy check box filters."""
            self.root.SetFilters(theId, 'copy_', self.root.copyValues)
   

except ImportError:  # kivy not installed
    print("Error loading kivy")
    
    class SummaryApp():       
        """Dummy App class for when kivy not installed.

        Print check/error totals.
        """
        def __init__(self, theSummaryList, theReportList, **kwargs):
            for report_it in theReportList:
                report_it.CreateDisplayList(theSummaryList)
    
#23456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
