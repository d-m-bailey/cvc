
#checkcvcdir = $(datadir)/check_cvc
checkcvc_sources = ResultFile.py \
	SummaryApp.py \
	SummaryFile.py \
	check_cvc.py \
	cvc_globals.py \
	summary.kv \
	summaryGUI.py \
	utility.py

all : ;

install : ;

check_cvc : $(checkcvc_sources) check_cvc.spec
	pyinstaller -F check_cvc.spec --clean
	cp dist/check_cvc .

install_check_cvc : check_cvc
	cp check_cvc ${exec_prefix}/bin
