#   box.awk: awk script to replace box devices in CDL netlist

#   Copyright 2106 D. Mitch Bailey

#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
BEGIN {
	# Set the value to the device prefix
        # two terminal devices are instances of boxes with 2 terminals
	twoTerminalDevice["res1"] = "R";
        # three terminal devices are instances of boxes with 2 terminals and SUB connection
	threeTerminalDevice["res2"] = "R";
}
/^+/ && myLine != "" {  # continuation lines. Remove leading '+' and join to previous line.
	sub(/^+/, " ");
	$0 = myLine $0;
	myLine = "";
}
myLine != "" {  # non continuation line following save line. Print the saved data.
	print myLine;
	myLine = "";
}
/^X/ {  # Instantiation. Processed multiple times if number of fields is too small.
	if ( NF > 3 && $4 in twoTerminalDevice ) {
		$0 = twoTerminalDevice[$4] $0;
	} else if ( NF > 4 && $5 in threeTernminalDevice ) {
		subTerminal = "$SUB=" $4;
		$4 = $5;
		$5 = subTerminal;
		$0 = threeTerminalDevice[$4] $0;
	} else if ( NF < 5 ) {  # Save line and try again
		myLine = $0;
		next;
	}
}
 {
	print $0
}
