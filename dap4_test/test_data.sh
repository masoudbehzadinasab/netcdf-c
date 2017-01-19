#!/bin/sh

if test "x$srcdir" = "x"; then srcdir=`dirname $0`; fi; export srcdir

. ${srcdir}/test_common.sh

cd ${DAPTESTFILES}
F=`ls -1 *.dap | sed -e 's/[.]dap//g' | tr '\r\n' '  '`
cd $WD

if test "x${RESET}" = x1 ; then rm -fr ${BASELINE}/*.dap.ncdump ; fi
for f in $F ; do
    echo "testing: ${f}"
    if ! ${VG} ./test_data ${DAPTESTFILES}/${f} ./results/${f}.nc ; then
        failure "./test_data ${DAPTESTFILES}/${f} ./results/${f}.nc"
    fi
    ../ncdump/ncdump ./results/${f}.nc > ./results/${f}.dap.ncdump
    if test "x${TEST}" = x1 ; then
	echo diff -wBb ${BASELINE}/${f}.dap.ncdump ./results/${f}.dap.ncdump 
	if ! diff -wBb ${BASELINE}/${f}.dap.ncdump ./results/${f}.dap.ncdump ; then
	    failure "diff -wBb ${BASELINE}/${f}.dap.ncdump ./results/${f}.dap.ncdump"
	fi
    elif test "x${RESET}" = x1 ; then
	echo "${f}:" 
	cp ./results/${f}.dap.ncdump ${BASELINE}/${f}.dap.ncdump
    fi
done

# Remove empty lines and trim lines in a cdl file
trim() {
  if test $# != 2 ; then
    echo "simplify: too few args"
  else
    rm -f $2
    while read -r iline; do
      oline=`echo $iline | sed -e 's/^[\t ]*\([^\t ]*\)[\t ]*$/\\1/'`
      if test "x$oline" = x ; then continue ; fi
      echo "$oline" >> $2
    done < $1
  fi
}

# Do cleanup on the baseline file
baseclean() {
  if test $# != 2 ; then
    echo "simplify: too few args"
  else
    rm -f $2
    while read -r iline; do
      oline=`echo $iline | tr "'" '"'`
      echo "$oline" >> $2
    done < $1
  fi
}

# Do cleanup on the result file
resultclean() {
  if test $# != 2 ; then
    echo "simplify: too few args"
  else
    rm -f $2
    while read -r iline; do
      oline=`echo $iline | sed -e 's|^\(netcdf.*\)[.]nc\(.*\)$|\\1\\2|'`
      echo "$oline" >> $2
    done < $1
  fi
}

if test "x${CDLDIFF}" = x1 ; then
  for f in $F ; do
    STEM=`echo $f | cut -d. -f 1`
    if ! test -f ${CDLTESTFILES}/${STEM}.cdl ; then
      echo "Not found: ${CDLTESTFILES}/${STEM}.cdl"
      continue
    fi
    echo "diff -wBb ${CDLTESTFILES}/${STEM}.cdl ./results/${f}.dap.ncdump"
    rm -f ./b1 ./b2 ./r1 ./r2
    trim ${CDLTESTFILES}/${STEM}.cdl ./b1
    trim ./results/${f}.dap.ncdump ./r1
    baseclean b1 b2
    resultclean r1 r2  
    if ! diff -wBb ./b2 ./r2 ; then
	failure "${f}" 
    fi
  done
fi

finish

