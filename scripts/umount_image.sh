#!/usr/bin/env bash

loopdev=$(cat loopdev.txt)
sudo losetup -d ${loopdev}
umount mount
