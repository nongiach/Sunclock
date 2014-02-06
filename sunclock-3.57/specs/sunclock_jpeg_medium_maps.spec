%define name sunclock_jpeg_medium_maps
%define version 1.0
%define release 1
%define prefix /usr

Summary: Medium resolution maps package for Sunclock.
Summary(fr): Ensemble de cartes résolution moyenne pour Sunclock.
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
the "map window". This file provides a lot of earth maps with different
definition for Sunclock.

%description -l fr
Sunclock affiche une carte de la Terre, et présente quelle portion est 
illuminée par le soleil. Il peut passer d'un aspect horloge carte 
miniature à une carte complete de la terre. Ce fichier fournit un ensemble
de cartes de la terre pour Sunclock.

%prep
rm -rf $RPM_BUILD_ROOT
%setup -q

%build


# %install
mkdir -p $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium
install  jpeg/medium/altitude.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/altitude.jpg
install  jpeg/medium/biomap.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/biomap.jpg
install  jpeg/medium/caida.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/caida.jpg
install  jpeg/medium/depthmap.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/depthmap.jpg
install  jpeg/medium/elevation.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/elevation.jpg
install  jpeg/medium/photo1.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/photo1.jpg
install  jpeg/medium/photo2.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/photo2.jpg
install  jpeg/medium/rainmap.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/rainmap.jpg
install  jpeg/medium/temperature.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/temperature.jpg
chmod 0644 $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/medium/*

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0644)
%{prefix}/share/sunclock/earthmaps/jpeg/medium/altitude.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/medium/biomap.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/medium/caida.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/medium/depthmap.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/medium/elevation.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/medium/photo1.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/medium/photo2.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/medium/rainmap.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/medium/temperature.jpg

%changelog
* Tue Oct 12 2001 Francois Massonneau <frmas@free.fr> 1.0-1
- First release
