# Makefile for processing several cvcrc files in a given directory based on netlist, power, and model files.
# define modes
modes := mode1 mode2
makefiles := $(modes:%=makefile.%)

# get date from current directory
date := $(notdir $(PWD))

all : clean $(makefiles) run

run :
  @read -r -p "Press enter to continue: "        
  @chmod u+x batchecv        
  ./batchecv
  
.SECONDEXPANSION:
$(modes) : clean makefile.$$@

clean:         
  @rm -f batchecv        
  @rm -f $(makefiles)
  
makefile.% : cvcrc.%
  @echo export date = $(date) > $@
  @sed -e 's/$$\([A-Za-z0-9_][A-Za-z0-9_]*\)/$$(\1)/g' -e 's/{/(/g' -e 's/}/)/g' $< >> $@
  @echo '$$(CVC_REPORT_FILE) : $$(CVC_NETLIST) $$(CVC_MODEL_FILE) $$(CVC_POWER_FILE)' >> $@
  @echo " cvc $<" >> $@
  @make --no-print-directory -n -f $@ date=$(date)         
  @echo "make --no-print-directory -f $@ date=$(date)" >> batchecv
