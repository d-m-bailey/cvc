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
myTopLib = ???
myTopBlock = ???

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
;	print(myDb~>instances->??)
	myHierarchy = parseString(myDevicePath "/")
	myDevice = car(reverse(myHierarchy))
	myCellPath = reverse(cdr(reverse(myHierarchy)))
	println(list(myHierarchy myCellPath myDevice))
	while(myCellPath != '() && myDb != nil
		myDb = find_cvc_instance(geGetWindowCellView() car(myCellPath))
		if(myDb then
			geSwitch(myWindow "r" myDb 0 0 0)
			myCellPath = cdr(myCellPath)
		)
	)
	geSelectObject(find_cvc_instance(geGetWindowCellView() myDevice))
)

procedure(cvc_probe_net()
;	myNetPath = clipboard
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
;	print(myDb~>instances->??)
	myHierarchy = parseString(myNetPath "/")
	myNet = car(reverse(myHierarchy))
	myCellPath = reverse(cdr(reverse(myHierarchy)))
	println(list(myHierarchy myCellPath myNet))
	while(myCellPath != '() && myDb != nil
		myDb = find_cvc_instance(geGetWindowCellView() car(myCellPath))
		if(myDb then
			geSwitch(myWindow "r" myDb 0 0 0)
			myCellPath = cdr(myCellPath)
		)
	)
	geAddNetProbe(find_cvc_instance(geGetWindowCellView() myNet))
)

procedure(find_cvc_instance(myDb instanceName)
	prog((myInstances)
		myInstances = myDb~>instances
		while( car(myInstances)
			myInstance = car(myInstances)
			; Skip the first letter. CDL adds 'X', 'M', 'R', etc. not in schematic.
			if( myInstance~>name == substring(instanceName 2) then
				println(strcat("found " instanceName))
				return(myInstance)
			)
			; Skip the first two letters.
			; Flattened cells may add 'MX' to schematic instance name.
			if( myInstance~>name == substring(instanceName 3) then
				println(strcat("found " instanceName))
				return(myInstance)
			)
			myInstances = cdr(myInstances)
		)
		println(strcat("could not find " instanceName))
		return('nil)
	)
)

