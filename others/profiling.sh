#!/bin/bash
perf record -g --call-graph=dwarf $@    # run program
perf script -i perf.data &> perf.unfold
./stackcollapse-perf.pl perf.unfold &> perf.fold
./flamegraph.pl perf.fold > perf.svg