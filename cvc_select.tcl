proc load_selection {} {
        set myClipboard [open "|xclip -o -selection clipboard" r]
        gets $myClipboard mySelection
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

        set     oidList {}
        regsub -all {\([^\)]*\)/} $mySelection " " mySelection
        regsub {^/} $mySelection "" mySelection
        regsub -all {/} $mySelection "-" mySelection
        #Gui:Print $mySelection
        lappend oidList "inst $topcell $mySelection"
        Gui:AppendCone $visu $oidList

        set col 14

        #set db [Gui:GetDataBase $visu]
        foreach oid $oidList {
                $db flathilight $oid set $col
        }
        Gui:HighlightChanged

        set     oidList {}
        regsub { [^ ]*$} $mySelection "" mySelection
        #Gui:Print $mySelection
        lappend oidList "inst $topcell $mySelection"
        Gui:AppendCone $visu $oidList

        Gui:ActivateTab $visu Cone
}

bind . <Control-k> {load_selection}
