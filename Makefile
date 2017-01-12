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
	@sed -e 's/$$\([A-Za-z0-9_][A-Za-z0-9_]*\)/$$(\1)/g' \
		-e 's/{/(/g' -e 's/}/)/g' $(@:makefile.%=cvcrc.%) >> $@
	@echo '$$(CVC_REPORT_FILE) :: $$(CVC_NETLIST) $$(CVC_MODEL_FILE) $$(CVC_POWER_FILE)' >> $@
	@echo "	cvc $(@:makefile.%=cvcrc.%)" >> $@
# Display makefile result without executing.
	@$(MAKE) --no-print-directory -n -f $@ 
# If update needed, add to batch job.
	@$(MAKE) --no-print-directory -q -f $@ && \
		echo "make --no-print-directory -f $@" >> batchecv

# Run batchecv after last target, if batchecv file exists.
$(LAST_GOAL) ::
ifneq ($(wildcard batchecv),)
	@read -r -p "Press enter to continue: "
	@chmod u+x batchecv
	./batchecv
endif

#23456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*123456789*
