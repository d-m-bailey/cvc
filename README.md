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

Installation
------------

Requirements:
- gcc 4.9.3
- python 2.7.10
- bison 3.0.1 (only if making changes to parser or compiling from github)

GUI requirements:
- kivy 1.10.0
- pyinstaller 3.1.1 (for standalone check_cvc)

Install CVC by:

1. download tarball from release page https://github.com/d-m-bailey/cvc/releases.
2. extract
3. cd cvc-\<version>
4. autoreconf -vif
5. ./configure --disable-nls [--prefix=<install_directory>]
6. make install

There have been problems compiling check_cvc on some Linux platfroms.
If GUI check_cvc does not compile, try
export SDL_VIDEO_X11_VISUALID=

If that still doesn't work, the check_cvc.py file in src_py can also be run via IDLE on PCs.
Just transfer the \*.log and \*.error.gz files.

Contribute
----------

- Issue Tracker: https://github.com/d-m-bailey/cvc/issues

Support
-------

If you are having issues, please let us know.
We have a mailing list located at: cvc@shuharisystem.com

License
-------

The project is licensed under the GPLv3 license.
