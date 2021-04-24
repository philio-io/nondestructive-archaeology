#!/bin/bash

for dist in 10 20 60 100 150 200 ; do
	for i in `seq -90 1 0`; do
		../sim -d 0.2 -s 0.05 -x -$dist -y 0.1 -a $i -f ../cliffh.poly
	done > $dist.data
done

gnuplot plot.gpl
