#!/bin/bash

catcher=cw04_catcher.x
sender=cw04_sender.x
signal_num=20

echo "***********TESTING CW04 SENDER AND CATCHER**********"
echo ""

echo "Testing kill mode"
echo ""

./$catcher kill_m &
pid=`ps | grep "$catcher" | awk '{ print $1 }'`

./$sender $pid $signal_num kill_m

echo ""
echo "Testing kill mode ended"

echo "Testing sigqueue mode"
echo ""

./$catcher sigqueue_m &
pid=`ps | grep "$catcher" | awk '{ print $1 }'`

./$sender $pid $signal_num sigqueue_m

echo ""
echo "Testing sigqueue mode ended"

echo "Testing sigrt mode"
echo ""

./$catcher sigrt_m &
pid=`ps | grep "$catcher" | awk '{ print $1 }'`

./$sender $pid $signal_num sigrt_m

echo ""
echo "Testing sigrt mode ended"

echo ""
echo "***********TESTING CW04 SENDER AND CATCHER ENDED**********"
