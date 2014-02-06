%define name sunclock_huge_earthmap
%define version 1.0
%define release 1
%define prefix /usr

Summary: The 11 Mb JPG map for Sunclock, the sophisticated clock.
Summary(fr): La carte de 11 Mb pour le programme Sunclock.
Name: %{name}
Version: %{version}
Release: %{release}
Serial: 0
Copyright: GPL
Group: Sciences/Astronomy
# Source: ftp://ftp.ac-grenoble.fr/ge/geosciences/%{name}-%{version}%{release}.tgz
Source: http://frmas.free.fr/files/%{name}-%{version}.tgz
URL: http://frmas.free.fr/li_1.htm
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}
# Requires: sunclock

%description
Sunclock displays a map of the Earth and shows which portion is illuminated
by the sun. It can commute between two states, the "clock window" and
the "map window". This file provides a huge jpg earth map (11mb).

%description -l fr
Sunclock affiche une carte de la Terre, et présente quelle portion est 
illuminée par le soleil. Il peut passer d'un aspect horloge carte 
miniature à une carte complete de la terre. Ce fichier fournit une superbe
carte de plus de 11mb.

%prep
rm -rf $RPM_BUILD_ROOT
%setup -q

%build


# %install
mkdir -p $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/large
install  sunclock_huge_earthmap.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/large/sunclock_huge_earthmap.jpg
chmod 0644 $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/large/sunclock_huge_earthmap.jpg

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0644)
%{prefix}/share/sunclock/earthmaps/large/sunclock_huge_earthmap.jpg

%changelog
* Tue Jun 12 2001 Francois Massonneau <frmas@free.fr>
- First release
