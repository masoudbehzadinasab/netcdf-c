#!/bin/sh

if test "x$srcdir" = "x"; then srcdir=`dirname $0`; fi; export srcdir

. ${srcdir}/test_common.sh

cd ${DMRTESTFILES}
F=`ls -1 *.dmr | sed -e 's/[.]dmr//g' | tr '\r\n' '  '`
cd $WD

CDL=
for f in ${F} ; do
STEM=`echo $f | cut -d. -f 1`
if test -f ${CDLTESTFILES}/${STEM}.cdl ; then
  CDL="${CDL} ${STEM}"
else
  echo "Not found: ${CDLTESTFILES}/${STEM}.cdl"
fi
done

if test "x${RESET}" = x1 ; then rm -fr ${BASELINE}/*.dmr.ncdump ; fi
for f in $F ; do
    if ! ./t_dmrmeta ${DMRTESTFILES}/${f}.dmr ./results/${f} ; then
        failure "./t_dmrmeta ${DMRTESTFILES}/${f}.dmr ./results/${f}"
    fi
    echo ncdump -h ./results/${f}
    ncdump -h ./results/${f} > ./results/${f}.dmr.ncdump
    if test "x${TEST}" = x1 ; then
	echo diff -wBb ${BASELINE}/${f}.dmr.ncdump ./results/${f}.dmr.ncdump
	if ! diff -wBb ${BASELINE}/${f}.dmr.ncdump ./results/${f}.dmr.ncdump ; then
	    failure "diff -wBb ${BASELINE}/${f}.ncdump ./results/${f}.dmr.ncdump"
	fi
    elif test "x${RESET}" = x1 ; then
	echo "${f}:" 
	cp ./results/${f}.dmr.ncdump ${BASELINE}/${f}.dmr.ncdump
    fi
done

if test "x${CDLDIFF}" = x1 ; then
  for f in $CDL ; do
    echo "diff -wBb ${CDLTESTFILES}/${f}.cdl ./results/${f}.dmr.ncdump"
    rm -f ./tmp
    cat ${CDLTESTFILES}/${f}.cdl \
    cat >./tmp
    echo diff -wBbu ./tmp ./results/${f}.dmr.ncdump
    if ! diff -wBbu ./tmp ./results/${f}.dmr.ncdump ; then
	failure "${f}" 
    fi
  done
fi

finish

