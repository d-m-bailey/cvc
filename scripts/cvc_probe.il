; cvc_probe_selection, cvc_probe_device, cvc_probe_net: loads a hierarchy

; Copyright 2016 D. Mitch Bailey, Shuhari System

;   This program is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.

;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.

;   You should have received a copy of the GNU General Public License
;   along with this program.  If not, see <http://www.gnu.org/licenses/>.

; Set the following 2 variables for each project
myTopLib = "????"
myTopBlock = "????"
myArrayStart = '_
myArrayEnd = '_

procedure(cvc_probe_selection()
	; Probe the clipboard text.
	; Requires xclip
	myClipboard = ipcBeginProcess("xclip -o -selection clipboard")
	ipcWait(myClipboard)
	mySelection = ipcReadProcess(myClipboard)
	println(list("probing " mySelection))
	cvc_probe_device(mySelection)
)

procedure(cvc_probe_device(theDevicePath)
	; Probe the device starting from the top of the hierarchy.
	; Sample: /XTOP(TOP)/XMAIN_0(MAIN)/M0
	rexCompile("([^)]*)")
	myDevicePath = rexReplace(theDevicePath "" 0)  ; Remove cell names '(*)'.
	myDb = dbOpenCellViewByType(myTopLib myTopBlock "schematic")
	myWindow = geOpen(
		?window hiOpenWindow()
		?lib myTopLib
		?cell myTopBlock
		?view "schematic"
		?viewType "schematic"
		?mode "r"
	)
;       print(myDb~>instances->??)
	myHierarchy = parseString(myDevicePath "/")
	myDevice = car(reverse(myHierarchy))
	myCellPath = reverse(cdr(reverse(myHierarchy)))
	println(list(myHierarchy myCellPath myDevice))
	while(myCellPath != '() && myDb != nil
		myInstance = split_array_instance(car(myCellPath))
		myDb = find_cvc_instance(geGetWindowCellView() car(myInstance))
		if(myDb then
			geSwitch(myWindow "r" myDb get_offset(myDb cadr(myInstance)) 0 0)
			myCellPath = cdr(myCellPath)
		)
	)
	geSelectObject(find_cvc_instance(geGetWindowCellView() myDevice))
)

procedure(cvc_probe_net()
;       myNetPath = clipboard
	myNetPath = "/IO/I"
	myDb = dbOpenCellViewByType("cvc_test" "test" "schematic")
	myWindow = geOpen(
		?window hiOpenWindow()
		?lib myTopLib
		?cell myTopBlock
		?view "schematic"
		?viewType "schematic"
		?mode "r"
	)
;       print(myDb~>instances->??)
	myHierarchy = parseString(myNetPath "/")
	myNet = car(reverse(myHierarchy))
	myCellPath = reverse(cdr(reverse(myHierarchy)))
	println(list(myHierarchy myCellPath myNet))
	while(myCellPath != '() && myDb != nil
		myInstance = split_array_instance(car(myCellPath))
		myDb = find_cvc_instance(geGetWindowCellView() car(myInstance))
		if(myDb then
			geSwitch(myWindow "r" myDb get_offset(myDb cadr(myInstance)) 0 0)
			myCellPath = cdr(myCellPath)
		)
	)
	geAddNetProbe(find_cvc_instance(geGetWindowCellView() myNet))
)

procedure(find_cvc_instance(myDb instanceName)
	prog((myInstances myInstance)
		myInstances = myDb~>instances
		while( car(myInstances)
			myInstance = car(myInstances)
			; Skip the first letter. CDL adds 'X', 'M', 'R', etc. not in schematic.
			if( myInstance~>baseName == substring(instanceName 2) then
				println(strcat("found " instanceName))
				return(myInstance)
			)
			; Skip the first two letters.
			; Flattened cells may add 'MX' to schematic instance name.
			if( myInstance~>baseName == substring(instanceName 3) then
				println(strcat("found " instanceName))
				return(myInstance)
			)
			myInstances = cdr(myInstances)
		)
		println(strcat("could not find " instanceName))
		return('nil)
	)
)

procedure(split_array_instance(instanceName)
	prog((myArrayBase myOffsetString myIndexLength)
		if(getchar(instanceName strlen(instanceName)) == myArrayEnd then
			myArrayBase = substring(instanceName 1 strlen(instanceName)-1)
			myOffsetString = substring(rindex(myArrayBase myArrayStart) 2)
			myIndexLength = strlen(myOffsetString) + 2
			println(strcat("array " substring(instanceName 1 strlen(instanceName)-myIndexLength) " " myOffsetString))
			return(list(substring(instanceName 1 strlen(instanceName)-myIndexLength) evalstring(myOffsetString)))
		)
		return(list(instanceName 0))
	)
)

procedure(get_offset(instance offset)
	prog((myArray myRange myStart myEnd)
		myArray = index(instance~>name "<")
		if( myArray then
			myRange = parseString(myArray "<:>")
			myStart = evalstring(car(myRange))
			myEnd = evalstring(cadr(myRange))
			println(list("range " myStart ":" myEnd " index " offset ))
			if( myStart < myEnd then
				return(offset - myStart)
			else
				return(myStart - offset)
			)
		)
		return(0)
	)
)
