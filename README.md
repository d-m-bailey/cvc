# CVC

CVC: Circuit Validity Checker. Voltage aware ERC checker for CDL netlists.

Features
--------
- Input netlist format is Calibre LVS CDL (Mentor, a Siemens business)
- Checks netlists with up to 4B devices (2^32).
- Power and device parameters from Microsoft Excel
- Hierarchical power files possible
- Ability to differentiate models by parameters
- Setup option to list models and power nets
- All rules are automated. No need to write rule files.
- Interactive netlist analyzer
- Script execution available
- Automatic subcircuit debug environment creation
- GUI to record error analyses results

Includes:  
src: Source code for CVC  
src_py: Source code for check_cvc, python/kivy program for checking results.  
scripts: Auxillary programs, scripts, and macros for CVC  
doc: Doxygen settings, codes for errors  

$project
========

$project will solve your problem of where to start with documentation,
by providing a basic explanation of how to do it easily.

Look how easy it is to use:

    import project
    # Get your stuff done
    project.do_stuff()

Installation
------------

Install $project by running:

    install project

Contribute
----------

- Issue Tracker: github.com/$project/$project/issues
- Source Code: github.com/$project/$project

Support
-------

If you are having issues, please let us know.
We have a mailing list located at: cvc@shuharisystem.com

License
-------

The project is licensed under the GPLv3 license.
