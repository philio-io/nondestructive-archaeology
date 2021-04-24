#!/bin/bash

for c in cube cliff cliffh earth pyramidh pyramid ; do
	for i in `seq -90 1 0`; do
		../sim -d 0.2 -s 0.05 -x -20 -y 0.1 -a $i -f ../$c.poly
	done > $c.data
done

gnuplot plot.gpl
