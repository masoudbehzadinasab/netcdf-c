if test "x$srcdir" = "x" ; then srcdir=`dirname $0`; fi; export srcdir

if test $# = 0 ; then
TEST=1
else
for arg in "$@"; do
  case "${arg}" in
  test) TEST=1 ;;
  reset) RESET=1 ;;
  diff) DIFF=1 ;;
  cdl) CDLDIFF=1 ;;
  *) echo unknown argument $arg ;;
  esac
done
fi

set -e
if test "x$SETX" = x1 ; then echo "file=$0"; set -x ; fi

# Define input paths
WD=`pwd`
cd ${srcdir}/testfiles; TESTFILES=`pwd` ; cd ${WD}
cd ${srcdir}/daptestfiles; DAPTESTFILES=`pwd` ; cd ${WD}
cd ${srcdir}/dmrtestfiles; DMRTESTFILES=`pwd` ; cd ${WD}
cd ${srcdir}/cdltestfiles; CDLTESTFILES=`pwd` ; cd ${WD}
cd ${srcdir}/baseline; BASELINE=`pwd` ; cd ${WD}
cd ${srcdir}/baselineraw; BASELINERAW=`pwd` ; cd ${WD}
cd ${srcdir}/baselineremote; BASELINEREM=`pwd` ; cd ${WD}

rm -fr ./results
mkdir -p ./results

FAILURES=
failure() {
  echo "*** Fail: $1"
  FAILURES=1
  if test "x$2" != x ; then
    exit 1
  fi
}

PUSHD() {
pushd $1 >>/dev/null
}
POPD() {
popd >>/dev/null
}

filesexist() {
    for x in "$@" ; do
	if ! test -f $x ; then
	  failure "missing file: $x"
	fi
    done
}

finish() {
if test "x$FAILURES" = x1 ; then
echo "*** Fail"
exit 1
else
echo "*** Pass"
exit 0
fi
}
