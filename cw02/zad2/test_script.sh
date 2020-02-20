#!/bin/bash

echo "***********TESTING CW02 DIRECTORIES**********"
echo ""

./cw02_directories.x test_dir/dd/ ">" 01-02-2016 nftw

./cw02_directories.x /home/piotr/Pulpit/ "=" 18-02-2019 other

./cw02_directories.x ./ "=" 03-04-2019 other

./cw02_directories.x ../ "<" 22-03-2019 nftw

echo ""
echo "***********TESTING CW02 DIRECTORIES ENDED**********"