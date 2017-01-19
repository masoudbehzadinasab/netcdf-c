#!/bin/sh

if test "x$srcdir" = "x"; then srcdir=`dirname $0`; fi
export srcdir;

. ${srcdir}/test_common.sh

F="\
nc4_nc_classic_comp.nc \
nc4_nc_classic_no_comp.nc \
nc4_strings.nc \
nc4_strings_comp.nc \
nc4_unsigned_types.nc \
nc4_unsigned_types_comp.nc \
ref_tst_compounds.nc \
"

failure() {
      echo "*** Fail: $1"
      exit 1
}

rm -fr ./results
mkdir -p ./results

if test "x${RESET}" = x1 ; then rm -fr ${BASELINEH}/*.dmp ; fi
for f in $F ; do
    URL="dap4://test.opendap.org:8080/opendap/nc4_test_files/${f}"
    echo "testing: $URL"
    if ! ../ncdump/ncdump ${URL} > ./results/${f}.dmp; then
        failure "${URL}"
    fi
    if test "x${TEST}" = x1 ; then
	if ! diff -wBb ${BASELINEH}/${f}.dmp ./results/${f}.dmp ; then
	    failure "diff ${f}.dmp"
	fi
    elif test "x${RESET}" = x1 ; then
	echo "${f}:" 
	cp ./results/${f}.dmp ${BASELINEH}/${f}.dmp
    fi
done

echo "*** Pass"
exit 0
