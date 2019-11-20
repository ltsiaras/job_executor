#!/bin/bash

arg1="issuejob"

for file
	do
	while read line
		do
			./jobCommander $arg1 $line
	done < $file
done
