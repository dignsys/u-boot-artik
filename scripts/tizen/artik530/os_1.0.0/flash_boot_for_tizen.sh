#!/bin/bash

echo "Fusing bootloader binaries..."
sudo fastboot flash partmap ./partmap_artik530_tizen_emmc.txt
sudo fastboot flash 2ndboot ./bl1-emmcboot.img
sudo fastboot flash bootloader ./bootloader.img
sudo fastboot flash env ./tizen_params.bin

sudo fastboot reboot

echo "Fusing done"
echo "You have to resize the rootfs after first booting"