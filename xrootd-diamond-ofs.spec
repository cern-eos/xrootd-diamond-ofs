%define _unpackaged_files_terminate_build 0
Name:		xrootd-diamond-ofs
Version:	0.1.1
Release:	1
Summary:	EOS Diamond XRootD OFS plugin 
Prefix:         /usr
Group:		Applications/File
License:	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Source:        %{name}-%{version}-%{release}.tar.gz
BuildRoot:     %{_tmppath}/%{name}-root

BuildRequires: cmake >= 2.6
BuildRequires: xrootd4-server-devel >= 4.0
BuildRequires: xrootd4-private-devel >= 4.0
BuildRequires: zlib zlib-devel

%if 0%{?rhel} >= 6 || %{?fedora}%{!?fedora:0}
%if %{?fedora}%{!?fedora:0} >= 18
BuildRequires: gcc, gcc-c++
%else
BuildRequires: gcc, gcc-c++
%endif
%else
BuildRequires: gcc44, gcc44-c++
%endif

Requires: xrootd4 >= 4.0 
Requires: zlib


%description
EOS Diamond XRootD OFS plugin

%prep
%setup -n %{name}-%{version}-%{release}

%build
test -e $RPM_BUILD_ROOT && rm -r $RPM_BUILD_ROOT
%if 0%{?rhel} < 6
export CC=/usr/bin/gcc44 CXX=/usr/bin/g++44
%endif

mkdir -p build
cd build
cmake ../ -DRELEASE=%{release} -DCMAKE_BUILD_TYPE=RelWithDebInfo
%{__make} %{_smp_mflags} 

%install
cd build
%{__make} install DESTDIR=$RPM_BUILD_ROOT
echo "Installed!"

%post 
/sbin/ldconfig

%postun
/sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
/usr/lib64/libdiamond_ofs.so
/usr/lib64/libdiamond_ofs.so



