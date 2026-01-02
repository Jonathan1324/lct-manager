#!/bin/sh
DIR=$1
PATTERN=$2
find "$DIR" -name "$PATTERN" | sed "s|$DIR/||"
