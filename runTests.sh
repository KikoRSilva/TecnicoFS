!#/bin/bash

inputdir=$1
outputdir=$2
maxthread=$3

for input in $inputdir/*.txt
do
	for i in $(seq 1 $maxthread)
	do
		output="$((basename $input) | sed "s/\.[a-z]*//g")-$i"
		echo InputFile=$input NumThread=$i
		./tecnicofs $input $outputdir/$output.txt $i | sed -n '$p'
		./tecnicofs $input $outputdir/$output.txt $i > $outputdir/$output.txt
	done
done
