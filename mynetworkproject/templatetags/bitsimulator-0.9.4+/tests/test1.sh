mkdir tests/test1
cd tests/test1
cp ../../$srcdir/tests/scenario.xml .
../../bitsimulator --awakenDuration 50000 --doNotUseNeighboursList
cmp ../../$srcdir/tests/expected-events.log results-events.log
ret=$?
rm results-*.log
rm -f scenario.xml
cd ..
rmdir test1
exit $ret
