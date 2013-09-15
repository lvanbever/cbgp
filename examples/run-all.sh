#!/bin/bash

CBGP=../src/cbgp

for f in example-*.cli; do
	$CBGP -c $f
	echo ""
done
