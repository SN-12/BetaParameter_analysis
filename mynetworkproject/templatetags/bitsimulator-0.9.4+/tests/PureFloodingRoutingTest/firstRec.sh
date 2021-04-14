#!/bin/bash
base_dir='/home/snouri/Downloads/mynetworkproject/mynetworkproject/templatetags/bitsimulator-0.9.4+/tests/PureFloodingRoutingTest'
grep ^1 $base_dir/events.log>$base_dir/receive.log

rm -rf $base_dir/extractFile.txt
file=$base_dir/'receive.log'
while read line; do
arr=($line)
#echo $line
echo ${arr[1]} ${arr[2]} >> $base_dir/extractFile.txt
done < $file

#sentTime= cat $base_dir/events.log|grep ^0|head -1|cut -d' ' -f2
sentTime=$1

rm -rf newFile.txt
awk -v myvar=$sentTime '{print $1,$2,$1-myvar}' $base_dir/extractFile.txt >> $base_dir/newFile.txt
awk '!a[$2]++' $base_dir/newFile.txt>$base_dir/targetFile.txt
 awk '{print $2,$3}' $base_dir/targetFile.txt > $base_dir/DelayPerNode.txt
# run:./firsrRec.sh

