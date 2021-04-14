#!/bin/bash
grep ^1 events.log>receive.log
rm -rf extractFile.txt
file='receive.log'
while read line; do
arr=($line)
#echo $line
echo ${arr[1]} ${arr[2]} >> extractFile.txt
done < $file

#sentTime= cat events.log|grep ^0|head -1|cut -d' ' -f2
sentTime=$1
dd=100
rm -rf newFile.txt
awk -v myvar=$sentTime '{print $1,$2,$1-myvar}' extractFile.txt >> newFile.txt
awk '!a[$2]++' newFile.txt>targetFile.txt
 awk '{print $2,$3}' targetFile.txt > DelayPerNode.txt
# run:./firsrRec.sh
