#!/bin/bash

rm -rf /tmp/bad_fs/*
umount /tmp/bad_fs
rm -rf /tmp/bad_fs
dmsetup remove /dev/mapper/errdev0
losetup -d /dev/loop0
