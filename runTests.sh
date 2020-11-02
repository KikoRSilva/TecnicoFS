!#/bin/bash

inputdir=$1
outputdir=$2
maxthread=$3

for input in $inputdir/*.txt
do
	for i in $(seq 1 $maxthread)
	do
		output="$input-$i"
		echo InputFile=$input NumThread=$i
		./tecnicofs $input $output $i
	done
done
