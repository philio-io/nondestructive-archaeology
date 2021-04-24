#!/bin/bash

for dist in 0.04 0.1 0.2 0.3 1 ; do
	for i in `seq -90 1 0`; do
		../sim -d $dist -s 0.05 -x -20 -y 0.1 -a $i -f ../cliffh.poly
	done > d$dist.data
done

gnuplot plot.gpl
