#!/bin/bash

for density in 0.1 1 10 100 1000 ; do
	sed "s/1.57/$density/" ../cliffh.poly > c.poly
	for i in `seq -90 1 0`; do
		../sim -d 0.2 -s 0.05 -x -20 -y 0.1 -a $i -f c.poly
	done > d$density.data
done

gnuplot plot.gpl
