#!/bin/bash

echo "Fusing ARTIK710 OS_3.0.0 bootloader binaries..."
sudo fastboot flash partmap ./partmap_artik710_tizen_emmc.txt
sudo fastboot flash 2ndboot ./bl1-emmcboot.img
sudo fastboot flash fip-loader ./fip-loader-emmc.img
sudo fastboot flash fip-nonsecure ./fip-nonsecure.img
sudo fastboot flash env ./tizen_params.bin
sudo fastboot flash boot ./boot.img

sudo fastboot reboot

echo "Fusing done"
echo "You have to resize the rootfs after first booting"