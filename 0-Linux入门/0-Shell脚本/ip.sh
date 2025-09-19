#!/bin/bash

ifconfig=$(ifconfig ens33)

IFS=$'\n'
cnt=0
for line in $ifconfig; do
	#let cnt+=1
	#echo $cnt
	#echo $line	
	if [[ "$line" == *"inet "* ]]; then
		echo ${line:13:15}
	fi
done
