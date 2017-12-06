#!/bin/bash

echo "Fusing bootloader binaries..."
sudo fastboot flash partmap ./partmap_artik530s_tizen_emmc.txt
sudo fastboot flash 2ndboot ./bl1-emmcboot.img
sudo fastboot flash loader ./loader-emmc.img
sudo fastboot flash blmon ./bl_mon.img
sudo fastboot flash bootloader ./bootloader.img
sudo fastboot flash env ./tizen_params.bin
sudo fastboot flash boot ./boot.img

sudo fastboot reboot

echo "Fusing done"
echo "You have to resize the rootfs after first booting"
