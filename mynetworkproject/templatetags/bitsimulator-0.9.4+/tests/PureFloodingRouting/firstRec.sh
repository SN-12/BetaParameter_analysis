#!/bin/bash
#grep ^1 events.log>receive.log
rm -rf extractFile.txt
file='receive.log'
while read line; do
arr=($line)
#echo $line
echo ${arr[1]} ${arr[2]} >> extractFile.txt
done < $file

rm -rf newFile.txt
awk '{print $1,$2,$1-6000040081438}' extractFile.txt >> newFile.txt
awk '!a[$2]++' newFile.txt>targetFile.txt
 awk '{print $2,$3}' targetFile.txt > DelayPerNode.txt
# run:./firsrRec.sh
