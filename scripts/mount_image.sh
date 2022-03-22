#!/usr/bin/env bash

loopdev=$(cat loopdev.txt)
sudo chmod a+rw ${loopdev}

if [ "$1" = "debug" ]
then
	./build/chisai -d ${loopdev} mount
else
	./build/chisai ${loopdev} mount
fi
