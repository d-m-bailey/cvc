proc load_selection {} {
	global gClipboardSource
	if { ! [info exists gClipboardSource] } {
		set gClipboardSource clipboard
	}
	set myClipboard [open "|xclip -o -selection $gClipboardSource" r]
	if { "$myClipboard" eq "" } {
		set mySelelction ""
	} else {
		gets $myClipboard mySelection
	}
	Gui:Print $mySelection

	set visu [Gui:GetMainVisualizer]

	set db [Gui:GetDataBase $visu]
	set topoid [Gui:GetCurrentModule $visu]
	if { $topoid == "" } {
		$db foreach top topoid {
			break
		}
	}
	set topcell [$db oid root $topoid]
	set path [$db oid path $topoid]

	set oidList {}
	regsub {\([^\)]*\)[     ]*$} $mySelection "" mySelection
	regsub -all {\([^\)]*\)/} $mySelection " " mySelection
	regsub -all {==} $mySelection "--" mySelection
	regsub {^/} $mySelection "" mySelection
	#regsub -all {/} $mySelection "-" mySelection
	#regsub -all {\[} $mySelection "<" mySelection
	#regsub -all {\]} $mySelection ">" mySelection
	#Gui:Print $mySelection
	lappend oidList "inst $topcell $path $mySelection"
	Gui:AppendCone $visu $oidList

	set col 12

	#set db [Gui:GetDataBase $visu]
	foreach oid $oidList {
		$db flathilight $oid set $col
	}
	Gui:HighlightChanged

	set oidList {}
	regsub { [^ ]*$} $mySelection "" mySelection
	#Gui:Print $mySelection
	lappend oidList "inst $topcell $path $mySelection"
	Gui:AppendCone $visu $oidList

	Gui:ActivateTab $visu Search&Cone
}

proc toggle_selection {} {
	global gClipboardSource
	if { [info exists gClipboardSource] && "$gClipboardSource" eq "clipboard" } {
		set gClipboardSource primary
		Gui:Print "Clipboard selection set to X"
	} else {
		set gClipboardSource clipboard
		Gui:Print "Clipboard selection set to Windows"
	}
}

bind . <Control-k> {load_selection}
bind . <Control-K> {toggle_selection}
