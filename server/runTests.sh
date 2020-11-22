#!/bin/bash

inputdir=$1
outputdir=$2
maxthread=$3

# check if there are only 3 arguments
if [ ! $# == 3 ] 
	then
		echo "Error: Invalid number of arguments, must be 4 arguments."
		exit 1
fi
# check if arg 1 and 2 are directories
if [ ! -d "$inputdir" ] 
	then
		echo "Error: Invalid input directory."
		exit 1
fi
if [ ! -d "$outputdir" ] 
	then
		echo "Error: Invalid output directory."
		exit 1
fi
# check the maxthreads parameter is not 0 or less
if [ ! $maxthread -gt 0 ] 
	then
		echo "Error: Invalid number, MaxThreads must be greater than 0."
		exit 1
fi

for input in $inputdir/*.txt
do
	for i in $(seq 1 $maxthread)
	do
		output="$((basename $input) | sed "s/\.[a-z]*//g")-$i"
		echo InputFile=$input NumThread=$i
		./tecnicofs $input $outputdir/$output.txt $i | tail -1
	done
done
