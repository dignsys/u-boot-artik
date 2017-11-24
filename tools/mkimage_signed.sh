#!/bin/bash

# U-BOOT binary signature tool
#
# Copyright (C) 2014 Samsung Electronics
# Przemyslaw Marczak <p.marczak@samsung.com>


# Sign header:
#{
# uint32_t magic;	/* image magic number */
# uint32_t size;	/* image data size */
# uint32_t valid;	/* valid flag */
# char date[12];	/* image creation timestamp - YYMMDDHH */
# char version[24];	/* image version */
# char bd_name[16];	/* target board name */
# char reserved[448];	/* reserved */
#}

INPUT_ARGS=2
MAX_ARGS=3
INPUT_BIN=${1}
CONFIG=${2}
VERSION=${3}

OUTPUT_BIN="u-boot-mmc.bin"
OUTPUT_SIZE=$((1024*1024))

SIGN_HDR_SIZE=512
INPUT_SIZE_LIMIT=$((${OUTPUT_SIZE} - ${SIGN_HDR_SIZE}))

# Check arguments count
if [ $# -lt $INPUT_ARGS -a $# -gt $MAX_ARGS ]; then
	echo Bad arguments number!
	echo "Usage:"
	echo "./mkimage_signed.sh <input binary> <config> {version}"
	echo "	- {version} is optional"
	echo "e.g.:"
	echo "./mkimage_signed.sh u-boot-multi.bin tizen_config"
	exit
fi

echo "#####################################"
echo "Running script: $0"
echo "Config: $CONFIG"
echo "Input binary: $INPUT_BIN"

# Check if given binary exists
if [ -s $INPUT_BIN ]; then
	# Check given binary size
	INPUT_SIZE=`du -b $INPUT_BIN | awk '{print $1}'`

	if [ ${INPUT_SIZE} -gt ${INPUT_SIZE_LIMIT} ]; then
		echo "Input binary size exceeds size limit!"
		echo "Max input size: ${INPUT_SIZE_LIMIT}"
		exit
	else
		echo "Input bytes: $INPUT_SIZE (Max size: ${INPUT_SIZE_LIMIT} B)"
	fi
else
	echo "File: $INPUT_BIN not exists!"
	exit
fi

echo -n "BoOt" > sig-magic
echo -n `date +%Y%m%d%H` > sig-date
if [ $VERSION ]; then
	echo -n $VERSION > sig-version
else
	echo -n "none" > sig-version
fi

if [ $CONFIG == "tizen_defconfig" ]; then
	echo -n "slp_midasq" > sig-board
elif [ $CONFIG == "odroid-xu3_defconfig" ]; then
	echo -n "odroid_xu3" > sig-board
elif [ $CONFIG == "artik530_raptor_config" ]; then
	echo -n "artik530_raptor" > sig-board
	OUTPUT_BIN="u-boot.bin"
	INPUT_SIZE_LIMIT=$((${OUTPUT_SIZE} - ${SIGN_HDR_SIZE} - 512))
elif [ $CONFIG == "artik710_raptor_config" ]; then
	echo -n "artik710_raptor" > sig-board
	INPUT_SIZE_LIMIT=$((${OUTPUT_SIZE} - ${SIGN_HDR_SIZE} - 1120))
	OUTPUT_BIN="u-boot.bin"
else
	echo -n "slp_u1" > sig-board
fi

cat sig-magic /dev/zero | head -c 12 > sig-tmp
cat sig-tmp sig-date /dev/zero | head -c 24 > sig-tmp2
cat sig-tmp2 sig-version /dev/zero | head -c 48 > sig-tmp
cat sig-tmp sig-board /dev/zero | head -c 512 > sig-hdr
cat $INPUT_BIN /dev/zero | head -c $INPUT_SIZE_LIMIT > u-boot-pad.bin
cat u-boot-pad.bin sig-hdr > $OUTPUT_BIN

echo
echo "Header info:"
echo "HDR length:   $SIGN_HDR_SIZE Bytes"
echo "SIG magic:   \"`cat sig-magic`\""
echo "SIG size:     0"
echo "SIG valid:    0"
echo "SIG date:    \"`cat sig-date`\" (YYMMDDHH)"
echo "SIG version: \"`cat sig-version`\""
echo "SIG board:   \"`cat sig-board`\""

rm -f sig-* u-boot-pad.bin

echo
echo "Output signed binary: ${OUTPUT_BIN}"
echo "#####################################"

