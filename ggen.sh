#!/bin/bash

help_msg="Generate a png image of the graph
Usage:
	./ggen.sh in.dot out.png
- in.dot is the textual representation of the graph in the dot syntax
- out.png the output image"


if [[ $# == 0 || "$1" == "-h" || "$1" == "--help" ]]; then
	echo "$help_msg"
    fi

if [ $# -eq 2 ]; then
    sfdp -x -Goverlap=scale -Tpng "$1" > "$2"
fi
