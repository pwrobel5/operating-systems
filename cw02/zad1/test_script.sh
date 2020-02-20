#!/bin/bash

out_file="wyniki.txt"
recordsizes=( 1 4 512 1024 4096 8192 )
recordnumbers=( 500 5000 )

echo "***********TESTING CW02 PROGRAM**********" > ${out_file}
echo "" >> ${out_file}

for recordsize in ${recordsizes[@]}
do

    echo "Testing for records of size: ${recordsize}"

    for recordnumber in ${recordnumbers[@]}
    do
        echo "Number of records: ${recordnumber}"
        binary_name=${recordnumber}-${recordsize}.b
        ./cw02_program.x generate ${binary_name} ${recordnumber} ${recordsize} >> ${out_file}

        ./cw02_program.x copy ${binary_name} copy-lib-${binary_name} ${recordnumber} ${recordsize} lib >> ${out_file}
        ./cw02_program.x copy ${binary_name} copy-sys-${binary_name} ${recordnumber} ${recordsize} sys >> ${out_file}

        ./cw02_program.x sort ${binary_name} ${recordnumber} ${recordsize} lib >> ${out_file}
        ./cw02_program.x sort copy-sys-${binary_name} ${recordnumber} ${recordsize} sys >> ${out_file}
    done

done

echo "" >> ${out_file}
echo "***********TESTING CW02 PROGRAM ENDED**********" >> ${out_file}