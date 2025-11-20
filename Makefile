# SGDK Makefile for Docker

# The zerasul/sgdk image puts SGDK at /sgdk
GDK := /sgdk

# Allow injection of flags from command line (e.g. EXTRA_FLAGS="-DTEST_BUILD")
OPTIONS += $(EXTRA_FLAGS)

# Include the standard SGDK makefile
include $(GDK)/makefile.gen
