## This is a automake file, part of Unidata's netCDF package.
# Copyright 2011, see the COPYRIGHT file for more information.

# This file builds and runs DAP tests.

# Put together AM_CPPFLAGS and AM_LDFLAGS.
include $(top_srcdir)/lib_flags.am

LDADD = ${top_builddir}/liblib/libnetcdf.la
AM_CPPFLAGS += -I$(top_builddir)/liblib -I$(top_builddir)/libdap4 -I$(top_builddir)/include

# Set up the tests; do the .sh first, then .c
check_PROGRAMS =
TESTS =
TESTS_ENVIRONMENT=TOPSRCDIR=${abs_top_srcdir}

test_ncuri_SOURCES = test_ncuri.c
test_parse_SOURCES = test_parse.c test_common.h
test_meta_SOURCES = test_meta.c test_common.h
test_data_SOURCES = test_data.c test_common.h

check_PROGRAMS += test_parse test_meta test_data
check_PROGRAMS += test_ncuri

TESTS += test_ncuri
TESTS += test_parse.sh
TESTS += test_meta.sh
TESTS += test_data.sh
TESTS += test_raw.sh

if ENABLE_DAP4_REMOTE_TESTS
endif

EXTRA_DIST = test_parse.sh test_meta.sh test_data.sh \
             test_raw.sh test_remote.sh test_hyrax.sh \
             test_common.sh \
	     daptestfiles dmrtestfiles cdltestfiles nctestfiles \
	     baseline baselineraw baselineremote

CLEANFILES = *.exe

# One last thing
BUILT_SOURCES = .daprc

.daprc:
	echo "#DAPRC" >.daprc

DISTCLEANFILES = .daprc results/* results

# Build the findtestserver program, but do not install
bin_PROGRAMS =
bin_PROGRAMS += findtestserver
findtestserver_SOURCES = findtestserver.c nctestserver.c


# The shell file maketests.sh is used to build the testdata
# for dap4 testing. It creates and fills the directories
# daptestfiles, dmrtestfiles, and cdltestfiles
# As a rule, this will only be invoked if there was a change
# in the GenerateRaw.java program in the thredds tree.
maketests::
	bash ./maketests.sh
