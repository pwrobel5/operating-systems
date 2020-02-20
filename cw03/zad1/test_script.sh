#!/bin/bash

echo "***********TESTING CW03 DIRECTORIES**********"
echo ""

echo "nftw for test_dir/dd"
./cw03_directories.x test_dir/dd nftw
echo "" 

echo "system functions for ~/Pulpit"
./cw03_directories.x ~/Pulpit other
echo ""

echo "system functions for ."
./cw03_directories.x . other
echo ""

echo "nftw for .."
./cw03_directories.x .. nftw

echo ""
echo "***********TESTING CW03 DIRECTORIES ENDED**********"
