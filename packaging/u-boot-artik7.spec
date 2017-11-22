Name: u-boot-artik7
Summary: Bootloader for Embedded boards based on ARM processor
Version: 2016.01
Release: 0
Group: System/Kernel
License: GPL-2.0+
ExclusiveArch: aarch64
URL: http://www.denx.de/wiki/U-Boot
Source0: %{name}-%{version}.tar.bz2
Source1001: packaging/u-boot-artik7.manifest

BuildRequires: u-boot-tools

%description
bootloader for Embedded boards based on ARM processor

%package -n nexell-tools
Summary: tools to create nexell soc loadable image
Group: System/Kernel

%description -n nexell-tools
This package includes SECURE_BINGEN and config files for the tool to create
image with nexell format.

%prep
%setup -q

%build
cp %{SOURCE1001} .

# Set configuration
make artik710_raptor_legacy_config

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
./tools/mkimage_signed.sh u-boot.bin artik710_raptor_config

# gen_fip_image
tools/fip_create/fip_create --dump --bl33 u-boot.bin fip-nonsecure.bin

# gen_nexell_image
tools/nexell/SECURE_BINGEN \
		-c S5P6818 -t 3rdboot \
		-n tools/nexell/nsih/raptor-64.txt \
		-i fip-nonsecure.bin \
		-o fip-nonsecure.img \
		-l 0x7df00000 -e 0x00000000

%install
rm -rf %{buildroot}

# u-boot installation
mkdir -p %{buildroot}/boot/u-boot
install -d %{buildroot}/boot/u-boot
install -m 755 fip-nonsecure.img %{buildroot}/boot/u-boot
install -m 755 params.bin %{buildroot}/boot/u-boot
mkdir -p %{buildroot}/boot/tools
install -d %{buildroot}/boot/tools
install -m 755 tools/nexell/SECURE_BINGEN %{buildroot}/boot/tools
install -m 755 tools/nexell/nsih/raptor-64.txt %{buildroot}/boot/tools

%clean

%files
%manifest u-boot-artik7.manifest
%defattr(-,root,root,-)
/boot/u-boot

%files -n nexell-tools
/boot/tools
