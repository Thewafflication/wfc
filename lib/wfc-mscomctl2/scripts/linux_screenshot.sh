#!/bin/bash
# Photographs the display while a program runs (under xvfb-run).
# Usage: linux_screenshot.sh <out.png> <seconds to wait> <command ...>
# This file is identical in every WFC library that has one.
out=$1
delay=$2
shift 2
"$@" &
pid=$!
sleep "$delay"
import -window root "$out"
kill "$pid" 2>/dev/null
wineserver -k 2>/dev/null
exit 0
