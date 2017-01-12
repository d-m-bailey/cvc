# Makefile for creating a batch job to run CVC on cvcrc files in the current directory 
# based on corresponding netlist, power, and model files.

# usage:
# make  <- default
# make all  <- same as default
# make mode1 mode2  <- for specified modes
# make cvcrc.mode1 cvcrc.modex  <- for specified modes

# First, delete batch job.
# Second, create batch job, displaying all specified modes that need to be rerun.
# Finally, upon confirmation, run batch job.

# Define modes.
MODES := mode1 mode2 mode3
MAKEFILES := $(MODES:%=makefile.%)
CVCRCFILES := $(MODES:%=cvcrc.%)

# Set first and last goal.
ifneq ($(MAKECMDGOALS),)
FIRST_GOAL := $(word 1, $(MAKECMDGOALS))
LAST_GOAL := $(word $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))
else
FIRST_GOAL := all
LAST_GOAL := all
endif

# Get date from current directory.
DATE := $(notdir $(PWD))

# Dummy default rule.
.PHONY: all
all ::

# Remove files before first target.
$(FIRST_GOAL) ::
	@echo cleaning files
	@rm -f batchecv
	@rm -f $(MAKEFILES)

# Real default rule.
all :: $(MAKEFILES)

.SECONDEXPANSION:
$(MODES) :: makefile.$$@

$(CVCRCFILES) :: $$(patsubst cvcrc.%,makefile.%,$$@)

makefile.% :
# Create makefile for each mode based on corresponding cvcrc file.
	@echo export date = $(DATE) > $@
# Convert cvcrc shell variables to make variables.
	@sed -e 's/$$\([A-Za-z0-9_][A-Za-z0-9_]*\)/$$(\1)/g' \
		-e 's/{/(/g' -e 's/}/)/g' $(@:makefile.%=cvcrc.%) >> $@
# Create report file target.
	@echo '$$(CVC_REPORT_FILE) :: $$(CVC_NETLIST) $$(CVC_MODEL_FILE) $$(CVC_POWER_FILE)' >> $@
	@echo "	cvc $(@:makefile.%=cvcrc.%)" >> $@
	@echo "" >> $@
# Check that report file ended correctly. If not, touch cvcrc file to rebuild.
	@echo "check ::" >> $@
	@echo '	@grep -q "CVC: End:" $$(CVC_REPORT_FILE) \
		&& true || touch $(@:makefile.%=cvcrc.%)' >> $@
	@$(MAKE) --no-print-directory -f $@ check
# Create batchecv file with commands for all modes that need updating.
	@$(MAKE) --no-print-directory -q -f $@ \
			&& echo "mode $(@:makefile.%=%) is up to date" \
			|| $(MAKE) --no-print-directory -n -f $@ | \
		uniq >> batchecv

# Run batchecv after last target, if batchecv file exists.
$(LAST_GOAL) ::
# In case batchecv does not exist, create empty file.
	@touch batchecv
	@echo ""
	@cat batchecv
# Pause for comfirmation if there are updates.
	@[ -s batchecv ] \
		&& read -r -n 1 -p "Press any key to continue, ctrl-C to quit: ==>" \
		|| echo no update necessary
	@chmod u+x batchecv
	@export date=$(date); ./batchecv

#23456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*
