#!/bin/bash

test_bin=cw04_tester.x
test_file1=plik.test
test_file2=plik2.test
test_pmin1=10
test_pmax1=20
test_bytes1=10
test_pmin2=3
test_pmax2=9
test_bytes2=6

list_file=lista.txt
monitoring_bin=cw04_monitor.x

echo "***********TESTING CW04 MONITOR**********"
echo ""

echo "Testing memory mode..."
echo ""

./$test_bin $test_file1 $test_pmin1 $test_pmax1 $test_bytes1 &
test_pid1=`ps | grep "$test_bin" | awk '{ print $1 }'`

./$test_bin $test_file2 $test_pmin2 $test_pmax2 $test_bytes2 &
test_pid2=`ps | grep "$test_bin" | awk '{ print $1 }' | tail -n1`

./$monitoring_bin $list_file

kill $test_pid1
kill $test_pid2


echo ""
echo "***********TESTING CW04 MONITOR ENDED**********"
