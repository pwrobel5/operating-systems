#!/bin/bash

test_bin=cw03_tester.x
test_file=plik.test
test_pmin=10
test_pmax=20
test_bytes=10
testing_time=120

list_file=lista.txt
monitoring_time=110
monitoring_bin=cw03_monitor.x

echo "***********TESTING CW03 MONITOR**********"
echo ""

echo "Testing memory mode..."
echo ""

./$test_bin $test_file $test_pmin $test_pmax $test_bytes &
test_pid=`ps | grep "$test_bin" | awk '{ print $1 }'`
./$monitoring_bin $list_file $monitoring_time memory_mode &
sleep $testing_time
kill $test_pid

echo ""
echo "Testing memory mode ended"
echo ""
rm -r -f archive
echo ""
echo "Testing cp mode"
echo ""

./$test_bin $test_file $test_pmin $test_pmax $test_bytes &
test_pid=`ps | grep "$test_bin" | awk '{ print $1 }'`
./$monitoring_bin $list_file $monitoring_time cp_mode &
sleep $testing_time
kill $test_pid

echo "Testing cp mode ended"
echo ""

echo ""
echo "***********TESTING CW03 MONITOR ENDED**********"
