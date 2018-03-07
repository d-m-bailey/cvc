
checkcvcdir = $(datadir)/check_cvc
checkcvc_DATA = ResultFile.py \
	SummaryApp.py \
	SummaryFile.py \
	check_cvc.py \
	cvc_globals.py \
	summary.kv \
	summaryGUI.py \
	utility.py
bin_PROGRAMS = dist/check_cvc

dist/check_cvc : $(checkcvc_DATA) check_cvc.spec
	pyinstaller -F check_cvc.spec
