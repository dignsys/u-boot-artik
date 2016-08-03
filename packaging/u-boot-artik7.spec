Name: u-boot-artik7
Summary: Bootloader for Embedded boards based on ARM processor
Version: 2012.07
Release: 0
Group: System/Kernel
License: GPL-2.0+
ExclusiveArch: %{arm}
URL: http://www.denx.de/wiki/U-Boot
Source0: %{name}-%{version}.tar.bz2
Source1001: packaging/u-boot-artik7.manifest

%description
bootloader for Embedded boards based on ARM processor

%prep
%setup -q

%build
cp %{SOURCE1001} .
make distclean
make artik7_config

# Build tools
make  %{?_smp_mflags} HOSTSTRIP=/bin/true tools

# Build image
make %{?_smp_mflags} EXTRAVERSION=`echo %{vcs} | sed 's/.*u-boot.*#\(.\{9\}\).*/-g\1-TIZEN.org/'`

# Generate params.bin
cp `find . -name "env_common.o"` copy_env_common.o
objcopy -O binary --only-section=.rodata `find . -name "copy_env_common.o"`
tr '\0' '\n' < copy_env_common.o > default_envs.txt
tools/mkenvimage -s 16384 -o params.bin default_envs.txt
rm copy_env_common.o default_envs.txt

%install
rm -rf %{buildroot}

# u-boot installation
mkdir -p %{buildroot}/boot/u-boot
install -d %{buildroot}/boot/u-boot
install -m 755 u-boot.bin %{buildroot}/boot/u-boot
install -m 755 params.bin %{buildroot}/boot/u-boot

%clean

%files
%manifest u-boot-artik7.manifest
%defattr(-,root,root,-)
/boot/u-boot
