Name: u-boot-artik530
Summary: Bootloader for Embedded boards based on ARM processor
Version: 2016.01
Release: 0
Group: System/Kernel
License: GPL-2.0+
ExclusiveArch: %{arm}
URL: http://www.denx.de/wiki/U-Boot
Source0: %{name}-%{version}.tar.bz2

BuildRequires: u-boot-tools

%description
bootloader for Embedded boards based on ARM processor

%package -n u-boot-artik533s
Summary: A bootloader for ARTIK533S board
Group: System/Kernel

%description -n u-boot-artik533s
bootloader for ARTIK533S Embedded boards based on ARM processor

%define os_version 3.0.0

%prep
%setup -q

%build

# Set configuration
make artik530_raptor_legacy_config

# Build tools
make  %{?_smp_mflags} HOSTSTRIP=/bin/true tools

# Build image
make %{?_smp_mflags} EXTRAVERSION=`echo %{vcs} | sed 's/.*u-boot.*#\(.\{9\}\).*/-g\1-TIZEN.org/'`

# Generate params.bin
cp `find . -name "env_common.o"` copy_env_common.o
objcopy -O binary --only-section=.rodata.default_environment `find . -name "copy_env_common.o"`
tr '\0' '\n' < copy_env_common.o > default_envs.txt
tools/mkenvimage -s 16384 -o params.bin default_envs.txt
rm copy_env_common.o default_envs.txt

# Sign u-boot.bin - output is: u-boot.bin
chmod 755 tools/mkimage_signed.sh
./tools/mkimage_signed.sh u-boot.bin artik530_raptor_config %{os_version}

# gen_nexell_image
tools/nexell/SECURE_BINGEN \
		-c S5P4418 -t 3rdboot \
		-n tools/nexell/nsih/raptor-emmc.txt \
		-i u-boot.bin \
		-o bootloader.img \
		-l 0x43c00000 -e 0x43c00000

%install
rm -rf %{buildroot}

# u-boot installation
mkdir -p %{buildroot}/boot/u-boot
install -d %{buildroot}/boot/u-boot
install -m 755 bootloader.img %{buildroot}/boot/u-boot/nonsigned-bootloader.img
install -m 755 ./scripts/tizen/artik530s/os_3.0.0/bootloader.img %{buildroot}/boot/u-boot
install -m 755 params.bin %{buildroot}/boot/u-boot

# u-boot artik533 installation
mkdir -p %{buildroot}/boot/u-boot-artik533s
install -d %{buildroot}/boot/u-boot-artik533s
install -m 755 ./scripts/tizen/artik533s/os_3.2.0/bootloader.img %{buildroot}/boot/u-boot-artik533s
install -m 755 ./scripts/tizen/artik533s/os_3.2.0/tizen_params.bin %{buildroot}/boot/u-boot-artik533s/params.bin

%clean

%files
%defattr(-,root,root,-)
/boot/u-boot

%files -n u-boot-artik533s
%defattr(-,root,root,-)
/boot/u-boot-artik533s
