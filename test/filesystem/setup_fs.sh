#!/bin/bash

./teardown_fs.sh
dd if=/dev/zero of=/var/lib/virtualblock.img bs=512 count=1048576
losetup /dev/loop0 /var/lib/virtualblock.img
#rm -f mapping.txt
#python3 make_sector_mapping_table.py -p .001 -b 1 > mapping.txt
cat mapping.txt | dmsetup create errdev0
mkfs.ext4 /dev/mapper/errdev0
mkdir /tmp/bad_fs
mount /dev/mapper/errdev0 /tmp/bad_fs
chown irods:irods /tmp/bad_fs
