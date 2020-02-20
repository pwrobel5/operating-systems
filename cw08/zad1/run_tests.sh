#!/bin/bash

threads="1 2 4 8"
filters="./filters/identity.txt ./filters/edge.txt ./filters/gaussian_blur.txt ./filters/30_filter.txt ./filters/65_filter.txt"
image="dragon.ascii.pgm"
output="out.ascii.pgm"

echo "***********TESTING CW08 FILTERING**********" > Times.txt
echo "" >> Times.txt

for filter in $filters
do

echo "Test for filter ${filter//"./filters/"/}" >> Times.txt
echo "" >> Times.txt

for thread_num in $threads
do

./cw08_filter.x $thread_num block $image $filter "${thread_num///}_${filter//"./filters/"/}_block_$output" >> Times.txt
echo "" >> Times.txt
./cw08_filter.x $thread_num interleaved $image $filter "${thread_num///}_${filter//"./filters/"/}_interleaved_$output" >> Times.txt
echo "" >> Times.txt

done

echo "Test for filter ${filter//"./filters/"/} ended" >> Times.txt
echo "" >> Times.txt

done

echo "" >> Times.txt
echo "***********TESTING CW03 RESOURCES ENDED**********" >> Times.txt
